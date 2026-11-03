#ifndef __PLATFORM_SAVEIMPORT_H__
#define __PLATFORM_SAVEIMPORT_H__

// -importsaves[=<install folder>]: brings the saves, high scores and settings of the original game, installed on this computer, into the
// port's user dir (docs/WEB_PLAYER.md, "Importing saves from an installed game"). Each game's main.cpp calls this once its user dir and game
// folder are known; with the option the import runs and the game then exits without starting; without it nothing happens at all.
//
// What is taken is only what the table lists for the game (platform/saveimport.json, compiled in): the original's registry values
// (read only: the real machine key in its 32-bit view, the VirtualStore copy that Windows made for a game that was not run as
// administrator, and the user's key, merged value by value, the VirtualStore first) and its save files (from the install folder, the
// VirtualStore copy of it and C:\ProgramData\PopCap Games\<game>). They become lines of the port's registry file (registry.ini) and
// files of the user dir. Whatever the import is about to replace is copied first to <user dir>/before-import-<date>/, and
// <user dir>/import-report.txt says what was done. The original's registry and folders are never written.
//
// Elsewhere than on Windows the option reports that it is not available.

#include <string>

namespace Platform
{

// theGameId: the game's id in the table ("bigmoney"). theGameDir: the install folder the game found (may be empty); -importsaves=<folder>
// replaces it. Returns true when the option was given: the game must then exit with *theExitCode (0: done or nothing to do, 1: a file could
// not be written, 2: the import could not run).
bool RunSaveImportIfRequested(int argc, char** argv, const char* theGameId, const std::string& theGameDir, const std::string& theUserDir, int* theExitCode);

}

#endif
