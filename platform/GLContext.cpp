// Sexy::GLContext (GLContext.h).
#include "GLContext.h"
#include "GLFuncs.h"

namespace Sexy
{

static SDL_GLContext TryContext(SDL_Window* theWindow, int theProfile, int theMajor, int theMinor)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, theProfile);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, theMajor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, theMinor);
	// The frame is drawn into framebuffers of the GPU path's own; the window's buffer only takes the present quad
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	return SDL_GL_CreateContext(theWindow);
}

GLContext* GLContext::Create(SDL_Window* theWindow)
{
	if (theWindow == nullptr || (SDL_GetWindowFlags(theWindow) & SDL_WINDOW_OPENGL) == 0) {
		SDL_Log("GL: the window was not made for OpenGL");
		return nullptr;
	}
	bool isES = true;
	SDL_GLContext aContext = TryContext(theWindow, SDL_GL_CONTEXT_PROFILE_ES, 3, 0);
#ifndef __EMSCRIPTEN__
	if (aContext == nullptr) {
		SDL_Log("GL: no ES 3.0 context (%s), trying GL 3.3 core", SDL_GetError());
		isES = false;
		aContext = TryContext(theWindow, SDL_GL_CONTEXT_PROFILE_CORE, 3, 3);
	}
#endif
	if (aContext == nullptr) {
		SDL_Log("GL: no context could be made: %s", SDL_GetError());
		return nullptr;
	}
	SDL_GL_MakeCurrent(theWindow, aContext);
	if (!LoadGLFuncs()) {
		SDL_GL_DeleteContext(aContext);
		return nullptr;
	}
	GLContext* aResult = new GLContext();
	aResult->mWindow = theWindow;
	aResult->mContext = aContext;
	aResult->mIsES = isES;
	aResult->mDescription = std::string(isES ? "ES 3.0" : "GL 3.3 core") + ": " +
		reinterpret_cast<const char*>(gGL.glGetString(GL_RENDERER)) + ", " +
		reinterpret_cast<const char*>(gGL.glGetString(GL_VERSION));
	SDL_Log("GL: %s", aResult->mDescription.c_str());
	return aResult;
}

GLContext::~GLContext()
{
	if (mContext != nullptr)
		SDL_GL_DeleteContext(mContext);
}

std::string GLContext::ShaderHeader() const
{
	if (mIsES)
		return "#version 300 es\nprecision highp float;\nprecision highp int;\n";
	return "#version 330 core\n";
}

void GLContext::MakeCurrent()
{
	SDL_GL_MakeCurrent(mWindow, mContext);
}

void GLContext::Swap()
{
	SDL_GL_SwapWindow(mWindow);
}

}  // namespace Sexy
