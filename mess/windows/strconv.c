//============================================================
//
//	strconv.c - Win32 MESS's string conversion functions
//
//============================================================

#include <stdlib.h>
#include "strconv.h"

int strconv_is_hibit_string(const char *source)
{
	if (!source)
		return 0;
	while(*source && ((*source & 0x80) == 0x00))
		source++;
	return *source != 0x00;
}



size_t strconv_copy_a2w_len(int codepage, const char *source)
{
	return MultiByteToWideChar(codepage, 0, source, -1, NULL, 0) * sizeof(WCHAR);
}



size_t strconv_copy_w2a_len(int codepage, const WCHAR *source)
{
	return WideCharToMultiByte(codepage, 0, source, -1, NULL, 0, NULL, NULL) * sizeof(char);
}



void strconv_copy_a2w(int codepage, WCHAR *str, const char *source)
{
	MultiByteToWideChar(codepage, 0, source, -1, str, 0x3FFFFFFF);
}



void strconv_copy_w2a(int codepage, char *str, const WCHAR *source)
{
	WideCharToMultiByte(codepage, 0, source, -1, str, 0x7FFFFFFF, NULL, NULL);
}

