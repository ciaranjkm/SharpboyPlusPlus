#pragma once
#include <SDL3/SDL.h>
#include "emulator/Emulator.h"
#include "emulator/emu_visuals/Graphics.h"

#include <thread>
#include <chrono>

class Application {
public:
	Application();
	~Application();

	//init functions
	void set_application_pointer(std::shared_ptr<Application> app);
	bool is_app_running();
	
	//emu instance
	void create_new_emu_instance(const std::string& rom_file_name, const bool& using_boot_rom);
	void close_emu_instance();

	//run/close
	void run();
	void close();

	//imgui + sdl helpers
	void toggle_imgui_shown();
	const std::array<uint32_t, 160 * 144>& get_frame_buffer();
	const std::vector<std::string>& get_rom_file_names();
	void refresh_rom_file_names();

	//todo move this stuff to a static class which stores this stuff 
	//timing for emulator to run (todo eventually sync to audio emulation)
	void start_timers_for_new_instance();
	void calculate_elapsed_time();

public:
	bool imgui_hidden = true;
	int selected_rom_index = 0;

	bool emu_running = false;
	bool use_boot_rom_next_instance = false;

	bool basic_debug_shown = false;
	bool ppu_debug_shown = false;

private:
	std::shared_ptr<Application> self = nullptr;
	std::shared_ptr<Emulator> instance = nullptr;
	std::vector<std::string> rom_file_names = std::vector<std::string>{"=== ROMS ==="};

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr; 

	bool initialised = false;

	bool sdl_running = false;
};