#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <zlib.h>
#include <mtd/mtd-user.h>

#include "bootloader_message.h"

#define COMMANDBUF_SIZE 32
#define STATUSBUF_SIZE      32
#define RECOVERYBUF_SIZE 768

#define BOOTINFO_OFFSET 864
#define SLOTBUF_SIZE    32
#define MISCBUF_SIZE  1088

#define CMD_RUN_RECOVERY   "boot-recovery"
#define MTD_PROC_FILENAME   "/proc/mtd"

int g_init = 0;
int g_mtd_number = 0;
MtdPartition mtdpartition[32] = {0};


/* Parse the contents of the file, which looks like:
 *
 *     # cat /proc/mtd
 *     dev:    size   erasesize  name
 *     mtd0: 00080000 00020000 "bootloader"
 *     mtd1: 00400000 00020000 "mfg_and_gsm"
 *     mtd2: 00400000 00020000 "0000000c"
 *     mtd3: 00200000 00020000 "0000000d"
 *     mtd4: 04000000 00020000 "system"
 *     mtd5: 03280000 00020000 "userdata"
 */
int mtd_scan_partitions() {
    int fd = 0;
    int i = 0;
    const char *bufp = NULL;
    ssize_t nbytes = 0;
    char buf[2048] = {0};

    if (g_init == 1) {
        return 0;
    }

    //Open and read the file contents.
    fd = open(MTD_PROC_FILENAME, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", MTD_PROC_FILENAME);
        return -1;
    }

    nbytes = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (nbytes < 0) {
        printf("read %s failed!\n", MTD_PROC_FILENAME);
        return -1;
    }

    bufp = buf;
    while (nbytes > 0) {
        int mtdnum;
        long long mtdsize, mtderasesize;
        int matches;
        char mtdname[64];

        mtdname[0] = '\0';
        mtdnum = -1;

        matches = sscanf(bufp, "mtd%d: %llx %llx \"%63[^\"]", &mtdnum, &mtdsize, &mtderasesize, mtdname);
        /* This will fail on the first line, which just contains
         * column headers.
         */

        if (matches == 4) {
            mtdpartition[g_mtd_number].device_index = mtdnum;
            mtdpartition[g_mtd_number].size = mtdnum;
            mtdpartition[g_mtd_number].erase_size = mtderasesize;
            mtdpartition[g_mtd_number].name = strdup(mtdname);

            if (mtdpartition[g_mtd_number].name == NULL) {
                printf("mtd%d name is NULL\n", mtdnum);
                return -1;
            }
            g_mtd_number++;
        }

        /* Eat the line.
        */
        while (nbytes > 0 && *bufp != '\n') {
            bufp++;
            nbytes--;
        }
        if (nbytes > 0) {
            bufp++;
            nbytes--;
        }
    }

    g_init = 1;
    for (i=0; i<g_mtd_number; i++) {
        printf("mtd%d  <------------>   %s\n", mtdpartition[i].device_index, mtdpartition[i].name);
    }

    return 0;
}

int get_mtd_by_name(char *name) {
    int i = 0;
    for (i=0; i<g_mtd_number; i++) {
        if (strcmp(mtdpartition[i].name, name) == 0) {
            return mtdpartition[i].device_index;
        }
    }
    return -1;
}

int get_mtd_size(char *mtdname) {
    int fd = 0;
    int ret = 0;
    mtd_info_t meminfo;

    //open mtd device
    fd = open(mtdname, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", mtdname);
        return -1;
    }

        //get meminfo
    ret = ioctl(fd, MEMGETINFO, &meminfo);
    if (ret < 0) {
        printf("get MEMGETINFO failed!\n");
        close(fd);
        return -1;
    }
    close(fd);

    return meminfo.size;
}

int nand_erase(const char *devicename, const int offset, const int len) {
    int fd;
    int ret = 0;
    struct stat st;
    mtd_info_t meminfo;
    erase_info_t erase;

    //open mtd device
    fd = open(devicename, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", devicename);
        return -1;
    }

    //check is a char device
    ret = fstat(fd, &st);
    if (ret < 0) {
        printf("fstat %s failed!\n", devicename);
        close(fd);
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        printf("%s: not a char device", devicename);
        close(fd);
        return -1;
    }

    //get meminfo
    ret = ioctl(fd, MEMGETINFO, &meminfo);
    if (ret < 0) {
        printf("get MEMGETINFO failed!\n");
        close(fd);
        return -1;
    }

    erase.length = meminfo.erasesize;

    for (erase.start = offset; erase.start < offset + len; erase.start += meminfo.erasesize) {
        loff_t bpos = erase.start;

        //check bad block
        ret = ioctl(fd, MEMGETBADBLOCK, &bpos);
        if (ret > 0) {
            printf("mtd: not erasing bad block at 0x%08llx\n", bpos);
            continue;  // Don't try to erase known factory-bad blocks.
        }

        if (ret < 0) {
            printf("MEMGETBADBLOCK error");
            close(fd);
            return -1;
        }

        //erase
        if (ioctl(fd, MEMERASE, &erase) < 0) {
            printf("mtd: erase failure at 0x%08llx\n", bpos);
            close(fd);
            return -1;
        }
    }

    close(fd);
    return 0;
}

static unsigned next_good_eraseblock(int fd, struct mtd_info_user *meminfo,
        unsigned block_offset)
{
    while (1) {
        loff_t offs;

        if (block_offset >= meminfo->size) {
            printf("not enough space in MTD device");
            return block_offset; /* let the caller exit */
        }

        offs = block_offset;
        if (ioctl(fd, MEMGETBADBLOCK, &offs) == 0)
            return block_offset;

        /* ioctl returned 1 => "bad block" */
        printf("Skipping bad block at 0x%08x\n", block_offset);
        block_offset += meminfo->erasesize;
    }
}

int nand_read(const char *device_name, const int mtd_offset,
                           char *data, size_t len) {

    mtd_info_t meminfo;
    unsigned int blockstart;
    unsigned int limit = 0;
    int real_size = 0;
    int size = 0;
    int ret = 0;
    int left = 0;
    int offset = mtd_offset;

    //open mtd device
    int fd = open(device_name, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", device_name);
        return -1;
    }

    //get meminfo
    ret = ioctl(fd, MEMGETINFO, &meminfo);
    if (ret < 0) {
        printf("get MEMGETINFO failed!\n");
        close(fd);
        return -1;
    }

    limit = meminfo.size;

    //check offset page aligned
    if (offset & (meminfo.writesize - 1)) {
        printf("start address is not page aligned");
        close(fd);
        return -1;
    }

    //malloc buffer for read
    char *tmp = (char *)malloc(meminfo.writesize);
    if (tmp == NULL) {
        printf("malloc %d size buffer failed!\n", meminfo.writesize);
        close(fd);
        return -1;
    }

    //if offset in a bad block, get next good block
    blockstart = offset & ~(meminfo.erasesize - 1);
    offset = next_good_eraseblock(fd, &meminfo, blockstart);
    printf("nand read find good block offset:%d\n", offset);
    if (offset >= limit) {
        printf("offset(%d) over limit(%d)\n", offset, limit);
        close(fd);
        free(tmp);
        return -1;
    }

    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek(%d) failed\n", offset);
        close(fd);
        free(tmp);
        return -1;
    }

    size = read(fd, tmp, meminfo.writesize);
    if (size != meminfo.writesize) {
        printf("need read(%d), but real(%d)\n", meminfo.writesize, size);
        close(fd);
        free(tmp);
        return -1;
    }

    memcpy(data, tmp, len);
    close(fd);
    free(tmp);
    return 0;
}

int nand_write(const char *device_name, const int mtd_offset,
                           const char *data, size_t len) {

    mtd_info_t meminfo;
    unsigned int blockstart;
    unsigned int limit = 0;
    int real_size = 0;
    int size = 0;
    int ret = 0;
    int left = 0;
    int offset = mtd_offset;

    //open mtd device
    int fd = open(device_name, O_WRONLY);
    if (fd < 0) {
        printf("open %s failed!\n", device_name);
        return -1;
    }

    //get meminfo
    ret = ioctl(fd, MEMGETINFO, &meminfo);
    if (ret < 0) {
        printf("get MEMGETINFO failed!\n");
        close(fd);
        return -1;
    }

    limit = meminfo.size;

    //check offset page aligned
    if (offset & (meminfo.writesize - 1)) {
        printf("start address is not page aligned");
        close(fd);
        return -1;
    }

    //malloc buffer for read
    char *tmp = (char *)malloc(meminfo.writesize);
    if (tmp == NULL) {
        printf("malloc %d size buffer failed!\n", meminfo.writesize);
        close(fd);
        return -1;
    }

    //get next good block
    blockstart = offset & ~(meminfo.erasesize - 1);
    offset = next_good_eraseblock(fd, &meminfo, blockstart);
    printf("nand write find good block offset:%d\n", offset);
    if (offset >= limit) {
        printf("offset(%d) over limit(%d)\n", offset, limit);
        close(fd);
        free(tmp);
        return -1;
    }

    memcpy(tmp, data, len);
    memset(tmp+len, 0, meminfo.writesize - len);

    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek(%d) failed\n", offset);
        close(fd);
        free(tmp);
        return -1;
    }

    size = write(fd, tmp, meminfo.writesize);
    if (size != meminfo.writesize) {
        printf("write err, need :%d, real :%d\n", meminfo.writesize, size );
        close(fd);
        free(tmp);
        return -1;
    }

    close(fd);
    free(tmp);
    return 0;
}

void dump_bootloader_message(struct bootloader_message *out) {
    BrilloBootInfo bootinfo;
    memcpy(&bootinfo, out->slot_suffix, SLOTBUF_SIZE);
    printf("out->stage:%s\n", out->stage);
    printf("bootinfo.active_slot:%d\n", bootinfo.active_slot);
    printf("bootinfo.bootctrl_suffix:%s\n", bootinfo.bootctrl_suffix);
    printf("bootinfo.slot_info[0].bootable:%d\n", bootinfo.slot_info[0].bootable);
    printf("bootinfo.slot_info[0].online:%d\n", bootinfo.slot_info[0].online);
    printf("bootinfo.slot_info[1].bootable:%d\n", bootinfo.slot_info[1].bootable);
    printf("bootinfo.slot_info[1].online:%d\n", bootinfo.slot_info[1].online);
}


static int get_bootloader_message_mtd(struct bootloader_message *out) {
    int ret = 0;
    struct bootloader_message temp;
    int mtd = get_mtd_by_name("misc");
    if (mtd < 0) {
        printf("get misc mtd num failed!\n");
        return -1;
    }

    char buff[128] = {0};
    sprintf(buff, "%s%d", "/dev/mtd", mtd);

    printf("buff:%s\n", buff);
    memset(&temp, 0, sizeof(temp));
    ret = nand_read(buff, 0, (char *)&temp, sizeof(struct bootloader_message));
    if (ret < 0) {
        printf("nand_read failed!\n");
        return -1;
    }

    memcpy(out, &temp, sizeof(temp));
    return 0;
}

static int set_bootloader_message_mtd(struct bootloader_message *in) {
    int ret = 0;
    int misc_size = 0;
    int mtd = get_mtd_by_name("misc");
    if (mtd < 0) {
        printf("get misc mtd num failed!\n");
        return -1;
    }

    char buff[128] = {0};
    sprintf(buff, "%s%d", "/dev/mtd", mtd);

    printf("buff:%s\n", buff);
    misc_size = get_mtd_size(buff);
    if (misc_size < 0) {
        printf("get %s size failed!\n", buff);
        return -1;
    }

    ret =nand_erase(buff, 0, misc_size);
    if (ret < 0) {
        printf("nand_erase failed!\n");
        return -1;
    }

    ret = nand_write(buff, 0, (char *)in, sizeof(struct bootloader_message));
    if (ret < 0) {
        printf("nand_read failed!\n");
        return -1;
    }

    return 0;
}

static int get_bootloader_message_block(struct bootloader_message *out) {
    const char *misc_device = "/dev/misc";

    FILE* f = fopen(misc_device, "rb");
    if (f == NULL) {
        printf("Can't open %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    struct bootloader_message temp;
    int count = fread(&temp, sizeof(temp), 1, f);
    if (count != 1) {
        printf("Failed reading %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    if (fclose(f) != 0) {
        printf("Failed closing %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    memcpy(out, &temp, sizeof(temp));
    return 0;
}


static int set_bootloader_message_block(const struct bootloader_message *in) {
    const char *misc_device = "/dev/misc";

    FILE* f = fopen(misc_device, "wb");
    if (f == NULL) {
        printf("Can't open %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    int count = fwrite(in, sizeof(*in), 1, f);
    if (count != 1) {
        printf("Failed writing %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    if (fclose(f) != 0) {
        printf("Failed closing %s\n(%s)\n", misc_device, strerror(errno));
        return -1;
    }
    return 0;
}

int get_store_device() {
    int ret = 0;
    ret = access("/proc/inand", F_OK);
    if (ret == 0 ) {
        printf("emmc device!\n");
        return 0;
    } else {
        printf("nand device!\n");
        return -1;
    }
}


int get_system_type() {
    int ret = 0;

    if (get_store_device() == 0) {
        ret = access("/dev/system_a", F_OK);
        if (ret == 0 ) {
            printf("double system!\n");
            return 0;
        } else {
            printf("single system!\n");
            return -1;
        }
    } else {
        ret = get_mtd_by_name("system_a");
        if (ret < 0) {
            printf("single system!\n");
            return -1;
        } else {
            printf("double system!\n");
            return 0;
        }
    }
}



static int get_bootloader_message(struct bootloader_message *out) {
    if (get_store_device() == 0) {
        return get_bootloader_message_block(out);
    } else {
        return get_bootloader_message_mtd(out);
    }
}

static int set_bootloader_message(struct bootloader_message *in) {
    if (get_store_device() == 0) {
        return set_bootloader_message_block(in);
    } else {
        return set_bootloader_message_mtd(in);
    }
}

int get_recovery_otapath(char *path) {
    int ret = 0;
    int len = 0;
    struct bootloader_message info;

    ret = get_bootloader_message(&info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    len = strlen(info.recovery);
    if ((len <= 0) || (len >= 256)) {
        printf("get_bootloader_message url length is error!\n");
        return -1;
    }

    if (strncmp(info.recovery, "http", 4)) {
        printf("get_bootloader_message url format is error!\n");
        return -1;
    }

    printf("get otapath :%s(%d)\n", info.recovery, strlen(info.recovery));
    memcpy(path, info.recovery,strlen(info.recovery));

    return 0;
}

int set_recovery_otapath(char *path) {
    int ret = 0;
    struct bootloader_message info;

    ret = get_bootloader_message(&info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    memcpy(info.command, CMD_RUN_RECOVERY, sizeof(CMD_RUN_RECOVERY));
    memset(info.recovery, 0, RECOVERYBUF_SIZE);
    memcpy(info.recovery, path, strlen(path));

    ret = set_bootloader_message(&info);
    if (ret != 0) {
        printf("set_bootloader_message failed!\n");
        return -1;
    }

    return 0;
}

int clean_recovery_otapath() {
    int ret = 0;
    struct bootloader_message info;

    ret = get_bootloader_message(&info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    memset(info.command, 0, COMMANDBUF_SIZE);
    memset(info.recovery, 0, RECOVERYBUF_SIZE);

    ret = set_bootloader_message(&info);
    if (ret != 0) {
        printf("set_bootloader_message failed!\n");
        return -1;
    }

    return 0;
}

int set_recovery() {
    int ret = 0;
    struct bootloader_message info;

    ret = get_bootloader_message(&info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    memcpy(info.command, CMD_RUN_RECOVERY, sizeof(CMD_RUN_RECOVERY));

    ret = set_bootloader_message(&info);
    if (ret != 0) {
        printf("set_bootloader_message failed!\n");
        return -1;
    }

    return 0;
}

int clear_recovery() {
    int ret = 0;
    struct bootloader_message info;

    ret = get_bootloader_message(&info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    memset(info.command, 0, COMMANDBUF_SIZE);

    ret = set_bootloader_message(&info);
    if (ret != 0) {
        printf("set_bootloader_message failed!\n");
        return -1;
    }

    return 0;
}

int get_active_slot_from_cmdline(int *slot) {

    char *p = NULL;
    char buffer[1024]={0};
    char slot_str[8] = {0};

    int fd = open("/proc/cmdline", O_RDONLY);
    if (fd < 0) {
        printf("open cmdline failed!\n");
        return -1;
    }

    int len = read(fd, buffer, 1024);
    if (len < 0) {
        printf("read cmdline failed!\n");
        close(fd);
        return -1;
    }

    close(fd);
    p = strstr(buffer, "slot_suffix");
    if (p == NULL) {
        printf("can not get slot from cmdline!\n");
        return -1;
    }

    strncpy(slot_str, p+strlen("slot_suffix="), 2);
    if (strcmp(slot_str, "_a") == 0) {
        *slot = 0;
    } else {
        *slot = 1;
    }

    return 0;
}

int get_active_slot_from_misc(int *slot) {
    int ret = 0;
    struct bootloader_message info;
    BrilloBootInfo bootinfo;

    ret = get_bootloader_message(&info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    memcpy(&bootinfo, info.slot_suffix, SLOTBUF_SIZE);
    *slot = bootinfo.active_slot;
    return 0;
}

int get_active_slot(int *slot) {
    return get_active_slot_from_cmdline(slot);
}

int set_active_slot(int slot) {
    int ret = 0;
    struct bootloader_message info;
    BrilloBootInfo bootinfo;

    ret = get_bootloader_message(&info);
    if (ret != 0) {
        printf("get_bootloader_message failed!\n");
        return -1;
    }

    memcpy(&bootinfo, info.slot_suffix, SLOTBUF_SIZE);
    if (slot == 0) {
        memcpy(bootinfo.bootctrl_suffix, "_a", 2);
    } else {
        memcpy(bootinfo.bootctrl_suffix, "_b", 2);
    }
    bootinfo.active_slot = slot;
    memcpy(info.slot_suffix, &bootinfo, SLOTBUF_SIZE);

    ret = set_bootloader_message(&info);
    if (ret != 0) {
        printf("set_bootloader_message failed!\n");
        return -1;
    }
    return 0;
}

int get_inactive_devicename(const char *partitionname, int slot, char *device) {
    int ret = 0;
    int mtd = 0;
    char tmp[128] = {0};

    if (get_store_device() == 0) {
        if (slot == 0) {
            sprintf(device, "/dev/%s%s", partitionname, "_b");
        } else {
            sprintf(device, "/dev/%s%s", partitionname, "_a");
        }
    } else {
        if (slot == 0) {
            sprintf(tmp, "%s%s", partitionname, "_b");
        } else {
            sprintf(tmp, "%s%s", partitionname, "_a");
        }
        mtd = get_mtd_by_name(tmp);
        if (mtd < 0) {
            printf("get %s mtd num failed!\n", tmp);
            return -1;
        }
        sprintf(device, "/dev/mtd%d", mtd);
    }

    printf("device name : %s\n", device);
    return 0;

}

int get_inactive_mtd(const char *partitionname) {
    int ret = 0;
    int slot = 0;
    int mtd = 0;
    char tmp[128] = {0};

    ret = get_active_slot(&slot);
    if (ret != 0) {
        printf("get active slot failed!\n");
        return -1;
    }

    if (slot == 0) {
        sprintf(tmp, "%s%s", partitionname, "_b");
    } else {
        sprintf(tmp, "%s%s", partitionname, "_a");
    }

    mtd = get_mtd_by_name(tmp);
    if (mtd < 0) {
        printf("get %s mtd num failed!\n", tmp);
        return -1;
    }

    return mtd;
}

int get_ubi_skip() {
    int ret = 0;
    int slot = 0;
    if (get_system_type() == 0) {

        ret = get_active_slot(&slot);
        if (ret < 0) {
            return 0;
        }

        if (slot == 0) {
            return 1;
        }

    }

    return 0;
}
