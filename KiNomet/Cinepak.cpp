/*

Kinomet Cinepak Decoder 
Original implementation notice below
If used in commericial projects please add this file in plaintext into your compiled code.

*/


/*
 * Radius Cinepak Video Decoder
 *
 * Copyright 2001 Dr. Tim Ferguson (see below)
 * Portions Copyright 2003 Mike McCormack for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

 /* Copyright notice from original source:
  * ------------------------------------------------------------------------
  * Radius Cinepak Video Decoder
  *
  * Dr. Tim Ferguson, 2001.
  * For more details on the algorithm:
  *         http://www.csse.monash.edu.au/~timf/videocodec.html
  *
  * This is basically a vector quantiser with adaptive vector density.  The
  * frame is segmented into 4x4 pixel blocks, and each block is coded using
  * either 1 or 4 vectors.
  *
  * There are still some issues with this code yet to be resolved.  In
  * particular with decoding in the strip boundaries.  However, I have not
  * yet found a sequence it doesn't work on.  Ill keep trying :)
  *
  * You may freely use this source code.  I only ask that you reference its
  * source in your projects documentation:
  *       Tim Ferguson: http://www.csse.monash.edu.au/~timf/
  * ------------------------------------------------------------------------ */

#include <stdlib.h>

#include "Cinepak.h"
#include "SmallBuffer.h"
#define bpp 2
unsigned char* basePointer;
int drawing = 0;

int sizeVar = 0;

/* ------------------------------------------------------------------------ */
unsigned char* in_buffer;
unsigned char uiclip[1024];
unsigned char* uiclp = NULL;

//basic tests
static_assert(sizeof(uiclip) == 1024, "Size is not 1024");
//static_assert(sizeof(cvid_codebook) == 18, "Codebook Size is not 18 bytes");
//WE are not acccepting changing videos at this time :D
int screenwidth, screenheight, frm_stride;


///Some macros we can get rid of in a recode
#define get_byte() tmpIo->GetByte()
#define skip_byte() tmpIo->SkipByte()
#define V1C cvid_v1_16(frm_ptr + ((y * frm_stride) + (x * bpp)), frame, frm_stride, v1_codebook + get_byte()); chunk_size--;
#define V4C cvid_v4_16(frm_ptr + ((y * frm_stride) + (x * bpp)), frame, frm_stride, v4_codebook + d0, v4_codebook + d1, v4_codebook + d2, v4_codebook + d3); chunk_size -= 4;

#define get_word() tmpIo->Read16()
#define get_long() tmpIo->Read32()
//#ifdef GBA
//IWRAM unsigned short get_word()
//#else 
//unsigned short get_word()
//#endif
//{
//	unsigned short val2 = (in_buffer[0] << 8 | in_buffer[1]);
//
//	in_buffer += 2;
//	return val2;
//}
//
//
//#ifdef GBA
//IWRAM unsigned long get_long()
//#else 
//unsigned long get_long()
//#endif
//{
//	unsigned long val2 = (in_buffer[0] << 24 | in_buffer[1] << 16 | in_buffer[2] << 8 | in_buffer[3]);
//
//	in_buffer += 4;
//
//	return val2;
//}


//#ifdef GBA
//IWRAM void read_codebook(memoryCodeBook* c, int mode)
//#else 
//void  read_codebook(memoryCodeBook* c, int mode)
//#endif
///* ---------------------------------------------------------------------- */
//{
//	signed int uvr, uvg, uvb;
//	oldcvid_codebook* curbk = ((oldcvid_codebook*)(in_buffer));
//	int y0 = curbk->y0;
//	int y1 = curbk->y1;
//	int y2 = curbk->y2;
//	int y3 = curbk->y3;
//	in_buffer += 4;//y0-y3;
//	if (mode)        /* black and white */
//	{
//		c->rgb[0] = MAKECOLOUR16(y0, y0, y0);
//		c->rgb[1] = MAKECOLOUR16(y1, y1, y1);
//		c->rgb[2] = MAKECOLOUR16(y2, y2, y2);
//		c->rgb[3] = MAKECOLOUR16(y3, y3, y3);
//	}
//	else            /* colour */
//	{		
//		signed 	int v = curbk->v;
//		signed 	int u = curbk->u;
//		in_buffer += 2;//we read v and u
//		uvr = v << 1;
//		uvg = -((u + 1) >> 1) - v;
//		uvb = u << 1;
//		
//		c->rgb[0] = MAKECOLOUR16(uiclp[y0 + uvr], uiclp[y0 + uvg], uiclp[y0 + uvb]);
//
//		c->rgb[1] = MAKECOLOUR16(uiclp[y1 + uvr], uiclp[y1 + uvg], uiclp[y1 + uvb]);
//
//		c->rgb[2] = MAKECOLOUR16(uiclp[y2 + uvr], uiclp[y2 + uvg], uiclp[y2 + uvb]);
//
//		c->rgb[3] = MAKECOLOUR16(uiclp[y3 + uvr], uiclp[y3 + uvg], uiclp[y3 + uvb]);
//
//	}
//}
//#ifndef GBA
//unsigned short inline MAKECOLOUR16(unsigned char r, unsigned  char g, unsigned char b)
//{
//	return ((((r >> 3) & 31) | (((g >> 3) & 31) << 5) | (((b >> 3) & 31) << 10)));
//}
//#endif


//Converts codebook v1 to gba colors
/* ------------------------------------------------------------------------ */
#ifdef GBA
IWRAM void cvid_v1_16(unsigned char* frm, unsigned char* limit, int stride, memoryCodeBook* cb)
#else 
void cvid_v1_16(unsigned char* frm, unsigned char* limit, int stride, memoryCodeBook* cb)
#endif
{

	unsigned short* vptr = (unsigned short*)frm;

	int width = stride >> 1;
	unsigned short* rgb = cb->rgb;

	unsigned short r0 = *rgb++;
	unsigned short r1 = *rgb++;
	unsigned short r2 = *rgb++;
	unsigned short r3 = *rgb++;

	vptr[0 * width + 0] = r0;
	vptr[0 * width + 1] = r0;

	vptr[0 * width + 2] = r1;
	vptr[0 * width + 3] = r1;

	vptr[1 * width + 0] = r0;
	vptr[1 * width + 1] = r0;


	vptr[1 * width + 2] = r1;
	vptr[1 * width + 3] = r1;

	vptr[2 * width + 0] = r2;
	vptr[2 * width + 1] = r2;

	vptr[2 * width + 2] = r3;
	vptr[2 * width + 3] = r3;

	vptr[3 * width + 0] = r2;
	vptr[3 * width + 1] = r2;

	vptr[3 * width + 2] = r3;
	vptr[3 * width + 3] = r3;


}


//Converts codebook v4 to gba colors
/* ------------------------------------------------------------------------ */
#ifdef GBA
IWRAM void cvid_v4_16(unsigned char* frm, unsigned char* limit, int stride, memoryCodeBook* cb0,
	memoryCodeBook* cb1, memoryCodeBook* cb2, memoryCodeBook* cb3)
#else 
void cvid_v4_16(unsigned char* frm, unsigned char* limit, int stride, memoryCodeBook* cb0,
	memoryCodeBook* cb1, memoryCodeBook* cb2, memoryCodeBook* cb3)
#endif
{

	unsigned short* vptr = (unsigned short*)frm;

	int width = stride >> 1;

	unsigned short* clrs = cb0->rgb;

		//screw calculattions.
	vptr[0 * width + 0] = *clrs++;
	vptr[0 * width + 1] = *clrs++;
	vptr[1 * width + 0] = *clrs++;
	vptr[1 * width + 1] = *clrs++;

	clrs = cb1->rgb;
	vptr[0 * width + 2] = *clrs++;
	vptr[0 * width + 3] = *clrs++;
	vptr[1 * width + 2] = *clrs++;
	vptr[1 * width + 3] = *clrs++;

	clrs = cb2->rgb;
	vptr[2 * width + 0] = *clrs++;
	vptr[2 * width + 1] = *clrs++;
	vptr[3 * width + 0] = *clrs++;
	vptr[3 * width + 1] = *clrs++;

	clrs = cb3->rgb;

	vptr[2 * width + 2] = *clrs++;
	vptr[2 * width + 3] = *clrs++;
	vptr[3 * width + 2] = *clrs++;
	vptr[3 * width + 3] = *clrs++;
}

cinepak_info* newpack()
{
	int b = sizeof(cinepak_info);

	return (cinepak_info*)malloc(sizeof(cinepak_info));
}

/* ------------------------------------------------------------------------
 * Call this function once at the start of the sequence and save the
 * returned context for calls to decode_cinepak().
 */
cinepak_info* decode_cinepak_init(int srcwidth, int srcheight)
{
	screenwidth = srcwidth;
	screenheight = srcheight;
	frm_stride = srcwidth * 2;
	sizeVar = 0;
	int i;
	cinepak_info* cvinfo = newpack();

	if (!cvinfo)
		return NULL;
	cvinfo->strip_num = 0;

	if (uiclp == NULL)
	{
		for (int i = 0; i < 1024; i++) uiclip[i] = 0;
		uiclp = uiclip + 512;
		for (i = -512; i < 512; i++)
			uiclp[i] = (i < 0 ? 0 : (i > 255 ? 255 : i));
	}
	for (int i = 0; i < MAX_STRIPS; i++)
	{
		cvinfo->v4_codebook[i] = nullptr;
		cvinfo->v1_codebook[i] = nullptr;
	}

	return cvinfo;
}
void free_codebooks(cinepak_info* cvinfo)
{
	for (int i = 0; i < MAX_STRIPS; i++)
	{
		if (cvinfo->v4_codebook[i] != nullptr)
		{
			free(cvinfo->v4_codebook[i]);
			cvinfo->v4_codebook[i] = nullptr;
		}
		if (cvinfo->v1_codebook[i] != nullptr)
		{
			free(cvinfo->v1_codebook[i]);
			cvinfo->v1_codebook[i] = nullptr;
		}
	}
}
void free_cvinfo(cinepak_info* cvinfo)
{

	free_codebooks(cvinfo);
	free(cvinfo);
}

int codeBooks = 0;
int codeBookSize()
{
	return sizeVar;
}
//current frame pointer, limit is lowest frame pointer can hit, stride is width in bytes, cb is the proper code book to use
typedef void (*fn_cvid_v1)(unsigned char* frm, unsigned char* limit,
	int stride, memoryCodeBook* cb);
typedef void (*fn_cvid_v4)(unsigned char* frm, unsigned char* limit, int stride,
	memoryCodeBook* cb0, memoryCodeBook* cb1,
	memoryCodeBook* cb2, memoryCodeBook* cb3);

/* ------------------------------------------------------------------------
 * This function decodes a buffer containing a Cinepak encoded frame.
 *
 * context - the context created by decode_cinepak_init().
 * buf - the input buffer to be decoded
 * size - the size of the input buffer
 * frame - the output frame buffer (24 or 32 bit per pixel)
 * width - the width of the output frame
 * height - the height of the output frame
 * bit_per_pixel - the number of bits per pixel allocated to the output
 *   frame (only 24 or 32 bpp are supported)
 */

void InitCodeBook(cinepak_info* cvinfo, int i)
{
	if ((cvinfo->v4_codebook[i] = (memoryCodeBook*)malloc(sizeof(memoryCodeBook) * 260)) == NULL)
	{
		while (1)
		{
			(char*)"shits' fucked mate1";
		}
		return;
	}
	if ((cvinfo->v1_codebook[i] = (memoryCodeBook*)malloc(sizeof(memoryCodeBook) * 260)) == NULL)
	{
		while (1)
		{
			(char*)"shits' fucked mate1";
		}

		ERR("CVID: codebook v1 alloc err\n");

		return;
	}
}
unsigned long maxNum = 0;
#define KillChunk in_buffer+=chunk_size;//while (chunk_size > 0) { skip_byte(); chunk_size--; }
#ifdef GBA
IWRAM int decode_cinepak(cinepak_info* cvinfo, unsigned char* inputFrame, int size,
	unsigned char* frame)
#else 
unsigned int decode_cinepak(cinepak_info* cvinfo, unsigned char* inputFrame, int size,
	unsigned char* frame)
#endif
{

	drawing = 1;
	memoryCodeBook* v4_codebook, * v1_codebook, * codebook = NULL;
	unsigned long  frame_flags, strips, cv_width, cv_height,
		cnum, strip_id, chunk_id, ci, flag, mask;
	long len, chunk_size;
	unsigned long x, y;
	signed long	top_size, y_bottom, x0, y0, x1, y1;
	unsigned char* frm_ptr;
	unsigned int i, cur_strip;
	int d0, d1, d2, d3;

	y = 0;
	y_bottom = 0;
	in_buffer = inputFrame;
	CinepakIo* tmpIo = new CinepakIo(in_buffer, size);
	if (tmpIo) {
		frame_flags = get_byte();
		len = get_byte() << 16;
		len |= get_byte() << 8;
		len |= get_byte();

		frm_ptr = frame;

		if (len != size)
		{
			if (len & 0x01) len++; /* AVIs tend to have a size mismatch */
			if (len != size)
			{

				//ERR("CVID: corruption %d (QT/AVI) != %ld (CV)\n", size, len);
				return BADSTRIP;

			}

		}


		free_codebooks(cvinfo);
		i = 0;
		cv_width = get_word();
		cv_height = get_word();
		strips = get_word();
		if (strips > maxNum)
		{
			maxNum = strips;
		}

		if (strips > cvinfo->strip_num)
		{
			if (strips >= MAX_STRIPS)
			{

				//	ERR("CVID: strip overflow (more than %d)\n", MAX_STRIPS);
				return BADSTRIP;

			}


			for (i = cvinfo->strip_num; i < strips; i++)//Init our codebooks.
			{
				InitCodeBook(cvinfo, i);

				//int cvidSize = sizeof(cvid_codebook);
				//sizeVar += 2 * sizeof(cvid_codebook) * 260;
				//codeBooks += 2;
				//#endif
			}
		}
		cvinfo->strip_num = strips;

		//TRACE("CVID: <%ld,%ld> strips %ld\n", cv_width, cv_height, strips);


		for (cur_strip = 0; cur_strip < strips; cur_strip++)
		{
			v4_codebook = cvinfo->v4_codebook[cur_strip];
			v1_codebook = cvinfo->v1_codebook[cur_strip];

			if (v4_codebook == nullptr)
			{
				cvinfo->v4_codebook[cur_strip] = (memoryCodeBook*)malloc(sizeof(memoryCodeBook) * 260);
				v4_codebook = cvinfo->v4_codebook[cur_strip];
			}
			if (v1_codebook == nullptr)
			{
				cvinfo->v1_codebook[cur_strip] = (memoryCodeBook*)malloc(sizeof(memoryCodeBook) * 260);
				v1_codebook = cvinfo->v1_codebook[cur_strip];
			}


			if ((cur_strip > 0) && (!(frame_flags & 0x01)))
			{
				v4_codebook = cvinfo->v4_codebook[cur_strip - 1];
				v1_codebook = cvinfo->v1_codebook[cur_strip - 1];
			}

			strip_id = get_word();        /* 1000 = key strip, 1100 = iter strip */
			top_size = get_word();
			y0 = get_word();        /* FIXME: most of these are ignored at the moment */
			x0 = get_word();
			y1 = get_word();
			x1 = get_word();

			y_bottom += y1;
			top_size -= 12;
			x = 0;

			/*	if (x1 != screenwidth)
					WARN("CVID: Warning x1 (%ld) != width (%d)\n", x1, screenwidth);*/

					//TRACE("   %d) %04lx %04ld <%ld,%ld> <%ld,%ld> yt %ld\n",
					//	cur_strip, strip_id, top_size, x0, y0, x1, y1, y_bottom);


			while (top_size > 0)
			{
				chunk_id = get_word();
				chunk_size = get_word();

				//TRACE("        %04lx %04ld\n", chunk_id, chunk_size);
				top_size -= chunk_size;
				chunk_size -= 4;

				switch (chunk_id)
				{
					/* -------------------- Codebook Entries -------------------- */
				case 0x2000:
				case 0x2200:
					codebook = (chunk_id == 0x2200 ? v1_codebook : v4_codebook);
					cnum = chunk_size / 6;
					for (i = 0; i < cnum; i++) tmpIo->ReadCodeBook(codebook + i, 0);
					break;

				case 0x2400:
				case 0x2600:        /* 8 bit per pixel */
					codebook = (chunk_id == 0x2600 ? v1_codebook : v4_codebook);
					cnum = chunk_size / 4;
					for (i = 0; i < cnum; i++) tmpIo->ReadCodeBook(codebook + i, 1);
					break;

				case 0x2100:
				case 0x2300:
					codebook = (chunk_id == 0x2300 ? v1_codebook : v4_codebook);

					ci = 0;
					while (chunk_size > 0)
					{
						flag = get_long();
						chunk_size -= 4;

						for (i = 0; i < 32; i++)
						{
							if (flag & 0x80000000)
							{
								chunk_size -= 6;
								tmpIo->ReadCodeBook(codebook + ci, 0);
							}

							ci++;
							flag <<= 1;
						}
					}
					KillChunk
						break;

				case 0x2500:
				case 0x2700:        /* 8 bit per pixel */
					codebook = (chunk_id == 0x2700 ? v1_codebook : v4_codebook);

					ci = 0;
					while (chunk_size > 0)
					{
						flag = get_long();
						chunk_size -= 4;

						for (i = 0; i < 32; i++)
						{
							if (flag & 0x80000000)
							{
								chunk_size -= 4;
								tmpIo->ReadCodeBook(codebook + ci, 1);
							}

							ci++;
							flag <<= 1;
						}
					}
					KillChunk
						break;

					/* -------------------- Frame -------------------- */
				case 0x3000:
					while ((chunk_size > 0) && (y < y_bottom))
					{
						flag = get_long();
						chunk_size -= 4;

						for (i = 0; i < 32; i++)
						{
							if (y >= y_bottom) break;
							if (flag & 0x80000000)    /* 4 bytes per block */
							{
								d0 = get_byte();
								d1 = get_byte();
								d2 = get_byte();
								d3 = get_byte();
								V4C

							}
							else        /* 1 byte per block */
							{
								V1C
							}

							x += 4;
							if (x >= screenwidth)
							{
								x = 0;
								y += 4;
							}
							flag <<= 1;
						}
					}
					KillChunk
						break;

				case 0x3100:
					while ((chunk_size > 0) && (y < y_bottom))
					{
						/* ---- flag bits: 0 = SKIP, 10 = V1, 11 = V4 ---- */
						flag = get_long();
						chunk_size -= 4;
						mask = 0x80000000;

						while ((mask) && (y < y_bottom))
						{
							if (flag & mask)
							{
								if (mask == 1)
								{
									if (chunk_size < 0) break;
									flag = get_long();
									chunk_size -= 4;
									mask = 0x80000000;
								}
								else mask >>= 1;

								if (flag & mask)        /* V4 */
								{
									d0 = get_byte();
									d1 = get_byte();
									d2 = get_byte();
									d3 = get_byte();
									V4C
								}
								else        /* V1 */
								{
									V1C
								}
							}        /* else SKIP */

							mask >>= 1;
							x += 4;
							if (x >= screenwidth)
							{
								x = 0;
								y += 4;
							}
						}
					}

					KillChunk
						break;

				case 0x3200:        /* each byte is a V1 codebook */
					while ((chunk_size > 0) && (y < y_bottom))
					{
						V1C
							x += 4;
						if (x >= screenwidth)
						{
							x = 0;
							y += 4;
						}
					}
					KillChunk
						break;

				default:

					//ERR("CVID: unknown chunk_id %08lx\n", chunk_id);


					KillChunk
						break;
				}
			}
		}

		if (len != size)
		{
			if (len & 0x01) len++; /* AVIs tend to have a size mismatch */
			if (len != size)
			{
				long xlen;
				skip_byte();
				xlen = get_byte() << 16;
				xlen |= get_byte() << 8;
				xlen |= get_byte(); /* Read Len */

				/*	WARN("CVID: END INFO chunk size %d cvid size1 %ld cvid size2 %ld\n",
						size, len, xlen);*/

			}
		}
		delete tmpIo;

	}
	return len;
}