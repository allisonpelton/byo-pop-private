// The C functions a game's web page calls (Module.ccall), before callMain and while the game runs
// (docs/MOD_MENU_DESIGN.md 4.4, 4.5). web/modpanel.js, the page's Mods panel, is their user. They are linked
// through -sEXPORTED_FUNCTIONS (cmake/ByopopWeb.cmake): this file is part of the sexyframework static library,
// from which EMSCRIPTEN_KEEPALIVE alone would not pull them. The calls that need the running game
// (byopop_menu, byopop_restart) are in framework/ModMenu.cpp. Web only.
//
// Strings go in and out as UTF-8; a returned string stays valid until the next call into this file.

#ifdef __EMSCRIPTEN__
#include "ModManager.h"
#include "VFS.h"
#include "WebPersist.h"

#include <emscripten.h>
#include <string>

// Module.onModsChanged(), after every change of the mods model (an edit from the page or the in-game menu, a
// rescan). Posted, not called: the page's handler reads the model again, which must not happen in the middle of
// the edit that announced it. Several changes in a row make one call.
EM_JS(void, byopop_post_mods_changed, (), {
	if (Module['byopopModsChangedPosted'])
		return;
	Module['byopopModsChangedPosted'] = true;
	setTimeout(function() {
		Module['byopopModsChangedPosted'] = false;
		if (typeof Module['onModsChanged'] === 'function')
			Module['onModsChanged']();
	}, 0);
});

static std::string gWebApiResult;

static void OnModsChanged()
{
	byopop_post_mods_changed();
}

extern "C"
{

// At boot, before the page reads the user dir (mods.json): brings back the files of the localStorage backup
// that are newer than the IndexedDB copies (Platform::RestoreUserDataBackup). main() restores again, which
// then changes nothing: restored files take the backup's times.
EMSCRIPTEN_KEEPALIVE void byopop_userdata_restore(const char* theUserDir)
{
	Platform::RestoreUserDataBackup(theUserDir ? std::string(theUserDir) : std::string());
}

// After byopop_userdata_restore, before callMain: the user dir and the mods dir the game will be started with.
// Scans the mods on disk, mods.json and mod-settings.json (the desired state) without applying anything, so
// the page can show and edit them before the game runs; main() then sets the same folders and applies them.
// From here on the page hears of every change through Module.onModsChanged.
EMSCRIPTEN_KEEPALIVE void byopop_mods_open(const char* theUserDir, const char* theModsDir)
{
	Platform::VFS::Get().SetUserDir(theUserDir ? theUserDir : "");
	Platform::ModManager& aManager = Platform::ModManager::Get();
	aManager.SetModsDir(theModsDir ? theModsDir : "");
	aManager.SetChangeCallback(&OnModsChanged);
	aManager.ScanDesired();
}

// The mods model as JSON (ModManager::GetStateJson: the mods in load order, their state, errors, settings and
// what waits for a restart). Any time after byopop_mods_open.
EMSCRIPTEN_KEEPALIVE const char* byopop_mods_state()
{
	gWebApiResult = Platform::ModManager::Get().GetStateJson();
	return gWebApiResult.c_str();
}

// One edit: {"op": "enable"|"move"|"set"|"reset"|"forget", "mod": id, ...} (ModManager::Edit), written to the
// user dir at once. Returns "" or why nothing was changed.
EMSCRIPTEN_KEEPALIVE const char* byopop_mods_edit(const char* theJson)
{
	gWebApiResult = Platform::ModManager::Get().Edit(theJson ? theJson : "");
	return gWebApiResult.c_str();
}

// After the page added or removed a mod folder: scans the folders and both files again
EMSCRIPTEN_KEEPALIVE void byopop_mods_rescan()
{
	Platform::ModManager::Get().ScanDesired();
}

}
#endif
