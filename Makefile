all: restc

restc: main.c env.c todo.c
	gcc -g -Wall -o restc main.c env.c todo.c `pkg-config fcgi jansson --cflags --libs`

main.c env.c: env.h
main.c todo.c: todo.h

clean:
	rm -f restc
