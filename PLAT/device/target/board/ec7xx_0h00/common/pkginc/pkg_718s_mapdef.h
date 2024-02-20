#ifndef __PKG_718S_MAPDEF_H__
#define __PKG_718S_MAPDEF_H__

#define AP_FLASH_BASE_LNA 0x800000



//For 718S, CP image in AP Flash
#define CP_PKGIMG_LNA (0x0081a000)

#define BL_PKGIMG_LNA (0x00803000)

#define BOOTLOADER_PKGIMG_LIMIT_SIZE (0x12000)

#ifdef __USER_CODE__
#ifdef __USER_MAP_CONF_FILE__
#include __USER_MAP_CONF_FILE__
#else
#ifndef AP_PKGIMG_LIMIT_SIZE
#if defined  GCF_FEATURE_MODE
#define AP_PKGIMG_LIMIT_SIZE (0x16e000)
#elif defined MID_FEATURE_MODE
#define AP_PKGIMG_LIMIT_SIZE (0x132000)
#else
#define AP_PKGIMG_LIMIT_SIZE (0x134000)
#endif
#endif //undefined AP_PKGIMG_LIMIT_SIZE
#endif
#else
#if defined  GCF_FEATURE_MODE
#define AP_PKGIMG_LIMIT_SIZE (0x16e000)
#elif defined MID_FEATURE_MODE
#define AP_PKGIMG_LIMIT_SIZE (0x132000)
#else
#define AP_PKGIMG_LIMIT_SIZE (0x134000)
#endif
#endif //__USER_CODE__

#if (defined MID_FEATURE_MODE) || (defined GCF_FEATURE_MODE)
#define AP_PKGIMG_LNA (0x00874000)
#define CP_PKGIMG_LIMIT_SIZE (0x5a000)
#else
#define AP_PKGIMG_LNA (0x00872000)
#define CP_PKGIMG_LIMIT_SIZE (0x58000)
#endif

#endif

