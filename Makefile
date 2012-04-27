
all: bootlinux bootbin

clean:
	-rm cpux86.o
	-rm log.o
	-rm bootlinux
	-rm bootlinux.o
	-rm bootbin
	-rm bootbin.o

# cpux86
cpux86.o: cpux86.h cpux86.c
	gcc -O -c cpux86.c -o cpux86.o -w -Wall

# log
log.o: log.h log.c
	gcc -O -c log.c -o log.o -w -Wall

# bootlinux
bootlinux.o: bootlinux.c
	gcc -O -c bootlinux.c -o bootlinux.o -w -Wall

bootlinux: cpux86.o log.o bootlinux.o
	gcc -O cpux86.o log.o bootlinux.o -o bootlinux -w -Wall

# bootbin
bootbin.o: bootbin.c
	gcc -O -c bootbin.c -o bootbin.o -w -Wall

bootbin: cpux86.o log.o bootbin.o
	gcc -O cpux86.o log.o bootbin.o -o bootbin -w -Wall
