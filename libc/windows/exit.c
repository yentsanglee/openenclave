#include "oelibc.h"
#include "kernel32.h"

void exit(int status)
{
    ExitProcess((UINT)status);
}
