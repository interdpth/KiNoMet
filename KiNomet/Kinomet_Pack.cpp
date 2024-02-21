
#include <stdlib.h>
#include "..\KiNomet\Kinomet_Pack.h"
#include <vector>
#include "..\KiNomet\Cinepak.h"

#include "..\MirrorIO\LargeBuffer.h"

#include "..\MirrorIO\SmallBuffer.h"
#include "AviApi.h"
#include "Compression.h"
#include <Windows.h>
#define error(s) {printf(s); exit(-1);}
#define get_byte(p) *(p++)
#define skip_byte(p) p++

std::vector<unsigned char> tmpBuffer;
int  read_codebook_raw(unsigned char** in_buffer, std::vector<unsigned char>* outbuf, int mode)

{
	signed int uvr, uvg, uvb;

	int max = 4;
	if (!mode) max = 6;
	for (int i = 0; i < max; i++)
	{
		outbuf->push_back(((unsigned char*)*in_buffer)[i]);
	}



	return max;

}

lookup actionLookup[] = {

 {0x2000,RegularRead},
 {0x2200,RegularRead_v4},

 {0x2400,UcharRead},
 {0x2600, UcharRead_v4},


 {0x2100,RegularChunkRead},
 {0x2300, RegularChunkRead_v4},

 {0x2500,UcharChunkRead},
 {0x2700, UcharChunkRead_v4},

  {0x3000,FiveBitRead},
 {0x3100,FiveBitFlagRead},
{0x3200,OneBitRead},
};


lookup* GetAction(int val) {
	for (int i = 0; i < 11; i++)
	{
		if (actionLookup[i].val == val)
		{
			return &actionLookup[i];
		}
	}

	return nullptr;
}
extern unsigned long maxNum;


#define FillChunk while(output->Pos() % 2 != 0) output->WriteByte(0xFF);


#define KillChunk in_buffer+=chunk_size;//while (chunk_size > 0) { skip_byte(); chunk_size--; }
#define NewKillChunk input->Seek(chunk_size, SEEK_CUR);//while (chunk_size > 0) { skip_byte(); chunk_size--; }

unsigned int decode_cinepak_encode_kinometpack(cinepak_info* memorycvinfo, int screenwidth, int screenheight, unsigned char* inputFrame, int size, std::vector<unsigned char>* destBuffer)
{
	memoryCodeBook* v4_codebook, * v1_codebook, * codebook = NULL;
	unsigned long
		cnum, strip_id, chunk_id, ci, flag, mask;
	long  chunk_size = 0;
	unsigned long x, y;
	signed long	top_size, y_bottom, x0, y0, x1, y1;

	y = 0;
	y_bottom = 0;

	//THE PROCESS IS SIMPLE. 

	//GET HEADER.

	//WRITE HEADER. 

	SmallBuffer* input = new SmallBuffer(inputFrame, size);
	LargeBuffer* output = new LargeBuffer(size);
	//What can I say?  i like my code

	CHeader CinePakhdr = { 0 };

	CinePakhdr.frame_flags = input->GetByte();
	CinePakhdr.length = input->GetByte() << 16;
	CinePakhdr.length |= input->GetByte() << 8;
	CinePakhdr.length |= input->GetByte();

	//Exit early.
	if (CinePakhdr.length != size)
	{
		if (CinePakhdr.length & 0x01) CinePakhdr.length++; /* AVIs tend to have a size mismatch */
		if (CinePakhdr.length != size)
		{
			return BADSTRIP;

		}

	}



	CinePakhdr.cv_width = input->Read16();
	CinePakhdr.cv_height = input->Read16();
	CinePakhdr.strips = input->Read16();



	//Apply the header. 
	KHeader* k = (KHeader*)output->GetCurrentBuffer();
	output->Seek(sizeof(KHeader), SEEK_SET);
	k->cv_height = (unsigned char)CinePakhdr.cv_height;
	k->cv_width = (unsigned char)CinePakhdr.cv_width;
	k->frame_flags = CinePakhdr.frame_flags;
	k->length = 0x0;//fill in at end;
	k->strips = CinePakhdr.strips;

	int i = 0;
	//Decode and encode here. 

	if (k->strips > maxNum)
	{
		maxNum = k->strips;
	}

	if (CinePakhdr.strips > memorycvinfo->strip_num)
	{
		if (CinePakhdr.strips >= MAX_STRIPS)
		{

			//	ERR("CVID: strip overflow (more than %d)\n", MAX_STRIPS);
			return BADSTRIP;

		}


		for (i = memorycvinfo->strip_num; i < CinePakhdr.strips; i++)//Init our codebooks.
		{
			InitCodeBook(memorycvinfo, i);

			//int cvidSize = sizeof(cvid_codebook);
			//sizeVar += 2 * sizeof(cvid_codebook) * 260;
			//codeBooks += 2;
			//#endif
		}
	}
	memorycvinfo->strip_num = CinePakhdr.strips;
	char sl[1024] = { 0 };
	//Beging strip loop.
	for (int cur_strip = 0; cur_strip < k->strips; cur_strip++)
	{
		v4_codebook = memorycvinfo->v4_codebook[cur_strip];
		v1_codebook = memorycvinfo->v1_codebook[cur_strip];

		if (v4_codebook == nullptr)
		{
			memorycvinfo->v4_codebook[cur_strip] = (memoryCodeBook*)malloc(sizeof(memoryCodeBook) * 260);
			v4_codebook = memorycvinfo->v4_codebook[cur_strip];
		}
		if (v1_codebook == nullptr)
		{
			memorycvinfo->v1_codebook[cur_strip] = (memoryCodeBook*)malloc(sizeof(memoryCodeBook) * 260);
			v1_codebook = memorycvinfo->v1_codebook[cur_strip];
		}


		if ((cur_strip > 0) && (!(CinePakhdr.frame_flags & 0x01)))
		{
			v4_codebook = memorycvinfo->v4_codebook[cur_strip - 1];
			v1_codebook = memorycvinfo->v1_codebook[cur_strip - 1];
		}

		strip_id = input->Read16();        /* 1000 = key strip, 1100 = iter strip */
		output->Write16(strip_id);
		top_size = input->Read16();
		output->Write16(top_size);


		y0 = input->Read16();

		x0 = input->Read16();

		y1 = input->Read16();
		output->Write16(y1); //Care about this
		x1 = input->Read16();

		y_bottom += y1;
		top_size -= 12;
		x = 0;

		while (top_size > 0)
		{
			chunk_id = input->Read16();
			chunk_size = input->Read16();
			int start = output->Pos();
			sprintf_s(sl, 1023, "%04lx %04ld\n", chunk_id, chunk_size);

			auto t = GetAction(chunk_id);
			output->WriteByte(t->act);


			unsigned short* newSize = (unsigned short*)output->GetCurrentBuffer();
			output->Write16(0);



			OutputDebugStringA(sl);

			top_size -= chunk_size;
			chunk_size -= 4;
			try {
				switch (chunk_id)
				{
					/* -------------------- Codebook Entries -------------------- */
				case 0x2000:
				case 0x2200:
					codebook = (chunk_id == 0x2200 ? v1_codebook : v4_codebook);
					cnum = chunk_size / 6;
					for (i = 0; i < cnum; i++) {


						for (int i = 0; i < 6; i++)
						{
							output->WriteByte(input->GetByte());
						}
						chunk_size -= 6;
					}
					//NewKillChunk
					break;

				case 0x2400:
				case 0x2600:        /* 8 bit per pixel */
					codebook = (chunk_id == 0x2600 ? v1_codebook : v4_codebook);
					cnum = chunk_size / 4;
					for (int i = 0; i < cnum; i++)
					{
						output->WriteByte(input->GetByte()); chunk_size -= 4;
					}
					//NewKillChunk
					break;

				case 0x2100:
				case 0x2300:
					codebook = (chunk_id == 0x2300 ? v1_codebook : v4_codebook);

					ci = 0;
					while (chunk_size > 0)
					{
						flag = input->Read32();
						chunk_size -= 4;
						output->Write32(flag);
						for (i = 0; i < 32; i++)
						{
							if (flag & 0x80000000)
							{

								chunk_size -= 6;
								for (int i = 0; i < 6; i++)
								{
									output->WriteByte(input->GetByte());
								}
							}

							ci++;
							flag <<= 1;
						}
					}
					//NewKillChunk
					//	FillChunk
					break;

				case 0x2500:
				case 0x2700:        /* 8 bit per pixel */
					codebook = (chunk_id == 0x2700 ? v1_codebook : v4_codebook);

					ci = 0;
					while (chunk_size > 0)
					{
						flag = input->Read32();
						chunk_size -= 4;
						output->Write32(flag);

						for (i = 0; i < 32; i++)
						{
							if (flag & 0x80000000)
							{
								chunk_size -= 4;
								for (int i = 0; i < 4; i++)
								{
									output->WriteByte(input->GetByte());
								}
							}

							ci++;
							flag <<= 1;
						}
					}
					//NewKillChunk
					//	FillChunk
					break;

					/* -------------------- Frame -------------------- */
				case 0x3000:
				{
					unsigned char fullBreak = false;
					while ((chunk_size > 4) && (y < y_bottom))//There we check for greater than 4 here becasue there is the intial flag read, as well as it read 4 bytes or 1.
					{
						flag = input->Read32();
						chunk_size -= 4;
						output->Write32(flag);

						for (i = 0; i < 32; i++)
						{
							if (y >= y_bottom || chunk_size < 0) break;
							if (flag & 0x80000000)    /* 4 bytes per block */
							{
								if (chunk_size <= 4) break; //Make sure there's enough data

								output->WriteByte(input->GetByte());
								output->WriteByte(input->GetByte());
								output->WriteByte(input->GetByte());
								output->WriteByte(input->GetByte());
								//cvid_v4_16(curDest + ((y * frm_stride) + (x * bpp)), destBuffer, frm_stride, v4_codebook + d0, v4_codebook + d1, v4_codebook + d2, v4_codebook + d3);
								chunk_size -= 4;

							}
							else        /* 1 byte per block */
							{
								if (chunk_size <= 0) break;
								output->WriteByte(input->GetByte());
								//cvid_v1_16(curDest + ((y * frm_stride) + (x * bpp)), destBuffer, frm_stride, v1_codebook + get_byte(in_buffer)); 
								chunk_size--;
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
					//NewKillChunk
				}
				//FillChunk
				break;

				case 0x3100:
					if (chunk_size == 857)
					{
						printf("Wait");
					}
					while ((chunk_size > 0) && (y < y_bottom))//We have it as 4 here because we need to have room for the flag at least.
					{
						/* ---- flag bits: 0 = SKIP, 10 = V1, 11 = V4 ---- */
						flag = input->Read32();
						output->Write32(flag);
						chunk_size -= 4;
						mask = 0x80000000;

						while ((chunk_size > 0) && (y < y_bottom))
						{
							/*if (flag & mask)
							{*/
							if (mask == 1)
							{

								flag = input->Read32();
								output->Write32(flag);
								chunk_size -= 4;
								mask = 0x80000000;
							}
							else mask >>= 1;

							if (flag & mask)        /* V4 */
							{

								output->WriteByte(input->GetByte());
								output->WriteByte(input->GetByte());
								output->WriteByte(input->GetByte());
								output->WriteByte(input->GetByte());

								chunk_size -= 4;
							}
							else        /* V1 */
							{
								//Finish? 
								output->WriteByte(input->GetByte());
								chunk_size--;
							}
							//}        /* else SKIP */

							mask >>= 1;
							x += 4;
							if (x >= screenwidth)
							{
								x = 0;
								y += 4;
							}
						}
					}

					//NewKillChunk
						//FillChunk
					break;

				case 0x3200:        /* each byte is a V1 codebook */
					while ((chunk_size > 0) && (y < y_bottom))
					{
						output->WriteByte(input->GetByte());
						chunk_size--;
						x += 4;
						if (x >= screenwidth)
						{
							x = 0;
							y += 4;
						}
					}
					//NewKillChunk
					//	FillChunk
					break;

				default:

					char sl[1024] = { 0 };
					sprintf_s(sl, 1023, "CVID: unknown chunk_id %08lx\n", chunk_id);
					OutputDebugStringA(sl);

					//FillChunk
					break;
				}
			}
			catch (char* err)
			{
				printf(err);
			}


			int nsize = chunk_size;
			while (chunk_size--)
			{
				//output->WriteByte();
				input->GetByte();//just run 
			}
			*newSize = (output->Pos() - start);

			if (nsize == 0)
			{
				sprintf_s(sl, 1023, "Processed %04lx. Processed %04ld bytes\n", chunk_id, nsize);
			}
			else {
				sprintf_s(sl, 1023, "Processed %04lx. Failed to process %04ld bytes, SKIPPING.\n", chunk_id, nsize);
			}
			OutputDebugStringA(sl);
		}
	}

	int len = output->Pos();
	//V2 will compress here.
	k->length = len;//maybe ditch this
	unsigned char* outbuf = output->GetBuffer();
	for (int i = 0; i < k->length; i++) {
		destBuffer->push_back(outbuf[i]);
	}

	delete input;
	delete output;
	return 	len;
}



//We will now do the 
#ifdef GBA
IWRAM
#endif
unsigned int decode_kinepak(cinepak_info* memorycvinfo, unsigned char* inputFrame, int size,
	unsigned char* destBuffer)

{

	drawing = 1;
	memoryCodeBook* v4_codebook, * v1_codebook, * codebook = NULL;
	unsigned long
		cnum, strip_id, chunk_id, ci, flag, mask;
	long  chunk_size;
	unsigned long x, y;
	signed long	top_size, y_bottom, x0, y0, x1, y1;
	unsigned char* curDest;
	unsigned int i, cur_strip;
	int d0, d1, d2, d3;

	y = 0;
	y_bottom = 0;
	unsigned char* in_buffer = inputFrame;
	KHeader* CinePakhdr = (KHeader*)in_buffer;
	in_buffer += sizeof(KHeader);

	curDest = destBuffer;

	if (CinePakhdr->length != size)
	{
		if (CinePakhdr->length & 0x01) CinePakhdr->length++; /* AVIs tend to have a size mismatch */
		if (CinePakhdr->length != size)
		{

			//ERR("CVID: corruption %d (QT/AVI) != %ld (CV)\n", size, len);
			return BADSTRIP;

		}

	}


	free_codebooks(memorycvinfo);
	i = 0;

	if (CinePakhdr->strips > maxNum)
	{
		maxNum = CinePakhdr->strips;
	}

	if (CinePakhdr->strips > memorycvinfo->strip_num)
	{
		if (CinePakhdr->strips >= MAX_STRIPS)
		{

			//	ERR("CVID: strip overflow (more than %d)\n", MAX_STRIPS);
			return BADSTRIP;

		}


		for (i = memorycvinfo->strip_num; i < CinePakhdr->strips; i++)//Init our codebooks.
		{
			InitCodeBook(memorycvinfo, i);

			//int cvidSize = sizeof(cvid_codebook);
			//sizeVar += 2 * sizeof(cvid_codebook) * 260;
			//codeBooks += 2;
			//#endif
		}
	}
	memorycvinfo->strip_num = CinePakhdr->strips;

	//TRACE("CVID: <%ld,%ld> strips %ld\n", cv_width, cv_height, strips);


	for (cur_strip = 0; cur_strip < CinePakhdr->strips; cur_strip++)
	{
		v4_codebook = memorycvinfo->v4_codebook[cur_strip];
		v1_codebook = memorycvinfo->v1_codebook[cur_strip];

		if (v4_codebook == nullptr)
		{
			memorycvinfo->v4_codebook[cur_strip] = (memoryCodeBook*)malloc(sizeof(memoryCodeBook) * 260);
			v4_codebook = memorycvinfo->v4_codebook[cur_strip];
		}
		if (v1_codebook == nullptr)
		{
			memorycvinfo->v1_codebook[cur_strip] = (memoryCodeBook*)malloc(sizeof(memoryCodeBook) * 260);
			v1_codebook = memorycvinfo->v1_codebook[cur_strip];
		}


		if ((cur_strip > 0) && (!(CinePakhdr->frame_flags & 0x01)))
		{
			v4_codebook = memorycvinfo->v4_codebook[cur_strip - 1];
			v1_codebook = memorycvinfo->v1_codebook[cur_strip - 1];
		}

		strip_id = get_ushort(&in_buffer);        /* 1000 = key strip, 1100 = iter strip */
		if (strip_id > 0x2000) {
			printf("aww shit");
		}
		top_size = get_ushort(&in_buffer);

		y1 = get_ushort(&in_buffer);

		y_bottom += y1;
		top_size -= 12;
		x = 0;

		/*	if (x1 != screenwidth)
				WARN("CVID: Warning x1 (%ld) != width (%d)\n", x1, screenwidth);*/

				//TRACE("   %d) %04lx %04ld <%ld,%ld> <%ld,%ld> yt %ld\n",
				//	cur_strip, strip_id, top_size, x0, y0, x1, y1, y_bottom);


		while (top_size > 0)
		{
			chunk_id = get_byte(in_buffer);
			chunk_size = get_ushort(&in_buffer);

			char sl[1024] = { 0 };
			sprintf_s(sl, 1023, "%04lx %04ld\n", chunk_id, chunk_size);
			OutputDebugStringA(sl);

			top_size -= chunk_size;
			chunk_size -= 4;

			switch (chunk_id)
			{
				/* -------------------- Codebook Entries -------------------- */
			case RegularRead:
			case RegularRead_v4:
				codebook = (chunk_id == RegularRead_v4 ? v1_codebook : v4_codebook);
				cnum = chunk_size / 6;
				for (i = 0; i < cnum; i++) read_codebook(&in_buffer, codebook + i, 0);
				break;

			case UcharRead:
			case UcharRead_v4:        /* 8 bit per pixel */
				codebook = (chunk_id == UcharRead_v4 ? v1_codebook : v4_codebook);
				cnum = chunk_size / 4;
				for (i = 0; i < cnum; i++) read_codebook(&in_buffer, codebook + i, 1);
				break;

			case RegularChunkRead:
			case RegularChunkRead_v4:
				codebook = (chunk_id == RegularChunkRead_v4 ? v1_codebook : v4_codebook);

				ci = 0;
				while (chunk_size > 0)
				{
					flag = get_long(&in_buffer);
					chunk_size -= 4;

					for (i = 0; i < 32; i++)
					{
						if (flag & 0x80000000)
						{
							chunk_size -= 6;
							read_codebook(&in_buffer, codebook + ci, 0);
						}

						ci++;
						flag <<= 1;
					}
				}
				KillChunk
					break;

			case UcharChunkRead:
			case UcharChunkRead_v4:        /* 8 bit per pixel */
				codebook = (chunk_id == UcharChunkRead_v4 ? v1_codebook : v4_codebook);

				ci = 0;
				while (chunk_size > 0)
				{
					flag = get_long(&in_buffer);
					chunk_size -= 4;

					for (i = 0; i < 32; i++)
					{
						if (flag & 0x80000000)
						{
							chunk_size -= 4;
							read_codebook(&in_buffer, codebook + ci, 1);
						}

						ci++;
						flag <<= 1;
					}
				}
				KillChunk
					break;

				/* -------------------- Frame -------------------- */
			case FiveBitRead:
				while ((chunk_size > 0) && (y < y_bottom))
				{
					flag = get_long(&in_buffer);
					chunk_size -= 4;

					for (i = 0; i < 32; i++)
					{
						if (y >= y_bottom) break;
						if (flag & 0x80000000)    /* 4 bytes per block */
						{
							d0 = get_byte(in_buffer);
							d1 = get_byte(in_buffer);
							d2 = get_byte(in_buffer);
							d3 = get_byte(in_buffer);
							cvid_v4_16(curDest + ((y * frm_stride) + (x * bpp)), destBuffer, frm_stride, v4_codebook + d0, v4_codebook + d1, v4_codebook + d2, v4_codebook + d3); chunk_size -= 4;

						}
						else        /* 1 byte per block */
						{
							cvid_v1_16(curDest + ((y * frm_stride) + (x * bpp)), destBuffer, frm_stride, v1_codebook + get_byte(in_buffer)); chunk_size--;
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

			case FiveBitFlagRead:
				while ((chunk_size > 0) && (y < y_bottom))
				{
					/* ---- flag bits: 0 = SKIP, 10 = V1, 11 = V4 ---- */
					flag = get_long(&in_buffer);
					chunk_size -= 4;
					mask = 0x80000000;

					while ((mask) && (y < y_bottom))
					{
						if (flag & mask)
						{
							if (mask == 1)
							{
								if (chunk_size < 0) break;
								flag = get_long(&in_buffer);
								chunk_size -= 4;
								mask = 0x80000000;
							}
							else mask >>= 1;

							if (flag & mask)        /* V4 */
							{
								d0 = get_byte(in_buffer);
								d1 = get_byte(in_buffer);
								d2 = get_byte(in_buffer);
								d3 = get_byte(in_buffer);
								cvid_v4_16(curDest + ((y * frm_stride) + (x * bpp)), destBuffer, frm_stride, v4_codebook + d0, v4_codebook + d1, v4_codebook + d2, v4_codebook + d3); chunk_size -= 4;
							}
							else        /* V1 */
							{
								cvid_v1_16(curDest + ((y * frm_stride) + (x * bpp)), destBuffer, frm_stride, v1_codebook + get_byte(in_buffer)); chunk_size--;
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

			case OneBitRead:        /* each byte is a V1 codebook */
				while ((chunk_size > 0) && (y < y_bottom))
				{
					cvid_v1_16(curDest + ((y * frm_stride) + (x * bpp)), destBuffer, frm_stride, v1_codebook + get_byte(in_buffer)); chunk_size--;
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

				char sl[1024] = { 0 };
				sprintf_s(sl, 1023, "CVID: unknown chunk_id %08lx\n", chunk_id);
				OutputDebugStringA(sl);
				KillChunk
					break;
			}
		}
	}

	return CinePakhdr->length;
}

//void LoadKinoVid(SmallBuffer* buf, kinoindex_entry** entries, unsigned char** moviPointer)
//{
//
//}


void Kinomet_Encode(unsigned char* src, int srcsize, vector<unsigned char>* dst)
{
	SmallBuffer* buf = new SmallBuffer(src, srcsize);
	{
		BITMAPINFOHEADER* bmpinf = nullptr;

		AVIStreamHeader* sthread = nullptr;
		MainAVIHeader* hdrz = nullptr;
		unsigned char* moviPointer = nullptr;
		LoadAviInfo(buf, &hdrz, &sthread, &bmpinf, &moviPointer);
		_avioldindex_entry* idxList = (_avioldindex_entry*)buf->GetCurrentBuffer();


	
		//Store what we need
		
			SmallBuffer* newFile = new SmallBuffer( srcsize);
			{
				unsigned char headr[4] = { 'K','I','N','O' };


				for (int i = 0; i < 4; i++)
				{
					newFile->WriteByte(headr[i]);
				}
				newFile->WriteByte(1);
				//Store the width and height

				newFile->WriteByte((unsigned char)bmpinf->biWidth);
				newFile->WriteByte((unsigned char)bmpinf->biHeight);


				unsigned short fps = sthread->dwRate;
				unsigned long totalFrames = hdrz->dwTotalFrames;


				newFile->WriteByte(fps);


				// we write curFrame

				cinepak_info* ci = decode_cinepak_init(240, 160);
				int curFrame = 0;
				std::vector< kinoindex_entry> headers;
				std::vector<unsigned char> rawFrames;
				int lastChunk = 0;
				while (curFrame < totalFrames)
				{
					char sl[1024] = { 0 };
					sprintf_s(sl, 1023, "Working on frame %d \n", curFrame);
					OutputDebugStringA(sl);
					_avioldindex_entry* cur = &idxList[curFrame];

					//Make sure we are a frame.
					if (cur->FourCC != TAG_00DC || cur->inf.dwSize == 0) {
						curFrame++;
						continue;
					}



					//REDESIGN 

					/*we have the headers buffer, and an overall frame buffer
					recode = run the encoder
					store new framesize then frame

					then write the headers and frames

					once working add compression

						current version saves like 10ish bytes per frame*/


					RawCineFrame* cineFrame = (RawCineFrame*)(&moviPointer[cur->inf.dwOffset]);
					int framesize = cineFrame->len;
					std::vector<unsigned char> rawFrame;
					int chunkLen = decode_cinepak_encode_kinometpack(ci, bmpinf->biWidth, bmpinf->biHeight, cineFrame->dat, cur->inf.dwSize, &rawFrame);
					kinoindex_entry kie = { 0 };
					kie.dwOffset = lastChunk;
					kie.dwSize = chunkLen;
					if (chunkLen == BADSTRIP || chunkLen < 0)
					{
						printf("ERROR");


					}
					else {

						//We have the new frame.
						//len
						int z = rawFrame.size();
						for (int i = 0; i < 4; i++)
						{
							rawFrames.push_back(((unsigned char*)&z)[i]);
						}


						//frame
						for (int i = 0; i < rawFrame.size(); i++)
						{
							rawFrames.push_back(rawFrame[i]);
						}



						sprintf_s(sl, 1023, "Oldsize: %04lx -> kie.dwOffset %04lx -> kie.dwSize  %04lx\n", framesize, lastChunk, chunkLen);
						OutputDebugStringA(sl);

						headers.push_back(kie);



						lastChunk += chunkLen;
					}
					curFrame++;
				}
				if (curFrame != totalFrames)
				{
					newFile->Write32(curFrame);
				}
				else {
					newFile->Write32(totalFrames);

				}

				//write header size 
				int sz = headers.size();

				newFile->Write32(headers.size());
				newFile->Write32(0xDEADBEEF);
				for (int i = 0; i < headers.size(); i++)
				{
					kinoindex_entry* k = &headers[i];

					
						newFile->Write32(k->dwOffset);
						newFile->Write32(k->dwSize);
					
				}

				newFile->Write32(0xDEADBEEF);

				for (int i = 0; i < rawFrames.size(); i++)
				{
					newFile->WriteByte(rawFrames[i]);
				}
			}
			int realSize = newFile->Pos();

			

		
			unsigned char* rawFile = newFile->GetBuffer();;

			for (int i = 0; i < realSize; i++)
			{
				dst->push_back(rawFile[i]);
			}

			delete newFile;
		
	}
	delete buf;
}