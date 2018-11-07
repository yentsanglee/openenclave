#ifndef _OE_LIBC_WARNINGS
#define _OE_LIBC_WARNINGS

/* warning C4464: relative include path contains '..' */
#pragma warning(disable : 4464)

/* warning C4311: 'type cast': pointer truncation from 'X' to 'Y' */
#pragma warning(disable : 4311)

/* warning C4706: assignment within conditional expression */
#pragma warning(disable : 4706)

/* warning C4554: '<<': check operator precedence for possible error; use parentheses to clarify precedence */
#pragma warning(disable : 4554)

/* warning C4293: '>>': shift count negative or too big, undefined behavior */
#pragma warning(disable : 4293)

/* warning C4711: function 'test_strcpy' selected for automatic inline expansion */
#pragma warning(disable : 4711)

/* warning C4464: relative include path contains '..' */
#pragma warning(disable : 4464)

/* warning C4242: '=': conversion from 'int' to 'unsigned char', possible loss of data */
#pragma warning(disable : 4242)

/* warning C4244: '=': conversion from 'int' to 'unsigned char', possible loss of data */
#pragma warning(disable : 4244)

/* warning C4146: unary minus operator applied to unsigned type, result still unsigned */
#pragma warning(disable : 4146)

#endif /* _OE_LIBC_WARNINGS */
