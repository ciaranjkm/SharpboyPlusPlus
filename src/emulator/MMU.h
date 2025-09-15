#pragma once

#include "defs.h"
#include <memory>
#include <array>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

class Emulator;

const int BOOT_ROM_SIZE = 0x100;

const int BASE_CARTRIDGE_SIZE = 0x8000;
const int BASE_EXTERNAL_RAM_SIZE = 0x2000;
const int WRAM_SIZE = 0x2000;
const int HRAM_SIZE = 0x7f;

enum io_addresses {
	io_JOYP = 0x00,
	io_true_JOYP = 0xff,
	io_SB = 0x01,
	io_SC = 0x02,
	io_DIV = 0x04,
	io_TIMA = 0x05,
	io_TMA = 0x06,
	io_TAC = 0x07,
	io_IF = 0x0f,

	//sound here

	io_LCDC = 0x40,
	io_STAT = 0x41,
	io_SCY = 0x42,
	io_SCX = 0x43,
	io_LY = 0x44,
	io_LYC = 0x45,
	io_DMA = 0x46,
	io_BGP = 0x47,
	io_OBP0 = 0x48,
	io_OBP1 = 0x49,
	io_WY = 0x4a,
	io_WX = 0x4b,

	io_BANK = 0x50,
};

struct io_map {
	//input
	byte JOYP = 0x00;
	byte true_JOYP = 0x00;

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
	std::vector<byte> eram = std::vector<byte>(BASE_EXTERNAL_RAM_SIZE);
	std::array<byte, WRAM_SIZE> wram = std::array<byte, WRAM_SIZE>();
	io_map io = io_map();
	std::array<byte, HRAM_SIZE> hram = std::array<byte, HRAM_SIZE>();
	byte IE = 0x00;
};

//todo make this the base class for mbc0, mbc1 etc and have them override the functions to redirect read/writes based on rom banking and external ram
class MMU {
public:
	MMU(std::shared_ptr<Emulator> emulator);
	~MMU();

	const bool is_mmu_initialised();
	void reset_mmu(const rom_header& header, const std::vector<byte>& rom, const std::array<byte, 0x100>& boot_rom);

	byte read_from_memory(const ushort& address);
	void write_to_memory(const ushort& address, const byte& value);

	byte read_io(const byte& io_target);
	void write_io(const byte& io_target, const byte& value);

	void swap_cartridge_and_boot_roms();
private:
	std::shared_ptr<Emulator> emulator = nullptr;

	bool initialised = false;
	bool using_boot_rom = false;

	memory_map memory;
};