#include "Emulator.h"

Emulator::Emulator() {
	this->m_using_boot_rom = false;
	this->m_initialised = true;
}

Emulator::~Emulator() {
	printf("[SB] Shutting down EMU object.\n"); // this doesnt print to console when emu is destroyed??
	//delete any pointers (fallback as already smart pointers) 
	close_emulator();
}

//instance setup

int Emulator::initialise_emu_instance(const std::string& rom_file_name, const bool& using_boot_rom) {
	this->m_using_boot_rom = using_boot_rom;

	//load rom file into memory and optionally boot rom + parse for rom header

	std::unique_ptr<std::vector<byte>> rom_file_ptr = std::make_unique<std::vector<byte>>();
	if (!load_rom_file(rom_file_name, *rom_file_ptr)) {
		printf("[SB] Failed to load ROM file from %s\n", rom_file_name.c_str());
		return -1;
	}

	m_header = rom_header();
	parse_rom_file_header(m_header, *rom_file_ptr);

	//load boot rom into memory
	std::unique_ptr<std::array<byte, 0x100>> boot_rom_ptr = std::make_unique<std::array<byte, 0x100>>();
	if (this->m_using_boot_rom) {
		if (!load_boot_rom_file("boot/boot.bin", *boot_rom_ptr)) {
			printf("[SB] Failed to load BOOT ROM file from boot/boot.bin\n");
		}
	}

	//init cpu and reset it
	m_cpu = std::make_unique<CPU>(shared_from_this());
	if (!m_cpu->is_cpu_initialised()) {
		printf("[SB] CPU init failed, stopping.\n");
		return -2;
	}
	m_cpu->reset_cpu(m_header.checksum_byte == 0);

	//init mmu and reset it
	m_mmu = std::make_unique<MMU>(shared_from_this());
	if (!m_mmu->is_mmu_initialised()) {
		printf("[SB] MMU init failed, stopping.\n");
		return -3;
	}
	m_mmu->reset_mmu(m_header, *rom_file_ptr, *boot_rom_ptr);

	//init timers and reset it 
	m_timers = std::make_unique<Timers>(shared_from_this());
	if (!m_timers->is_timers_initialised()) {
		m_cpu = nullptr;
		m_mmu = nullptr;
		m_timers = nullptr;

		printf("[SB] TIMER init failed, stopping.\n");
		return -4;
	}
	m_timers->reset_timers();

	//init ppu and reset it
	m_ppu = std::make_unique<PPU>(shared_from_this());
	if (!m_ppu->is_ppu_initialised()) {
		m_cpu = nullptr;
		m_mmu = nullptr;
		m_timers = nullptr;
		m_ppu = nullptr;

		printf("[SB] PPU init failed, stopping.\n");
		return -5;
	}
	m_ppu->reset_ppu();

	if (using_boot_rom) {
		tick_other_components(4);
	}

	m_initialised = true;
	printf("+----------------------------------------+\n");
	printf("[SB] Success initialing emulator with %s\n", rom_file_name.c_str());
	if (this->m_using_boot_rom) {
		printf("[SB] Starting emulator now with boot rom!\n");
	}
	else {
		printf("[SB] Starting emulator without boot rom!\n");
	}

	return 0;
}

bool Emulator::is_using_boot_rom() {
	return m_using_boot_rom;
}

void Emulator::close_emulator() {
	printf("+----------------------------------------+\n");

	m_ppu.reset();
	m_ppu = nullptr;

	m_mmu.reset();
	m_mmu = nullptr;

	m_timers.reset();
	m_timers = nullptr;

	m_cpu.reset();
	m_cpu = nullptr;
}

//execution 

int Emulator::run_next_instruction() {
	int cycles_completed = 0;
	m_cpu->step_cpu(cycles_completed, false);
	return cycles_completed;
}

//ticks for other components

void Emulator::tick_other_components(const int& cycles) {
	for (int i = 0; i < cycles; i++) {
		m_timers->timers_tick();
		m_ppu->ppu_tick();
		m_ppu->dma_tick();
		//apu.tick
	}
}

//interrupts + input

void Emulator::trigger_interrupt(const interrupt_types& interrupt) {
	if (interrupt >= int_VBLANK && interrupt <= int_JOYPAD) {
		byte IF = io_instant_read(io_IF);
		IF = IF | (1 << interrupt);
		io_instant_write(io_IF, IF);
	}
}

void Emulator::clear_interrupt(const int& interrupt) {
	if (interrupt >= int_VBLANK && interrupt <= int_JOYPAD) {
		byte IF = io_instant_read(io_IF);
		IF = IF & ~(1 << interrupt);
		io_instant_write(io_IF, IF);
	}
}

void Emulator::trigger_keypress(const joypad_buttons& button, const bool& key_down) {
	bool pressed_before = false;

	switch (button) {
	case button_up:
		pressed_before = m_joypad.up;
		m_joypad.up = key_down;
		break;

	case button_down:
		pressed_before = m_joypad.down;
		m_joypad.down = key_down;
		break;

	case button_left:
		pressed_before = m_joypad.left;
		m_joypad.left = key_down;
		break;

	case button_right:
		pressed_before = m_joypad.right;
		m_joypad.right = key_down;
		break;

	case button_start:
		pressed_before = m_joypad.start;
		m_joypad.start = key_down;
		break;

	case button_select:
		pressed_before = m_joypad.select;
		m_joypad.select = key_down;
		break;

	case button_a:
		pressed_before = m_joypad.a;
		m_joypad.a = key_down;
		break;

	case button_b:
		pressed_before = m_joypad.b;
		m_joypad.b = key_down;
		break;
	}
	/*
	if (!pressed_before && key_down) {
		byte joyp = io_instant_read(io_true_JOYP);
		bool is_selected_group = false;

		if (button >= button_start && button <= button_a) { // Action buttons
			is_selected_group = !(joyp & 0x20);
		}
		else { 
			is_selected_group = !(joyp & 0x10);
		}

		if (is_selected_group) {
			trigger_interrupt(int_JOYPAD);
		}
	}
	*/
}

byte Emulator::read_joypad_state() {
	byte joyp = io_instant_read(io_true_JOYP);

	byte result = joyp & 0xF0;
	byte keys = 0x0F; // all released (active high)

	if (!(joyp & 0x20)) {
		if (m_joypad.start)  keys &= ~(1 << 3);
		if (m_joypad.select) keys &= ~(1 << 2);
		if (m_joypad.b)      keys &= ~(1 << 1);
		if (m_joypad.a)      keys &= ~(1 << 0);
	}

	if (!(joyp & 0x10)) {
		if (m_joypad.down)  keys &= ~(1 << 3);
		if (m_joypad.up)    keys &= ~(1 << 2);
		if (m_joypad.left)  keys &= ~(1 << 1);
		if (m_joypad.right) keys &= ~(1 << 0);
	}

	result |= keys;

	return result;
}

//bus functions

byte Emulator::bus_read(const ushort& address) {
	if ((address >= 0x8000 && address <= 0xa000) || (address >= 0xfe00 && address < 0xfea0)) {
		return m_ppu->read_ppu_memory(address);
	}
	else if (address >= 0xff00 && address < 0xff80) {
		return io_instant_read((byte)(address & 0x00ff));
	}
	else {
		return m_mmu->read_from_memory(address);
	}
}

void Emulator::bus_write(const ushort& address, const byte& value) {
	if ((address >= 0x8000 && address <= 0xa000) || (address >= 0xfe00 && address < 0xfea0)) {
		m_ppu->write_ppu_memory(address, value);
	}
	else if (address >= 0xff00 && address < 0xff80) {
		return io_instant_write((byte)(address & 0x00ff), value);
	}
	else {
		m_mmu->write_to_memory(address, value);
	}
}

byte Emulator::io_instant_read(const byte& io_target) {
	if (io_target >= io_DIV && io_target <= io_TAC) {
		return m_timers->read_timer_io(io_target);
	}
	else if (io_target >= io_LCDC && io_target <= io_WX) {
		return m_ppu->read_ppu_io(io_target);
	}
	else {
		return m_mmu->read_io(io_target);
	}
}

void Emulator::io_instant_write(const byte& io_target, const byte& value) {
	if (io_target >= io_DIV && io_target <= io_TAC) {
		m_timers->io_instant_write(io_target, value);
		return;
	}
	else if (io_target >= io_LCDC && io_target <= io_WX) {
		m_ppu->write_ppu_io(io_target, value);
		return;
	}
	else {
		m_mmu->write_io(io_target, value);
		return;
	}
}

//ppu functions

const std::array<uint32_t, 160 * 144>& Emulator::get_frame_buffer() {
	return m_ppu->get_bg_frame_buffer();
}

bool Emulator::draw_ready() {
	return m_ppu->is_draw_ready();
}

void Emulator::reset_draw_ready() {
	m_ppu->reset_draw_ready();
}

//debug

cpu_data Emulator::get_cpu_data() {
	return m_cpu->get_data();
}

std::array<uint32_t, 64> Emulator::get_next_tile(const int& index) {
	return m_ppu->get_next_tile(index);
}

//rom file loading (todo move this to application class and load the roms into MMU
//					then when they go out of scope of init they will be destroyed)
bool Emulator::load_rom_file(const std::string& file_name, std::vector<byte>& rom) {
	if (!std::filesystem::exists(file_name)) {
		printf("[SB] Path to rom doesn't exist.\n");
		return false;
	}

	std::ifstream rom_file(file_name, std::ios::binary | std::ios::ate);
	if (rom_file) {
		std::streamsize s_size = rom_file.tellg();
		rom_file.seekg(0, std::ios::beg);

		if (s_size < BOOT_ROM_SIZE) {
			printf("[SB] Rom is not a valid size for DMG. Try another one!\n");
			return false;
		}

		rom.clear();
		rom.resize(s_size);

		if (rom_file.read(reinterpret_cast<char*>(rom.data()), s_size)) {
			printf("[SB] Success reading %d bytes from %s\n", (int)s_size, file_name.c_str());
		}

		rom_file.close();
		return true;
	}

	printf("[SB] Failed to open ROM file at %s\n", file_name.c_str());
	return false;
}

bool Emulator::load_boot_rom_file(const std::string& file_name, std::array<byte, 0x100>& boot_rom) {
	if (!std::filesystem::exists(file_name)) {
		printf("[SB] Path to boot rom doesn't exist. Not using one!\n");
		return false;
	}

	std::ifstream boot_rom_file(file_name, std::ios::binary | std::ios::ate);
	if (boot_rom_file) {
		std::streamsize s_size = boot_rom_file.tellg();
		boot_rom_file.seekg(0, std::ios::beg);

		if (boot_rom_file.read(reinterpret_cast<char*>(boot_rom.data()), s_size)) {
			printf("[SB] Success reading %d bytes from %s\n", (int)s_size, file_name.c_str());
		}

		boot_rom_file.close();
		return true;
	}
	printf("[SB] Failed to open BOOT ROM file at %s\n", file_name.c_str());
	return false;
}

void Emulator::parse_rom_file_header(rom_header& header, const std::vector<byte>& rom) {
	//title
	const int title_start = 0x0134;
	const int title_length = 0x0010;

	for (int i = 0; i < title_length; i++) {
		header.name.push_back(static_cast<char>(rom[title_start + i]));
	}

	//cartridge type
	const int cartridge_type_start = 0x0147;
	byte cart_type = rom[cartridge_type_start];
	switch (cart_type) {
	case ROM_ONLY: header.cartridge_type = ROM_ONLY; break;
	case MBC1: header.cartridge_type = MBC1; break;
	case MBC1_RAM: header.cartridge_type = MBC1_RAM; break;
	case MBC1_RAM_BATT: header.cartridge_type = MBC1_RAM_BATT; break;
	case MBC2: header.cartridge_type = MBC2; break;
	case MBC2_BATT: header.cartridge_type = MBC2_BATT; break;
	case MBC3: header.cartridge_type = MBC3; break;
	case MBC3_RAM: header.cartridge_type = MBC3_RAM; break;
	case MBC3_RAM_BATT: header.cartridge_type = MBC3_RAM_BATT; break;
	case MBC3_TIMER_BATT: header.cartridge_type = MBC3_TIMER_BATT; break;
	case MBC5: header.cartridge_type = MBC5; break;
	case MBC5_RAM: header.cartridge_type = MBC5_RAM; break;
	case MBC5_RAM_BATT: header.cartridge_type = MBC5_RAM_BATT; break;

	default: header.cartridge_type = ROM_ONLY; break;
	}

	//rom size
	const int rom_size_start = 0x0148;
	byte rom_size = rom[rom_size_start];
	switch (rom_size) {
	case rom_KiB_32: header.cart_rom_size = rom_KiB_32; break;
	case rom_KiB_64: header.cart_rom_size = rom_KiB_64; break;
	case rom_KiB_128: header.cart_rom_size = rom_KiB_128; break;
	case rom_KiB_256: header.cart_rom_size = rom_KiB_256; break;
	case rom_KiB_512: header.cart_rom_size = rom_KiB_512; break;
	case rom_MiB_1: header.cart_rom_size = rom_MiB_1; break;
	case rom_MiB_2: header.cart_rom_size = rom_MiB_2; break;
	case rom_MiB_4: header.cart_rom_size = rom_MiB_4; break;
	case rom_MiB_8: header.cart_rom_size = rom_MiB_8; break;
	
	default: header.cart_rom_size = rom_KiB_32; break;
	}

	//ram size
	const int ram_size_start = 0x0149;
	byte ram_size = rom[ram_size_start];
	switch (ram_size) {
	case ram_NONE: header.cart_ram_size = ram_NONE; break;
	case ram_UNUSED: header.cart_ram_size = ram_UNUSED; break;
	case ram_KiB_8: header.cart_ram_size = ram_KiB_8; break;
	case ram_KiB_32: header.cart_ram_size = ram_KiB_32; break;
	case ram_KiB_128: header.cart_ram_size = ram_KiB_128; break;
	case ram_KiB_64: header.cart_ram_size = ram_KiB_64; break;

	default: header.cart_ram_size = ram_NONE; break;

	}

	//checksum
	uint8_t checksum = 0;
	for (uint16_t address = 0x0134; address <= 0x014C; address++) {
		checksum = checksum - rom[address] - 1;
	}

	header.checksum_byte = checksum;
	return;
}