// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "print.h"
#include <openenclave/internal/syscall/unistd.h>
#include "lock.h"
#include "process.h"
#include "string.h"
#include "syscall.h"

ve_lock_t __ve_print_lock;

void ve_put(const char* s)
{
    ve_syscall3(OE_SYS_write, OE_STDOUT_FILENO, (long)s, (long)ve_strlen(s));
}

void ve_puts(const char* s)
{
    ve_lock(&__ve_print_lock);
    const char nl = '\n';
    ve_syscall3(OE_SYS_write, OE_STDOUT_FILENO, (long)s, (long)ve_strlen(s));
    ve_syscall3(OE_SYS_write, OE_STDOUT_FILENO, (long)&nl, 1);
    ve_unlock(&__ve_print_lock);
}

void ve_putc(char c)
{
    ve_syscall3(OE_SYS_write, OE_STDOUT_FILENO, (long)&c, 1);
}

typedef enum type
{
    TYPE_none,
    TYPE_s,
    TYPE_c,
    TYPE_p,
    TYPE_o,
    TYPE_u,
    TYPE_d,
    TYPE_i,
    TYPE_x,
    TYPE_X,
    TYPE_zu,
    TYPE_zd,
    TYPE_zi,
    TYPE_lu,
    TYPE_ld,
    TYPE_li,
    TYPE_lx,
    TYPE_llu,
    TYPE_lld,
    TYPE_lli,
    TYPE_llx,
} type_t;

static const char* _parse(const char* p, type_t* type)
{
    if (p[0] == 's')
    {
        *type = TYPE_s;
        p++;
    }
    else if (p[0] == 'c')
    {
        *type = TYPE_c;
    }
    else if (p[0] == 'o')
    {
        *type = TYPE_o;
        p++;
    }
    else if (p[0] == 'u')
    {
        *type = TYPE_u;
        p++;
    }
    else if (p[0] == 'd')
    {
        *type = TYPE_d;
        p++;
    }
    else if (p[0] == 'i')
    {
        *type = TYPE_i;
        p++;
    }
    else if (p[0] == 'x')
    {
        *type = TYPE_x;
        p++;
    }
    else if (p[0] == 'X')
    {
        *type = TYPE_X;
        p++;
    }
    else if (p[0] == 'l' && p[1] == 'u')
    {
        *type = TYPE_lu;
        p += 2;
    }
    else if (p[0] == 'l' && p[1] == 'l' && p[2] == 'u')
    {
        *type = TYPE_llu;
        p += 3;
    }
    else if (p[0] == 'l' && p[1] == 'd')
    {
        *type = TYPE_ld;
        p += 2;
    }
    else if (p[0] == 'l' && p[1] == 'l' && p[2] == 'd')
    {
        *type = TYPE_lld;
        p += 3;
    }
    else if (p[0] == 'l' && p[1] == 'i')
    {
        *type = TYPE_li;
        p += 2;
    }
    else if (p[0] == 'l' && p[1] == 'l' && p[2] == 'i')
    {
        *type = TYPE_lli;
        p += 3;
    }
    else if (p[0] == 'l' && p[1] == 'x')
    {
        *type = TYPE_lx;
        p += 2;
    }
    else if (p[0] == 'l' && p[1] == 'X')
    {
        *type = TYPE_lx;
        p += 2;
    }
    else if (p[0] == 'l' && p[1] == 'l' && p[2] == 'x')
    {
        *type = TYPE_llx;
        p += 3;
    }
    else if (p[0] == 'l' && p[1] == 'l' && p[2] == 'X')
    {
        *type = TYPE_llx;
        p += 3;
    }
    else if (p[0] == 'z' && p[1] == 'u')
    {
        *type = TYPE_zu;
        p += 2;
    }
    else if (p[0] == 'z' && p[1] == 'd')
    {
        *type = TYPE_zd;
        p += 2;
    }
    else if (p[0] == 'z' && p[1] == 'i')
    {
        *type = TYPE_zi;
        p += 2;
    }
    else if (p[0] == 'p')
    {
        *type = TYPE_p;
        p += 1;
    }
    else
    {
        *type = TYPE_none;
    }

    return p;
}

static void _put_c(char c)
{
    ve_syscall3(OE_SYS_write, OE_STDOUT_FILENO, (long)&c, 1);
}

static void _put_s(const char* s)
{
    if (!s)
        s = "(null)";

    ve_syscall3(OE_SYS_write, OE_STDOUT_FILENO, (long)s, (long)ve_strlen(s));
}

static void _put_o(uint64_t x)
{
    ve_ostr_buf buf;
    ve_put(ve_ostr(&buf, x, NULL));
}

static void _put_d(int64_t x)
{
    ve_dstr_buf buf;
    ve_put(ve_dstr(&buf, x, NULL));
}

static void _put_u(uint64_t x)
{
    ve_ustr_buf buf;
    ve_put(ve_ustr(&buf, x, NULL));
}

static void _put_x(uint64_t x)
{
    ve_xstr_buf buf;
    ve_put(ve_xstr(&buf, x, NULL));
}

void ve_print(const char* format, ...)
{
    ve_lock(&__ve_print_lock);

    oe_va_list ap;
    const char* p = format;

    oe_va_start(ap, format);

    while (*p)
    {
        if (p[0] == '%')
        {
            type_t type;

            p = _parse(++p, &type);

            switch (type)
            {
                case TYPE_none:
                {
                    /* Skip unknown argument type. */
                    oe_va_arg(ap, uint64_t);
                    break;
                }
                case TYPE_s:
                {
                    _put_s(oe_va_arg(ap, char*));
                    break;
                }
                case TYPE_c:
                {
                    _put_c((char)oe_va_arg(ap, int));
                    break;
                }
                case TYPE_o:
                {
                    _put_o(oe_va_arg(ap, unsigned int));
                    break;
                }
                case TYPE_d:
                case TYPE_i:
                {
                    _put_d(oe_va_arg(ap, int));
                    break;
                }
                case TYPE_ld:
                case TYPE_li:
                case TYPE_lld:
                case TYPE_lli:
                {
                    _put_d(oe_va_arg(ap, int64_t));
                    break;
                }
                case TYPE_u:
                {
                    _put_u(oe_va_arg(ap, unsigned int));
                    break;
                }
                case TYPE_lu:
                case TYPE_llu:
                {
                    _put_u(oe_va_arg(ap, uint64_t));
                    break;
                }
                case TYPE_x:
                case TYPE_X:
                {
                    _put_x(oe_va_arg(ap, unsigned int));
                    break;
                }
                case TYPE_lx:
                case TYPE_llx:
                {
                    _put_x(oe_va_arg(ap, uint64_t));
                    break;
                }
                case TYPE_zd:
                case TYPE_zi:
                {
                    _put_d(oe_va_arg(ap, int64_t));
                    break;
                }
                case TYPE_zu:
                {
                    _put_u(oe_va_arg(ap, uint64_t));
                    break;
                }
                case TYPE_p:
                {
                    _put_x(oe_va_arg(ap, uint64_t));
                    break;
                }
            }
        }
        else
        {
            _put_c(*p++);
        }
    }

    oe_va_end(ap);

    ve_unlock(&__ve_print_lock);
}
