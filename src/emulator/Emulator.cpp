#include "Emulator.h"

Emulator::Emulator(const bool& single_step_mode, const bool& using_boot_rom) {
	this->initialised = false;
	
	//disable sdl for cli only, unable to use video, input and sound

	this->using_SDL = false;
	this->sdl_running = false;

	this->single_step_test_mode = single_step_mode;
	this->using_boot_rom = using_boot_rom;
}

Emulator::Emulator(const bool& using_SDL) {
	this->using_boot_rom = true;
	this->single_step_test_mode = false;

	this->using_SDL = using_SDL;
	if (!this->using_SDL) {
		printf("[SB] Not creating SDL objects, exiting!\n");
		this->initialised = false;
		return;
	}

	this->initialised = true;
}

Emulator::~Emulator() {
	//delete any pointers (fallback as already smart pointers) 
	close_emulator();

	printf("[SB] Shutting down EMU object.\n");
}

bool Emulator::set_emu_pointer(std::shared_ptr<Emulator> emulator_ptr) {
	this->current_emulator_instance = emulator_ptr;
	if (this->current_emulator_instance == nullptr) {
		printf("[SB] Emulator is not valid, given nullptr!\n");
		return false;
	}
	else {
		return true;
	}
}

int Emulator::initialise_emu_instance(const std::string& rom_file_name) {
	if (this->single_step_test_mode) {
		//done with for now will reimpliment later on 
		return -255;
	}
	
	//load rom file into memory and optionally boot rom + parse for rom header

	std::unique_ptr<std::vector<byte>> rom_file_ptr = std::make_unique<std::vector<byte>>();
	if (!load_rom_file(rom_file_name, *rom_file_ptr)) {
		printf("[SB] Failed to load ROM file from %s\n", rom_file_name.c_str());
		return -1;
	}

	header = rom_header();
	parse_rom_file_header(header, *rom_file_ptr);

	this->using_boot_rom = false;
	//load boot rom into memory
	std::unique_ptr<std::array<byte, 0x100>> boot_rom_ptr = std::make_unique<std::array<byte, 0x100>>();
	if (this->using_boot_rom) {
		if (!load_boot_rom_file("boot/boot.bin", *boot_rom_ptr)) {
			printf("[SB] Failed to load BOOT ROM file from boot/boot.bin\n");
			this->using_boot_rom = false;
		}
	}

	//init cpu and reset it
	current_emulator_instance->CPU_ptr = std::make_unique<CPU>(this->current_emulator_instance);
	if (!CPU_ptr->is_cpu_initialised()) {
		CPU_ptr = nullptr;
		printf("[SB] CPU init failed, stopping.\n");
		return -2;
	}
	CPU_ptr->reset_cpu(this->using_boot_rom, header.checksum_byte == 0);

	//init mmu and reset it
	current_emulator_instance->MMU_ptr = std::make_unique<MMU>(this->current_emulator_instance);
	if (!MMU_ptr->is_mmu_initialised()) {
		CPU_ptr = nullptr;
		MMU_ptr = nullptr;

		printf("[SB] MMU init failed, stopping.\n");
		return -3;
	}
	MMU_ptr->reset_mmu(header, *rom_file_ptr, this->using_boot_rom, *boot_rom_ptr);

	//init timers and reset it 
	current_emulator_instance->TIMER_ptr = std::make_unique<Timers>(this->current_emulator_instance);
	if (!TIMER_ptr->is_timers_initialised()) {
		CPU_ptr = nullptr;
		MMU_ptr = nullptr;
		TIMER_ptr = nullptr;

		printf("[SB] TIMER init failed, stopping.\n");
		return -4;
	}

	//init ppu and reset it
	current_emulator_instance->PPU_ptr = std::make_unique<PPU>(this->current_emulator_instance, this->using_SDL, renderer);
	if (!PPU_ptr->is_ppu_initialised()) {
		CPU_ptr = nullptr;
		MMU_ptr = nullptr;
		TIMER_ptr = nullptr;
		PPU_ptr = nullptr;

		printf("[SB] PPU init failed, stopping.\n");
		return -5;
	}

	tick_other_components(4);
	initialised = true;

	return 0;
}

void Emulator::set_running(const bool& state) {
	running = state;
}

void Emulator::run_emulator() {
	if (!initialised) {
		printf("[SB] Not initialised can't run emulator!\n");
		return;
	}
	/*
	if (!using_SDL) {
		while (running) {
			auto now = clock::now();
			std::chrono::duration<double> elapsed = now - start_time;

			long long expected_cycles = static_cast<long long>(elapsed.count() * CLOCKSPEED);

			int cycles = 0;

			while (emu_cycles < expected_cycles) {
				CPU_ptr->step_cpu(cycles, false); 
				emu_cycles += cycles;

				if (!sdl_running || !running) {
					break;
				}
			}

			// show fps
			
			if (std::chrono::duration<double>(now - last_fps_time).count() >= 1.0) {
				long long cyclesThisSecond = emu_cycles - cyclesAtLastReport;
				std::cout << "Cycles this second: " << cyclesThisSecond << "\n";

				cyclesAtLastReport = emu_cycles;
				last_fps_time = now;
			}
			
			// Tiny delay to yield CPU — optional
			SDL_Delay(1);
		}
	}
	*/

	sdl_running = true;
	running = true;

	auto last_time = std::chrono::high_resolution_clock::now();
	double cycles_pending = 0;

	const double CLOCKSPEED = 4194304.0;
	const double MILLISECONDS = 1000.0;

	tick_other_components(4);

	while (sdl_running) {
		SDL_Event e;
		poll_SDL_events(&e, current_emulator_instance);

		if (!sdl_running) {
			break;
		}

		if (running) {
			auto now = std::chrono::high_resolution_clock::now();
			double elapsed = std::chrono::duration<double, std::milli>(now - last_time).count();
			last_time = now;

			cycles_pending = (CLOCKSPEED / MILLISECONDS) * elapsed;

			while (cycles_pending >= 1.0) {
				int cycles_completed = 0;
				CPU_ptr->step_cpu(cycles_completed, false);
				cycles_pending -= cycles_completed;
			}
			SDL_Delay(1);
		}

		SDL_Delay(16);
	}

	destroy_SDL_components(window, renderer);
} 

void Emulator::close_emulator() {
	sdl_running = false;
	running = false;

	//destory emu components here first

	PPU_ptr = nullptr;
	MMU_ptr = nullptr;
	TIMER_ptr = nullptr;
	CPU_ptr = nullptr;

	//do other shutdown here (sdl etc)
	printf("shutdown emu\n");
}

const bool Emulator::get_single_step_mode() {
	return &single_step_test_mode;
}

bool Emulator::initialise_SDL() {
	bool initialised_SDL = false;

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("[SB] Failed SDL initialisation, error: %s\n", SDL_GetError());
		return initialised_SDL;
	}
	printf("[SB] SDL initialised\n");

	window = SDL_CreateWindow("Sharpboy++", 720, 640, SDL_WINDOW_RESIZABLE);
	if (window == nullptr) {
		SDL_Quit();

		printf("[SB] Failed SDL window initialisation, error: %s\n", SDL_GetError());
		return initialised_SDL;
	}
	printf("[SB] SDL window initialised\n");

	renderer = SDL_CreateRenderer(window, NULL);
	if (renderer == nullptr) {
		SDL_DestroyWindow(window);
		window = nullptr;

		SDL_Quit();

		printf("[SB] Failed SDL renderer initialisation, error: %s\n", SDL_GetError());
		return initialised_SDL;
	}
	printf("[SB] SDL renderer initialised\n");

	return !initialised_SDL;
}

void Emulator::render_texture(SDL_Texture* texture) {
	SDL_FRect dest = { 0, 0, 160 * 1, 144 * 1 };
	SDL_RenderTexture(renderer, texture, NULL, &dest);
}

void Emulator::clear_bg_renderer() {
	SDL_RenderClear(renderer);
}

void Emulator::tick_other_components(const int& cycles) {
	for (int i = 0; i < cycles; i++) {
		TIMER_ptr->timers_tick();
		PPU_ptr->ppu_tick();
		MMU_ptr->dma_tick();
		//apu.tick
	}
}

byte Emulator::fetch_next_byte(ushort& pc) {
	byte opcode = MMU_ptr->read_from_memory(pc);
	return opcode;
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

byte Emulator::memory_instant_read(const ushort& address) {
	return MMU_ptr->unblocked_read(address);
}

ppu_modes Emulator::get_current_ppu_mode() {
	return PPU_ptr->get_current_mode();
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