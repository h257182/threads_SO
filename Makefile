all: programa

programa: programa.c
	gcc -o programa programa.c -lpthread

clean:
	rm -f programa