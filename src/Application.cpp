#include "Application.h"

Application::Application() {
	initialise_SDL_components(sdl_running, &window, &renderer, &texture);
}

Application::~Application() {
	//close emulator if its still open

	close_imgui();
	close_SDL(&window, &renderer);

	window = nullptr;
	renderer = nullptr;
}

void Application::set_application_pointer(std::shared_ptr<Application> app) {
	self = app;
}

bool Application::is_app_running() {
	return sdl_running;
}

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
		printf("[SB] No instance currently running, open one to close it!\n");
		return;
	}

	instance.reset();
	instance = nullptr;

	emu_running = false;

	printf("[SB] Closed emu instance successfully!\n");
}

void Application::run() {
    // Create Game Boy render texture (do this once during initialization)
    SDL_Texture* gb_texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    if (!gb_texture) {
        printf("Failed to create Game Boy texture: %s\n", SDL_GetError());
        // Handle error
    }

    // Timing variables
    double cycle_accumulator = 0.0;
    double display_accumulator = 0.0;
    auto last_time = std::chrono::high_resolution_clock::now();
    bool new_gb_frame_available = false;
    const double GB_CPU_FREQ = 4194304.0;
    const double DISPLAY_FPS = 144.0; // Your display refresh rate
    const double DISPLAY_FRAME_TIME = 1.0 / DISPLAY_FPS;

    // Define where to draw the Game Boy screen (scale it up)
    SDL_FRect gb_display_rect = { 50, 50, 320, 288 }; // 2x scale, adjust as needed

    while (sdl_running) {
        auto current_time = std::chrono::high_resolution_clock::now();
        double delta_time = std::chrono::duration<double>(current_time - last_time).count();
        last_time = current_time;

        // Handle events
        SDL_Event event;
        poll_SDL_events(&event, self);
        if (!sdl_running) break;

        if (!imgui_hidden) {
            draw_imgui();
        }

        // === GAME BOY EMULATION (runs at GB speed) ===
        if (emu_running) {
            cycle_accumulator += delta_time * GB_CPU_FREQ;

            while (cycle_accumulator > 0) {
                int cycles = instance->run_next_instruction();
                cycle_accumulator -= cycles;

                if (instance->draw_ready()) {
                    new_gb_frame_available = true;
                    instance->reset_draw_ready();
                }
            }

            // Update Game Boy texture when we have a new frame
            if (new_gb_frame_available) {
                // Set render target to Game Boy texture
                SDL_SetRenderTarget(renderer, gb_texture);

                // Draw Game Boy frame buffer to texture
                draw_frame_buffer(instance->get_frame_buffer(), &gb_texture, &renderer);

                // Reset render target back to screen
                SDL_SetRenderTarget(renderer, NULL);

                new_gb_frame_available = false;
            }
        }

        // === DISPLAY RENDERING (runs at display refresh rate) ===
        display_accumulator += delta_time;
        if (display_accumulator >= DISPLAY_FRAME_TIME) {
            display_accumulator -= DISPLAY_FRAME_TIME;

            // Clear the screen
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            SDL_RenderClear(renderer);

            if (emu_running) {
                // Draw the Game Boy texture to screen (scaled up)
                SDL_RenderTexture(renderer, gb_texture, NULL, &gb_display_rect);
            }

            // Draw GUI on top
            if (!imgui_hidden) {
                render_imgui(&renderer);
            }

            // Present everything to screen
            SDL_RenderPresent(renderer);
        }

        std::this_thread::sleep_for(std::chrono::duration<double>(0.01));
    }

	close_imgui();
	close_SDL(&window, &renderer);
}

void Application::close() {
	sdl_running = false;
}

void Application::toggle_imgui_shown() {
	imgui_hidden = !imgui_hidden;
}