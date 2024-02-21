#pragma once
#include <vector>
using namespace std;
typedef struct kinofile {
	unsigned char hdr[4];
	unsigned char version;
	unsigned char Width;
	unsigned char Height;
	unsigned char fps;
	unsigned long totalFrames;

	unsigned long headerCount;
	//hdrs//
};
void Kinomet_Encode(unsigned char* src, int srcsize, vector<unsigned char>* dst);