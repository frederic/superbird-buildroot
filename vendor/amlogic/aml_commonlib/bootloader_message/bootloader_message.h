
#ifndef _INIT_BOOTLOADER_MESSAGE_H
#define _INIT_BOOTLOADER_MESSAGE_H

#include <stdint.h>


struct bootloader_message {
    char command[32];
    char status[32];
    char recovery[768];

    // The 'recovery' field used to be 1024 bytes.  It has only ever
    // been used to store the recovery command line, so 768 bytes
    // should be plenty.  We carve off the last 256 bytes to store the
    // stage string (for multistage packages) and possible future
    // expansion.
    char stage[32];
    char slot_suffix[32];
    char reserved[192];
};

typedef struct BrilloSlotInfo {
    uint8_t bootable;
    uint8_t online;
    uint8_t reserved[2];
} BrilloSlotInfo;

typedef struct BrilloBootInfo {
    // Used by fs_mgr. Must be NUL terminated.
    char bootctrl_suffix[4];

    // Magic for identification - must be 'B', 'C', 'c' (short for
    // "boot_control copy" implementation).
    uint8_t magic[3];

    // Version of BrilloBootInfo struct, must be 0 or larger.
    uint8_t version;

    // Currently active slot.
    uint8_t active_slot;

    // Information about each slot.
    BrilloSlotInfo slot_info[2];
    uint8_t attemp_times;

    uint8_t reserved[14];
} BrilloBootInfo;

typedef struct MtdPartition {
    int device_index;
    long long size;
    long long erase_size;
    char *name;
} MtdPartition;

int reboot_recovery();
int clear_recovery();
int get_active_slot(int *slot);
int set_active_slot(int slot);
int set_recovery_otapath(char *path);
int get_recovery_otapath(char * path);
int clean_recovery_otapath();
int get_inactive_mtd(const char *partitionname);

#endif

