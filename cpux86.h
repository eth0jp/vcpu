#ifndef CPU_X86_H
#define CPU_X86_H

// int

typedef char int8;
typedef short int16;
typedef int int32;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;


// Descriptor

typedef struct {
	uint32 limit;
	uint32 base;
	uint16 attribute;
} Descriptor;


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
	uint8 modrm_mod;
	uint8 modrm_reg;
	uint8 modrm_rm;
	uint8 sib_scale;
	uint8 sib_index;
	uint8 sib_base;
} CPUx86;


// register

#define cpu_regist_eax(cpu)	(cpu)->regs[0]
#define cpu_regist_ecx(cpu)	(cpu)->regs[1]
#define cpu_regist_edx(cpu)	(cpu)->regs[2]
#define cpu_regist_ebx(cpu)	(cpu)->regs[3]
#define cpu_regist_esp(cpu)	(cpu)->regs[4]
#define cpu_regist_ebp(cpu)	(cpu)->regs[5]
#define cpu_regist_esi(cpu)	(cpu)->regs[6]
#define cpu_regist_edi(cpu)	(cpu)->regs[7]

#define cpu_regist_ax(cpu)	((cpu)->regs[0] & 0xFFFF)
#define cpu_regist_cx(cpu)	((cpu)->regs[1] & 0xFFFF)
#define cpu_regist_dx(cpu)	((cpu)->regs[2] & 0xFFFF)
#define cpu_regist_bx(cpu)	((cpu)->regs[3] & 0xFFFF)
#define cpu_regist_sp(cpu)	((cpu)->regs[4] & 0xFFFF)
#define cpu_regist_bp(cpu)	((cpu)->regs[5] & 0xFFFF)
#define cpu_regist_si(cpu)	((cpu)->regs[6] & 0xFFFF)
#define cpu_regist_di(cpu)	((cpu)->regs[7] & 0xFFFF)

#define cpu_regist_al(cpu)	((cpu)->regs[0] & 0xFF)
#define cpu_regist_cl(cpu)	((cpu)->regs[1] & 0xFF)
#define cpu_regist_dl(cpu)	((cpu)->regs[2] & 0xFF)
#define cpu_regist_bl(cpu)	((cpu)->regs[3] & 0xFF)

#define cpu_regist_ah(cpu)	((cpu)->regs[0]>>8 & 0xFF)
#define cpu_regist_ch(cpu)	((cpu)->regs[1]>>8 & 0xFF)
#define cpu_regist_dh(cpu)	((cpu)->regs[2]>>8 & 0xFF)
#define cpu_regist_bh(cpu)	((cpu)->regs[3]>>8 & 0xFF)


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


// uintp

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


// uintp
extern uint32 uintp_val(uintp *p);
extern uint32 uintp_val_ze(uintp *p);
extern void uintp_val_copy(uintp *dst, uintp *src);
extern void set_uintp_val(uintp *dst, uint32 val);
extern void set_uintp_val_ze(uintp *dst, uint32 val);
extern int uintp_msb(uintp *target);
extern int uintp_lsb(uintp *target);
extern int bit_count8(uint8 val);

// memory
extern void mem_store8(CPUx86 *cpu, uint32 idx, uint8 value);
extern int mem_store_file(CPUx86 *cpu, uint32 idx, char *fname);
extern int mem_store_fp(CPUx86 *cpu, uint32 idx, FILE *fp);
extern uint8 mem_eip_load8(CPUx86 *cpu);
extern uint32 mem_eip_load8_se(CPUx86 *cpu);
extern uint16 mem_eip_load16(CPUx86 *cpu);
extern uint32 mem_eip_load24(CPUx86 *cpu);
extern uint32 mem_eip_load32(CPUx86 *cpu);
extern void* mem_eip_ptr(CPUx86 *cpu, int add);

// segment
extern uint32 seg_ss(CPUx86 *cpu);

// modrm
extern void mem_eip_load_modrm(CPUx86 *cpu);
extern uint32 cpu_sib_offset(CPUx86 *cpu);
extern uint32 cpu_modrm_offset(CPUx86 *cpu);
extern void cpu_modrm_address(CPUx86 *cpu, uintp *result);
extern void cpu_modrm_address_m16_32(CPUx86 *cpu, uintp *limit, uintp *base);

// opcode
extern void opcode_aam(CPUx86 *cpu, uintp *val);
extern void opcode_adc(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_add(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_and(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_call(CPUx86 *cpu, uintp *val);
extern void opcode_cli(CPUx86 *cpu);
extern void opcode_cmp(CPUx86 *cpu, uintp *src1, uintp *src2);
extern void opcode_dec(CPUx86 *cpu, uintp *target);
extern void opcode_in(CPUx86 *cpu, uintp *src);
extern void opcode_inc(CPUx86 *cpu, uintp *target);
extern void opcode_int(CPUx86 *cpu, uintp *val);
extern void opcode_into(CPUx86 *cpu);
extern void opcode_jmp_short(CPUx86 *cpu, uintp *rel);
extern void opcode_jz(CPUx86 *cpu, uintp *rel);
extern void opcode_jnz(CPUx86 *cpu, uintp *rel);
extern void opcode_js(CPUx86 *cpu, uintp *rel);
extern void opcode_mov(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_movsx(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_movzx(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_out(CPUx86 *cpu, uintp *port, uintp *val);
extern void opcode_or(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_pop(CPUx86 *cpu, uintp *dst);
extern void opcode_push(CPUx86 *cpu, uintp *val);
extern void opcode_sar(CPUx86 *cpu, uintp *dst, uintp *count);
extern void opcode_sal(CPUx86 *cpu, uintp *dst, uintp *count);
extern void opcode_sbb(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_shr(CPUx86 *cpu, uintp *dst, uintp *count);
extern void opcode_shl(CPUx86 *cpu, uintp *dst, uintp *count);
extern void opcode_sub(CPUx86 *cpu, uintp *dst, uintp *src);
extern void opcode_test(CPUx86 *cpu, uintp *src1, uintp *src2);
extern void opcode_xor(CPUx86 *cpu, uintp *dst, uintp *src);

// dump
extern void int2bin(char *dest, int val, int bitlen);
extern void dump_cpu(CPUx86 *cpu);

// cpu
extern CPUx86* new_cpux86(size_t mem_size);
extern void delete_cpux86(CPUx86 *cpu);
extern void cpu_current_reset(CPUx86 *cpu);
extern void set_cpu_eflags_sf_zf_pf(CPUx86 *cpu, uintp *target);
extern void exec_cpux86(CPUx86 *cpu);
extern void run_cpux86(CPUx86 *cpu);


#endif
