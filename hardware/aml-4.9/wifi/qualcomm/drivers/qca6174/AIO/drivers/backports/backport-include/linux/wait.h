#ifndef __BACKPORT_LINUX_WAIT_H
#define __BACKPORT_LINUX_WAIT_H
#include_next <linux/wait.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
extern int bit_wait(void *);
extern int bit_wait_io(void *);

static inline int
backport_wait_on_bit(void *word, int bit, unsigned mode)
{
	return wait_on_bit(word, bit, bit_wait, mode);
}

static inline int
backport_wait_on_bit_io(void *word, int bit, unsigned mode)
{
	return wait_on_bit(word, bit, bit_wait_io, mode);
}

#define wait_on_bit LINUX_BACKPORT(wait_on_bit)
#define wait_on_bit_io LINUX_BACKPORT(wait_on_bit_io)

#endif

#endif /* __BACKPORT_LINUX_WAIT_H */
