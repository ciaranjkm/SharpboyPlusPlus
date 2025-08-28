#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>

#include "../_definitions.h"
#include <memory>

class Emulator;

bool initialise_SDL_components(SDL_Window* window, SDL_Renderer* renderer);

void poll_SDL_events(SDL_Event* event, std::shared_ptr<Emulator> emulator_ptr);

void destroy_SDL_components(SDL_Window* window, SDL_Renderer* renderer);
