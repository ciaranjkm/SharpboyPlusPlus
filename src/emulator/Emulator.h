#pragma once

#include "_definitions.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <chrono>
#include <thread>
#include <array>
#include "CPU.h"
#include "MMU.h"
#include "Timers.h"
#include "PPU.h"

#include "emu_visuals/Graphics.h"

class Emulator {
public:
	//constructors
	Emulator();
	~Emulator();

	//instance setup
	int initialise_emu_instance(const std::string& rom_file_name, const bool& using_boot_rom);
	void set_emu_pointer(std::shared_ptr<Emulator> emulator_ptr);
	const bool& is_using_boot_rom() const;
	void close_emulator();

	//execution
	int run_next_instruction();

	//ticks for other components
	void tick_other_components(const int& cycles);

	//interrupts
	void trigger_interrupt(const interrupt_types& interrupt);
	void clear_interrupt(const int& interrupt);

	//memory/io read write
	byte bus_read(const ushort& address);
	void bus_write(const ushort& address, const byte& value);
	byte io_instant_read(const byte& io_target);
	void io_instant_write(const byte& io_target, const byte& value);

	//ppu functions
	ppu_modes get_current_ppu_mode();
	std::array<uint32_t, 160 * 144> get_frame_buffer();
	bool draw_ready();
	void reset_draw_ready();

	//get debug information
	cpu_data get_cpu_data();
	std::array<uint32_t, 64> get_next_tile(const int& index);
 
private:
	std::shared_ptr<Emulator> current_emulator_instance = nullptr;
	rom_header header;

	std::unique_ptr<CPU> CPU_ptr = nullptr;
	std::unique_ptr<MMU> MMU_ptr = nullptr;
	std::unique_ptr<Timers> TIMER_ptr = nullptr;
	std::unique_ptr<PPU> PPU_ptr = nullptr;
	//apu

	//control bools
	bool initialised = false;
	bool single_step_test_mode = false;
	bool using_boot_rom = false;

private:
	bool load_rom_file(const std::string& file_name, std::vector<byte>& rom_file);
	void parse_rom_file_header(rom_header& header, const std::vector<byte>& rom);

	bool load_boot_rom_file(const std::string& file_name, std::array<byte, 0x100>& boot_rom);
};