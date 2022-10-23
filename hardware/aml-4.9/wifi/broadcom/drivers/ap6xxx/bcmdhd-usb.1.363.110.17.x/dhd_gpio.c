
#include <osl.h>
#include <dhd_linux.h>

#ifdef CONFIG_DHD_USE_STATIC_BUF
extern void *dhd_wlan_mem_prealloc(int section, unsigned long size);
#endif /* CONFIG_DHD_USE_STATIC_BUF */

static int dhd_wlan_get_mac_addr(unsigned char *buf)
{
	int err = 0;

	printf("======== %s ========\n", __FUNCTION__);
#ifdef EXAMPLE_GET_MAC
	/* EXAMPLE code */
	{
		struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
	}
#endif /* EXAMPLE_GET_MAC */

	return err;
}
#ifdef CONFIG_DHD_USE_STATIC_BUF
extern void *bcmdhd_mem_prealloc(int section, unsigned long size);
void* dhd_wlan_mem_prealloc(int section, unsigned long size)
{
	void *alloc_ptr = NULL;
	alloc_ptr = bcmdhd_mem_prealloc(section, size);
	if (alloc_ptr) {
		printf("success alloc section %d, size %ld\n", section, size);
		if (size != 0L)
			bzero(alloc_ptr, size);
		return alloc_ptr;
	}
	printf("can't alloc section %d\n", section);
	return NULL;
}
#endif

#if !defined(WL_WIRELESS_EXT)
struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];	/* ISO 3166-1 country abbreviation */
	char custom_locale[WLC_CNTRY_BUF_SZ];	/* Custom firmware locale */
	int32 custom_locale_rev;		/* Custom local revisin default -1 */
};
#endif

static struct cntry_locales_custom brcm_wlan_translate_custom_table[] = {
	/* Table should be filled out based on custom platform regulatory requirement */
	{"",   "XT", 49},  /* Universal if Country code is unknown or empty */
	{"US", "US", 0},
};

#ifdef CUSTOM_FORCE_NODFS_FLAG
struct cntry_locales_custom brcm_wlan_translate_nodfs_table[] = {
	{"",   "XT", 50},  /* Universal if Country code is unknown or empty */
	{"US", "US", 0},
};
#endif

static void *dhd_wlan_get_country_code(char *ccode
#ifdef CUSTOM_FORCE_NODFS_FLAG
	, u32 flags
#endif
)
{
	struct cntry_locales_custom *locales;
	int size;
	int i;

	if (!ccode)
		return NULL;

#ifdef CUSTOM_FORCE_NODFS_FLAG
	if (flags & WLAN_PLAT_NODFS_FLAG) {
		locales = brcm_wlan_translate_nodfs_table;
		size = ARRAY_SIZE(brcm_wlan_translate_nodfs_table);
	} else {
#endif
		locales = brcm_wlan_translate_custom_table;
		size = ARRAY_SIZE(brcm_wlan_translate_custom_table);
#ifdef CUSTOM_FORCE_NODFS_FLAG
	}
#endif

	for (i = 0; i < size; i++)
		if (strcmp(ccode, locales[i].iso_abbrev) == 0)
			return &locales[i];
	return NULL;
}

struct resource dhd_wlan_resources[] = {
	[0] = {
		.name	= "bcmdhd_wlan_irq",
		.start	= 0, /* Dummy */
		.end	= 0, /* Dummy */
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE
			| IORESOURCE_IRQ_HIGHLEVEL, /* Dummy */
	},
};

struct wifi_platform_data dhd_wlan_control = {
	.get_mac_addr	= dhd_wlan_get_mac_addr,
#ifdef CONFIG_DHD_USE_STATIC_BUF
	.mem_prealloc	= dhd_wlan_mem_prealloc,
#endif /* CONFIG_DHD_USE_STATIC_BUF */
	.get_country_code = dhd_wlan_get_country_code,
};

int dhd_wlan_init_plat_data(void)
{
	printf("======== %s ========\n", __FUNCTION__);
	return 0;
}

void dhd_wlan_deinit_plat_data(wifi_adapter_info_t *adapter)
{
	printf("======== %s ========\n", __FUNCTION__);
}

