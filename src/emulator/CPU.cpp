#include "CPU.h"
#include "Emulator.h"

CPU::CPU(std::shared_ptr<Emulator> emulator_ptr) {
	//set pointer to parent emu and check if it was nullptr or not
	this->emulator_ptr = emulator_ptr;
	if (this->emulator_ptr != nullptr) {
		initialised = true;
	}
	else {
		initialised = false;
	}
}

CPU::~CPU() {
	//set the emu pointer to nullpointer to -1 ref count
	this->emulator_ptr = nullptr;
	printf("[SB] Shutting down CPU object.\n");
}

//public
const bool CPU::is_cpu_initialised() {
	return &initialised;
}

void CPU::reset_cpu(const bool& using_boot_rom, const bool& check_sum_zero) {
	std::unique_ptr<cpu_data> new_data = std::make_unique<cpu_data>();
	
	new_data->a = 0x00;
	new_data->f = 0x00;
	new_data->b = 0x00;
	new_data->c = 0x00;
	new_data->d = 0x00;
	new_data->e = 0x00;
	new_data->h = 0x00;
	new_data->l = 0x00;

	new_data->pc = 0x0000;
	new_data->sp = 0x0000;

	if (using_boot_rom) {
		this->data = *new_data;
		return;
	}

	new_data->a = 0x01;
	new_data->f = check_sum_zero ? 0x80 : 0xb0;
	new_data->b = 0x00;
	new_data->c = 0x13;
	new_data->d = 0x00;
	new_data->e = 0xd8;
	new_data->h = 0x01;
	new_data->l = 0x4d;

	new_data->pc = 0x0100;
	new_data->sp = 0xfffe;

	this->data = *new_data;
}

void CPU::step_cpu(int& cycles, const bool& print_debug_to_console) {
	/*
	FILE* f = fopen("logs.txt", "w");
	if (f) {
		byte one = emulator_ptr->memory_instant_read(data.pc);
		byte two = emulator_ptr->memory_instant_read(data.pc + 1);
		byte three = emulator_ptr->memory_instant_read(data.pc + 2);
		byte four = emulator_ptr->memory_instant_read(data.pc + 3);

		fprintf(f, "A: %02X A: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n",
			data.a, data.f, data.b, data.c, data.d, data.e, data.h, data.l, data.sp, data.pc, one, two, three, four);

		printf("A: %02X A: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n",
			data.a, data.f, data.b, data.c, data.d, data.e, data.h, data.l, data.sp, data.pc, one, two, three, four);
		fclose(f);
	}
	*/
	
	if (false) {
		byte one = emulator_ptr->memory_instant_read(data.pc);
		byte two = emulator_ptr->memory_instant_read(data.pc + 1);
		byte three = emulator_ptr->memory_instant_read(data.pc + 2);
		byte four = emulator_ptr->memory_instant_read(data.pc + 3);

		printf("A: %02X A: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n",
			data.a, data.f, data.b, data.c, data.d, data.e, data.h, data.l, data.sp, data.pc, one, two, three, four);
	}
	

	if (data.halted) {
		interrupt_pending = is_interrupt_pending();
		if ((interrupt_pending != 0x00)) {
			data.halted = false;
			return;
		}
		else {
			emulator_ptr->tick_other_components(4);
			cycles += 4;
			
			return;
		}
	}
	else {
		byte opcode = fetch_opcode();

		int t_cycles = 0;
		t_cycles = handle_interupts(cycles);
		if (t_cycles > 0) {
			return;
		}

		emulator_ptr->tick_other_components(2);

		if (enable_ime_next_cycle) {
			data.ime = true;
			enable_ime_next_cycle = false;
		}

		if (halt_bug_next_instruction) {
			data.pc--;
			halt_bug_next_instruction = false;
		}

		execute_opcode(cycles, opcode);
	}
}

byte CPU::fetch_opcode() {
	emulator_ptr->tick_other_components(2);
	
	byte opcode = emulator_ptr->fetch_next_byte(data.pc);
	data.pc++;

	interrupt_pending = is_interrupt_pending(); //check opcodes on t 3 of fetch

	return opcode;
}

byte CPU::fetch_next_byte() {
	emulator_ptr->tick_other_components(2);
	byte value = emulator_ptr->fetch_next_byte(data.pc);
	data.pc++;

	emulator_ptr->tick_other_components(2);
	return value;
}

//privates
int CPU::HALT() {
	if (interrupt_pending != 0 && !data.ime) {
		halt_bug_next_instruction = true;
		data.halted = false;

		return cycles_NONE;
	}

	data.halted = true;

	return cycles_NONE;
}

void CPU::internal_cycle_other_components() {
	emulator_ptr->tick_other_components(4);
}

const byte CPU::is_interrupt_pending() {
	byte IF = emulator_ptr->io_instant_read(io_IF);
	byte IE = emulator_ptr->memory_instant_read(0xffff);

	return ((IF & IE) & 0x1f);
}

int CPU::handle_interupts(int& cycles) {
	if (interrupt_pending != 0x00 && data.ime) {
		emulator_ptr->tick_other_components(2); //allign to clock on fetch of opcode
		cycles += 4; //account for 2 tick for opcode fetch

		data.pc--;
		internal_cycle_other_components(); //dec pc tick 4

		data.ime = false;
		data.halted = false;

		data.sp--;
		internal_cycle_other_components(); //sp-- tick 4
		cycles += 4;

		byte pc_low = (byte)(data.pc & 0xff);
		byte pc_high = (byte)(data.pc >> 8);

		write_to_bus(data.sp, pc_high); //write pc high tick 4
		cycles += 4;

		interrupt_pending = is_interrupt_pending(); // recheck interrupts 

		data.sp--;
		write_to_bus(data.sp, pc_low); //write pc low tick 4
		cycles += 4;

		int bit = -1;
		for (int i = 0; i < 5; i++) {
			if (interrupt_pending & (1 << i)) {
				bit = i;
				break;
			}
		}

		bool cleared_ie = false;
		ushort interrupt_vector = 0x0000;
		switch (bit) {
		case 0: interrupt_vector = 0x40; break; // V-Blank
		case 1: interrupt_vector = 0x48; break; // LCD STAT
		case 2: interrupt_vector = 0x50; break; // Timer
		case 3: interrupt_vector = 0x58; break; // Serial
		case 4: interrupt_vector = 0x60; break; // Joypad
		default:
			cleared_ie = true; break;
		}

		data.pc = interrupt_vector;

		//if we havent cleared our interrupt on the push, clear the bit in IF to service interrupt
		if (!cleared_ie) {
			emulator_ptr->clear_interrupt(bit);
		}

		return cycles_TWENTY;
	}

	return cycles_NONE;
}

void CPU::execute_opcode(int& cycles, const byte& opcode) {
	cycles = 0;
	switch (opcode) {
		//0x00->0x0f
	case inst_NOOP: cycles += 4; return;
	case inst_LD_BC_N16: cycles = LD_R16_N16(data.b, data.c); return;
	case inst_LD_BC_A: cycles = LD_R16_A(data.b, data.c, data.a); return;
	case inst_INC_BC: cycles = INC_R16(data.b, data.c); return;
	case inst_INC_B: cycles = INC_R8(data.b); return;
	case inst_DEC_B: cycles = DEC_R8(data.b); return;
	case inst_LD_B_N8: cycles = LD_R8_N8(data.b); return;
	case inst_RLCA: cycles = RLCA(data.a); return;
	case inst_LD_N16_SP: cycles = LD_N16_SP(data.sp); return;
	case inst_ADD_HL_BC: cycles = ADD_HL_R16(data.h, data.l, data.b, data.c); return;
	case inst_LD_A_BC: cycles = LD_A_R16(data.a, data.b, data.c); return;
	case inst_DEC_BC: cycles = DEC_R16(data.b, data.c); return;
	case inst_INC_C: cycles = INC_R8(data.c); return;
	case inst_DEC_C: cycles = DEC_R8(data.c); return;
	case inst_LD_C_N8: cycles = LD_R8_N8(data.c); return;
	case inst_RRCA: cycles = RRCA(data.a); return;

		//0x10->0x1f
	case inst_STOP_N8: cycles = STOP(); return;
	case inst_LD_DE_N16: cycles = LD_R16_N16(data.d, data.e); return;
	case inst_LD_DE_A: cycles = LD_R16_A(data.d, data.e, data.a); return;
	case inst_INC_DE: cycles = INC_R16(data.d, data.e); return;
	case inst_INC_D: cycles = INC_R8(data.d); return;
	case inst_DEC_D: cycles = DEC_R8(data.d); return;
	case inst_LD_D_N8: cycles = LD_R8_N8(data.d); return;
	case inst_RLA: cycles = RLA(data.a); return;
	case inst_JR_E8: cycles = JR_E8(); return;
	case inst_ADD_HL_DE: cycles = ADD_HL_R16(data.h, data.l, data.d, data.e); return;
	case inst_LD_A_DE: cycles = LD_A_R16(data.a, data.d, data.e); return;
	case inst_DEC_DE: cycles = DEC_R16(data.d, data.e); return;
	case inst_INC_E: cycles = INC_R8(data.e); return;
	case inst_DEC_E: cycles = DEC_R8(data.e); return;
	case inst_LD_E_N8: cycles = LD_R8_N8(data.e); return;
	case inst_RRA: cycles = RRA(data.a); return;

		//0x20-0x2f
	case inst_JR_NZ_E8: cycles = JR_CC_E8(!get_flag_state(flags_ZERO)); return;
	case inst_LD_HL_N16: cycles = LD_R16_N16(data.h, data.l); return;
	case inst_LDI_HL_A: cycles = LD_HLINC_A(data.h, data.l, data.a); return;
	case inst_INC_HL: cycles = INC_R16(data.h, data.l); return;
	case inst_INC_H: cycles = INC_R8(data.h); return;
	case inst_DEC_H: cycles = DEC_R8(data.h); return;
	case inst_LD_H_N8: cycles = LD_R8_N8(data.h); return;
	case inst_DAA: cycles = DAA(data.a); return;
	case inst_JR_Z_E8: cycles = JR_CC_E8(get_flag_state(flags_ZERO)); return;
	case inst_ADD_HL_HL: cycles = ADD_HL_R16(data.h, data.l, data.h, data.l); return;
	case inst_LD_A_HLI: cycles = LD_A_HLINC(data.a, data.h, data.l); return;
	case inst_DEC_HL: cycles = DEC_R16(data.h, data.l); return;
	case inst_INC_L: cycles = INC_R8(data.l); return;
	case inst_DEC_L: cycles = DEC_R8(data.l); return;
	case inst_LD_L_N8: cycles = LD_R8_N8(data.l); return;
	case inst_CPL: cycles = CPL(data.a); return;

		//0x30->0x3f
	case inst_JR_NC_E8: cycles = JR_CC_E8(!get_flag_state(flags_CARRY)); return;
	case inst_LD_SP_N16: cycles = LD_SP_N16(data.sp); return;
	case inst_LDD_HL_A: cycles = LD_HLDEC_A(data.h, data.l, data.a); return;
	case inst_INC_SP: cycles += 8; data.sp++; internal_cycle_other_components(); return;
	case inst_INC_memHL: cycles = INC_HL(data.h, data.l); return;
	case inst_DEC_memHL: cycles = DEC_HL(data.h, data.l); return;
	case inst_LD_HL_N8: cycles = LD_HL_N8(data.h, data.l); return;
	case inst_SCF: cycles = SCF(); return;
	case inst_JR_C_E8: cycles = JR_CC_E8(get_flag_state(flags_CARRY)); return;
	case inst_ADD_HL_SP: cycles = ADD_HL_R16(data.h, data.l, (byte)(data.sp >> 8), (byte)(data.sp & 0xff)); return;
	case inst_LD_A_HLD: cycles = LD_A_HLDEC(data.a, data.h, data.l); return;
	case inst_DEC_SP: cycles += 8; data.sp--; internal_cycle_other_components(); return;
	case inst_INC_A: cycles = INC_R8(data.a); return;
	case inst_DEC_A: cycles = DEC_R8(data.a); return;
	case inst_LD_A_N8: cycles = LD_R8_N8(data.a); return;
	case inst_CCF: cycles = CCF(); return;

		//0x40->0x4f
	case inst_LD_B_B: cycles = LD_R8_R8(data.b, data.b); return;
	case inst_LD_B_C: cycles = LD_R8_R8(data.b, data.c); return;
	case inst_LD_B_D: cycles = LD_R8_R8(data.b, data.d); return;
	case inst_LD_B_E: cycles = LD_R8_R8(data.b, data.e); return;
	case inst_LD_B_H: cycles = LD_R8_R8(data.b, data.h); return;
	case inst_LD_B_L: cycles = LD_R8_R8(data.b, data.l); return;
	case inst_LD_B_HL: cycles = LD_R8_HL(data.b, data.h, data.l); return;
	case inst_LD_B_A: cycles = LD_R8_R8(data.b, data.a); return;
	case inst_LD_C_B: cycles = LD_R8_R8(data.c, data.b); return;
	case inst_LD_C_C: cycles = LD_R8_R8(data.c, data.c); return;
	case inst_LD_C_D: cycles = LD_R8_R8(data.c, data.d); return;
	case inst_LD_C_E: cycles = LD_R8_R8(data.c, data.e); return;
	case inst_LD_C_H: cycles = LD_R8_R8(data.c, data.h); return;
	case inst_LD_C_L: cycles = LD_R8_R8(data.c, data.l); return;
	case inst_LD_C_HL: cycles = LD_R8_HL(data.c, data.h, data.l); return;
	case inst_LD_C_A: cycles = LD_R8_R8(data.c, data.a); return;

		//0x50->0x5f
	case inst_LD_D_B: cycles = LD_R8_R8(data.d, data.b); return;
	case inst_LD_D_C: cycles = LD_R8_R8(data.d, data.c); return;
	case inst_LD_D_D: cycles = LD_R8_R8(data.d, data.d); return;
	case inst_LD_D_E: cycles = LD_R8_R8(data.d, data.e); return;
	case inst_LD_D_H: cycles = LD_R8_R8(data.d, data.h); return;
	case inst_LD_D_L: cycles = LD_R8_R8(data.d, data.l); return;
	case inst_LD_D_HL: cycles = LD_R8_HL(data.d, data.h, data.l); return;
	case inst_LD_D_A: cycles = LD_R8_R8(data.d, data.a); return;
	case inst_LD_E_B: cycles = LD_R8_R8(data.e, data.b); return;
	case inst_LD_E_C: cycles = LD_R8_R8(data.e, data.c); return;
	case inst_LD_E_D: cycles = LD_R8_R8(data.e, data.d); return;
	case inst_LD_E_E: cycles = LD_R8_R8(data.e, data.e); return;
	case inst_LD_E_H: cycles = LD_R8_R8(data.e, data.h); return;
	case inst_LD_E_L: cycles = LD_R8_R8(data.e, data.l); return;
	case inst_LD_E_HL: cycles = LD_R8_HL(data.e, data.h, data.l); return;
	case inst_LD_E_A: cycles = LD_R8_R8(data.e, data.a); return;

		//0x60->0x6f
	case inst_LD_H_B: cycles = LD_R8_R8(data.h, data.b); return;
	case inst_LD_H_C: cycles = LD_R8_R8(data.h, data.c); return;
	case inst_LD_H_D: cycles = LD_R8_R8(data.h, data.d); return;
	case inst_LD_H_E: cycles = LD_R8_R8(data.h, data.e); return;
	case inst_LD_H_H: cycles = LD_R8_R8(data.h, data.h); return;
	case inst_LD_H_L: cycles = LD_R8_R8(data.h, data.l); return;
	case inst_LD_H_HL: cycles = LD_R8_HL(data.h, data.h, data.l); return;
	case inst_LD_H_A: cycles = LD_R8_R8(data.h, data.a); return;
	case inst_LD_L_B: cycles = LD_R8_R8(data.l, data.b); return;
	case inst_LD_L_C: cycles = LD_R8_R8(data.l, data.c); return;
	case inst_LD_L_D: cycles = LD_R8_R8(data.l, data.d); return;
	case inst_LD_L_E: cycles = LD_R8_R8(data.l, data.e); return;
	case inst_LD_L_H: cycles = LD_R8_R8(data.l, data.h); return;
	case inst_LD_L_L: cycles = LD_R8_R8(data.l, data.l); return;
	case inst_LD_L_HL: cycles = LD_R8_HL(data.l, data.h, data.l); return;
	case inst_LD_L_A: cycles = LD_R8_R8(data.l, data.a); return;

		//0x70->0x7f
	case inst_LD_HL_B: cycles = LD_HL_R8(data.h, data.l, data.b); return;
	case inst_LD_HL_C: cycles = LD_HL_R8(data.h, data.l, data.c); return;
	case inst_LD_HL_D: cycles = LD_HL_R8(data.h, data.l, data.d); return;
	case inst_LD_HL_E: cycles = LD_HL_R8(data.h, data.l, data.e); return;
	case inst_LD_HL_H: cycles = LD_HL_R8(data.h, data.l, data.h); return;
	case inst_LD_HL_L: cycles = LD_HL_R8(data.h, data.l, data.l); return;
	case inst_HALT: HALT(); return;//not impl
		return;
	case inst_LD_HL_A: cycles = LD_HL_R8(data.h, data.l, data.a); return;
	case inst_LD_A_B: cycles = LD_R8_R8(data.a, data.b); return;
	case inst_LD_A_C: cycles = LD_R8_R8(data.a, data.c); return;
	case inst_LD_A_D: cycles = LD_R8_R8(data.a, data.d); return;
	case inst_LD_A_E: cycles = LD_R8_R8(data.a, data.e); return;
	case inst_LD_A_H: cycles = LD_R8_R8(data.a, data.h); return;
	case inst_LD_A_L: cycles = LD_R8_R8(data.a, data.l); return;
	case inst_LD_A_HL: cycles = LD_R8_HL(data.a, data.h, data.l); return;
	case inst_LD_A_A: cycles = LD_R8_R8(data.a, data.a); return;

		//0x80->0x8f 
	case inst_ADD_A_B: cycles = ADD_R8(data.a, data.b); return;
	case inst_ADD_A_C: cycles = ADD_R8(data.a, data.c); return;
	case inst_ADD_A_D: cycles = ADD_R8(data.a, data.d); return;
	case inst_ADD_A_E: cycles = ADD_R8(data.a, data.e); return;
	case inst_ADD_A_H: cycles = ADD_R8(data.a, data.h); return;
	case inst_ADD_A_L: cycles = ADD_R8(data.a, data.l); return;
	case inst_ADD_A_HL: cycles = ADD_HL(data.a, data.h, data.l); return;
	case inst_ADD_A_A: cycles = ADD_R8(data.a, data.a); return;
	case inst_ADC_A_B: cycles = ADC_R8(data.a, data.b); return;
	case inst_ADC_A_C: cycles = ADC_R8(data.a, data.c); return;
	case inst_ADC_A_D: cycles = ADC_R8(data.a, data.d); return;
	case inst_ADC_A_E: cycles = ADC_R8(data.a, data.e); return;
	case inst_ADC_A_H: cycles = ADC_R8(data.a, data.h); return;
	case inst_ADC_A_L: cycles = ADC_R8(data.a, data.l); return;
	case inst_ADC_A_HL: cycles = ADC_HL(data.a, data.h, data.l); return;
	case inst_ADC_A_A: cycles = ADC_R8(data.a, data.a); return;

		//0x90->0x9f
	case inst_SUB_A_B: cycles = SUB_R8(data.a, data.b); return;
	case inst_SUB_A_C: cycles = SUB_R8(data.a, data.c); return;
	case inst_SUB_A_D: cycles = SUB_R8(data.a, data.d); return;
	case inst_SUB_A_E: cycles = SUB_R8(data.a, data.e); return;
	case inst_SUB_A_H: cycles = SUB_R8(data.a, data.h); return;
	case inst_SUB_A_L: cycles = SUB_R8(data.a, data.l); return;
	case inst_SUB_A_HL: cycles = SUB_HL(data.a, data.h, data.l); return;
	case inst_SUB_A_A: cycles = SUB_R8(data.a, data.a); return;
	case inst_SBC_A_B: cycles = SBC_R8(data.a, data.b); return;
	case inst_SBC_A_C: cycles = SBC_R8(data.a, data.c); return;
	case inst_SBC_A_D: cycles = SBC_R8(data.a, data.d); return;
	case inst_SBC_A_E: cycles = SBC_R8(data.a, data.e); return;
	case inst_SBC_A_H: cycles = SBC_R8(data.a, data.h); return;
	case inst_SBC_A_L: cycles = SBC_R8(data.a, data.l); return;
	case inst_SBC_A_HL: cycles = SBC_HL(data.a, data.h, data.l); return;
	case inst_SBC_A_A: cycles = SBC_R8(data.a, data.a); return;

		//0xa0->0xaf
	case inst_AND_A_B: cycles = AND_R8(data.a, data.b); return;
	case inst_AND_A_C: cycles = AND_R8(data.a, data.c); return;
	case inst_AND_A_D: cycles = AND_R8(data.a, data.d); return;
	case inst_AND_A_E: cycles = AND_R8(data.a, data.e); return;
	case inst_AND_A_H: cycles = AND_R8(data.a, data.h); return;
	case inst_AND_A_L: cycles = AND_R8(data.a, data.l); return;
	case inst_AND_A_HL: cycles = AND_HL(data.a, data.h, data.l); return;
	case inst_AND_A_A: cycles = AND_R8(data.a, data.a); return;
	case inst_XOR_A_B: cycles = XOR_R8(data.a, data.b); return;
	case inst_XOR_A_C: cycles = XOR_R8(data.a, data.c); return;
	case inst_XOR_A_D: cycles = XOR_R8(data.a, data.d); return;
	case inst_XOR_A_E: cycles = XOR_R8(data.a, data.e); return;
	case inst_XOR_A_H: cycles = XOR_R8(data.a, data.h); return;
	case inst_XOR_A_L: cycles = XOR_R8(data.a, data.l); return;
	case inst_XOR_A_HL: cycles = XOR_HL(data.a, data.h, data.l); return;
	case inst_XOR_A_A: cycles = XOR_R8(data.a, data.a); return;

		//0xb0->0xbf
	case inst_OR_A_B: cycles = OR_R8(data.a, data.b); return;
	case inst_OR_A_C: cycles = OR_R8(data.a, data.c); return;
	case inst_OR_A_D: cycles = OR_R8(data.a, data.d); return;
	case inst_OR_A_E: cycles = OR_R8(data.a, data.e); return;
	case inst_OR_A_H: cycles = OR_R8(data.a, data.h); return;
	case inst_OR_A_L: cycles = OR_R8(data.a, data.l); return;
	case inst_OR_A_HL: cycles = OR_HL(data.a, data.h, data.l); return;
	case inst_OR_A_A: cycles = OR_R8(data.a, data.a); return;
	case inst_CP_A_B: cycles = CP_R8(data.a, data.b); return;
	case inst_CP_A_C: cycles = CP_R8(data.a, data.c); return;
	case inst_CP_A_D: cycles = CP_R8(data.a, data.d); return;
	case inst_CP_A_E: cycles = CP_R8(data.a, data.e); return;
	case inst_CP_A_H: cycles = CP_R8(data.a, data.h); return;
	case inst_CP_A_L: cycles = CP_R8(data.a, data.l); return;
	case inst_CP_A_HL: cycles = CP_HL(data.a, data.h, data.l); return;
	case inst_CP_A_A: cycles = CP_R8(data.a, data.a); return;

		//0xc0->0xcf
	case inst_RET_NZ: cycles = RET_CC(data.sp, !get_flag_state(flags_ZERO)); return;
	case inst_POP_BC: cycles = POP_R16(data.sp, data.b, data.c); return;
	case inst_JP_NZ_N16: cycles = JP_CC_N16(!get_flag_state(flags_ZERO)); return;
	case inst_JP_N16: cycles = JP_N16(); return;
	case inst_CALL_NZ_N16: cycles = CALL_CC_N16(data.sp, !get_flag_state(flags_ZERO)); return;
	case inst_PUSH_BC: cycles = PUSH_R16(data.sp, data.b, data.c); return;
	case inst_ADD_A_N8: cycles = ADD_N8(data.a); return;
	case inst_RST_00: cycles = RST_N8(data.sp, 0x00); return;
	case inst_RET_Z: cycles = RET_CC(data.sp, get_flag_state(flags_ZERO)); return;
	case inst_RET: cycles = RET(data.sp); return;
	case inst_JP_Z_N16: cycles = JP_CC_N16(get_flag_state(flags_ZERO)); return;
	case inst_CB: execute_cb_opcode(cycles); return;
	case inst_CALL_Z_N16: cycles = CALL_CC_N16(data.sp, get_flag_state(flags_ZERO)); return;
	case inst_CALL_N16: cycles = CALL_N16(data.sp); return;
	case inst_ADC_A_N8: cycles = ADC_N8(data.a); return;
	case inst_RST_08: cycles = RST_N8(data.sp, 0x08); return;

		//0xd0->0xdf
	case inst_RET_NC: cycles = RET_CC(data.sp, !get_flag_state(flags_CARRY)); return;
	case inst_POP_DE: cycles = POP_R16(data.sp, data.d, data.e); return;
	case inst_JP_NC_N16: cycles = JP_CC_N16(!get_flag_state(flags_CARRY)); return;
	case inst_CALL_NC_N16: cycles = CALL_CC_N16(data.sp, !get_flag_state(flags_CARRY)); return;
	case inst_PUSH_DE: cycles = PUSH_R16(data.sp, data.d, data.e); return;
	case inst_SUB_A_N8: cycles = SUB_N8(data.a); return;
	case inst_RST_10: cycles = RST_N8(data.sp, 0x10); return;
	case inst_RET_C: cycles = RET_CC(data.sp, get_flag_state(flags_CARRY)); return;
	case inst_RETI: cycles = RETI(data.sp); return;
	case inst_JP_C_N16: cycles = JP_CC_N16(get_flag_state(flags_CARRY)); return;
	case inst_CALL_C_N16: cycles = CALL_CC_N16(data.sp, get_flag_state(flags_CARRY)); return;
	case inst_SBC_A_N8: cycles = SBC_N8(data.a); return;
	case inst_RST_18: cycles = RST_N8(data.sp, 0x18); return;

		//0xe0->0xef
	case inst_LDH_N8_A: cycles = LDH_N8_A(data.a); return;
	case inst_POP_HL: cycles = POP_R16(data.sp, data.h, data.l); return;
	case inst_LDH_C_A: cycles = LDH_C_A(data.c, data.a); return;
	case inst_PUSH_HL: cycles = PUSH_R16(data.sp, data.h, data.l); return;
	case inst_AND_A_N8: cycles = AND_N8(data.a); return;
	case inst_RST_20: cycles = RST_N8(data.sp, 0x20); return;
	case inst_ADD_SP_E8: cycles = ADD_SP_E8(data.sp); return;
	case inst_JP_HL: cycles = JP_HL(data.h, data.l); return;
	case inst_LD_N16_A: cycles = LD_N16_A(data.a); return;
	case inst_XOR_A_N8: cycles = XOR_N8(data.a); return;
	case inst_RST_28: cycles = RST_N8(data.sp, 0x28); return;

		//0xf0->0xff
	case inst_LDH_A_N8: cycles = LDH_A_N8(data.a); return;
	case inst_POP_AF: cycles = POP_AF(data.sp, data.a, data.f); return;
	case inst_LDH_A_C: cycles = LDH_A_C(data.a, data.c); return;
	case inst_DI: cycles = DI(); return;
	case inst_PUSH_AF: cycles = PUSH_R16(data.sp, data.a, data.f); return;
	case inst_OR_A_N8: cycles = OR_N8(data.a); return;
	case inst_RST_30: cycles = RST_N8(data.sp, 0x30); return;
	case inst_LD_HL_SP_E8: cycles = LD_HL_SP_E8(data.h, data.l, data.sp); return;
	case inst_LD_SP_HL: cycles = LD_SP_HL(data.sp, data.h, data.l); return;
	case inst_LD_A_N16: cycles = LD_A_N16(data.a); return;
	case inst_EI: cycles = EI(); return;
	case inst_CP_A_N8: cycles = CP_N8(data.a); return;
	case inst_RST_38: cycles = RST_N8(data.sp, 0x38); return;
	
	}
}

void CPU::execute_cb_opcode(int& cycles) {
	byte opcode = fetch_next_byte();

	cycles = 0;
	switch (opcode) {
		//0x00->0x0f
	case inst_cb_RLC_B: cycles = RLC_R8(data.b); break;
	case inst_cb_RLC_C: cycles = RLC_R8(data.c); break;
	case inst_cb_RLC_D: cycles = RLC_R8(data.d); break;
	case inst_cb_RLC_E: cycles = RLC_R8(data.e); break;
	case inst_cb_RLC_H: cycles = RLC_R8(data.h); break;
	case inst_cb_RLC_L: cycles = RLC_R8(data.l); break;
	case inst_cb_RLC_HL: cycles = RLC_HL(data.h, data.l); break;
	case inst_cb_RLC_A: cycles = RLC_R8(data.a); break;
	case inst_cb_RRC_B: cycles = RRC_R8(data.b); break;
	case inst_cb_RRC_C: cycles = RRC_R8(data.c); break;
	case inst_cb_RRC_D: cycles = RRC_R8(data.d); break;
	case inst_cb_RRC_E: cycles = RRC_R8(data.e); break;
	case inst_cb_RRC_H: cycles = RRC_R8(data.h); break;
	case inst_cb_RRC_L: cycles = RRC_R8(data.l); break;
	case inst_cb_RRC_HL: cycles = RRC_HL(data.h, data.l); break;
	case inst_cb_RRC_A: cycles = RRC_R8(data.a); break;

		//0x10-0x1f
	case inst_cb_RL_B: cycles = RL_R8(data.b); break;
	case inst_cb_RL_C: cycles = RL_R8(data.c); break;
	case inst_cb_RL_D: cycles = RL_R8(data.d); break;
	case inst_cb_RL_E: cycles = RL_R8(data.e); break;
	case inst_cb_RL_H: cycles = RL_R8(data.h); break;
	case inst_cb_RL_L: cycles = RL_R8(data.l); break;
	case inst_cb_RL_HL: cycles = RL_HL(data.h, data.l); break;
	case inst_cb_RL_A: cycles = RL_R8(data.a); break;
	case inst_cb_RR_B: cycles = RR_R8(data.b); break;
	case inst_cb_RR_C: cycles = RR_R8(data.c); break;
	case inst_cb_RR_D: cycles = RR_R8(data.d); break;
	case inst_cb_RR_E: cycles = RR_R8(data.e); break;
	case inst_cb_RR_H: cycles = RR_R8(data.h); break;
	case inst_cb_RR_L: cycles = RR_R8(data.l); break;
	case inst_cb_RR_HL: cycles = RR_HL(data.h, data.l); break;
	case inst_cb_RR_A: cycles = RR_R8(data.a); break;

		//0x20->0x2f
	case inst_cb_SLA_B: cycles = SLA_R8(data.b); break;
	case inst_cb_SLA_C: cycles = SLA_R8(data.c); break;
	case inst_cb_SLA_D: cycles = SLA_R8(data.d); break;
	case inst_cb_SLA_E: cycles = SLA_R8(data.e); break;
	case inst_cb_SLA_H: cycles = SLA_R8(data.h); break;
	case inst_cb_SLA_L: cycles = SLA_R8(data.l); break;
	case inst_cb_SLA_HL: cycles = SLA_HL(data.h, data.l); break;
	case inst_cb_SLA_A: cycles = SLA_R8(data.a); break;
	case inst_cb_SRA_B: cycles = SRA_R8(data.b); break;
	case inst_cb_SRA_C: cycles = SRA_R8(data.c); break;
	case inst_cb_SRA_D: cycles = SRA_R8(data.d); break;
	case inst_cb_SRA_E: cycles = SRA_R8(data.e); break;
	case inst_cb_SRA_H: cycles = SRA_R8(data.h); break;
	case inst_cb_SRA_L: cycles = SRA_R8(data.l); break;
	case inst_cb_SRA_HL: cycles = SRA_HL(data.h, data.l); break;
	case inst_cb_SRA_A: cycles = SRA_R8(data.a); break;

		//0x30->0x3f
	case inst_cb_SWAP_B: cycles = SWAP_R8(data.b); break;
	case inst_cb_SWAP_C: cycles = SWAP_R8(data.c); break;
	case inst_cb_SWAP_D: cycles = SWAP_R8(data.d); break;
	case inst_cb_SWAP_E: cycles = SWAP_R8(data.e); break;
	case inst_cb_SWAP_H: cycles = SWAP_R8(data.h); break;
	case inst_cb_SWAP_L: cycles = SWAP_R8(data.l); break;
	case inst_cb_SWAP_HL: cycles = SWAP_HL(data.h, data.l); break;
	case inst_cb_SWAP_A: cycles = SWAP_R8(data.a); break;
	case inst_cb_SRL_B: cycles = SRL_R8(data.b); break;
	case inst_cb_SRL_C: cycles = SRL_R8(data.c); break;
	case inst_cb_SRL_D: cycles = SRL_R8(data.d); break;
	case inst_cb_SRL_E: cycles = SRL_R8(data.e); break;
	case inst_cb_SRL_H: cycles = SRL_R8(data.h); break;
	case inst_cb_SRL_L: cycles = SRL_R8(data.l); break;
	case inst_cb_SRL_HL: cycles = SRL_HL(data.h, data.l); break;
	case inst_cb_SRL_A: cycles = SRL_R8(data.a); break;

		//0x40->0x4f
	case inst_cb_BIT_0_B: cycles = BIT_B_R8(0, data.b); break;
	case inst_cb_BIT_0_C: cycles = BIT_B_R8(0, data.c); break;
	case inst_cb_BIT_0_D: cycles = BIT_B_R8(0, data.d); break;
	case inst_cb_BIT_0_E: cycles = BIT_B_R8(0, data.e); break;
	case inst_cb_BIT_0_H: cycles = BIT_B_R8(0, data.h); break;
	case inst_cb_BIT_0_L: cycles = BIT_B_R8(0, data.l); break;
	case inst_cb_BIT_0_HL: cycles = BIT_B_HL(0, data.h, data.l); break;
	case inst_cb_BIT_0_A: cycles = BIT_B_R8(0, data.a); break;
	case inst_cb_BIT_1_B: cycles = BIT_B_R8(1, data.b); break;
	case inst_cb_BIT_1_C: cycles = BIT_B_R8(1, data.c); break;
	case inst_cb_BIT_1_D: cycles = BIT_B_R8(1, data.d); break;
	case inst_cb_BIT_1_E: cycles = BIT_B_R8(1, data.e); break;
	case inst_cb_BIT_1_H: cycles = BIT_B_R8(1, data.h); break;
	case inst_cb_BIT_1_L: cycles = BIT_B_R8(1, data.l); break;
	case inst_cb_BIT_1_HL: cycles = BIT_B_HL(1, data.h, data.l); break;
	case inst_cb_BIT_1_A: cycles = BIT_B_R8(1, data.a); break;

		//0x50->0x5f
	case inst_cb_BIT_2_B: cycles = BIT_B_R8(2, data.b); break;
	case inst_cb_BIT_2_C: cycles = BIT_B_R8(2, data.c); break;
	case inst_cb_BIT_2_D: cycles = BIT_B_R8(2, data.d); break;
	case inst_cb_BIT_2_E: cycles = BIT_B_R8(2, data.e); break;
	case inst_cb_BIT_2_H: cycles = BIT_B_R8(2, data.h); break;
	case inst_cb_BIT_2_L: cycles = BIT_B_R8(2, data.l); break;
	case inst_cb_BIT_2_HL: cycles = BIT_B_HL(2, data.h, data.l); break;
	case inst_cb_BIT_2_A: cycles = BIT_B_R8(2, data.a); break;
	case inst_cb_BIT_3_B: cycles = BIT_B_R8(3, data.b); break;
	case inst_cb_BIT_3_C: cycles = BIT_B_R8(3, data.c); break;
	case inst_cb_BIT_3_D: cycles = BIT_B_R8(3, data.d); break;
	case inst_cb_BIT_3_E: cycles = BIT_B_R8(3, data.e); break;
	case inst_cb_BIT_3_H: cycles = BIT_B_R8(3, data.h); break;
	case inst_cb_BIT_3_L: cycles = BIT_B_R8(3, data.l); break;
	case inst_cb_BIT_3_HL: cycles = BIT_B_HL(3, data.h, data.l); break;
	case inst_cb_BIT_3_A: cycles = BIT_B_R8(3, data.a); break;

		//0x60->0x6f
	case inst_cb_BIT_4_B: cycles = BIT_B_R8(4, data.b); break;
	case inst_cb_BIT_4_C: cycles = BIT_B_R8(4, data.c); break;
	case inst_cb_BIT_4_D: cycles = BIT_B_R8(4, data.d); break;
	case inst_cb_BIT_4_E: cycles = BIT_B_R8(4, data.e); break;
	case inst_cb_BIT_4_H: cycles = BIT_B_R8(4, data.h); break;
	case inst_cb_BIT_4_L: cycles = BIT_B_R8(4, data.l); break;
	case inst_cb_BIT_4_HL: cycles = BIT_B_HL(4, data.h, data.l); break;
	case inst_cb_BIT_4_A: cycles = BIT_B_R8(4, data.a); break;
	case inst_cb_BIT_5_B: cycles = BIT_B_R8(5, data.b); break;
	case inst_cb_BIT_5_C: cycles = BIT_B_R8(5, data.c); break;
	case inst_cb_BIT_5_D: cycles = BIT_B_R8(5, data.d); break;
	case inst_cb_BIT_5_E: cycles = BIT_B_R8(5, data.e); break;
	case inst_cb_BIT_5_H: cycles = BIT_B_R8(5, data.h); break;
	case inst_cb_BIT_5_L: cycles = BIT_B_R8(5, data.l); break;
	case inst_cb_BIT_5_HL: cycles = BIT_B_HL(5, data.h, data.l); break;
	case inst_cb_BIT_5_A: cycles = BIT_B_R8(5, data.a); break;

		//0x70->0x7f
	case inst_cb_BIT_6_B: cycles = BIT_B_R8(6, data.b); break;
	case inst_cb_BIT_6_C: cycles = BIT_B_R8(6, data.c); break;
	case inst_cb_BIT_6_D: cycles = BIT_B_R8(6, data.d); break;
	case inst_cb_BIT_6_E: cycles = BIT_B_R8(6, data.e); break;
	case inst_cb_BIT_6_H: cycles = BIT_B_R8(6, data.h); break;
	case inst_cb_BIT_6_L: cycles = BIT_B_R8(6, data.l); break;
	case inst_cb_BIT_6_HL: cycles = BIT_B_HL(6, data.h, data.l); break;
	case inst_cb_BIT_6_A: cycles = BIT_B_R8(6, data.a); break;
	case inst_cb_BIT_7_B: cycles = BIT_B_R8(7, data.b); break;
	case inst_cb_BIT_7_C: cycles = BIT_B_R8(7, data.c); break;
	case inst_cb_BIT_7_D: cycles = BIT_B_R8(7, data.d); break;
	case inst_cb_BIT_7_E: cycles = BIT_B_R8(7, data.e); break;
	case inst_cb_BIT_7_H: cycles = BIT_B_R8(7, data.h); break;
	case inst_cb_BIT_7_L: cycles = BIT_B_R8(7, data.l); break;
	case inst_cb_BIT_7_HL: cycles = BIT_B_HL(7, data.h, data.l); break;
	case inst_cb_BIT_7_A: cycles = BIT_B_R8(7, data.a); break;

		//0x80->0x8f
	case inst_cb_RES_0_B: cycles = RES_B_R8(0, data.b); break;
	case inst_cb_RES_0_C: cycles = RES_B_R8(0, data.c); break;
	case inst_cb_RES_0_D: cycles = RES_B_R8(0, data.d); break;
	case inst_cb_RES_0_E: cycles = RES_B_R8(0, data.e); break;
	case inst_cb_RES_0_H: cycles = RES_B_R8(0, data.h); break;
	case inst_cb_RES_0_L: cycles = RES_B_R8(0, data.l); break;
	case inst_cb_RES_0_HL: cycles = RES_B_HL(0, data.h, data.l); break;
	case inst_cb_RES_0_A: cycles = RES_B_R8(0, data.a); break;
	case inst_cb_RES_1_B: cycles = RES_B_R8(1, data.b); break;
	case inst_cb_RES_1_C: cycles = RES_B_R8(1, data.c); break;
	case inst_cb_RES_1_D: cycles = RES_B_R8(1, data.d); break;
	case inst_cb_RES_1_E: cycles = RES_B_R8(1, data.e); break;
	case inst_cb_RES_1_H: cycles = RES_B_R8(1, data.h); break;
	case inst_cb_RES_1_L: cycles = RES_B_R8(1, data.l); break;
	case inst_cb_RES_1_HL: cycles = RES_B_HL(1, data.h, data.l); break;
	case inst_cb_RES_1_A: cycles = RES_B_R8(1, data.a); break;

		//0x90->0x9f
	case inst_cb_RES_2_B: cycles = RES_B_R8(2, data.b); break;
	case inst_cb_RES_2_C: cycles = RES_B_R8(2, data.c); break;
	case inst_cb_RES_2_D: cycles = RES_B_R8(2, data.d); break;
	case inst_cb_RES_2_E: cycles = RES_B_R8(2, data.e); break;
	case inst_cb_RES_2_H: cycles = RES_B_R8(2, data.h); break;
	case inst_cb_RES_2_L: cycles = RES_B_R8(2, data.l); break;
	case inst_cb_RES_2_HL: cycles = RES_B_HL(2, data.h, data.l); break;
	case inst_cb_RES_2_A: cycles = RES_B_R8(2, data.a); break;
	case inst_cb_RES_3_B: cycles = RES_B_R8(3, data.b); break;
	case inst_cb_RES_3_C: cycles = RES_B_R8(3, data.c); break;
	case inst_cb_RES_3_D: cycles = RES_B_R8(3, data.d); break;
	case inst_cb_RES_3_E: cycles = RES_B_R8(3, data.e); break;
	case inst_cb_RES_3_H: cycles = RES_B_R8(3, data.h); break;
	case inst_cb_RES_3_L: cycles = RES_B_R8(3, data.l); break;
	case inst_cb_RES_3_HL: cycles = RES_B_HL(3, data.h, data.l); break;
	case inst_cb_RES_3_A: cycles = RES_B_R8(3, data.a); break;

		//0xa0->0xaf
	case inst_cb_RES_4_B: cycles = RES_B_R8(4, data.b); break;
	case inst_cb_RES_4_C: cycles = RES_B_R8(4, data.c); break;
	case inst_cb_RES_4_D: cycles = RES_B_R8(4, data.d); break;
	case inst_cb_RES_4_E: cycles = RES_B_R8(4, data.e); break;
	case inst_cb_RES_4_H: cycles = RES_B_R8(4, data.h); break;
	case inst_cb_RES_4_L: cycles = RES_B_R8(4, data.l); break;
	case inst_cb_RES_4_HL: cycles = RES_B_HL(4, data.h, data.l); break;
	case inst_cb_RES_4_A: cycles = RES_B_R8(4, data.a); break;
	case inst_cb_RES_5_B: cycles = RES_B_R8(5, data.b); break;
	case inst_cb_RES_5_C: cycles = RES_B_R8(5, data.c); break;
	case inst_cb_RES_5_D: cycles = RES_B_R8(5, data.d); break;
	case inst_cb_RES_5_E: cycles = RES_B_R8(5, data.e); break;
	case inst_cb_RES_5_H: cycles = RES_B_R8(5, data.h); break;
	case inst_cb_RES_5_L: cycles = RES_B_R8(5, data.l); break;
	case inst_cb_RES_5_HL: cycles = RES_B_HL(5, data.h, data.l); break;
	case inst_cb_RES_5_A: cycles = RES_B_R8(5, data.a); break;

		//0xb0->0xbf
	case inst_cb_RES_6_B: cycles = RES_B_R8(6, data.b); break;
	case inst_cb_RES_6_C: cycles = RES_B_R8(6, data.c); break;
	case inst_cb_RES_6_D: cycles = RES_B_R8(6, data.d); break;
	case inst_cb_RES_6_E: cycles = RES_B_R8(6, data.e); break;
	case inst_cb_RES_6_H: cycles = RES_B_R8(6, data.h); break;
	case inst_cb_RES_6_L: cycles = RES_B_R8(6, data.l); break;
	case inst_cb_RES_6_HL: cycles = RES_B_HL(6, data.h, data.l); break;
	case inst_cb_RES_6_A: cycles = RES_B_R8(6, data.a); break;
	case inst_cb_RES_7_B: cycles = RES_B_R8(7, data.b); break;
	case inst_cb_RES_7_C: cycles = RES_B_R8(7, data.c); break;
	case inst_cb_RES_7_D: cycles = RES_B_R8(7, data.d); break;
	case inst_cb_RES_7_E: cycles = RES_B_R8(7, data.e); break;
	case inst_cb_RES_7_H: cycles = RES_B_R8(7, data.h); break;
	case inst_cb_RES_7_L: cycles = RES_B_R8(7, data.l); break;
	case inst_cb_RES_7_HL: cycles = RES_B_HL(7, data.h, data.l); break;
	case inst_cb_RES_7_A: cycles = RES_B_R8(7, data.a); break;

		//0xc0->0xcf
	case inst_cb_SET_0_B: cycles = SET_B_R8(0, data.b); break;
	case inst_cb_SET_0_C: cycles = SET_B_R8(0, data.c); break;
	case inst_cb_SET_0_D: cycles = SET_B_R8(0, data.d); break;
	case inst_cb_SET_0_E: cycles = SET_B_R8(0, data.e); break;
	case inst_cb_SET_0_H: cycles = SET_B_R8(0, data.h); break;
	case inst_cb_SET_0_L: cycles = SET_B_R8(0, data.l); break;
	case inst_cb_SET_0_HL: cycles = SET_B_HL(0, data.h, data.l); break;
	case inst_cb_SET_0_A: cycles = SET_B_R8(0, data.a); break;
	case inst_cb_SET_1_B: cycles = SET_B_R8(1, data.b); break;
	case inst_cb_SET_1_C: cycles = SET_B_R8(1, data.c); break;
	case inst_cb_SET_1_D: cycles = SET_B_R8(1, data.d); break;
	case inst_cb_SET_1_E: cycles = SET_B_R8(1, data.e); break;
	case inst_cb_SET_1_H: cycles = SET_B_R8(1, data.h); break;
	case inst_cb_SET_1_L: cycles = SET_B_R8(1, data.l); break;
	case inst_cb_SET_1_HL: cycles = SET_B_HL(1, data.h, data.l); break;
	case inst_cb_SET_1_A: cycles = SET_B_R8(1, data.a); break;

		//0xd0->0xdf
	case inst_cb_SET_2_B: cycles = SET_B_R8(2, data.b); break;
	case inst_cb_SET_2_C: cycles = SET_B_R8(2, data.c); break;
	case inst_cb_SET_2_D: cycles = SET_B_R8(2, data.d); break;
	case inst_cb_SET_2_E: cycles = SET_B_R8(2, data.e); break;
	case inst_cb_SET_2_H: cycles = SET_B_R8(2, data.h); break;
	case inst_cb_SET_2_L: cycles = SET_B_R8(2, data.l); break;
	case inst_cb_SET_2_HL: cycles = SET_B_HL(2, data.h, data.l); break;
	case inst_cb_SET_2_A: cycles = SET_B_R8(2, data.a); break;
	case inst_cb_SET_3_B: cycles = SET_B_R8(3, data.b); break;
	case inst_cb_SET_3_C: cycles = SET_B_R8(3, data.c); break;
	case inst_cb_SET_3_D: cycles = SET_B_R8(3, data.d); break;
	case inst_cb_SET_3_E: cycles = SET_B_R8(3, data.e); break;
	case inst_cb_SET_3_H: cycles = SET_B_R8(3, data.h); break;
	case inst_cb_SET_3_L: cycles = SET_B_R8(3, data.l); break;
	case inst_cb_SET_3_HL: cycles = SET_B_HL(3, data.h, data.l); break;
	case inst_cb_SET_3_A: cycles = SET_B_R8(3, data.a); break;

		//0xe0->0xef
	case inst_cb_SET_4_B: cycles = SET_B_R8(4, data.b); break;
	case inst_cb_SET_4_C: cycles = SET_B_R8(4, data.c); break;
	case inst_cb_SET_4_D: cycles = SET_B_R8(4, data.d); break;
	case inst_cb_SET_4_E: cycles = SET_B_R8(4, data.e); break;
	case inst_cb_SET_4_H: cycles = SET_B_R8(4, data.h); break;
	case inst_cb_SET_4_L: cycles = SET_B_R8(4, data.l); break;
	case inst_cb_SET_4_HL: cycles = SET_B_HL(4, data.h, data.l); break;
	case inst_cb_SET_4_A: cycles = SET_B_R8(4, data.a); break;
	case inst_cb_SET_5_B: cycles = SET_B_R8(5, data.b); break;
	case inst_cb_SET_5_C: cycles = SET_B_R8(5, data.c); break;
	case inst_cb_SET_5_D: cycles = SET_B_R8(5, data.d); break;
	case inst_cb_SET_5_E: cycles = SET_B_R8(5, data.e); break;
	case inst_cb_SET_5_H: cycles = SET_B_R8(5, data.h); break;
	case inst_cb_SET_5_L: cycles = SET_B_R8(5, data.l); break;
	case inst_cb_SET_5_HL: cycles = SET_B_HL(5, data.h, data.l); break;
	case inst_cb_SET_5_A: cycles = SET_B_R8(5, data.a); break;

		//0xf0->0xff
	case inst_cb_SET_6_B: cycles = SET_B_R8(6, data.b); break;
	case inst_cb_SET_6_C: cycles = SET_B_R8(6, data.c); break;
	case inst_cb_SET_6_D: cycles = SET_B_R8(6, data.d); break;
	case inst_cb_SET_6_E: cycles = SET_B_R8(6, data.e); break;
	case inst_cb_SET_6_H: cycles = SET_B_R8(6, data.h); break;
	case inst_cb_SET_6_L: cycles = SET_B_R8(6, data.l); break;
	case inst_cb_SET_6_HL: cycles = SET_B_HL(6, data.h, data.l); break;
	case inst_cb_SET_6_A: cycles = SET_B_R8(6, data.a); break;
	case inst_cb_SET_7_B: cycles = SET_B_R8(7, data.b); break;
	case inst_cb_SET_7_C: cycles = SET_B_R8(7, data.c); break;
	case inst_cb_SET_7_D: cycles = SET_B_R8(7, data.d); break;
	case inst_cb_SET_7_E: cycles = SET_B_R8(7, data.e); break;
	case inst_cb_SET_7_H: cycles = SET_B_R8(7, data.h); break;
	case inst_cb_SET_7_L: cycles = SET_B_R8(7, data.l); break;
	case inst_cb_SET_7_HL: cycles = SET_B_HL(7, data.h, data.l); break;
	case inst_cb_SET_7_A: cycles = SET_B_R8(7, data.a); break;

	}	
}

byte CPU::read_from_bus(const ushort& address) {
	byte opcode = emulator_ptr->bus_read(address);
	emulator_ptr->tick_other_components(2);
	emulator_ptr->tick_other_components(2);
	return opcode;
}

void CPU::write_to_bus(const ushort& address, const byte& value) {
	emulator_ptr->bus_write(address, value);
	emulator_ptr->tick_other_components(2);
	emulator_ptr->tick_other_components(2);
}

const bool CPU::get_flag_state(const cpu_flags& flag) {
	return (data.f & (1 << flag)) != 0;
}

void CPU::set_flag_state(const cpu_flags& flag, const bool& state) {
	if (state) {
		data.f |= (1 << flag);
	}
	else {
		data.f &= ~(1 << flag);
	}

	data.f &= 0xf0;
}
