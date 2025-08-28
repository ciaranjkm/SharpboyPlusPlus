#pragma once

#include "_definitions.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <chrono>
#include <thread>
#include "CPU.h"
#include "MMU.h"
#include "Timers.h"
#include "PPU.h"

#include "emu_visuals/Graphics.h"

class Emulator {
public:
	Emulator();
	~Emulator();

	//instance setup
	void set_emu_pointer(std::shared_ptr<Emulator> emulator_ptr);
	bool is_using_boot_rom();
	void run_emulator();
	void close_emulator();

	//todo remove sst stuff
	bool get_single_step_mode();

	//graphics 
	void initialise_SDL();
	//gui funcs
	void toggle_show_imgui();

	//functions for other comps to tick
	void tick_other_components(const int& cycles);

	//interrupts
	void trigger_interrupt(const interrupt_types& interrupt);
	void clear_interrupt(const int& interrupt);

	//memory functions
	byte bus_read(const ushort& address);
	void bus_write(const ushort& address, const byte& value);
	byte read_timer_io(const byte& timer_io);
	void write_timer_io(const byte& timer_io, const byte& value);
	byte read_ppu_io(const byte& ppu_io);
	void write_ppu_io(const byte& ppu_io, const byte& value);
	byte io_instant_read(const io_addresses& io_target);
	void io_instant_write(const io_addresses& io_target, const byte& value);

	//ppu functions
	ppu_modes get_current_ppu_mode();

private:
	std::shared_ptr<Emulator> current_emulator_instance = nullptr;
	rom_header header;

	std::unique_ptr<CPU> CPU_ptr = nullptr;
	std::unique_ptr<MMU> MMU_ptr = nullptr;
	std::unique_ptr<Timers> TIMER_ptr = nullptr;
	std::unique_ptr<PPU> PPU_ptr = nullptr;
	//apu

	//sdl stuff
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	bool sdl_initialised = false;
	bool sdl_running = false;

	//control bools
	bool initialised = false;
	bool single_step_test_mode = false;
	bool emulator_running = false;
	bool using_boot_rom = false;

	bool show_imgui = true;

private:
	int initialise_emu_instance(const std::string& rom_file_name, const bool& using_boot_rom);
	void set_running(const bool& state);

	bool load_rom_file(const std::string& file_name, std::vector<byte>& rom_file);
	void parse_rom_file_header(rom_header& header, const std::vector<byte>& rom);

	bool load_boot_rom_file(const std::string& file_name, std::array<byte, 0x100>& boot_rom);
};