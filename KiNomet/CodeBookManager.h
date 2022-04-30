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
	memoryCodeBook* v4Book; 
	memoryCodeBook* v1Book; 
public:
	CodeBookManager();
	memoryCodeBook* GetCodeBook(int thisStrip, int version);
	void SetCodeBook(int thisStrip);
	void AddCodeBook(RawCodeBook* book);
	void AddCodeBook(int strip);
	void PopLatestCodeBook();
};

