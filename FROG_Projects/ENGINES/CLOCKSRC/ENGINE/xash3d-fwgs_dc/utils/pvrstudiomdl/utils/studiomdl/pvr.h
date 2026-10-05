// PVR.H
#include <stdint.h>
#define GBIXHEADER  (('X'<<24)+('I'<<16)+('B'<<8)+'G')
#define PVRTSIGN    (('T'<<24)+('R'<<16)+('V'<<8)+'P')

#define PVR_TWIDDLE 0x01
#define PVR_VQ      0x03
#define PVR_RECT    0x09
#define PVR_VQ_MIPMAP 0x04

#pragma pack(push,1)
typedef struct gbix_s
{
    uint32_t version;           // "GBIX" in ASCII
    uint32_t nextTagOffset;     // bytes number to next tag
    unsigned long long globalIndex;
} gbix_t;

typedef struct pvrt_s 
{
    uint32_t version;          // "PVRT" in ASCII
    uint32_t textureDataSize;  // Size of rest of the file
    uint8_t colorFormat;       // 0x01 for RGB565
    uint8_t imageFormat;       // 0x01=twiddled, 0x03=VQ, 0x09=rectangle
    uint16_t zeroes;          // Always 0
    uint16_t width;
    uint16_t height;
} pvrt_t;
#pragma pack(pop)

int LoadPVR(const char * pvr_file);