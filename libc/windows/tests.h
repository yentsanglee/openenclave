#ifndef _OELIBC_TESTS_H
#define _OELIBC_TESTS_H

void __test_failed(const char* cond, const char* file, size_t line, const char* func);

#define TEST(COND) \
    do \
    { \
        if (!COND) \
        { \
            __test_failed(#COND, __FILE__, __LINE__, __FUNCTION__); \
        } \
    } \
    while (0)

void test_string(void);

void test_stdarg(void);

void test_pow(void);

#endif /* _OELIBC_TESTS_H */
