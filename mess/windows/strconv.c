#include <stdlib.h>
#include "strconv.h"

size_t copy_a2w_len(const char *source)
{
	return MultiByteToWideChar(CP_ACP, 0, source, -1, NULL, 0) * sizeof(WCHAR);
}

size_t copy_w2a_len(const WCHAR *source)
{
	return WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL) * sizeof(char);
}

const WCHAR *copy_a2w(WCHAR *str, const char *source)
{
	MultiByteToWideChar(CP_ACP, 0, source, -1, str, copy_a2w_len(source) / sizeof(WCHAR));
	return str;
}

const char *copy_w2a(char *str, const WCHAR *source)
{
	WideCharToMultiByte(CP_ACP, 0, source, -1, str, copy_w2a_len(source) / sizeof(char), NULL, NULL);
	return str;
}

