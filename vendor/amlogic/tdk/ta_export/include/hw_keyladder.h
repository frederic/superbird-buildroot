#ifndef __MESON_HW_KEYLADDER_H_
#define __MESON_HW_KEYLADDER_H_

/*
#define KEY_LDR_USR_CNTL       (0xc8830000 + (0x20 << 2))
#define KEY_LDR_USR_RAM_W      (0xc8830000 + (0x21 << 2))
#define KEY_LDR_VENDOR_ID      (0xc8830000 + (0x22 << 2))
#define KEY_LDR_NONCE_0        (0xc8830000 + (0x26 << 2))
#define KEY_LDR_NONCE_1        (0xc8830000 + (0x27 << 2))
#define KEY_LDR_NONCE_2        (0xc8830000 + (0x28 << 2))
#define KEY_LDR_NONCE_3        (0xc8830000 + (0x29 << 2))
*/

#define WLOC_KEY1              0
#define WLOC_NONCE             4
#define WLOC_KEY2              8
#define WLOC_KEY3              12
#define WLOC_KEY4              16
#define WLOC_KEY5              20
#define WLOC_KEY6              24
#define WLOC_KEY7              28

void kl_run(void *data);
void kl_get_response_to_challenge(void *data);

#endif
