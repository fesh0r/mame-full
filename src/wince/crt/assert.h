#ifndef ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
int __cdecl _fail(const char *exp, const char *file, int lineno);
#define assert(exp) ((void) ((exp) || (_fail(#exp, __FILE__, __LINE__))))
#else /* !DEBUG */
#define assert(exp)	((void) 0)
#endif /* DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* ASSERT_H */

