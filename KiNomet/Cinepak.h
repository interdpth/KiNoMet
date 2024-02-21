#pragma once
#ifndef CINEPAK_H__
#define CINEPAK_H__
#ifndef  GBA
#include <Windows.h>
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
#define bpp 2
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

//Archival, delete one day
//#ifdef GBA
//typedef struct __attribute__((__packed__))
//#else
//typedef struct
//#endif
//{
//    unsigned char y0, y1, y2, y3;
//    signed char u, v;
//     unsigned char reds[4], greens[4], blues[4];
//} arachicoldcvid_codebook;
//

/// <summary>
/// We only store converted colors in this doo hickey
/// </summary>
typedef struct
{
    //unsigned char y0, y1, y2, y3;
    //signed char u, v;

    unsigned short rgb[4];
    //  unsigned char reds[4], greens[4], blues[4];
} memoryCodeBook;

typedef struct
{
	unsigned long len;
	unsigned char dat[0];
}RawCineFrame;
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

enum CvidAction :unsigned char {
	RegularRead,
	RegularRead_v4,
	UcharRead,
	UcharRead_v4,

	RegularChunkRead,
	RegularChunkRead_v4,

	UcharChunkRead,
	UcharChunkRead_v4,

	FiveBitRead,
	FiveBitFlagRead,
	OneBitRead,

	SKIPFRAME,


};

struct lookup {
	unsigned short val;
	CvidAction act;
};


struct CHeader {
	unsigned char frame_flags;
	unsigned long length;

	unsigned long cv_width;
	unsigned long cv_height;
	unsigned int strips;
};

struct KHeader {
	unsigned char frame_flags;
	unsigned long length;
//#ifndef GBA
//	unsigned short cv_width;
//	unsigned short cv_height;
//#else
	unsigned short cv_width;
	unsigned short cv_height;
//#endif
	unsigned int strips;
};
extern int drawing;
void  read_codebook(unsigned char** in_buffer, memoryCodeBook* c, int mode);
void InitCodeBook(cinepak_info* cvinfo, int i);
void free_codebooks(cinepak_info* cvinfo);
extern int screenwidth, screenheight, frm_stride;
#ifndef GBA
unsigned short MAKECOLOUR16(unsigned char r, unsigned char g, unsigned char b);
#else 
extern "C" unsigned short MAKECOLOUR16(unsigned char r, unsigned char g, unsigned char b);
extern "C" int GBA_RLEDECOMP(void* src, void* dst);
extern "C"  int GBA_LZDECOMP(void* src, void* dst);
#endif
#ifdef GBA 
IWRAM int decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size, unsigned char* frame);
cinepak_info* decode_cinepak_init(int srcwidth, int srcheight);
#else
unsigned int decode_cinepak(cinepak_info* cvinfo, unsigned char* buf, int size, unsigned char* frame);
cinepak_info* decode_cinepak_init(int srcwidth, int srcheight);
#endif 
unsigned long get_long(unsigned char** in_buffer);
unsigned short get_ushort(unsigned char** in_buffer);
void free_cvinfo(cinepak_info* cvinfo);
inline void cvid_v1_16(unsigned char* frm, unsigned char* limit, int stride, memoryCodeBook* cb);
inline void cvid_v4_16(unsigned char* frm, unsigned char* limit, int stride, memoryCodeBook* cb0,
	memoryCodeBook* cb1, memoryCodeBook* cb2, memoryCodeBook* cb3);
#endif

class Cinepak
{
};
