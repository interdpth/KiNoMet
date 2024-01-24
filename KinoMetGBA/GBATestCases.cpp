//const char charArray[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()";
//volatile unsigned short* palmem = (unsigned short*)0x5000000;
//volatile unsigned short* vram = (unsigned short*)0x6000000;
//volatile unsigned char* regularMem = (unsigned char*)0x3000000;
////Copy 3 bytes to 0x5000000;
//unsigned short oldPal[] = { 0x1234, 0x5678 };
//unsigned char newpal[] = { 0x0a, 0x0b, 0x0c };
////void safe_full_copy(unsigned char* dst, unsigned char* src, int olen)
////{
////	unsigned int len = olen;
////	unsigned int bytesCopied = 0;
////	while (len > 0)
////	{
////		bytesCopied = safe_memcpy(dst, src, len);
////		len -= bytesCopied;
////	}
////}
//int Test1()
//{
//	memorybuffer buffer =
//	{
//		buffer.src = (unsigned char*)oldPal,
//		buffer.dst = (unsigned char*)palmem,
//		buffer.len = 4
//	};
//
//	safe_memcpy(&buffer);
//	int b = 0xDEADBEEF;
//	if (palmem[0] != 0x1234 && palmem[1] != 0x5678)
//	{
//		while (1);
//	}
//	return 0;
//}
//int Test2()
//{
//	//odd copy
//	memorybuffer buffer =
//	{
//		buffer.src = (unsigned char*)newpal,
//		buffer.dst = (unsigned char*)palmem,
//		buffer.len = 3
//	};
//	//Should copy 1 byte, then copy 2.
//	safe_memcpy(&buffer);
//	int b = 0xDEAD5EED;
//	if (palmem[0] != 0x0b0a && palmem[1] != 0x560c)
//	{
//		while (1);
//	}
//	return 0;
//}
//int Test3()
//{
//	memorybuffer buffer =
//	{
//		buffer.src = (unsigned char*)charArray,
//		buffer.dst = (unsigned char*)palmem,
//		buffer.len = 60,
//	};
//	safe_memcpy(&buffer);
//	int b = 0xBEEFBEEF;
//	for (int i = 0; i < 60; i++)
//	{
//		if (charArray[i] != (((char*)palmem)[i]))
//		{
//			goto loop;
//		}
//	}
//	return 0;
//loop:
//	while (1)
//	{
//		b = 0x5EEDBEEF;
//	}
//
//}
//
//
//int Test4()
//{
//
//	unsigned char* tmpVram = (unsigned char*)vram;
//	memorybuffer buffer =
//	{
//	   buffer.src = (unsigned char*)&charArray[37],
//		buffer.dst = &tmpVram[1],
//		buffer.len = 7,
//	};
//	safe_memcpy(&buffer);
//	int b = 0x5EED5EED;
//	for (int i = 0; i < 7; i++)
//	{
//		if (charArray[i + 37] != (((char*)vram)[i + 1]))
//		{
//			while (1);
//		}
//	}
//
//
//	return 0;
//}
//int Test()
//{
//
//
//
//	//even copy
//	int ret = Test1();
//	if (ret != 0) while (1);
//	;	//odd copy
//	ret = Test2();
//	if (ret != 0) while (1);
//
//	ret = Test3();
//	if (ret != 0) while (1);
//
//	ret = Test4();
//	if (ret != 0) while (1);
//
//	return 1;
//}
//
