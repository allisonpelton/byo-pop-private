#ifndef __WORDHARMONY_ENGINE_APP_H__
#define __WORDHARMONY_ENGINE_APP_H__

// The application (base constructor 0x4382f0, vtable 0x484948; the Windows application 0x455050, vtable 0x48a398,
// 0xf4 bytes). It keeps the resource manager, the image and font caches, the map from names to nodes, the game's
// controller (+0xa4), the root window (+0xa8) and the timing. The global 0x49bfbc points to it (0x435f20).
//
// In the port the Windows layer's parts (the window, DirectDraw, DirectSound, the clock) come from the Host.

#include "Node.h"
#include "ResourceManager.h"
#include <cstdint>
#include <map>
#include <string>

namespace engine
{

class Font;
class Graphics;
class Image;
class SoundSystem;

// The configuration the controller fills in (the application's +0x88)
// The controller's settings for the window (filled by its configure, 0x402eb0; the run loop 0x4552b0 refuses a
// configuration without a title or a frame rate)
struct AppConfig
{
	const char* mTitle;  // +0x88
	int mIcon;           // +0x8c: the window class's icon resource (101)
	int mFrameRate;      // +0x90: frames per second
	int mWidth;          // +0x94
	int mHeight;         // +0x98
	bool mUnknown9C;     // +0x9c: false
	bool mUseFiles;      // +0xa0: loose files instead of the archive
};

// The game's controller (the application's +0xa4; the game creates it in 0x407370)
class Controller : public Object
{
public:
	virtual void configure(AppConfig& theConfig) = 0;  // slot 0x04
	virtual void init() = 0;                           // slot 0x08: after the window and the display exist
	virtual void shutdown() = 0;                       // slot 0x0c
	virtual void update(UpdateInfo& theInfo) = 0;      // slot 0x10: once a frame, before the root window
	virtual void activate(bool theActive) {}           // slot 0x14: the window's focus gained or lost
};

class App : public Object
{
public:
	App();            // 0x4382f0, 0x455050
	~App() override;  // 0x4381b0

	static App* instance();  // 0x435f20
	static SoundSystem* getSoundSystem();  // 0x435f30
	// 0x74: 0x435f40, sets the global that getSoundSystem returns
	void setSoundSystem(SoundSystem* theSoundSystem);

	// The resource manager's functions (slots 0x18-0x34)
	void addArchive(const char* thePath) { mResources->addArchive(thePath); }                  // 0x18
	bool exists(const char* theName) { return mResources->exists(theName); }                   // 0x1c
	bool fileExists(const char* theName) { return mResources->fileExists(theName); }           // 0x20
	void setUseFiles(bool theUseFiles) { mResources->setUseFiles(theUseFiles); }               // 0x24
	InputStream* openRawStream(const char* theName) { return mResources->openRawStream(theName); }  // 0x28
	InputStream* openStream(const char* theName) { return mResources->openStream(theName); }        // 0x2c
	InputStream* openFile(const char* theName) { return mResources->openFile(theName); }            // 0x30
	InputStream* openBufferedFile(const char* theName) { return mResources->openBufferedFile(theName); }  // 0x34

	// 0x38: 0x437b20, an image of the archive, cached by name
	Image* getImage(const char* theName);
	// 0x3c: 0x437be0, the same from a loose file
	Image* getFileImage(const char* theName);
	// 0x40: 0x437dc0, a font of the archive, cached by name
	Font* getFont(const char* theName);
	// 0x44: 0x437e90, the same from a loose file
	Font* getFileFont(const char* theName);
	// 0x48: 0x453f80 and 0x4c: 0x454010, a new image (0x457410) loaded by 0x457510
	Image* createImage(const char* theName, bool theFromFile);
	// 0x50: 0x437ca0, adds an image to the cache under a name; false when the name is taken
	bool addImage(Image* theImage, const char* theName);
	// 0x54: 0x436a70
	Image* findImage(const char* theName);
	// 0x58: the log (the release build's does nothing)
	void log(const char* theText);
	// 0x5c: 0x454180, timeGetTime()
	uint32_t getTime();
	// 0x60: 0x454190
	int getTicksPerSecond() { return 1000; }
	// 0x70: 0x454230, PostQuitMessage(theCode)
	void quit(int theCode);
	// 0x78: 0x4540a0 and 0x7c: 0x454110, a new font loaded from the archive or a loose file
	Font* createFont(const char* theName, bool theFromFile);

	// The map from names to nodes (+0x64): 0x437f60 adds, 0x436ff0 removes
	void registerNode(Node* theNode);
	void unregisterNode(Node* theNode);
	Node* findNode(const std::string& theName);

	ResourceManager* getResources() { return mResources; }
	Window* getRootWindow() { return mRootWindow; }
	Controller* getController() { return mController; }

	// The run loop's start (0x4552b0, up to its loop): the controller, the resource manager, the configuration and
	// the controller's init; the host has made the window and the display
	void start(Controller* theController);
	// 0x4542e0: one frame
	void frame(Graphics* g);
	// The run loop's end
	void shutdown();

	const AppConfig& getConfig() const { return mConfig; }

protected:
	std::map<std::string, Ref<Image>> mImages;  // +0x4c
	std::map<std::string, Ref<Font>> mFonts;    // +0x58
	std::multimap<std::string, Node*> mNodes;   // +0x64
	Ref<ResourceManager> mResources;            // +0x70
	AppConfig mConfig;                          // +0x88
	Ref<Controller> mController;                // +0xa4
	Ref<Window> mRootWindow;                    // +0xa8
	uint32_t mLastTime;                         // +0xb4
};

} // namespace engine

#endif
