
#ifndef FOTA_PATCH_H
#define FOTA_PATCH_H


typedef struct
{
    uint16_t psegNumPerPkg;      /* total segCnt @every pkg */
    uint16_t nextFwupPsegId :8;  /* started with 0 @every pkg */
    uint16_t isPsegEverBkup :1;
    uint16_t backwards      :1;
    uint16_t belongsPkgId   :4;
    uint16_t rsvdBit        :2;
    uint32_t fwupZoneId     :8;  /* FotaNvmZoneId_e */
    uint32_t baseFwSize     :24; /* old image real size */
    uint32_t nextFwSize     :24; /* new image real size */
    uint32_t rsvdBits2      :8;
    uint32_t parPkgPlSize;       /* delta size @every pkg, no pkgh */
    uint32_t parPkgPlOffs;       /* delta offset @every pkg in *.par */
    //void*    extras;
}FotaPsegCtrlUnit_t;

typedef int32_t (*pseg_refesh_callback)(uint8_t pkgId, uint8_t psegId);
typedef void    (*wdt_kick_callback)(void);


typedef struct
{
    pseg_refesh_callback  fwup_refresh_cb;
    pseg_refesh_callback  bkup_refresh_cb;
    wdt_kick_callback     wdt_kick_cb;
}FotaPscuCallbacks_t;

int32_t FOTA_patchImage(FotaPsegCtrlUnit_t *pscu, const FotaPscuCallbacks_t *callbacks);


#endif
