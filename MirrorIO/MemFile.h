#pragma once
#include <stdio.h>
#include <iostream>

class GBAU32Pointer {
public:	
	GBAU32Pointer( unsigned char* dat)
	{
		_data = dat;	
		_offset = -1;
	}

	GBAU32Pointer(unsigned long offs, unsigned char* dat)
	{
		_data = dat;
		
		NewPointer(_offset);//Sets the data
	}

	unsigned long &operator=(unsigned long newval)
	{
		*(unsigned long*)_data[_offset] = newval;	
		return newval;
	}	

	unsigned long operator==(unsigned long newval) 
	{
		return _value == *(unsigned long*)_data[_offset];
	}
	
private:
	unsigned long _offset;
	unsigned char* _data;
	unsigned long _value;

	/// <summary>
	/// Shouldn't be used?
	/// </summary>
	/// <param name="off"></param>
	void NewPointer(unsigned long off)
	{	
	    _offset = off & 0x7FFFFFF;

		_value = *(unsigned long*)_data[_offset];
	}
};

class MemFile
{
public:

	//Singleton reference
	static MemFile* currentFile;
    
	//Takes in a FILE pointer, and currents a copy of the file in memory.
	MemFile(FILE* fp);

	//Makes a blank mem file of the expected size
	MemFile(int size);

	//Creates a memfile from a given file name, fails if below expected size.
	MemFile(char* file, int expectedMinSize);

	//Returns the memfile size
	int FileSize();
	//Saves current file
	void save();

	//Saves to file.
	void save(char* file);

	//Seek an offset for opertations in memfile
	void seek(unsigned long offset);

	//Reads in size * count to destation from memfile
	void fread(void* dst, int count, int size);	

	//gets a char from current memfile
	unsigned char fgetc();
	//puts a char from current memfile
	unsigned char fputc(unsigned char val);
	//Writes in size * count to destation from memfile
	void fwrite(void* dst, int count, int size);

	//Checks filesize validity.
	bool ValidFileSize();

	//Returns memfile as a byte array.
	unsigned char* GetFile();
	
	//Returns current file index
	int ftell();

	//Cleanup
	~MemFile();
private: 
	unsigned char* _theFile; 
	long fileIndex; 
	char* fileName;
	int _expectedFileSize;
	int _fileSize;
};

