#pragma once
#include "../externals/nlohmann/json.hpp"
#include <fstream>
#include <array>

#include "_definitions.h"
#include <memory>
#include <string>
#include <vector>

class Emulator;

struct cpu_data {
	byte a = 0x00;
	byte f = 0x00;
	byte b = 0x00;
	byte c = 0x00;
	byte d = 0x00;
	byte e = 0x00;
	byte h = 0x00;
	byte l = 0x00;

	ushort pc = 0x0000;
	ushort sp = 0x0000;

	bool ime = false;
	bool halted = false;
};

struct single_step_test_cycle {
	ushort address = 0x0000;
	byte value = 0x00;
	std::string operation = std::string("");
};

class CPU {
public:
	CPU(std::shared_ptr<Emulator> emulator_ptr);
	~CPU();

	const bool is_cpu_initialised();
	void reset_cpu(const bool& using_boot_rom, const bool& check_sum_zero);

	void step_cpu(int& cycles, const bool& print_debug_to_console);
	//shutdown

private:
	void internal_cycle_other_components();
	byte fetch_opcode();
	byte fetch_next_byte();

	//todo move interrupts to be checked every t cycle and do a flag to emu which can be checked rather than memory reads etc
	const byte is_interrupt_pending();
	int handle_interupts(int& cycles);

	void execute_opcode(int& cycles, const byte& opcode);
	void execute_cb_opcode(int& cycles);

	//read/write to memory
	byte read_from_bus(const ushort& address);
	void write_to_bus(const ushort& address, const byte& value);

	const bool get_flag_state(const cpu_flags& flag);
	void set_flag_state(const cpu_flags& flag, const bool& state);

private:
	std::shared_ptr<Emulator> emulator_ptr;
	bool initialised = false;

	cpu_data data;
	bool enable_ime_next_cycle = false;
	byte interrupt_pending = 0x00;

	int t = 2;

	bool halt_bug_next_instruction = false;
private:
	//opcode functions (include memory vector and cycles for testing)

	//8 bit load instructions
	int LD_R8_R8(byte& register_one, const byte& register_two);
	int LD_R8_N8(byte& register_one);
	int LD_R8_HL(byte& register_one, const byte& h, const byte& l);
	int LD_HL_R8(const byte& h, const byte& l, const byte& );
	int LD_HL_N8(const byte& h, const byte& l);
	int LD_A_R16(byte& a, const byte& register_high, const byte& register_low);
	int LD_R16_A(const byte& register_high, const byte& register_low, const byte& a);
	int LD_A_N16(byte& a);
	int LD_N16_A(const byte& a);
	int LDH_A_C(byte& a, const byte& c);
	int LDH_C_A(const byte& c, const byte& a);
	int LDH_A_N8(byte& a);
	int LDH_N8_A(const byte& a);
	int LD_A_HLDEC(byte& a, byte& h, byte& l);
	int LD_HLDEC_A(byte& h, byte& l, const byte& a);
	int LD_A_HLINC(byte& a, byte& h, byte& l);
	int LD_HLINC_A(byte& h, byte& l, const byte& a);

	//16 bit load instructions
	int LD_R16_N16(byte& register_high, byte& register_low);
	int LD_N16_SP(const ushort& sp);
	int LD_SP_N16(ushort& sp);
	int LD_SP_HL(ushort& sp, const byte& h, const byte& l);
	int PUSH_R16(ushort& sp, const byte& register_high, const byte& register_low);
	int POP_AF(ushort& sp, byte& a, byte& f);
	int POP_R16(ushort& sp, byte& register_high, byte& register_low);
	int LD_HL_SP_E8(byte& h, byte& l, const ushort& sp);

	//8 bit arithmetic and logic instructions
	int ADD_R8(byte& a, const byte& register_value);
	int ADD_HL(byte& a, const byte& h, const byte& l);
	int ADD_N8(byte& a);
	int ADC_R8(byte& a, const byte& register_value);
	int ADC_HL(byte& a, const byte& h, const byte& l);
	int ADC_N8(byte& a);
	int SUB_R8(byte& a, const byte& register_value);
	int SUB_HL(byte& a, const byte& h, const byte& l);
	int SUB_N8(byte& a);
	int SBC_R8(byte& a, const byte& register_value);
	int SBC_HL(byte& a, const byte& h, const byte& l);
	int SBC_N8(byte& a);
	int CP_R8(const byte& a, const byte& register_value);
	int CP_HL(const byte& a, const byte& h, const byte& l);
	int CP_N8(const byte& a);
	int INC_R8(byte& register_value);
	int INC_HL(const byte& h, const byte& l);
	int DEC_R8(byte& register_value);
	int DEC_HL(const byte& h, const byte& l);
	int AND_R8(byte& a, const byte& register_value);
	int AND_HL(byte& a, const byte& h, const byte& l);
	int AND_N8(byte& a);
	int OR_R8(byte& a, const byte& register_value);
	int OR_HL(byte& a, const byte& h, const byte& l);
	int OR_N8(byte& a);
	int XOR_R8(byte& a, const byte& register_value);
	int XOR_HL(byte& a, const byte& h, const byte& l);
	int XOR_N8(byte& a);
	int CCF();
	int SCF();
	int DAA(byte& a);
	int CPL(byte& a);

	//16 bit arithmetic instructions
	int INC_R16(byte& register_high, byte& register_low);
	int DEC_R16(byte& register_high, byte& register_low);
	int ADD_HL_R16(byte& h, byte& l, const byte& register_high, const byte& register_low);
	int ADD_SP_E8(ushort& sp);

	//rotate shift and bit op instructions
	int RLCA(byte& a);
	int RRCA(byte& a);
	int RLA(byte& a);
	int RRA(byte& a);
	int RLC_R8(byte& register_value);
	int RLC_HL(const byte& h, const byte& l);
	int RRC_R8(byte& register_value);
	int RRC_HL(const byte& h, const byte& l);
	int RL_R8(byte& register_value);
	int RL_HL(const byte& h, const byte& l);
	int RR_R8(byte& register_value);
	int RR_HL(const byte& h, const byte& l);
	int SLA_R8(byte& register_value);
	int SLA_HL(const byte& h, const byte& l);
	int SRA_R8(byte& register_value);
	int SRA_HL(const byte& h, const byte& l);
	int SWAP_R8(byte& register_value);
	int SWAP_HL(const byte& h, const byte& l);
	int SRL_R8(byte& register_value);
	int SRL_HL(const byte& h, const byte& l);
	int BIT_B_R8(const int& bit, const byte& register_value);
	int BIT_B_HL(const int& bit, const byte& h, const byte& l);
	int RES_B_R8(const int& bit, byte& register_value);
	int RES_B_HL(const int& bit, const byte& h, const byte& l);
	int SET_B_R8(const int& bit, byte& register_value);
	int SET_B_HL(const int& bit, const byte& h, const byte& l);

	//control flow instructions
	int JP_N16();
	int JP_HL(const byte& h, const byte& l);
	int JP_CC_N16(const bool& condition);
	int JR_E8();
	int JR_CC_E8(const bool& condition);
	int CALL_N16(ushort& sp);
	int CALL_CC_N16(ushort& sp, const bool& condition);
	int RET(ushort& sp);
	int RET_CC(ushort& sp, const bool& condition);
	int RETI(ushort& sp);
	int RST_N8(ushort& sp, const byte& vector);

	//misc instructions
	int HALT();
	int STOP();
	int DI();
	int EI();
};