#include "CodeBookManager.h"

CodeBookManager::CodeBookManager()
{
	strip = 0;
	v4Book = nullptr;
	v1Book = nullptr;
}
void CodeBookManager::AddCodeBook(RawCodeBook* book)
{

}

void CodeBookManager::PopLatestCodeBook()
{
	codePointers.pop_back();
	strip;
	SetCodeBook(strip);
}
void CodeBookManager::AddCodeBook(int strip)
{

}

void CodeBookManager::SetCodeBook(int thisStrip)
{

}
memoryCodeBook* CodeBookManager::GetCodeBook(int thisStrip, int version)
{

	//is index loaded? 
	//Load index 
	//How much space do we need? 
//	if(version == 0) return this.
	return nullptr;
}

