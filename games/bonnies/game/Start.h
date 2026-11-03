#ifndef __BONNIES_START_H__
#define __BONNIES_START_H__

// What the port's host calls to start the game (src/main.cpp, src/BonniesHost.cpp), after loadExe and
// runStaticInitializers (engine/Exe.h). The host includes only this, Exe.h and Host.h: the engine's header names
// (Graphics.h, Widget.h, ...) are the framework's too.

namespace bonnies
{

// Dictionary.h, Book.h: the tables copied from the exe
void loadLetterOdds();
void loadBooks();
// BonniesApp.h: 0x401000 WinMain's game
int bonniesMain();

} // namespace bonnies

#endif
