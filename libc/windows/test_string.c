#define _GNU_SOURCE
#include "tests.h"
#include <string.h>

void _test_bcmp(void)
{
    long long x = 0x1234567812345678;
    long long y = 0x1234567812345678;
    long long z = 0x8765432187654321;
    TEST(memcmp(&x, &y, 8) == 0);
    TEST(memcmp(&x, &z, 8) != 0);
}

void _test_bcopy(void)
{
    long long x = 0x1111111111111111;
    long long y = 0;
    bcopy(&x, &y, 8);
    TEST(memcmp(&x, &y, 8) == 0);
}

void _test_bzero(void)
{
	long long x = 0x1234567812345678;
	long long y = 0;
	bzero(&x, 8);
	TEST(memcmp(&x, &y, 8) == 0);
}

void _test_index(void)
{
	const char * s = "Sphinx of black quartz, judge my vow";
	TEST(index(s, 'z') == &s[21]);
}

void _test_memccpy(void)
{
	const char * s = "Pack my box with five dozen liquor jugs";
	char buf[1024] = {0};
	memccpy(buf, s, 'k', 1024);
	TEST(strcmp(buf, "Pack") == 0);
}

void _test_memchr(void)
{
	unsigned char x[] = { 0x12, 0x00, 0x56, 0x78, 0xff, 0xff, 0xff, 0xff };
	unsigned char * y = (unsigned char *)memchr(&x, 0x56, 8);
	TEST(*y == 0x56);
}

void _test_memcmp(void)
{
    TEST(memcmp("aaa", "aaa", 3) == 0);
}

void _test_memcpy(void)
{
    char buf[16];
    char * ret = memcpy(buf, "hello", 5);
    TEST(memcmp(buf, "hello", 5) == 0);
	TEST(ret == buf);
}

void _test_memmem(void)
{
	unsigned char haystack[] = { 0x0, 0x11, 0x22, 0x00, 0x33, 0x44, 0x00, 0x55 };
	unsigned char needle[] = { 0x33, 0x44 };
	unsigned char noexist_needle[] = { 0x33, 0x55 };

	unsigned char * ret = (unsigned char *)memmem(haystack, 8, needle, 2);

	TEST(ret[0] == 0x33);
	TEST(ret[1] == 0x44);

	ret = (unsigned char *)memmem(haystack, 8, noexist_needle, 2);
	TEST(ret == NULL);
}

void _test_memmove(void)
{
	unsigned char buf[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	memmove(&buf[5], &buf[0], 5);
	TEST(buf[5] == 0);
	TEST(buf[8] == 3);
}

void _test_mempcpy(void)
{
	unsigned char buf1[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	unsigned char buf2[16];
	unsigned char * ret = (unsigned char *)mempcpy(buf2, buf1, 10);
	TEST(buf2[7] == 7);
	TEST(buf2[7] != 6);
	TEST(ret == &buf2[10]);
}

void _test_memrchr(void)
{
	unsigned char x[] = { 0x12, 0x34, 0x56, 0x78, 0xff, 0x00, 0xff, 0xaa };
	unsigned char * y = (unsigned char *)memrchr(&x[0], 0x78, 8);
	TEST(*y == 0x78);
	TEST(y == &x[3]);
}

void _test_memset(void)
{
	unsigned char buf[16] = {0};
	memset(buf, 1, 16);
	TEST(buf[3] == 1);
	TEST(buf[6] == 1);
}

void _test_rindex(void)
{
	const char * s = "Five quacking zephyrs jolt my wax bed";
	TEST( (rindex(s, 'o') == &s[23]) );
}

void _test_stpcpy(void)
{
	char buf[1024] = {0};
	const char * s = "abc";
	char * ret = stpcpy(buf, s);
	TEST(ret[0] == '\0');
	TEST(ret[-1] == 'c');
	TEST(ret[-3] == 'a');
	TEST(ret == &buf[3]);
}

void _test_stpncpy(void)
{
	char buf[1024] = {0};
	const char * s = "abc";
	char * ret = stpncpy(buf, s, 10);
	TEST(ret[0] == '\0');
	TEST(ret[-1] == 'c');
	TEST(ret[-3] == 'a');
	TEST(ret == &buf[3]);
}

void _test_strcasecmp(void)
{
	const char * s1 = "aBc";
	const char * s2 = "AbC";
	const char * s3 = "CCC";
	TEST(strcasecmp(s1, s2) == 0);
	TEST(strcasecmp(s1, s3) != 0);
}

void _test_strcasestr(void)
{
	const char * s = "Waxy and quivering, jocks fumble the pizza.";
	char * ret = strcasestr(s, "fumble");
	TEST(ret != NULL);
	TEST(ret == &s[26]);
}

void _test_strcat(void)
{

}

void _test_strchr(void)
{

}

void _test_strchrnul(void)
{

}

void _test_strcmp(void)
{
    TEST(strcmp("aaa", "aaa") == 0);
}

void _test_strcpy(void)
{
    char buf[1024];
    strcpy(buf, "hello");
    TEST(strcmp(buf, "hello") == 0);
}

void _test_strcspn(void)
{

}

void _test_strdup(void)
{

}

void _test_strerror_r(void)
{

}

void _test_strlcat(void)
{

}

void _test_strlcpy(void)
{

}

void _test_strlen(void)
{

}

void _test_strncasecmp(void)
{

}

void _test_strncat(void)
{

}

void _test_strncmp(void)
{

}

void _test_strncpy(void)
{

}

void _test_strndup(void)
{

}

void _test_strnlen(void)
{

}

void _test_strpbrk(void)
{

}

void _test_strrchr(void)
{

}

void _test_strsep(void)
{

}

void _test_strspn(void)
{

}

void _test_strstr(void)
{

}

void _test_strtok(void)
{

}

void _test_strtok_r(void)
{

}

void _test_strverscmp(void)
{

}

void _test_swab(void)
{

}

void _test_wcpcpy(void)
{

}

void _test_wcpncpy(void)
{

}

void _test_wcscasecmp(void)
{

}

void _test_wcscasecmp_l(void)
{

}

void _test_wcscat(void)
{

}

void _test_wcschr(void)
{

}

void _test_wcscmp(void)
{

}

void _test_wcscpy(void)
{

}

void _test_wcscspn(void)
{

}

void _test_wcsdup(void)
{

}

void _test_wcslen(void)
{

}

void _test_wcsncasecmp(void)
{

}

void _test_wcsncasecmp_l(void)
{

}

void _test_wcsncat(void)
{

}

void _test_wcsncmp(void)
{

}

void _test_wcsncpy(void)
{

}

void _test_wcsnlen(void)
{

}

void _test_wcspbrk(void)
{

}

void _test_wcsrchr(void)
{

}

void _test_wcsspn(void)
{

}

void _test_wcsstr(void)
{

}

void _test_wcstok(void)
{

}

void _test_wcswcs(void)
{

}

void _test_wmemchr(void)
{

}

void _test_wmemcmp(void)
{

}

void _test_wmemcpy(void)
{

}

void _test_wmemmove(void)
{

}

void _test_wmemset(void)
{

}


void test_string(void)
{
    _test_bcmp();
    _test_bcopy();
    _test_bzero();
    _test_index();
    _test_memccpy();
    _test_memchr();
    _test_memcmp();
    _test_memcpy();
    _test_memmem();
    _test_memmove();
    _test_mempcpy();
    _test_memrchr();
    _test_memset();
    _test_rindex();
    _test_stpcpy();
    _test_stpncpy();
    _test_strcasecmp();
    _test_strcasestr();
    _test_strcat();
    _test_strchr();
    _test_strchrnul();
    _test_strcmp();
    _test_strcpy();
    _test_strcspn();
    _test_strdup();
    _test_strerror_r();
    _test_strlcat();
    _test_strlcpy();
    _test_strlen();
    _test_strncasecmp();
    _test_strncat();
    _test_strncmp();
    _test_strncpy();
    _test_strndup();
    _test_strnlen();
    _test_strpbrk();
    _test_strrchr();
    _test_strsep();
    _test_strspn();
    _test_strstr();
    _test_strtok();
    _test_strtok_r();
    _test_strverscmp();
    _test_swab();
    _test_wcpcpy();
    _test_wcpncpy();
    _test_wcscasecmp();
    _test_wcscasecmp_l();
    _test_wcscat();
    _test_wcschr();
    _test_wcscmp();
    _test_wcscpy();
    _test_wcscspn();
    _test_wcsdup();
    _test_wcslen();
    _test_wcsncasecmp();
    _test_wcsncasecmp_l();
    _test_wcsncat();
    _test_wcsncmp();
    _test_wcsncpy();
    _test_wcsnlen();
    _test_wcspbrk();
    _test_wcsrchr();
    _test_wcsspn();
    _test_wcsstr();
    _test_wcstok();
    _test_wcswcs();
    _test_wmemchr();
    _test_wmemcmp();
    _test_wmemcpy();
    _test_wmemmove();
    _test_wmemset();

    print("==== passed test_string()\n");
}
