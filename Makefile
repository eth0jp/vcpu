
all: bootlinux bootbin

cpux86.o: cpux86.c
	gcc -O -c cpux86.c -o cpux86.o -w -Wall

# bootlinux
bootlinux.o: bootlinux.c
	gcc -O -c bootlinux.c -o bootlinux.o -w -Wall

bootlinux: cpux86.o bootlinux.o
	gcc -O cpux86.o bootlinux.o -o bootlinux -w -Wall

# bootbin
bootbin.o: bootbin.c
	gcc -O -c bootbin.c -o bootbin.o -w -Wall

bootbin: cpux86.o bootbin.o
	gcc -O cpux86.o bootbin.o -o bootbin -w -Wall
