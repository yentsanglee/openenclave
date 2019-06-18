#ifndef _VE_HOST_GLOBALS_H
#define _VE_HOST_GLOBALS_H

typedef struct _globals
{
    int sock;
    int child_sock;
} globals_t;

extern globals_t globals;

#endif /* _VE_HOST_GLOBALS_H */
