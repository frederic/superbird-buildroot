#include <linux/bug.h>
#include <linux/export.h>
#include <linux/types.h>
#include <linux/mmdebug.h>
#include <linux/mm.h>

#include <asm/memory.h>

phys_addr_t __virt_to_phys(unsigned long x)
{
	unsigned long s, e;

	s = (unsigned long)_text;
	e = (unsigned long)__bss_stop;
	if (x >= s && x <= e) /* kernel image range */
		return __virt_to_phys_nodebug(x);

	WARN(!__is_lm_address(x),
	     "virt_to_phys used for non-linear address: %pK (%pS)\n",
	      (void *)x,
	      (void *)x);

	return __virt_to_phys_nodebug(x);
}
EXPORT_SYMBOL(__virt_to_phys);

phys_addr_t __phys_addr_symbol(unsigned long x)
{
	/*
	 * This is bounds checking against the kernel image only.
	 * __pa_symbol should only be used on kernel symbol addresses.
	 */
	WARN((x < (unsigned long) KERNEL_START ||
	     x > (unsigned long) KERNEL_END),
	    "Invalid input address of %lx, %pS\n", x, (void *)x);
	return __pa_symbol_nodebug(x);
}
EXPORT_SYMBOL(__phys_addr_symbol);
