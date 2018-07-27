# it's a makefile

CFLAGS	= `pkg-config --libs --cflags glib-2.0` -lhammer

all:	libhammering.a

libhammering.a: json.o http.o
	ar rcs $@ $^

json.o: json.c json.h parser-helpers.h

http.o:	http.c http.h parser-helpers.h

test.o: test.c json.h http.h parser-helpers.h test_suite.h

btest: test.o libhammering.a
	gcc ${CFLAGS} -o $@ $^

test: btest
	./btest

clean:
	rm -f *.o *.a btest
