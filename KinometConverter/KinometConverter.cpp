// KinometConverter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include "..\KiNomet\Cinepak.h"
#include "..\Kinomet\Kinomet.h"
#include "..\KiNomet\Kinomet_Pack.h"
int main(int argc, char* argv[])
{

	//File IO tests








	if (argc != 3) {
		printf("Can only support 2 parameters");
	}

	char* infile = argv[1];
	char* outfile = argv[2];
	int buferlen = 0;
	FILE* fp = nullptr;
	unsigned char* srcBuffer = nullptr;
	fopen_s(&fp, infile, "rb");

	if (fp) {
		fseek(fp, 0, SEEK_END);
		buferlen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		srcBuffer = new unsigned char[buferlen];
		fread(srcBuffer, 1, buferlen, fp);
		fclose(fp);

	}

	std::vector<unsigned char> theDst;

	FILE* fp2 = nullptr;
	fopen_s(&fp2, outfile, "wb");
	if (fp2) 
	{
		Kinomet_Encode(srcBuffer, buferlen, &theDst);
		unsigned char* dat = &theDst.front();
		fwrite((void*)dat, theDst.size(), 1,  fp2);
		fclose(fp2);
		delete[] srcBuffer;
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
