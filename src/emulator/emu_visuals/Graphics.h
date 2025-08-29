#pragma once

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <SDL3/SDL.h>

#include "../_definitions.h"
#include <memory>

class Application;

//initialisation
void initialise_ImGui_components(SDL_Window** window, SDL_Renderer** renderer);
void initialise_SDL_components(bool& sdl_running, SDL_Window** window, SDL_Renderer** renderer, SDL_Texture** texture);

//poll events
void poll_SDL_events(SDL_Event* event, std::shared_ptr<Application> app);

//sdl rendering
void clear_background(SDL_Renderer** renderer, const int& r, const int& g, const int& b, const int& a);
void present_renderer(SDL_Renderer** renderer);
void draw_frame_buffer(const std::array<uint32_t, 160 * 144>& buffer, SDL_Texture** texture, SDL_Renderer** renderer);

//imgui
void draw_imgui();
void render_imgui(SDL_Renderer** renderer);

//cleanup
void close_imgui();
void close_SDL(SDL_Window** window, SDL_Renderer** renderer);
