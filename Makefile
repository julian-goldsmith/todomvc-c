all: restc

restc: main.c env.c
	gcc -g -Wall -o restc main.c env.c `pkg-config fcgi jansson --cflags --libs`

main.c env.c: env.h

clean:
	rm -f restc
