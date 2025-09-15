#pragma once

#include <cstdint>
#include <string>

typedef uint8_t byte;
typedef uint16_t ushort;
typedef int8_t sbyte;

enum interrupt_types {
	int_VBLANK = 0,
	int_LCD = 1,
	int_TIMER = 2,
	int_SERIAL = 3,
	int_JOYPAD = 4,
};

enum joypad_buttons {
	button_up,
	button_down,
	button_left,
	button_right,
	button_a,
	button_b,
	button_start,
	button_select,
};

enum cartridge_types {
	int_NONE = 0xff,
	ROM_ONLY = 0x00,
	MBC1 = 0x01,
	MBC1_RAM = 0x02,
	MBC1_RAM_BATT = 0x03,
	MBC2 = 0x05,
	MBC2_BATT = 0x06,
	ROM_BATT = 0x08,
	ROM_RAM_BATT = 0x09,
	MBC3 = 0x11,
	MBC3_TIMER_BATT = 0x10,
	MBC3_RAM = 0x12,
	MBC3_RAM_BATT = 0x13,
	MBC5 = 0x19,
	MBC5_RAM = 0x1a,
	MBC5_RAM_BATT = 0x1b,
};

enum cart_rom_sizes {
	rom_NONE = 0xff,
	rom_KiB_32 = 0x00,
	rom_KiB_64 = 0x01,
	rom_KiB_128 = 0x02,
	rom_KiB_256 = 0x03,
	rom_KiB_512 = 0x04,
	rom_MiB_1 = 0x05,
	rom_MiB_2 = 0x06,
	rom_MiB_4 = 0x07,
	rom_MiB_8 = 0x08
};

enum cart_ram_sizes {
	ram_INVALID = 0xff,
	ram_NONE = 0x00,
	ram_UNUSED = 0x01,
	ram_KiB_8 = 0x02,
	ram_KiB_32 = 0x03,
	ram_KiB_128 = 0x04,
	ram_KiB_64 = 0x05
};

struct rom_header {
	std::string name = "";
	cartridge_types cartridge_type = ROM_ONLY;
	cart_rom_sizes cart_rom_size = rom_KiB_32;
	cart_ram_sizes cart_ram_size = ram_INVALID;
	byte checksum_byte = 0x00;
};

struct joypad_state {
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool a = false;
	bool b = false;
	bool start = false;
	bool select = false;
};
