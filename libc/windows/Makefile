TARGET=main.exe

TOP=..\..

MUSLDIR=$(TOP)\3rdparty\musl\musl

INCLUDES=
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

# string:
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\stpcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strcmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memcmp.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memcpy.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\memset.c
SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\string\strlen.c

# math:
#SOURCES=$(SOURCES) ..\..\3rdparty\musl\musl\src\math\pow.c

OBJECTS=$(SOURCES:.c=.obj)

$(TARGET): $(OBJECTS)
        link $(LINKFLAGS) /out:$(TARGET) $(OBJECTS) kernel32.lib

.c.obj:
	cl $(CLFLAGS) $(INCLUDES) /c /Fo$@ $<

#$(SOURCES): force_rebuild
#force_rebuild:

clean:
	@ del /F /Q $(OBJECTS) $(TARGET)

