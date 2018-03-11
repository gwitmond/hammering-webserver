# it's a makefile

CFLAGS	= `pkg-config --libs --cflags glib-2.0` -lhammer

btest: test.o json.o http.o
	gcc ${CFLAGS} -o $@ $^

test: btest
	./btest
