#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char int8;
typedef short int16;
typedef int int32;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;


typedef struct {
	uint16 limit;	// Table Limit
	uint32 base;	// Linear Base Address
} DescTableReg;

typedef struct {
	// 一般レジスタ群
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
	uint16 ss;	// スタックセグメント
	uint16 cs;	// コードセグメント
	uint16 ds;	// データセグメント
	uint16 es;	// エクストラセグメント
	uint16 fs;	// Fセグメント
	uint16 gs;	// Gセグメント
	// EFLAGSレジスタ
	uint32 eflags;
	// 命令ポインタ
	uint32 eip;

	// システムレジスタ群
	// システムアドレスレジスタ
	DescTableReg gdtr;	// Global Descriptor Table Register
	DescTableReg idtr;	// Interrupt Descriptor Table Register
	uint16 ldtr;		// Local Descriptor Table
	uint16 tr;
	// コントロールレジスタ
	uint32 cr0;
	uint32 cr1;
	uint32 cr2;
	uint32 cr3;

	// メモリ
	uint8 *mem;
	size_t mem_size;

	// 処理中
	struct {
		uint8 operand_size :1;	// 0x66 オペランドサイズプリフィックス
		uint8 address_size :1;	// 0x67 アドレスサイズプリフィックス
		uint8 segment_cs :1;	// 0x2E セグメントオーバーライドプリフィックス(CS)
		uint8 segment_ss :1;	// 0x36 セグメントオーバーライドプリフィックス(SS)
		uint8 segment_ds :1;	// 0x3E セグメントオーバーライドプリフィックス(DS)
		uint8 segment_es :1;	// 0x26 セグメントオーバーライドプリフィックス(ES)
		uint8 segment_fs :1;	// 0x64 セグメントオーバーライドプリフィックス(FS)
		uint8 segment_gs :1;	// 0x65 セグメントオーバーライドプリフィックス(GS)
		uint8 repne :1;			// 0xF2 リピートプリフィックス(REPNE/REPZE)
		uint8 rep :1;			// 0xF3 リピートプリフィックス(REP/REPE/REPZ)
		uint8 rex :4;			// 0x40~0x4F REXプリフィックス
		uint32 vex3;			// 0xC4 VEXプリフィックス
		uint16 vex2;			// 0xC5 VEXプリフィックス
	} prefix;
	uint8 modrm;
	uint8 sib;
} CPUx86;


void cpu_current_reset(CPUx86 *cpu)
{
	memset(&(cpu->prefix), 0, sizeof(cpu->prefix));
	cpu->modrm = 0;
	cpu->sib = 0;
}


// register

#define cpu_regist_eax(cpu)	cpu->regs[0]
#define cpu_regist_ecx(cpu)	cpu->regs[1]
#define cpu_regist_edx(cpu)	cpu->regs[2]
#define cpu_regist_ebx(cpu)	cpu->regs[3]
#define cpu_regist_esp(cpu)	cpu->regs[4]
#define cpu_regist_ebp(cpu)	cpu->regs[5]
#define cpu_regist_esi(cpu)	cpu->regs[6]
#define cpu_regist_edi(cpu)	cpu->regs[7]


// EFLAGS

#define CPU_EFLAGS_CF		0x00000001
#define CPU_EFLAGS_PF		0x00000004
#define CPU_EFLAGS_AF		0x00000010
#define CPU_EFLAGS_ZF		0x00000040
#define CPU_EFLAGS_SF		0x00000080
#define CPU_EFLAGS_TF		0x00000100
#define CPU_EFLAGS_IF		0x00000200
#define CPU_EFLAGS_DF		0x00000400
#define CPU_EFLAGS_OF		0x00000800
#define CPU_EFLAGS_IOPL		0x00003000
#define CPU_EFLAGS_NT		0x00004000
#define CPU_EFLAGS_RF		0x00010000
#define CPU_EFLAGS_VM		0x00020000
#define CPU_EFLAGS_AC		0x00040000
#define CPU_EFLAGS_VIF		0x00080000
#define CPU_EFLAGS_VIP		0x00100000
#define CPU_EFLAGS_ID		0x00200000

#define CPU_EFLAGS_CF_BIT	0
#define CPU_EFLAGS_PF_BIT	2
#define CPU_EFLAGS_AF_BIT	4
#define CPU_EFLAGS_ZF_BIT	6
#define CPU_EFLAGS_SF_BIT	7
#define CPU_EFLAGS_TF_BIT	8
#define CPU_EFLAGS_IF_BIT	9
#define CPU_EFLAGS_DF_BIT	10
#define CPU_EFLAGS_OF_BIT	11
#define CPU_EFLAGS_IOPL_BIT	12
#define CPU_EFLAGS_NT_BIT	14
#define CPU_EFLAGS_RF_BIT	16
#define CPU_EFLAGS_VM_BIT	17
#define CPU_EFLAGS_AC_BIT	18
#define CPU_EFLAGS_VIF_BIT	19
#define CPU_EFLAGS_VIP_BIT	20
#define CPU_EFLAGS_ID_BIT	21

#define set_cpu_eflags(cpu, type, val)	(cpu->eflags ^= ((val) << type##_BIT) ^ (type & cpu->eflags))
#define cpu_eflags(cpu, type)			((cpu->eflags & type) >> type##_BIT)


// cr0

#define CR0_PE		0x00000001
#define CR0_MP		0x00000002
#define CR0_EM		0x00000004
#define CR0_TS		0x00000008
#define CR0_ET		0x00000010
#define CR0_NE		0x00000020
#define CR0_WP		0x00010000
#define CR0_AM		0x00040000
#define CR0_NW		0x20000000
#define CR0_CD		0x40000000
#define CR0_PG		0x80000000

#define CR0_PE_BIT	0
#define CR0_MP_BIT	1
#define CR0_EM_BIT	2
#define CR0_TS_BIT	3
#define CR0_ET_BIT	4
#define CR0_NE_BIT	5
#define CR0_WP_BIT	16
#define CR0_AM_BIT	18
#define CR0_NW_BIT	29
#define CR0_CD_BIT	30
#define CR0_PG_BIT	31

#define set_cpu_cr0(cpu, type, val)	(cpu->cr0 ^= ((val) << type##_BIT) ^ (type & cpu->cr0))
#define cpu_cr0(cpu, type)			((cpu->cr0 & type) >> type##_BIT)


// segment descriptor

typedef struct {
	uint16 limitL;
	uint16 baseL;
	uint8 baseM;
	uint8 type;
	uint8 limitH;
	uint8 baseH;
} SegDesc;

#define segdesc_limit(desc)	(segdesc_g(desc) ? (((desc)->limitH & 0x0F << 16) | ((desc)->limitL)) << 12 : ((desc)->limitH & 0x0F << 16) | ((desc)->limitL))
#define segdesc_base(desc)	(((desc)->baseH << 24) | ((desc)->baseM << 16) | ((desc)->baseL))

#define segdesc_p(desc)		(((desc)->type >> 7) & 0x01)
#define segdesc_dpl(desc)	(((desc)->type >> 5) & 0x03)
#define segdesc_s(desc)		(((desc)->type >> 4) & 0x01)
#define segdesc_type(desc)	(((desc)->type >> 1) & 0x07)
#define segdesc_a(desc)		((desc)->type & 0x01)

#define segdesc_g(desc)		(((desc)->limitH >> 7) & 0x01)
#define segdesc_d(desc)		(((desc)->limitH >> 6) & 0x01)
#define segdesc_avl(desc)	(((desc)->limitH >> 4) & 0x01)


// uintx

typedef struct {
	union {
		uint8 uint8;
		uint16 uint16;
		uint32 uint32;
	} val;
	unsigned char type;
} uintx;

typedef struct {
	union {
		void *voidp;
		uint8 *uint8p;
		uint16 *uint16p;
		uint32 *uint32p;
	} ptr;
	unsigned char type;
} uintp;

#define UINTX_INT8		1
#define UINTX_UINT8		1
#define UINTX_INT16		2
#define UINTX_UINT16	2
#define UINTX_INT32		4
#define UINTX_UINT32	4

#define uintx_val(u)	( (u)->type==1 ? (u)->val.uint8 : \
						(u)->type==2 ? (u)->val.uint16 : \
						(u)->val.uint32 )

#define set_uintx_val(u, _val)	( (u)->type==1 ? ((u)->val.uint8=(_val)&0xFF) : \
								(u)->type==2 ? ((u)->val.uint16=(_val)&0xFFFF) : \
								((u)->val.uint32=(_val)&0xFFFFFFFF) )

#define uintp_ptr(u)			( (u)->type==1 ? (u)->ptr.uint8p : \
								(u)->type==2 ? (u)->ptr.uint16p : \
								(u)->ptr.uint32p )

#define uintp_val(u)			( (u)->type==1 ? *((u)->ptr.uint8p) : \
								(u)->type==2 ? *((u)->ptr.uint16p) : \
								*((u)->ptr.uint32p) )

#define set_uintp_val(u, val)	( (u)->type==1 ? (*((u)->ptr.uint8p)=val&0xFF) : \
								(u)->type==2 ? (*((u)->ptr.uint16p)=val&0xFFFF) : \
								(*((u)->ptr.uint32p)=val&0xFFFFFFFF) )

void uintx_read(uintx *x, void *target, int len)
{
	uintp targetp;
	targetp.ptr.voidp = target;

	if (len==1) {
		x->val.uint8 = *(targetp.ptr.uint8p);
		x->type = 1;
	} else if (len==2) {
		x->val.uint16 = *(targetp.ptr.uint16p);
		x->type = 2;
	} else {
		x->val.uint32 = *(targetp.ptr.uint32p);
		x->type = 4;
	}
}

void uintp_val_copy(uintp *dstp, uintp *srcp)
{
	if (dstp->type==1) {
		if (srcp->type==1) {
			*(dstp->ptr.uint8p) = *(srcp->ptr.uint8p);
		} else if (srcp->type==2) {
			*(dstp->ptr.uint8p) = *(srcp->ptr.uint16p);
		} else {
			*(dstp->ptr.uint8p) = *(srcp->ptr.uint32p);
		}
	} else if (dstp->type==2) {
		if (srcp->type==1) {
			*(dstp->ptr.uint16p) = *(srcp->ptr.uint8p);
		} else if (srcp->type==2) {
			*(dstp->ptr.uint16p) = *(srcp->ptr.uint16p);
		} else {
			*(dstp->ptr.uint16p) = *(srcp->ptr.uint32p);
		}
	} else {
		if (srcp->type==1) {
			*(dstp->ptr.uint32p) = *(srcp->ptr.uint8p);
		} else if (srcp->type==2) {
			*(dstp->ptr.uint32p) = *(srcp->ptr.uint16p);
		} else {
			*(dstp->ptr.uint32p) = *(srcp->ptr.uint32p);
		}
	}
}

void voidp_val_copy(void *dst, int dstlen, void *src, int srclen)
{
	uintp dstp;
	uintp srcp;

	dstp.ptr.voidp = dst;
	dstp.type = dstlen;

	srcp.ptr.voidp = src;
	srcp.type = srclen;

	uintp_val_copy(&dstp, &srcp);
}


// memory

#define mem_store8(cpu, idx, value)		((uint8*)cpu->mem)[(idx)] = (value)&0xFF;
#define mem_store_file(cpu, idx, fname)	{FILE *fp = fopen(fname, "rb"); mem_store_fp(cpu, idx, fp); fclose(fp);}
#define mem_store_fp(cpu, idx, fp)		{uint32 tidx=idx; uint32 tmp; while((tmp=fgetc(fp))!=EOF) {mem_store8(cpu, tidx++, tmp);}}

uint8 mem_eip_load8(CPUx86 *cpu)
{
	uint8 val = cpu->mem[cpu->eip];
	cpu->eip += 1;
	return val;
}

uint16 mem_eip_load16(CPUx86 *cpu)
{
	uint16 val = cpu->mem[cpu->eip] + (cpu->mem[cpu->eip+1]<<8);
	cpu->eip += 2;
	return val;
}

uint32 mem_eip_load24(CPUx86 *cpu)
{
	uint32 val = cpu->mem[cpu->eip] + (cpu->mem[cpu->eip+1]<<8) + (cpu->mem[cpu->eip+2]<<16);
	cpu->eip += 3;
	return val;
}

uint32 mem_eip_load32(CPUx86 *cpu)
{
	uint32 val = cpu->mem[cpu->eip] + (cpu->mem[cpu->eip+1]<<8) + (cpu->mem[cpu->eip+2]<<16) + (cpu->mem[cpu->eip+3]<<24);
	cpu->eip += 4;
	return val;
}

void* mem_eip_ptr(CPUx86 *cpu, int add)
{
	void *p = &(cpu->mem[cpu->eip]);
	cpu->eip += add;
	return p;
}


// segment

uint32 seg_ss(CPUx86 *cpu)
{
	if (cpu_cr0(cpu, CR0_PE)) {
		// プロテクトモード
		if (0<cpu->ss) {
			// セグメントレジスタ有効
			if ((cpu->ss>>4) < cpu->gdtr.limit) {
				return cpu->mem[cpu->gdtr.base + cpu->ss];
			} else {
				// index error
			}
		}
	} else {
		// リアルモード
		return cpu->ss << 4;
	}
	return 0;
}


// Mod R/M

void mem_eip_load_modrm(CPUx86 *cpu)
{
	cpu->modrm = mem_eip_load8(cpu);
	if (cpu->modrm&0xC0!=0xC0 && cpu->modrm&38==0x20) {
		cpu->sib = mem_eip_load8(cpu);
	} else {
		cpu->sib = 0;
	}
}

int modrm_mod(CPUx86 *cpu)
{
	return cpu->modrm>>6 & 0x03;
}

int modrm_reg(CPUx86 *cpu)
{
	return cpu->modrm>>3 & 0x07;
}

int modrm_rm(CPUx86 *cpu)
{
	return cpu->modrm & 0x07;
}

int is_modrm_r(CPUx86 *cpu)
{
	//return modrm->modrm>>3&0x07 && modrm->modrm&0x07;
	return modrm_reg(cpu) && modrm_rm(cpu);
}

#define cpu_operand_size(cpu)	(((cpu_cr0(cpu, CR0_PE) + ((cpu)->prefix.operand_size ? 1 : 0)) & 1) ? 2 : 4)

void* modrm_address(CPUx86 *cpu)
{
	int mod;
	int reg;
	uint32 offset;
	void *address;

	mod = modrm_mod(cpu);
	reg = modrm_reg(cpu);

	if (cpu_cr0(cpu, CR0_PE)) {
		// プロテクトモード
		switch (mod) {
		case 0x00:
			switch (reg) {
			case 0x00:	// [EAX]
			case 0x01:	// [ECX]
			case 0x02:	// [EDX]
			case 0x03:	// [EBX]
			case 0x06:	// [ESI]
			case 0x07:	// [EDI]
				offset = cpu->regs[reg & 0x07];
				break;
			case 0x04:	// [<SIB>]
				// todo
				break;
			case 0x05:	// [disp32]
				offset = mem_eip_load32(cpu);
				break;
			}
			address = &(cpu->mem[offset]);
			break;
		case 0x01:
			switch (reg) {
			case 0x00:	// [EAX + disp8]
			case 0x01:	// [ECX + disp8]
			case 0x02:	// [EDX + disp8]
			case 0x03:	// [EBX + disp8]
			case 0x05:	// [EBP + disp8]
			case 0x06:	// [ESI + disp8]
			case 0x07:	// [EDI + disp8]
				offset = cpu->regs[reg & 0x07] + mem_eip_load8(cpu);
				break;
			case 0x04:	// [<SIB> + disp8]
				// todo
				break;
			}
			address = &(cpu->mem[offset]);
			break;
		case 0x02:
			switch (reg) {
			case 0x00:	// [EAX + disp16]
			case 0x01:	// [ECX + disp16]
			case 0x02:	// [EDX + disp16]
			case 0x03:	// [EBX + disp16]
			case 0x05:	// [EBP + disp16]
			case 0x06:	// [ESI + disp16]
			case 0x07:	// [EDI + disp16]
				offset = cpu->regs[reg & 0x07] + mem_eip_load16(cpu);
				break;
			case 0x04:	// [<SIB> + disp16]
				// todo
				break;
			}
			address = &(cpu->mem[offset]);
			break;
		case 0x03:
			address = &(cpu->regs[reg]);
			break;
		}
	} else {
		// リアルモード
		switch (mod) {
		case 0x00:	// [レジスタ + レジスタ]
			switch (reg) {
			case 0x00:	// [BX + SI]
				offset = cpu_regist_ebx(cpu) + cpu_regist_esi(cpu);
				break;
			case 0x01:	// [BX + DI]
				offset = cpu_regist_ebx(cpu) + cpu_regist_edi(cpu);
				break;
			case 0x02:	// [BP + SI]
				offset = cpu_regist_ebp(cpu) + cpu_regist_esi(cpu);
				break;
			case 0x03:	// [BP + DI]
				offset = cpu_regist_ebp(cpu) + cpu_regist_edi(cpu);
				break;
			case 0x04:	// [SI]
				offset = cpu_regist_esi(cpu);
				break;
			case 0x05:	// [DI]
				offset = cpu_regist_edi(cpu);
				break;
			case 0x06:	// [disp16]
				offset = mem_eip_load16(cpu);
				break;
			case 0x07:	// [BX]
				offset = cpu_regist_ebx(cpu);
				break;
			}
			address = &(cpu->mem[offset & 0xFFFF]);
			break;
		case 0x01:	// [レジスタ + disp8]
			switch (reg) {
			case 0x00:	// [BX+SI + disp8]
				offset = cpu_regist_ebx(cpu) + cpu_regist_esi(cpu) + mem_eip_load8(cpu);
				break;
			case 0x01:	// [BX+DI + disp8]
				offset = cpu_regist_ebx(cpu) + cpu_regist_edi(cpu) + mem_eip_load8(cpu);
				break;
			case 0x02:	// [BP+SI + disp8]
				offset = cpu_regist_ebp(cpu) + cpu_regist_esi(cpu) + mem_eip_load8(cpu);
				break;
			case 0x03:	// [BP+DI + disp8]
				offset = cpu_regist_ebp(cpu) + cpu_regist_edi(cpu) + mem_eip_load8(cpu);
				break;
			case 0x04:	// [SI + disp8]
				offset = cpu_regist_esi(cpu) + mem_eip_load8(cpu);
				break;
			case 0x05:	// [DI + disp8]
				offset = cpu_regist_edi(cpu) + mem_eip_load8(cpu);
				break;
			case 0x06:	// [BP + disp8]
				offset = cpu_regist_ebp(cpu) + mem_eip_load8(cpu);
				break;
			case 0x07:	// [BX + disp8]
				offset = cpu_regist_ebx(cpu) + mem_eip_load8(cpu);
				break;
			}
			address = &(cpu->mem[offset & 0xFFFF]);
			break;
		case 0x02:	// [レジスタ + disp16]
			switch (reg) {
			case 0x00:	// [BX+SI + disp16]
				offset = cpu_regist_ebx(cpu) + cpu_regist_esi(cpu) + mem_eip_load16(cpu);
				break;
			case 0x01:	// [BX+DI + disp16]
				offset = cpu_regist_ebx(cpu) + cpu_regist_edi(cpu) + mem_eip_load16(cpu);
				break;
			case 0x02:	// [BP+SI + disp16]
				offset = cpu_regist_ebp(cpu) + cpu_regist_esi(cpu) + mem_eip_load16(cpu);
				break;
			case 0x03:	// [BP+DI + disp16]
				offset = cpu_regist_ebp(cpu) + cpu_regist_edi(cpu) + mem_eip_load16(cpu);
				break;
			case 0x04:	// [SI + disp16]
				offset = cpu_regist_esi(cpu) + mem_eip_load16(cpu);
				break;
			case 0x05:	// [DI + disp16]
				offset = cpu_regist_edi(cpu) + mem_eip_load16(cpu);
				break;
			case 0x06:	// [BP + disp16]
				offset = cpu_regist_ebp(cpu) + mem_eip_load16(cpu);
				break;
			case 0x07:	// [BX + disp16]
				offset = cpu_regist_ebx(cpu) + mem_eip_load16(cpu);
				break;
			}
			address = &(cpu->mem[offset & 0xFFFF]);
			break;
		case 0x03:	// レジスタ
			address = &(cpu->regs[reg]);
			break;
		}
	}
	return address;
}


// stack

void stack_push(CPUx86 *cpu, uint32 val)
{
	cpu_regist_esp(cpu) -= 4;
	cpu->mem[seg_ss(cpu) + cpu_regist_esp(cpu)] = val;
}

void stack_pop(CPUx86 *cpu)
{
	// todo
}


// call

void call(CPUx86 *cpu, int32 val)
{
	stack_push(cpu, cpu->eip);
	cpu->eip = cpu->eip + val;
}

void ret(CPUx86 *cpu, uint32 val)
{
	// todo
}

// opcode

void opcode_add(CPUx86 *cpu, void *dst, int dstlen, void *src, int srclen)
{
	uintp dstp;
	uintp srcp;

	dstp.ptr.voidp = dst;
	dstp.type = dstlen;

	srcp.ptr.voidp = src;
	srcp.type = srclen;

	set_uintp_val(&dstp, uintp_val(&dstp) + uintp_val(&srcp));
}

void opcode_or(CPUx86 *cpu, void *dst, int dstlen, void *src, int srclen)
{
	uintp dstp;
	uintp srcp;

	dstp.ptr.voidp = dst;
	dstp.type = dstlen;

	srcp.ptr.voidp = src;
	srcp.type = srclen;

	set_uintp_val(&dstp, uintp_val(&dstp) | uintp_val(&srcp));
}

void opcode_sub(CPUx86 *cpu, void *dst, int dstlen, void *src, int srclen)
{
	uintp dstp;
	uintp srcp;

	dstp.ptr.voidp = dst;
	dstp.type = dstlen;

	srcp.ptr.voidp = src;
	srcp.type = srclen;

	set_uintp_val(&dstp, uintp_val(&dstp) - uintp_val(&srcp));
}

// dump

void int2bin(char *dest, int val, int bitlen)
{
	int i;
	for (i=0; i<bitlen; i++) {
		dest[i] = val & 1<<(bitlen-1-i) ? '1' : '0';
	}
	if (i<sizeof(dest)) {
		dest[i] = '\0';
	}
}

void dump_cpu(CPUx86 *cpu)
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
	uint8 opcode;
	int c=0;
	int i;
	int is_prefix;
	uintp p;

	while (c++<20) {
		dump_cpu(cpu);

		cpu_current_reset(cpu);
		is_prefix = 1;

		while (is_prefix) {
			opcode = mem_eip_load8(cpu);
			printf("eip: %08X opcode: %X\n", cpu->eip-1, opcode);

			// prefix
			switch (opcode) {
			case 0x66:	// オペランドサイズプリフィックス
				cpu->prefix.operand_size = 1;
				break;
			case 0x67:	// アドレスサイズプリフィックス
				cpu->prefix.address_size = 1;
				break;
			case 0x26:	// セグメントオーバーライドプリフィックス(CS)
				cpu->prefix.segment_cs = 1;
				break;
			case 0x2E:	// セグメントオーバーライドプリフィックス(SS)
				cpu->prefix.segment_ss = 1;
				break;
			case 0x36:	// セグメントオーバーライドプリフィックス(DS)
				cpu->prefix.segment_ds = 1;
				break;
			case 0x3E:	// セグメントオーバーライドプリフィックス(ES)
				cpu->prefix.segment_es = 1;
				break;
			case 0x64:	// セグメントオーバーライドプリフィックス(FS)
				cpu->prefix.segment_fs = 1;
				break;
			case 0x65:	// セグメントオーバーライドプリフィックス(GS)
				cpu->prefix.segment_gs = 1;
				break;
			case 0xF2:	// リピートプリフィックス(REPNE/REPZE)
				cpu->prefix.repne = 1;
				break;
			case 0xF3:	// リピートプリフィックス(REP/REPE/REPZ)
				cpu->prefix.rep = 1;
				break;

			case 0x40:	// REXプリフィックス
			case 0x41:	// REXプリフィックス
			case 0x42:	// REXプリフィックス
			case 0x43:	// REXプリフィックス
			case 0x44:	// REXプリフィックス
			case 0x45:	// REXプリフィックス
			case 0x46:	// REXプリフィックス
			case 0x47:	// REXプリフィックス
			case 0x48:	// REXプリフィックス
			case 0x49:	// REXプリフィックス
			case 0x4A:	// REXプリフィックス
			case 0x4B:	// REXプリフィックス
			case 0x4C:	// REXプリフィックス
			case 0x4D:	// REXプリフィックス
			case 0x4E:	// REXプリフィックス
			case 0x4F:	// REXプリフィックス
				cpu->prefix.rex = opcode;
				break;
			case 0xC4:	// VEXプリフィックス(3byte)
				cpu->prefix.vex3 = mem_eip_load24(cpu);
				break;
			case 0xC5:	// VEXプリフィックス(2byte)
				cpu->prefix.vex2 = mem_eip_load16(cpu);
				break;
			default:
				is_prefix = 0;
			}

			// opcode
			if (!is_prefix) {
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
						mem_eip_load_modrm(cpu);
						switch (modrm_mod(cpu)) {
						case 0x00:
							printf("todo opcode 0x83 mod 0x00 reg 0x%X\n", modrm_reg(cpu));
							exit(1);
							break;
						case 0x01:
							printf("todo opcode 0x83 mod 0x01 reg 0x%X\n", modrm_reg(cpu));
							exit(1);
							break;
						case 0x03:	// レジスタ+disp16
							//printf("todo opcode 0x83 mod 0x03 reg 0x%X\n", modrm_reg(cpu));
							//exit(1);
							switch (modrm_reg(cpu)) {
							case 5:
								//printf("rm: %d\n", modrm_rm(cpu));
								//p.ptr.voidp = &(cpu->mem[cpu->regs[modrm_rm(cpu)]]);
								//p.ptr.uint16p += mem_eip_load16(cpu);
								opcode_sub(cpu, modrm_address(cpu), cpu_operand_size(cpu), mem_eip_ptr(cpu, 1), 1);
								break;
							default:
								printf("tod opcode 0x83\n");
								exit(0);
							}
							break;
						case 0x04:	// レジスタ
							switch (modrm_reg(cpu)) {
							case 0:
								//cpu->regs[modrm_rm(cpu)] += mem_eip_load8(cpu);
								opcode_add(cpu, &(cpu->regs[modrm_rm(cpu)]), cpu_operand_size(cpu), mem_eip_ptr(cpu, 1), 1);
								break;
							case 1:
								//cpu->regs[modrm_rm(cpu)] |= mem_eip_load8(cpu);
								opcode_or(cpu, &(cpu->regs[modrm_rm(cpu)]), cpu_operand_size(cpu), mem_eip_ptr(cpu, 1), 1);
								break;
							case 2:
								cpu->regs[modrm_rm(cpu)] += mem_eip_load8(cpu) + cpu_eflags(cpu, CPU_EFLAGS_CF);
								// todo set flag: OF SF ZF AF PF
								printf("todo opcode 0x83 mod 0x04 reg 0x02\n");
								exit(1);
								break;
							case 3:
								cpu->regs[modrm_rm(cpu)] -= mem_eip_load8(cpu) + cpu_eflags(cpu, CPU_EFLAGS_CF);
								// todo set flag: OF SF ZF AF PF CF
								printf("todo opcode 0x83 mod 0x04 reg 0x03\n");
								break;
							case 4:
								cpu->regs[modrm_rm(cpu)] &= mem_eip_load8(cpu);
								// todo set flag: OF CF SF ZF PF
								printf("todo opcode 0x83 mod 0x04 reg 0x04\n");
								break;
							case 5:
								cpu->regs[modrm_rm(cpu)] -= mem_eip_load8(cpu);
								// todo set flag: OF SF ZF AF PF CF
								printf("todo opcode 0x83 mod 0x04 reg 0x05\n");
								break;
							case 6:
								cpu->regs[modrm_rm(cpu)] ^= mem_eip_load8(cpu);
								// todo set flag: OF CF SF ZF PF
								printf("todo opcode 0x83 mod 0x04 reg 0x06\n");
								break;
							case 7:
								set_cpu_eflags(cpu, CPU_EFLAGS_ZF, cpu->regs[modrm_rm(cpu)]==mem_eip_load8(cpu) ? 1 : 0);
								// todo set flag: CF OF SF ZF AF PF
								printf("todo opcode 0x83 mod 0x04 reg 0x07\n");
								break;
							}
							break;
						}
						break;
					case 0x89:	// 89 /r sz : mov r/m32 r32
						mem_eip_load_modrm(cpu);
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
						cpu->regs[opcode & 0x07] = mem_eip_load32(cpu);
						break;

					// 0xE0
					case 0xE8:	// E8 cd sz : call rel32
						call(cpu, mem_eip_load32(cpu));
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
