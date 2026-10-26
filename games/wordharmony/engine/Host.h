#ifndef __WORDHARMONY_ENGINE_HOST_H__
#define __WORDHARMONY_ENGINE_HOST_H__

// The port's host: what the engine's Windows layer (0x454a40-0x45ee00) got from Windows, DirectDraw and DirectSound.
// The app in src/ implements it over the shared framework and platform layer.

#include <cstdint>
#include <string>
#include <vector>

namespace engine
{

class Image;

class Host
{
public:
	virtual ~Host() {}
	// timeGetTime()
	virtual uint32_t timeMs() = 0;
	// A file of the game's folder (WHArchive.saf, res/...), by its relative name with '/'
	virtual bool readGameFile(const std::string& theName, std::vector<uint8_t>& theData) = 0;
	virtual bool gameFileExists(const std::string& theName) = 0;
	// An image decoded from a PNG or JPEG file's bytes, with the alpha of a mask's bytes when theMask is not empty
	// (0x457510); null when it does not decode
	virtual Image* createImage(const std::vector<uint8_t>& theData, const std::vector<uint8_t>* theMask) = 0;
	// The registry values under HKEY_CURRENT_USER\Software\GameHouse\WordHarmony (0x421750)
	virtual bool registryReadString(const std::string& theName, std::string& theValue) = 0;
	virtual bool registryWriteString(const std::string& theName, const std::string& theValue) = 0;
	virtual bool registryReadInt(const std::string& theName, int& theValue) = 0;
	virtual bool registryWriteInt(const std::string& theName, int theValue) = 0;
	// The window's full screen mode (App slots 0x04 and 0x08: the window's 0x456fa0 and 0x455b80)
	virtual void setFullScreen(bool theFullScreen) = 0;
	virtual bool isFullScreen() = 0;
	// ShellExecute() of a link
	virtual void openURL(const std::string& theURL) = 0;
	virtual void log(const char* theText) = 0;
	virtual void showMessage(const char* theCaption, const char* theText) = 0;
};

void setHost(Host* theHost);
Host* getHost();

} // namespace engine

#endif
