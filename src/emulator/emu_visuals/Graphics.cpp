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

void initialise_SDL_components(bool& sdl_running, SDL_Window** window, SDL_Renderer** renderer, SDL_Texture** texture) {
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
		close_SDL(window, renderer);

		printf("[SB] Failed SDL window initialisation, error: %s\n", SDL_GetError());
		sdl_running = false;
		return;
	}
	printf("[SB] SDL window initialised\n");

	*renderer = SDL_CreateRenderer(*window, NULL);
	if (*renderer == nullptr) {
		close_SDL(window, renderer);

		printf("[SB] Failed SDL renderer initialisation, error: %s\n", SDL_GetError());
		sdl_running = false;
		return;
	}
	// Enable VSync after creation
	if (!SDL_SetRenderVSync(*renderer, 2)) {
		printf("VSync enabled successfully\n");
	}
	else {
		printf("Failed to enable VSync: %s\n", SDL_GetError());
	}
	printf("[SB] SDL renderer initialised\n");

	*texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
	if (*texture == nullptr) {
		close_SDL(window, renderer);

		*texture = nullptr;
		printf("[SB] Failed SDL texture initialisation, error: %s\n", SDL_GetError());
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

			case SDLK_N:
				app->create_new_emu_instance("roms/PASSED/02-interrupts.gb", false);
				break;

			case SDLK_M:
				app->close_emu_instance();
				break;

			default: break;
			}
			break;

		default: break;
		}
	}
}

void clear_background(SDL_Renderer** renderer, const int& r, const int& g, const int& b, const int& a) {
	SDL_SetRenderDrawColor(*renderer, r, g, b, a);
	SDL_RenderClear(*renderer);
}

void present_renderer(SDL_Renderer** renderer) {
	SDL_RenderPresent(*renderer);
}

void draw_frame_buffer(const std::array<uint32_t, 160 * 144>& buffer, SDL_Texture** texture, SDL_Renderer** renderer) {
	SDL_RenderClear(*renderer);
	SDL_UpdateTexture(*texture, NULL, buffer.data(), 160 * sizeof(uint32_t));
	SDL_RenderTexture(*renderer, *texture, NULL, NULL);
}

void draw_imgui() {
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("sharpboy++");
	{

	}
	ImGui::End();

	ImGui::Render();
}

void render_imgui(SDL_Renderer** renderer) {
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), *renderer);
}

void close_imgui() {
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void close_SDL(SDL_Window** window, SDL_Renderer** renderer) {
	if (*window != nullptr) {
		SDL_DestroyWindow(*window);
		*window = nullptr;
		window = nullptr;
	}

	if (*renderer != nullptr) {
		SDL_DestroyRenderer(*renderer);
		*renderer = nullptr;
		renderer = nullptr;
	}

	SDL_Quit();
}