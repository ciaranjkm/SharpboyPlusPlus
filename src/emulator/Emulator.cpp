#include "Emulator.h"

Emulator::Emulator() {
	this->using_boot_rom = false;
	this->single_step_test_mode = false;

	this->initialised = true;
}

Emulator::~Emulator() {
	printf("[SB] Shutting down EMU object.\n"); // this doesnt print to console when emu is destroyed??
	//delete any pointers (fallback as already smart pointers) 
	close_emulator();
	close_imgui();
	destroy_SDL_components(window, renderer);
}



void Emulator::set_emu_pointer(std::shared_ptr<Emulator> emulator_ptr) {
	this->current_emulator_instance = emulator_ptr;
}

bool Emulator::is_using_boot_rom() {
	return using_boot_rom;
}

void Emulator::run_emulator() {
	if (!sdl_initialised) {
		printf("[SB] SDL initialised can't run emulator!\n");
		return;
	}

	//todo
	//temporary for testing, move to imgui button action and check for running emus first, work on ppu needed before this!
	initialise_emu_instance("roms/PASSED/instr_timing.gb", false);
	emulator_running = true; //todo change this not running by default

	sdl_running = true;

	//todo add function to start clock for emulator to run at correct speed and toggle a bool flag to stop function running again before emu is closed
	auto last_time = std::chrono::high_resolution_clock::now();
	double cycles_pending = 0;

	const double CLOCKSPEED = 4194304.0;
	const double MILLISECONDS = 1000.0;

	while (sdl_running) {
		SDL_Event e;
		poll_SDL_events(&e, current_emulator_instance);

		if (!sdl_running) {
			break;
		}
		if (show_imgui) {
			render_imgui();
		}

		//drawimgui

		if (emulator_running) {
			//todo add all timing into a class maybe static so only 1 emu at a time?
			auto now = std::chrono::high_resolution_clock::now();
			double elapsed = std::chrono::duration<double, std::milli>(now - last_time).count();
			last_time = now;

			cycles_pending = (CLOCKSPEED / MILLISECONDS) * elapsed;

			while (cycles_pending >= 1.0) {
				int cycles_completed = 0;
				CPU_ptr->step_cpu(cycles_completed, false);
				cycles_pending -= cycles_completed;
			}

			if (PPU_ptr->is_draw_read()) {
				if (show_imgui) {
					ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
				}

				SDL_RenderPresent(renderer);

				PPU_ptr->reset_draw_ready();
			}
		}
		else {
			SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
			SDL_RenderClear(renderer);

			if (show_imgui) {
				ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
			}

			SDL_RenderPresent(renderer);
		}


		//render gameboy

		//renderimgui

		//delay to slow cpu a bit
		SDL_Delay(1);
	}

	close_imgui();
	destroy_SDL_components(window, renderer);
}

void Emulator::close_emulator() {
	printf("+----------------------------------------+\n");
	sdl_running = false;
	emulator_running = false;

	PPU_ptr = nullptr;
	MMU_ptr = nullptr;
	TIMER_ptr = nullptr;
	CPU_ptr = nullptr;
}

void Emulator::set_running(const bool& state) {
	emulator_running = state;
}

bool Emulator::get_single_step_mode() {
	return single_step_test_mode;
}



void Emulator::initialise_SDL() {
	initialise_SDL_components(sdl_initialised, &window, &renderer);
}

void Emulator::toggle_show_imgui() {
	show_imgui = !show_imgui;
}



void Emulator::tick_other_components(const int& cycles) {
	for (int i = 0; i < cycles; i++) {
		TIMER_ptr->timers_tick();
		PPU_ptr->ppu_tick();
		MMU_ptr->dma_tick();
		//apu.tick
	}
}

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



byte Emulator::bus_read(const ushort& address) {
	byte value = MMU_ptr->read_from_memory(address);
	return value;
}

void Emulator::bus_write(const ushort& address, const byte& value) {
	MMU_ptr->write_to_memory(address, value);
}

byte Emulator::read_timer_io(const byte& timer_io) {
	return TIMER_ptr->read_timer_io(timer_io);
}

void Emulator::write_timer_io(const byte& timer_io, const byte& value) {
	TIMER_ptr->write_timer_io(timer_io, value);
	return;
}

byte Emulator::read_ppu_io(const byte& ppu_io) {
	return PPU_ptr->read_ppu_io(ppu_io);
}

void Emulator::write_ppu_io(const byte& ppu_io, const byte& value) {
	PPU_ptr->write_ppu_io(ppu_io, value);
	return;
}

byte Emulator::io_instant_read(const io_addresses& io_target) {
	return MMU_ptr->read_io(io_target);
}

void Emulator::io_instant_write(const io_addresses& io_target, const byte& value) {
	MMU_ptr->write_io(io_target, value);
	return;
}



ppu_modes Emulator::get_current_ppu_mode() {
	return PPU_ptr->get_current_mode();
}

int Emulator::initialise_emu_instance(const std::string& rom_file_name, const bool& using_boot_rom) {
	if (this->single_step_test_mode) {
		//done with for now will reimpliment later on 
		return -255;
	}

	this->using_boot_rom = false;

	//load rom file into memory and optionally boot rom + parse for rom header

	std::unique_ptr<std::vector<byte>> rom_file_ptr = std::make_unique<std::vector<byte>>();
	if (!load_rom_file(rom_file_name, *rom_file_ptr)) {
		printf("[SB] Failed to load ROM file from %s\n", rom_file_name.c_str());
		return -1;
	}

	header = rom_header();
	parse_rom_file_header(header, *rom_file_ptr);

	//load boot rom into memory
	std::unique_ptr<std::array<byte, 0x100>> boot_rom_ptr = std::make_unique<std::array<byte, 0x100>>();
	if (this->using_boot_rom) {
		if (!load_boot_rom_file("boot/boot.bin", *boot_rom_ptr)) {
			printf("[SB] Failed to load BOOT ROM file from boot/boot.bin\n");
		}
	}

	//init cpu and reset it
	current_emulator_instance->CPU_ptr = std::make_unique<CPU>(this->current_emulator_instance);
	if (!CPU_ptr->is_cpu_initialised()) {
		CPU_ptr = nullptr;
		printf("[SB] CPU init failed, stopping.\n");
		return -2;
	}
	CPU_ptr->reset_cpu(header.checksum_byte == 0);

	//init mmu and reset it
	current_emulator_instance->MMU_ptr = std::make_unique<MMU>(this->current_emulator_instance);
	if (!MMU_ptr->is_mmu_initialised()) {
		CPU_ptr = nullptr;
		MMU_ptr = nullptr;

		printf("[SB] MMU init failed, stopping.\n");
		return -3;
	}
	MMU_ptr->reset_mmu(header, *rom_file_ptr, *boot_rom_ptr);

	//init timers and reset it 
	current_emulator_instance->TIMER_ptr = std::make_unique<Timers>(this->current_emulator_instance);
	if (!TIMER_ptr->is_timers_initialised()) {
		CPU_ptr = nullptr;
		MMU_ptr = nullptr;
		TIMER_ptr = nullptr;

		printf("[SB] TIMER init failed, stopping.\n");
		return -4;
	}
	TIMER_ptr->reset_timers();

	//init ppu and reset it
	current_emulator_instance->PPU_ptr = std::make_unique<PPU>(this->current_emulator_instance, renderer);
	if (!PPU_ptr->is_ppu_initialised()) {
		CPU_ptr = nullptr;
		MMU_ptr = nullptr;
		TIMER_ptr = nullptr;
		PPU_ptr = nullptr;

		printf("[SB] PPU init failed, stopping.\n");
		return -5;
	}
	PPU_ptr->reset_ppu();

	if (using_boot_rom) {
		tick_other_components(4);
	}

	initialised = true;
	printf("+----------------------------------------+\n");
	printf("[SB] Success initialing emulator with %s\n", rom_file_name.c_str());
	if (this->using_boot_rom) {
		printf("[SB] Starting emulator now with boot rom!\n");
	}
	else {
		printf("[SB] Starting emulator without boot rom!\n");
	}

	return 0;
}

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