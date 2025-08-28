#pragma once

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <SDL3/SDL.h>

#include "../_definitions.h"
#include <memory>

class Emulator;

void initialise_ImGui_components(SDL_Window** window, SDL_Renderer** renderer);

void initialise_SDL_components(bool& sdl_initialised, SDL_Window** window, SDL_Renderer** renderer);

void poll_SDL_events(SDL_Event* event, std::shared_ptr<Emulator> emulator_ptr);

void render_imgui();

void close_imgui();

void destroy_SDL_components(SDL_Window* window, SDL_Renderer* renderer);
