#pragma once
#ifndef CINEPAK_H__
#define CINEPAK_H__

#include <stdint.h>
#define ICCVID_MAGIC mmioFOURCC('c', 'v', 'i', 'd')
#define compare_fourcc(fcc1, fcc2) (((fcc1)^(fcc2))&~0x20202020)

#define DBUG    0
#define MAX_STRIPS 32

#define ERR printf
#define WARN printf
#define TRACE printf
/* ------------------------------------------------------------------------ */
typedef struct
{
    unsigned char y0, y1, y2, y3;
    char u, v;
    unsigned char r[4], g[4], b[4];
} cvid_codebook;

typedef struct {
    cvid_codebook* v4_codebook[MAX_STRIPS];
    cvid_codebook* v1_codebook[MAX_STRIPS];
    unsigned int strip_num;
} cinepak_info;

typedef struct _ICCVID_Info
{
    int         dwMagic;
    int           bits_per_pixel;
    cinepak_info* cvinfo;
} ICCVID_Info;

void decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size,
    unsigned char* frame, unsigned int width, unsigned int height, int bit_per_pixel);
cinepak_info* decode_cinepak_init(void);
void free_cvinfo(cinepak_info* cvinfo);
#endif