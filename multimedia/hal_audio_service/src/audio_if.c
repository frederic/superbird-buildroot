#include <string.h>
#include <dlfcn.h>
#include <hardware/hardware.h>
#include <hardware/audio.h>
#include <pthread.h>
#include <system/audio.h>
#include <cutils/log.h>

#include "audio_if.h"

#define AUDIO_HAL_LIB "libaudio_hal.so"
static pthread_mutex_t if_mutex = PTHREAD_MUTEX_INITIALIZER;
static int if_ref_cnt;
static audio_hw_device_t *hw_dev;

static int load(const char *id,
    const char *path,
    struct hw_module_t **pHmi)
{
    int status = -EINVAL;
    void *handle = NULL;
    struct hw_module_t *hmi = NULL;

    handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        const char *err_str = dlerror();
        ALOGE("load: module=%s\n%s", path, err_str?err_str:"unknown");
        status = -EINVAL;
        goto done;
    }

    /* Get the address of the struct hal_module_info. */
    const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
    hmi = (struct hw_module_t *)dlsym(handle, sym);
    if (!hmi) {
        ALOGE("load: couldn't find symbol %s", sym);
        status = -EINVAL;
        goto done;
    }

    /* Check that the id matches */
    if (strcmp(id, hmi->id) != 0) {
        ALOGE("load: id=%s != hmi->id=%s", id, hmi->id);
        status = -EINVAL;
        goto done;
    }

    hmi->dso = handle;

    /* success */
    status = 0;

done:
    if (status != 0) {
        hmi = NULL;
        if (handle != NULL) {
            dlclose(handle);
            handle = NULL;
        }
    } else {
        ALOGE("loaded HAL id=%s path=%s hmi=%p handle=%p",
                    id, path, *pHmi, handle);
    }

    *pHmi = hmi;

    return status;
}

int audio_hw_load_interface(audio_hw_device_t **dev)
{
    int r;
    struct hw_module_t *pHmi;

    pthread_mutex_lock(&if_mutex);
    if (if_ref_cnt) {
        *dev = hw_dev;
        if_ref_cnt++;
        pthread_mutex_unlock(&if_mutex);
        return 0;
    }

    r = load(AUDIO_HARDWARE_MODULE_ID, AUDIO_HAL_LIB, &pHmi);
    if (r) {
        ALOGE("audio_hw_load_module couldn't load audio_hal module");
        goto out;
    }

    r = pHmi->methods->open(pHmi, AUDIO_HARDWARE_INTERFACE,
            (struct hw_device_t**)dev);
    if (r) {
        ALOGE("audio_hw_load_module couldn't open device");
        goto out;
    }
    if_ref_cnt++;
    hw_dev = *dev;
    pthread_mutex_unlock(&if_mutex);
    return 0;

out:
    *dev = NULL;
    pthread_mutex_unlock(&if_mutex);
    return r;
}

void audio_hw_unload_interface(audio_hw_device_t *dev)
{
    struct hw_module_t *pHmi = dev->common.module;

    pthread_mutex_lock(&if_mutex);
    if (if_ref_cnt >= 1)
        if_ref_cnt--;
    if (if_ref_cnt == 0 && pHmi) {
        dlclose(pHmi->dso);
        pHmi->dso = NULL;
        hw_dev = NULL;
    }
    pthread_mutex_unlock(&if_mutex);
}
