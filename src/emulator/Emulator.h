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

class Emulator : public std::enable_shared_from_this<Emulator> {
public:
	//constructors
	Emulator();
	~Emulator();

	//instance setup
	int initialise_emu_instance(const std::string& rom_file_name, const bool& using_boot_rom);
	bool is_using_boot_rom();
	void close_emulator();

	//execution
	int run_next_instruction();

	//ticks for other components
	void tick_other_components(const int& cycles);

	//interrupts + input (todo optional input with optional sdl)
	void trigger_interrupt(const interrupt_types& interrupt);
	void clear_interrupt(const int& interrupt);
	void trigger_keypress(const joypad_buttons& button, const bool& key_down);
	byte read_joypad_state();

	//bus functions
	byte bus_read(const ushort& address);
	void bus_write(const ushort& address, const byte& value);
	byte io_instant_read(const byte& io_target);
	void io_instant_write(const byte& io_target, const byte& value);

	//ppu functions
	const std::array<uint32_t, 160 * 144>& get_frame_buffer();
	bool draw_ready();
	void reset_draw_ready();

	//get debug information
	cpu_data get_cpu_data();
	std::array<uint32_t, 64> get_next_tile(const int& index);
 
private:
	rom_header m_header = rom_header();

	std::unique_ptr<CPU> m_cpu = nullptr;
	std::unique_ptr<MMU> m_mmu = nullptr;
	std::unique_ptr<Timers> m_timers = nullptr;
	std::unique_ptr<PPU> m_ppu = nullptr;
	//apu

	//control bools
	bool m_initialised = false;
	bool m_using_boot_rom = false;

	//joypad state
	joypad_state m_joypad = joypad_state();

private:
	//todo move this into application class or file manager class maybe?
	bool load_rom_file(const std::string& file_name, std::vector<byte>& rom_file);
	void parse_rom_file_header(rom_header& header, const std::vector<byte>& rom);

	bool load_boot_rom_file(const std::string& file_name, std::array<byte, 0x100>& boot_rom);
};