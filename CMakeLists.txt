# CMakeList.txt : CMake project for SharpboyPlusPlus, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.14)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("SharpboyPlusPlus")

include(FetchContent)

#fetch sdl3
FetchContent_Declare(
	SDL3
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG release-3.2.x
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

if(NOT SDL3_FOUND)
    message(STATUS "getting sdl from github...")
    FetchContent_MakeAvailable(SDL3)
else()
    message(STATUS "using local sdl3")
endif()

# Fetch ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(ImGui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(ImGui PUBLIC  
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(ImGui PUBLIC SDL3::SDL3-static)

# Add source to this project's executable.
add_executable (SharpboyPlusPlus "main.cpp" "src/externals/nlohmann/json.hpp" "src/emulator/Emulator.h" "src/emulator/Emulator.cpp" "src/emulator/CPU.h" "src/emulator/CPU.cpp" "src/emulator/MMU.h" "src/emulator/MMU.cpp" "src/emulator/Instruction_definitions.cpp" "src/emulator/Timers.h" "src/emulator/Timers.cpp" "src/emulator/PPU.h" "src/emulator/PPU.cpp"  "src/emulator/emu_visuals/Graphics.h" "src/emulator/emu_visuals/Graphics.cpp" "src/Application.h" "src/Application.cpp")

target_link_libraries(SharpboyPlusPlus PRIVATE ImGui SDL3::SDL3-static)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SharpboyPlusPlus PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
