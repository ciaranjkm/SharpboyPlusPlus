#pragma once

#include "_definitions.h"
#include <memory>
#include <SDL3/SDL.h>
#include <array>
#include <queue>

class Emulator;

struct fifo_pixel {
	byte colour = 0x00;
	byte pallete = 0x00;
	byte priority = 0x00;
	bool sprite = false;
	byte sprite_pallete = 0x00;
};

enum fifo_state {
	fifo_NONE,
	fifo_PUSHING,
	fifo_FETCH_TILE_HIGH,
	fifo_FETCH_TILE_LOW,
	fifo_FETCH_TILE_NUMBER
};

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

	std::array<uint32_t, 160 * 144> get_bg_frame_buffer();

	//debug methods for showing tilemaps etc 
	std::array<uint32_t, 64> get_next_tile(const int& index);

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

	ppu_modes current_mode = ppu_NONE;
	
	const int SCREEN_WIDTH = 160;
	const int SCREEN_HEIGHT = 144;

	bool render_line = false;
	bool render_frame = false;

	bool blocked_vram = false;

	bool draw_ready = false;

	std::array<uint32_t, 160 * 144> background_pixel_buffer = std::array<uint32_t, 160 * 144>();
	std::array<uint32_t, 4> gb_colors = std::array<uint32_t, 4>();

	std::queue<fifo_pixel> bg_fifo_queue = std::queue<fifo_pixel>();
	fifo_state current_bg_fifo_state = fifo_FETCH_TILE_NUMBER;
	
	int fifo_ticks = 0;
	int bg_fifo_x = 0;
	int bg_fifo_y = 0;
	int onscreen_x = 0;
	
	bool start_of_fifo_scanline = false;
	bool primed_fifo = false;

	byte current_pixel_id = 0x00;
	ushort current_pixel_address = 0x0000;
	byte current_pixel_low = 0x00;
	byte current_pixel_high = 0x00;

private:
	ushort get_tile_address_from_id(const byte& tile_id);
	byte read_vram(const ushort& address);

	void bg_fetcher_tick();
	void fetcher_get_tile_number();
	void fetcher_get_tile_low();
	void fetcher_get_tile_high();
	void fetcher_push_row();

	//fifo helper methods for either fifo
	void push_pixel(std::queue<fifo_pixel>& fifo, const fifo_pixel& pixel);
	fifo_pixel pop_pixel(std::queue<fifo_pixel>& fifo) ;
	void clear_fifo(std::queue<fifo_pixel>& fifo);

	//push pixel to the screen, todo modify to use either fifo
	void output_bg_pixel();

	//debug methods for showing tilemaps etc 
};