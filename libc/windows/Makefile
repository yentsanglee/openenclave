TARGET=main.exe

TOP=..\..

MUSLDIR=$(TOP)\3rdparty\musl\musl

INCLUDES=
INCLUDES=$(INCLUDES) /I$(MUSLDIR)\arch\generic\bits
INCLUDES=$(INCLUDES) /I.
INCLUDES=$(INCLUDES) /Iinclude 
INCLUDES=$(INCLUDES) /Iinclude\internal 
INCLUDES=$(INCLUDES) /I$(MUSLDIR)
INCLUDES=$(INCLUDES) /I$(MUSLDIR)\include 
INCLUDES=$(INCLUDES) /I$(MUSLDIR)\src\internal


CLFLAGS=
CLFLAGS=$(CLFLAGS) /FI oelibcwarnings.h
CLFLAGS=$(CLFLAGS) /nologo 
CLFLAGS=$(CLFLAGS) /Wall 
CLFLAGS=$(CLFLAGS) /O1
CLFLAGS=$(CLFLAGS) /GS-
CLFLAGS=$(CLFLAGS) /Oi-

LINKFLAGS=
LINKFLAGS=$(LINKFLAGS) /nologo 
LINKFLAGS=$(LINKFLAGS) /nodefaultlib 
LINKFLAGS=$(LINKFLAGS) /entry:oemain 
LINKFLAGS=$(LINKFLAGS) /subsystem:console

SOURCES=
SOURCES=$(SOURCES) main.c
SOURCES=$(SOURCES) print.c
SOURCES=$(SOURCES) _fltused.c
SOURCES=$(SOURCES) write_stdout.c
SOURCES=$(SOURCES) write_stderr.c
SOURCES=$(SOURCES) test_string.c
SOURCES=$(SOURCES) __test_failed.c
SOURCES=$(SOURCES) exit.c
SOURCES=$(SOURCES) test_stdarg.c

# libc
SOURCES=$(SOURCES) ..\strerror.c

# ctype
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\ctype\tolower.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\ctype\toupper.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\ctype\iswalpha.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\ctype\towctrans.c

# string:
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\bcmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\bcopy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\bzero.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\index.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memccpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memchr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memcmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memmem.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memmove.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\mempcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memrchr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memset.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\rindex.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\stpcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\stpncpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strcasecmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strcasestr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strcat.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strchr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strchrnul.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strcmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strcspn.c
#SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strdup.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strerror_r.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strlcat.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strlcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strlen.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strncasecmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strncat.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strncmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strncpy.c
#SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strndup.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strnlen.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strpbrk.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strrchr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strsep.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strspn.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strstr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strtok.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strtok_r.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strverscmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\swab.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcpcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcpncpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcscasecmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcscasecmp_l.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcscat.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcschr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcscmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcscpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcscspn.c
#SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsdup.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcslen.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsncasecmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsncasecmp_l.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsncat.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsncmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsncpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsnlen.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcspbrk.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsrchr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsspn.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcsstr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcstok.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wcswcs.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wmemchr.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wmemcmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wmemcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wmemmove.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\wmemset.c


# math:
#SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\math\pow.c

OBJECTS=$(SOURCES:.c=.obj)

$(TARGET): $(OBJECTS)
        link $(LINKFLAGS) /out:$(TARGET) $(OBJECTS) kernel32.lib

.c.obj:
	cl $(CLFLAGS) $(INCLUDES) /c /Fo$@ $<

$(SOURCES): force_rebuild
force_rebuild:

clean:
	@ del /F /Q $(OBJECTS) $(TARGET)

