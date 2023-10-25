
#include "fota_utils.h"
#include "fota_zipcodec.h"
//#include "lzmaEc.h"

#if 0
extern int EncodeFile(const char *srcFile, const char *decodeFile, char *rs);
extern int DecodeFile(const char *srcFile, const char *decodeFile, char *rs);
int32_t FOTA_encodeLzma(FotaBufferDesc_t *outBuf, FotaBufferDesc_t *inBuf)
{
    int32_t    size = 0;
    char    *encUri = FOTA_DEPS_LZMA_URI".enc";
    char    *decUri = FOTA_DEPS_LZMA_URI".dec";
    char         rs[UTL_BUF_SIZE_512];

    utlWriteBufToFile(decUri, inBuf->buffer + inBuf->offset, inBuf->length);

    if(0 != EncodeFile(decUri, encUri, rs))
    {
        FOTA_TRACE(LOG_DEBUG, "lzma encode: encode file fail!\n");
        return -1;
    }

    size = utlReadFileToBuf(encUri, outBuf->buffer+outBuf->offset, UTL_VALUE_UINT32_UNDEF);

    FOTA_BUF_DESC_APPEND(outBuf, size);

    return 0;
}

int32_t FOTA_decodeLzma(FotaBufferDesc_t *outBuf, FotaBufferDesc_t *inBuf)
{
    int32_t    size = 0;
    char    *encUri = FOTA_DEPS_LZMA_URI".enc";
    char    *decUri = FOTA_DEPS_LZMA_URI".dec";
    char         rs[UTL_BUF_SIZE_512];

    utlWriteBufToFile(encUri, inBuf->buffer + inBuf->offset, inBuf->length);

    if(0 != DecodeFile(encUri, decUri, rs))
    {
        FOTA_TRACE(LOG_DEBUG, "lzma decode: decode sect file fail!\n");
        return -1;
    }

    size = utlReadFileToBuf(decUri, outBuf->buffer+outBuf->offset, UTL_VALUE_UINT32_UNDEF);

    FOTA_BUF_DESC_APPEND(outBuf, size);

    return 0;
}

#else
extern int lzmaDecodeEc(unsigned char *outStream, unsigned int *outStreamLen, unsigned char *inStream, unsigned int inStreamLen, unsigned int dataType, unsigned int *lzmaUnpackLen);
extern int lzmaEncodeEc(unsigned char *outStreamData, unsigned int *outStreamDataLen, unsigned char *inStreamData, unsigned int inStreamDataLen);

//extern int lzmaEcEncode(unsigned char *outStreamData, int *outStreamDataLen, unsigned char *inStreamData, int inStreamDataLen);
//extern int lzmaEcDecode(unsigned char *outStreamData, int *outStreamDataLen, unsigned char *inStreamData, int inStreamDataLen);


int32_t FOTA_encodeLzma(FotaBufferDesc_t *outBuf, FotaBufferDesc_t *inBuf)
{
    int32_t   ret = FOTA_EOK;
#if FEATURE_LZMA_ENABLE
    unsigned int  size = 0;

    //lzmaEcEncode(outBuf->buffer + outBuf->offset, &size, inBuf->buffer + inBuf->offset, inBuf->length);
    if(/*SZ_OK != */(ret = lzmaEncodeEc(outBuf->buffer + outBuf->offset, &size, inBuf->buffer + inBuf->offset, inBuf->length)))
    {
        FOTA_TRACE(LOG_DEBUG,"lzma encode: failed! errno(%d)\n", ret);
    }
    FOTA_BUF_DESC_APPEND(outBuf, size);
#else
    FOTA_TRACE(LOG_DEBUG,"lzma encode: unspported err!\n");
#endif

    return ret;
}

int32_t FOTA_decodeLzma(FotaBufferDesc_t *outBuf, FotaBufferDesc_t *inBuf)
{
    int32_t   ret = FOTA_EOK;
#if FEATURE_LZMA_ENABLE
    unsigned int  size = 0;
    unsigned int  temp = 0;

    //lzmaEcDecode(outBuf->buffer + outBuf->offset, &size, inBuf->buffer + inBuf->offset, inBuf->length);
    if(/*SZ_OK != */(ret = lzmaDecodeEc(outBuf->buffer + outBuf->offset, &size, inBuf->buffer + inBuf->offset, inBuf->length, 0/*DECOMPRESS_DATA_TYPE_HEADER*/, &temp)))
    {
        FOTA_TRACE(LOG_DEBUG,"lzma decode: failed! errno(%d)\n", ret);
    }

    FOTA_BUF_DESC_APPEND(outBuf, size);
#else
    FOTA_TRACE(LOG_DEBUG,"lzma decode: unspported err!\n");
#endif

    return ret;
}

#endif




