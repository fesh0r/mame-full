//============================================================
//
//	strconv.c - Win32 MESS's string conversion functions
//
//============================================================

#include <stdlib.h>
#include "strconv.h"

size_t copy_a2w_len(int codepage, const char *source)
{
	return MultiByteToWideChar(codepage, 0, source, -1, NULL, 0) * sizeof(WCHAR);
}



size_t copy_w2a_len(int codepage, const WCHAR *source)
{
	return WideCharToMultiByte(codepage, 0, source, -1, NULL, 0, NULL, NULL) * sizeof(char);
}



void copy_a2w(int codepage, WCHAR *str, const char *source)
{
	MultiByteToWideChar(codepage, 0, source, -1, str, copy_a2w_len(codepage, source) / sizeof(WCHAR));
}



void copy_w2a(int codepage, char *str, const WCHAR *source)
{
	WideCharToMultiByte(codepage, 0, source, -1, str, copy_w2a_len(codepage, source) / sizeof(char), NULL, NULL);
}

