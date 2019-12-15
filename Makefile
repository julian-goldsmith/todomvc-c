all: todomvc-c

todomvc-c: main.c env.c todo.c handlers.c
	gcc -g -O0 -Wall -o todomvc-c main.c env.c todo.c handlers.c `pkg-config fcgi jansson libpq --cflags --libs`

clean:
	rm -f todomvc-c

handlers.h: env.h todo.h
env.c: env.h
todo.c: todo.h
main.c handlers.c: handlers.h
