#ifndef __SPROUT_HOST_H__
#define __SPROUT_HOST_H__

// The port's host: what the engine's Windows layer (0x454a40 (wordharmony)-0x45ee00 (wordharmony)) got from Windows,
// DirectDraw and DirectSound. The app in src/ implements it over the shared framework and platform layer.

#include <cstdint>
#include <string>
#include <vector>

namespace engine
{

class Image;
class SoundSystem;

class Host
{
public:
	virtual ~Host() {}
	// timeGetTime()
	virtual uint32_t timeMs() = 0;
	// The signal build's clock (QueryPerformanceCounter and QueryPerformanceFrequency, the application's slots 0xac and
	// 0xb0 (pizzafrenzy)), which times its run loop's frames
	virtual uint64_t timeTicks() { return timeMs(); }
	virtual uint64_t ticksPerSecond() { return 1000; }
	// A file of the game's folder (WHArchive.saf, res/...), by its relative name with '/'
	virtual bool readGameFile(const std::string& theName, std::vector<uint8_t>& theData) = 0;
	virtual bool gameFileExists(const std::string& theName) = 0;
	// An image decoded from a PNG or JPEG file's bytes, with the alpha of a mask's bytes when theMask is not empty
	// (0x457510 (wordharmony)); null when it does not decode
	virtual Image* createImage(const std::vector<uint8_t>& theData, const std::vector<uint8_t>* theMask) = 0;
	// The registry values under HKEY_CURRENT_USER\Software\GameHouse\WordHarmony (0x421750 (wordharmony))
	virtual bool registryReadString(const std::string& theName, std::string& theValue) = 0;
	virtual bool registryWriteString(const std::string& theName, const std::string& theValue) = 0;
	virtual bool registryReadInt(const std::string& theName, int& theValue) = 0;
	virtual bool registryWriteInt(const std::string& theName, int theValue) = 0;
	// The signal build's registry keys (PersistKey slots 0x0c and 0x14 (pizzafrenzy)): the names of the keys one level
	// below a key, and of the values in it ("" for its default value). theKey is the name below the application's
	// registry key, "" for that key itself. False when the key is not there.
	virtual bool registryGetSubKeys(const std::string& theKey, std::vector<std::string>& theNames) { return false; }
	virtual bool registryGetValueNames(const std::string& theKey, std::vector<std::string>& theNames)
	{
		return false;
	}
	// port: the cells and the colours of an image the engine's image interface cannot copy. A copy of the part of an
	// image (the original's new image drawn over by the part); and a copy with every colour's hue, saturation and
	// value shifted by HsvShiftFilter (ImageFilter.h). Null when the host has no such copy.
	virtual Image* cropImage(Image* theImage, int theLeft, int theTop, int theWidth, int theHeight) { return nullptr; }
	virtual Image* shiftImageColors(Image* theImage, float theHue, float theSaturation, float theValue)
	{
		return nullptr;
	}
	// The window's full screen mode (App slots 0x04 and 0x08: the window's 0x456fa0 (wordharmony) and
	// 0x455b80 (wordharmony))
	virtual void setFullScreen(bool theFullScreen) = 0;
	virtual bool isFullScreen() = 0;
	// The program's end (App slot 0x70: PostQuitMessage())
	virtual void quit() = 0;
	// ShellExecute() of a link
	virtual void openURL(const std::string& theURL) = 0;
	virtual void log(const char* theText) = 0;
	virtual void showMessage(const char* theCaption, const char* theText) = 0;
	// The sound system of the window (DirectSound, 0x443ca0 (wordharmony)); null without sound
	virtual SoundSystem* createSoundSystem() = 0;
	// The signal build's LoadStringA of the exe's string table (the application's slot 0x8c (pizzafrenzy),
	// 0x4bd790 (pizzafrenzy)): "" for an id it has no text for
	virtual std::string loadString(int theId) { return ""; }
	// GetCommandLineA(): the program's name and its arguments
	virtual std::string commandLine() { return ""; }
	// port: a seed given for the game's srand of the tick count (-seed=N), for the identical-play runs
	virtual bool testSeed(unsigned int& theSeed) { return false; }
	// port(mod): a fix of the fixes mod is on (Platform::FixOn)
	virtual bool fixOn(const char* theFix) { return false; }
};

void setHost(Host* theHost);
Host* getHost();

} // namespace engine

#endif
