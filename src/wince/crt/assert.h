#ifndef ASSERT_H

#ifndef NDEBUG
int _fail(const char *exp, const char *file, int lineno);
#define assert(exp) ((void) ((exp) || (_fail(#exp, __FILE__, __LINE__))))
#else /* !DEBUG */
#define assert(exp)	((void) 0)
#endif /* DEBUG */

#endif /* ASSERT_H */

