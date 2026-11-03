// Sexy::LoadGLFuncs (GLFuncs.h).
#include "GLFuncs.h"

#include <SDL.h>

namespace Sexy
{

GLFuncs gGL;

bool LoadGLFuncs()
{
	bool aComplete = true;
#define BYOPOP_GL_LOAD(ret, name, args) \
	gGL.name = reinterpret_cast<ret (BYOPOP_GLAPI*) args>(SDL_GL_GetProcAddress(#name)); \
	if (gGL.name == nullptr) { \
		SDL_Log("GL: %s is missing", #name); \
		aComplete = false; \
	}
	BYOPOP_GL_FUNCS(BYOPOP_GL_LOAD)
#undef BYOPOP_GL_LOAD
	return aComplete;
}

}  // namespace Sexy
