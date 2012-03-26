#include <stdio.h>
#include "cpux86.h"

int main(void)
{
	CPUx86 *cpu;

	cpu = new_cpux86(1024*1024*32);
	mem_store_file(cpu, 0x00100000, "../jslinux/files/vmlinux26.bin");
	mem_store_file(cpu, 0x00400000, "../jslinux/files/root.bin");
	mem_store_file(cpu, 0x10000, "../jslinux/files/linuxstart.bin");
	cpu->eip = 0x10000;
	cpu_regist_eax(cpu) = 0x2000000;
	cpu_regist_ebx(cpu) = 0x200000;
	set_cpu_cr0(cpu, CR0_PE, 1);
	run_cpux86(cpu);
	delete_cpux86(cpu);

	return 0;
}
