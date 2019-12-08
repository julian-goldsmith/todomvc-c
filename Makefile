all: restc

restc: main.c env.c todo.c handlers.c
	gcc -g -Wall -pedantic -o restc main.c env.c todo.c handlers.c `pkg-config fcgi jansson --cflags --libs`
	#gcc -Wl,-s -flto -O3 -Wall -o restc main.c env.c todo.c `pkg-config fcgi jansson --cflags --libs --static`

clean:
	rm -f restc

handlers.h: env.h todo.h
env.c: env.h
todo.c: todo.h
main.c handlers.c: handlers.h
