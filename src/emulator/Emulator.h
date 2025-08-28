#pragma once

#include <iostream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <chrono>
#include <thread>
#include "_definitions.h"
#include "CPU.h"
#include "MMU.h"
#include "Timers.h"
#include "PPU.h"

#include "emu_visuals/Visuals.h"
#include "emu_visuals/DebugVisuals.h"

class Emulator {
public:
	//cli only const
	Emulator();
	~Emulator();

	void set_emu_pointer(std::shared_ptr<Emulator> emulator_ptr);
	const bool is_using_boot_rom();

	//emu control functions start/stop/run
	void run_emulator();
	void close_emulator();
	const bool get_single_step_mode();

	//init sdl 
	void initialise_SDL();

	void render_texture(SDL_Texture* texture);
	void clear_bg_renderer();

	//functions for other components to call
	void tick_other_components(const int& cycles);
	void trigger_interrupt(const interrupt_types& interrupt);
	void clear_interrupt(const int& interrupt);

	//memory functions
	byte bus_read(const ushort& address);
	void bus_write(const ushort& address, const byte& value);

	//io read write
	byte read_timer_io(const byte& timer_io);
	void write_timer_io(const byte& timer_io, const byte& value);

	byte read_ppu_io(const byte& ppu_io);
	void write_ppu_io(const byte& ppu_io, const byte& value);
	
	byte io_instant_read(const io_addresses& io_target);
	void io_instant_write(const io_addresses& io_target, const byte& value);

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
	bool running = false;
	bool using_boot_rom = false;

private:
	int initialise_emu_instance(const std::string& rom_file_name, const bool& using_boot_rom);
	void set_running(const bool& state);

	bool load_rom_file(const std::string& file_name, std::vector<byte>& rom_file);
	void parse_rom_file_header(rom_header& header, const std::vector<byte>& rom);

	bool load_boot_rom_file(const std::string& file_name, std::array<byte, 0x100>& boot_rom);
};