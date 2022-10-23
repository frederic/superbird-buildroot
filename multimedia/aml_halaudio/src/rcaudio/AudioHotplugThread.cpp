/*
**
** Copyright 2012, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**  @author   Hugo Hong
**  @version  1.0
**  @date     2018/04/01
**  @par function description:
**  - 1 bluetooth rc audio device hot plug thread
*/

#define LOG_TAG "AudioHAL:AudioHotplugThread"
#include <utils/log.h>

#include <assert.h>
#include <dirent.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sound/asound.h>

#include <utils/misc.h>
#include <utils/String8.h>

#include "AudioHotplugThread.h"

#include <linux/input.h>
#include <linux/hidraw.h>
#include "huitong_audio.h"

// This name is used to recognize the AndroidTV Remote mic so we can
// use it for voice recognition.
#define ANDROID_TV_REMOTE_AUDIO_DEVICE_NAME "ATVRAudio"
// Name of the virtual sound card created by the hid driver.
#ifndef DIA_REMOTE_AUDIO_DEVICE_ID
#define DIA_REMOTE_AUDIO_DEVICE_ID "DIAAudio"
#endif
#define AUDIO_HOTPLUG_THREAD_PRIORITY       (-16)
namespace android {

/*
 * ALSA parameter manipulation routines
 *
 * TODO: replace this when TinyAlsa offers a suitable API
 */
static inline int param_is_mask(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_interval *param_to_interval(
        struct snd_pcm_hw_params *p, int n)
{
    assert(p->intervals);
    if (!param_is_interval(n)) return NULL;
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    assert(p->masks);
    if (!param_is_mask(n)) return NULL;
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static inline void snd_mask_any(struct snd_mask *mask)
{
    memset(mask, 0xff, sizeof(struct snd_mask));
}

static inline void snd_interval_any(struct snd_interval *i)
{
    i->min = 0;
    i->openmin = 0;
    i->max = UINT_MAX;
    i->openmax = 0;
    i->integer = 0;
    i->empty = 0;
}

static void param_init(struct snd_pcm_hw_params *p)
{
    int n = 0;

    memset(p, 0, sizeof(*p));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
        struct snd_mask *m = param_to_mask(p, n);
        snd_mask_any(m);
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
        struct snd_interval *i = param_to_interval(p, n);
        snd_interval_any(i);
    }
    p->rmask = 0xFFFFFFFF;
}

/*
 * Hotplug thread
 */

const char* AudioHotplugThread::kThreadName = "ATVRemoteAudioHotplug";

// directory where device nodes appear
const char* AudioHotplugThread::kDeviceDir = "/dev";
// directory where ALSA device nodes appear
const char* AudioHotplugThread::kAlsaDeviceDir = "/dev/snd";


// filename suffix for ALSA nodes representing capture devices
const char  AudioHotplugThread::kDeviceTypeCapture = 'c';

AudioHotplugThread::AudioHotplugThread(Callback& callback)
    : mCallback(callback)
    , mShutdownEventFD(-1)
{
}

AudioHotplugThread::~AudioHotplugThread()
{
    if (mShutdownEventFD != -1) {
        ::close(mShutdownEventFD);
    }
}

bool AudioHotplugThread::start()
{
    mShutdownEventFD = eventfd(0, EFD_NONBLOCK);
    if (mShutdownEventFD == -1) {
        return false;
    }

    return (run(kThreadName) == NO_ERROR);
}

void AudioHotplugThread::shutdown()
{
    requestExit();
    uint64_t tmp = 1;
    ::write(mShutdownEventFD, &tmp, sizeof(tmp));
    join();
}

bool AudioHotplugThread::parseCaptureDeviceName(const char* name,
                                                unsigned int* card,
                                                unsigned int* device)
{
    char deviceType;
    int ret = sscanf(name, "pcmC%uD%u%c", card, device, &deviceType);
    return (ret == 3 && deviceType == kDeviceTypeCapture);
}

static inline void getAlsaParamInterval(const struct snd_pcm_hw_params& params,
                                        int n, unsigned int* min,
                                        unsigned int* max)
{
    struct snd_interval* interval = param_to_interval(
        const_cast<struct snd_pcm_hw_params*>(&params), n);
    *min = interval->min;
    *max = interval->max;
}

// This was hacked out of "alsa_utils.cpp".
static int s_get_alsa_card_name(char *name, size_t len, int card_id)
{
    int fd;
    int amt = -1;
    snprintf(name, len, "/proc/asound/card%d/id", card_id);
    fd = open(name, O_RDONLY);
    if (fd >= 0) {
        amt = read(fd, name, len - 1);
        if (amt > 0) {
            // replace the '\n' at the end of the proc file with '\0'
            name[amt - 1] = 0;
        }
        close(fd);
    }
    return amt;
}

bool AudioHotplugThread::getDeviceInfo(unsigned int pcmCard,
                                       unsigned int pcmDevice,
                                       DeviceInfo* info)
{
    bool result = false;
    int ret = 0;
    int len = 0;
    char cardName[64] = "";

    assert(info != NULL);

    memset(info, 0, sizeof(DeviceInfo));

    String8 devicePath = String8::format("%s/pcmC%dD%d%c",
            kAlsaDeviceDir, pcmCard, pcmDevice, kDeviceTypeCapture);

    ALOGD("AudioHotplugThread::getDeviceInfo opening %s", devicePath.string());
    int alsaFD = open(devicePath.string(), O_RDONLY);
    if (alsaFD == -1) {
        ALOGE("AudioHotplugThread::getDeviceInfo open failed for %s", devicePath.string());
        goto done;
    }

    // query the device's ALSA configuration space
    struct snd_pcm_hw_params params;
    param_init(&params);
    ret = ioctl(alsaFD, SNDRV_PCM_IOCTL_HW_REFINE, &params);
    if (ret == -1) {
        ALOGE("AudioHotplugThread: refine ioctl failed");
        goto done;
    }

    info->pcmCard = pcmCard;
    info->pcmDevice = pcmDevice;
    getAlsaParamInterval(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
                         &info->minSampleBits, &info->maxSampleBits);
    getAlsaParamInterval(params, SNDRV_PCM_HW_PARAM_CHANNELS,
                         &info->minChannelCount, &info->maxChannelCount);
    getAlsaParamInterval(params, SNDRV_PCM_HW_PARAM_RATE,
                         &info->minSampleRate, &info->maxSampleRate);

    // Ugly hack to recognize Remote mic and mark it for voice recognition
    info->forVoiceRecognition = false;
    len = s_get_alsa_card_name(cardName, sizeof(cardName), pcmCard);
    ALOGD("AudioHotplugThread get_alsa_card_name returned %d, %s", len, cardName);
    if (len > 0) {
        if (strcmp(DIA_REMOTE_AUDIO_DEVICE_ID, cardName) == 0) {
            ALOGD("AudioHotplugThread found Android TV remote mic on Card %d, for VOICE_RECOGNITION", pcmCard);
            info->forVoiceRecognition = true;
        }
    }

    result = info->forVoiceRecognition;

done:
    if (alsaFD != -1) {
        close(alsaFD);
    }
    return result;
}

bool AudioHotplugThread::getDeviceInfo(const unsigned int hidrawIndex,
                                       DeviceInfo* info)
{
    bool result = false;
    struct hidraw_devinfo devInfo;
    char devicePath[32] = {0};
    int fd = -1;

    assert(info != NULL);

    memset(info, 0, sizeof(DeviceInfo));

    sprintf(devicePath, "/dev/hidraw%d", hidrawIndex);

    //check hidraw
    if (0 != access(devicePath, F_OK)) {
        //ALOGE("%s could not access %s, %s\n", __FUNCTION__, devicePath, strerror(errno));
        goto done;
    }

    fd = open(devicePath, O_RDWR|O_NONBLOCK);
    if (fd <= 0) {
        //ALOGE("%s could not open %s, %s\n", __FUNCTION__, devicePath, strerror(errno));
        goto done;
    }
    memset(&devInfo, 0, sizeof(struct hidraw_devinfo));
    if (ioctl(fd, HIDIOCGRAWINFO, &devInfo)) {
        goto done;
    }

    ALOGD("%s info.bustype:0x%x, info.vendor:0x%x, info.product:0x%x\n",
        __FUNCTION__, devInfo.bustype, devInfo.vendor, devInfo.product);

    /*please define array for differnt vid&pid with the same rc platform*/
    if (devInfo.bustype == BUS_BLUETOOTH) {
        info->forVoiceRecognition = true;
        info->hidraw_index = hidrawIndex;
        info->hidraw_device = devInfo.bustype;
        result = true;
    }

done:
    if (fd > 0) {
        close(fd);
    }

    return result;
}

// scan the ALSA device directory for a usable capture device
void AudioHotplugThread::scanSoundCardDevice()
{
    DIR* alsaDir;
    DeviceInfo deviceInfo;

    alsaDir = opendir(kAlsaDeviceDir);
    if (alsaDir == NULL)
        return;

    while (true) {
        struct dirent *entry = NULL;
        entry = readdir(alsaDir);
        if (entry == NULL)
            break;
        unsigned int pcmCard, pcmDevice;
        if (parseCaptureDeviceName(entry->d_name, &pcmCard, &pcmDevice)) {
            if (getDeviceInfo(pcmCard, pcmDevice, &deviceInfo)) {
                mCallback.onDeviceFound(deviceInfo);
            }
        }
    }

    closedir(alsaDir);
}

void AudioHotplugThread::scanHidrawDevice()
{
    DeviceInfo deviceInfo;

    for (unsigned int i=0; i < MAX_HIDRAW_ID; i++) {
        if (getDeviceInfo(i, &deviceInfo)) {
            mCallback.onDeviceFound(deviceInfo, true);
        }
    }
}

void AudioHotplugThread::scanForDevice() {
    scanHidrawDevice();
    scanSoundCardDevice();
}

void AudioHotplugThread::handleHidrawEvent(struct inotify_event *event) {
    unsigned int hidrawId = MAX_HIDRAW_ID;
    char *name = ((char *) event) + offsetof(struct inotify_event, name);

    if (strstr(name, "hidraw") == NULL) {
        //no hidraw event, do nothing
        return;
    }
    int ret = sscanf(name, "hidraw%d", &hidrawId);
    if (ret == 1 && hidrawId < MAX_HIDRAW_ID) {
        if (event->mask & IN_CREATE) {
            // Some devices can not be opened immediately after the
            // inotify event occurs.  Add a delay to avoid these
            // races.  (50ms was chosen arbitrarily)
            /*
            const int kOpenTimeoutMs = 50;
            struct pollfd pfd = {mShutdownEventFD, POLLIN, 0};
            if (poll(&pfd, 1, kOpenTimeoutMs) == -1) {
                ALOGE("AudioHotplugThread: poll failed");
                return;
            } else if (pfd.revents & POLLIN) {
                // shutdown requested
                return;
            }*/

            DeviceInfo deviceInfo;
            if (getDeviceInfo(hidrawId, &deviceInfo)) {
                mCallback.onDeviceFound(deviceInfo, true);
            }
         }
         else if (event->mask & IN_DELETE) {
             mCallback.onDeviceRemoved(hidrawId);
         }
    }

}

void AudioHotplugThread::handleSoundCardEvent(struct inotify_event *event) {
    char *name = ((char *) event) + offsetof(struct inotify_event, name);
    unsigned int pcmCard, pcmDevice;

    if (parseCaptureDeviceName(name, &pcmCard, &pcmDevice)) {
        if (event->mask & IN_CREATE) {
            // Some devices can not be opened immediately after the
            // inotify event occurs.  Add a delay to avoid these
            // races.  (50ms was chosen arbitrarily)
            const int kOpenTimeoutMs = 50;
            struct pollfd pfd = {mShutdownEventFD, POLLIN, 0};
            if (poll(&pfd, 1, kOpenTimeoutMs) == -1) {
                ALOGE("AudioHotplugThread: poll failed");
                return;
            } else if (pfd.revents & POLLIN) {
                // shutdown requested
                return;
            }

            DeviceInfo deviceInfo;
            if (getDeviceInfo(pcmCard, pcmDevice, &deviceInfo)) {
                mCallback.onDeviceFound(deviceInfo);
            }
        }
        else if (event->mask & IN_DELETE) {
            mCallback.onDeviceRemoved(pcmCard, pcmDevice);
        }
    }
}

int AudioHotplugThread::handleDeviceEvent(int inotifyFD, int wfds[]) {

    // parse the filesystem change events
    char eventBuf[256] = {0};
    int ret = read(inotifyFD, eventBuf, sizeof(eventBuf));
    if (ret == -1) {
        ALOGE("AudioHotplugThread: read failed");
        return ret;
    }

    for (int i = 0; i < ret;) {
        if ((ret - i) < (int)sizeof(struct inotify_event)) {
            ALOGE("AudioHotplugThread: read an invalid inotify_event");
            break;
        }

        struct inotify_event *event =
                reinterpret_cast<struct inotify_event*>(eventBuf + i);

        if ((ret - i) < (int)(sizeof(struct inotify_event) + event->len)) {
            ALOGE("AudioHotplugThread: read a bad inotify_event length");
            break;
        }

        //dispatch event
        if (event->wd == wfds[0]) {
            //hidraw node insert
            handleHidrawEvent(event);
        }
        else {
            //sound card insert
            handleSoundCardEvent(event);
        }

        i += sizeof(struct inotify_event) + event->len;
    }

    return ret;
}

bool AudioHotplugThread::threadLoop()
{
    int flags = 0;
    int inotifyFD = -1;
    int watchFDs[2] = {-1}; //hidraw + sound card
    const char *devDir[2] = {kDeviceDir, kAlsaDeviceDir};

    //raise thread priority
    setpriority(PRIO_PROCESS, gettid(), AUDIO_HOTPLUG_THREAD_PRIORITY);

    // watch for changes to the ALSA device directory
    inotifyFD = inotify_init();
    if (inotifyFD == -1) {
        ALOGE("AudioHotplugThread: inotify_init failed");
        goto done;
    }
    flags = fcntl(inotifyFD, F_GETFL, 0);
    if (flags == -1) {
        ALOGE("AudioHotplugThread: F_GETFL failed");
        goto done;
    }

    if (fcntl(inotifyFD, F_SETFL, flags | O_NONBLOCK) == -1) {
        ALOGE("AudioHotplugThread: F_SETFL failed");
        goto done;
    }

    //add watch notify
    for (int i = 0; i < 2; i++) {
        watchFDs[i] = inotify_add_watch(inotifyFD, devDir[i],
                                IN_CREATE | IN_DELETE);
        if (watchFDs[i] == -1) {
            ALOGE("AudioHotplugThread:inotify_add_watch %s failed, i=%d,err=%d", devDir[i], i, errno);
            goto done;
        }
    }

    // check for any existing capture devices
    scanForDevice();
    while (!exitPending()) {
        // wait for a change to the ALSA directory or a shutdown signal
        struct pollfd fds[2] = {
            { inotifyFD, POLLIN, 0 },
            { mShutdownEventFD, POLLIN, 0 }
        };
        int ret = poll(fds, NELEM(fds), -1);
        if (ret == -1) {
            ALOGE("AudioHotplugThread: poll failed");
            break;
        } else if (fds[1].revents & POLLIN) {
            // shutdown requested
            break;
        }

        if (!(fds[0].revents & POLLIN)) {
            continue;
        }

        // parse the filesystem change events
        ret = handleDeviceEvent(inotifyFD, watchFDs);
        if (ret == -1) break;
    }

done:
    //remove watch fds
    for (int i = 0; i < 2; i++) {
        if (watchFDs[i] != -1)
            inotify_rm_watch(inotifyFD, watchFDs[i]);
    }
    if (inotifyFD != -1) {
        close(inotifyFD);
    }

    return false;
}

}; // namespace android
