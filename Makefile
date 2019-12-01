all: restc

restc: main.c
	gcc -g -Wall -o restc main.c `pkg-config fcgi jansson --cflags --libs`

clean:
	rm -f restc
