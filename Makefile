all: restc

# TODO: pkg-config
restc: main.c
	gcc -o restc -g main.c -lfcgi

clean:
	rm -f restc
