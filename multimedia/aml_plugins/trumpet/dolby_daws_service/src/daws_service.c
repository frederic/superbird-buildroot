/**
 * @file "daws_service.c"
 *
 * @brief Dolby Audio for Wireless Speakers ALSA Plugin Service
 *
 * EXPERIMENTAL/REFERENCE CODE ONLY. FOR DEMO PURPOSES ONLY
 *
 * @ingroup daws_solns_alsa
 */


#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <linux/input.h>
#include <dlfcn.h>

#include "daws_xml_parser.h"
#include "daws_service.h"
#include "libevdev.h"

#define SHM_KEY "//daws_dolby"
static int use_key = 1;

typedef int
(*daws_dap_xml_parser_p)
( const char *filename  /**< [in] Config XML file that needs to be parsed */
  , int b_dump_xml       /**< [in] If a verbose XML parsing is desired. */
  , unsigned sample_rate /**< [in] DAP Supported Sample Rate */
  , void *p_dap_config   /**< [out] Once the XML is parsed, the values are stored in this structure */
);
daws_dap_xml_parser_p daws_dap_xml_parser;
void *fd;

void get_profile_name
( daws_preset_mode mode  /**< [in] The DAP Preset Mode specifier enum */
  , char *name            /**< [out] The DAP Preset Mode in String format */
);

char getKey(void);
int libevdev_create(struct libevdev **evdev);
char libevdev_getKey(struct libevdev *dev);

int main(int argc, char** argv)
{
    char              key;
    void            * pShmAddr;
    int               ret;
    int               shmId;
    char              profile_name_from[MAX_NAME_SIZE];
    char              profile_name_to[MAX_NAME_SIZE];
    int               fd;
    int               parser_fd;
    struct libevdev   *dev = NULL;
    daws_dap_config * pDawCfg;
    char input[5];
    if (argc < 2) {
        printf("\nTuning XML is missing ...\n");
        printf("Usage: %s tuning_file.xml\n", argv[0]);
        return 0;
    }
    int id;
    static int use_key = 1;
    unsigned int channels =  2;

    shmId = shm_open(SHM_KEY, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);
    if (shmId < 0) {
        printf("Failed to create Shared Memory\n");
        exit(0);
    }
    ret = ftruncate(shmId, sizeof(daws_dap_config));
    pShmAddr = mmap(0, sizeof(daws_dap_config), PROT_READ | PROT_WRITE, MAP_SHARED, shmId, 0);
    if (pShmAddr == (void *) - 1) {
        printf("Shared Memory attach Failed\n");
        exit(0);
    }
    pDawCfg = (daws_dap_config *) pShmAddr;
    parser_fd= dlopen("/tmp/ds/0x21_0x1234_0x1d.so", RTLD_LAZY);
    if (parser_fd != NULL) {
        daws_dap_xml_parser = dlsym(parser_fd, "daws_dap_xml_parser");
    } else {
        printf("cant find decoder lib %s\n", dlerror());
        daws_dap_xml_parser = NULL;
    }
    ret = daws_dap_xml_parser(argv[1], 0, 48000, pDawCfg);
    if (ret < 0) {
        printf("Failed to read configuration %d\n", ret);
        goto done;
    }
    if (channels == 1 )
        pDawCfg->input_format = DAP_CPDP_FORMAT_1;
    else if (channels == 2 )
        pDawCfg->input_format = DAP_CPDP_FORMAT_2;

    if (use_key) {
        fd = libevdev_create(&dev);
        if (fd < 0 || dev == NULL) {
            printf("libevdev_create failed\n");
            exit(0);;
        }
    }

    while ( 1) {
        if (!use_key) {
            //printf("Press p to toggle through preset modes\n");
            //printf("Press Esc to Exit\n");
            key = getKey();
        } else {
            //printf("Press KEY_LEFT to toggle through preset modes\n");
            //printf("Press KEY_VOLUMEUP to Exit\n");
            key = libevdev_getKey(dev);
        }
        switch (key) {
            case 'p': {
                int id = pDawCfg->preset_idx + 1;
                if (id >= PRESET_MAX_MODES) {
                    id = PRESET_DYNAMIC_MODE;
                }
                get_profile_name (pDawCfg->preset_idx, profile_name_from);
                get_profile_name (id, profile_name_to);
                printf("Changing Preset %d (%s) to %d (%s)\n", pDawCfg->preset_idx, profile_name_from, id, profile_name_to);
                __atomic_store_n(&pDawCfg->preset_idx, id, __ATOMIC_SEQ_CST);
                pDawCfg->preset_idx = id;
            }
            break;
            case 'o': {
                int id = PRESET_OFF_MODE;
                get_profile_name (pDawCfg->preset_idx, profile_name_from);
                get_profile_name (id, profile_name_to);
                printf("\nChanging Preset %d (%s) to %d (%s)\n", pDawCfg->preset_idx, profile_name_from, id, profile_name_to);
                __atomic_store_n(&pDawCfg->preset_idx, id, __ATOMIC_SEQ_CST);
                pDawCfg->preset_idx = id;
            }
            break;
            case 'm': {
                int id = PRESET_MUSIC_MODE;
                get_profile_name (pDawCfg->preset_idx, profile_name_from);
                get_profile_name (id, profile_name_to);
                printf("\nChanging Preset %d (%s) to %d (%s)\n", pDawCfg->preset_idx, profile_name_from, id, profile_name_to);
                __atomic_store_n(&pDawCfg->preset_idx, id, __ATOMIC_SEQ_CST);
                pDawCfg->preset_idx = id;
            }
            break;
        }
        if (key == 27) break;
    }

done:
    libevdev_free(dev);
    close(fd);
    close(parser_fd);
    ret = munmap(pShmAddr, sizeof(int));
    if (ret != 0) {
        printf("munmap failed\n");
    }
    pShmAddr = 0;
    pDawCfg = 0;
    ret = shm_unlink(SHM_KEY);
    if (ret != 0) {
        printf("shm_unlink failed\n");
    }
    printf("daws service exit\n");
    return 0;
}

void get_profile_name(daws_preset_mode mode, char *name)
{
    switch (mode) {
        case PRESET_DYNAMIC_MODE:
            strcpy(name, "dynamic");
            break;
        case PRESET_MOVIE_MODE:
            strcpy(name, "movie");
            break;
        case PRESET_MUSIC_MODE:
            strcpy(name, "music");
            break;
        case PRESET_VOICE_MODE:
            strcpy(name, "voice");
            break;
        case PRESET_GAME_MODE:
            strcpy(name, "game");
            break;
        case PRESET_CUSTOM_MODE:
            strcpy(name, "Personalize");
            break;
        case PRESET_CUSTOM_A_MODE:
            strcpy(name, "Custom A");
            break;
        case PRESET_CUSTOM_B_MODE:
            strcpy(name, "Custom B");
            break;
        case PRESET_CUSTOM_C_MODE:
            strcpy(name, "Custom C");
            break;
        case PRESET_CUSTOM_D_MODE:
            strcpy(name, "Custom D");
            break;
        default:
        case PRESET_OFF_MODE:
            strcpy(name, "off");
            break;
    }
    return;
}

char getKey(void)
{
    char key;
    struct termios oldattr, newattr;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    key = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return key;
}

int libevdev_create(struct libevdev **evdev)
{
    struct libevdev *dev = NULL;
    int fd, rc;
    fd = open("/dev/input/event2", O_RDONLY);
    if (fd < 0) {
        printf ("open /dev/input/event1 device error!\n");
        return fd;
    }
    rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        printf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        goto out;
    }
    printf("Input device name: \"%s\"\n", libevdev_get_name(dev));
    printf("Input device ID: bus %#x vendor %#x product %#x\n",
    libevdev_get_id_bustype(dev),
    libevdev_get_id_vendor(dev),
    libevdev_get_id_product(dev));
    if (!libevdev_has_event_type(dev, EV_KEY) ||
        !libevdev_has_event_code(dev, EV_KEY, KEY_LEFT) ||
        !libevdev_has_event_code(dev, EV_KEY, KEY_RIGHT) ||
        !libevdev_has_event_code(dev, EV_KEY, KEY_VOLUMEDOWN) ||
        !libevdev_has_event_code(dev, EV_KEY, KEY_VOLUMEUP)) {
        printf("This device does not look like a KEY\n");
        goto out;
    }
    *evdev = dev;
    return fd;
out:
    libevdev_free(dev);
    close(fd);
    *evdev = NULL;
    return -1;
}

char libevdev_getKey(struct libevdev *dev)
{
    int rc;
    char key = 0;
    struct input_event ev;
    while (1) {
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (ev.type != EV_KEY || ev.value)
                continue;
            printf("Event: %s %s %d\n",
                   libevdev_event_type_get_name(ev.type),
                   libevdev_event_code_get_name(ev.type, ev.code), ev.value);
            if (ev.code == KEY_LEFT)
                key = 'p';
            else if (ev.code == KEY_RIGHT)
                key = 'o';
            else if (ev.code == KEY_VOLUMEDOWN)
                key = 'm';
            else if (ev.code == KEY_VOLUMEUP)
                key = 27;
            break;
        } else {
            printf("error: rc = %d\n", rc);
            break;
        }
    }
    return key;
}
