#include "Graphics.h"
#include "../../Application.h"

void initialise_ImGui_components(SDL_Window** window, SDL_Renderer** renderer) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // optional
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // optional

	ImGui_ImplSDL3_InitForSDLRenderer(*window, *renderer);
	ImGui_ImplSDLRenderer3_Init(*renderer);
}

void init_main_SDL_components(bool& sdl_running, SDL_Window** window, SDL_Renderer** renderer, SDL_Texture** texture, SDL_Texture** debug_tilemap_texture){
	sdl_running = false;
	printf("+----------------------------------------+\n");
	printf("[SB] Starting SDL initialisation...\n");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("[SB] Failed SDL initialisation, error: %s\n", SDL_GetError());
		sdl_running = false;
		return;
	}
	printf("[SB] SDL initialised\n");

	*window = SDL_CreateWindow("Sharpboy++", 720, 640, SDL_WINDOW_RESIZABLE);
	if (*window == nullptr) {
		close_SDL(window, renderer, texture, debug_tilemap_texture);

		printf("[SB] Failed SDL window initialisation, error: %s\n", SDL_GetError());
		sdl_running = false;
		return;
	}
	printf("[SB] SDL window initialised\n");

	*renderer = SDL_CreateRenderer(*window, NULL);
	if (*renderer == nullptr) {
		close_SDL(window, renderer, texture, debug_tilemap_texture);

		printf("[SB] Failed SDL renderer initialisation, error: %s\n", SDL_GetError());
		sdl_running = false;
		return;
	}

	*texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
	if (*texture == nullptr) {
		close_SDL(window, renderer, texture, debug_tilemap_texture);

		*texture = nullptr;
		printf("[SB] Failed SDL texture initialisation, error: %s\n", SDL_GetError());
		sdl_running = false;
		return;
	}

	*debug_tilemap_texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 128, 192);
	if (*debug_tilemap_texture == nullptr) {
		close_SDL(window, renderer, texture, debug_tilemap_texture);

		*debug_tilemap_texture = nullptr;
		printf("[SB] Failed SDL debug texture initialisation, error: %s\n", SDL_GetError());
		sdl_running = false;
		return;
	}


	printf("[SB] SDL initialisation completed!\n");
	printf("+----------------------------------------+\n");

	initialise_ImGui_components(window, renderer);
	sdl_running = true;
}

//take emu pointer to run function on key presses etc, called from emu run
void poll_SDL_events(SDL_Event* event, std::shared_ptr<Application> app) {
	while (SDL_PollEvent(event)) {
		ImGui_ImplSDL3_ProcessEvent(event);

		switch (event->type) {
		case SDL_EVENT_QUIT:
			app->close();
			break;

		case SDL_EVENT_KEY_DOWN:
			switch (event->key.key) {
			case SDLK_SPACE:
				app->toggle_imgui_shown();
				break;

			default: break;
			}
			break;

		default: break;
		}
	}
}


//sdl rendering
void clear_background(SDL_Renderer** renderer, const int& r, const int& g, const int& b, const int& a) {
	SDL_SetRenderDrawColor(*renderer, r, g, b, a);
	SDL_RenderClear(*renderer);
}

void present_renderer(SDL_Renderer** renderer) {
	SDL_RenderPresent(*renderer);
}

void update_gb_texture(SDL_Texture** texture, SDL_Renderer** renderer, std::shared_ptr<Application> app) {
	if (*texture == nullptr) {
		*texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
	}

	const std::array<uint32_t, 160 * 144>& buffer = app->get_frame_buffer();
	SDL_UpdateTexture(*texture, NULL, buffer.data(), 160 * sizeof(uint32_t));

}

void draw_gb_frame(SDL_Texture** texture, SDL_Renderer** renderer) {
	SDL_RenderTexture(*renderer, *texture, NULL, NULL);
}


//imgui drawing
void draw_load_rom_gui(std::shared_ptr<Application> app) {
	ImGui::SeparatorText("Load ROMs:");

	if (ImGui::Button("Load ROM")) {
		if (app->selected_rom_index != 0) {
			app->create_new_emu_instance(app->get_rom_file_names()[app->selected_rom_index], app->use_boot_rom_next_instance);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh ROMs")) {
		app->refresh_rom_file_names();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Use Boot ROM?", &app->use_boot_rom_next_instance);

	ImGui::PushItemWidth((ImGui::GetWindowWidth() - 15));
	if (ImGui::BeginListBox("##SelectRom")) {
		for (int n = 0; n < app->get_rom_file_names().size(); n++) {
			const bool is_selected = (app->selected_rom_index == n);
			if (ImGui::Selectable(app->get_rom_file_names()[n].data(), is_selected)) {
				app->selected_rom_index = n;
			}

			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndListBox();
	}
	ImGui::PopItemWidth();

}

void draw_current_rom_information_gui(std::shared_ptr<Application> app) {
	ImGui::SeparatorText("Current ROM Information");
	if (app->emu_initialised) {
		ImGui::Text("Title: %d", "tetris");
		ImGui::Text("ROM Size: %d", "32Kib");
		ImGui::Text("RAM Size: %d", "N/A");

		if (ImGui::Button("Start ROM")) {
			app->emu_running = true;
		}
		ImGui::SameLine(); 
		if (ImGui::Button("Pause ROM")) {
			app->emu_running = !app->emu_running;
		}
		ImGui::SameLine();
		if (ImGui::Button("Close ROM")) {
			app->close_emu_instance();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save ROM")) {
			printf("[SB] Saving not impl yet...\n");
		}

		ImGui::SeparatorText("Debug Options");
		ImGui::Checkbox("Basic Debug Information", &app->basic_debug_shown);
		ImGui::Checkbox("PPU Debug Information", &app->ppu_debug_shown);
	}
	else {
		ImGui::Text("Load a ROM to see this information!");
	}
}

void draw_ppu_tilemap(std::shared_ptr<Application> app, SDL_Texture** debug_tilemap_texture) {
	if (app->emu_running) {
		if (app->ppu_debug_shown) {
			for (int i = 0; i < 384; i++) {
				std::array<uint32_t, 64> tile{};
				tile = app->get_tile_map_data(i);
				int tx = (i % 16) * 8;
				int ty = (i / 16) * 8;

				SDL_Rect dst = { tx, ty, 8, 8 };
				SDL_UpdateTexture(*debug_tilemap_texture, &dst, tile.data(), 8 * sizeof(uint32_t));
			}

			ImGui::Begin("Sharpboy++ Debug | PPU Information");
			{
				ImTextureID texID = (ImTextureID)(*debug_tilemap_texture);
				ImGui::Image(texID, ImVec2(128 * 2, 192 * 2));
			}
			ImGui::End();
		}
	}
}

void draw_cpu_debugger(std::shared_ptr<Application> app) {
	if (app->emu_initialised) {
		if (app->basic_debug_shown) {
			cpu_data data = app->get_cpu_data();
			ImGui::Begin("Sharpboy++ Debug | CPU Information");
			{
				ImGui::SeparatorText("Registers");
				ImGui::Text("AF: 0x%02X%02X", data.a, data.f);
				ImGui::Text("BC: 0x%02X%02X", data.b, data.c);
				ImGui::Text("DE: 0x%02X%02X", data.d, data.e);
				ImGui::Text("HL: 0x%02X%02X", data.h, data.l);

				ImGui::Separator();
				ImGui::Text("PC: 0x%04X", data.pc);
				ImGui::Text("SP: 0x%04X", data.sp);

				ImGui::Separator();
				ImGui::Checkbox("Interrupt Master Enable", &data.ime);
				ImGui::Checkbox("Halted", &data.halted);
			}
			ImGui::End();
		}
	}
}

void draw_imgui(std::shared_ptr<Application> app, SDL_Texture** debug_tilemap_texture) {
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	//main emu controls
	ImGui::Begin("Sharpboy++");
	{
		draw_load_rom_gui(app);
		draw_current_rom_information_gui(app);
	}
	ImGui::End();

	//debug information
	draw_cpu_debugger(app);
	draw_ppu_tilemap(app, debug_tilemap_texture);

	ImGui::Render();
}

void render_imgui(SDL_Renderer** renderer) {
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), *renderer);
}


//cleanup methods
void close_imgui() {
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void close_SDL(SDL_Window** window, SDL_Renderer** renderer, SDL_Texture** texture, SDL_Texture** debug_tile_map_texture) {
	if (*window != nullptr) {
		SDL_DestroyWindow(*window);
		*window = nullptr;
	}

	if (*renderer != nullptr) {
		SDL_DestroyRenderer(*renderer);
		*renderer = nullptr;
	}

	if (*texture != nullptr) {
		SDL_DestroyTexture(*texture);
		*texture = nullptr;
	}

	if (*debug_tile_map_texture != nullptr) {
		SDL_DestroyTexture(*debug_tile_map_texture);
		*debug_tile_map_texture = nullptr;
	}

	SDL_Quit();
	printf("[SB] Quit SDL completed!\n");
}
