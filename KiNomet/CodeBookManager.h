#pragma once
#include "Cinepak.h"
typedef struct RawCodeBook
{
	int id;
	oldcvid_codebook* raw;
	int readcount;
};
 class CodeBookManager
{
	//Store two code books at a time
	//extra loaded on command for x uses

private:
	vector<RawCodeBook*> codePointers;
	int strip;
	cvid_codebook* v4Book; 
	cvid_codebook* v1Book; 
public:
	CodeBookManager();
	cvid_codebook* GetCodeBook(int thisStrip, int version);
	void SetCodeBook(int thisStrip);
	void AddCodeBook(RawCodeBook* book);
	void AddCodeBook(int strip);
	void PopLatestCodeBook();
};

