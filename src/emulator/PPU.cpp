#include "PPU.h"
#include "Emulator.h"

PPU::PPU(std::shared_ptr<Emulator> emulator) {
    if (emulator == nullptr) {
        //return if emu is null, don't init ppu bool
        return;
    }

    this->emulator = emulator;
    initialised = true;
}

PPU::~PPU() {
    this->emulator.reset();
    this->emulator = nullptr;

    printf("[SB] Shutting down PPU object\n");
}

//init and tick methods

void PPU::reset_ppu() {
    bool using_boot_rom = emulator->is_using_boot_rom();

    m_ppu_io.lcdc = 0x00;
    m_ppu_io.stat = 0x00;
    m_ppu_io.scy = 0x00;
    m_ppu_io.scx = 0x00;
    m_ppu_io.ly = 0x00;
    m_ppu_io.lyc = 0x00;
    m_ppu_io.bgp = 0x00;
    m_ppu_io.obp0 = 0x00;
    m_ppu_io.obp1 = 0x00;

    if (using_boot_rom) {
        return;
    }

    m_ppu_io.lcdc = 0x91;
    m_ppu_io.stat = 0x85;
    m_ppu_io.scy = 0x00;
    m_ppu_io.scx = 0x00;
    m_ppu_io.ly = 0x00;
    m_ppu_io.lyc = 0x00;
    m_ppu_io.bgp = 0xfc;
}

bool PPU::is_ppu_initialised() {
	return initialised;
}

void PPU::ppu_tick() {
    m_internal_ticks++;
    
    bool lcd_enabled = (m_ppu_io.lcdc & 0x80) != 0;
    if (!lcd_enabled) {
        handle_lcd_off();
        return;
    }
    else if (m_lcd_off) {
        handle_lcd_on();
    }
    
    switch (m_current_mode) {
    case ppu_OAM_SEARCH:
        sprite_dma_search_tick();

        if (m_internal_ticks == 80) {
            change_mode(ppu_DRAW_MODE);

            reset_bg_fifo(m_background_fifo, m_bg_fifo_queue);
        }
        break;

    case ppu_DRAW_MODE:
        should_push_window();
        tick_background_fetcher();

        if (!m_sprite_fifo.sprite_found) {
            check_for_sprite_fetch();
        }

        if (m_sprite_fifo.sprite_found) {
            tick_sprite_fetcher();
        }

        mix_pixels();

        if (m_background_fifo.on_screen_x >= SCREEN_WIDTH) {
            change_mode(ppu_HBLANK);
        }
        break;

    case ppu_HBLANK:
        if (m_internal_ticks >= 456) {
            end_of_scanline();
        }
        break;

    case ppu_VBLANK:
        if (m_internal_ticks >= 456) {
            vblank_next_line();
        }
        break;
    }

    update_lyc();
}

void PPU::dma_tick() {
    if (m_dma.start_new_dma) {
        //tick down if we're starting a new dma
        m_dma.dma_delay--;

        //if 1 m cycle has passed
        if (m_dma.dma_delay == 0) {
            m_dma.dma_active = true;
            m_dma.start_new_dma = false;

            m_dma.dma_address = (ushort)(m_ppu_io.dma << 8);

            m_dma.dma_cycles = 0;
            m_dma.dma_ticks_this_cycle = 0;
            m_dma.dma_delay = 0;

            return;
        }
    }


    if (m_dma.dma_active) {
        m_dma.dma_ticks_this_cycle++;

        //if one m cycle has passed
        if (m_dma.dma_ticks_this_cycle == 4) {
            m_dma.dma_ticks_this_cycle = 0;

            m_dma.dma_cycles++;

            byte dma_value = emulator->bus_read(m_dma.dma_address);
            m_dma.dma_address++;
            
            int dma_index = (m_dma.dma_cycles - 1);
            dma_write(dma_index, dma_value);
        }

        //if our total dma cycles is the total amount for a cycle, turn off dma and reset it
        if (m_dma.dma_cycles == DMA_TOTAL_TICKS) {
            m_dma.dma_active = false;
            m_dma.dma_address = 0x0000;
            m_dma.dma_cycles = 0;
            m_dma.dma_ticks_this_cycle = 0;
            m_dma.dma_delay = 0;
        }
    }
}

//read/write and memory methods

byte PPU::read_ppu_io(const byte& ppu_io) {
    switch (ppu_io) {
    case io_LY: return m_ppu_io.ly;
    case io_LYC: return m_ppu_io.lyc;
    case io_DMA: return 0xff;
    case io_STAT: return m_ppu_io.stat;
    case io_LCDC: return m_ppu_io.lcdc;
    case io_SCX: return m_ppu_io.scx;
    case io_SCY: return m_ppu_io.scy;
    case io_BGP: return m_ppu_io.bgp;
    case io_OBP0: return m_ppu_io.obp0;
    case io_OBP1: return m_ppu_io.obp1;
    case io_WX: return m_ppu_io.wx;
    case io_WY: return m_ppu_io.wy;
    default: return 0xff;
    }
}

void PPU::write_ppu_io(const byte& ppu_io, const byte& value) {
    switch (ppu_io) {
    case io_LY: m_ppu_io.ly = value; return;
    case io_LYC: m_ppu_io.lyc = value; return;
    case io_STAT: m_ppu_io.stat = value; return;
    case io_LCDC: m_ppu_io.lcdc = value; return;
    case io_SCX: m_ppu_io.scx = value; return;
    case io_SCY: m_ppu_io.scy = value; return;
    case io_BGP: m_ppu_io.bgp = value; return;
    case io_OBP0: m_ppu_io.obp0 = value; return;
    case io_OBP1: m_ppu_io.obp1 = value; return;
    case io_WX: m_ppu_io.wx = value; return;
    case io_WY: m_ppu_io.wy = value; return;

    case io_DMA:
        m_ppu_io.dma = value;
        m_dma.start_new_dma = true;
        m_dma.dma_delay = DEFAULT_DMA_DELAY;
        return;

    default: return;
    }
}

byte PPU::read_ppu_memory(const ushort& address) {
    if (address >= 0x8000 && address < 0xa000) {
        return m_ppu_memory.vram[(ushort)(address - 0x8000)];
    }
    else if (address >= 0xfe00 && address < 0xfea0) {
        //if (m_dma.dma_active) {
            //return 0xff;
        //}

        return m_ppu_memory.oam[(ushort)(address - 0xfe00)];
    }
    
    return 0xff;
}

void PPU::write_ppu_memory(const ushort& address, const byte& value) {
    if (address >= 0x8000 && address < 0xa000) {
        if (!m_vram_blocked) {
            m_ppu_memory.vram[(ushort)(address - 0x8000)] = value;
        }
    }
    else if (address >= 0xfe00 && address < 0xfea0) {
        if (!m_oam_blocked) {
            m_ppu_memory.oam[(ushort)(address - 0xfe00)] = value;
        }
    }
}

void PPU::dma_write(const int& oam_index, const byte& value) {
    m_ppu_memory.oam[oam_index] = value;
}

//drawing methods

bool PPU::is_draw_ready() {
    return m_draw_ready;
}

void PPU::reset_draw_ready() {
    m_draw_ready = false;
}

const std::array<uint32_t, 160 * 144>& PPU::get_bg_frame_buffer() const {
    return m_bg_frame_buffer;
}

//debug methods todo

std::array<uint32_t, 64> PPU::get_next_tile(const int& index) {
    std::array<uint32_t, 64> tile = std::array<uint32_t, 64>();
    ushort tile_address = 0x8000 + (index * 16);

    int count = 0;
    for (int row = 0; row < 16; row += 2) {
        byte low = emulator->bus_read(tile_address + row);
        byte high = emulator->bus_read(tile_address + row + 1);

        for (int bit = 7; bit >= 0; bit--) {
            byte low_bit = (low >> bit) & 0x1;
            byte high_bit = (high >> bit) & 0x1;

            byte colour = (high_bit << 1) | low_bit;

            int palette_shift = colour * 2;
            int palette_colour = (m_ppu_io.bgp >> palette_shift) & 0x03;

            tile[count++] = m_pallete_colours[palette_colour];
        }
    }

    return tile;
}

//PRIVATES
//general helper methods for ppu function
void PPU::trigger_interrupt(const bool& is_vblank) {
    if (is_vblank) {
		emulator->trigger_interrupt(int_VBLANK);
        return;
    }

	emulator->trigger_interrupt(int_LCD);
}

void PPU::handle_lcd_off() {
    m_ppu_io.ly = 0;
    m_internal_ticks = 0;

    change_mode(ppu_HBLANK);
    m_vram_blocked = false;
    m_lcd_off = true;
}

void PPU::handle_lcd_on() {
    m_lcd_off = false;
    m_ppu_io.ly = 0;
    m_internal_ticks = 0;
    change_mode(ppu_OAM_SEARCH);
}

void PPU::change_mode(ppu_modes new_mode) {
    if (new_mode == m_current_mode) {
        return;
    }

    m_current_mode = new_mode;
    m_ppu_io.stat = (m_ppu_io.stat & 0xfc) | new_mode;

    switch (new_mode) {
    case ppu_OAM_SEARCH:
        reset_sprite_search();

        m_oam_blocked = true;
        if (m_ppu_io.stat & 0x20) {
            trigger_interrupt(false);
        }
        break;

    case ppu_VBLANK:
        m_vram_blocked = false;
        m_oam_blocked = false;
        if (m_ppu_io.stat & 0x10) {
            trigger_interrupt(false);
        }
        break;

    case ppu_HBLANK:
		m_vram_blocked = false;
        m_oam_blocked = false;
        
        //todo
        //scx mod 8 should affect the timing of when ly is incremented
        //0 -> 51 ticks after stat interrupt (this function call)
        //1-4 -> 50 ticks after stat interrupt
        //5-7 -> 49 ticks after interrupt

        if (m_ppu_io.stat & 0x08) {
            trigger_interrupt(false);
        }
        break;

    case ppu_DRAW_MODE:
        m_vram_blocked = true;
        break;
    }
}

void PPU::end_of_scanline() {
    m_internal_ticks = 0;
    m_ppu_io.ly++;

    if (m_ppu_io.ly == 144) {
        change_mode(ppu_VBLANK);
        trigger_interrupt(true);
        m_draw_ready = true;
    }
    else {
        change_mode(ppu_OAM_SEARCH);
        m_background_fifo.start_of_scanline = true;
    }

    if (m_background_fifo.window_tile) {
        m_background_fifo.current_window_y++;
    }
    m_background_fifo.window_tile = false;

    clear_fifo(m_bg_fifo_queue);
    clear_fifo(m_sprite_fifo_queue);
}

void PPU::vblank_next_line() {
    m_internal_ticks = 0;
    m_ppu_io.ly++;

    if (m_ppu_io.ly == 154) {
        m_ppu_io.ly = 0;
        change_mode(ppu_OAM_SEARCH);
        m_background_fifo.current_window_y = 0;
    }
}

void PPU::update_lyc() {
    bool previous = (m_ppu_io.stat & 0x04);
    bool next = (m_ppu_io.ly == m_ppu_io.lyc);

    if (next) {
        m_ppu_io.stat |= 0x04;
    }
    else {
        m_ppu_io.stat &= ~0x04;
    }

    if (!previous && next && (m_ppu_io.stat & 0x40)) {
        trigger_interrupt(false);
    }
}

//fifo function methods

void PPU::tick_background_fetcher() {
    if (m_background_fifo.paused) {
        return;
    }

    m_background_fifo.fifo_ticks++;

    if (m_background_fifo.fifo_ticks != 2) {
        return; //do nothing, only tick every 2 t cycles
    }

    m_background_fifo.fifo_ticks = 0;
      
    switch (m_background_fifo.current_state) {
    case fifo_FETCH_TILE_NUMBER:
        fetcher_get_tile_number();
        m_background_fifo.current_state = fifo_FETCH_TILE_LOW;
        break;

    case fifo_FETCH_TILE_LOW:
        fetcher_get_tile_low();
        m_background_fifo.current_state = fifo_FETCH_TILE_HIGH;
        break;

    case fifo_FETCH_TILE_HIGH:
        fetcher_get_tile_high();
        if (m_background_fifo.start_of_scanline) {
            m_background_fifo.current_state = fifo_FETCH_TILE_NUMBER;
            m_background_fifo.start_of_scanline = false;
            break;
        }
        m_background_fifo.current_state = fifo_PUSHING;
        break;

    case fifo_PUSHING:
        fetcher_push_row();
        m_background_fifo.current_state = fifo_FETCH_TILE_NUMBER;
        break;

    case fifo_NONE:
        break;
    }
}

void PPU::should_push_window() {
    if (!m_background_fifo.window_tile) {
        bool window_enabled = (m_ppu_io.lcdc & 0x20) != 0;
        bool window_y_condition = m_ppu_io.ly >= m_ppu_io.wy;
        bool window_x_condition = m_background_fifo.on_screen_x >= (m_ppu_io.wx - 7);

        if (window_enabled && window_y_condition && window_x_condition) {
            clear_fifo(m_bg_fifo_queue);
            m_background_fifo.fifo_ticks = 0;
            m_background_fifo.current_state = fifo_FETCH_TILE_NUMBER;
            m_background_fifo.window_tile = true;
            m_background_fifo.current_x = 0;
        }
    }
}

void PPU::fetcher_get_tile_number() {
    ushort tile_map_base = 0x9800;
    if (!m_background_fifo.window_tile) {
        if ((m_ppu_io.lcdc & 0x08) != 0) {
            tile_map_base = 0x9c00;
        }

        //fetch offset for background tile map
        tile_map_base += ((m_ppu_io.scx / 8) + m_background_fifo.current_x) & 0x1f;
        tile_map_base += 32 * (((m_ppu_io.ly + m_ppu_io.scy) & 0xFF) / 8);

        m_background_fifo.current_pixel_id = read_ppu_memory(tile_map_base);
        return;
    }

    if ((m_ppu_io.lcdc & 0x40) != 0) {
        tile_map_base = 0x9c00;
    }

    tile_map_base += m_background_fifo.current_x;
    tile_map_base += 32 * (m_background_fifo.current_window_y / 8);

    m_background_fifo.current_pixel_id = read_ppu_memory(tile_map_base);
    return;
}

void PPU::fetcher_get_tile_low() {
    ushort tile_data_address = get_tile_address_from_id(m_background_fifo.current_pixel_id);

    if (!m_background_fifo.window_tile) {
        tile_data_address += (2 * ((m_ppu_io.ly + m_ppu_io.scy) % 8));
        m_background_fifo.current_pixel_low = read_ppu_memory(tile_data_address);
        return;
    }

    tile_data_address += (2 * (m_background_fifo.current_window_y % 8));
    m_background_fifo.current_pixel_low = read_ppu_memory(tile_data_address);
    return;
}

void PPU::fetcher_get_tile_high() {
    ushort tile_data_address = get_tile_address_from_id(m_background_fifo.current_pixel_id);
    if (!m_background_fifo.window_tile) {
        tile_data_address += (2 * ((m_ppu_io.ly + m_ppu_io.scy) % 8));
        m_background_fifo.current_pixel_high = read_ppu_memory(tile_data_address + 1);
        return;
    }

    tile_data_address += (2 * (m_background_fifo.current_window_y % 8));
    m_background_fifo.current_pixel_high = read_ppu_memory(tile_data_address + 1);
    return;
}

void PPU::fetcher_push_row() {
    if (m_bg_fifo_queue.size() < 8) {
        //loop from left to right
        for (int bit = 7; bit >= 0; bit--) {
            byte low_bit = (m_background_fifo.current_pixel_low >> bit) & 0x1;
            byte high_bit = (m_background_fifo.current_pixel_high >> bit) & 0x1;
            byte colour = (high_bit << 0x1) | low_bit;

            fifo_pixel new_pixel = {
                .colour = colour,
                .pallete = m_ppu_io.bgp,
                .sprite = false,
                .background_priority = 0x00
            };

            push_pixel(m_bg_fifo_queue, new_pixel);
        }
        m_background_fifo.current_x++;
    }
}

void PPU::sprite_dma_search_tick() {
    m_sprite_search.ticks++;

    if (m_sprite_search.ticks >= 2) {
        m_sprite_search.ticks = 0;

        if (m_sprite_search.index >= 40) {
            return; // scanned all OAM
        }

        if (m_sprite_buffer.size() >= SPRITE_BUFFER_SIZE) {
            return; // buffer full (should be 10 max on real HW)
        }

        fifo_sprite next_sprite;
        int address = m_sprite_search.index * 4;

        next_sprite.y_pos = m_ppu_memory.oam[address++];
        next_sprite.x_pos = m_ppu_memory.oam[address++];
        next_sprite.tile_index = m_ppu_memory.oam[address++];
        next_sprite.flags = m_ppu_memory.oam[address];

        m_sprite_search.index++;

        int sprite_height = ((m_ppu_io.lcdc & 0x04) != 0) ? 16 : 8;

        // Correct coordinate translation
        int sprite_top = next_sprite.y_pos - 16;
        int sprite_bottom = sprite_top + sprite_height;

        bool in_y_range = (m_ppu_io.ly >= sprite_top && m_ppu_io.ly < sprite_bottom);
        bool visible_x = (next_sprite.x_pos > 0 && next_sprite.x_pos < 168);

        if (in_y_range && visible_x) {
            m_sprite_buffer.push_back(next_sprite);
        }
    }
}

void PPU::check_for_sprite_fetch() {
    if (m_sprite_buffer.size() == 0) {
        return;
    }

    for (int i = 0; i < m_sprite_buffer.size(); i++) {
        bool fetch = (m_sprite_buffer[i].x_pos == m_background_fifo.on_screen_x + 8);
        if (fetch) {
			m_sprite_fifo.current_sprite = m_sprite_buffer[i];
            m_sprite_fifo.current_state = fifo_FETCH_TILE_LOW;
            m_sprite_fifo.sprite_found = true;

			m_sprite_buffer.erase(m_sprite_buffer.begin() + i);

            m_background_fifo.paused = true;
            m_background_fifo.fifo_ticks = 0;
            m_background_fifo.current_state = fifo_FETCH_TILE_NUMBER;
            return;
        }
    }

    return;
}

void PPU::tick_sprite_fetcher() {
    m_sprite_fifo.ticks++;

    if (m_sprite_fifo.ticks < 2) {
        return;
    }

    m_sprite_fifo.ticks = 0;

    switch (m_sprite_fifo.current_state) {
    case fifo_FETCH_TILE_LOW:
        sprite_get_tile_low();
        m_sprite_fifo.current_state = fifo_FETCH_TILE_HIGH;
        break;

    case fifo_FETCH_TILE_HIGH:
        sprite_get_tile_high();
        m_sprite_fifo.current_state = fifo_PUSHING;
        break;

    case fifo_PUSHING:
        sprite_push_row();
        m_sprite_fifo.current_state = fifo_NONE;
        break;

    case fifo_NONE:
        return;
    }
}

void PPU::sprite_get_tile_low() {
    //check sprite height and get the row in the sprite we need
    int sprite_height = ((m_ppu_io.lcdc & 0x04) != 0) ? 16 : 8;
    int sprite_row = (m_ppu_io.ly + 16) - m_sprite_fifo.current_sprite.y_pos;

    //are we flipped vertically?
    if (m_sprite_fifo.current_sprite.y_flip()) {
        sprite_row = (sprite_height - 1) - sprite_row;
    }

    int tile_index = m_sprite_fifo.current_sprite.tile_index;

    //get the tile index and sprite row for tall sprites
    if (sprite_height == 16) {
        tile_index &= 0xfe;  
        if (sprite_row >= 8) {
            tile_index += 1;     
            sprite_row -= 8;    
        }
    }

    //read sprite data from vram
    ushort base_sprite_address = 0x8000 + (tile_index * 16) + (sprite_row * 2);
    m_sprite_fifo.current_sprite_low = read_ppu_memory(base_sprite_address);
}

void PPU::sprite_get_tile_high() {
    //check sprite height and get the row in the sprite we need
    int sprite_height = ((m_ppu_io.lcdc & 0x04) != 0) ? 16 : 8;
    int sprite_row = (m_ppu_io.ly + 16) - m_sprite_fifo.current_sprite.y_pos;

    //are we flips vertically?
    if (m_sprite_fifo.current_sprite.y_flip()) {
        sprite_row = (sprite_height - 1) - sprite_row;
    }

    int tile_index = m_sprite_fifo.current_sprite.tile_index;

    //get the tile index and sprite row for tall sprites
    if (sprite_height == 16) {
        tile_index &= 0xfe;  
        if (sprite_row >= 8) {
            tile_index += 1;     
            sprite_row -= 8;  
        }
    }

    //read sprite data from vram
    ushort base_sprite_address = 0x8000 + (tile_index * 16) + (sprite_row * 2);
    m_sprite_fifo.current_sprite_high = read_ppu_memory(base_sprite_address + 1);
}

void PPU::sprite_push_row() {
    int sprite_on_x = (int)m_sprite_fifo.current_sprite.x_pos - 8;

    // wait till we reach the sprite on screen
    if (m_background_fifo.on_screen_x < sprite_on_x) {
        return; 
    }

    // push all 8 pixels at once
    for (int px = 0; px < 8; ++px) {
        int bit = m_sprite_fifo.current_sprite.x_flip() ? px : (7 - px);
        byte low_bit = (m_sprite_fifo.current_sprite_low >> bit) & 0x1;
        byte high_bit = (m_sprite_fifo.current_sprite_high >> bit) & 0x1;
        byte colour = (high_bit << 1) | low_bit;

        fifo_pixel new_pixel = {
            .colour = colour,
            .pallete = m_sprite_fifo.current_sprite.use_pallet_one() ? m_ppu_io.obp1 : m_ppu_io.obp0,
            .sprite = true,
            .background_priority = m_sprite_fifo.current_sprite.background_priority(),
            .screen_x = sprite_on_x + px,
        };

        push_pixel(m_sprite_fifo_queue, new_pixel);  // Remove the condition here
    }

    //allow background fifo and mixing to resume
    m_background_fifo.paused = false;
    m_sprite_fifo.sprite_found = false;
}

//fifo helper methods

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

ushort PPU::get_tile_address_from_id(const byte& tile_id) {
    ushort tile_address = 0x0000;
    ushort tile_data_base_address = 0x9000;
    if ((m_ppu_io.lcdc & 0x10) != 0) {
        tile_data_base_address = 0x8000;
        tile_address = (tile_data_base_address + m_background_fifo.current_pixel_id * 16);
    }
    else {
        sbyte signed_tile_number = (sbyte)m_background_fifo.current_pixel_id;
        tile_address = (tile_data_base_address + signed_tile_number * 16);
    }

    return tile_address;
}

void PPU::reset_bg_fifo(background_fifo_context& fifo, std::queue<fifo_pixel>& fifo_queue) {
    fifo.current_x = 0;
    fifo.current_pixel_id = 0x00;
    fifo.current_pixel_low = 0x00;
    fifo.current_pixel_high = 0x00;
    fifo.current_state = fifo_FETCH_TILE_NUMBER;
    fifo.fifo_ticks = 0;
    fifo.on_screen_x = 0;

    clear_fifo(fifo_queue);

    fifo.start_of_scanline = true;
    fifo.scx_discard = false;
}

void PPU::reset_sprite_search() {
    m_sprite_search.index = 0;
    m_sprite_search.ticks = 0;

    m_sprite_buffer.clear();
    m_sprite_buffer.resize(0);
}

void PPU::mix_pixels() {
    if (m_bg_fifo_queue.empty()) return;

    if (m_background_fifo.paused) return;

    // Handle SCX discard (only background, not sprites!)
    if (!m_background_fifo.scx_discard) {
        if (m_bg_fifo_queue.size() >= 8) {
            for (int i = 0; i < (m_ppu_io.scx % 8); i++) {
                m_bg_fifo_queue.pop();
            }
            m_background_fifo.scx_discard = true;
        }
        return;
    }

    // Pop background pixel
    fifo_pixel bg_pixel = pop_pixel(m_bg_fifo_queue);
    fifo_pixel final_pixel = bg_pixel; // default to background

    // Simple sprite check: if sprite pixel exists and isn't transparent, use it
    if (!m_sprite_fifo_queue.empty()) {
        fifo_pixel sprite_pixel = pop_pixel(m_sprite_fifo_queue);
        if (sprite_pixel.colour != 0) { // 0 = transparent
            final_pixel = sprite_pixel; // sprite always wins if not transparent
        }
    }

    // Simple palette resolve - no priority checks
    int palette_shift = final_pixel.colour * 2;
    int palette_colour;

    if (final_pixel.sprite) {
        // Use sprite palette
        const byte obp = final_pixel.pallete;
        palette_colour = (obp >> palette_shift) & 0x03;
    }
    else {
        // Use background palette
        palette_colour = (m_ppu_io.bgp >> palette_shift) & 0x03;
    }

    m_bg_frame_buffer[m_ppu_io.ly * SCREEN_WIDTH + m_background_fifo.on_screen_x] =
        m_pallete_colours[palette_colour];

    m_background_fifo.on_screen_x++;
}