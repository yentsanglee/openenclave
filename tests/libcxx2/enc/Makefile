export PKG_CONFIG_PATH=$(HOME)/openenclave/build/pkgconfig

TOP=../../..

TARGET=$(TOP)/build/tests/libcxx2/enc/libcxx2_enc

CXX=clang++-7
CFLAGS=$(shell pkg-config oeenclave-clang++ --cflags)
LIBS=$(shell pkg-config oeenclave-clang++ --libs)

CFLAGS+=-Wno-unused-command-line-argument

INCLUDES+=-I/root/openenclave/3rdparty/libcxx/libcxx/test 
INCLUDES+=-I/root/openenclave/3rdparty/libcxx/libcxx/test/support
INCLUDES+=-I/root/openenclave/3rdparty/libcxx/libcxx/fuzzing

SOURCES=$(wildcard test[0-9]*.cpp)
OBJECTS=$(SOURCES:.cpp=.cpp.o)

all: $(OBJECTS)
	$(CXX) -c -o enc.o $(CFLAGS) $(INCLUDES) enc.cpp
	$(CXX) -c -o fuzzing.o $(CFLAGS) $(INCLUDES) fuzzing.cpp
	$(CXX) -c -o memory_resource.o $(CFLAGS) $(INCLUDES) memory_resource.cpp
	$(CXX) -o $(TARGET) $(OBJECTS) enc.o fuzzing.o memory_resource.o $(LIBS)

%.cpp.o : %.cpp
	$(CXX) -o $<.tmp -S $(CFLAGS) $(INCLUDES) $<
	sed -f sub.sed $<.tmp | sed "s/__main__/$*/g" > $<.s
	$(CXX) -c -o $@ $(CFLAGS) $(INCLUDES) $<.s

clean:
	rm -f $(TARGET) $(OBJECTS) enc.o fuzzing.o memory_resource.o

objects:
	echo $(OBJECTS)


distclean:
	rm -rf test* *.o *.s *.tmp
