#ifndef __PKG_718P_MAPDEF_H__
#define __PKG_718P_MAPDEF_H__

#define AP_FLASH_BASE_LNA 0x800000

#define AP_PKGIMG_LNA (0x0087e000)

//For 718P, CP image in AP Flash
#define CP_PKGIMG_LNA (0x0081a000)

#define BL_PKGIMG_LNA (0x00803000)
#ifdef __USER_CODE__
#define BOOTLOADER_PKGIMG_LIMIT_SIZE (0x11000)
#else
#define BOOTLOADER_PKGIMG_LIMIT_SIZE (0x12000)
#endif

#ifdef __USER_CODE__
#ifndef AP_PKGIMG_LIMIT_SIZE
#define AP_PKGIMG_LIMIT_SIZE (0x2ca000)
#endif
#else
#define AP_PKGIMG_LIMIT_SIZE (0x30b000)
#endif

#define CP_PKGIMG_LIMIT_SIZE (0x64000)

#endif

