# tnfs-1994
The Need for Speed (1994) 3DO Recreation

This project is a reimplementation of the classic racing simulator **The Need for Speed**, originally released for the **3DO console in 1994**.
The goal is to recreate the original game look and feel, and make it natively playable on modern PCs with SDL2 and improved graphics.

> ⚠️ To play the game, you must own a legal copy of the original **3DO CD-ROM**. The assets from the disc are required and are not distributed with this project.

## Features
- Faithful recreation of the original 3DO version gameplay.
- SDL2/OpenGL 3D and Audio in pure C without any additional libraries.

Missing features: FMV playback, Replays and Highlights, Best times records save.

## How to play
1. clone/download repo
2. install `libsdl2-dev`
3. `make`
4. extract disc contents: `DriveData` and `frontend` folders, to the project's `assets` folder. Recommended tool: [3dt](https://github.com/trapexit/3dt)
5. run `./build/tnfs`

Use the keys to drive:
* Up/Down - Accel and brake
* Left/Right - Steer
* Space - Handbrake
* A/Z - Change gears
* C - Change camera
* H - Horn
* D - Crash cars
* R - Reset car
* F9 - Toggle dashboard
* F12 - Free camera/viewing mode

## Resources/Tools used
* https://3dodev.com/documentation/file_formats/games/nfs
* https://github.com/trapexit/3it
* https://github.com/trapexit/3dt
* https://github.com/AndyGura/nfs-resources-converter
* https://ghidra-sre.org/

