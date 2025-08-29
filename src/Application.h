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

	//imgui + sdl
	void toggle_imgui_shown();

	//timing for emulator to run (todo eventually sync to audio emulation)
	void start_timers_for_new_instance();
	void calculate_elapsed_time();
	void delay_for_ms();

private:
	//private functions

private:
	std::shared_ptr<Application> self = nullptr;
	std::shared_ptr<Emulator> instance = nullptr;

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;

	bool initialised = false;
	bool sdl_running = false;
	bool emu_running = false;

	bool imgui_hidden = true;
};