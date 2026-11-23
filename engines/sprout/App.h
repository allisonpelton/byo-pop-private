#ifndef __SPROUT_APP_H__
#define __SPROUT_APP_H__

// The application (base constructor 0x4382f0 (wordharmony), vtable 0x484948 (wordharmony); the Windows application
// 0x455050 (wordharmony), vtable 0x48a398 (wordharmony), 0xf4 bytes). It keeps the resource manager, the image and font
// caches, the game's controller (+0xa4), the root window (+0xa8) and the timing. The global 0x49bfbc (wordharmony)
// points to it (0x435f20 (wordharmony)).
//
// In the port the Windows layer's parts (the window, DirectDraw, DirectSound, the clock) come from the Host.

#include "Node.h"
#include "ResourceManager.h"
#include "Signal.h"
#include <cstdint>
#include <map>
#include <string>

namespace engine
{

// The window's size in pixels (the game's configure, 0x402eb0 (wordharmony), says it)
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

class Font;
class Graphics;
class Image;
class Sound;
class SoundSystem;

// The configuration the controller fills in (the application's +0x88) The controller's settings for the window (filled
// by its configure, 0x402eb0 (wordharmony); the run loop 0x4552b0 (wordharmony) refuses a configuration without a title
// or a frame rate)
struct AppConfig
{
	const char* mTitle;  // +0x88
	int mIcon;           // +0x8c: the window class's icon resource (101)
	int mFrameRate;      // +0x90: frames per second
	int mWidth;          // +0x94
	int mHeight;         // +0x98
	// +0x9c: starts full screen (the window's 0x456fa0 (wordharmony)); false, and not read by the port
	bool mFullScreen;
	bool mUseFiles;      // +0xa0: loose files instead of the archive
};

// The game's controller (the application's +0xa4; the game creates it in 0x407370 (wordharmony))
class Controller : public Object
{
public:
	virtual void configure(AppConfig& theConfig) = 0;  // slot 0x04
	virtual void init() = 0;                           // slot 0x08: after the window and the display exist
	virtual void shutdown() = 0;                       // slot 0x0c
	// slot 0x10: once a frame, before the root window (the signal build's application takes its frames from the
	// application's update signal instead, and has nothing here)
	virtual void update(UpdateInfo& theInfo) {}
	virtual void activate(bool theActive) {}           // slot 0x14: the window's focus gained or lost
};

class App : public Object
{
public:
	App();            // 0x4382f0 (wordharmony), 0x455050 (wordharmony)
	~App() override;  // 0x4381b0 (wordharmony)

	static App* instance();  // 0x435f20 (wordharmony)
	static SoundSystem* getSoundSystem();  // 0x435f30 (wordharmony)
	// 0x74: 0x435f40 (wordharmony), sets the global that getSoundSystem returns
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

	// 0x38: 0x437b20 (wordharmony), an image of the archive, cached by name
	Image* getImage(const char* theName);
	// 0x3c: 0x437be0 (wordharmony), the same from a loose file
	Image* getFileImage(const char* theName);
	// 0x40: 0x437dc0 (wordharmony), a font of the archive, cached by name
	Font* getFont(const char* theName);
	// 0x48: 0x453f80 (wordharmony) and 0x4c: 0x454010 (wordharmony), a new image (0x457410 (wordharmony)) loaded by
	// 0x457510 (wordharmony)
	Image* createImage(const char* theName, bool theFromFile);
	// 0x54: 0x436a70 (wordharmony)
	Image* findImage(const char* theName);
	// 0x58: 0x445310 (feedingfrenzy): an image added to the table under a name; false, with the table as it was,
	// when the name has an image already
	bool addImage(const char* theName, Image* theImage);
	// 0x58: the log (the release build's does nothing)
	void log(const char* theText);
	// The signal build's slot 0x58: 0x461a50 (pizzafrenzy): false without a sound system; true when the name is loaded
	// already; else the file, opened through openFile (the archive or a loose file), is loaded under the name
	bool loadSound(const std::string& theName, const std::string& theFile);
	// The signal build's slot 0x5c: 0x461b70 (pizzafrenzy), the same for a sound played as it goes (the music)
	bool loadSoundStream(const std::string& theName, const std::string& theFile);
	// The signal build's slot 0x60: 0x461c90 (pizzafrenzy), a new voice of a loaded sound; null without a sound system
	Sound* createSound(const std::string& theName, bool theStatic, int theCategory);
	// 0x5c: 0x454180 (wordharmony), timeGetTime()
	uint32_t getTime();
	// 0x60: 0x454190 (wordharmony)
	int getTicksPerSecond() { return 1000; }
	// 0x70: 0x454230 (wordharmony), PostQuitMessage(theCode)
	void quit(int theCode);
	// The signal build's slot 0x8c: 0x4bd790 (pizzafrenzy), a text of the exe's string table
	std::string loadString(int theId);
	// 0x78: 0x4540a0 (wordharmony), a new font loaded from the archive
	Font* createFont(const char* theName);

	// The map from names to nodes (+0x64): 0x437f60 (wordharmony) adds, 0x436ff0 (wordharmony) removes

	ResourceManager* getResources() { return mResources; }
	Window* getRootWindow() { return mRootWindow; }
	Controller* getController() { return mController; }

	// The run loop's start (0x4552b0 (wordharmony), up to its loop): the controller, the resource manager, the
	// configuration and the controller's init; the host has made the window and the display
	void start(Controller* theController);
	// 0x4542e0 (wordharmony): one frame
	void frame(Graphics* g);
	// The signal build's (Pizza Frenzy's) run loop, which sends the frame and the window's input through the
	// application's signals and makes no root window: the controller makes its own scene and connects it.
	// startSignalled is the loop's start (0x4bda70 (pizzafrenzy) and 0x4bdf20 (pizzafrenzy) up to the loop), the
	// controller's configure and init; frameSignalled is one frame (0x4bd210 (pizzafrenzy)): the seconds since the
	// last frame (at most 0.33) to the update signal, then the screen to the draw signal (when there is one). The
	// times are the host's ticks (timeTicks, ticksPerSecond).
	void startSignalled(Controller* theController);
	void frameSignalled(Graphics* g);
	// The run loop's end
	void shutdown();

	const AppConfig& getConfig() const { return mConfig; }

	// port (the "memreport" test command, docs/TEST_COMMANDS.md): how many pictures the image table holds and the bytes
	// of their 32-bit pixels (the fonts' pictures are in it)
	int getImageMemory(size_t* theBytes) const;

	// The signal build's signals (0x463c10 (pizzafrenzy) makes them): the run loop's and the window procedure's
	// (0x4bd3b0 (pizzafrenzy))
	Signal<UpdateInfo&> mUpdateSignal;    // +0x04: each frame
	Signal<Graphics*> mDrawSignal;        // +0x14: each frame after the update, when the display can draw
	Signal<bool> mActivateSignal;         // +0x24: WM_ACTIVATE, the window's focus gained (true) or lost
	Signal<int> mKeyDownSignal;           // +0x34: WM_KEYDOWN, not a repeat: the virtual key
	Signal<int, int> mKeyRepeatSignal;    // +0x44: WM_KEYDOWN, every one: the key and its repeat count
	Signal<int> mKeyUpSignal;             // +0x54: WM_KEYUP
	Signal<int> mCharSignal;              // +0x64: WM_CHAR, the character
	Signal<Point&> mMouseDownSignal;      // +0x74: WM_LBUTTONDOWN and WM_LBUTTONDBLCLK, in the screen's pixels
	Signal<Point&> mMouseUpSignal;        // +0x84: WM_LBUTTONUP
	Signal<Point&> mRightDownSignal;      // +0x94: WM_RBUTTONDOWN
	Signal<Point&> mRightUpSignal;        // +0xa4: WM_RBUTTONUP
	Signal<Point&> mMouseMoveSignal;      // +0xb4: WM_MOUSEMOVE
	Signal<> mCloseSignal;                // +0xc4: WM_SYSCOMMAND SC_CLOSE
	Signal<> mMinimizeSignal;             // +0xd4: SC_MINIMIZE
	Signal<> mMaximizeSignal;             // +0xe4: SC_MAXIMIZE
	Signal<> mRestoreSignal;              // +0xf4: SC_RESTORE
	Signal<int> mTimerSignal;             // +0x104: WM_TIMER 1, every 100 ms while the window is dragged or sized: 100
	// The signal build's last mouse position in the screen's pixels (+0x264 (pizzafrenzy)), which the host keeps as
	// the window procedure did (0x4bd050 (pizzafrenzy)); slot 0xc8 (pizzafrenzy) returns it
	Point mMousePosition;
	Point getMousePosition() const { return mMousePosition; }
	// Feeding Frenzy's player fish puts the pointer back at a point of the screen once it has read it (0x4108a0
	// (feedingfrenzy)), so that the next reading is how far the pointer moved: the position the game sees is that point
	// at once, and the host moves the real pointer there
	void warpMousePosition(int theX, int theY);

protected:
	std::map<std::string, Ref<Image>> mImages;  // +0x4c
	std::map<std::string, Ref<Font>> mFonts;    // +0x58
	std::multimap<std::string, Node*> mNodes;   // +0x64
	Ref<ResourceManager> mResources;            // +0x70
	AppConfig mConfig;                          // +0x88
	Ref<Controller> mController;                // +0xa4
	Ref<Window> mRootWindow;                    // +0xa8
	uint32_t mLastTime;                         // +0xb4
	uint64_t mLastTicks = 0;                    // the signal build's +0x210: the last frame's start
};

} // namespace engine

#endif
