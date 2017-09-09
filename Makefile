all: ep1.o
	gcc ep1-servidor-exemplo.o -o ep1

ep1.o: ep1-servidor-exemplo.c
	gcc -c ep1-servidor-exemplo.c

clean:
	rm *o ep1