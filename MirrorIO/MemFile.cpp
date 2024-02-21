
#include "pch.h"
#include "MemFile.h"
//Static object 
MemFile* MemFile::currentFile;

MemFile::MemFile(FILE* fp)
{
	_expectedFileSize = 0;
	_fileSize = 0;
	_theFile = 0;
	fileIndex = 0;
	fileName = 0;

}
MemFile::MemFile(int size)
{
	_expectedFileSize = 0;
	_fileSize = 0;
	_theFile = 0;
	fileIndex = 0;
	fileName = 0;
}
unsigned char* MemFile::GetFile()
{
	return _theFile;
}
int MemFile::FileSize()
{
	return _fileSize;

}
bool MemFile::ValidFileSize()
{
	return _expectedFileSize <= _fileSize; 
}


MemFile::MemFile(char* file, int expectedMinSize)
{
	_expectedFileSize = expectedMinSize;
	FILE* FP = nullptr;
	fopen_s(&FP, file, "r+b");
	fileName = file;
	//get file size
	fseek(FP, 0, SEEK_END);
	_fileSize = ::ftell(FP);


	//read full size
	fseek(FP, 0,SEEK_SET);
	_theFile = new unsigned char[_fileSize];
	memset(_theFile, 0, _fileSize);
	::fread(_theFile, 1, _fileSize , FP);
	fclose(FP);
	FP = NULL;
}

//Saves to disk
void MemFile::save()
{
	save(fileName);
}
//Saves to disk by name
void MemFile::save(char* file)
{
	//std functions
	
	FILE* FP = nullptr;
	fopen_s(&FP, file, "w+b");

	::fseek(FP, 0, SEEK_SET);
	::fwrite(_theFile, _fileSize, 1,FP);
	::fclose(FP);
}
unsigned char MemFile::fgetc()
{
	return _theFile[fileIndex++];
}

void MemFile::seek(unsigned long offset)
{
	if (offset >= 32000000)
	{
		char buffer[1000] = { 0 };
		sprintf_s(buffer, 1000, "attempted to seek out of bounds, %d (0x%X)", offset, offset);
		throw new std::exception(buffer);
	}
	fileIndex = offset; 
}
void MemFile::fread(void* dst, int count, int size)
{
	int readSize = count*size;
	if (readSize + fileIndex > _fileSize)
	{
		readSize = this->_fileSize - readSize + fileIndex;
	}
	memcpy(dst, &_theFile[fileIndex], readSize);
	fileIndex += readSize;
}

void MemFile::fwrite(void* src, int count, int size)
{
	int readSize = count*size;
	if (readSize > 0x800000)
	{
		char buffer[1000] = { 0 };
		sprintf_s(buffer, 1000, "attempted to read %p size %d many times, write size: %d", &src, count, size );
		throw new std::exception(buffer);
	}
	if (readSize + fileIndex > this->_fileSize)
	{
		readSize = this->_fileSize - readSize + fileIndex;
	}
	memcpy(&_theFile[fileIndex],src,  readSize);
	fileIndex += readSize;	
}

unsigned char MemFile::fputc(unsigned char val)
{
	_theFile[fileIndex++]=val;
	return _theFile[fileIndex-1];
}

int MemFile::ftell()
{
	return fileIndex;
}

MemFile::~MemFile()
{
	delete[] _theFile;
}
