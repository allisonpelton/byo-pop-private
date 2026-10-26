// Test hooks: a screen larger than the original's (-canvas=<width>x<height>), to see what the game draws beyond
// 640x480. The configuration places some things off the screen (the timed mode's button on the new game screen, its
// high score rows on the main menu and the high scores screen, all at 1000); the game sizes its screen layers to the
// display, so with a larger one it draws them. The window still shows the top left 640x480, where the clicks go.
//
// Nothing clears the screen beyond the backgrounds, so what earlier screens drew there stays. The -script command
// canvasshot <file> therefore blanks everything outside 640x480, makes every layer redraw in the next frame and saves
// the whole screen as a PNG after it: only the screen in play is in the picture.
//
//   tiptop.exe -canvas=1280x1200 -script=<file>     with lines like: 600 canvasshot shots/menu.png

#include "TipTopApp.h"
#include "Gui.h"
#include "Lowos.h"

#include <SDL.h>
#include <stb_image_write.h>

#include <cstdint>
#include <vector>

using namespace Sexy;

// The -script command canvasshot <file> (Script.cpp)
void TipTopApp::StartCanvasShot(const std::string& theFile)
{
	int aWidth, aHeight;
	lowos::HostScreenSize(&aWidth, &aHeight);
	uint32_t* aScreen = lowos::HostScreen();
	for (int y = 0; y < aHeight; y++)
		for (int x = (y < 480) ? 640 : 0; x < aWidth; x++)
			aScreen[y * aWidth + x] = 0;
	tiptop::MarkAllLayersDirty();
	mCanvasShot = theFile;
}

// After the frame that redrew everything (RunFrame)
void TipTopApp::SaveCanvasShot()
{
	int aWidth, aHeight;
	lowos::HostScreenSize(&aWidth, &aHeight);
	const uint32_t* aScreen = lowos::HostScreen();
	std::vector<uint8_t> aRGBA((size_t)aWidth * (size_t)aHeight * 4);
	for (size_t i = 0; i < (size_t)aWidth * (size_t)aHeight; i++)
	{
		aRGBA[i * 4 + 0] = (uint8_t)(aScreen[i] >> 16);
		aRGBA[i * 4 + 1] = (uint8_t)(aScreen[i] >> 8);
		aRGBA[i * 4 + 2] = (uint8_t)aScreen[i];
		aRGBA[i * 4 + 3] = 0xff;
	}
	if (stbi_write_png(mCanvasShot.c_str(), aWidth, aHeight, 4, aRGBA.data(), aWidth * 4) != 0)
		SDL_Log("canvasshot: %s (%dx%d, update %d)", mCanvasShot.c_str(), aWidth, aHeight, mUpdateCount);
	else
		SDL_Log("canvasshot: cannot write %s", mCanvasShot.c_str());
	mCanvasShot.clear();
}
