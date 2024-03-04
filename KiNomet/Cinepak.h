#pragma once
#ifndef CINEPAK_H__
#define CINEPAK_H__
#ifndef  GBA
#include <stdio.h>
#else
#include "Gba.h"
#endif // ! GBA
#define  EMPTY  0
#define  ERRORDECODING  0xFFFFFFFF
#define  BADSTRIP ERRORDECODING-1
#include "MemoryBuffers.h"
#include <vector>
using namespace std;
#define ICCVID_MAGIC mmioFOURCC('c', 'v', 'i', 'd')
#define compare_fourcc(fcc1, fcc2) (((fcc1)^(fcc2))&~0x20202020)
#include "CinePakio.h"
#define DBUG    0

#define cinepak_strip_Length 260
#define MAX_STRIPS 10+1

//Honestly someththing better can happen in this chunk
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

//CinePak instance info
#ifdef GBA
typedef struct __attribute__((__packed__))
#else
typedef struct
#endif
{
    memoryCodeBook* v4_codebook[MAX_STRIPS];
    memoryCodeBook* v1_codebook[MAX_STRIPS];
    unsigned int strip_num;
} cinepak_info;


extern int drawing;
void free_codebooks(cinepak_info* cvinfo);

#ifdef GBA 
IWRAM int decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size, unsigned char* frame);
cinepak_info* decode_cinepak_init(int srcwidth, int srcheight);
#else
unsigned int decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size, unsigned char* frame);
cinepak_info* decode_cinepak_init(int srcwidth, int srcheight);
#endif 

void free_cvinfo(cinepak_info* cvinfo);

#endif