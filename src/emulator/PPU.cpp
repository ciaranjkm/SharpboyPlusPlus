#include "PPU.h"
#include "Emulator.h"

PPU::PPU(std::shared_ptr<Emulator> emulator_ptr) {
    gb_colors = std::array<uint32_t, 4>{
            0xffffffff,
            0xd3d3d3ff,
            0x222222ff,
            0x000000ff
    };
        
	this->emulator_ptr = emulator_ptr;
	if (this->emulator_ptr != nullptr) {
		initialised = true;
		current_mode = ppu_OAM_SEARCH;
	}
	else {
		initialised = false;
	}
}

PPU::~PPU() {
    this->emulator_ptr.reset();
    this->emulator_ptr = nullptr;

    printf("[SB] Shutting down PPU object\n");
}

void PPU::reset_ppu() {
    bool using_boot_rom = emulator_ptr->is_using_boot_rom();

    lcdc = 0x00;
    stat = 0x00;
    scy = 0x00;
    scx = 0x00;
    ly = 0x00;
    lyc = 0x00;
    bgp = 0x00;
    obp0 = 0x00;
    obp1 = 0x00;

    if (using_boot_rom) {
        return;
    }

    lcdc = 0x91;
    stat = 0x85;
    scy = 0x00;
    scx = 0x00;
    ly = 0x00;
    lyc = 0x00;
    bgp = 0xfc;
}

bool PPU::is_ppu_initialised() {
	return initialised;
}

void PPU::ppu_tick() {
    internal_cycles++;

    // Handle LCD enable/disable
    if (((lcdc & 0x80) != 0) && lcd_previously_off) {
        internal_cycles = 0;
        current_mode = ppu_OAM_SEARCH;
        lcd_previously_off = false;

        // Set initial STAT mode
        stat = (stat & 0xFC) | current_mode;
    }
    else if ((lcdc & 0x80) == 0) {
        if (!lcd_previously_off) {
            // LCD just turned off - reset state
            ly = 0;
            current_mode = ppu_HBLANK;
            stat = (stat & 0xFC) | current_mode;
            blocked_vram = true;
        }
        lcd_previously_off = true;
        return;
    }

    // Check LYC comparison (should happen every cycle LY changes)
    bool lyc_match = (ly == lyc);
    if (lyc_match) {
        stat |= 0x04;  // Set LYC flag
        if ((stat & 0x40) != 0) {  // LYC interrupt enabled
            emulator_ptr->trigger_interrupt(int_LCD);
        }
    }
    else {
        stat &= 0xFB;  // Clear LYC flag
    }

    switch (current_mode) {
    case ppu_OAM_SEARCH:
        if (internal_cycles == 80) {
            current_mode = ppu_DRAW_MODE;

            clear_fifo(bg_fifo_queue);
            onscreen_x = 0;
            bg_fifo_x = 0;
            primed_fifo = false;

            stat = (stat & 0xFC) | current_mode;
        }
        return;

    case ppu_DRAW_MODE:
        bg_fetcher_tick();
        output_bg_pixel();        

        if (onscreen_x >= SCREEN_WIDTH) {
            current_mode = ppu_HBLANK; 
        }

        return;

    case ppu_HBLANK:
        if (internal_cycles == 456) {  
            internal_cycles -= 456;
            ly++;

            if (ly == 144) {
                current_mode = ppu_VBLANK;

                stat = (stat & 0xFC) | current_mode;

                emulator_ptr->trigger_interrupt(int_VBLANK);

                if ((stat & 0x10) != 0) {
                    emulator_ptr->trigger_interrupt(int_LCD);
                }

                draw_ready = true;
            }
            else {
                current_mode = ppu_OAM_SEARCH;

                stat = (stat & 0xFC) | current_mode;

                if ((stat & 0x20) != 0) {
                    emulator_ptr->trigger_interrupt(int_LCD);
                }
            }
        }
        return;

    case ppu_VBLANK:
        if (internal_cycles >= 456) {  
            internal_cycles -= 456;
            ly++;

            if (ly == 154) { 
                ly = 0;

                current_mode = ppu_OAM_SEARCH;

                stat = (stat & 0xFC) | current_mode;

                if ((stat & 0x20) != 0) {
                    emulator_ptr->trigger_interrupt(int_LCD);
                }

                blocked_vram = false;
            }
        }
        return;
    }
}

byte PPU::read_ppu_io(const byte& ppu_io) {
    switch (ppu_io) {
    case io_LY: return ly;
    case io_LYC: return lyc;
    case io_STAT: return stat;
    case io_LCDC: return lcdc;
    case io_SCX: return scx;
    case io_SCY: return scy;
    case io_BGP: return bgp;
    case io_OBP0: return obp0;
    case io_OBP1: return obp1;
    case io_WX: return wx;
    case io_WY: return wy;
    default: return 0xff;
    }
}

void PPU::io_instant_write(const byte& ppu_io, const byte& value) {
    switch (ppu_io) {
    case io_LY: ly = value; return;
    case io_LYC: lyc = value; return;
    case io_STAT: stat = value; return;
    case io_LCDC: lcdc = value; return;
    case io_SCX: scx = value; return;
    case io_SCY: scy = value; return;
    case io_BGP: bgp = value; return;
    case io_OBP0: obp0 = value; return;
    case io_OBP1: obp1 = value; return;
    case io_WX: wx = value; return;
    case io_WY: wy = value; return;
    default: return;
    }
}

ppu_modes PPU::get_current_mode() {
	return current_mode;
}

bool PPU::is_draw_ready() {
    return draw_ready;
}

void PPU::reset_draw_ready() {
    draw_ready = false;
}

std::array<uint32_t, 160 * 144> PPU::get_bg_frame_buffer() {
    return background_pixel_buffer;
}

std::array<uint32_t, 64> PPU::get_next_tile(const int& index) {
    std::array<uint32_t, 64> tile = std::array<uint32_t, 64>();
    ushort tile_address = 0x8000 + (index * 16);

    int count = 0;
    for (int row = 0; row < 16; row += 2) {
        byte low = emulator_ptr->bus_read(tile_address + row);
        byte high = emulator_ptr->bus_read(tile_address + row + 1);

        for (int bit = 7; bit >= 0; bit--) {
            byte low_bit = (low >> bit) & 0x1;
            byte high_bit = (high >> bit) & 0x1;

            byte colour = (high_bit << 1) | low_bit;

            int palette_shift = colour * 2;
            int palette_colour = (bgp >> palette_shift) & 0x03;

            tile[count++] = gb_colors[palette_colour];
        }
    }

    return tile;
}

ushort PPU::get_tile_address_from_id(const byte& tile_id)  {
    ushort tile_address = 0x0000;
    ushort tile_data_base_address = 0x9000;
    if ((lcdc & 0x10) != 0) {
        tile_data_base_address = 0x8000;
        tile_address = (tile_data_base_address + current_pixel_id * 16);
    }
    else {
        sbyte signed_tile_number = (sbyte)current_pixel_id;
        tile_address = (tile_data_base_address + signed_tile_number * 16);
    }

    return tile_address;
}

byte PPU::read_vram(const ushort& address) {
    if (address >= 0x8000 && address < 0xa000) {
        if (blocked_vram) {
            return 0xff;
        }

        return emulator_ptr->bus_read(address);
    }
}

void PPU::bg_fetcher_tick() {
    fifo_ticks++;

    if (fifo_ticks < 2) {
        return; //do nothing, only tick every 2 t cycles
    }

    fifo_ticks = 0;
      
    switch (current_bg_fifo_state) {
    case fifo_FETCH_TILE_NUMBER:
        fetcher_get_tile_number();
        current_bg_fifo_state = fifo_FETCH_TILE_LOW;
        break;

    case fifo_FETCH_TILE_LOW:
        fetcher_get_tile_low();
        current_bg_fifo_state = fifo_FETCH_TILE_HIGH;
        break;

    case fifo_FETCH_TILE_HIGH:
        fetcher_get_tile_high();
        current_bg_fifo_state = fifo_PUSHING;
        break;

    case fifo_PUSHING:
        fetcher_push_row();
        current_bg_fifo_state = fifo_FETCH_TILE_NUMBER;
        break;

    case fifo_NONE:
        break;
    }
}

void PPU::fetcher_get_tile_number() {
    ushort tile_map_base = 0x9800;
    if ((lcdc & 0x8) != 0) {
        tile_map_base = 0x9c00;
    }

    tile_map_base += ((scx / 8) + bg_fifo_x) & 0x1f;
    tile_map_base += 32 * (((ly + scy) & 0xFF) / 8);

    current_pixel_id = read_vram((ushort)(tile_map_base));
}

void PPU::fetcher_get_tile_low() {
    ushort tile_data_address = get_tile_address_from_id(current_pixel_id);
    tile_data_address += (2 * ((ly + scy) % 8));

    current_pixel_low = read_vram(tile_data_address);
}

void PPU::fetcher_get_tile_high() {
    ushort tile_data_address = get_tile_address_from_id(current_pixel_id);
    tile_data_address += 2 * ((ly + scy) % 8);
    current_pixel_high = read_vram(tile_data_address + 1);
}

void PPU::fetcher_push_row() {
    if (bg_fifo_queue.size() < 8) {
        //loop from left to right
        for (int bit = 7; bit >= 0; bit--) {
            byte low_bit = (current_pixel_low >> bit) & 0x1;
            byte high_bit = (current_pixel_high >> bit) & 0x1;
            byte colour = (high_bit << 0x1) | low_bit;

            fifo_pixel new_pixel = {
                .colour = colour,
                .pallete = bgp,
                .priority = 0x00,
                .sprite = 0x00,
                .sprite_pallete = 0x00
            };

            push_pixel(bg_fifo_queue, new_pixel);
        }
        bg_fifo_x++;
    }
}

void PPU::push_pixel(std::queue<fifo_pixel>& fifo, const fifo_pixel& pixel) {
    if (fifo.size() < 16) {
        fifo.push(pixel);
    }
}

fifo_pixel PPU::pop_pixel(std::queue<fifo_pixel>& fifo) {
    if (!fifo.empty()) {
        fifo_pixel pixel = fifo.front();
        fifo.pop();
        return pixel;
    }
    
    return {};
}

void PPU::clear_fifo(std::queue<fifo_pixel>& fifo) {
    while (!fifo.empty()) {
        fifo.pop();
    }
}

void PPU::output_bg_pixel() {
    if (!bg_fifo_queue.empty()) {
        if (!primed_fifo) {
            if (bg_fifo_queue.size() >= 8) {
                for (int i = 0; i < (scx & 7); i++) {
                    bg_fifo_queue.pop();
                }
                primed_fifo = true;
                return;
            }
        }

        if (primed_fifo) {
            fifo_pixel pixel = pop_pixel(bg_fifo_queue);

            int palette_shift = pixel.colour * 2;
            int palette_colour = (bgp >> palette_shift) & 0x03;

            background_pixel_buffer[ly * SCREEN_WIDTH + onscreen_x] = gb_colors[palette_colour];

            onscreen_x++;
        }
    }
}