#include "PPU.h"
#include "Emulator.h"

PPU::PPU(std::shared_ptr<Emulator> emulator_ptr, SDL_Renderer* renderer) {
    this->renderer = renderer;
    if (this->renderer != nullptr) {
        background_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
        gb_colors = std::array<uint32_t, 4>{
            0xffffffff,
            0xd3d3d3d3,
            0x222222ff,
            0x000000ff
        };
    }
    else {
        this->renderer = nullptr;
    }

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
	this->emulator_ptr = nullptr;
	this->renderer = nullptr;

	if (background_texture != nullptr) {
		SDL_DestroyTexture(background_texture);
		background_texture = nullptr;
	}

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
        if (internal_cycles >= 80) {
            internal_cycles -= 80;
            current_mode = ppu_DRAW_MODE;

            // Update STAT mode
            stat = (stat & 0xFC) | current_mode;

            // No STAT interrupt for entering draw mode
        }
        break;

    case ppu_DRAW_MODE:
        if (internal_cycles >= 172) {
            internal_cycles -= 172;
            current_mode = ppu_HBLANK;

            // Update STAT mode
            stat = (stat & 0xFC) | current_mode;

            // Trigger STAT interrupt if H-blank interrupt enabled
            if ((stat & 0x08) != 0) {
                emulator_ptr->trigger_interrupt(int_LCD);
            }

            // Render the current line
            if (ly < 144) {
               render_background_scanline();
            }
        }
        break;

    case ppu_HBLANK:
        if (internal_cycles >= 204) {  // FIXED: was checking >= 24
            internal_cycles -= 204;
            ly++;

            if (ly == 144) {
                // Entering V-blank
                current_mode = ppu_VBLANK;

                // Update STAT mode
                stat = (stat & 0xFC) | current_mode;

                // Trigger V-blank interrupt
                emulator_ptr->trigger_interrupt(int_VBLANK);

                // Trigger STAT interrupt if V-blank STAT interrupt enabled
                if ((stat & 0x10) != 0) {
                    emulator_ptr->trigger_interrupt(int_LCD);
                }

                // Render the complete frame
                render_texture();
            }
            else {
                // Going to next line - back to OAM search
                current_mode = ppu_OAM_SEARCH;

                // Update STAT mode
                stat = (stat & 0xFC) | current_mode;

                // Trigger STAT interrupt if OAM interrupt enabled
                if ((stat & 0x20) != 0) {
                    emulator_ptr->trigger_interrupt(int_LCD);
                }
            }
        }
        break;

    case ppu_VBLANK:
        if (internal_cycles >= 456) {  // Full scanline length
            internal_cycles -= 456;
            ly++;

            if (ly == 154) {  // After line 153, reset to 0
                ly = 0;
                current_mode = ppu_OAM_SEARCH;

                // Update STAT mode
                stat = (stat & 0xFC) | current_mode;

                // Trigger STAT interrupt if OAM interrupt enabled
                if ((stat & 0x20) != 0) {
                    emulator_ptr->trigger_interrupt(int_LCD);
                }
            }
        }
        break;
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

void PPU::write_ppu_io(const byte& ppu_io, const byte& value) {
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

bool PPU::is_draw_read() {
    return draw_ready;
}
void PPU::reset_draw_ready() {
    draw_ready = false;
}

void PPU::render_background_scanline() {
	//bg and window enable bit
	if ((lcdc & 0x01) == 0) {
		for (int i = 0; i < SCREEN_WIDTH; i++) {
			background_pixel_buffer[ly * SCREEN_WIDTH + i] = gb_colors[0]; //render white to scanline when bit is cleared
		}
		return;
	}

	for (int x = 0; x < SCREEN_WIDTH; x++) {
        int background_x = (scx + x) % 256;
		int background_y = (scy + ly) % 256;

		int tile_x = background_x / 8;
		int tile_y = background_y / 8;

		int tile_index = tile_y * 32 + tile_x;

		ushort tile_address = 0x0000;

		ushort tile_map_base_address = 0x9800;
		if ((lcdc & 0x08) != 0) {
			tile_map_base_address = 0x9c00;
		}
		byte tile_number = emulator_ptr->bus_read((ushort)(tile_map_base_address + tile_index));

		ushort tile_data_base_address = 0x9000;
		if ((lcdc & 0x10) != 0) {
			tile_data_base_address = 0x8000;
			tile_address = (tile_data_base_address + tile_number * 16);
		}
		else {
			sbyte signed_tile_number = (sbyte)tile_number;
			tile_address = (tile_data_base_address + signed_tile_number * 16);
		}

		int line_in_tile = background_y & 7;

		byte tile_low = emulator_ptr->bus_read((ushort)(tile_address + (line_in_tile * 2)));
		byte tile_high = emulator_ptr->bus_read((ushort)(tile_address + (line_in_tile * 2) + 1));

		int bit_index = 7 - (background_x % 8);
		int colour_bit = ((tile_high >> bit_index) & 0x01) << 1 | ((tile_low >> bit_index) & 0x1);

		byte bgp = emulator_ptr->bus_read((ushort)(0xff00 | io_BGP));
		int pallete_shift = colour_bit * 2;
		int pallete_colour = (bgp >> pallete_shift) & 0x03;

		background_pixel_buffer[ly * SCREEN_WIDTH + x] = gb_colors[pallete_colour];
	}
}

void PPU::render_texture() {
	SDL_RenderClear(renderer);
	SDL_UpdateTexture(background_texture, NULL, background_pixel_buffer.data(), SCREEN_WIDTH * sizeof(uint32_t));
	SDL_RenderTexture(renderer, background_texture, NULL, NULL);
    draw_ready = true;
}