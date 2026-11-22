// NotTranslated.cpp: see NotTranslated.h.

#include "NotTranslated.h"

#include <SDL.h>
#include <cstdlib>

[[noreturn]] void Sexy::NotTranslated(const char* theWhat, const char* theFile, int theLine)
{
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Not translated yet: %s (%s:%d)", theWhat, theFile, theLine);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Peggle", theWhat, nullptr);
	abort();
}
