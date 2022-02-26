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

  //#define ORIGINAL

#include <stdlib.h>

#include "Gba.h"
#include "Cinepak.h"
unsigned char* basePointer;

void Trace(char* a, ...)
{
}

void ERR(char* a, ...)

{
}
int sizeVar = 0;

/* ------------------------------------------------------------------------ */
unsigned char* in_buffer;
unsigned char uiclip[1024];
unsigned char* uiclp = NULL;

//basic tests
static_assert(sizeof(uiclip) == 1024, "Size is not 1024");
//static_assert(sizeof(cvid_codebook) == 18, "Codebook Size is not 18 bytes");

#define REG_VCOUNT *(volatile unsigned short*)0x04000006
void vid_vsync()
{
	while (REG_VCOUNT < 128);
}

/*
typedef struct {
	cvid_codebook* v4_codebook[MAX_STRIPS];
	cvid_codebook* v1_codebook[MAX_STRIPS];
	unsigned int strip_num;
} cinepak_info;

*/

#define get_byte() *(in_buffer++)
#define skip_byte() in_buffer++

#ifdef GBA
IWRAM unsigned short get_word()
#else 
unsigned short get_word()
#endif
{//	((unsigned short)(in_buffer += 2, (in_buffer[-2] << 8 | in_buffer[-1])))

	unsigned short val2 = (in_buffer[0] << 8 | in_buffer[1]);

	in_buffer += 2;
	return val2;
}


#ifdef GBA
IWRAM unsigned long get_long()
#else 
unsigned long get_long()
#endif
{//	((unsigned short)(in_buffer += 2, (in_buffer[-2] << 8 | in_buffer[-1])))

	unsigned long val2 = (in_buffer[0] << 24 | in_buffer[1] << 16 | in_buffer[2] << 8 | in_buffer[3]);

	in_buffer += 4;

	return val2;
}


#ifdef GBA
IWRAM void read_codebook(cvid_codebook* c, int mode)
#else 
void  read_codebook(cvid_codebook* c, int mode)
#endif
/* ---------------------------------------------------------------------- */
{
	int uvr, uvg, uvb;
	oldcvid_codebook curbk;
	if (mode)        /* black and white */
	{
		memcpy(&curbk.y0, in_buffer, 4); in_buffer += 4;
		int y0 = curbk.y0;
		int y1 = curbk.y1;
		int y2 = curbk.y2;

		int y3 = curbk.y3;
		c->rgb[0] = MAKECOLOUR16(y0, y0, y0);
		c->rgb[1] = MAKECOLOUR16(y1, y1, y1);
		c->rgb[2] = MAKECOLOUR16(y2, y2, y2);
		c->rgb[3] = MAKECOLOUR16(y3, y3, y3);
	}
	else            /* colour */
	{

		memcpy(&curbk.y0, in_buffer, 6); in_buffer += 6;
		int y0 = curbk.y0;
		int y1 = curbk.y1;
		int y2 = curbk.y2;

		int y3 = curbk.y3;
		uvr = curbk.v << 1;
		uvg = -((curbk.u + 1) >> 1) - curbk.v;
		uvb = curbk.u << 1;


		c->rgb[0] = MAKECOLOUR16(uiclp[y0 + uvr], uiclp[y0 + uvg], uiclp[y0 + uvb]);

		c->rgb[1] = MAKECOLOUR16(uiclp[y1 + uvr], uiclp[y1 + uvg], uiclp[y1 + uvb]);

		c->rgb[2] = MAKECOLOUR16(uiclp[y2 + uvr], uiclp[y2 + uvg], uiclp[y2 + uvb]);

		c->rgb[3] = MAKECOLOUR16(uiclp[y3 + uvr], uiclp[y3 + uvg], uiclp[y3 + uvb]);

	}
}


unsigned short inline MAKECOLOUR16(unsigned char r, unsigned  char g, unsigned char b) {
#ifdef DEBUG
	return ((((r >> 3) & 31) | (((g >> 3) & 31) << 5) | (((b >> 3) & 31) << 10)));
#else 
	return ((((r >> 3) & 31) | (((g >> 3) & 31) << 5) | (((b >> 3) & 31) << 10)));
#endif 
}



/* ------------------------------------------------------------------------ */
#ifdef GBA
IWRAM void cvid_v1_16(unsigned char* frm, unsigned char* limit, int stride, cvid_codebook* cb)
#else 
void cvid_v1_16(unsigned char* frm, unsigned char* limit, int stride, cvid_codebook* cb)
#endif
{

	unsigned short* vptr = (unsigned short*)frm;

	int width = stride >> 1;

	unsigned short r0 = cb->rgb[0];
	unsigned short r1 = cb->rgb[1];
	unsigned short r2 = cb->rgb[2];;
	unsigned short r3 = cb->rgb[3];;




	//*((unsigned long*)(vptr[0 * width + 0])) = r0 << 16 | r0;
	////vptr[0 * width + 1] = vptr[0 * width + 0];
	//*((unsigned long*)(vptr[0 * width + 2])) = r1 << 16 | r1;


	//*((unsigned long*)(vptr[1 * width + 0])) = r0 << 16 | r0;


	//*((unsigned long*)(vptr[1 * width + 2])) = r1 << 16 | r1;

	//*((unsigned long*)(vptr[2 * width + 0])) = r2 << 16 | r2;

	//*((unsigned long*)(vptr[2 * width + 2])) = r3 << 16 | r3;

	//*((unsigned long*)(vptr[3 * width + 0])) = r2 << 16 | r2;

	//*((unsigned long*)(vptr[3 * width + 2])) = r3 << 16 | r3;
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

	//vptr[0 * width + 0] = cb->rgb[0];
	//vptr[0 * width + 1] = vptr[0 * width + 0];
	//vptr[0 * width + 2] = cb->rgb[1];
	//vptr[0 * width + 3] = vptr[0 * width + 2];


	//vptr[1 * width + 0] = cb->rgb[0];
	//vptr[1 * width + 1] = vptr[1 * width + 0];


	//vptr[1 * width + 2] = cb->rgb[1];
	//vptr[1 * width + 3] = vptr[1 * width + 2];

	//vptr[2 * width + 0] = cb->rgb[2];
	//vptr[2 * width + 1] = vptr[2 * width + 0];

	//vptr[2 * width + 2] = cb->rgb[3];
	//vptr[2 * width + 3] = vptr[2 * width + 2];

	//vptr[3 * width + 0] = cb->rgb[2];
	//vptr[3 * width + 1] = vptr[3 * width + 0];

	//vptr[3 * width + 2] = cb->rgb[3];
	//vptr[3 * width + 3] = vptr[3 * width + 2];
}

#define longcolors (*clrs++ << 16) | (*clrs++);
/* ------------------------------------------------------------------------ */
#ifdef GBA
IWRAM void cvid_v4_16(unsigned char* frm, unsigned char* limit, int stride, cvid_codebook* cb0,
	cvid_codebook* cb1, cvid_codebook* cb2, cvid_codebook* cb3)
#else 
void cvid_v4_16(unsigned char* frm, unsigned char* limit, int stride, cvid_codebook* cb0,
	cvid_codebook* cb1, cvid_codebook* cb2, cvid_codebook* cb3)
#endif
{

	unsigned short* vptr = (unsigned short*)frm;

	int width = stride / 2;

	cvid_codebook* curBook = cb0;
	unsigned short* clrs = curBook->rgb;
	//*((unsigned long*)(vptr[0 * width + 0])) = longcolors
	//	* ((unsigned long*)(vptr[0 * width + 1])) = longcolors
	//	* ((unsigned long*)(vptr[1 * width + 0])) = longcolors
	//	* ((unsigned long*)(vptr[1 * width + 1])) = longcolors

	//	curBook = cb1;
	//clrs = curBook->rgb;
	//*((unsigned long*)(vptr[0 * width + 2])) = longcolors
	//	* ((unsigned long*)(vptr[0 * width + 3])) = longcolors
	//	* ((unsigned long*)(vptr[1 * width + 2])) = longcolors
	//	* ((unsigned long*)(vptr[1 * width + 3])) = longcolors

	//	curBook = cb2;
	//clrs = curBook->rgb;
	//*((unsigned long*)(vptr[2 * width + 0])) = longcolors
	//	* ((unsigned long*)(vptr[2 * width + 1])) = longcolors
	//	* ((unsigned long*)(vptr[3 * width + 0])) = longcolors
	//	* ((unsigned long*)(vptr[3 * width + 1])) = longcolors

	//	curBook = cb3;
	//

	//*((unsigned long*)(vptr[2 * width + 2])) = longcolors
	//	* ((unsigned long*)(vptr[2 * width + 3])) = longcolors
	//	* ((unsigned long*)(vptr[3 * width + 2])) = longcolors
	//	* ((unsigned long*)(vptr[3 * width + 3])) = longcolors*/
		//screw calculattions.
		vptr[0 * width + 0] = *clrs++;
		vptr[0 * width + 1] = *clrs++;
		vptr[1 * width + 0] = *clrs++;
		vptr[1 * width + 1] = *clrs++;

		curBook = cb1;
		clrs = curBook->rgb;
		vptr[0 * width + 2] = *clrs++;
		vptr[0 * width + 3] = *clrs++;
		vptr[1 * width + 2] = *clrs++;
		vptr[1 * width + 3] = *clrs++;

		curBook = cb2;
		clrs = curBook->rgb;
		vptr[2 * width + 0] = *clrs++;
		vptr[2 * width + 1] = *clrs++;
		vptr[3 * width + 0] = *clrs++;
		vptr[3 * width + 1] = *clrs++;

		curBook = cb3;
		clrs = curBook->rgb;

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
cinepak_info* decode_cinepak_init(void)
{
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
	unsigned int i;

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
	int stride, cvid_codebook* cb);
typedef void (*fn_cvid_v4)(unsigned char* frm, unsigned char* limit, int stride,
	cvid_codebook* cb0, cvid_codebook* cb1,
	cvid_codebook* cb2, cvid_codebook* cb3);

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
	if ((cvinfo->v4_codebook[i] = (cvid_codebook*)malloc(sizeof(cvid_codebook) * 260)) == NULL)
	{
		while (1)
		{
			(char*)"shits' fucked mate1";
		}
		return;
		}
	if ((cvinfo->v1_codebook[i] = (cvid_codebook*)malloc(sizeof(cvid_codebook) * 260)) == NULL)
	{
		while (1)
		{
			(char*)"shits' fucked mate1";
		}
		ERR("CVID: codebook v1 alloc err\n");
		return;
	}
}
int maxNum = 0;
#define KillChunk in_buffer+=chunk_size;//while (chunk_size > 0) { skip_byte(); chunk_size--; }
#ifdef GBA
IWRAM void decode_cinepak(cinepak_info* cvinfo, unsigned char* inputFrame, int size,
	unsigned char* frame, unsigned int width, unsigned int height)
#else 
void decode_cinepak(cinepak_info* cvinfo, unsigned char* inputFrame, int size,
	unsigned char* frame, unsigned int width, unsigned int height)
#endif
{
	cvid_codebook* v4_codebook, * v1_codebook, * codebook = NULL;
	unsigned long x, y, y_bottom, frame_flags, strips, cv_width, cv_height,
		cnum, strip_id, chunk_id, x0, y0, x1, y1, ci, flag, mask;
	long len, top_size, chunk_size;
	unsigned char* frm_ptr;
	unsigned int i, cur_strip;
	int d0, d1, d2, d3, frm_stride, bpp = 2;
	fn_cvid_v1 cvid_v1 = (fn_cvid_v1)cvid_v1_16;
	fn_cvid_v4 cvid_v4 = (fn_cvid_v4)cvid_v4_16;
	y = 0;
	y_bottom = 0;
	in_buffer = inputFrame;

	frame_flags = get_byte();
	len = get_byte() << 16;
	len |= get_byte() << 8;
	len |= get_byte();


	frm_stride = width * bpp;
	frm_ptr = frame;

	if (len != size)
	{
		if (len & 0x01) len++; /* AVIs tend to have a size mismatch */
		if (len != size)
		{
			ERR("CVID: corruption %d (QT/AVI) != %ld (CV)\n", size, len);
			/* return; */
		}
	}
	;

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
			ERR("CVID: strip overflow (more than %d)\n", MAX_STRIPS);
			exit(-1);
		}

		for (i = 0; i < strips; i++)//Init our codebooks.
		{
			InitCodeBook(cvinfo, i);
			
			int cvidSize = sizeof(cvid_codebook);
			sizeVar += 2 * sizeof(cvid_codebook) * 260;
			codeBooks  +=2;
			//#endif
		}
	}
	cvinfo->strip_num = strips;

	TRACE("CVID: <%ld,%ld> strips %ld\n", cv_width, cv_height, strips);
	//Why can't we free here.
	for (cur_strip = 0; cur_strip < strips; cur_strip++)
	{
		v4_codebook = cvinfo->v4_codebook[cur_strip];
		v1_codebook = cvinfo->v1_codebook[cur_strip];

		if (v4_codebook == nullptr)
		{
			cvinfo->v4_codebook[cur_strip] = (cvid_codebook*)malloc(sizeof(cvid_codebook) * 260);
			v4_codebook = cvinfo->v4_codebook[cur_strip];
		}
		if (v1_codebook == nullptr)
		{
			cvinfo->v1_codebook[cur_strip] = (cvid_codebook*)malloc(sizeof(cvid_codebook) * 260);
			v1_codebook = cvinfo->v1_codebook[cur_strip];
		}


		if ((cur_strip > 0) && (!(frame_flags & 0x01)))
		{
			v4_codebook = cvinfo->v4_codebook[cur_strip - 1];
			v1_codebook = cvinfo->v1_codebook[cur_strip - 1];
			//memcpy(cvinfo->v4_codebook[cur_strip], cvinfo->v4_codebook[cur_strip - 1], 260 * sizeof(cvid_codebook));
			//memcpy(cvinfo->v1_codebook[cur_strip], cvinfo->v1_codebook[cur_strip - 1], 260 * sizeof(cvid_codebook));
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
		if (x1 != width)
			WARN("CVID: Warning x1 (%ld) != width (%d)\n", x1, width);

		TRACE("   %d) %04lx %04ld <%ld,%ld> <%ld,%ld> yt %ld\n",
			cur_strip, strip_id, top_size, x0, y0, x1, y1, y_bottom);

		while (top_size > 0)
		{
			chunk_id = get_word();
			chunk_size = get_word();

			TRACE("        %04lx %04ld\n", chunk_id, chunk_size);
			top_size -= chunk_size;
			chunk_size -= 4;

			switch (chunk_id)
			{
				/* -------------------- Codebook Entries -------------------- */
			case 0x2000:
			case 0x2200:
				codebook = (chunk_id == 0x2200 ? v1_codebook : v4_codebook);
				cnum = chunk_size / 6;
				for (i = 0; i < cnum; i++) read_codebook(codebook + i, 0);
				break;

			case 0x2400:
			case 0x2600:        /* 8 bit per pixel */
				codebook = (chunk_id == 0x2600 ? v1_codebook : v4_codebook);
				cnum = chunk_size / 4;
				for (i = 0; i < cnum; i++) read_codebook(codebook + i, 1);
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
							read_codebook(codebook + ci, 0);
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
							read_codebook(codebook + ci, 1);
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
							chunk_size -= 4;

							cvid_v4(frm_ptr + ((y * frm_stride) + (x * bpp)), frame, frm_stride, v4_codebook + d0, v4_codebook + d1, v4_codebook + d2, v4_codebook + d3);

						}
						else        /* 1 byte per block */
						{

							cvid_v1(frm_ptr + ((y * frm_stride) + (x * bpp)), frame, frm_stride, v1_codebook + get_byte());

							chunk_size--;
						}

						x += 4;
						if (x >= width)
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
								chunk_size -= 4;
#ifdef ORIGINAL
								cvid_v4(frm_ptr + (y * frm_stride + x * bpp), frm_end, frm_stride, v4_codebook + d0, v4_codebook + d1, v4_codebook + d2, v4_codebook + d3);
#else
								cvid_v4(frm_ptr + ((y * frm_stride) + (x * bpp)), frame, frm_stride, v4_codebook + d0, v4_codebook + d1, v4_codebook + d2, v4_codebook + d3);
#endif
							}
							else        /* V1 */
							{
								chunk_size--;
#ifdef ORIGINAL
								cvid_v1(frm_ptr + (y * frm_stride + x * bpp), frm_end, frm_stride, v1_codebook + get_byte());
#else
								cvid_v1(frm_ptr + ((y * frm_stride) + (x * bpp)), frame, frm_stride, v1_codebook + get_byte());
#endif
							}
						}        /* else SKIP */

						mask >>= 1;
						x += 4;
						if (x >= width)
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
#ifdef ORIGINAL
					cvid_v1(frm_ptr + (y * frm_stride + x * bpp), frm_end, frm_stride, v1_codebook + get_byte());
#else
					cvid_v1(frm_ptr + ((y * frm_stride) + (x * bpp)), frame, frm_stride, v1_codebook + get_byte());
#endif
					chunk_size--;
					x += 4;
					if (x >= width)
					{
						x = 0;
						y += 4;
					}
				}
				KillChunk
					break;

			default:
				ERR("CVID: unknown chunk_id %08lx\n", chunk_id);
				KillChunk
					break;
			}
		}
	}
#ifdef GBA
	vid_vsync();
#endif
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
			WARN("CVID: END INFO chunk size %d cvid size1 %ld cvid size2 %ld\n",
				size, len, xlen);
		}
	}
}
//
//void ICCVID_dump_BITMAPINFO(const BITMAPINFO* bmi)
//{
//    TRACE(
//        "planes = %d\n"
//        "bpp    = %d\n"
//        "height = %d\n"
//        "width  = %d\n"
//        "compr  = %s\n",
//        bmi->bmiHeader.biPlanes,
//        bmi->bmiHeader.biBitCount,
//        bmi->bmiHeader.biHeight,
//        bmi->bmiHeader.biWidth,
//        debugstr_an((const char*)&bmi->bmiHeader.biCompression, 4));
//}
//
//static inline int ICCVID_CheckMask(RGBQUAD bmiColors[3], COLORREF redMask, COLORREF blueMask, COLORREF greenMask)
//{
//    COLORREF realRedMask = MAKECOLOUR32(bmiColors[0].rgbRed, bmiColors[0].rgbGreen, bmiColors[0].rgbBlue);
//    COLORREF realBlueMask = MAKECOLOUR32(bmiColors[1].rgbRed, bmiColors[1].rgbGreen, bmiColors[1].rgbBlue);
//    COLORREF realGreenMask = MAKECOLOUR32(bmiColors[2].rgbRed, bmiColors[2].rgbGreen, bmiColors[2].rgbBlue);
//
//    TRACE("\nbmiColors[0] = 0x%08x\nbmiColors[1] = 0x%08x\nbmiColors[2] = 0x%08x\n",
//        realRedMask, realBlueMask, realGreenMask);
//
//    if ((realRedMask == redMask) &&
//        (realBlueMask == blueMask) &&
//        (realGreenMask == greenMask))
//        return TRUE;
//    return FALSE;
//}
//
//static LRESULT ICCVID_DecompressQuery(ICCVID_Info* info, LPBITMAPINFO in, LPBITMAPINFO out)
//{
//    TRACE("ICM_DECOMPRESS_QUERY %p %p %p\n", info, in, out);
//
//    if ((info == NULL) || (info->dwMagic != ICCVID_MAGIC))
//        return ICERR_BADPARAM;
//
//    TRACE("in: ");
//    ICCVID_dump_BITMAPINFO(in);
//
//    if (in->bmiHeader.biCompression != ICCVID_MAGIC)
//        return ICERR_BADFORMAT;
//
//    if (out)
//    {
//        TRACE("out: ");
//        ICCVID_dump_BITMAPINFO(out);
//
//        if (in->bmiHeader.biPlanes != out->bmiHeader.biPlanes)
//            return ICERR_BADFORMAT;
//        if (in->bmiHeader.biHeight != out->bmiHeader.biHeight)
//            return ICERR_BADFORMAT;
//        if (in->bmiHeader.biWidth != out->bmiHeader.biWidth)
//            return ICERR_BADFORMAT;
//
//        switch (out->bmiHeader.biBitCount)
//        {
//        case 16:
//            if (out->bmiHeader.biCompression == BI_BITFIELDS)
//            {
//                if (!ICCVID_CheckMask(out->bmiColors, 0x7C00, 0x03E0, 0x001F) &&
//                    !ICCVID_CheckMask(out->bmiColors, 0xF800, 0x07E0, 0x001F))
//                {
//                    TRACE("unsupported output bit field(s) for 16-bit colors\n");
//                    return ICERR_BADFORMAT;
//                }
//            }
//            break;
//        case 24:
//        case 32:
//            break;
//        default:
//            TRACE("unsupported output bitcount = %d\n", out->bmiHeader.biBitCount);
//            return ICERR_BADFORMAT;
//        }
//    }
//
//    return ICERR_OK;
//}
//
//static LRESULT ICCVID_DecompressGetFormat(ICCVID_Info* info, LPBITMAPINFO in, LPBITMAPINFO out)
//{
//    DWORD size;
//
//    TRACE("ICM_DECOMPRESS_GETFORMAT %p %p %p\n", info, in, out);
//
//    if ((info == NULL) || (info->dwMagic != ICCVID_MAGIC))
//        return ICERR_BADPARAM;
//
//    size = in->bmiHeader.biSize;
//    if (in->bmiHeader.biBitCount <= 8)
//        size += in->bmiHeader.biClrUsed * sizeof(RGBQUAD);
//
//    if (out)
//    {
//        memcpy(out, in, size);
//        out->bmiHeader.biCompression = BI_RGB;
//        out->bmiHeader.biSizeImage = in->bmiHeader.biHeight
//            * in->bmiHeader.biWidth * 4;
//        return ICERR_OK;
//    }
//    return size;
//}
//
//static LRESULT ICCVID_DecompressBegin(ICCVID_Info* info, LPBITMAPINFO in, LPBITMAPINFO out)
//{
//    TRACE("ICM_DECOMPRESS_BEGIN %p %p %p\n", info, in, out);
//
//    if ((info == NULL) || (info->dwMagic != ICCVID_MAGIC))
//        return ICERR_BADPARAM;
//
//    info->bits_per_pixel = out->bmiHeader.biBitCount;
//
//    if (info->bits_per_pixel == 16)
//    {
//        if (out->bmiHeader.biCompression == BI_BITFIELDS)
//        {
//            if (ICCVID_CheckMask(out->bmiColors, 0x7C00, 0x03E0, 0x001F))
//                info->bits_per_pixel = 15;
//            else if (ICCVID_CheckMask(out->bmiColors, 0xF800, 0x07E0, 0x001F))
//                info->bits_per_pixel = 16;
//            else
//            {
//                TRACE("unsupported output bit field(s) for 16-bit colors\n");
//                return ICERR_UNSUPPORTED;
//            }
//        }
//        else
//            info->bits_per_pixel = 15;
//    }
//
//    TRACE("bit_per_pixel = %d\n", info->bits_per_pixel);
//
//    if (info->cvinfo)
//        free_cvinfo(info->cvinfo);
//    info->cvinfo = decode_cinepak_init();
//
//    return ICERR_OK;
//}
//
//static LRESULT ICCVID_Decompress(ICCVID_Info* info, ICDECOMPRESS* icd, DWORD size)
//{
//    LONG width, height;
//
//    TRACE("ICM_DECOMPRESS %p %p %d\n", info, icd, size);
//
//    if ((info == NULL) || (info->dwMagic != ICCVID_MAGIC))
//        return ICERR_BADPARAM;
//    if (info->cvinfo == NULL)
//    {
//        ERR("ICM_DECOMPRESS sent after ICM_DECOMPRESS_END\n");
//        return ICERR_BADPARAM;
//    }
//
//    width = icd->lpbiInput->biWidth;
//    height = icd->lpbiInput->biHeight;
//
//    decode_cinepak(info->cvinfo, icd->lpInput, icd->lpbiInput->biSizeImage,
//        icd->lpOutput, width, height, info->bits_per_pixel);
//
//    return ICERR_OK;
//}
//
//static LRESULT ICCVID_DecompressEx(ICCVID_Info* info, ICDECOMPRESSEX* icd, DWORD size)
//{
//    LONG width, height;
//
//    TRACE("ICM_DECOMPRESSEX %p %p %d\n", info, icd, size);
//
//    if ((info == NULL) || (info->dwMagic != ICCVID_MAGIC))
//        return ICERR_BADPARAM;
//    if (info->cvinfo == NULL)
//    {
//        ERR("ICM_DECOMPRESSEX sent after ICM_DECOMPRESS_END\n");
//        return ICERR_BADPARAM;
//    }
//
//    /* FIXME: flags are ignored */
//
//    width = icd->lpbiSrc->biWidth;
//    height = icd->lpbiSrc->biHeight;
//
//    decode_cinepak(info->cvinfo, icd->lpSrc, icd->lpbiSrc->biSizeImage,
//        icd->lpDst, width, height, info->bits_per_pixel);
//
//    return ICERR_OK;
//}

//static LRESULT ICCVID_Close(ICCVID_Info* info)
//{
//    if ((info == NULL) || (info->dwMagic != ICCVID_MAGIC))
//        return 0;
//    if (info->cvinfo)
//        free_cvinfo(info->cvinfo);
//    heap_free(info);
//    return 1;
//}

//static LRESULT ICCVID_GetInfo(ICCVID_Info* info, ICINFO* icinfo, DWORD dwSize)
//{
//    if (!icinfo) return sizeof(ICINFO);
//    if (dwSize < sizeof(ICINFO)) return 0;
//
//    icinfo->dwSize = sizeof(ICINFO);
//    icinfo->fccType = ICTYPE_VIDEO;
//    icinfo->fccHandler = info ? info->dwMagic : ICCVID_MAGIC;
//    icinfo->dwFlags = 0;
//    icinfo->dwVersion = ICVERSION;
//    icinfo->dwVersionICM = ICVERSION;
//
//    LoadStringW(ICCVID_hModule, IDS_NAME, icinfo->szName, sizeof(icinfo->szName) / sizeof(WCHAR));
//    LoadStringW(ICCVID_hModule, IDS_DESCRIPTION, icinfo->szDescription, sizeof(icinfo->szDescription) / sizeof(WCHAR));
//    /* msvfw32 will fill icinfo->szDriver for us */
//
//    return sizeof(ICINFO);
//}

//static LRESULT ICCVID_DecompressEnd(ICCVID_Info* info)
//{
//    if (info->cvinfo)
//    {
//        free_cvinfo(info->cvinfo);
//        info->cvinfo = NULL;
//    }
//    return ICERR_OK;
//}

//LRESULT WINAPI ICCVID_DriverProc(DWORD_PTR dwDriverId, HDRVR hdrvr, UINT msg,
//    LPARAM lParam1, LPARAM lParam2)
//{
//    ICCVID_Info* info = (ICCVID_Info*)dwDriverId;
//
//    TRACE("%ld %p %d %ld %ld\n", dwDriverId, hdrvr, msg, lParam1, lParam2);
//
//    switch (msg)
//    {
//    case DRV_LOAD:
//        TRACE("Loaded\n");
//        return 1;
//    case DRV_ENABLE:
//        return 0;
//    case DRV_DISABLE:
//        return 0;
//    case DRV_FREE:
//        return 0;
//
//    case DRV_OPEN:
//    {
//        ICINFO* icinfo = (ICINFO*)lParam2;
//
//        TRACE("Opened\n");
//
//        if (icinfo && compare_fourcc(icinfo->fccType, ICTYPE_VIDEO)) return 0;
//
//        info = heap_alloc(sizeof(ICCVID_Info));
//        if (info)
//        {
//            info->dwMagic = ICCVID_MAGIC;
//            info->cvinfo = NULL;
//        }
//        return (LRESULT)info;
//    }
//
//    case DRV_CLOSE:
//        return ICCVID_Close(info);
//
//    case ICM_GETINFO:
//        return ICCVID_GetInfo(info, (ICINFO*)lParam1, (DWORD)lParam2);
//
//    case ICM_DECOMPRESS_QUERY:
//        return ICCVID_DecompressQuery(info, (LPBITMAPINFO)lParam1,
//            (LPBITMAPINFO)lParam2);
//    case ICM_DECOMPRESS_GET_FORMAT:
//        return ICCVID_DecompressGetFormat(info, (LPBITMAPINFO)lParam1,
//            (LPBITMAPINFO)lParam2);
//    case ICM_DECOMPRESS_BEGIN:
//        return ICCVID_DecompressBegin(info, (LPBITMAPINFO)lParam1,
//            (LPBITMAPINFO)lParam2);
//    case ICM_DECOMPRESS:
//        return ICCVID_Decompress(info, (ICDECOMPRESS*)lParam1,
//            (DWORD)lParam2);
//    case ICM_DECOMPRESSEX:
//        return ICCVID_DecompressEx(info, (ICDECOMPRESSEX*)lParam1,
//            (DWORD)lParam2);
//
//    case ICM_DECOMPRESS_END:
//        return ICCVID_DecompressEnd(info);
//
//    case ICM_COMPRESS_QUERY:
//        FIXME("compression not implemented\n");
//        return ICERR_BADFORMAT;
//
//    case ICM_CONFIGURE:
//        return ICERR_UNSUPPORTED;
//
//    default:
//        FIXME("Unknown message: %04x %ld %ld\n", msg, lParam1, lParam2);
//    }
//    return ICERR_UNSUPPORTED;
//}

//BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
//{
//    TRACE("(%p,%d,%p)\n", hModule, dwReason, lpReserved);
//
//    switch (dwReason)
//    {
//    case DLL_PROCESS_ATTACH:
//        DisableThreadLibraryCalls(hModule);
//        ICCVID_hModule = hModule;
//        break;
//    }
//    return TRUE;
//}