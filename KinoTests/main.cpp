

#include <iostream>
#include <vector>
#include "..\Kinomet\Kinomet.h"
#include "..\KiNomet\Kinomet_Pack.h"
#include <stdio.h>
#include "..\MirrorIO\LargeBuffer.h"
struct k {
	unsigned long a;
	unsigned short b;
};

void fail()
{
	printf("lol");
	exit(-1);
}

void SmallBufferTest()
{
	unsigned char tmp = 0xFB;
	unsigned short tmp1 = 0x01cF;
	unsigned long tmp2 = 0x8123456;
	k kl = { 0xDEADBEEF, 0xDEAD };
	FILE* fp = nullptr;

	unsigned char* tmpArray = new unsigned char[1 + 2 + 4 + 4 + 2];

	SmallBuffer* b = new SmallBuffer(tmpArray, 1 + 2 + 4 + 4 + 2);
	b->WriteByte(tmp);
	b->Write16(tmp1);
	b->Write32(tmp2);
	b->Write32(kl.a);
	b->Write16(kl.b);
	fopen_s(&fp, "heytest.gba", "w+b");
	if (fp)
	{
		fwrite(b->GetBuffer(), 1, b->Pos(), fp);
		fclose(fp);
	}


	fopen_s(&fp, "heytest.gba", "r+b");
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* tmpArray2 = new unsigned char[len];
	fread(tmpArray2, 1, len, fp);

	fclose(fp);

	SmallBuffer* sb2 = new SmallBuffer(tmpArray2, len);


	b->Seek(0, SEEK_SET);//Reset old bufer
	sb2->Seek(0, SEEK_SET);

	if (b->GetByte() != sb2->GetByte())
	{
		fail();
	}

	if (b->Read16() != sb2->Read16())
	{
		fail();
	}
	if (b->Read32() != sb2->Read32())
	{
		fail();
	}
	if (b->Read32() != sb2->Read32())
	{
		fail();
	}
	if (b->Read16() != sb2->Read16())
	{
		fail();
	}

	delete b;
	delete sb2;
	delete[] tmpArray;
	delete[] tmpArray2;
}



void largeBufferTest()
{
	unsigned char tmp = 0xFB;
	unsigned short tmp1 = 0x01cF;
	unsigned long tmp2 = 0x8123456;
	k kl = { 0xDEADBEEF, 0xDEAD };
	FILE* fp = nullptr;

	unsigned char* tmpArray = new unsigned char[1 + 2 + 4 + 4 + 2];

	LargeBuffer* b = new LargeBuffer(tmpArray, 1 + 2 + 4 + 4 + 2);
	b->WriteByte(tmp);
	b->Write16(tmp1);
	b->Write32(tmp2);
	b->Write32(kl.a);
	b->Write16(kl.b);
	fopen_s(&fp, "heytest.gba", "w+b");
	if (fp)
	{
		fwrite(b->GetBuffer(), 1, b->Pos(), fp);
		fclose(fp);
	}


	fopen_s(&fp, "heytest.gba", "r+b");
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* tmpArray2 = new unsigned char[len];
	fread(tmpArray2, 1, len, fp);

	fclose(fp);

	LargeBuffer* sb2 = new LargeBuffer(tmpArray2, len);


	b->Seek(0, SEEK_SET);//Reset old bufer
	sb2->Seek(0, SEEK_SET);

	if (b->GetByte() != sb2->GetByte())
	{
		fail();
	}

	if (b->Read16() != sb2->Read16())
	{
		fail();
	}
	if (b->Read32() != sb2->Read32())
	{
		fail();
	}
	if (b->Read32() != sb2->Read32())
	{
		fail();
	}
	if (b->Read16() != sb2->Read16())
	{
		fail();
	}

	delete b;
	delete sb2;
	delete[] tmpArray;
	delete[] tmpArray2;
}




void endianBufferTest()
{
	unsigned char tmp = 0xFB;
	unsigned short tmp1 = 0x01cF;
	unsigned long tmp2 = 0x8123456;
	k kl = { 0xDEADBEEF, 0xDEAD };
	FILE* fp = nullptr;

	unsigned char* tmpArray = new unsigned char[1 + 2 + 4 + 4 + 2];

	SmallBuffer* b = new SmallBuffer(tmpArray, 1 + 2 + 4 + 4 + 2);
	b->WriteByte(tmp);
	b->Write16(tmp1);
	b->Write32(tmp2);
	b->Write32(kl.a);
	b->Write16(kl.b);
	fopen_s(&fp, "heytest.gba", "w+b");
	if (fp)
	{
		fwrite(b->GetBuffer(), 1, b->Pos(), fp);
		fclose(fp);
	}


	fopen_s(&fp, "heytest.gba", "r+b");
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* tmpArray2 = new unsigned char[len];
	fread(tmpArray2, 1, len, fp);

	fclose(fp);

	LargeBuffer* sb2 = new LargeBuffer(tmpArray2, len);


	b->Seek(0, SEEK_SET);//Reset old bufer
	b->SetEndian(BE);
	sb2->Seek(0, SEEK_SET);
	//Non byte should be not equal so we fail for test there.
	if (b->GetByte() != sb2->GetByte())
	{
		fail();
	}

	if (b->Read16() == sb2->Read16())
	{
		fail();
	}
	if (b->Read32() == sb2->Read32())
	{
		fail();
	}
	if (b->Read32() == sb2->Read32())
	{
		fail();
	}
	if (b->Read16() == sb2->Read16())
	{
		fail();
	}
	delete b;
	delete sb2;
	delete[] tmpArray;
	delete[] tmpArray2;
}


int main(int argc, char* argv[])
{

	printf("SmallBuffer fileio test\n");
	SmallBufferTest();
	printf("SmallBuffer fileio test pass\n");


	printf("largeBuffer fileio test\n");
	largeBufferTest();
	printf("largeBuffer fileio test pass\n");

	printf("endianBuffer fileio test\n");
	endianBufferTest();
	printf("endianBuffer fileio test pass\n");

  //TEST 1  
  //READ WRITE SMALLBUFFER
 

//TEST2 
	//READ WRITE LARGERBUFFER

	///TEst3 TEST BETWEEN LARGE AND SMALL BUFFER


}
