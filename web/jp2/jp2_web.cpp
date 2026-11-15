/* The desktop themes' JPEG 2000 decoder (web/gamepics.js, web/themes.js; docs/WEB_PLAYER.md, "Desktop themes"): built for
   the web as jp2.js and jp2.wasm beside the pages, which the listing page loads only when a theme whose wallpaper is a .jp2
   (Bejeweled Twist's) is prepared. It is the framework's own decoder (platform/openjpeg/JP2Decoder.cpp, OpenJPEG under it),
   compiled in here as it is, without the framework: its image-format registration and its log line are stood in for below.
   No game code and no game files are in it. */
#include "../../platform/openjpeg/JP2Decoder.cpp"
#include <emscripten.h>

/* the framework's parts that JP2Decoder.cpp names but this module never calls */
namespace Platform
{
void RegisterImageFormat(const char*, ImageDetector, ImageDecoder) {}
}
extern "C" void SDL_Log(const char*, ...) {}

namespace
{
std::vector<uint32_t> sPixels;
int sWidth = 0, sHeight = 0;
}

extern "C" {

/* Decodes theSize bytes of a .jp2 (or .j2k) file: 1 when done, its pixels then jp2_pixels() (ARGB, a uint32 each, rows top to
   bottom), jp2_width() by jp2_height(); 0 when the file cannot be decoded */
EMSCRIPTEN_KEEPALIVE int jp2_decode(const uint8_t* theData, int theSize)
{
	std::vector<uint8_t> aData(theData, theData + theSize);
	std::string anErrors;
	sPixels.clear();
	sWidth = sHeight = 0;
	if (theSize <= 0 || !Platform::Decode(aData, sWidth, sHeight, sPixels, nullptr, anErrors))
	{
		sPixels.clear();
		sWidth = sHeight = 0;
		return 0;
	}
	return 1;
}
EMSCRIPTEN_KEEPALIVE int jp2_width() { return sWidth; }
EMSCRIPTEN_KEEPALIVE int jp2_height() { return sHeight; }
EMSCRIPTEN_KEEPALIVE const uint32_t* jp2_pixels() { return sPixels.data(); }
/* frees the last picture's pixels */
EMSCRIPTEN_KEEPALIVE void jp2_release()
{
	std::vector<uint32_t>().swap(sPixels);
	sWidth = sHeight = 0;
}

}
