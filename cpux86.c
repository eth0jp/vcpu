#include <stdio.h>
#include <stdlib.h>

typedef char int8;
typedef short int16;
typedef int int32;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#define EFLAGS_CF	0x00000001
#define EFLAGS_PF	0x00000004
#define EFLAGS_AF	0x00000010
#define EFLAGS_ZF	0x00000040
#define EFLAGS_SF	0x00000080
#define EFLAGS_TF	0x00000100
#define EFLAGS_IF	0x00000200
#define EFLAGS_DF	0x00000400
#define EFLAGS_OF	0x00000800
#define EFLAGS_IOPL	0x00003000
#define EFLAGS_NT	0x00004000
#define EFLAGS_RF	0x00010000
#define EFLAGS_VM	0x00020000
#define EFLAGS_AC	0x00040000
#define EFLAGS_VIF	0x00080000
#define EFLAGS_VIP	0x00100000
#define EFLAGS_ID	0x00200000

#define EFLAGS_CF_SHIFT		0
#define EFLAGS_PF_SHIFT		2
#define EFLAGS_AF_SHIFT		4
#define EFLAGS_ZF_SHIFT		6
#define EFLAGS_SF_SHIFT		7
#define EFLAGS_TF_SHIFT		8
#define EFLAGS_IF_SHIFT		9
#define EFLAGS_DF_SHIFT		10
#define EFLAGS_OF_SHIFT		11
#define EFLAGS_IOPL_SHIFT	12
#define EFLAGS_NT_SHIFT		14
#define EFLAGS_RF_SHIFT		16
#define EFLAGS_VM_SHIFT		17
#define EFLAGS_AC_SHIFT		18
#define EFLAGS_VIF_SHIFT	19
#define EFLAGS_VIP_SHIFT	20
#define EFLAGS_ID_SHIFT		21

#define set_cpu_eflags(cpu, type, val)	(cpu->eflags ^= ((val) << type##_SHIFT) ^ (type & cpu->eflags))
#define cpu_eflags(cpu, type)			((cpu->eflags & type) >> type##_SHIFT)


typedef struct {
	// 汎用レジスタ
	// regs[0]: eax アキュムレータ
	// regs[1]: ecx カウンタ
	// regs[2]: edx データ
	// regs[3]: ebx ベース
	// regs[4]: esp スタックポインタ
	// regs[5]: ebp スタックベースポインタ
	// regs[6]: esi ソース
	// regs[7]: edi デスティネーション
	uint32 regs[8];
	// セグメントレジスタ
	uint32 ss;	// スタックセグメント
	uint32 cs;	// コードセグメント
	uint32 ds;	// データセグメント
	uint32 es;	// エクストラセグメント
	uint32 fs;	// Fセグメント
	uint32 gs;	// Gセグメント
	// EFLAGSレジスタ
	uint32 eflags;
	// 命令ポインタ
	uint32 eip;
	// メモリ
	uint8 *mem;
	size_t mem_size;
	// 処理中
	uint8 modrm;
	uint8 sib;
} CPUx86;

#define cpu_regist_eax(cpu)	cpu->regs[0]
#define cpu_regist_ecx(cpu)	cpu->regs[1]
#define cpu_regist_edx(cpu)	cpu->regs[2]
#define cpu_regist_ebx(cpu)	cpu->regs[3]
#define cpu_regist_esp(cpu)	cpu->regs[4]
#define cpu_regist_ebp(cpu)	cpu->regs[5]
#define cpu_regist_esi(cpu)	cpu->regs[6]
#define cpu_regist_edi(cpu)	cpu->regs[7]


typedef union {
	uint8 uint8;
	uint16 uint16;
	uint32 uint32;
} uintx;


// mem

#define mem_store8(cpu, idx, value)		((uint8*)cpu->mem)[(idx)] = (value)&0xFF;
#define mem_store_file(cpu, idx, fname)	{FILE *fp = fopen(fname, "rb"); mem_store_fp(cpu, idx, fp); fclose(fp);}
#define mem_store_fp(cpu, idx, fp)		{uint32 tidx=idx; uint32 tmp; while((tmp=fgetc(fp))!=EOF) {mem_store8(cpu, tidx++, tmp);}}

inline uint8 mem_load8(CPUx86 *cpu)
{
	uint8 val = cpu->mem[cpu->eip];
	cpu->eip += 1;
	return val;
}

inline uint16 mem_load16(CPUx86 *cpu)
{
	uint16 val = cpu->mem[cpu->eip] + (cpu->mem[cpu->eip+1]<<8);
	cpu->eip +=2;
	return val;
}

inline uint32 mem_load32(CPUx86 *cpu)
{
	uint32 val = cpu->mem[cpu->eip] + (cpu->mem[cpu->eip+1]<<8) + (cpu->mem[cpu->eip+2]<<16) + (cpu->mem[cpu->eip+3]<<24);
	cpu->eip +=4;
	return val;
}

inline void* mem_ptr(CPUx86 *cpu)
{
	return &(cpu->mem[cpu->eip]);
}


// Mod R/M

inline void mem_load_modrm(CPUx86 *cpu)
{
	cpu->modrm = mem_load8(cpu);
	if (cpu->modrm&0xC0!=0xC0 && cpu->modrm&38==0x20) {
		cpu->sib = mem_load8(cpu);
	} else {
		cpu->sib = 0;
	}
}

inline int modrm_mod(CPUx86 *cpu)
{
	return cpu->modrm>>6 & 0x03;
}

inline int modrm_reg(CPUx86 *cpu)
{
	return cpu->modrm>>3 & 0x07;
}

inline int modrm_rm(CPUx86 *cpu)
{
	return cpu->modrm & 0x07;
}

inline int is_modrm_r(CPUx86 *cpu)
{
	//return modrm->modrm>>3&0x07 && modrm->modrm&0x07;
	return modrm_reg(cpu) && modrm_rm(cpu);
}


// stack

inline void stack_push(CPUx86 *cpu, uint32 val)
{
	cpu_regist_esp(cpu) -= 4;
	cpu->mem[cpu->ss + cpu_regist_esp(cpu)] = val;
}


// call

inline void call(CPUx86 *cpu, int32 val)
{
	stack_push(cpu, cpu->eip);
	cpu->eip = cpu->eip + val;
}

inline void ret(CPUx86 *cpu, uint32 val)
{
}


// dump

inline void int2bin(char *dest, int val, int bitlen)
{
	int i;
	for (i=0; i<bitlen; i++) {
		dest[i] = val & 1<<(bitlen-1-i) ? '1' : '0';
	}
	if (i<sizeof(dest)) {
		dest[i] = '\0';
	}
}

inline void dump_cpu(CPUx86 *cpu)
{
	char b[33];
	int i;
	char *regs_arr[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};

	printf("dump_cpu:\n");
	printf("  eip: 0x%X\n", cpu->eip);

	for (i=0; i<8; i++) {
		printf("  %s: 0x%X\n", regs_arr[i], cpu->regs[i]);
	}

	printf("  eflags: 0x%X\n", cpu->eflags);

	int2bin(b, cpu->modrm, 8);
	printf("  modrm: 0x%X mod: %c%c reg: %c%c%c rm: %c%c%c\n", cpu->modrm, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
}


// cpu

CPUx86* new_cpux86(size_t mem_size)
{
	CPUx86 *cpu = malloc(sizeof(CPUx86));
	cpu->eflags = 2;
	cpu->mem = (uint8*)malloc(mem_size);
	cpu->mem_size = mem_size;
	return cpu;
}

void delete_cpux86(CPUx86 *cpu)
{
	if (cpu) {
		if (cpu->mem) {
			free(cpu->mem);
		}
		free(cpu);
	}
}

void exec_cpux86(CPUx86 *cpu)
{
	uint8 prefix;
	uint8 opcode;
	int c=0;
	int i;

/*
	for (i=0x3000; i<0x3010; i++) {
		printf("%08X: %02X\n", i, cpu->mem[i]);
	}
*/

	while (c++<20) {
		dump_cpu(cpu);

		opcode = mem_load8(cpu);

		printf("eip: %08X opcode: %X\n", cpu->eip-1, opcode);

		// prefix
		switch (opcode) {
		case 0x66:
			// オペランドサイズプリフィックス
		case 0x67:
			// アドレスサイズプリフィックス
		case 0x26:
		case 0x2E:
		case 0x36:
		case 0x3E:
		case 0x64:
		case 0x65:
			// セグメントオーバーライドプリフィックス
		case 0xF2:
		case 0xF3:
			// リピートプリフィックス
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F:
			// REXプリフィックス
		case 0xC4:
		case 0xC5:
			// VEXプリフィックス
			prefix = opcode;
			opcode = mem_load8(cpu);
			break;
		default:
			prefix = 0;
		}

		if (opcode!=0x0F) {
			// 1byte opcode

			switch (opcode) {
			// 0x50
			case 0x50:	// 50 sz : push eax
			case 0x51:	// 51 sz : push ecx
			case 0x52:	// 52 sz : push edx
			case 0x53:	// 53 sz : push ebx
			case 0x54:	// 54 sz : push esp
			case 0x55:	// 55 sz : push ebp
			case 0x56:	// 56 sz : push esi
			case 0x57:	// 57 sz : push edi
				stack_push(cpu, cpu->regs[opcode & 0x07]);
				break;

			// 0x80
			case 0x83:
				// 83 /0 ib sz : add r/m32 imm8
				// 83 /1 ib sz : or r/m32 imm8
				// 83 /2 ib sz : adc r/m32 imm8
				// 83 /3 ib sz : sbb r/m32 imm8
				// 83 /4 ib sz : and r/m32 imm8
				// 83 /5 ib sz : sub r/m32 imm8
				// 83 /6 ib sz : xor r/m32 imm8
				// 83 /7 ib sz : cmp r/m32 imm8
				mem_load_modrm(cpu);
				switch (modrm_mod(cpu)) {
				case 0x00:
					printf("todo opcode 0x83 mod 0x00\n");
					exit(1);
					break;
				case 0x01:
					printf("todo opcode 0x83 mod 0x01\n");
					exit(1);
					break;
				case 0x03:
					printf("todo opcode 0x83 mod 0x03\n");
					exit(1);
					break;
				case 0x04:
					switch (modrm_reg(cpu)) {
					case 0:
						cpu->regs[modrm_rm(cpu)] += mem_load8(cpu);
						break;
					case 1:
						cpu->regs[modrm_rm(cpu)] |= mem_load8(cpu);
						break;
					case 2:
						cpu->regs[modrm_rm(cpu)] += mem_load8(cpu) + cpu_eflags(cpu, EFLAGS_CF);
						// todo set flag: OF SF ZF AF PF
						printf("todo opcode 0x83 mod 0x04 reg 0x02\n");
						exit(1);
						break;
					case 3:
						cpu->regs[modrm_rm(cpu)] -= mem_load8(cpu) + cpu_eflags(cpu, EFLAGS_CF);
						// todo set flag: OF SF ZF AF PF CF
						printf("todo opcode 0x83 mod 0x04 reg 0x03\n");
						break;
					case 4:
						cpu->regs[modrm_rm(cpu)] &= mem_load8(cpu);
						// todo set flag: OF CF SF ZF PF
						printf("todo opcode 0x83 mod 0x04 reg 0x04\n");
						break;
					case 5:
						cpu->regs[modrm_rm(cpu)] -= mem_load8(cpu);
						// todo set flag: OF SF ZF AF PF CF
						printf("todo opcode 0x83 mod 0x04 reg 0x05\n");
						break;
					case 6:
						cpu->regs[modrm_rm(cpu)] ^= mem_load8(cpu);
						// todo set flag: OF CF SF ZF PF
						printf("todo opcode 0x83 mod 0x04 reg 0x06\n");
						break;
					case 7:
						set_cpu_eflags(cpu, EFLAGS_ZF, cpu->regs[modrm_rm(cpu)]==mem_load8(cpu) ? 1 : 0);
						// todo set flag: CF OF SF ZF AF PF
						printf("todo opcode 0x83 mod 0x04 reg 0x07\n");
						break;
					}
					break;
				}
				break;
			case 0x89:	// 89 /r sz : mov r/m32 r32
				mem_load_modrm(cpu);
				if (is_modrm_r(cpu)) {
					switch (modrm_mod(cpu)) {
					case 0x00:
						printf("todo opcode 0x89 mod 0x00\n");
						exit(1);
						break;
					case 0x01:
						printf("todo opcode 0x89 mod 0x01\n");
						exit(1);
						break;
					case 0x02:
						printf("todo opcode 0x89 mod 0x02\n");
						exit(1);
						break;
					case 0x03:
						cpu->regs[modrm_rm(cpu)] = cpu->regs[modrm_reg(cpu)];
						break;
					}
				}
				break;

			// 0xB0
			case 0xB8:	// B8 sz : mov eax imm32
			case 0xB9:	// B9 sz : mov ecx imm32
			case 0xBA:	// BA sz : mov edx imm32
			case 0xBB:	// BB sz : mov ebx imm32
			case 0xBC:	// BC sz : mov esp imm32
			case 0xBD:	// BD sz : mov ebp imm32
			case 0xBE:	// BE sz : mov esi imm32
			case 0xBF:	// BF sz : mov edi imm32
				cpu->regs[opcode & 0x07] = mem_load32(cpu);
				break;

			// 0xE0
			case 0xE8:	// E8 cd sz : call rel32
				call(cpu, mem_load32(cpu));
				break;

			// not implemented opcode
			default:
				printf("not implemented opcode: 0x%X\n", opcode);
				exit(1);
			}
		} else {
			// 2byte opcode
			printf("not implemented opcode: 0x%X\n", opcode);
			exit(1);
		}
	}
}

void run_cpux86(CPUx86 *cpu)
{
	exec_cpux86(cpu);
}

int main(void)
{
	CPUx86 *cpu;

	cpu = new_cpux86(1024*1024*5);
	mem_store_file(cpu, 0x10000, "../jslinux/files/linuxstart.bin");
	cpu->eip = 0x10000;
	cpu_regist_eax(cpu) = 0x2000000;
	cpu_regist_ebx(cpu) = 0x200000;
	run_cpux86(cpu);
	delete_cpux86(cpu);

	return 0;
}
