#include "CPU.h"

//8 bit load instructions

int CPU::LD_R8_R8(byte& register_one, const byte& register_two) {
	register_one = register_two;
	
	return cycles_FOUR;
}

int CPU::LD_R8_N8(byte& register_one) {
	byte n8 = fetch_next_byte();

	register_one = n8;
	return cycles_EIGHT;
}

int CPU::LD_R8_HL(byte& register_one, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	register_one = value;
	return cycles_EIGHT;
}

int CPU::LD_HL_R8(const byte& h, const byte& l, const byte& register_value) {
	ushort hl = (ushort)(h << 8 | l);
	write_to_bus(hl, register_value);

	return cycles_EIGHT;
}

int CPU::LD_HL_N8(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = fetch_next_byte();

	write_to_bus(hl, value);
	return cycles_TWELVE;
}

int CPU::LD_A_R16(byte& a, const byte& register_high, const byte& register_low) {
	ushort address = (ushort)(register_high << 8 | register_low);
	byte value = read_from_bus(address);

	a = value;
	return cycles_EIGHT;
}

int CPU::LD_R16_A(const byte& register_high, const byte& register_low, const byte& a) {
	ushort address = (ushort)(register_high << 8 | register_low);

	write_to_bus(address, a);
	return cycles_EIGHT;
}

int CPU::LD_A_N16(byte& a) {
	byte low = fetch_next_byte();
	byte high = fetch_next_byte();

	ushort address = (ushort)(high << 8 | low);

	byte value = read_from_bus(address);
	a = value;
	return cycles_SIXTEEN;
}

int CPU::LD_N16_A(const byte& a) {
	byte low = fetch_next_byte();
	byte high = fetch_next_byte();

	ushort address = (ushort)(high << 8 | low);

	write_to_bus(address, a);
	return cycles_SIXTEEN;
}

int CPU::LDH_A_C(byte& a, const byte& c) {
	ushort address = (ushort)(0xff << 8 | c);
	byte value = read_from_bus(address);

	a = value;
	return cycles_EIGHT;
}

int CPU::LDH_C_A(const byte& c, const byte& a) {
	ushort address = (ushort)(0xff << 8 | c);
	
	write_to_bus(address, a);
	return cycles_EIGHT;
}

int CPU::LDH_A_N8(byte& a) {
	byte n8 = fetch_next_byte();

	ushort address = (ushort)(0xff << 8 | n8);
	byte value = read_from_bus(address);
	
	a = value;
	return cycles_TWELVE;
}

int CPU::LDH_N8_A(const byte& a) {
	byte n8 = fetch_next_byte();

	ushort address = (ushort)(0xff << 8 | n8);
	write_to_bus(address, a);

	return cycles_TWELVE;
}

int CPU::LD_A_HLDEC(byte& a, byte& h, byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	a = value;
	hl--;

	h = (byte)(hl >> 8);
	l = (byte)(hl & 0xff);

	return cycles_EIGHT;
}

int CPU::LD_HLDEC_A(byte& h, byte& l, const byte& a) {
	ushort hl = (ushort)(h << 8 | l);
	
	write_to_bus(hl, a);
	hl--;

	h = (byte)(hl >> 8);
	l = (byte)(hl & 0xff);

	return cycles_EIGHT;
}

int CPU::LD_A_HLINC(byte& a, byte& h, byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	a = value;
	hl++;

	h = (byte)(hl >> 8);
	l = (byte)(hl & 0xff);

	return cycles_EIGHT;
}

int CPU::LD_HLINC_A(byte& h, byte& l, const byte& a) {
	ushort hl = (ushort)(h << 8 | l);

	write_to_bus(hl, a);
	hl++;

	h = (byte)(hl >> 8);
	l = (byte)(hl & 0xff);

	return cycles_EIGHT;
}

//16 bit load instructions

int CPU::LD_R16_N16(byte& register_high, byte& register_low) {
	byte n16_low = fetch_next_byte();
	byte n16_high = fetch_next_byte();

	register_high = n16_high;
	register_low = n16_low;

	return cycles_TWELVE;
}

int CPU::LD_N16_SP(const ushort& sp) {
	byte n16_low = fetch_next_byte();
	byte n16_high = fetch_next_byte();

	ushort address = (ushort)(n16_high << 8 | n16_low);
	byte sp_low = (byte)(sp & 0xff);
	byte sp_high = (byte)(sp >> 8);

	write_to_bus(address, sp_low);
	write_to_bus((ushort)(address + 1), sp_high);
	
	return cycles_TWENTY;
}

int CPU::LD_SP_N16(ushort& sp) {
	byte n16_low = fetch_next_byte();
	byte n16_high = fetch_next_byte();

	ushort n16 = (ushort)((n16_high << 8) | n16_low);
	sp = n16;
	return cycles_TWELVE;
}

int CPU::LD_SP_HL(ushort& sp, const byte& h, const byte& l) {
	ushort new_sp = (ushort)(h << 8 | l);
	sp = new_sp;
	internal_cycle_other_components();

	return cycles_EIGHT;
}

int CPU::PUSH_R16(ushort& sp, const byte& register_high, const byte& register_low) {
	internal_cycle_other_components();
	sp--;

	write_to_bus(sp, register_high);
	sp--;

	write_to_bus(sp, register_low);

	return cycles_SIXTEEN;
}

int CPU::POP_AF(ushort& sp, byte& a, byte& f) {
	byte new_f = read_from_bus(sp);
	sp++;

	byte new_a = read_from_bus(sp);
	sp++;

	a = new_a;
	f = new_f & 0xf0;
	return cycles_TWELVE;
}

int CPU::POP_R16(ushort& sp, byte& register_high, byte& register_low) {
	byte new_reg_low = read_from_bus(sp);
	sp++;

	byte new_reg_high = read_from_bus(sp);
	sp++;

	register_high = new_reg_high;
	register_low = new_reg_low;
	return cycles_TWELVE;
}

int CPU::LD_HL_SP_E8(byte& h, byte& l, const ushort& sp) {
	byte e8 = fetch_next_byte();

	byte sp_low = (byte)(sp & 0xff);
	int l_result = sp_low + e8;
	l = (byte)l_result;

	set_flag_state(flags_ZERO, false);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, (sp_low & 0xf) + (e8 & 0xf) > 0xf);
	set_flag_state(flags_CARRY, (sp_low + e8) > 0xff);

	bool z_sign = (sbyte)e8 < 0;
	byte adj = 0x00;
	if (z_sign) {
		adj = 0xff;
	}

	internal_cycle_other_components();

	int h_result = (sp >> 8) + adj + get_flag_state(flags_CARRY);
	h = h_result;
	return cycles_TWELVE;
}

//8 bit arithmetic and logic instructions

int CPU::ADD_R8(byte& a, const byte& register_value) {
	int result = a + register_value;
	bool new_half_carry = (a & 0xf) + (register_value & 0xf) > 0xf;
	bool new_carry = (a + register_value) > 0xff;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::ADD_HL(byte& a, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	int result = a + value;
	bool new_half_carry = (a & 0xf) + (value & 0xf) > 0xf;
	bool new_carry = (a + value) > 0xff;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::ADD_N8(byte& a) {
	byte n8 = fetch_next_byte();

	int result = a + n8;
	bool new_half_carry = (a & 0xf) + (n8 & 0xf) > 0xf;
	bool new_carry = (a + n8) > 0xff;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::ADC_R8(byte& a, const byte& register_value) {
	bool c_flag = get_flag_state(flags_CARRY);
	int result = a + register_value + c_flag;

	bool new_half_carry = (a & 0xf) + (register_value & 0xf) + c_flag > 0xf;
	bool new_carry = (a + register_value) + c_flag > 0xff;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::ADC_HL(byte& a, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	bool c_flag = get_flag_state(flags_CARRY);
	int result = a + value + c_flag;

	bool new_half_carry = (a & 0xf) + (value & 0xf) + c_flag > 0xf;
	bool new_carry = (a + value) + c_flag > 0xff;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::ADC_N8(byte& a) {
	byte n8 = fetch_next_byte();

	bool c_flag = get_flag_state(flags_CARRY);
	int result = a + n8 + c_flag;

	bool new_half_carry = (a & 0xf) + (n8 & 0xf) + c_flag > 0xf;
	bool new_carry = (a + n8) + c_flag > 0xff;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::SUB_R8(byte& a, const byte& register_value) {
	int result = a - register_value;
	bool new_half_carry = (a & 0xf) < (register_value & 0xf);
	bool new_carry = a < register_value;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::SUB_HL(byte& a, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	int result = a - value;
	bool new_half_carry = (a & 0xf) < (value & 0xf);
	bool new_carry = a < value;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::SUB_N8(byte& a) {
	byte n8 = fetch_next_byte();

	int result = a - n8;
	bool new_half_carry = (a & 0xf) < (n8 & 0xf);
	bool new_carry = a < n8;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::SBC_R8(byte& a, const byte& register_value) {
	bool c_flag = get_flag_state(flags_CARRY);
	
	int result = a - register_value - c_flag;
	bool new_half_carry = (a & 0xf) < (register_value & 0xf) + c_flag;
	bool new_carry = a < register_value + c_flag;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::SBC_HL(byte& a, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	bool c_flag = get_flag_state(flags_CARRY);

	int result = a - value - c_flag;
	bool new_half_carry = (a & 0xf) < (value & 0xf) + c_flag;
	bool new_carry = a < value + c_flag;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::SBC_N8(byte& a) {
	byte n8 = fetch_next_byte();

	bool c_flag = get_flag_state(flags_CARRY);

	int result = a - n8 - c_flag;
	bool new_half_carry = (a & 0xf) < (n8 & 0xf) + c_flag;
	bool new_carry = a < n8 + c_flag;

	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;

}

int CPU::CP_R8(const byte& a, const byte& register_value) {
	int result = a - register_value;
	bool new_half_carry = (a & 0xf) < (register_value & 0xf);
	bool new_carry = a < register_value;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::CP_HL(const byte& a, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	int result = a - value;
	bool new_half_carry = (a & 0xf) < (value & 0xf);
	bool new_carry = a < value;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::CP_N8(const byte& a) {
	byte n8 = fetch_next_byte();

	int result = a - n8;
	bool new_half_carry = (a & 0xf) < (n8 & 0xf);
	bool new_carry = a < n8;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::INC_R8(byte& register_value) {
	int result = register_value + 1;
	bool new_half_carry = (register_value & 0xf) + 1 > 0xf;

	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);

	return cycles_FOUR;
}

int CPU::INC_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	int result = value + 1;
	bool new_half_carry = (value & 0xf) + 1 > 0xf;

	write_to_bus(hl, (byte)result);

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);

	return cycles_EIGHT;
}

int CPU::DEC_R8(byte& register_value) {
	int result = register_value - 1;
	bool new_half_carry = (register_value & 0xf) < 1;

	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);

	return cycles_FOUR;
}

int CPU::DEC_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	int result = value - 1;
	bool new_half_carry = (value & 0xf) < 1;

	write_to_bus(hl, (byte)result);

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, new_half_carry);

	return cycles_EIGHT;
}

int CPU::AND_R8(byte& a, const byte& register_value) {
	int result = a & register_value;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, true);
	set_flag_state(flags_CARRY, false);

	return cycles_FOUR;
}

int CPU::AND_HL(byte& a, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	int result = a & value;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, true);
	set_flag_state(flags_CARRY, false);

	return cycles_EIGHT;
}

int CPU::AND_N8(byte& a) {
	byte n8 = fetch_next_byte();

	int result = a & n8;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, true);
	set_flag_state(flags_CARRY, false);

	return cycles_EIGHT;
}

int CPU::OR_R8(byte& a, const byte& register_value) {
	int result = a | register_value;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, false);

	return cycles_FOUR;
}

int CPU::OR_HL(byte& a, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	int result = a | value;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, false);

	return cycles_EIGHT;
}

int CPU::OR_N8(byte& a) {
	byte n8 = fetch_next_byte();

	int result = a | n8;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, false);

	return cycles_EIGHT;

}

int CPU::XOR_R8(byte& a, const byte& register_value) {
	int result = a ^ register_value;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, false);

	return cycles_FOUR;
}

int CPU::XOR_HL(byte& a, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	int result = a ^ value;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, false);

	return cycles_EIGHT;
}

int CPU::XOR_N8(byte& a) {
	byte n8 = fetch_next_byte();

	int result = a ^ n8;
	a = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, false);

	return cycles_EIGHT;

}

int CPU::CCF() {
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);

	bool c_flag = get_flag_state(flags_CARRY);
	set_flag_state(flags_CARRY, !c_flag);

	return cycles_FOUR;
}

int CPU::SCF() {
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, true);

	return cycles_FOUR;
}

int CPU::DAA(byte& a) {
	byte temp_a = a;
	byte adjustment = 0x00;

	bool set_carry_flag = false;
	bool n_flag = get_flag_state(flags_SUBTRACTION);
	bool h_flag = get_flag_state(flags_HALFCARRY);
	bool c_flag = get_flag_state(flags_CARRY);

	if (!n_flag) {
		if (h_flag || ((a & 0x0f) > 0x09)) {
			adjustment |= 0x06;
		}
		if (c_flag || (a > 0x99)) {
			adjustment |= 0x60;
			set_carry_flag = true;
		}

		temp_a += adjustment;
	}
	else {
		if (h_flag) {
			adjustment |= 0x06;
		}
		if (c_flag) {
			adjustment |= 0x60;
		}
		temp_a -= adjustment;
	}

	a = temp_a;

	set_flag_state(flags_ZERO, a == 0);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, set_carry_flag || c_flag);

	return cycles_FOUR;
}

int CPU::CPL(byte& a) {
	a = ~a;

	set_flag_state(flags_SUBTRACTION, true);
	set_flag_state(flags_HALFCARRY, true);

	return cycles_FOUR;
}

//16 bit arithmetic

int CPU::INC_R16(byte& register_high, byte& register_low) {
	ushort joined_register = (ushort)(register_high << 8 | register_low);
	int result = joined_register + 1;

	byte low_result = (byte)((ushort)result & 0xff);
	byte high_result = (byte)((ushort)result >> 8);

	register_high = high_result;
	register_low = low_result;

	internal_cycle_other_components();

	return cycles_EIGHT;
}

int CPU::DEC_R16(byte& register_high, byte& register_low) {
	ushort joined_register = (ushort)(register_high << 8 | register_low);
	int result = joined_register - 1;

	byte low_result = (byte)((ushort)result & 0xff);
	byte high_result = (byte)((ushort)result >> 8);

	register_high = high_result;
	register_low = low_result;

	internal_cycle_other_components();

	return cycles_EIGHT;
}

int CPU::ADD_HL_R16(byte& h, byte& l, const byte& register_high, const byte& register_low) {
	ushort hl = (ushort)(h << 8 | l);
	ushort joined_register = (ushort)(register_high << 8 | register_low);

	int result = hl + joined_register;

	bool new_half_carry = (hl & 0xfff) + (joined_register & 0xfff) > 0xfff;
	bool new_carry = hl + joined_register > 0xffff;

	l = (byte)((ushort)result & 0xff);

	internal_cycle_other_components();

	h = (byte)((ushort)result >> 8);

	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::ADD_SP_E8(ushort& sp) {
	sbyte e8 = (sbyte)fetch_next_byte();
	int result = sp + e8;

	byte sp_low = (byte)(sp & 0xff);
	bool new_half_carry = (sp_low & 0xf) + ((byte)e8 & 0xf) > 0xf;
	bool new_carry = sp_low + (byte)e8 > 0xff;

	set_flag_state(flags_ZERO, false);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, new_half_carry);
	set_flag_state(flags_CARRY, new_carry);

	internal_cycle_other_components();
	internal_cycle_other_components();

	sp = (ushort)result;
	return cycles_TWELVE;
}

//rotate shift and bit op instructions

int CPU::RLCA(byte& a) {
	byte b7 = (a >> 7) & 0x1;
	bool new_carry = b7 != 0;

	a = (a << 1) | b7;

	set_flag_state(flags_ZERO, false);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::RRCA(byte& a) {
	byte b0 = a & 0x1;
	bool new_carry = b0 != 0;

	a = (a >> 1) | (b0 << 7);

	set_flag_state(flags_ZERO, false);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::RLA(byte& a) {
	byte current_carry = (byte)get_flag_state(flags_CARRY);
	byte b7 = (a >> 7) & 0x1;
	bool new_carry = b7 != 0;

	a = (a << 1) | current_carry;

	set_flag_state(flags_ZERO, false);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::RRA(byte& a) {
	byte current_carry = (byte)get_flag_state(flags_CARRY);
	byte b0 = a & 0x1;
	bool new_carry = b0 != 0;

	a = (a >> 1) | (current_carry << 7);

	set_flag_state(flags_ZERO, false);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_FOUR;
}

int CPU::RLC_R8(byte& register_value) {
	byte b7 = (register_value >> 7) & 0x1;
	bool new_carry = b7 != 0;

	int result = (register_value << 1) | b7;
	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::RLC_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	byte b7 = (value >> 7) & 0x1;
	bool new_carry = b7 != 0;

	int result = (value << 1) | b7;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	write_to_bus(hl, (byte)result);
	return cycles_SIXTEEN;
}

int CPU::RRC_R8(byte& register_value) {
	byte b0 = register_value & 0x1;
	bool new_carry = b0 != 0;

	int result = (register_value >> 1) | (b0 << 7);
	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::RRC_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	byte b0 = value & 0x1;
	bool new_carry = b0 != 0;

	int result = (value >> 1) | (b0 << 7);

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	write_to_bus(hl, (byte)result);
	return cycles_SIXTEEN;
}

int CPU::RL_R8(byte& register_value) {
	byte b7 = (register_value >> 7) & 0x1;
	byte current_carry = (byte)get_flag_state(flags_CARRY);
	bool new_carry = b7 != 0;

	int result = (register_value << 1) | current_carry;
	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::RL_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	byte b7 = (value >> 7) & 0x1;
	byte current_carry = (byte)get_flag_state(flags_CARRY);
	bool new_carry = b7 != 0;

	int result = (value << 1) | current_carry;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	write_to_bus(hl, (byte)result);

	return cycles_SIXTEEN;
}

int CPU::RR_R8(byte& register_value) {
	byte b0 = register_value & 0x1;
	byte current_carry = (byte)get_flag_state(flags_CARRY);
	bool new_carry = b0 != 0;

	int result = (register_value >> 1) | (current_carry << 7);
	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::RR_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);
	
	byte b0 = value & 0x1;
	byte current_carry = (byte)get_flag_state(flags_CARRY);
	bool new_carry = b0 != 0;

	int result = (value >> 1) | (current_carry << 7);

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	write_to_bus(hl, (byte)result);

	return cycles_SIXTEEN;
}

int CPU::SLA_R8(byte& register_value) {
	byte b7 = (register_value >> 7) & 0x1;
	bool new_carry = b7 != 0;

	int result = (register_value << 1) | 0;
	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);
	
	return cycles_EIGHT;
}

int CPU::SLA_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	byte b7 = (value >> 7) & 0x1;
	bool new_carry = b7 != 0;

	int result = (value << 1) | 0;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	write_to_bus(hl, (byte)result);

	return cycles_SIXTEEN;
}

int CPU::SRA_R8(byte& register_value) {
	byte b7 = (register_value >> 7) & 0x1;
	byte b0 = register_value & 0x1;
	bool new_carry = b0 != 0;

	int result = (register_value >> 1) | (b7 << 7);
	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::SRA_HL(const byte& h, const byte& l) {
	ushort hl = (h << 8 | l);
	byte value = read_from_bus(hl);

	byte b7 = (value >> 7) & 0x1;
	byte b0 = value & 0x1;
	bool new_carry = b0 != 0;

	int result = (value >> 1) | (b7 << 7);

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	write_to_bus(hl, (byte)result);

	return cycles_SIXTEEN;
}

int CPU::SWAP_R8(byte& register_value) {
	int result = (register_value << 4) | (register_value >> 4);
	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, false);

	return cycles_EIGHT;
}

int CPU::SWAP_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);
	
	int result = (value << 4) | (value >> 4);

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, false);

	write_to_bus(hl, (byte)result);

	return cycles_SIXTEEN;
}

int CPU::SRL_R8(byte& register_value) {
	byte b0 = register_value & 0x1;
	bool new_carry = b0 != 0;

	int result = (register_value >> 1) & (~(0x1 << 7));
	register_value = (byte)result;

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	return cycles_EIGHT;
}

int CPU::SRL_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	byte b0 = value & 0x1;
	bool new_carry = b0 != 0;

	int result = (value >> 1) & (~(0x1 << 7));

	set_flag_state(flags_ZERO, (byte)result == 0);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, false);
	set_flag_state(flags_CARRY, new_carry);

	write_to_bus(hl, (byte)result);

	return cycles_SIXTEEN;
}

int CPU::BIT_B_R8(const int& bit, const byte& register_value) {
	bool new_zero = (register_value & (0x1 << bit)) != 0;

	set_flag_state(flags_ZERO, !new_zero);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, true);
	
	return cycles_EIGHT;
}

int CPU::BIT_B_HL(const int& bit, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	bool new_zero = (value & (0x1 << bit)) != 0;

	set_flag_state(flags_ZERO, !new_zero);
	set_flag_state(flags_SUBTRACTION, false);
	set_flag_state(flags_HALFCARRY, true);

	return cycles_TWELVE;
}

int CPU::RES_B_R8(const int& bit, byte& register_value) {
	byte mask = ~(1 << bit);
	register_value &= mask;

	return cycles_EIGHT;
}

int CPU::RES_B_HL(const int& bit, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	byte mask = ~(1 << bit);
	value &= mask;

	write_to_bus(hl, value);
	return cycles_SIXTEEN;
}

int CPU::SET_B_R8(const int& bit, byte& register_value) {
	byte set = (1 << bit);
	register_value |= set;

	return cycles_EIGHT;
}

int CPU::SET_B_HL(const int& bit, const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	byte value = read_from_bus(hl);

	byte set = (1 << bit);
	value |= set;

	write_to_bus(hl, value);
	return cycles_SIXTEEN;
}

//control flow instructions

int CPU::JP_N16() {
	byte n16_low = fetch_next_byte();
	byte n16_high = fetch_next_byte();

	ushort jump = (ushort)((n16_high << 8) | n16_low);

	data.pc = jump;
	internal_cycle_other_components();

	return cycles_SIXTEEN;
}

int CPU::JP_HL(const byte& h, const byte& l) {
	ushort hl = (ushort)(h << 8 | l);
	data.pc = hl;

	return cycles_FOUR;
}

int CPU::JP_CC_N16(const bool& condition) {
	byte n16_low = fetch_next_byte();
	byte n16_high = fetch_next_byte();

	ushort jump = (ushort)((n16_high << 8) | n16_low);
	
	if (condition) {
		data.pc = jump;
		internal_cycle_other_components();
		return cycles_SIXTEEN;
	}

	return cycles_TWELVE;
}

int CPU::JR_E8() {
	sbyte e8 = (sbyte)fetch_next_byte();
	int result = data.pc + e8;

	internal_cycle_other_components();

	data.pc = (ushort)result;
	return cycles_TWELVE;
}

int CPU::JR_CC_E8(const bool& condition) {
	sbyte e8 = (sbyte)fetch_next_byte();
	int result = data.pc + e8;

	if (condition) {
		data.pc = (ushort)result;
		internal_cycle_other_components();
		return cycles_TWELVE;
	}

	return cycles_EIGHT;
}

int CPU::CALL_N16(ushort& sp) {
	byte n16_low = fetch_next_byte();
	byte n16_high = fetch_next_byte();

	ushort call = (ushort)((n16_high << 8) | n16_low);

	byte pc_low = data.pc & 0xff;
	byte pc_high = data.pc >> 8;

	internal_cycle_other_components();

	sp--;
	write_to_bus(sp, pc_high);
	sp--;
	write_to_bus(sp, pc_low);

	data.pc = call;
	return cycles_TWENTYFOUR;
}

int CPU::CALL_CC_N16(ushort& sp, const bool& condition) {
	byte n16_low = fetch_next_byte();
	byte n16_high = fetch_next_byte();

	ushort call = (ushort)((n16_high << 8) | n16_low);

	byte pc_low = data.pc & 0xff;
	byte pc_high = data.pc >> 8;

	if (condition) {
		sp--;
		internal_cycle_other_components();
		write_to_bus(sp, pc_high);
		sp--;
		write_to_bus(sp, pc_low);

		data.pc = call;
		return cycles_TWENTYFOUR;
	}

	return cycles_TWELVE;
}

int CPU::RET(ushort& sp) {
	byte ret_low = read_from_bus(sp);
	sp++;
	byte ret_high = read_from_bus(sp);
	sp++;

	ushort ret = (ushort)((ret_high << 8) | ret_low);

	data.pc = ret;
	internal_cycle_other_components();

	return cycles_SIXTEEN;
}

int CPU::RET_CC(ushort& sp, const bool& condition) {
	internal_cycle_other_components();

	if (condition) {
		byte ret_low = read_from_bus(sp);
		sp++;
		byte ret_high = read_from_bus(sp);
		sp++;

		ushort ret = (ushort)((ret_high << 8) | ret_low);

		data.pc = ret;
		internal_cycle_other_components();
		return cycles_TWENTY;
	}

	return cycles_EIGHT;
}

int CPU::RETI(ushort& sp) {
	byte ret_low = read_from_bus(sp);
	sp++;
	byte ret_high = read_from_bus(sp);
	sp++;

	ushort ret = (ushort)((ret_high << 8) | ret_low);

	data.pc = ret;
	data.ime = true;
	internal_cycle_other_components();
	return cycles_SIXTEEN;
}

int CPU::RST_N8(ushort& sp, const byte& vector) {
	sp--;
	internal_cycle_other_components();

	byte pc_low = data.pc & 0xff;
	byte pc_high = data.pc >> 8;

	write_to_bus(sp, pc_high);
	sp--;
	write_to_bus(sp, pc_low);

	data.pc = (ushort)(0x0000 | vector);
	return cycles_SIXTEEN;
}

//misc instructions

int CPU::STOP() {
	//todo
	printf("not impl\n");
	return cycles_FOUR;
}

int CPU::DI() {
	data.ime = false;
	return cycles_FOUR;
}

int CPU::EI() {
	t = 2;
	enable_ime_next_cycle = true;
	return cycles_FOUR;
}
