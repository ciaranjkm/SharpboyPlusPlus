#include "Application.h"

Application::Application() {
	init_main_SDL_components(sdl_running, &window, &renderer, &texture);
}

Application::~Application() {
	//close emulator if its still open

	close_imgui();
	close_SDL(&window, &renderer, &texture);

	window = nullptr;
	renderer = nullptr;
}

//init
void Application::set_application_pointer(std::shared_ptr<Application> app) {
	self = app;
}

bool Application::is_app_running() {
	return sdl_running;
}

//instance control
void Application::create_new_emu_instance(const std::string& rom_file_name, const bool& using_boot_rom) {
	if (instance != nullptr || emu_running) {
		printf("[SB] Already an instance running, close this one first before starting a new one!\n");
		return;
	}

	instance = std::make_shared<Emulator>();
	instance->set_emu_pointer(instance);
	if (instance->initialise_emu_instance(rom_file_name, using_boot_rom) < 0) {
		return;
	}

	printf("[SB] Created new emulator instance successfully!\n");
	emu_running = true;
}

void Application::close_emu_instance() {
	if (instance == nullptr) {
		return;
	}

	instance->close_emulator();
	instance.reset();
	instance = nullptr;

	emu_running = false;

	printf("[SB] Closed emu instance successfully!\n");
}

//main loop
void Application::run() {
	const int GB_CPU_CLOCKSPEED = 4194304;
	auto last_time = std::chrono::high_resolution_clock::now();

	bool started_timing = false;

    while (sdl_running) {
		//poll events for sdl
        SDL_Event event;
        poll_SDL_events(&event, self);
        if (!sdl_running) break;

		//clear background and render ui
		clear_background(&renderer, 0, 0, 0, 255);
        if (!imgui_hidden) {
            draw_imgui(self);
        }

        if (emu_running) {
			if (!started_timing) {
				started_timing = true;
				last_time = std::chrono::high_resolution_clock::now();
			}

			auto current_time = std::chrono::high_resolution_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_time);

			int cycles_to_execute = (elapsed.count() * GB_CPU_CLOCKSPEED) / 1000000000;

            while (cycles_to_execute > 0) {
				int cycles = 0;
                cycles += instance->run_next_instruction();
				cycles_to_execute -= cycles;

				//update gb screen when a new frame is ready (on vblank)
				if (instance->draw_ready()) {
					update_gb_texture(&texture, &renderer, self);
					instance->reset_draw_ready();
				}

				last_time = current_time;
            }

			//draw our gb texture every frame to avoid tearing
			draw_gb_frame(&texture, &renderer);
        }
		else {
			started_timing = false;
		}

		//render imgui for the main window (load/save rom etc)
		if (!imgui_hidden) {
			render_imgui(&renderer);
		}

		//present renderers
		present_renderer(&renderer);

		//delay for 1ms to sleep cpu a bit
		std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

	close_emu_instance();

	close_imgui();
	close_SDL(&window, &renderer, &texture);
}

void Application::close() {
	sdl_running = false;
}

//imgui + sdl helper functions
void Application::toggle_imgui_shown() {
	imgui_hidden = !imgui_hidden;
}

const std::array<uint32_t, 160 * 144>& Application::get_frame_buffer() {
	return instance->get_frame_buffer();
}

const std::vector<std::string>& Application::get_rom_file_names() {
	return rom_file_names;
}

void Application::refresh_rom_file_names() {
	const char rom_path[] = "roms/";
	rom_file_names.clear();
	rom_file_names.push_back("=== ROMS ===");

	if (!std::filesystem::exists(rom_path)) {
		if (!std::filesystem::create_directory(rom_path)) {
			printf("[SHARPBOY]:: Unable to create roms directory, try creating one manually.\n");
			return;
		}

		printf("[SHARPBOY]:: ROM path does not exist, creating a folder in the root directory of SHARPBOY.\n");
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(rom_path)) {
		if (entry.is_regular_file() && entry.path().extension() == ".gb") {
			rom_file_names.push_back(entry.path().string());
		}
	}

	printf("[SHARPBOY]:: Success loading roms from %s.\n", rom_path);
}
