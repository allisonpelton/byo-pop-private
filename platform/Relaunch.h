#ifndef __PLATFORM_RELAUNCH_H__
#define __PLATFORM_RELAUNCH_H__

// Starting the game again after a restart from the mod menu (docs/MOD_MENU_DESIGN.md 6.4). A game's main()
// ends with:
//
//   anApp->Shutdown();
//   std::vector<std::string> aRestartArgs;
//   bool restart = anApp->TakeRelaunch(&aRestartArgs);
//   delete anApp;
//   if (restart) Platform::Relaunch(aRestartArgs, "Bejeweled Deluxe");
//
// The web build never gets there: the page reloads instead (SexyAppBase, Module.onGameExit).

#include <string>
#include <vector>

namespace Platform
{

// Starts this program again with theArgs (UTF-8, as SDL's argv gives them), in the current directory and with
// no handles inherited, and returns without waiting. If that fails it says so in a message box ("Could not
// restart <theGameName>. Your game was saved; start it again."; in a test run only in the log) and returns
// false. Windows only; elsewhere it returns false.
bool Relaunch(const std::vector<std::string>& theArgs, const std::string& theGameName);

// Before the app shuts down to restart: lets the new process bring its window to the front (this one gives up
// the foreground as Shutdown hides its window). Windows only.
void AllowRelaunchForeground();

// The Windows command line for theArgs: each quoted as the C runtime (CommandLineToArgvW) splits it back
std::string QuoteCommandLineArg(const std::string& theArg);

}

#endif
