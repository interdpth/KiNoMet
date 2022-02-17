#pragma once
#ifndef CINEPAK_H__
#define CINEPAK_H__
#ifndef  GBA
#include <stdio.h>
#else
#include "Gba.h"
#endif // ! GBA
#pragma push(1)
#include <vector>
using namespace std;
#define ICCVID_MAGIC mmioFOURCC('c', 'v', 'i', 'd')
#define compare_fourcc(fcc1, fcc2) (((fcc1)^(fcc2))&~0x20202020)

#define DBUG    0
#define MAX_STRIPS 2

#define ERR printf
#define WARN printf
#define TRACE printf
/* ------------------------------------------------------------------------ */
#ifdef GBA
typedef struct __attribute__((__packed__))
#else
typedef struct 
#endif
{
    unsigned char y0, y1, y2, y3;
    char u, v;
    unsigned char r[4], g[4], b[4];
} cvid_codebook;

#ifdef GBA
typedef struct __attribute__((__packed__))
#else
typedef struct
#endif
{
    cvid_codebook* v4_codebook[MAX_STRIPS];
    cvid_codebook* v1_codebook[MAX_STRIPS];
    unsigned int strip_num;
} cinepak_info;
unsigned short MAKECOLOUR16(unsigned char r, unsigned char g, unsigned char b);
#ifdef GBA 
IWRAM void decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size, unsigned char* frame, unsigned int width, unsigned int height, int bit_per_pixel);
cinepak_info* decode_cinepak_init(void);
#else
void decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size, unsigned char* frame, unsigned int width, unsigned int height, int bit_per_pixel);
cinepak_info* decode_cinepak_init(void);
#endif 

void free_cvinfo(cinepak_info* cvinfo);

#endif