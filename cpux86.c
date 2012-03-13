#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char int8;
typedef short int16;
typedef int int32;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;


// Descriptor Table Register

typedef struct {
	uint16 limit;	// Table Limit
	uint32 base;	// Linear Base Address
} DescTableReg;


// CPUx86

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
	//cpu->modrm = 0;
	//cpu->sib = 0;
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

#define cpu_regist_ax(cpu)	(cpu->regs[0] & 0xFFFF)
#define cpu_regist_cx(cpu)	(cpu->regs[1] & 0xFFFF)
#define cpu_regist_dx(cpu)	(cpu->regs[2] & 0xFFFF)
#define cpu_regist_bx(cpu)	(cpu->regs[3] & 0xFFFF)
#define cpu_regist_sp(cpu)	(cpu->regs[4] & 0xFFFF)
#define cpu_regist_bp(cpu)	(cpu->regs[5] & 0xFFFF)
#define cpu_regist_si(cpu)	(cpu->regs[6] & 0xFFFF)
#define cpu_regist_di(cpu)	(cpu->regs[7] & 0xFFFF)


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


// Segment Descriptor

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
		void *voidp;
		uint8 *uint8p;
		uint16 *uint16p;
		uint32 *uint32p;
	} ptr;
	// 1 => uint8
	// 2 => uint16
	// 4 => uint32
	char type;
} uintp;

#define UINTX_INT8		1
#define UINTX_UINT8		1
#define UINTX_INT16		2
#define UINTX_UINT16	2
#define UINTX_INT32		4
#define UINTX_UINT32	4

// 符号拡張されたuint32を返す
uint32 uintp_val(uintp *p)
{
	uint32 val;
	switch (p->type) {
	case 1:
		val = *(p->ptr.uint8p);
		if (val & 0x80) {
			val |= 0xFFFFFF00;
		}
		break;
	case 2:
		val = *(p->ptr.uint16p);
		if (val & 0x8000) {
			val |= 0xFFFF0000;
		}
		break;
	case 4:
		val = *(p->ptr.uint32p);
		break;
	}
	return val;
}

// ゼロ拡張されたuint32を返す
uint32 uintp_val_ze(uintp *p)
{
	uint32 val;
	switch (p->type) {
	case 1:
		val = *(p->ptr.uint8p) & 0xFF;
		break;
	case 2:
		val = *(p->ptr.uint16p) & 0xFFFF;
		break;
	case 4:
		val = *(p->ptr.uint32p) & 0xFFFFFFFF;
		break;
	}
	return val;
}

// uintpの値を比較する
int uintp_cmp(uintp *p1, uintp *p2)
{
/*
	if ((0<p1->type && 0<p2->type) || p1->type==p2->type) {
		return uintp_val(p1) - uintp_val(p2);
	}
	if (p1->type==-1) {
		if (p2->type==2) {
			return (uintp_val(p1) & 0xFFFF) - uintp_val(p2);
		} else {
			return uintp_val(p1) - uintp_val(p2);
		}
	} else {
		if (p1->type==2) {
			return uintp_val(p1) - (uintp_val(p2) & 0xFFFF);
		} else {
			return uintp_val(p1) - uintp_val(p2);
		}
	}
*/
	return (int)uintp_val(p1) - (int)uintp_val(p2);
}

// srcの値をdstにコピーする
void uintp_val_copy(uintp *dst, uintp *src)
{
	if (dst->type==1) {
		*(dst->ptr.uint8p) = uintp_val(src);
	} else if (dst->type==2) {
		*(dst->ptr.uint16p) = uintp_val(src);
	} else {
		*(dst->ptr.uint32p) = uintp_val(src);
	}
}

// valを符号拡張してdstに代入
void set_uintp_val(uintp *dst, uint32 val)
{
	uintp src;
	src.ptr.voidp = &val;
	src.type = dst->type;
	uintp_val_copy(dst, &src);
}

// valをゼロ拡張してdstに代入
void set_uintp_val_ze(uintp *dst, uint32 val)
{
	uintp src;
	src.ptr.voidp = &val;
	src.type = 4;
	uintp_val_copy(dst, &src);
}


// memory

#define mem_store8(cpu, idx, value)		(cpu)->mem[(idx)] = ((value)&0xFF)
#define mem_store_file(cpu, idx, fname)	{FILE *fp = fopen(fname, "rb"); mem_store_fp((cpu), (idx), fp); fclose(fp);}
#define mem_store_fp(cpu, idx, fp)		{uint32 tidx=(idx); int tmp; while((tmp=fgetc(fp))!=EOF) {mem_store8(cpu, tidx++, tmp);}}

uint8 mem_eip_load8(CPUx86 *cpu)
{
	uint8 val = cpu->mem[cpu->eip];
	cpu->eip += 1;
	return val;
}

uint32 mem_eip_load8_se(CPUx86 *cpu)
{
	uint8 val8 = mem_eip_load8(cpu);
	uint32 val32;
	if (val8 & 0x80) {
		//val32 = val8 | 0xFFFFFF00;
		val32 = val8 | 0x0000FF00;
	} else {
		val32 = val8 & 0x000000FF;
	}
	return val32;
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
	if (cpu_cr0(cpu, CR0_PE) && cpu_modrm_mod(cpu)!=3 && cpu_modrm_rm(cpu)==5) {
	//if (cpu_modrm_mod(cpu)!=3 && cpu_modrm_rm(cpu)==5) {
		printf("load sib\n");
		cpu->sib = mem_eip_load8(cpu);
	} else {
		cpu->sib = 0;
	}
}

int cpu_modrm_mod(CPUx86 *cpu)
{
	return cpu->modrm>>6 & 0x03;
}

int cpu_modrm_reg(CPUx86 *cpu)
{
	return cpu->modrm>>3 & 0x07;
}

int cpu_modrm_rm(CPUx86 *cpu)
{
	return cpu->modrm & 0x07;
}

int is_cpu_modrm_r(CPUx86 *cpu)
{
	//return modrm->modrm>>3&0x07 && modrm->modrm&0x07;
	return cpu_modrm_reg(cpu) && cpu_modrm_rm(cpu);
}

int cpu_sib_scale(CPUx86 *cpu)
{
	return cpu->sib>>6 & 0x03;
}

int cpu_sib_index(CPUx86 *cpu)
{
	return cpu->sib>>3 & 0x03;
}

int cpu_sib_base(CPUx86 *cpu)
{
	return cpu->sib & 0x03;
}

//#define cpu_operand_size(cpu)	(((cpu_cr0(cpu, CR0_PE) + ((cpu)->prefix.operand_size ? 1 : 0)) & 1) ? 2 : 4)
#define cpu_operand_size(cpu)	((cpu_cr0(cpu, CR0_PE)==(cpu)->prefix.operand_size) ? 2 : 4)

void cpu_modrm_address(CPUx86 *cpu, uintp *result, int use_reg)
{
	int mod;
	int rm;
	uint32 offset;

	mod = cpu_modrm_mod(cpu);
	rm = use_reg ? cpu_modrm_reg(cpu) : cpu_modrm_rm(cpu);

	if (cpu_cr0(cpu, CR0_PE)) {
		// プロテクトモード
		switch (mod) {
		case 0x00:
			switch (rm) {
			case 0x00:	// [EAX]
			case 0x01:	// [ECX]
			case 0x02:	// [EDX]
			case 0x03:	// [EBX]
			case 0x06:	// [ESI]
			case 0x07:	// [EDI]
				offset = cpu->regs[rm];
				break;
			case 0x04:	// [<SIB>]
				// todo
				break;
			case 0x05:	// [disp32]
				offset = mem_eip_load32(cpu);
				break;
			}
			result->ptr.voidp = &(cpu->mem[offset]);
			result->type = cpu->prefix.operand_size ? 2 : 4;
			break;
		case 0x01:
			switch (rm) {
			case 0x00:	// [EAX + disp8]
			case 0x01:	// [ECX + disp8]
			case 0x02:	// [EDX + disp8]
			case 0x03:	// [EBX + disp8]
			case 0x05:	// [EBP + disp8]
			case 0x06:	// [ESI + disp8]
			case 0x07:	// [EDI + disp8]
				offset = cpu->regs[rm] + mem_eip_load8_se(cpu);
				break;
			case 0x04:	// [<SIB> + disp8]
				// todo
				break;
			}
			result->ptr.voidp = &(cpu->mem[offset]);
			result->type = cpu->prefix.operand_size ? 2 : 4;
			break;
		case 0x02:
			switch (rm) {
			case 0x00:	// [EAX + disp16]
			case 0x01:	// [ECX + disp16]
			case 0x02:	// [EDX + disp16]
			case 0x03:	// [EBX + disp16]
			case 0x05:	// [EBP + disp16]
			case 0x06:	// [ESI + disp16]
			case 0x07:	// [EDI + disp16]
				offset = cpu->regs[rm] + mem_eip_load16(cpu);
				break;
			case 0x04:	// [<SIB> + disp16]
				// todo
				break;
			}
			result->ptr.voidp = &(cpu->mem[offset]);
			result->type = cpu->prefix.operand_size ? 2 : 4;
			break;
		case 0x03:
			result->ptr.voidp = &(cpu->regs[rm]);
			result->type = 4;
			break;
		}
	} else {
		// リアルモード
		switch (mod) {
		case 0x00:	// [レジスタ + レジスタ]
			switch (rm) {
			case 0x00:	// [BX + SI]
				offset = cpu_regist_bx(cpu) + cpu_regist_si(cpu);
				break;
			case 0x01:	// [BX + DI]
				offset = cpu_regist_bx(cpu) + cpu_regist_di(cpu);
				break;
			case 0x02:	// [BP + SI]
				offset = cpu_regist_bp(cpu) + cpu_regist_si(cpu);
				break;
			case 0x03:	// [BP + DI]
				offset = cpu_regist_bp(cpu) + cpu_regist_di(cpu);
				break;
			case 0x04:	// [SI]
				offset = cpu_regist_si(cpu);
				break;
			case 0x05:	// [DI]
				offset = cpu_regist_di(cpu);
				break;
			case 0x06:	// [disp16]
				offset = mem_eip_load16(cpu);
				break;
			case 0x07:	// [BX]
				offset = cpu_regist_bx(cpu);
				break;
			}
			result->ptr.voidp = &(cpu->mem[cpu->regs[rm] + offset]);
			result->type = cpu->prefix.operand_size ? 2 : 4;
			break;
		case 0x01:	// [レジスタ + disp8]
			switch (rm) {
			case 0x00:	// [BX + SI + disp8]
				offset = cpu_regist_bx(cpu) + cpu_regist_si(cpu) + mem_eip_load8_se(cpu);
				break;
			case 0x01:	// [BX + DI + disp8]
				offset = cpu_regist_bx(cpu) + cpu_regist_di(cpu) + mem_eip_load8_se(cpu);
				break;
			case 0x02:	// [BP + SI + disp8]
				offset = cpu_regist_bp(cpu) + cpu_regist_si(cpu) + mem_eip_load8_se(cpu);
				break;
			case 0x03:	// [BP + DI + disp8]
				offset = cpu_regist_bp(cpu) + cpu_regist_di(cpu) + mem_eip_load8_se(cpu);
				break;
			case 0x04:	// [SI + disp8]
				offset = cpu_regist_si(cpu) + mem_eip_load8_se(cpu);
				break;
			case 0x05:	// [DI + disp8]
				offset = cpu_regist_di(cpu) + mem_eip_load8_se(cpu);
				break;
			case 0x06:	// [BP + disp8]
				offset = cpu_regist_bp(cpu) + mem_eip_load8_se(cpu);
				break;
			case 0x07:	// [BX + disp8]
				offset = cpu_regist_bx(cpu) + mem_eip_load8_se(cpu);
				break;
			}
			result->ptr.voidp = &(cpu->mem[cpu->regs[rm] + offset]);
			result->type = cpu->prefix.operand_size ? 2 : 4;
			break;
		case 0x02:	// [レジスタ + disp16]
			switch (rm) {
			case 0x00:	// [BX + SI + disp16]
				offset = cpu_regist_bx(cpu) + cpu_regist_si(cpu) + mem_eip_load16(cpu);
				break;
			case 0x01:	// [BX + DI + disp16]
				offset = cpu_regist_bx(cpu) + cpu_regist_di(cpu) + mem_eip_load16(cpu);
				break;
			case 0x02:	// [BP + SI + disp16]
				offset = cpu_regist_bp(cpu) + cpu_regist_si(cpu) + mem_eip_load16(cpu);
				break;
			case 0x03:	// [BP + DI + disp16]
				offset = cpu_regist_bp(cpu) + cpu_regist_di(cpu) + mem_eip_load16(cpu);
				break;
			case 0x04:	// [SI + disp16]
				offset = cpu_regist_si(cpu) + mem_eip_load16(cpu);
				break;
			case 0x05:	// [DI + disp16]
				offset = cpu_regist_di(cpu) + mem_eip_load16(cpu);
				break;
			case 0x06:	// [BP + disp16]
				offset = cpu_regist_bp(cpu) + mem_eip_load16(cpu);
				break;
			case 0x07:	// [BX + disp16]
				offset = cpu_regist_bx(cpu) + mem_eip_load16(cpu);
				break;
			}
			result->ptr.voidp = &(cpu->mem[cpu->regs[rm] + offset]);
			result->type = cpu->prefix.operand_size ? 2 : 4;
			break;
		case 0x03:	// レジスタ
			result->ptr.voidp = &(cpu->regs[rm]);
			result->type = 4;
			break;
		}
	}
}


// stack

void stack_push(CPUx86 *cpu, uintp *val)
{
	uintp dst;

	if (cpu_cr0(cpu, CR0_PE)) {
		// stack size is 32bit
		// todo
	} else {
		// stack size is 16bit
		if (val->type==2) {
			// operand size is 2byte
			cpu_regist_esp(cpu) -= 2;
			//dst.ptr.voidp = &(cpu->mem[seg_ss(cpu) + cpu_regist_esp(cpu)]);
			dst.ptr.voidp = &(cpu->mem[cpu_regist_esp(cpu)]);
			dst.type = 2;
			uintp_val_copy(&dst, val);
		} else if (val->type==4) {
			// operand size is 4byte
			cpu_regist_esp(cpu) -= 4;
			//dst.ptr.voidp = &(cpu->mem[seg_ss(cpu) + cpu_regist_esp(cpu)]);
			dst.ptr.voidp = &(cpu->mem[cpu_regist_esp(cpu)]);
			dst.type = 4;
			uintp_val_copy(&dst, val);
		}
	}
}

void stack_pop(CPUx86 *cpu)
{
	// todo
}


// opcode

void opcode_adc(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(dst) + uintp_val(src) + cpu_eflags(cpu, CPU_EFLAGS_CF));

	// todo set flag: OF SF ZF AF PF
}

void opcode_add(CPUx86 *cpu, uintp *dst, uintp *src)
{
	uint32 val;

	val = uintp_val(dst) + uintp_val(src);
	set_uintp_val(dst, val);

	// todo set flag: OF SF ZF AF CF PF
}

void opcode_and(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(dst) & uintp_val(src));

	// todo set flag: OF CF SF ZF PF
}

void opcode_call(CPUx86 *cpu, uintp *val)
{
	uintp eip;
	eip.ptr.voidp = &(cpu->eip);
	eip.type = 4;

	stack_push(cpu, &eip);
	cpu->eip = cpu->eip + uintp_val(val);
}

void opcode_cli(CPUx86 *cpu)
{
	if (cpu_cr0(cpu, CR0_PE)) {
		// Reset Interrupt Flag
		set_cpu_eflags(cpu, CPU_EFLAGS_IF, 0);
	} else {
		if (!cpu_cr0(cpu, CPU_EFLAGS_VM)) {
			// todo
			// CPL???
		} else {
			if (cpu_cr0(cpu, CPU_EFLAGS_IOPL)==3) {
				// Reset Interrupt Flag
				set_cpu_eflags(cpu, CPU_EFLAGS_IF, 0);
			} else {
				//if (cpu_eflags(cpu, CPU_EFLAGS_IOPL)<3 && VME)
				// todo
			}
		}
	}
}

void opcode_cmp(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_cpu_eflags(cpu, CPU_EFLAGS_ZF, uintp_cmp(dst, src)==0 ? 1 : 0);

	// todo set flag: CF OF SF ZF AF PF
}

void opcode_dec(CPUx86 *cpu, uintp *target)
{
	set_uintp_val(target, uintp_val(target) - 1);
}

void opcode_in(CPUx86 *cpu, uintp *src)
{
	// todo
	//cpu_regist_eax(cpu) = 
}

void opcode_inc(CPUx86 *cpu, uintp *target)
{
	set_uintp_val(target, uintp_val(target) + 1);
}

void opcode_into(CPUx86 *cpu)
{
	// todo
}

void opcode_jmp_short(CPUx86 *cpu, uintp *rel)
{
	cpu->eip += (int)uintp_val(rel);
}

void opcode_jz(CPUx86 *cpu, uintp *rel)
{
	if (cpu_eflags(cpu, CPU_EFLAGS_ZF)) {
		cpu->eip += uintp_val(rel);
		if (rel->type==2) {
			cpu->eip &= 0xFFFF;
		} else if (rel->type==4) {
			// todo
		}
	}
}

void opcode_jnz(CPUx86 *cpu, uintp *rel)
{
	if (cpu_eflags(cpu, CPU_EFLAGS_ZF)) {
		cpu->eip += uintp_val(rel);
		if (rel->type==2) {
			cpu->eip &= 0xFFFF;
		} else if (rel->type==4) {
			// todo
		}
	}
}

void opcode_out(CPUx86 *cpu, uintp *port, uintp *val)
{
	// todo

	printf("out: %x\n", uintp_val(val));
}

void opcode_or(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(dst) | uintp_val(src));

	// todo set flag: OF CF SF ZF PF
}

void opcode_mov(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(src));
}

void opcode_movsx(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(src));
}

void opcode_movzx(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val_ze(src));
}

void opcode_sbb(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(dst) - uintp_val(src) - cpu_eflags(cpu, CPU_EFLAGS_CF));

	// todo set flag: OF SF ZF AF PF CF
}

void opcode_sub(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(dst) - uintp_val(src));

	// todo set flag: OF SF ZF AF PF CF
}

void opcode_test(CPUx86 *cpu, uintp *src1, uintp *src2)
{
	// todo
}

void opcode_xor(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(dst) ^ uintp_val(src));

	// todo set flag: OF CF SF ZF PF
}


// dump

void int2bin(char *dest, int val, int bitlen)
{
	int i;
	for (i=0; i<bitlen; i++) {
		dest[i] = val & 1<<(bitlen-1-i) ? '1' : '0';
	}
	if ((unsigned int)i<sizeof(dest)) {
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
	uintp operand1;
	uintp operand2;

	while (c++<40) {
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
		}

		// opcode
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
				// src register
				operand1.ptr.voidp = &(cpu->regs[opcode & 0x07]);
				operand1.type = 4;

				// operation
				stack_push(cpu, &operand1);
				break;

			// 70
			case 0x74:	// 74 cb : jz rel8
				// relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_jz(cpu, &operand1);
				break;

			case 0x75:	// 75 cb : jnz rel8
				//relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_jnz(cpu, &operand1);

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

				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1, 0);

				// src immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, 1);
				//operand2.type = -1;
				operand2.type = 1;

				// operation
				switch (cpu_modrm_reg(cpu)) {
				case 0:
					opcode_add(cpu, &operand1, &operand2);
					break;
				case 1:
					opcode_or(cpu, &operand1, &operand2);
					break;
				case 2:
					opcode_adc(cpu, &operand1, &operand2);
					break;
				case 3:
					opcode_sbb(cpu, &operand1, &operand2);
					break;
				case 4:
					opcode_and(cpu, &operand1, &operand2);
					break;
				case 5:
					opcode_sub(cpu, &operand1, &operand2);
					break;
				case 6:
					opcode_xor(cpu, &operand1, &operand2);
					break;
				case 7:
					opcode_cmp(cpu, &operand1, &operand2);
					break;
				}
				break;
			case 0x84:	// 84 /r : test r/m8 r8
				// modrm
				mem_eip_load_modrm(cpu);

				// src1 register/memory
				cpu_modrm_address(cpu, &operand1, 0);

				// src2 regisetr
				operand2.ptr.voidp = &(cpu->regs[cpu_modrm_rm(cpu)]);
				operand2.type = 1;

				// operation
				opcode_test(cpu, &operand1, &operand2);
				break;
			case 0x89:	// 89 /r sz : mov r/m32 r32
				mem_eip_load_modrm(cpu);
				if (is_cpu_modrm_r(cpu)) {
					// dst register/memory
					cpu_modrm_address(cpu, &operand1, 0);

					// src register
					operand2.ptr.voidp = &(cpu->regs[cpu_modrm_reg(cpu)]);
					operand2.type = 4;

					// operation
					opcode_mov(cpu, &operand1, &operand2);
				}
				break;

			case 0x8B:	// 8B /r sz : mov r32 r/m32
				mem_eip_load_modrm(cpu);
				if (is_cpu_modrm_r(cpu)) {
					// src register
					operand1.ptr.voidp = &(cpu->regs[cpu_modrm_reg(cpu)]);
					operand1.type = 4;

					// dst register/memory
					cpu_modrm_address(cpu, &operand2, 0);

					// operation
					opcode_mov(cpu, &operand1, &operand2);
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
				// dst register
				operand1.ptr.voidp = &(cpu->regs[opcode & 0x07]);
				operand1.type = 4;

				// src immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, 4);
				operand2.type = 4;

				// operation
				opcode_mov(cpu, &operand1, &operand2);
				break;

			// 0xC0
			case 0xC7:	// C7 /0 id sz : mov r/m32 imm32
				mem_eip_load_modrm(cpu);
				switch (cpu_modrm_reg(cpu)) {
				case 0:
					// ???
					mem_eip_load8(cpu);	// todo

					// dst register/memory
					cpu_modrm_address(cpu, &operand1, 0);

					// src immediate
					operand2.ptr.voidp = mem_eip_ptr(cpu, 4);
					operand2.type = 4;

					// operation
					opcode_mov(cpu, &operand1, &operand2);
					break;
				default:
					printf("not mapped opcode: 0xC0 reg %d\n", cpu_modrm_reg(cpu));
					break;
				}
				break;

			case 0xCE:	// CE : into
				opcode_into(cpu);
				break;

			// 0xE0
			case 0xE8:	// E8 cd sz : call rel32
				// src relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 4);
				operand1.type = 4;

				// operation
				opcode_call(cpu, &operand1);
				break;

			case 0xEB:	// EB cb : jmp rel8
				// src relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_jmp_short(cpu, &operand1);
				break;

			case 0xED:	// ED sz : in eax dx
				// src
				operand1.ptr.voidp = &(cpu_regist_edx(cpu));
				operand1.type = cpu_operand_size(cpu);

				// operation
				opcode_in(cpu, &operand1);
				break;

			case 0xEE:	// EE : out dx al
				// output port
				operand1.ptr.voidp = &(cpu_regist_edx(cpu));
				operand1.type = 2;

				// output data
				operand2.ptr.voidp = &(cpu_regist_eax(cpu));
				operand2.type = 1;

				// operation
				opcode_out(cpu, &operand1, &operand2);
				break;

			case 0xFA:	// FA : cli
				opcode_cli(cpu);
				break;

			case 0xFE:
				mem_eip_load_modrm(cpu);

				switch (cpu_modrm_reg(cpu)) {
				case 0:	// FE /0 : inc r/m8
					// target
					cpu_modrm_address(cpu, &operand1, 0);
					operand1.type = 1;

					// operation
					opcode_inc(cpu, &operand1);
					break;
				case 1:	// FE /1 : dec r/m8
					// target
					cpu_modrm_address(cpu, &operand1, 0);
					operand1.type = 1;

					// operation
					opcode_dec(cpu, &operand1);
					break;
				default:
					printf("not mapped opcode: 0xFE reg %d\n", cpu_modrm_reg(cpu));
					break;
				}
				break;

			// not implemented opcode
			default:
				printf("not implemented opcode: 0x%02X\n", opcode);
				exit(1);
			}
		} else {
			opcode = mem_eip_load8(cpu);
			printf("opcode: %X\n", opcode);

			// 2byte opcode
			switch (opcode) {
			case 0xB6:	// 0F B6 /r sz : movzx r32 r/m8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register
				operand1.ptr.voidp = &(cpu->regs[cpu_modrm_reg(cpu)]);
				operand1.type = 4;

				// src register/memory
				cpu_modrm_address(cpu, &operand2, 0);
				operand2.type = 1;

				// operation
				opcode_movzx(cpu, &operand1, &operand2);
				break;

			case 0xBE:	// 0F BE /r sz : movsx r32 r/m8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register
				operand1.ptr.voidp = &(cpu->regs[cpu_modrm_reg(cpu)]);
				operand1.type = 4;

				// src register/memory
				cpu_modrm_address(cpu, &operand2, 0);
				operand2.type = 1;

				// operation
				opcode_movsx(cpu, &operand1, &operand2);
				break;

			default:
				printf("not implemented opcode: 0x0F%02X\n", opcode);
				exit(1);
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

	cpu = new_cpux86(1024*1024*32);
/*
	mem_store_file(cpu, 0x00100000, "../jslinux/files/vmlinux26.bin");
	mem_store_file(cpu, 0x00400000, "../jslinux/files/root.bin");
	mem_store_file(cpu, 0x10000, "../jslinux/files/linuxstart.bin");
	cpu->eip = 0x10000;
	cpu_regist_eax(cpu) = 0x2000000;
	cpu_regist_ebx(cpu) = 0x200000;
*/
	mem_store_file(cpu, 0x00, "./asm/bootstrap.o");
    cpu->eip = 0x00;
	run_cpux86(cpu);
	delete_cpux86(cpu);

	return 0;
}
