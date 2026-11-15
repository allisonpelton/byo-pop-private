# Rocket Mania Deluxe

This is a new build of Rocket Mania Deluxe, the 2003 puzzle game by PopCap Games and Nuclide, that runs on current
Windows. It was rebuilt from the original program and plays the same way. It comes without any of the game's own
files. You need your own copy of the game: this program uses its pictures, sounds, music and texts.

It comes from the byo-pop project, https://github.com/allisonpelton/byo-pop, where questions and bug reports go.

## What you need

- Windows 10 or 11.
- Your own copy of Rocket Mania Deluxe. It was checked with the Steam release and with the copy on the PopCap Game
  Collection disc. With another version of the game some texts may be missing.

## Setting it up

1. Download the ZIP from the Releases page and unzip it, for example into Documents. It makes a folder called
   byo-pop-rocketmania. Do not unzip it into the game's own folder. This program's rocketmania.exe has the same name
   as the game's RocketMania.exe and would replace it.
2. Open your copy's folder. In Steam, right-click the game in your library and choose Manage, then Browse local files.
   A copy installed from a disc or a download is usually in C:\Program Files (x86)\PopCap Games\Rocket Mania Deluxe.
3. From that folder, copy the folders images, data, sounds, music and properties, and the file RocketMania.exe, into
   the folder byo-pop-rocketmania\assets\rocketmania. The program reads the game's texts from RocketMania.exe. It
   never starts it.
4. Start rocketmania.exe in the byo-pop-rocketmania folder.

If the game is installed in Program Files\PopCap Games\Rocket Mania Deluxe, the program finds it there and you can
skip step 3. You can also point it at any folder that holds the game's files, for example in a shortcut:

    rocketmania.exe -gamedir="D:\Games\Rocket Mania Deluxe"

## Your saves

The program keeps its players, games in progress, high scores and settings in
%APPDATA%\byo-pop\Rocket Mania Deluxe. It never changes the original game's own files.

To bring over the high scores, settings and games in progress of the original game installed on this computer, run
this once with the game's install folder. It copies them and closes without starting the game:

    rocketmania.exe -importsaves="C:\Program Files (x86)\Steam\steamapps\common\Rocket Mania Deluxe"

Anything the import replaces is copied first into a before-import folder next to the saves, and import-report.txt
says what was brought over.

## How to play

Each tile on the board carries a piece of fuse. Turn the tiles so that a fuse runs from a lit match on the left side of
the board to one or more rockets on the right. When the path is complete it burns, the rockets go up, and new tiles fall
in to take the place of the burnt ones. Launching several rockets with one fuse scores more. Coins, gems, bombs, clocks
and extra matches turn up on some tiles as you go further.

The main menu has three games, each with three levels of difficulty:

- Classic: launch enough rockets to finish each level before the night is over.
- Arcade: new tiles keep falling in from the top. The game ends when they pile up to the top of the board.
- Strategy: there is no clock, but you have only so many matches.

Between levels the coins you collected buy upgrades for your rocket launchers, five coins for each step. Once every
launcher is full, coins turn into points instead. Before your first game of each kind a short hint explains how it
works. You can switch the hints off, or show them again, in Options.

## Controls

- Left-click a tile to turn it a quarter turn. Right-click turns it the other way.
- In Arcade, drag a tile sideways to move it to the next column if there is room, and click the strip at the top of the
  board to make the next tiles drop at once.
- Space pauses the game and resumes it. A click on the board also resumes.
- Alt+Enter switches between a window and full screen. Options has the same choice, along with the music and sound
  volumes.

## Building from source

The build makes a 32-bit Windows program with llvm-mingw. You need CMake 3.20 or later, Ninja, an llvm-mingw
toolchain (i686 target) and the SDL2 development package for MinGW (2.32.x), whose `i686-w64-mingw32` folder holds
the CMake files. Point `LLVM_MINGW_ROOT` at the toolchain folder, then from this folder:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-i686.cmake \
      -DSDL2_DIR=<SDL2>/i686-w64-mingw32/lib/cmake/SDL2
    cmake --build build --target rocketmania

`build/rocketmania.exe` and `SDL2.dll`, the one DLL it needs (the C++ runtime is linked in), are in `build/`. To
copy the game's files into `assets/rocketmania/`, where the program finds them when it runs from `build/`:

    python games/rocketmania/tools/stage_rocketmania_assets.py "<your Rocket Mania Deluxe folder>"

Without a folder the script looks in Steam's usual library folders.

The code is a translation of the original's game classes into C++, on a portable copy of PopCap's SexyAppFramework with
SDL2 under it. `games/rocketmania/docs/FRAMEWORK_2003.md` describes the 2003 version of that framework, which the game
was built on, and how the port maps it.
