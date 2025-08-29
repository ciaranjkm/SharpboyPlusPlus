#pragma once

#include "_definitions.h"
#include <memory>
#include <SDL3/SDL.h>
#include <array>

class Emulator;

class PPU {
public:
	PPU(std::shared_ptr<Emulator> emulator_ptr);
	~PPU();

	void reset_ppu();
	bool is_ppu_initialised();
	void ppu_tick();

	byte read_ppu_io(const byte& ppu_io);
	void io_instant_write(const byte& ppu_io, const byte& value);
	ppu_modes get_current_mode();

	bool is_draw_ready();
	void reset_draw_ready();

	const std::array<uint32_t, 160 * 144>& get_bg_frame_buffer();

private:
	std::shared_ptr<Emulator> emulator_ptr = nullptr;
	bool initialised = false;

	byte ly = 0x00;
	byte stat = 0x00;
	byte lyc = 0x00;
	byte lcdc = 0x00;
	byte scx = 0x00;
	byte scy = 0x00;
	byte bgp = 0x00;
	byte obp0 = 0x00;
	byte obp1 = 0x00;
	byte wx = 0x00;
	byte wy = 0x00;

	bool lcd_previously_off = false;
	bool vblank_active = false;
	int internal_cycles = 0;

	int mode3_len = 172;
	ppu_modes current_mode = ppu_NONE;
	
	const int SCREEN_WIDTH = 160;
	const int SCREEN_HEIGHT = 144;

	bool render_line = false;
	bool render_frame = false;

	bool draw_ready = false;

	std::array<uint32_t, 160 * 144> background_pixel_buffer = std::array<uint32_t, 160 * 144>();
	std::array<uint32_t, 4> gb_colors = std::array<uint32_t, 4>();

	//sdl components, textures for rendering -> call emulator methods with textures as params to render

private:
	void draw_background_scanline();
	void draw_frame();
};