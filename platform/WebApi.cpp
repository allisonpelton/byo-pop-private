// The C functions a game's web page calls (Module.ccall), before callMain and while the game runs. They are
// linked through -sEXPORTED_FUNCTIONS (cmake/ByopopWeb.cmake): this file is part of the sexyframework static
// library, from which EMSCRIPTEN_KEEPALIVE alone would not pull them. Web only.

#ifdef __EMSCRIPTEN__
#include "WebPersist.h"

#include <emscripten.h>
#include <string>

extern "C"
{

// At boot, before the page reads the user dir (mods.json): brings back the files of the localStorage backup
// that are newer than the IndexedDB copies (Platform::RestoreUserDataBackup). main() restores again, which
// then changes nothing: restored files take the backup's times.
EMSCRIPTEN_KEEPALIVE void byopop_userdata_restore(const char* theUserDir)
{
	Platform::RestoreUserDataBackup(theUserDir ? std::string(theUserDir) : std::string());
}

}
#endif
