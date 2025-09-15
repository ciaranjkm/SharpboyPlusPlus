#pragma once

#include "defs.h"
#include <memory>
#include <SDL3/SDL.h>
#include <array>
#include <queue>
#include <vector>

class Emulator;

enum fifo_states {
	fifo_NONE,
	fifo_PUSHING,
	fifo_FETCH_TILE_HIGH,
	fifo_FETCH_TILE_LOW,
	fifo_FETCH_TILE_NUMBER
};

enum ppu_modes {
	ppu_NONE = 5,
	ppu_OAM_SEARCH = 2,
	ppu_DRAW_MODE = 3,
	ppu_HBLANK = 0,
	ppu_VBLANK = 1
};

struct fifo_pixel {
	byte colour = 0x00;
	byte pallete = 0x00;
	bool sprite = false;
	bool background_priority = false;
	int screen_x = 0;
};

struct fifo_sprite {
	byte y_pos = 0x00;
	byte x_pos = 0x00;
	byte tile_index = 0x00;
	byte flags = 0x00;
	
	bool background_priority() const { return (flags & 0x80) != 0; }
	bool y_flip() const { return (flags & 0x40) != 0; }
	bool x_flip() const { return (flags & 0x20) != 0; }
	bool use_pallet_one() const { return (flags & 0x10) != 0; }
};

struct sprite_dma_search_context {
	int ticks = 0;
	int index = 0;

};

struct sprite_fifo_context {
	bool fetching = false;
	fifo_states current_state = fifo_NONE;

	int ticks = 0;

	bool sprite_found = false;
	fifo_sprite current_sprite = fifo_sprite();

	byte current_sprite_low = 0x00;
	byte current_sprite_high = 0x00;
	ushort current_sprite_pixel_address;
};

struct background_fifo_context {
	fifo_states current_state = fifo_FETCH_TILE_NUMBER;
	int fifo_ticks = 0;
	bool paused = false;

	int current_x = 0;
	int on_screen_x = 0;

	int current_window_y = 0;
	bool window_tile = false;

	bool start_of_scanline = true;
	bool scx_discard = false;

	byte current_pixel_id = 0x00;
	byte current_pixel_low = 0x00;
	byte current_pixel_high = 0x00;
	ushort current_pixel_address = 0x0000;
};

struct ppu_io {
	byte ly = 0x00;
	byte stat = 0x00;
	byte lyc = 0x00;
	byte dma = 0x00;
	byte lcdc = 0x00;
	byte scx = 0x00;
	byte scy = 0x00;
	byte bgp = 0x00;
	byte obp0 = 0x00;
	byte obp1 = 0x00;
	byte wx = 0x00;
	byte wy = 0x00;
};

const int VRAM_SIZE = 0x2000;
const int OAM_SIZE = 0xa0;

struct ppu_memory {
	std::array<byte, VRAM_SIZE> vram = std::array<byte, VRAM_SIZE>();
	std::array<byte, OAM_SIZE> oam = std::array<byte, OAM_SIZE>();
};

struct dma_context {
	bool start_new_dma = false;
	bool dma_active = false;
	int dma_delay = 0;
	int dma_cycles = 0;
	int dma_ticks_this_cycle = 0;
	ushort dma_address = 0x0000;
};

const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 144;
const std::array<uint32_t, 4> m_pallete_colours = std::array<uint32_t, 4>{
			0xffffffff,
			0xd3d3d3ff,
			0x222222ff,
			0x000000ff
};

const int DEFAULT_DMA_DELAY = 8;
const int DMA_TOTAL_TICKS = 160;

const int SPRITE_BUFFER_SIZE = 10;
const int SPRITE_SEARCH_TICKS = 8;

class PPU {
public:
	PPU(std::shared_ptr<Emulator> emulator);
	~PPU();

	//init and tick methods
	void reset_ppu();
	bool is_ppu_initialised();
	void ppu_tick();
	void dma_tick();

	//read/write and memory methods
	byte read_ppu_io(const byte& ppu_io);
	void write_ppu_io(const byte& ppu_io, const byte& value);

	byte read_ppu_memory(const ushort& address);
	void write_ppu_memory(const ushort& address, const byte& value);

	//drawing methods
	bool is_draw_ready();
	void reset_draw_ready();

	const std::array<uint32_t, 160 * 144>& get_bg_frame_buffer() const;

	//debug methods for showing tilemaps etc todo fix this stuff and make it work more efficiently
	std::array<uint32_t, 64> get_next_tile(const int& index);

private:
	//reference to emulator for bus access
	std::shared_ptr<Emulator> emulator = nullptr;
	bool initialised = false;

	//general ppu variables and structs
	ppu_io m_ppu_io = ppu_io();
	ppu_memory m_ppu_memory = ppu_memory();
	dma_context m_dma = dma_context();
	ppu_modes m_current_mode = ppu_NONE;

	bool m_lcd_off = true;
	int m_internal_ticks = 0;

	bool m_trigger_lyc = false;

	bool wy_ly_this_frame = false;

	bool m_vram_blocked = false;
	bool m_oam_blocked = false;

	bool m_draw_ready = false;

	//fifo variables and structs
	background_fifo_context m_background_fifo = background_fifo_context();
	std::queue<fifo_pixel> m_bg_fifo_queue = std::queue<fifo_pixel>();
	bool start_of_scanline = false;
	bool primed_fifo = false;

	sprite_fifo_context m_sprite_fifo = sprite_fifo_context();
	std::queue<fifo_pixel> m_sprite_fifo_queue = std::queue<fifo_pixel>();
	sprite_dma_search_context m_sprite_search = sprite_dma_search_context();
	std::vector<fifo_sprite> m_sprite_buffer = std::vector<fifo_sprite>();
	 
	//frame buffers
	std::array<uint32_t, SCREEN_WIDTH* SCREEN_HEIGHT> m_bg_frame_buffer = std::array<uint32_t, SCREEN_WIDTH* SCREEN_HEIGHT>();

private:
	//general helper methods for ppu function
	void dma_write(const int& oam_index, const byte& value);
	void trigger_interrupt(const bool& is_vblank);
	
	void handle_lcd_off();
	void handle_lcd_on();
	void change_mode(ppu_modes new_mode);
	void end_of_scanline();
	void vblank_next_line();
	void update_lyc();

	//fifo function methods
	void tick_background_fetcher();
	void should_push_window();
	void fetcher_get_tile_number();
	void fetcher_get_tile_low();
	void fetcher_get_tile_high();
	void fetcher_push_row();

	//sprite fifo methods 
	void sprite_dma_search_tick();	
	void check_for_sprite_fetch();
	void tick_sprite_fetcher();
	void sprite_get_tile_low();
	void sprite_get_tile_high();
	void sprite_push_row();

	//fifo helper methods for either fifo
	void push_pixel(std::queue<fifo_pixel>& fifo, const fifo_pixel& pixel);
	fifo_pixel pop_pixel(std::queue<fifo_pixel>& fifo) ;
	void clear_fifo(std::queue<fifo_pixel>& fifo);

	ushort get_tile_address_from_id(const byte& tile_id);
	void reset_bg_fifo(background_fifo_context& fifo, std::queue<fifo_pixel>& fifo_queue);

	void reset_sprite_search();

	//todo modify to use both fifos and mix pixels here
	void mix_pixels();
};