all: libc libf
	gcc -fPIC -c main.c
	gcc -o main main.o -L$(CURDIR) $(CURDIR)/libf.so
	ldd main

libf:
	gcc -fPIC -c libf.c
	gcc -shared -o libf.so -nostdlib -static-libgcc libf.o

libc:
	gcc -c -fPIC libc.c
	ar rv libc.a libc.o

clean:
	rm -f libf.so libc.a libc.o libf.o main.o main
