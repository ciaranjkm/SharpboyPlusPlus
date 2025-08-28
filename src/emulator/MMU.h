#pragma once

#include "_definitions.h"
#include <memory>
#include <array>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

class Emulator;

const int BOOT_ROM_SIZE = 0x100;

const int BASE_CARTRIDGE_SIZE = 0x8000;
const int VRAM_SIZE = 0x2000;
const int BASE_EXTERNAL_RAM_SIZE = 0x2000;
const int WRAM_SIZE = 0x2000;
const int OAM_SIZE = 0xa0;
const int IO_SIZE = 0x80;
const int HRAM_SIZE = 0x7f;

struct io_map {
	//input
	byte JOYP = 0x00;

	//serial
	byte SB = 0x00;
	byte SC = 0x00;

	//interrupts
	byte IF = 0x00;

	//sound not impl

	//ppu
	byte DMA = 0x00;

	//boot rom control
	byte BANK = 0x00;
};

struct memory_map {
	std::array<byte, BOOT_ROM_SIZE> boot_rom = std::array<byte, BOOT_ROM_SIZE>();

	std::vector<byte> cartridge = std::vector<byte>(BASE_CARTRIDGE_SIZE);
	std::array<byte, VRAM_SIZE> vram = std::array<byte, VRAM_SIZE>();
	std::vector<byte> eram = std::vector<byte>(BASE_EXTERNAL_RAM_SIZE);
	std::array<byte, WRAM_SIZE> wram = std::array<byte, WRAM_SIZE>();
	std::array<byte, OAM_SIZE> oam = std::array<byte, OAM_SIZE>();
	io_map io = io_map();
	std::array<byte, HRAM_SIZE> hram = std::array<byte, HRAM_SIZE>();
	byte IE = 0x00;
};

class MMU {
public:
	MMU(std::shared_ptr<Emulator> emulator_ptr);
	~MMU();

	const bool is_mmu_initialised();
	void reset_mmu(const rom_header& header, const std::vector<byte>& rom, const std::array<byte, 0x100>& boot_rom);

	byte read_from_memory(const ushort& address);
	void write_to_memory(const ushort& address, const byte& value);

	byte unblocked_read(const ushort& address);
	void unblocked_write(const ushort& address, const byte& value);

	byte read_io(const byte& io_target);
	void write_io(const byte& io_target, const byte& value);

	void swap_cartridge_and_boot_roms();

	void dma_tick();

private:
	std::shared_ptr<Emulator> emulator_ptr;

	bool initialised = false;
	bool using_boot_rom = false;
	memory_map memory;

	ushort dma_address = 0x0000;
	bool start_new_dma = false;
	bool dma_active = false;
	int dma_delay = 0;

	int dma_ticks_this_cycles = 0;
	int dma_cycles = 0;

	const int DEFAULT_DMA_DELAY = 8;
	const int DMA_TOTAL_TICKS = 160;
	int total_dma_ticks = 0;
};