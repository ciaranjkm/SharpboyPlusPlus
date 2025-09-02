# Sharpboy++ 
### A Gameboy (DMG) emulator in C++ with SDL and ImGui
This is my first attempt at a major project in C++, trying to learn as much as possible whilst working on it. It's currently a w.i.p loading boot rom and some games with limited functionality with 32Kb ROM sizes and no gui. Currently passing all of mooneye's test roms for timing, cpu instructions, interrupts and OAM DMA.

## Overview
The emulator is a DMG emulator emulating the original GameBoy. It currently supports only background rending and no input. You can make a folder called <code> roms/ </code> to place your .gb files or run the program and try refreshing roms and it will create one for you. Also create a folder called <code> boot/ </code> and upload your own boot.bin file it can run the boot rom for you! (TODO: make this file creation automatic). 

### Controls
You can open/close the menu by pressing the <code> space </code> key.

## Building 
This project uses cmake and requires SDL3 and ImGui to build correctly and currently supports only Windows at the moment (it could support MacOS and Linux, I haven't checked and am unable to). I made the project grab SDL3 from GitHub if it hasn't found it in the PATH, ImGui is always grabbed GitHub as its not big and I wanted to use the docking branch. To build the project run the following commands in the source directory:

```
mkdir build
cd build
cmake ..
```

## Screenshots
<img src="https://i.imgur.com/FSRMmRo.png" alt="Image 1" width="300" height="275">     <img src="https://i.imgur.com/1PIV4VB.png" alt="Image 2" width="300" height="275">
<img src="https://i.imgur.com/jCv7FTa.png" alt="Image 3" width="300" height="275">     <img src="https://i.imgur.com/C8d67el.png" alt="Image 4" width="300" height="275">

## Features
#### Implemented Features:
- M cycle accurate emulation
- GUI
- Start / Stop / Pause emulator
- Many many more...
  
#### Features To Implement:
- Audio
- Eventually syncing to audio sampling
- Input (custom keybinds)
- MBC implementation
