#include "Graphics.h"
#include "../Emulator.h"

void initialise_ImGui_components(SDL_Window** window, SDL_Renderer** renderer) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // optional
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // optional

	ImGui_ImplSDL3_InitForSDLRenderer(*window, *renderer);
	ImGui_ImplSDLRenderer3_Init(*renderer);
}

void initialise_SDL_components(bool& sdl_initialised, SDL_Window** window, SDL_Renderer** renderer) {
	sdl_initialised = false;
	printf("+----------------------------------------+\n");
	printf("[SB] Starting SDL initialisation...\n");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("[SB] Failed SDL initialisation, error: %s\n", SDL_GetError());
		sdl_initialised = false;
		return;
	}
	printf("[SB] SDL initialised\n");

	*window = SDL_CreateWindow("Sharpboy++", 720, 640, SDL_WINDOW_RESIZABLE);
	if (window == nullptr) {
		SDL_Quit();

		printf("[SB] Failed SDL window initialisation, error: %s\n", SDL_GetError());
		sdl_initialised = false;
		return;
	}
	printf("[SB] SDL window initialised\n");

	*renderer = SDL_CreateRenderer(*window, NULL);
	if (renderer == nullptr) {
		SDL_DestroyWindow(*window);
		window = nullptr;

		SDL_Quit();

		printf("[SB] Failed SDL renderer initialisation, error: %s\n", SDL_GetError());
		sdl_initialised = false;
		return;
	}
	printf("[SB] SDL renderer initialised\n");


	printf("[SB] SDL initialisation completed!\n");
	printf("+----------------------------------------+\n");

	initialise_ImGui_components(window, renderer);
	sdl_initialised = true;
}

//take emu pointer to run function on key presses etc, called from emu run
void poll_SDL_events(SDL_Event* event, std::shared_ptr<Emulator> emulator_ptr) {
	while (SDL_PollEvent(event)) {
		ImGui_ImplSDL3_ProcessEvent(event);

		switch (event->type) {

		case SDL_EVENT_QUIT: 
			emulator_ptr->close_emulator();
			break;

		case SDL_EVENT_KEY_DOWN:
			switch (event->key.key) {
			case SDLK_SPACE:
				emulator_ptr->toggle_show_imgui();
				break;

			default: break;
			}
			break;

		default: break;
		}
	}
}

void render_imgui() {
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("sharpboy++");
	{

	}
	ImGui::End();

	ImGui::Render();
}

void close_imgui() {
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void destroy_SDL_components(SDL_Window* window, SDL_Renderer* renderer) {
	SDL_DestroyWindow(window);
	window = nullptr;

	SDL_DestroyRenderer(renderer);
	renderer = nullptr;

	SDL_Quit();
}