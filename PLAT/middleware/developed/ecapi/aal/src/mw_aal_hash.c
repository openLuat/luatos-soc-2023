
#include "mw_aal_hash.h"


void     mwAalInitSha256( MwAalSha256Ctx_t *ctx, int is224)
{
    ctx->is224 = is224;

#ifdef MW_SHA_MODE_HW
#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
    sctInit();
#else
    SctShaSegStart(ctx->is224 ? SCT_SHA_224 : SCT_SHA_256, NULL, 0);
#endif
#else
    mbedtls_sha256_init( &ctx->info);
    mbedtls_sha256_starts_ret(&ctx->info, ctx->is224);
#endif
}

void     mwAalDeinitSha256(MwAalSha256Ctx_t *ctx)
{
#ifdef MW_SHA_MODE_HW
#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
    sctDeInit();
#else
    //do nothing!
#endif

#else
    //do nothing!
#endif
}

uint32_t mwAalUpdateSha256(MwAalSha256Ctx_t *ctx, const uint8_t *input, uint8_t * output, size_t ilen, uint32_t last)
{
    uint32_t RetValue = 0;

#ifdef MW_SHA_MODE_HW
#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
    RetValue = shaUpdate(ctx->is224 ? SHA_TYPE_224 : SHA_TYPE_256, (uint32_t)input, (uint32_t)output, ilen, last);
#else
    SctShaAppendSeg(ctx->is224 ? SCT_SHA_224 : SCT_SHA_256, (uint8_t*)input, ilen);

    if(last)
    {
        SctShaSegEnd(ctx->is224 ? SCT_SHA_224 : SCT_SHA_256, SCT_BIG_ENDIAN, output);
    }
#endif

    return RetValue;
#else
    if (last == 0)
    {
        return mbedtls_sha256_update_ret( &ctx->info, input, ilen );
    }
    else
    {
        RetValue = mbedtls_sha256_update_ret(&ctx->info, (uint8_t*)input, ilen);
        if (RetValue !=0)
        {
            return -1;
        }

        RetValue = mbedtls_sha256_finish_ret(&ctx->info, output);
        if (RetValue !=0)
        {
            return -1;
        }
        return 0;
    }

#endif
}



