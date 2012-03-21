#include <stdio.h>
#include "cpux86.h"

int main(int argc, char *argv[])
{
	CPUx86 *cpu;

	if (argc!=2) {
		fprintf(stderr, "Usage: %s binaryfile\n", argv[0]);
		return 1;
	}

	cpu = new_cpux86(1024*1024*32);
	if (-1<mem_store_file(cpu, 0x00, argv[1])) {
		cpu->eip = 0x00;
		run_cpux86(cpu);
	} else {
		fprintf(stderr, "file read error\n");
	}
	delete_cpux86(cpu);

	return 0;
}
