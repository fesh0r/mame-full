#ifndef STRCONV_H
#define STRCONV_H

#include <windows.h>

size_t copy_a2w_len(const char *source);
size_t copy_w2a_len(const WCHAR *source);
const WCHAR *copy_a2w(WCHAR *str, const char *source);
const char *copy_w2a(char *str, const WCHAR *source);

#define A2W(source)	((source) ? (copy_a2w((WCHAR *) alloca(copy_a2w_len(source)), (source))) : NULL)
#define W2A(source)	((source) ? (copy_w2a((char *) alloca(copy_w2a_len(source)), (source))) : NULL)

#ifdef UNICODE
#define A2T(source)	A2W(source)
#define T2A(source)	W2A(source)
#define W2T(source)	(source)
#define T2W(source)	(source)
#else
#define A2T(source)	(source)
#define T2A(source)	(source)
#define W2T(source)	W2A(source)
#define T2W(source)	A2W(source)
#endif

#endif // STRCONV_H
