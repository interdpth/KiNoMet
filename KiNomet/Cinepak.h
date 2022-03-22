#pragma once
#ifndef CINEPAK_H__
#define CINEPAK_H__
#ifndef  GBA
#include <stdio.h>
#else
#include "Gba.h"
#endif // ! GBA

#include <vector>
using namespace std;
#define ICCVID_MAGIC mmioFOURCC('c', 'v', 'i', 'd')
#define compare_fourcc(fcc1, fcc2) (((fcc1)^(fcc2))&~0x20202020)

#define DBUG    0

#define cinepak_strip_Length 260
#define MAX_STRIPS 4+1

#ifdef GBA
#define ERR printDebug
#define WARN printDebug
#define TRACE printDebug
#else 
#define ERR printf
#define WARN printf
#define TRACE printf
#endif 
/* ------------------------------------------------------------------------ */
#ifdef GBA
typedef struct __attribute__((__packed__))
#else
typedef struct
#endif
{
    unsigned char y0, y1, y2, y3;
    signed char u, v;
   /* unsigned char reds[4], greens[4], blues[4];*/
} oldcvid_codebook;


#ifdef GBA
typedef struct __attribute__((__packed__))
#else
typedef struct
#endif
{
    unsigned char y0, y1, y2, y3;
    signed char u, v;
     unsigned char reds[4], greens[4], blues[4];
} arachicoldcvid_codebook;



typedef struct
{
    //unsigned char y0, y1, y2, y3;
    //signed char u, v;

    unsigned short rgb[4];
    //  unsigned char reds[4], greens[4], blues[4];
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
extern int drawing;
void free_codebooks(cinepak_info* cvinfo);

#ifndef GBA
unsigned short MAKECOLOUR16(unsigned char r, unsigned char g, unsigned char b);
#else 
extern "C" unsigned short MAKECOLOUR16(unsigned char r, unsigned char g, unsigned char b);
extern "C" int GBA_RLEDECOMP(void* src, void* dst);
extern "C"  int GBA_LZDECOMP(void* src, void* dst);
#endif
#ifdef GBA 
IWRAM void decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size, unsigned char* frame);
cinepak_info* decode_cinepak_init(int srcwidth, int srcheight);
#else
void decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size, unsigned char* frame);
cinepak_info* decode_cinepak_init(int srcwidth, int srcheight);
#endif 

void free_cvinfo(cinepak_info* cvinfo);

#endif