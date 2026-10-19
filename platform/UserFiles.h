#ifndef __PLATFORM_USERFILES_H__
#define __PLATFORM_USERFILES_H__

// Files in the user dir (VFS::GetUserDir()) that the port writes outside the game's own save code:
// mods.json, mod-settings.json and mods-status.json (platform/ModManager.h). Every write goes through a
// temporary file and an atomic replace, so a crash or a second process never sees half a file, and it is
// reported as a user data change: the web build syncs IndexedDB and marks its localStorage backup out of
// date (SexyAppBase installs the callback that does), so the change survives a page that is killed.
//
// Windows paths are UTF-8 (SDL's argv and SDL_GetPrefPath) and these functions open them with the wide
// API. Elsewhere they use the C library directly.

#include <string>

namespace Platform
{

// Reads a whole file (a host path). Returns false if it does not exist or cannot be read.
bool ReadHostFile(const std::string& thePath, std::string* theData);

// Writes theData to <user dir>/<theRelPath>: <name>.tmp first, then replaces the file in one step. Then
// UserDataChanged(). Returns false (and leaves the old file) on failure.
bool WriteUserFileAtomic(const std::string& theRelPath, const std::string& theData);

// Renames <user dir>/<theFrom> to <user dir>/<theTo>, replacing it; then UserDataChanged().
bool RenameUserFile(const std::string& theFrom, const std::string& theTo);

// Removes <user dir>/<theRelPath> if it exists; then UserDataChanged(). Returns false if nothing was removed.
bool RemoveUserFile(const std::string& theRelPath);

// Called after every change to the user dir made here (the running app marks its backup out of date).
void SetUserDataChangedCallback(void (*theCallback)());

// Syncs the user dir and runs the callback. Web without a running app (before callMain): updates the
// localStorage backup at once, since nothing else will.
void UserDataChanged();

}

#endif
