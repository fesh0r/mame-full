//============================================================
//
//	strconv.h - Win32 MESS's string conversion functions
//
//	This provides similar string conversion macros to the ones
//	in ATL (e.g.  A2T, A2W, W2T) etc.  Additionally, UTF-8
//	string conversion is also supported
//
//	Unlike the ATL macros, these macros do some tricks to
//	eliminate the need for USES_CONVERSION
//
//============================================================

#ifndef STRCONV_H
#define STRCONV_H

#include <windows.h>
#include <malloc.h>

#ifdef __GNUC__
#if (__GNUC__ < 3)
#define UNEXPECTED_(exp)	(exp)
#else
#define UNEXPECTED_(exp)	 __builtin_expect((exp), 0)
#endif
#else
#define UNEXPECTED_(exp)	(exp)
#endif

// Used to check for 7-bit ASCII string, so that A2U and U2A
// can short circuit conversion
int strconv_is_hibit_string(const char *source);

size_t strconv_copy_a2w_len(int codepage, const char *source);
size_t strconv_copy_w2a_len(int codepage, const WCHAR *source);
void strconv_copy_a2w(int codepage, WCHAR *str, const char *source);
void strconv_copy_w2a(int codepage, char *str, const WCHAR *source);

INLINE WCHAR *strconv_copy_a2w_inline(int codepage, WCHAR *str, const char *source)
{
	strconv_copy_a2w(codepage, str, source);
	return str;
}

INLINE char *strconv_copy_w2a_inline(int codepage, char *str, const WCHAR *source)
{
	strconv_copy_w2a(codepage, str, source);
	return str;
}

#if 0
#define STRCONV_COPY(codepage, source, chartype, lenfunc, copyfunc)	\
	({const void *source_ = (source); copyfunc((codepage), (chartype *) alloca(lenfunc((codepage), (source_))), (source_));})
#else
#define STRCONV_COPY(codepage, source, chartype, lenfunc, copyfunc)	\
	(copyfunc((codepage), (chartype *) alloca(lenfunc((codepage), (source))), (source)))
#endif

#define A2W_CP(codepage, source, not_null)	\
	((not_null || (source)) ? STRCONV_COPY(codepage, source, WCHAR, \
		strconv_copy_a2w_len, strconv_copy_a2w_inline) : NULL)

#define W2A_CP(codepage, source, not_null)	\
	((not_null || (source)) ? STRCONV_COPY(codepage, source, char, \
		strconv_copy_w2a_len, strconv_copy_w2a_inline) : NULL)

#define A2W(source)	A2W_CP(CP_ACP, (source), 0)
#define W2A(source)	W2A_CP(CP_ACP, (source), 0)
#define U2W(source)	A2W_CP(CP_UTF8, (source), 0)
#define W2U(source)	W2A_CP(CP_UTF8, (source), 0)

// ANSI <==> UTF8; these can be hairy
#define U2A(source)	(UNEXPECTED_(strconv_is_hibit_string(source)) \
	? W2A_CP(CP_ACP,  A2W_CP(CP_UTF8, (source), 1), 1) : (source))
#define A2U(source)	(UNEXPECTED_(strconv_is_hibit_string(source)) \
	? W2A_CP(CP_UTF8, A2W_CP(CP_ACP,  (source), 1), 1) : (source))

// These macros implement TCHAR conversion in terms of A2W, W2A, U2W and U2A
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
