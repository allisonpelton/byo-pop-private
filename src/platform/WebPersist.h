#ifndef __PLATFORM_WEBPERSIST_H__
#define __PLATFORM_WEBPERSIST_H__

// Web: keeps the user dir (saves, settings, mods.json) across page loads. The page mounts it on
// IDBFS, which is saved to IndexedDB asynchronously: a sync started as the page unloads never
// completes, and a page that is killed (an embedding app closing its web view, a crashed tab) gets no
// unload events at all. So the user dir is also backed up to localStorage, synchronously, as it would
// be after a shutdown at that moment, and the backup is restored at the next start. The browser
// commits localStorage to disk shortly after (Chromium: within about 5-60 s, and at once when a tab
// closes or Chrome for Android goes to the background), so only a crash of the whole browser can bring
// back a game up to about a minute behind. Restore orders the copies by file time, so a system clock
// moved backwards can make it keep the older one. web/shell.html lets only one page use the storage
// at a time. Natively these are no-ops: files are written directly.

#include <string>
#include <vector>

namespace Platform
{

// A file as it would be written now, in place of the one at mRelPath (relative to the user dir)
struct BackupFile
{
	std::string mRelPath;
	std::string mData;
};

// At startup, before anything reads the user dir: brings back the files of the backup that are newer
// than the IndexedDB copies, and removes files that had been deleted by the time it was taken.
void RestoreUserDataBackup(const std::string& theUserDir);

// Replaces the backup with the user dir as it is now, with theFiles in place of the files at their
// paths (a file that is already there with the same bytes keeps its own time).
void BackupUserData(const std::vector<BackupFile>& theFiles);

// Saves the user dir to IndexedDB in the background. Requests made while a sync is running are
// coalesced into one follow-up sync. Module.sexySyncBusy / sexySyncPending tell the page whether a
// save is still being written.
void SyncUserData();

}

#endif
