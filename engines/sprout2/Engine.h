#ifndef __SPROUT2_ENGINE_H__
#define __SPROUT2_ENGINE_H__

// The engine: the one object every part of the game reaches through the global the original keeps at 0x5ab9e4
// (feedingfrenzy2), getEngine() here. engine::Engine is the original's own abstract class (vtable 0x55ff3c... with 70
// slots, its constructor 0x495f30 (feedingfrenzy2)): the input signals, the caches of images and fonts, the archives,
// and the platform's services, which its Windows implementation dxdriver::EngineImpl (0x494e30 (feedingfrenzy2))
// provides over a window, DirectDraw and DirectSound. The port's EngineImpl provides them over the framework. Each slot
// is named with its offset in the vtable.

#include "Geometry.h"
#include "Object.h"
#include "sigslot.h"

#include <cstdint>
#include <map>
#include <string>

namespace simplesound
{
class SimpleSound;
class SoundHandle;
}

namespace engine
{

class Component;
class DocHandler;
class DrawContext;
class Font;
class Image;
class Mouse;
class InputStream;
class PersistKey;
class RandomAccessInputStream;
class Reader;
class ResourceManager;
class Surface;

// What the update signal passes to every slot (built on the stack by the frame, 0x438700 (feedingfrenzy2); its
// constructor 0x435b30 (feedingfrenzy2))
struct UpdateContext
{
	Vector2f mPosition;           // +0x00: (0, 0)
	float mElapsed = 0;           // +0x08: the seconds since the last frame, at most 0.33
	uint64_t mFrame = 0;          // +0x10: the frame's number, from 0
	int mDetailLevel = 0;         // +0x18: the engine's detail level (getDetailLevel)
};

class Engine
{
public:
	Engine();
	virtual ~Engine();

	// --- The platform's services (EngineImpl's) ---

	// 0x28 (0x424720 (feedingfrenzy2)) and 0x2c (0x424710 (feedingfrenzy2)): the window's client size
	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
	// 0xa4 (0x41c200 (feedingfrenzy2)): a file of the disk is there (a relative path is the game folder's)
	virtual bool diskFileExists(const std::string& thePath) = 0;
	// 0xa8 (0x40ad60 (feedingfrenzy2)): a file of the disk opened, null when it cannot be
	virtual RandomAccessInputStream* openDiskFile(const std::string& thePath) = 0;
	// 0xcc and 0xd0 (0x49a920 (feedingfrenzy2)): a line for the debugger and the log file, printf style
	virtual void log(const char* theFormat, ...) = 0;
	// 0xe4 (0x4383a0 (feedingfrenzy2)) and 0xe8 (0x435ba0 (feedingfrenzy2)): the clock in ticks
	virtual uint64_t getTicks() = 0;
	virtual uint64_t getTicksPerSecond() = 0;
	// 0xc8 (0x48da20 (feedingfrenzy2)): the run loop's frames per second, 0 for no wait between frames (EngineImpl's
	// +0x58, with the frame's period in ticks at +0x160)
	virtual void setFrameRate(int theFramesPerSecond) = 0;
	// 0xf8 (0x4a4a00 (feedingfrenzy2)): the program ends (PostQuitMessage)
	virtual void quit() = 0;
	// 0xbc (0x48b200 (feedingfrenzy2)): the key below a key, made when it is not there; null when it cannot be opened
	virtual PersistKey* openPersistKey(PersistKey* theParent, const std::string& theName) = 0;
	// 0xc0 (0x492cf0 (feedingfrenzy2)): the key Software\<path> of HKEY_LOCAL_MACHINE (root 0) or HKEY_CURRENT_USER
	// (root 1), made when it is not there; null when it cannot be opened
	virtual PersistKey* openPersistKey(int theRoot, const std::string& thePath) = 0;
	// port: what the edit box asks Windows for itself. GetAsyncKeyState's key down (a virtual key)
	virtual bool isKeyDown(int theKey) = 0;
	// port: the clipboard's text (CF_TEXT), false when it holds none
	virtual bool getClipboardText(std::string& theText) = 0;
	// 0x100: the mouse a game steers by (Mouse.h), null where the platform has none
	virtual Mouse* getMouse() { return nullptr; }

	// --- The engine's own ---

	// 0x20 (0x49f680) and 0x24 (0x47d870) (feedingfrenzy2): the options' detail level, 0 to 2 (EngineImpl's +0x2a0;
	// the option screen sets it, and spawners and selectors pick by it)
	virtual void setDetailLevel(int theDetailLevel) { mDetailLevel = theDetailLevel; }
	virtual int getDetailLevel() { return mDetailLevel; }

	// 0x3c (0x49fb50 (feedingfrenzy2)): an archive added to those files are looked for in
	virtual void addArchive(const std::string& thePath);
	// 0x40 (0x4603e0 (feedingfrenzy2)): in an archive, or on the disk when loose files are used
	virtual bool fileExists(const std::string& thePath);
	// 0x48 (0x49b8d0 (feedingfrenzy2)): files read from the disk instead of the archives (the game sets false)
	virtual void setUseLooseFiles(bool theUseLooseFiles);
	// 0x4c (0x466cb0 (feedingfrenzy2)): a file opened from an archive or the disk; null when there is none
	virtual InputStream* openFile(const std::string& thePath);
	// 0x50 (0x41dc50 (feedingfrenzy2)): the same, read through a reader
	virtual Reader* openReader(const std::string& thePath);
	// 0x58 (0x538632 (feedingfrenzy2)): a file of the disk read through a reader; null when it cannot be opened. (Slots
	// 0x44 and 0x54 only pass the disk's fileExists and openFile on to diskFileExists and openDiskFile; the port's
	// callers call those.)
	virtual Reader* openDiskReader(const std::string& thePath);
	// 0x5c (0x487c80 (feedingfrenzy2)) and 0x60 (0x487c70 (feedingfrenzy2)): the digests of archived files checked or
	// not (the port never checks them)
	virtual void setCheckDigests(bool theCheckDigests);

	// 0x64 (0x425f30 (feedingfrenzy2)): an XML file of the archives parsed into the handler; false when it is missing
	// or the parser stopped at an error
	virtual bool parseXml(const std::string& thePath, DocHandler* theHandler);
	// 0x68 (0x54423a (feedingfrenzy2)): the same with a file of the disk
	virtual bool parseDiskXml(const std::string& thePath, DocHandler* theHandler);

	// 0x6c (0x469660 (feedingfrenzy2)): an image of the archives, loaded the first time and kept under its path
	virtual Surface* getImage(const std::string& thePath);
	// 0x70 (0x52000f (feedingfrenzy2)): the same with a file of the disk
	virtual Surface* getDiskImage(const std::string& thePath);
	// 0x74 (0x41bf50 (feedingfrenzy2)): a font, loaded the first time and kept under its path
	virtual Font* getFont(const std::string& thePath);
	// 0xfc (0x507c32 (feedingfrenzy2)): the global sound system (simplesound::getSimpleSound) set
	virtual void setSoundSystem(simplesound::SimpleSound* theSoundSystem);
	// port: the original's 0x4a2790 (feedingfrenzy2), from the application's start (0x4a1a70): a new SimpleSoundDX
	// over the window (the engine's slot 0x30), null when DirectSound cannot start. The engine has none of its own
	virtual simplesound::SimpleSound* makeSoundSystem() { return nullptr; }
	// 0x7c (0x478280 (feedingfrenzy2)): the file read whole into a sound registered under the name (the sound
	// system's slot 0x04); true when it was, or one was already; false without a sound system or the file
	virtual bool registerSound(const std::string& theName, const std::string& thePath);
	// 0x80 (0x416950 (feedingfrenzy2)): the same with the file read as it plays (the sound system's slot 0x08)
	virtual bool registerSoundStream(const std::string& theName, const std::string& thePath);
	// 0x84 (0x481580 (feedingfrenzy2)): a new handle of the sound registered under the name (the sound system's slot
	// 0x18); null without a sound system or such a sound
	virtual simplesound::SoundHandle* getSoundHandle(const std::string& theName, int theSetupValue, int theCategory);
	// 0x88 (EngineImpl's 0x46c4c0 (feedingfrenzy2)): a new image loaded from the archives; an empty one when the file
	// cannot be read
	virtual Surface* loadImage(const std::string& thePath);
	// 0x8c (EngineImpl's 0x53a0d8 (feedingfrenzy2)): the same from the disk
	virtual Surface* loadDiskImage(const std::string& thePath);
	// 0x90 (0x51ffda (feedingfrenzy2)): an image kept under a path unless one is kept there already; true when it was
	virtual bool addImage(Surface* theImage, const std::string& thePath);
	// 0x94 (0x469650 (feedingfrenzy2)): the image kept under the path, null for none
	virtual Surface* findImage(const std::string& thePath);
	// 0x9c (EngineImpl's 0x4673d0 (feedingfrenzy2)): a new image of the size (the port's pixels cleared, the
	// original's as the allocation found them)
	virtual Surface* createSurface(int theWidth, int theHeight);
	// 0xa0 (EngineImpl's 0x41c490 (feedingfrenzy2)): a new draw context drawing into the image
	virtual DrawContext* createDrawContext(Surface* theTarget);
	// 0x4719f0 and 0x418120 (feedingfrenzy2), from Theme::load: the kept images, and the kept fonts, that nothing but
	// the engine holds let go
	void releaseUnusedImages();
	void releaseUnusedFonts();
	// 0x110 (EngineImpl's 0x407760 (feedingfrenzy2)): a new font loaded from the archives
	virtual Font* createFont(const std::string& thePath);

	// --- The input and frame signals, in the order of their fields (+0x04 to +0x124) ---

	sigslot::signal1<UpdateContext*> mUpdateSignal;                     // +0x04: each frame, before it is drawn
	sigslot::signal1<DrawContext*> mDrawSignal;                         // +0x14: each frame, while the display is ready
	sigslot::signal1<bool> mActivateSignal;                             // +0x24: WM_ACTIVATE, and false on a minimize
	sigslot::signal1<int> mKeyDownSignal;                               // +0x34: WM_KEYDOWN (the first, not a repeat)
	sigslot::signal2<int, int> mKeyRepeatSignal;                        // +0x44: WM_KEYDOWN repeated: the key, the count
	sigslot::signal1<int> mKeyUpSignal;                                 // +0x54: WM_KEYUP
	sigslot::signal1<char> mCharSignal;                                 // +0x64: WM_CHAR
	sigslot::signal1<const Vector2<int>&> mMouseDownSignal;             // +0x74: the left button
	sigslot::signal1<const Vector2<int>&> mMouseUpSignal;               // +0x84
	sigslot::signal1<const Vector2<int>&> mRightDownSignal;             // +0x94
	sigslot::signal1<const Vector2<int>&> mRightUpSignal;               // +0xa4
	sigslot::signal1<const Vector2<int>&> mMouseMoveSignal;             // +0xb4
	sigslot::signal2<const Vector2<int>&, int> mMouseWheelSignal;       // +0xc4: the place and WM_MOUSEWHEEL's delta
	// +0xd4: a signal0 the window never emits (not yet seen connected)
	sigslot::signal0 mMinimizeSignal;                                   // +0xe4: SC_MINIMIZE
	sigslot::signal0 mMaximizeSignal;                                   // +0xf4: SC_MAXIMIZE
	sigslot::signal0 mRestoreSignal;                                    // +0x104: SC_RESTORE, or another copy started
	// +0x114 (a signal2<const char*, const char*>) and +0x124 (a signal1<const std::string&>): not yet seen emitted

	// --- The table of named components (+0x148: a multimap of names to components) ---

	// 0x45b940 (feedingfrenzy2), from Component::setName
	void addNamedComponent(const std::string& theName, Component* theComponent);
	// 0x4536c0 (feedingfrenzy2): the component's entry under its name taken out
	void removeNamedComponent(Component* theComponent);
	// 0x41ea70 (feedingfrenzy2): the first component under the name, null for none
	Component* findNamedComponent(const std::string& theName);

	// 0xb8 (EngineImpl's 0x419820 (feedingfrenzy2)): the format with its inserts filled in, by FormatMessageA from a
	// string in the user's language (%1 to %99 an insert, typed by a printf type between exclamation marks after it,
	// %% a percent sign, %n a line break, %0 the end). port: not virtual, as the platform's part is Windows' own and
	// done here, with whole numbers for the inserts, which is all the games pass
	std::string formatMessage(const std::string& theFormat, int theInsert);

protected:
	std::map<std::string, Ref<Surface>> mImages;              // +0x134
	std::map<std::string, Ref<Font>> mFonts;                  // +0x140
	std::multimap<std::string, Component*> mNamedComponents;  // +0x14c
	ResourceManager* mResourceManager;                        // +0x158
	int mDetailLevel;                                         // EngineImpl's +0x2a0
};

// The global engine (0x5ab9e4 (feedingfrenzy2)), set by the engine's constructor
Engine* getEngine();

} // namespace engine

#endif
