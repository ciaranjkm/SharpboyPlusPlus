#include "src/Emulator/Emulator.h"

//TODO REMOVE HEADLESS MODE 
//REQUIRE SDL TO RUN

int main(int argc, char* argv[]) {
	const std::string art = R"ART(+----------------------------------------------------------------------------------------------+
|     ______     __  __     ______     ______     ______   ______     ______     __  __        |
|    /\  ___\   /\ \_\ \   /\  __ \   /\  == \   /\  == \ /\  == \   /\  __ \   /\ \_\ \       |
|    \ \___  \  \ \  __ \  \ \  __ \  \ \  __<   \ \  _-/ \ \  __<   \ \ \/\ \  \ \____ \      |
|     \/\_____\  \ \_\ \_\  \ \_\ \_\  \ \_\ \_\  \ \_\    \ \_____\  \ \_____\  \/\_____\     |
|      \/_____/   \/_/\/_/   \/_/\/_/   \/_/ /_/   \/_/     \/_____/   \/_____/   \/_____/     |
|                                                                                              |
+----------------------------------------------------------------------------------------------+
)ART";

	std::cout << art << std::endl;

	bool using_SDL = true;
	bool single_step_mode = false;
	bool using_boot_rom = true;
	
	std::shared_ptr<Emulator> e = std::make_shared<Emulator>();
	if (e == nullptr) {
		return -1;
	}

	e->set_emu_pointer(e);
	e->initialise_SDL();
	e->run_emulator();
	e->close_emulator();
}