#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpux86.h"
#include "log.h"


// uintx

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

// msbの値を返す
int uintp_msb(uintp *target)
{
	uint32 bit;
	switch (target->type) {
	case 1:
		bit = 0x80;
		break;
	case 2:
		bit = 0x8000;
		break;
	case 4:
		bit = 0x80000000;
		break;
	default:
		bit = 0x00;
	}
	return (uintp_val(target) & bit) ? 1 : 0;
}

// lsbの値を返す
int uintp_lsb(uintp *target)
{
	return (uintp_val(target) & 0x01) ? 1 : 0;
}

// 立っているビット数を数える
int bit_count8(uint8 val)
{
	uint8 count;
	count = (val & 0x55) + ((val >> 1) & 0x55);
	count = (count & 0x33) + ((count >> 2) & 0x33);
	return (count & 0x0f) + ((count >> 4) & 0x0f);
}


// memory

void mem_store8(CPUx86 *cpu, uint32 idx, uint8 value)
{
	cpu->mem[idx] = value;
}

int mem_store_file(CPUx86 *cpu, uint32 idx, char *fname)
{
	FILE *fp;
	int bytes;
	fp = fopen(fname, "rb");
	bytes = -1;
	if (fp) {
		bytes = mem_store_fp(cpu, idx, fp);
		fclose(fp);
	}
	return bytes;
}

int mem_store_fp(CPUx86 *cpu, uint32 idx, FILE *fp)
{
	int tidx;
	int tmp;
	tidx = idx;
	while ((tmp=fgetc(fp))!=EOF) {
		mem_store8(cpu, tidx++, tmp&0xFF);
	}
	return tidx-idx;
}

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
	uint8 tmp;
	tmp = mem_eip_load8(cpu);
	cpu->modrm_mod = tmp>>6 & 0x03;
	cpu->modrm_reg = tmp>>3 & 0x07;
	cpu->modrm_rm = tmp & 0x07;
	if (cpu_cr0(cpu, CR0_PE) && cpu->modrm_mod!=3 && cpu->modrm_rm==4) {
		tmp = mem_eip_load8(cpu);
		cpu->sib_scale = tmp>>6 & 0x03;
		cpu->sib_index = tmp>>3 & 0x07;
		cpu->sib_base = tmp & 0x07;
	} else {
		cpu->sib_scale = 0;
		cpu->sib_index = 0;
		cpu->sib_base = 0;
	}
}

uint32 cpu_sib_offset(CPUx86 *cpu)
{
	uint32 offset = 0;

	// base
	if (cpu->modrm_mod==0x00 && cpu->sib_base==5) {
	} else {
		offset += cpu->regs[cpu->sib_base];
	}

	// index scale
	if (cpu->sib_index==4) {
	} else {
		offset += cpu->regs[cpu->sib_index] < cpu->sib_scale;
	}

	return offset;
}


#define cpu_operand_size(cpu)	((cpu_cr0(cpu, CR0_PE)==(cpu)->prefix.operand_size) ? 2 : 4)

uint32 cpu_modrm_offset(CPUx86 *cpu)
{
	int mod;
	int rm;
	uint32 offset;

	mod = cpu->modrm_mod;
	rm = cpu->modrm_rm;
	offset = 0xFFFFFFFF;

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
				offset = cpu_sib_offset(cpu);
				break;
			case 0x05:	// [disp32]
				offset = mem_eip_load32(cpu);
				break;
			}
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
				offset = cpu_sib_offset(cpu) + mem_eip_load8_se(cpu);
				break;
			}
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
				offset = cpu_sib_offset(cpu) + mem_eip_load16(cpu);
				break;
			}
			break;
		case 0x03:
			break;
		}
	} else {
		// リアルモード
		switch (mod) {
		case 0x00:	// [レジスタ + レジスタ]
			switch (rm) {
			case 0x04:
				rm = mem_eip_load8(cpu) & 0x07;
				offset = cpu->regs[rm];
				break;
			default:
				offset = cpu->regs[rm];
				break;
			}
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
				rm = mem_eip_load8_se(cpu) & 0x07;
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
			offset += cpu->regs[rm];
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
			offset += cpu->regs[rm];
			break;
		case 0x03:	// レジスタ
			break;
		}
	}
	return offset;
}

void cpu_modrm_address(CPUx86 *cpu, uintp *result)
{
	uint32 offset;

	if (cpu->modrm_mod==3) {
		result->ptr.voidp = &(cpu->regs[cpu->modrm_rm]);
		result->type = 4;
	} else {
		offset = cpu_modrm_offset(cpu);
		result->ptr.voidp = &(cpu->mem[offset]);
		result->type = cpu_operand_size(cpu);
	}
}

void cpu_modrm_address_m16_32(CPUx86 *cpu, uintp *limit, uintp *base)
{
	uint32 offset;

	if (cpu->modrm_mod==3) {
		log_error("cpu_modrm_address_m16_32 exception\n");
	} else {
		offset = cpu_modrm_offset(cpu);
		limit->ptr.voidp = &(cpu->mem[offset]);
		limit->type = 2;
		base->ptr.voidp = &(cpu->mem[offset+2]);
		base->type = 4;
	}
}


// opcode

void opcode_aam(CPUx86 *cpu, uintp *val)
{
	uint8 ah, al, imm8;

	imm8 = uintp_val(val) & 0xFF;
	ah = cpu_regist_al(cpu) / imm8;
	al = cpu_regist_al(cpu) % imm8;

	cpu_regist_eax(cpu) = ah<<8 | al;

	// todo set flag: SF ZF PF
}

void opcode_adc(CPUx86 *cpu, uintp *dst, uintp *src)
{
	uint32 val;
	uint32 cf;
	cf = cpu_eflags(cpu, CPU_EFLAGS_CF);
	val = uintp_val(dst) + uintp_val(src) + cf;

	// OF CF
	if (uintp_val(dst) != val-uintp_val(src)-cf) {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, 1);
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, 1);
	} else {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, 0);
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, 0);
	}

	set_uintp_val(dst, val);

	// AF
	//set_cpu_eflags(cpu, CPU_EFLAGS_AF, )

	// SF ZF PF
	set_cpu_eflags_sf_zf_pf(cpu, dst);

	// todo set flag: AF
}

void opcode_add(CPUx86 *cpu, uintp *dst, uintp *src)
{
	uint32 val;
	val = uintp_val(dst) + uintp_val(src);

	// OF CF
	if (uintp_val(dst) != val-uintp_val(src)) {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, 1);
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, 1);
	} else {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, 0);
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, 0);
	}

	set_uintp_val(dst, val);

	// AF
	//set_cpu_eflags(cpu, CPU_EFLAGS_AF, )

	// SF ZF PF
	set_cpu_eflags_sf_zf_pf(cpu, dst);

	// todo set flag: AF
}

void opcode_and(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(dst) & uintp_val(src));

	// OF
	set_cpu_eflags(cpu, CPU_EFLAGS_OF, 0);

	// CF
	set_cpu_eflags(cpu, CPU_EFLAGS_CF, 0);

	// SF ZF PF
	set_cpu_eflags_sf_zf_pf(cpu, dst);
}

void opcode_call(CPUx86 *cpu, uintp *val)
{
	uintp eip;
	eip.ptr.voidp = &(cpu->eip);
	eip.type = 4;

	opcode_push(cpu, &eip);
	cpu->eip = cpu->eip + uintp_val(val);
}

void opcode_cli(CPUx86 *cpu)
{
	if (!cpu_cr0(cpu, CR0_PE)) {
		// Reset Interrupt Flag
		set_cpu_eflags(cpu, CPU_EFLAGS_IF, 0);
	} else {
		if (!cpu_eflags(cpu, CPU_EFLAGS_VM)) {
			// todo
			// CPL???
			log_error("todo opcode_cli !vm\n");
		} else {
			if (cpu_eflags(cpu, CPU_EFLAGS_IOPL)==3) {
				// Reset Interrupt Flag
				set_cpu_eflags(cpu, CPU_EFLAGS_IF, 0);
			} else {
				//if (cpu_eflags(cpu, CPU_EFLAGS_IOPL)<3 && VME)
				// todo
				log_error("todo opcode_cli\n");
			}
		}
	}
}

void opcode_cmp(CPUx86 *cpu, uintp *src1, uintp *src2)
{
	uint32 temp_val;
	uintp temp;
	temp.ptr.voidp = &temp_val;
	temp.type = src1->type;
	uintp_val_copy(&temp, src1);

	opcode_sub(cpu, &temp, src2);
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

void opcode_int(CPUx86 *cpu, uintp *val)
{
	// todo
}

void opcode_into(CPUx86 *cpu)
{
	// todo
}

void opcode_jle_short(CPUx86 *cpu, uintp *rel)
{
	if (cpu_eflags(cpu, CPU_EFLAGS_ZF) || cpu_eflags(cpu, CPU_EFLAGS_SF)!=cpu_eflags(cpu, CPU_EFLAGS_OF)) {
		opcode_jmp_short(cpu, rel);
	}
}

void opcode_jmp_far(CPUx86 *cpu, uintp *segment, uintp *offset)
{
	uint32 tempEIP = 0;

	if (!cpu_cr0(cpu, CR0_PE) || (cpu_cr0(cpu, CR0_PE) && cpu_eflags(cpu, CPU_EFLAGS_VM))) {
		// real-address or virtual-8086 mode
		// todo
	} else if (cpu_cr0(cpu, CR0_PE) && !cpu_eflags(cpu, CPU_EFLAGS_VM)) {
		// Protected mode, not virtual-8086 mode
	}
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
			log_error("todo opcode_jz\n");
		}
	}
}

void opcode_jnz(CPUx86 *cpu, uintp *rel)
{
	if (!cpu_eflags(cpu, CPU_EFLAGS_ZF)) {
		cpu->eip += uintp_val(rel);
		if (rel->type==2) {
			cpu->eip &= 0xFFFF;
		} else if (rel->type==4) {
			// todo
			log_error("todo opcode_jnz\n");
		}
	}
}

void opcode_js(CPUx86 *cpu, uintp *rel)
{
	if (cpu_eflags(cpu, CPU_EFLAGS_SF)) {
		cpu->eip += uintp_val(rel);
		if (rel->type==2) {
			cpu->eip &= 0xFFFF;
		} else if (rel->type==4) {
			// todo
			log_error("todo opcode_js\n");
		}
	}
}

void opcode_lea(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(src));
}

void opcode_lgdt(CPUx86 *cpu, uintp *limit, uintp *base)
{
	if (cpu_operand_size(cpu)==2) {
		cpu->gdtr.limit = uintp_val(limit);
		cpu->gdtr.base = uintp_val(base) & 0x00FFFFFF;
	} else if (cpu_operand_size(cpu)==4) {
		cpu->gdtr.limit = uintp_val(limit);
		cpu->gdtr.base = uintp_val(base);
	}
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
	printf("movzx: 0x%X 0x%X\n", uintp_val(src), uintp_val_ze(src));
	set_uintp_val(dst, uintp_val_ze(src));
}

void opcode_out(CPUx86 *cpu, uintp *port, uintp *val)
{
	// todo

	printf("out: 0x%X\n", uintp_val(val));
}

void opcode_or(CPUx86 *cpu, uintp *dst, uintp *src)
{
	uint32 val;
	val = uintp_val(dst) | uintp_val(src);
	set_uintp_val(dst, val);

	// OF
	set_cpu_eflags(cpu, CPU_EFLAGS_OF, 0);

	// CF
	set_cpu_eflags(cpu, CPU_EFLAGS_CF, 0);

	// SF ZF PF
	set_cpu_eflags_sf_zf_pf(cpu, dst);
}

void opcode_pop(CPUx86 *cpu, uintp *dst)
{
	uintp src;

	// todo
	// 『IA-32 インテル ® アーキテクチャ・ソフト ウェア・デベロッパーズ・マニュアル、上巻』の第 6 章の「スタックアクセスにお けるアドレスサイズ属性」
	if (cpu_cr0(cpu, CR0_PE)) {
		// stack size is 32bit
		if (dst->type==4) {
			// operand size is 4byte
			src.ptr.voidp = &(cpu->mem[cpu_regist_esp(cpu)]);
			src.type = 4;
			uintp_val_copy(dst, &src);
			cpu_regist_esp(cpu) += 4;
		} else if (dst->type==2) {
			// operand size is 2byte
			src.ptr.voidp = &(cpu->mem[cpu_regist_esp(cpu)]);
			src.type = 2;
			uintp_val_copy(dst, &src);
			cpu_regist_esp(cpu) += 2;
		}
	} else {
		// stack size is 16bit
		if (dst->type==2) {
			// operand size is 2byte
			src.ptr.voidp = &(cpu->mem[cpu_regist_esp(cpu)]);
			src.type = 2;
			uintp_val_copy(dst, &src);
			cpu_regist_esp(cpu) += 2;
		} else if (dst->type==4) {
			// operand size is 4byte
			src.ptr.voidp = &(cpu->mem[cpu_regist_esp(cpu)]);
			src.type = 4;
			uintp_val_copy(dst, &src);
			cpu_regist_esp(cpu) += 4;
		}
	}
}

void opcode_push(CPUx86 *cpu, uintp *val)
{
	uintp dst;

	// todo
	// 『IA-32 インテル ® アーキテクチャ・ソフト ウェア・デベロッパーズ・マニュアル、上巻』の第 6 章の「スタックアクセスにお けるアドレスサイズ属性」
	if (cpu_cr0(cpu, CR0_PE)) {
		// stack size is 32bit
		if (val->type==4) {
			// operand size is 4byte
			cpu_regist_esp(cpu) -= 4;
			dst.ptr.voidp = &(cpu->mem[cpu_regist_esp(cpu)]);
			dst.type = 4;
			uintp_val_copy(&dst, val);
		} else {
			// operand size is 2byte
			dst.ptr.voidp = &(cpu->mem[cpu_regist_esp(cpu)]);
			dst.type = 2;
			uintp_val_copy(&dst, val);
		}
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

void opcode_ret_neer(CPUx86 *cpu)
{
	uintp dst;
	uint32 dst_val;
	dst.ptr.voidp = &dst_val;
	dst.type = 4;

	opcode_pop(cpu, &dst);
	cpu->eip = uintp_val_ze(&dst);
}

void opcode_sar(CPUx86 *cpu, uintp *dst, uintp *count)
{
	uint32 temp_count;
	temp_count = uintp_val(count) & 0x1F;

	while (temp_count) {
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, uintp_lsb(dst));
		set_uintp_val(dst, (int32)uintp_val(dst) / 2);
		temp_count--;
	}

	if ((uintp_val(count) & 0x1F)==1) {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, 0);
	}
}

void opcode_sal(CPUx86 *cpu, uintp *dst, uintp *count)
{
	uint32 temp_count;
	temp_count = uintp_val(count) & 0x1F;

	while (temp_count) {
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, uintp_msb(dst));
		set_uintp_val(dst, uintp_val(dst) * 2);
		temp_count--;
	}

	if ((uintp_val(count) & 0x1F)==1) {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, uintp_msb(dst) ^ cpu_eflags(cpu, CPU_EFLAGS_CF));
	}
}

void opcode_sbb(CPUx86 *cpu, uintp *dst, uintp *src)
{
	set_uintp_val(dst, uintp_val(dst) - uintp_val(src) - cpu_eflags(cpu, CPU_EFLAGS_CF));

	// todo set flag: OF SF ZF AF PF CF
}

void opcode_shr(CPUx86 *cpu, uintp *dst, uintp *count)
{
	uint32 temp_count;
	uint32 temp_dest_val;
	uintp temp_dest;

	temp_count = uintp_val(count) & 0x1F;

	temp_dest.ptr.voidp = &temp_dest_val;
	temp_dest.type = dst->type;
	uintp_val_copy(&temp_dest, dst);

	while (temp_count) {
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, uintp_lsb(dst));
		set_uintp_val(dst, uintp_val(dst) / 2);
		temp_count--;
	}

	// OF
	if ((uintp_val(count) & 0x1F)==1) {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, uintp_msb(&temp_dest));
	}

	// SF ZF PF
	if ((uintp_val(count) & 0x1F)!=0) {
		set_cpu_eflags_sf_zf_pf(cpu, dst);
	}
}

void opcode_shl(CPUx86 *cpu, uintp *dst, uintp *count)
{
	opcode_sal(cpu, dst, count);
}

void opcode_sub(CPUx86 *cpu, uintp *dst, uintp *src)
{
	uint32 val;
	val = uintp_val(dst) - uintp_val(src);

	// OF CF
	if (uintp_val(dst) != val+uintp_val(src)) {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, 1);
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, 1);
	} else {
		set_cpu_eflags(cpu, CPU_EFLAGS_OF, 0);
		set_cpu_eflags(cpu, CPU_EFLAGS_CF, 0);
	}

	set_uintp_val(dst, val);

	// SF ZF PF
	set_cpu_eflags_sf_zf_pf(cpu, dst);
}

void opcode_test(CPUx86 *cpu, uintp *src1, uintp *src2)
{
	uintp temp;
	uint32 temp_val;
	temp.ptr.voidp = &temp_val;
	temp.type = src1->type;
	set_uintp_val(&temp, uintp_val(src1) & uintp_val(src2));

	// OF CF
	set_cpu_eflags(cpu, CPU_EFLAGS_OF, 0);
	set_cpu_eflags(cpu, CPU_EFLAGS_CF, 0);

	// SF ZF PF
	set_cpu_eflags_sf_zf_pf(cpu, &temp);
}

void opcode_xor(CPUx86 *cpu, uintp *dst, uintp *src)
{
	uint32 val;
	val = uintp_val(dst) ^ uintp_val(src);
	set_uintp_val(dst, val);

	// OF
	set_cpu_eflags(cpu, CPU_EFLAGS_OF, 0);

	// CF
	set_cpu_eflags(cpu, CPU_EFLAGS_CF, 0);

	// SF ZF PF
	set_cpu_eflags_sf_zf_pf(cpu, dst);
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
	uint8 tmp;

	printf("dump_cpu:\n");
	printf("  eip: 0x%X\n", cpu->eip);

	for (i=0; i<8; i++) {
		printf("  %s: 0x%X\n", regs_arr[i], cpu->regs[i]);
	}

	printf("  eflags: 0x%X\n", cpu->eflags);

	// modrm
	tmp = cpu->modrm_mod<<6 | cpu->modrm_reg<<3 | cpu->modrm_rm;
	int2bin(b, tmp, 8);
	printf("  modrm: 0x%X mod: %c%c reg: %c%c%c rm: %c%c%c\n", tmp, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);

	// sib
	tmp = cpu->sib_scale<<6 | cpu->sib_index<<3 | cpu->sib_base;
	int2bin(b, tmp, 8);
	printf("  sib: 0x%X scale: %c%c index: %c%c%c base: %c%c%c\n", tmp, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
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

void cpu_current_reset(CPUx86 *cpu)
{
	memset(&(cpu->prefix), 0, sizeof(cpu->prefix));
	//cpu->modrm_mod = 0;
	//cpu->modrm_reg = 0;
	//cpu->modrm_rm = 0;
	//cpu->sib = 0;
}

// eflags(SF ZF PF)を設定
void set_cpu_eflags_sf_zf_pf(CPUx86 *cpu, uintp *target)
{
	uint32 val;
	val = uintp_val(target);

	// PF
	set_cpu_eflags(cpu, CPU_EFLAGS_PF, (bit_count8(val&0xFF) & 0x01) ? 0 : 1);

	// ZF
	set_cpu_eflags(cpu, CPU_EFLAGS_ZF, val==0 ? 1 : 0);

	// SF
	set_cpu_eflags(cpu, CPU_EFLAGS_SF, uintp_msb(target));
}

void exec_cpux86(CPUx86 *cpu)
{
	uint8 opcode;
	int c=0;
	int i;
	int is_prefix;
	uintp operand1;
	uintp operand2;
	uint32 offset;

	while (c++<30000) {
		log_info("[%d]\n", c);
		dump_cpu(cpu);

		cpu_current_reset(cpu);
		is_prefix = 1;

		while (is_prefix) {
			opcode = mem_eip_load8(cpu);
			log_info("eip: %08X opcode: %X\n", cpu->eip-1, opcode);
			//log_info("operand_size: %d\n", cpu_operand_size(cpu));

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
			// 0x00
			case 0x00:	// 00 /r : add r/m8 r8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);
				operand1.type = 1;

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = 1;

				// operation
				opcode_add(cpu, &operand1, &operand2);
				break;

			case 0x01:	// 01 /r sz : add r/m32 r32
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = cpu_operand_size(cpu);

				// operation
				opcode_add(cpu, &operand1, &operand2);
				break;

			case 0x02:	// 02 /r : add r8 r/m8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register
				operand1.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand1.type = 1;

				// src register/memory
				cpu_modrm_address(cpu, &operand2);
				operand2.type = 1;

				// operation
				opcode_add(cpu, &operand1, &operand2);
				break;

			case 0x03:	// 03 /r sz : add r32 r/m32
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register
				operand1.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand1.type = cpu_operand_size(cpu);

				// src register/memory
				cpu_modrm_address(cpu, &operand2);

				// operation
				opcode_add(cpu, &operand1, &operand2);
				break;

			case 0x07:	// 07 : pop es
				// dst register
				operand1.ptr.voidp = &(cpu->es);
				operand1.type = 2;

				// operation
				opcode_pop(cpu, &operand1);
				break;

			case 0x10:	// 10 /r : adc r/m8 r8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);
				operand1.type = 1;

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = 1;

				// operation
				opcode_adc(cpu, &operand1, &operand2);
				break;

			case 0x11:	// 11 /r sz : adc r/m32 r32
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = cpu_operand_size(cpu);

				// operation
				opcode_adc(cpu, &operand1, &operand2);
				break;

			case 0x18:	// 18 /r : sbb r/m8 r8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);
				operand1.type = 1;

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = 1;

				// operation
				opcode_sbb(cpu, &operand1, &operand2);
				break;

			// 0x20
			case 0x2D:	// 2D id sz : sub eax imm32
				// dst register
				operand1.ptr.voidp = &(cpu->regs[0]);
				operand1.type = cpu_operand_size(cpu);

				// src immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, cpu_operand_size(cpu));
				operand2.type = cpu_operand_size(cpu);

				// operation
				opcode_sub(cpu, &operand1, &operand2);
				break;

			case 0x29:	// 29 /r sz : sub r/m32 r32
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = cpu_operand_size(cpu);

				// operation
				opcode_sub(cpu, &operand1, &operand2);
				break;

			// 0x30
			case 0x31:	// 31 /r sz : xor r/m32 r32
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = cpu_operand_size(cpu);

				// operation
				opcode_xor(cpu, &operand1, &operand2);
				break;

			case 0x39:	// 39 /r sz : cmp r/m32 r32
				// modrm
				mem_eip_load_modrm(cpu);

				// xrc1 register/memory
				cpu_modrm_address(cpu, &operand1);

				// src2 register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = cpu_operand_size(cpu);

				// operation
				opcode_cmp(cpu, &operand1, &operand2);
				break;

			case 0x3C:	// 3C ib : cmp al imm8
				// src1 register
				operand1.ptr.voidp = &(cpu_regist_eax(cpu));
				operand1.type = 1;

				// src2 immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand2.type = 1;

				// operation
				opcode_cmp(cpu, &operand1, &operand2);
				break;

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
				operand1.type = cpu_operand_size(cpu);

				// operation
				opcode_push(cpu, &operand1);
				break;

			case 0x58:	// 58 sz : pop eax
			case 0x59:	// 59 sz : pop ecx
			case 0x5A:	// 5A sz : pop edx
			case 0x5B:	// 5B sz : pop ebx
			case 0x5C:	// 5C sz : pop esp
			case 0x5D:	// 5D sz : pop ebp
			case 0x5E:	// 5E sz : pop esi
			case 0x5F:	// 5F sz : pop edi
				// dst register
				operand1.ptr.voidp = &(cpu->regs[opcode & 0x07]);
				operand1.type = cpu_operand_size(cpu);

				// operation
				opcode_pop(cpu, &operand1);
				break;

			// 0x70
			case 0x74:	// 74 cb : jz rel8
				// relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_jz(cpu, &operand1);
				break;

			case 0x75:	// 75 cb : jnz rel8
				// relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_jnz(cpu, &operand1);
				break;

			case 0x78:	// 78 cb : js rel8
				// relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_js(cpu, &operand1);
				break;

			case 0x7E:	// 7E cb : jle rel8
				// relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_jle_short(cpu, &operand1);
				break;

			// 0x80
			case 0x80:
			case 0x82:
				// 80 /0 ib : add r/m8 imm8
				// 80 /1 ib : or r/m8 imm8
				// 80 /2 ib : adc r/m8 imm8
				// 80 /3 ib : sbb r/m8 imm8
				// 80 /4 ib : and r/m8 imm8
				// 80 /5 ib : sub r/m8 imm8
				// 80 /6 ib : xor r/m8 imm8
				// 80 /7 ib : cmp r/m8 imm8

				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);
				operand1.type = 1;

				// src immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand2.type = 1;

				// operation
				switch (cpu->modrm_reg) {
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
				cpu_modrm_address(cpu, &operand1);

				// src immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand2.type = 1;

				// operation
				switch (cpu->modrm_reg) {
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
				cpu_modrm_address(cpu, &operand1);
				operand1.type = 1;

				// src2 regisetr
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_rm]);
				operand2.type = 1;

				// operation
				opcode_test(cpu, &operand1, &operand2);
				break;

			case 0x85:	// 85 /r sz : test r/m32 r32
				// modrm
				mem_eip_load_modrm(cpu);

				// src1 register/memory
				cpu_modrm_address(cpu, &operand1);

				// src2 register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_rm]);
				operand2.type = 4;

				// operation
				opcode_test(cpu, &operand1, &operand2);
				break;

			case 0x88:	// 88 /r : mov r/m8 r8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);
				operand1.type = 1;

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_rm]);
				operand2.type = 1;

				// operation
				opcode_mov(cpu, &operand1, &operand2);
				break;

			case 0x89:	// 89 /r sz : mov r/m32 r32
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);

				// src register
				operand2.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand2.type = cpu_operand_size(cpu);

				// operation
				opcode_mov(cpu, &operand1, &operand2);
				break;

			case 0x8B:	// 8B /r sz : mov r32 r/m32
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register
				operand1.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand1.type = cpu_operand_size(cpu);

				// src register/memory
				cpu_modrm_address(cpu, &operand2);

				// operation
				opcode_mov(cpu, &operand1, &operand2);
				break;

			case 0x8D:	// 8D /r sz : lea r32 m
				// modrm
				mem_eip_load_modrm(cpu);
				if (cpu->modrm_mod==0x03) {
					// exception UD
					// todo
				}

				// dst register
				operand1.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand1.type = 4;

				// src memory
				offset = cpu_modrm_offset(cpu);
				operand2.ptr.voidp = &(offset);
				operand2.type = 4;

				// operation
				opcode_lea(cpu, &operand1, &operand2);
				break;

			// 0x90
			case 0x90:	// nop
				break;

			// 0xA0
			case 0xA3:	// A3 sz : mov moffs32 eax
				// dst memory
				operand1.ptr.voidp = mem_eip_ptr(cpu, 4);	// todo アドレスサイズ
				operand1.type = 4;

				// src register
				operand2.ptr.voidp = &(cpu_regist_eax(cpu));
				operand1.type = 4;

				// operation
				opcode_mov(cpu, &operand1, &operand2);
				break;

			// 0xB0
			case 0xB0:	// B0 : mov al,imm8
			case 0xB1:	// B1 : mov cl,imm8
			case 0xB2:	// B2 : mov dl,imm8
			case 0xB3:	// B3 : mov bl,imm8
			case 0xB4:	// B4 : mov ah,imm8
			case 0xB5:	// B5 : mov ch,imm8
			case 0xB6:	// B6 : mov dh,imm8
			case 0xB7:	// B7 : mov bh,imm8
				// dst register
				operand1.ptr.voidp = &(cpu->regs[opcode & 0x03]);
				if (4<=(opcode & 0x07)) {
					operand1.ptr.uint8p++;
				}
				operand1.type = 1;

				// src immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand2.type = 1;

				// operation
				opcode_mov(cpu, &operand1, &operand2);
				break;

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
			case 0xC0:
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);
				operand1.type = 1;

				// src immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand2.type = 1;

				// operation
				switch (cpu->modrm_reg) {
				case 0:	// C0 /0 ib : rol r/m8 imm8
					//opcode_rol(cpu, &operand1, &operand2);
					log_error("todo 0xC0 /0\n");
					exit(1);
					break;
				case 1:	// C0 /1 ib : ror r/m8 imm8
					//opcode_ror(cpu, &operand1, &operand2);
					log_error("todo 0xC0 /1\n");
					exit(1);
					break;
				case 2:	// C0 /2 ib : rcl r/m8 imm8
					//opcode_rcl(cpu, &operand1, &operand2);
					log_error("todo 0xC0 /2\n");
					exit(1);
					break;
				case 3:	// C0 /3 ib : rcr r/m8 imm8
					//opcode_rcr(cpu, &operand1, &operand2);
					log_error("todo 0xC0 /3\n");
					exit(1);
					break;
				case 4:	// C0 /4 ib : sal r/m8 imm8
				case 6:	// C0 /6 ib = salとして動作する
					opcode_sal(cpu, &operand1, &operand2);
					break;
				case 5:	// C0 /5 ib : shr r/m8 imm8
					opcode_shr(cpu, &operand1, &operand2);
					break;
				case 7:	// C0 /7 ib : sar r/m8 imm8
					opcode_sar(cpu, &operand1, &operand2);
					break;
				}
				break;

			case 0xC1:
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register/memory
				cpu_modrm_address(cpu, &operand1);
				operand1.type = cpu_operand_size(cpu);

				// src immediate
				operand2.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand2.type = 1;

				// operation
				switch (cpu->modrm_reg) {
				case 0:	// C1 /0 ib sz : rol r/m32 imm8
					//opcode_rol(cpu, &operand1, &operand2);
					log_error("todo 0xC1 /0\n");
					exit(1);
					break;
				case 1:	// C1 /1 ib sz : ror r/m32 imm8
					//opcode_ror(cpu, &operand1, &operand2);
					log_error("todo 0xC1 /1\n");
					exit(1);
					break;
				case 2:	// C1 /2 ib sz : rcl r/m32 imm8
					//opcode_rcl(cpu, &operand1, &operand2);
					log_error("todo 0xC1 /2\n");
					exit(1);
					break;
				case 3:	// C1 /3 ib sz : rcr r/m32 imm8
					//opcode_rcr(cpu, &operand1, &operand2);
					log_error("todo 0xC1 /3\n");
					exit(1);
					break;
				case 4:	// C1 /4 ib sz : sal r/m32 imm8
				case 6:	// C1 /6 ib sz = salとして動作する
					opcode_sal(cpu, &operand1, &operand2);
					break;
				case 5:	// C1 /5 ib sz : shr r/m32 imm8
					opcode_shr(cpu, &operand1, &operand2);
					break;
				case 7:	// C1 /7 ib sz : sar r/m32 imm8
					opcode_sar(cpu, &operand1, &operand2);
					break;
				}
				break;

			case 0xC3:	// C3 : ret
				opcode_ret_neer(cpu);
				break;

			case 0xC6:	// C6 /0 ib : mov r/m8 imm8
				// modrm
				mem_eip_load_modrm(cpu);

				// reg
				switch (cpu->modrm_reg) {
				case 0:
					// dst register/memory
					cpu_modrm_address(cpu, &operand1);
					operand1.type = 1;

					// src immediate
					operand2.ptr.voidp = mem_eip_ptr(cpu, 1);
					operand2.type = 1;

					// operation
					opcode_mov(cpu, &operand1, &operand2);
					break;
				default:
					log_error("not mapped opcode: 0xC6 reg %d\n", cpu->modrm_reg);
					break;
				}
				break;

			case 0xC7:	// C7 /0 id sz : mov r/m32 imm32
				// modrm
				mem_eip_load_modrm(cpu);

				// reg
				switch (cpu->modrm_reg) {
				case 0:
					// dst register/memory
					cpu_modrm_address(cpu, &operand1);

					// src immediate
					operand2.ptr.voidp = mem_eip_ptr(cpu, cpu_operand_size(cpu));
					operand2.type = cpu_operand_size(cpu);

					// operation
					opcode_mov(cpu, &operand1, &operand2);
					break;
				default:
					log_error("not mapped opcode: 0xC7 reg %d\n", cpu->modrm_reg);
					break;
				}
				break;

			case 0xCD:	// CD ib : int imm8
				// src immediate
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_int(cpu, &operand1);
				break;

			case 0xCE:	// CE : into
				opcode_into(cpu);
				break;

			// 0xD0
			case 0xD4:	// D4 ib : aam imm8
				// src immediate
				operand1.ptr.voidp = mem_eip_ptr(cpu, 1);
				operand1.type = 1;

				// operation
				opcode_aam(cpu, &operand1);
				break;

			// 0xE0
			case 0xE8:	// E8 cd sz : call rel32
				// src relative address
				operand1.ptr.voidp = mem_eip_ptr(cpu, 4);
				operand1.type = 4;

				// operation
				opcode_call(cpu, &operand1);
				break;

			case 0xEA:	// EA cp sz : jmp ptr16:32
				// src offset
				operand1.ptr.voidp = mem_eip_ptr(cpu, cpu_operand_size(cpu));
				operand1.type = cpu_operand_size(cpu);

				// src segment
				operand2.ptr.voidp = mem_eip_ptr(cpu, 2);
				operand2.type = 2;

				// operation
				opcode_jmp_far(cpu, &operand2, &operand1);
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
				// modrm
				mem_eip_load_modrm(cpu);

				switch (cpu->modrm_reg) {
				case 0:	// FE /0 : inc r/m8
					// target
					cpu_modrm_address(cpu, &operand1);
					operand1.type = 1;

					// operation
					opcode_inc(cpu, &operand1);
					break;
				case 1:	// FE /1 : dec r/m8
					// target
					cpu_modrm_address(cpu, &operand1);
					operand1.type = 1;

					// operation
					opcode_dec(cpu, &operand1);
					break;
				default:
					log_error("not mapped opcode: 0xFE reg %d\n", cpu->modrm_reg);
					break;
				}
				break;

			// not implemented opcode
			default:
				log_error("not implemented opcode: 0x%02X\n", opcode);
				exit(1);
			}
		} else {
			opcode = mem_eip_load8(cpu);
			log_info("opcode: %X\n", opcode);

			// 2byte opcode
			switch (opcode) {
			case 0x01:
				// 0F 01 /0 : sgdt m
				// 0F 01 /1 : sidt m
				// 0F 01 /2 : lgdt m16&32
				// 0F 01 /3 : lidt m16&32
				// 0F 01 /4 sz : smsw r32/m16
				// 0F 01 /6 : lmsw r/m16
				// 0F 01 /7 : invlpg m

				// modrm
				mem_eip_load_modrm(cpu);

				switch (cpu->modrm_reg) {
				case 0:
					log_error("not implemented opcode: 0x0F01 /%d\n", cpu->modrm_reg);
					break;
				case 1:
					log_error("not implemented opcode: 0x0F01 /%d\n", cpu->modrm_reg);
					break;
				case 2:
					// src m16&32
					cpu_modrm_address_m16_32(cpu, &operand1, &operand2);

					// operation
					opcode_lgdt(cpu, &operand1, &operand2);
					break;
				case 3:
					log_error("not implemented opcode: 0x0F01 /%d\n", cpu->modrm_reg);
					break;
				case 4:
					log_error("not implemented opcode: 0x0F01 /%d\n", cpu->modrm_reg);
					break;
				case 6:
					log_error("not implemented opcode: 0x0F01 /%d\n", cpu->modrm_reg);
					break;
				case 7:
					log_error("not implemented opcode: 0x0F01 /%d\n", cpu->modrm_reg);
					break;
				}
				break;

			case 0xB6:	// 0F B6 /r sz : movzx r32 r/m8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register
				operand1.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand1.type = cpu_operand_size(cpu);

				// src register/memory
				cpu_modrm_address(cpu, &operand2);
				operand2.type = 1;

				// operation
				opcode_movzx(cpu, &operand1, &operand2);
				break;

			case 0xBE:	// 0F BE /r sz : movsx r32 r/m8
				// modrm
				mem_eip_load_modrm(cpu);

				// dst register
				operand1.ptr.voidp = &(cpu->regs[cpu->modrm_reg]);
				operand1.type = cpu_operand_size(cpu);

				// src register/memory
				cpu_modrm_address(cpu, &operand2);
				operand2.type = 1;

				// operation
				opcode_movsx(cpu, &operand1, &operand2);
				break;

			case 0xF1:	// 0F F1 /r : psllw mm mm/m64
				log_error("not implemented opcode: 0x0FF1\n");
				break;

			default:
				log_error("not implemented opcode: 0x0F%02X\n", opcode);
				exit(1);
			}
		}
	}
}

void run_cpux86(CPUx86 *cpu)
{
	exec_cpux86(cpu);
}
