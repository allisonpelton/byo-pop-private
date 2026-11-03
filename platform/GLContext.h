#ifndef __GLCONTEXT_H__
#define __GLCONTEXT_H__

// port: the OpenGL context of the GPU path (framework/GPUInterface.h): OpenGL ES 3.0 (WebGL2 in a browser, the driver's
// ES context natively), else GL 3.3 core. One per game; the window must have been made with SDL_WINDOW_OPENGL.

#include <SDL.h>

#include <string>

namespace Sexy
{

class GLContext
{
public:
	// Makes the context on theWindow, makes it current and loads the GL functions; null (with the reason logged) when
	// neither kind can be made
	static GLContext* Create(SDL_Window* theWindow);
	~GLContext();

	// ES 3.0 / WebGL2 (shaders take "#version 300 es"), or GL 3.3 core ("#version 330 core" and a VAO bound)
	bool IsES() const { return mIsES; }
	// The version line and precision a shader source starts with in this context
	std::string ShaderHeader() const;
	const std::string& Description() const { return mDescription; }

	void MakeCurrent();
	void Swap();

private:
	GLContext() = default;
	SDL_Window* mWindow = nullptr;
	SDL_GLContext mContext = nullptr;
	bool mIsES = false;
	std::string mDescription;
};

}  // namespace Sexy

#endif
