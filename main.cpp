#include "src/Emulator/Emulator.h"

int main(int argc, char* argv[]) {
	bool using_SDL = true;
	bool single_step_mode = false;
	bool using_boot_rom = true;
	
	if (!using_SDL) {
		std::shared_ptr<Emulator> e = std::make_shared<Emulator>(single_step_mode, using_boot_rom);
		if (!e->set_emu_pointer(e)) {
			return -1;
		}

		if (e->initialise_emu_instance("roms/pop_timing.gb") < 0) {
			return -1;
		}

		e->run_emulator();
		e->close_emulator();
		return 0;
	}

	std::shared_ptr<Emulator> e = std::make_shared<Emulator>(using_SDL);
	if (!e->set_emu_pointer(e)) {
		return -1;
	}

	if (!e->initialise_SDL()) {
		return -1;
	}
	//temporary for testing, move to imgui button action and check for running emus first, work on ppu needed before this!
	e->initialise_emu_instance("roms/PASSED/intr_timing.gb");
	e->run_emulator();
	e->close_emulator();
	e = nullptr;
}