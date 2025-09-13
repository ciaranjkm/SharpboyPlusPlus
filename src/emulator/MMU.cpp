#include "MMU.h"
#include "Emulator.h"

MMU::MMU(std::shared_ptr<Emulator> emulator) {
	if (emulator == nullptr) {
		return;
	}

	this->emulator = emulator;
}

MMU::~MMU() {
	this->emulator = nullptr;
	printf("[SB] Shutting down MMU object\n");
}

//public

const bool MMU::is_mmu_initialised() {
	return &initialised;
}

void MMU::reset_mmu(const rom_header& header, const std::vector<byte>& rom, const std::array<byte, 0x100>& boot_rom) {
	using_boot_rom = emulator->is_using_boot_rom();
	
	//use header to setup memory and set flags for mbc controllers
	
	//just 32kb test roms for now
	memory.cartridge = rom;
	memory.boot_rom = boot_rom;

	if (using_boot_rom) {
		swap_cartridge_and_boot_roms(); //swap first 256 bytes to allow boot rom to run first, then swap back when bank written to and delete boot rom
		return;
	}

	memory.io.JOYP = 0xcf;
	memory.io.IF = 0xe1;
	memory.io.BANK = 0x01;
}

//read/write with blocking of oam when dma or when ppu is active
byte MMU::read_from_memory(const ushort& address) {
	//printf("[SB] MMU read at %04X\n", address);
	
	if (address == 0xffff) {
		return memory.IE;
	}
	else if (address >= 0x0000 && address < 0x8000) {
		return memory.cartridge[address];
	}
	else if (address >= 0xa000 && address < 0xc000) {
		return 0xff;
		return memory.eram[address];
	}
	else if (address >= 0xc000 && address < 0xe000) {
		return memory.wram[(ushort)(address - 0xc000)];
	}
	else if (address >= 0xe000 && address < 0xfe00) {
		return read_from_memory((ushort)(address - 0x2000));
	}
	else if (address >= 0xfea0 && address < 0xff00) {
		return 0xff; //not usable
	}
	else if (address >= 0xff00 && address < 0xff80) {
		return read_io((io_addresses)(address & 0xff));
	}
	else if (address >= 0xff80 && address < 0xffff) {
		return memory.hram[(byte)(address - 0xff80)];
	}

	//printf("[DEBUG]:: Unkown read address: 0x%04X | data returned: 0xff\n", address);
	return 0xff;
}

void MMU::write_to_memory(const ushort& address, const byte& value) {
	//TESTING FOR BLARGGS
	if (address == 0xff02 && value == 0x81) {
		byte data = read_io(io_SB);
		printf("%c", data);
		write_io(io_SB, 0x00);
		return;
	}

	if (address == 0xffff) {
		memory.IE = value;
		return;
	}
	else if (address >= 0x0000 && address < 0x8000) {
		//rom writes for bank switching when enabled
		//memory.cartridge[address] = value;
		return;
	}
	else if (address >= 0xa000 && address < 0xc000) {
		//disabled external ram by default
		//memory.eram[(ushort)(address - 0xa000)] = value;
		return;
	}
	else if (address >= 0xc000 && address < 0xe000) {
		memory.wram[(ushort)(address - 0xc000)] = value;
		return;
	}
	else if (address >= 0xe000 && address < 0xfe00) {
		//echo (mirror of 0xc000-0xddff)
		write_to_memory((ushort)(address - 0x2000), value);
		return;
	}
	else if (address >= 0xfea0 && address < 0xff00) {
		return; //not usable;
	}
	else if (address >= 0xff00 && address < 0xff80) {
		write_io((io_addresses)(address - 0xff00), value);
		return;
	}
	else if (address >= 0xff80 && address < 0xffff) {
		memory.hram[(ushort)(address - 0xff80)] = value;
		return;
	}

	return;
}

byte MMU::read_io(const byte& io_target) {
	switch (io_target) {
	case io_JOYP: return emulator->read_joypad_state();
	case io_true_JOYP: return memory.io.JOYP;
	case io_SB: return memory.io.SB;
	case io_SC: return memory.io.SC;
	case io_IF: return memory.io.IF;
	case io_DMA: return 0xff;
	case io_BANK: return memory.io.BANK;
	};
	return 0xff; //bogus read	
}

void MMU::write_io(const byte& io_target, const byte& value) {
	switch (io_target) {
	case io_JOYP: memory.io.JOYP = value; return;
	case io_SB: memory.io.SB = value; return;
	case io_SC: memory.io.SC = value; return;
	case io_IF: memory.io.IF = value | 0xe0; return;

	case io_BANK:
		if (value == 0x01) {
			if (using_boot_rom) {
				swap_cartridge_and_boot_roms();
				using_boot_rom = false;
				return;
			}
		}

		memory.io.BANK = value;
		return;
	};
}

void MMU::swap_cartridge_and_boot_roms() {
	//copy first bytes from rom to buffer
	std::array<byte, 0x100> first_bytes = std::array<byte, 0x100>();
	for (int x = 0; x < first_bytes.size(); x++) {
		first_bytes[x] = memory.cartridge[x];
	}

	//copy boot rom into cartridge, then copy buffer into rom
	for (int x = 0; x < first_bytes.size(); x++) {
		memory.cartridge[x] = memory.boot_rom[x];
		memory.boot_rom[x] = first_bytes[x];
	}
}