#include "src/Application.h"
#include <iostream>

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
	
	std::shared_ptr<Application> app = std::make_shared<Application>();
	
	if (!app->is_app_running()) {
		return -1;
	}

	app->set_application_pointer(app);
	app->run();
}