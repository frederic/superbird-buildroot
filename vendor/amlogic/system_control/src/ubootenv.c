#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <mtd/mtd-user.h>
#include <pthread.h>
#include <syslog.h>
#include <zlib.h>

#include "ubootenv.h"
#include "common.h"

#define LOG_TAG "systemcontrol"

static pthread_mutex_t env_lock = PTHREAD_MUTEX_INITIALIZER;

char BootenvPartitionName[32] = {0,};
char PROFIX_UBOOTENV_VAR[32] = {0,};

static unsigned int ENV_PARTITIONS_SIZE = 0;
static unsigned int ENV_EASER_SIZE = 0;
static unsigned int ENV_SIZE = 0;
static int ENV_INIT_DONE = 0;

static struct environment env_data;
static struct env_attribute env_attribute_header;

extern char **environ;

static env_attribute *env_parse_attribute(void) {
    char *value;
    char *key;
    char *line;
    char *proc = env_data.data;
    char *next_proc;
    env_attribute_t *attr = &env_attribute_header;
    int n_char;
    int n_char_end;

    memset(attr, 0, sizeof(env_attribute_t));
    do {
        next_proc = proc + strlen(proc) + sizeof(char);
        key = strchr(proc, (int)'=');
        if (key != NULL) {
            *key = 0;
            strcpy(attr->key, proc);
            strcpy(attr->value, key + sizeof(char));
        } else {
            syslog(LOG_ERR, "[ubootenv] error need '=' skip this value");
        }
        if (!(*next_proc)) {
            break;
        }
        proc = next_proc;
        attr->next = (env_attribute_t *)malloc(sizeof(env_attribute_t));
        if (!attr->next) {
            syslog(LOG_ERR, "[ubootenv] error malloc fail");
            break;
        }
        memset(attr->next, 0, sizeof(env_attribute_t));
        attr = attr->next;
    } while (1);

    return &env_attribute_header;
}

static int env_revert_attribute(void) {
    int len;
    env_attribute_t *attr = &env_attribute_header;
    char *data = env_data.data;
    memset(env_data.data, 0, ENV_SIZE);

    do {
        len = sprintf(data, "%s=%s", attr->key, attr->value);
        if (len < (int)(sizeof(char) * 3)) {
            syslog(LOG_ERR, "[ubootenv] invalid data");
        } else {
            data += len + sizeof(char);
        }
        attr = attr->next;
    } while (attr);

    return 0;
}

env_attribute_t *bootenv_get_attr(void) {
    return &env_attribute_header;
}

void bootenv_print(void) {
    env_attribute *attr = &env_attribute_header;
    while (attr != NULL) {
        syslog(LOG_INFO, "[ubootenv] key(%s) value(%s)", attr->key, attr->value);
        attr = attr->next;
    }
}

int read_bootenv() {
    int fd;
    int ret;
    uint32_t crc_calc;
    env_attribute_t *attr;
    struct mtd_info_user info;
    struct env_image *image;
    char *addr;

    if ((fd = open(BootenvPartitionName, O_RDONLY)) < 0) {
        syslog(LOG_ERR, "[ubootenv] open %s error %s", BootenvPartitionName, strerror(errno));

        return -1;
    }

    addr = (char *)malloc(ENV_PARTITIONS_SIZE);
    if (!addr) {
        syslog(LOG_ERR, "[ubootenv] not enough mm for env(%u bytes)", ENV_PARTITIONS_SIZE);
        close(fd);

        return -2;
    }
    memset(addr, 0, ENV_PARTITIONS_SIZE);
    env_data.image = addr;
    image = (struct env_image *)addr;
    env_data.crc = &(image->crc);
    env_data.data = image->data;

    ret = read(fd, env_data.image, ENV_PARTITIONS_SIZE);
    if (ret == (int)ENV_PARTITIONS_SIZE) {
        crc_calc = crc32(0, (uint8_t *)env_data.data, ENV_SIZE);
        if (crc_calc != *(env_data.crc)) {
            syslog(LOG_ERR, "[ubootenv] CRC fail save_crc=%08x calc_crc=%08x", *env_data.crc, crc_calc);
            close(fd);

            return -3;
        }
        pthread_mutex_lock(&env_lock);
        attr = env_parse_attribute();
        pthread_mutex_unlock(&env_lock);
        if (!attr) {
            close(fd);

            return -4;
        }
        bootenv_print();
    } else {
        syslog(LOG_ERR, "[ubootenv] read error 0x%x", ret);
        close(fd);

        return -5;
    }
    close(fd);

    return 0;
}

const char *bootenv_get_value(const char *key) {
    if (!ENV_INIT_DONE) {
        return NULL;
    }

    env_attribute_t *attr = &env_attribute_header;
    while (attr) {
        if (!strcmp(key, attr->key)) {
            return attr->value;
        }
        attr = attr->next;
    }

    return NULL;
}

int bootenv_set_value(const char *key, const char *value, int creat_args_flag) {
    env_attribute_t *attr = &env_attribute_header;
    env_attribute_t *last = attr;

    while (attr) {
        if (!strcmp(key, attr->key)) {
            strcpy(attr->value, value);

            return 2;
        }
        last = attr;
        attr = attr->next;
    }

    if (creat_args_flag) {
        syslog(LOG_ERR, "[ubootenv] ubootenv.var.%s not found, create now", key);
        attr = (env_attribute_t *)malloc(sizeof(env_attribute_t));
        last->next = attr;
        memset(attr, 0, sizeof(env_attribute_t));
        strcpy(attr->key, key);
        strcpy(attr->value, value);

        return 1;
    }

    return 0;
}

int save_bootenv(void) {
    int fd;
    int err;
    struct erase_info_user erase;
    struct mtd_info_user info;
    unsigned char *data = NULL;

    env_revert_attribute();
    *(env_data.crc) = crc32(0, (uint8_t *)env_data.data, ENV_SIZE);
    if ((fd = open(BootenvPartitionName, O_RDWR)) < 0) {
        syslog(LOG_ERR, "[ubootenv] open %s fail", BootenvPartitionName);

        return -1;
    }
    if (strstr(BootenvPartitionName, "mtd")) {
        memset(&info, 0, sizeof(info));
        err = ioctl(fd, MEMGETINFO, &info);
        if (err < 0) {
            syslog(LOG_ERR, "[ubootenv] get MTD info fail");
            close(fd);

            return -4;
        }
        erase.start = 0;
        if (info.erasesize > ENV_PARTITIONS_SIZE) {
            data = (unsigned char *)malloc(info.erasesize);
            if (!data) {
                syslog(LOG_ERR, "[ubootenv] oom");
                close(fd);

                return -5;
            }
            memset(data, 0, info.erasesize);
            err = read(fd, (void *)data, info.erasesize);
            if (err != (int)info.erasesize) {
                syslog(LOG_ERR, "[ubootenv] read fail");
                free(data);
                close(fd);

                return -6;
            }
            memcpy(data, env_data.image, ENV_PARTITIONS_SIZE);
            erase.length = info.erasesize;
        } else {
            erase.length = info.erasesize;
        }

        err = ioctl(fd, MEMERASE, &erase);
        if (err < 0) {
            syslog(LOG_ERR, "[ubootenv] MEMERASE error %d", err);
            close(fd);

            return -2;
        }
        if (info.erasesize > ENV_PARTITIONS_SIZE) {
            lseek(fd, 0L, SEEK_SET);
            err = write(fd, data, info.erasesize);
            free(data);
        } else {
            err = write(fd, env_data.image, ENV_PARTITIONS_SIZE);
        }
    } else {
        err = write(fd, env_data.image, ENV_PARTITIONS_SIZE);
    }
    close(fd);
    if (err < 0) {
        syslog(LOG_ERR, "[ubootenv] write size %d error", ENV_PARTITIONS_SIZE);

        return -3;
    }

    return 0;
}

static int is_bootenv_varible(const char *prop_name) {
    if (!prop_name || !(*prop_name))
        return 0;

    if (!(*PROFIX_UBOOTENV_VAR))
        return 0;

    if (!strncmp(prop_name, PROFIX_UBOOTENV_VAR, strlen(PROFIX_UBOOTENV_VAR))
            && strlen(prop_name) > strlen(PROFIX_UBOOTENV_VAR))
        return 1;

    return 0;
}

static void bootenv_prop_init(const char *key, const char *value, void *cookie) {
    if (is_bootenv_varible(key)) {
        const char *varible_name = key + strlen(PROFIX_UBOOTENV_VAR);
        const char *varible_value = bootenv_get_value(varible_name);
        if (!varible_value)
            varible_name = "";
        if (strcmp(varible_value, value)) {
            syslog(LOG_INFO, "[ubootenv] setenv key(%s) value(%s)", key, varible_value);
            setenv(key, varible_value, 1);
            (*((int *)cookie))++;
        }
    }
}

char *read_sys_env(char *env, char *key, char *value) {
    char *index = strchr(env, '=');
    if (!index)
        return index;

    strncpy(key, env, index - env);
    key[index - env] = '\0';
    strncpy(value, index + 1, strlen(index + 1));
    value[strlen(index + 1)] = '\0';

    return index;
}

int bootenv_property_list(void (*propfn)(const char *key, const char *value, void *cookie),
        void *cookie) {
    char name[PROP_NAME_MAX] = {0,};
    char value[PROP_VALUE_MAX] = {0,};
    char **env = environ;

    while (*env) {
        if (read_sys_env(*env, name, value)) {
            propfn(name, value, cookie);
        }
        env++;
    }

    return 0;
}

void bootenv_props_load(void) {
    int count = 0;
    char bootcmd[32] = {0,};
    char *env;

    bootenv_property_list(bootenv_prop_init, (void *)&count);
    sprintf(bootcmd, "%sbootcmd", PROFIX_UBOOTENV_VAR);
    env = getenv(bootcmd);
    if (!env || !env[0]) {
        const char *value = bootenv_get_value("bootcmd");
        syslog(LOG_INFO, "[ubootenv] key(%s) value(%s)", bootcmd, value);
        if (value) {
            syslog(LOG_INFO, "[ubootenv] setenv key(%s) value(%s)", bootcmd, value);
            setenv(bootcmd, value, 1);
            count++;
        }
    }
    syslog(LOG_INFO, "[ubootenv] set property count(%d)", count);
    ENV_INIT_DONE = 1;
}

int bootenv_init(void) {
    struct stat st;
    struct mtd_info_user info;
    int err;
    int fd;
    int ret = -1;
    int i = 0;
    int count = 0;
    char prefix[PROP_VALUE_MAX] = {0,};

    if (stat("/dev/env", &st)) {
        syslog(LOG_ERR, "[ubootenv] /dev/env not found");

        return -1;
    }

    sprintf(BootenvPartitionName, "/dev/env");
    ENV_PARTITIONS_SIZE = 0x10000;
    ENV_SIZE = ENV_PARTITIONS_SIZE - sizeof(uint32_t);
    syslog(LOG_INFO, "[ubootenv] using /dev/env size(%d)(%d)", ENV_PARTITIONS_SIZE, ENV_SIZE);

    while (i < MAX_UBOOT_RWRETRY && ret < 0) {
        i++;
        ret = read_bootenv();
        if (ret < 0)
            syslog(LOG_ERR, "[ubootenv] cannot read %s (%d)", BootenvPartitionName, ret);
        if (ret < -2)
            free(env_data.image);
    }
    if (i >= MAX_UBOOT_RWRETRY) {
        syslog(LOG_ERR, "[ubootenv] read %s fail", BootenvPartitionName);

        return -2;
    }

    strcpy(prefix, "ubootenv.var");
    syslog(LOG_INFO, "[ubootenv] setenv key(ro.ubootenv.varible.prefix) value(%s)", prefix);
    setenv("ro.ubootenv.varible.prefix", prefix, 1);
    sprintf(PROFIX_UBOOTENV_VAR, "%s.", prefix);
    bootenv_props_load();

    return 0;
}

int bootenv_reinit(void) {
    pthread_mutex_lock(&env_lock);
    if (env_data.image) {
        free(env_data.image);
        env_data.image = NULL;
        env_data.crc = NULL;
        env_data.data = NULL;
    }

    env_attribute_t *pAttr = env_attribute_header.next;
    memset(&env_attribute_header, 0, sizeof(env_attribute_t));
    env_attribute_t *pTmp = NULL;
    while (pAttr) {
        pTmp = pAttr;
        pAttr = pAttr->next;
        free(pTmp);
    }
    bootenv_init();
    pthread_mutex_unlock(&env_lock);

    return 0;
}

int bootenv_update(const char *name, const char *value) {
    const char *varible_name = 0;
    int i = 0;
    int ret = -1;

    if (!ENV_INIT_DONE) {
        syslog(LOG_ERR, "[ubootenv] need init");

        return -1;
    }
    syslog(LOG_INFO, "[ubootenv] update name(%s) value(%s)", name, value);
    if (!strcmp(name, "ubootenv.var.bootcmd")) {
        varible_name = "bootcmd";
    } else {
        if (!is_bootenv_varible(name)) {
            syslog(LOG_ERR, "[ubootenv] %s is not a ubootenv varible", name);

            return -2;
        }
        varible_name = name + strlen(PROFIX_UBOOTENV_VAR);
    }
    const char *varible_value = bootenv_get_value(varible_name);
    if (!varible_value)
        varible_value = "";
    if (!strcmp(value, varible_value))
        return 0;

    pthread_mutex_lock(&env_lock);
    bootenv_set_value(varible_name, value, 1);
    while (i < MAX_UBOOT_RWRETRY && ret < 0) {
        i++;
        ret = save_bootenv();
        if (ret < 0)
            syslog(LOG_ERR, "[ubootenv] cannot write %s (%d)", BootenvPartitionName, ret);
    }
    if (i < MAX_UBOOT_RWRETRY)
        syslog(LOG_INFO, "[ubootenv] save to %s succeed", BootenvPartitionName);
    pthread_mutex_unlock(&env_lock);

    return ret;
}

const char *bootenv_get(const char *key) {
    if (!is_bootenv_varible(key)) {
        syslog(LOG_ERR, "[ubootenv] %s is not a ubootenv varible", key);

        return NULL;
    }

    pthread_mutex_lock(&env_lock);
    const char *varible_name = key + strlen(PROFIX_UBOOTENV_VAR);
    const char *env = bootenv_get_value(varible_name);
    pthread_mutex_unlock(&env_lock);

    return env;
}
