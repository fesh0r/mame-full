//============================================================
//
//	strconv.h - Win32 MESS's string conversion functions
//
//	This provides similar string conversion macros to the ones
//	in ATL (e.g.  A2T, A2W, W2T) etc.  Additionally, UTF-8
//	string conversion is also supported
//
//============================================================

#ifndef STRCONV_H
#define STRCONV_H

#include <windows.h>

size_t copy_a2w_len(int codepage, const char *source);
size_t copy_w2a_len(int codepage, const WCHAR *source);
void copy_a2w(int codepage, WCHAR *str, const char *source);
void copy_w2a(int codepage, char *str, const WCHAR *source);

INLINE WCHAR *copy_a2w_inline(int codepage, WCHAR *str, const char *source)
{
	copy_a2w(codepage, str, source);
	return str;
}

INLINE char *copy_w2a_inline(int codepage, char *str, const WCHAR *source)
{
	copy_w2a(codepage, str, source);
	return str;
}

#define A2W_CP(codepage, source)	\
	((source) ? (copy_a2w_inline((codepage), (WCHAR *) alloca(copy_a2w_len((codepage), (source))), (source))) : NULL)

#define W2A_CP(codepage, source)	\
	((source) ? (copy_w2a_inline((codepage), (char *) alloca(copy_w2a_len((codepage), (source))), (source))) : NULL)

#define A2W(source)	A2W_CP(CP_ACP, (source))
#define W2A(source)	W2A_CP(CP_ACP, (source))
#define U2W(source)	A2W_CP(CP_UTF8, (source))
#define W2U(source)	W2A_CP(CP_UTF8, (source))

#define U2A(source)	W2A(U2W(source))
#define A2U(source)	W2U(A2W(source))

// These macros implement TCHAR conversion in terms of A2W and W2A
#ifdef UNICODE
#define A2T(source)	A2W(source)
#define T2A(source)	W2A(source)
#define W2T(source)	(source)
#define T2W(source)	(source)
#define U2T(source)	U2W(source)
#define T2U(source)	W2U(source)
#else // !UNICODE
#define A2T(source)	(source)
#define T2A(source)	(source)
#define W2T(source)	W2A(source)
#define T2W(source)	A2W(source)
#define U2T(source)	U2A(source)
#define T2U(source)	A2U(source)
#endif // UNICODE

#endif // STRCONV_H
