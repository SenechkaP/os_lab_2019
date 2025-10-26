CC := gcc
CFLAGS := -I.

all: libsum.a psum

sumlib.o: sumlib.c sumlib.h
	$(CC) $(CFLAGS) -c sumlib.c -o sumlib.o

libsum.a: sumlib.o
	ar rcs libsum.a sumlib.o

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c -o utils.o

psum: parallel_sum.c sumlib.h utils.h libsum.a utils.o
	$(CC) $(CFLAGS) parallel_sum.c utils.o libsum.a -o psum -lpthread

clean:
	rm -f *.o *.a psum
