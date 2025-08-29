#include "MMU.h"
#include "Emulator.h"

MMU::MMU(std::shared_ptr<Emulator> emulator_ptr) {
	this->emulator_ptr = emulator_ptr;
	if (this->emulator_ptr != nullptr) {
		initialised = true;
	}
	else {
		initialised = false;
	}
}

MMU::~MMU() {
	this->emulator_ptr = nullptr;
	printf("[SB] Shutting down MMU object\n");
}

//public

const bool MMU::is_mmu_initialised() {
	return &initialised;
}

void MMU::reset_mmu(const rom_header& header, const std::vector<byte>& rom, const std::array<byte, 0x100>& boot_rom) {
	this->using_boot_rom = emulator_ptr->is_using_boot_rom();
	
	//use header to setup memory and set flags for mbc controllers
	
	//just 32kb test roms for now
	memory.cartridge = rom;
	memory.boot_rom = boot_rom;

	if (this->using_boot_rom) {
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
	else if (address >= 0x8000 && address < 0xa000) {
		//block reads to vram from cpu when ppu is not in vblank
		//ppu_modes ppu_state = emulator_ptr->get_current_ppu_mode();
		//if (ppu_state == ppu_VBLANK || ppu_state == ppu_HBLANK) {
			return memory.vram[(ushort)(address - 0x8000)];
		//}
		//else {
			return 0xff;
		//}
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
	else if (address >= 0xfe00 && address < 0xfea0) {
		if (dma_active) {
			return 0xff;
		}

		return memory.oam[(ushort)(address - 0xfe00)];
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
	//printf("[SB] MMU write at %04X with value %02X\n", address, value);

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
		memory.cartridge[address] = value;
		return;
	}
	else if (address >= 0x8000 && address < 0xa000) {
		//block reads to vram from cpu when ppu is not in vblank
		//ppu_modes ppu_state = emulator_ptr->get_current_ppu_mode();
		//if (ppu_state == ppu_VBLANK || ppu_state == ppu_HBLANK) {
			memory.vram[(ushort)(address - 0x8000)] = value;
			return;
		//}

		return;
	}
	else if (address >= 0xa000 && address < 0xc000) {
		memory.eram[(ushort)(address - 0xa000)] = value;
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
	else if (address >= 0xfe00 && address < 0xfea0) {
		if (dma_active) {
			return;
		}

		memory.oam[(ushort)(address - 0xfe00)] = value;
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

	//printf("[DEBUG]:: Unknown write address: 0x%04X | value: 0x%02X\n", address, value);
	return;
}

//read/write without blocking of oam when dma or when ppu is active
byte MMU::unblocked_read(const ushort& address) {
	//printf("[SB] MMU read at %04X\n", address);

	if (address == 0xffff) {
		return memory.IE;
	}
	else if (address >= 0x0000 && address < 0x8000) {
		return memory.cartridge[address];
	}
	else if (address >= 0x8000 && address < 0xa000) {
		return memory.vram[(ushort)(address - 0x8000)];
	}
	else if (address >= 0xa000 && address < 0xc000) {
		return memory.eram[(ushort)(address - 0xa000)];
	}
	else if (address >= 0xc000 && address < 0xe000) {
		return memory.wram[(ushort)(address - 0xc000)];
	}
	else if (address >= 0xe000 && address < 0xfe00) {
		return read_from_memory((ushort)(address - 0x2000));
	}
	else if (address >= 0xfe00 && address < 0xfea0) {
		return memory.oam[(ushort)(address - 0xfe00)];
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

void MMU::unblocked_write(const ushort& address, const byte& value) {
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
		memory.cartridge[address] = value;
		return;
	}
	else if (address >= 0x8000 && address < 0xa000) {
		memory.vram[(ushort)(address - 0x8000)] = value;
		return;
	}
	else if (address >= 0xa000 && address < 0xc000) {
		memory.eram[(ushort)(address - 0xa000)] = value;
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
	else if (address >= 0xfe00 && address < 0xfea0) {
		memory.oam[(ushort)(address - 0xfe00)] = value;
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

	//printf("[DEBUG]:: Unknown write address: 0x%04X | value: 0x%02X\n", address, value);
	return;
}

byte MMU::read_io(const byte& io_target) {
	//should the io be for timer/ppu, redirect the read
	if (io_target >= io_DIV && io_target <= io_TAC) {
		return emulator_ptr->io_instant_read(io_target);
	}
	else if (io_target >= io_LCDC && io_target <= io_WX && io_target != io_DMA) {
		return emulator_ptr->io_instant_read(io_target);
	}
	else {
		switch (io_target) {
		case io_JOYP: return 0xff;
		case io_SB: return memory.io.SB;
		case io_SC: return memory.io.SC;
		case io_IF: return memory.io.IF;
		case io_DMA: return 0xff;
		case io_BANK: return memory.io.BANK;
		};
		return 0xff; //bogus read
	}	
}

void MMU::write_io(const byte& io_target, const byte& value) {
	//should our io be for the timer/ppu, redirect the write
	if (io_target >= io_DIV && io_target <= io_TAC) {
		emulator_ptr->io_instant_write(io_target, value);
		return;
	}
	else if (io_target >= io_LCDC && io_target <= io_WX && io_target != io_DMA) {
		emulator_ptr->io_instant_write(io_target, value);
		return;
	}
	else {
		switch (io_target) {
		case io_JOYP: return;
		case io_SB: memory.io.SB = value; return;
		case io_SC: memory.io.SC = value; return;
		case io_IF: memory.io.IF = value | 0xe0; return;

		//start new dma on dma write
		case io_DMA:
			memory.io.DMA = value;
			start_new_dma = true;
			dma_delay = DEFAULT_DMA_DELAY;
			return;

		//remove boot rom if value written is 0x1 and using boot rom is true
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

void MMU::dma_tick() {
	if (start_new_dma) {
		//tick down if we're starting a new dma
		dma_delay--;

		//if 1 m cycle has passed
		if (dma_delay == 0) {
			dma_active = true;
			start_new_dma = false;
			dma_address = (ushort)(memory.io.DMA << 8);
			dma_cycles = 0;
			dma_ticks_this_cycles = 0;
			dma_delay = 0;
			return;
		}
	}

	
	if (dma_active) {
		//increase ticks for dma
		dma_ticks_this_cycles++;

		//if one m cycle has passed
		if (dma_ticks_this_cycles == 4) {
			dma_ticks_this_cycles = 0;

			//increase total cycles for dma to find when to stop
			dma_cycles++;

			//read the value and put into oam
			byte dma_value = read_from_memory(dma_address++);
			memory.oam[dma_cycles - 1] = dma_value;
		}

		//if our total dma cycles is the total amount for a cycle, turn off dma and reset it
		if (dma_cycles == DMA_TOTAL_TICKS) {
			dma_active = false;
			dma_address = 0x0000;
			dma_cycles = 0;
			dma_ticks_this_cycles = 0;
			dma_delay = 0;
			total_dma_ticks = 0;
		}
	}
}
