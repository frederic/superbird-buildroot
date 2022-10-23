#ifndef UBOOTENV_H
#define UBOOTENV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_UBOOT_RWRETRY       5

typedef struct env_image {
    uint32_t crc;
    char data[];
} env_image_t;

typedef struct environment {
    void *image;
    uint32_t *crc;
    char *data;
} environment_t;

typedef struct env_attribute {
    struct env_attribute *next;
    char key[256];
    char value[1024];
} env_attribute_t;

int bootenv_init();
int bootenv_reinit();
const char *bootenv_get(const char *key);
int bootenv_update(const char *name, const char *value);
void bootenv_print(void);

env_attribute *bootenv_get_attr(void);

#ifdef __cplusplus
}
#endif

#endif
