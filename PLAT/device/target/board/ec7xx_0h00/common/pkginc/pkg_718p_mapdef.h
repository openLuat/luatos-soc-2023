#ifndef __PKG_718P_MAPDEF_H__
#define __PKG_718P_MAPDEF_H__

#define AP_FLASH_BASE_LNA 0x800000

//For 718P, CP image in AP Flash
#define CP_PKGIMG_LNA (0x0081a000)

#define BL_PKGIMG_LNA (0x00803000)
#ifdef __USER_CODE__
#define BOOTLOADER_PKGIMG_LIMIT_SIZE (0x11000)
#ifdef __USER_MAP_CONF_FILE__
#include __USER_MAP_CONF_FILE__
#else


#if defined (FEATURE_AMR_CP_ENABLE) || defined (FEATURE_VEM_CP_ENABLE)
#ifndef AP_PKGIMG_LIMIT_SIZE
#define AP_PKGIMG_LIMIT_SIZE (0x286000)
#endif
#else
#ifndef AP_PKGIMG_LIMIT_SIZE
#define AP_PKGIMG_LIMIT_SIZE (0x2c2000)
#endif
#endif

#endif // __USER_MAP_CONF_FILE__

#if defined (FEATURE_AMR_CP_ENABLE) || defined (FEATURE_VEM_CP_ENABLE)

#define AP_PKGIMG_LNA (0x008BA000)
#define CP_PKGIMG_LIMIT_SIZE (0xA0000)

#else

#define AP_PKGIMG_LNA (0x0087e000)
#define CP_PKGIMG_LIMIT_SIZE (0x64000)

#endif


#else //user_code

#define BOOTLOADER_PKGIMG_LIMIT_SIZE (0x12000)
#if defined (FEATURE_AMR_CP_ENABLE) || defined (FEATURE_VEM_CP_ENABLE)

#define AP_PKGIMG_LNA (0x008BA000)

#ifdef FEATURE_EXCEPTION_FLASH_DUMP_ENABLE
#define AP_PKGIMG_LIMIT_SIZE (0x2CF000 - FLASH_EXCEP_DUMP_SIZE)
#else
#define AP_PKGIMG_LIMIT_SIZE (0x2CF000)
#endif

#define CP_PKGIMG_LIMIT_SIZE (0xA0000)

#else

#define AP_PKGIMG_LNA (0x0087e000)

#ifdef FEATURE_EXCEPTION_FLASH_DUMP_ENABLE
#define AP_PKGIMG_LIMIT_SIZE (0x30b000 - FLASH_EXCEP_DUMP_SIZE)
#else
#define AP_PKGIMG_LIMIT_SIZE (0x30b000)
#endif

#define CP_PKGIMG_LIMIT_SIZE (0x64000)

#endif



#endif


#endif

