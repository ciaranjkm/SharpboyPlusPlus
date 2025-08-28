#include "Visuals.h"
#include "../Emulator.h"

bool initialise_SDL_components(SDL_Window* window, SDL_Renderer* renderer) {
	bool initialised_SDL = false;

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("[SB] Failed SDL initialisation, error: %s\n", SDL_GetError());
		return initialised_SDL;
	}
	printf("[SB] SDL initialised\n");

	window = SDL_CreateWindow("Sharpboy++", 720, 640, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SURFACE_VSYNC_ADAPTIVE);
	if (window == nullptr) {
		SDL_Quit();

		printf("[SB] Failed SDL window initialisation, error: %s\n", SDL_GetError());
		return initialised_SDL;
	}
	printf("[SB] SDL window initialised\n");

	renderer = SDL_CreateRenderer(window, NULL);
	if (renderer == nullptr) {
		SDL_DestroyWindow(window);
		window = nullptr;

		SDL_Quit();

		printf("[SB] Failed SDL renderer initialisation, error: %s\n", SDL_GetError());
		return initialised_SDL;
	}
	printf("[SB] SDL renderer initialised\n");

	return !initialised_SDL;
}

void poll_SDL_events(SDL_Event* event, std::shared_ptr<Emulator> emulator_ptr) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_EVENT_QUIT) {
			emulator_ptr->close_emulator();
		}
	}
}

void destroy_SDL_components(SDL_Window* window, SDL_Renderer* renderer) {
	SDL_DestroyWindow(window);
	window = nullptr;

	SDL_DestroyRenderer(renderer);
	renderer = nullptr;

	SDL_Quit();
}