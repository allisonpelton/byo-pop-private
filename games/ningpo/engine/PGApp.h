#ifndef __NINGPO_PGAPP_H__
#define __NINGPO_PGAPP_H__

// PGApp (0x40d4c0..0x410f80, vtable 0x463050; the game's subclass has 0x4622a8): the application. It owns the
// display, the input, the sounds and music, the cvars and the controls, and runs the event loop (mainLoop): the
// events go first to the app (preHandleEvent: activation, painting, the controls) and then to the game
// (handleEvent); when the queue is empty, the dirty rectangles are redrawn (renderDirty: the game's render, then
// the controls) and flipped to the window. Time is the event's (mEventTime); the game takes the time spent paused
// (getTime) off its clock.
//
// The original's subsystems that the game does not use are left out: the trace, journal and demo files, the
// sprite cursor (Ning Po's cursor sprite is null, 0x401a30), DirectInput, and the registration keys (the port
// plays as registered).

#include "HAudio.h"
#include "HCvar.h"
#include "HInput.h"
#include "PGSprite.h"
#include "Pg.h"

#include <list>
#include <memory>
#include <string>

namespace pg
{

class HDisplay;
class HFileSystem;
struct HFont;
class PGControl;

// The app's random numbers (vtable 0x463124, at PGApp+0x5f34): a Mersenne Twister seeded the old way (each word
// the last times 69069)
class PGRandom
{
public:
	uint32_t mSeed;              // +0x04
	uint32_t mState[624];        // +0x08
	uint32_t mIndex;             // +0x9c8

	PGRandom() : mSeed(0), mIndex(625) { mState[0] = 0; }
	// 0x4131d0 (slot 4)
	void seed(uint32_t theSeed);
	// 0x413220 (slot 8): 0..1
	// 0x413250 (slot 0xc): 0..theMax, both included
	uint32_t next(uint32_t theMax);
};

// HRegistry (vtable 0x4634cc, 0x4133b0..0x4136d0): the values under Software\<company>\<product>; port: the host's
class HRegistry
{
public:
	// 0x4134b0 (slot 0)
	// 0x413400 makes the key
	bool open(const char* theCompany, const char* theProduct) { (void)theCompany; (void)theProduct; return true; }
	// 0x413510 (slot 4)
	// 0x413490 closes the key
	bool close() { return true; }
	// 0x413640 (slot 8): a string value
	bool readString(const char* theName, char* theBuf, int theSize);
	// 0x4136d0 (slot 0xc)
	bool writeString(const char* theName, const char* theValue);
	// 0x413520 (slot 0x10): binary data, its size in *theSizeOut
	bool readData(const char* theName, void* theBuf, int theSize, int* theSizeOut);
	// 0x4135c0 (slot 0x14)
	bool writeData(const char* theName, const void* theData, int theSize);
};

// 0x413360: the one registry object
HRegistry* GetRegistry();

// A sprite and its file, as the loading screen's table lists them (the game's at NingPo+0xa914)
struct PGSpriteEntry
{
	PGSprite* mSprite;
	const char* mName;
};

class PGApp
{
public:
	// The loading screen (0x40f210..0x40f750)
	PGSprite mLoadProgress;       // +0x0c
	PGSprite mLoadBackground;     // +0x60
	PGSprite mLoadBottomLeft;     // +0xb4
	PGSprite mLoadBottomRight;    // +0x108
	PGPoint mLoadAt;              // +0x15c where the progress sprite is revealed
	bool mLinksDisabled;          // +0x164 the registry's "disableLinks"
	HDisplay* mDisplay;           // +0x168
	bool mHasAudio;               // +0x16c the DirectSound device; port: the platform's mixer
	HFileSystem* mFileSystem;     // +0x174 the archive
	HRectList mDirty;             // +0x178
	bool mMinimized;              // +0x217c
	bool mPaused;                 // +0x217d the game's pause; the loop waits for events while it is set
	bool mActive;                 // +0x217e
	float mMusicVolume;           // +0x2180 0..1
	int mMusicVoice;              // +0x2184
	int mPauseCount;              // +0x2188
	PyTime mPausedTime;           // +0x2190 the time spent paused, up to the last pause (slot 0xcc clears it)
	PyTime mPauseStart;           // +0x2198
	PyTime mPauseEnd;             // +0x21a0
	HCvarManager mCvars;          // +0x21a8
	HInput mInput;                // +0x21b0
	PGSoundManager mSounds;       // +0x3a18
	PGRandom mRandom;             // +0x5f34
	bool mRandomSeeded;           // +0x6900
	std::list<PGControl*> mControls;  // +0x6904 the newest first
	PyTime mEventTime;            // +0x6920 the time of the event being handled
	std::shared_ptr<Platform::SoundData> mMusic;  // +0x692c the music stream (+0x6a8c its file)
	HMixer mMixer;                // +0x6930
	bool mMusicOn;                // +0x6a90

	PGApp();                      // 0x40d4c0
	virtual ~PGApp();             // 0x40d620

	// 0x40f840 PGApp::run
	void run(const char* theCmdLine, const char* theResourceDir);

	// slot 0x24: true keeps the loop going without events; false (Ning Po's, 0x405c60) waits for them
	virtual bool isRealtime() { return true; }
	// slot 0x30 (0x40e040 PGApp::init)
	// 0x40dfb0 PGApp::registerCvars is part of init (the cvars it registers)
	virtual bool init(const char* theCmdLine, const char* theResourceDir);
	// slot 0x34 (0x40e430 PGApp::shutdown)
	// 0x40dbd0 and 0x423600 free the display, 0x40dc40 the input, 0x40e6f0 the audio stream, 0x4240c0 the display's destructor
	// 0x40dd30 shut the sound
	virtual void shutdown();
	// slot 0x38 (0x40e4f0): the controls, the archive and the cvars freed
	// 0x40e990 deletes the controls
	virtual void cleanup();
	// slot 0x5c: true quits
	virtual bool handleEvent(const PGEvent* theEvent) = 0;
	// slot 0x60: the screen under theRect; theUnderDialog while a dialog is up
	virtual void render(const PGRect* theRect, bool theUnderDialog) = 0;
	// slot 0x74 (0x40d940 PGApp::setActive)
	virtual void setActive(bool theActive) { mActive = theActive; }

	// 0x40d820 PGApp::mainLoop: true when the game quit
	bool mainLoop();
	// 0x40d950 PGApp::preHandleEvent: true when a control took the event
	bool preHandleEvent(const PGEvent* theEvent);
	// 0x40dad0 PGApp::dispatchToControls
	bool dispatchToControls(const PGEvent* theEvent);
	// 0x40dd90 PGApp::drawControls: with theUnderDialog only those shown under dialogs
	void drawControls(const PGRect* theClip, bool theUnderDialog);
	// 0x40de00 PGApp::invalidateRect: clamped to the screen
	void invalidateRect(const PGRect* theRect);
	// 0x40dea0 PGApp::invalidateAll
	void invalidateAll();
	// 0x40ded0 PGApp::renderDirty
	// 0x427d00 (slot 0x70) does nothing
	void renderDirty(bool theUnderDialog);
	// 0x40df50 PGApp::canDraw (slot 0x6c)
	bool canDraw() const;
	// 0x40df70 PGApp::endFrame: the display flipped and the dirty rectangles forgotten
	void endFrame();
	// 0x40ed70 PGApp::presentDirty (slot 0x10): the sprite cursor drawn in each dirty rectangle; port: none
	// port: the original drew its sprite cursor here (left out, coverage.txt)
	void presentDirty() {}

	// 0x40e940 (slot 0x54)
	void addControl(PGControl* theControl);
	// 0x40ea00 (slot 0x58)

	// 0x40d690 PGApp::getTime (slot 0xc8): the time spent paused, the pause going on included
	PyTime getTime() const;
	// 0x40d710 PGApp::beginPause (slot 0xc0)
	// 0x40d43b (the HNop test shared with endPause)
	void beginPause();
	// 0x40d770 PGApp::endPause (slot 0xc4)
	void endPause();
	// 0x40d810 PGApp::isPaused (slot 0xd0)
	bool isPaused() const { return mPauseCount > 0; }

	// 0x40e350 (slot 0xb8): the archive mounted
	// 0x4279a0 mounts the archive, 0x427350 tells an archive from a folder
	bool openResourceFile(const char* theName);
	// 0x40e320 (slot 0xbc)
	void closeResourceFile();
	// 0x40ecc0 (slot 0x8c): the display, the input and the sound
	// 0x40db70 makes the display (0x423ff0, constructors 0x424050 and 0x422b10), 0x40dc70 and 0x40e650 the audio
	// 0x40dc00 made the DirectInput device
	bool initDevices(bool theFullscreen, int theWidth, int theHeight, int theChannels, int theBits, int theRate);

	// 0x40e590 (slot 0x90): the music read into memory and opened as a stream that loops from theLoopStart
	bool loadMusic(const char* theName, int theLoopStart);
	// 0x40e730 (slot 0x94)
	void freeMusic();
	// 0x40e570 (slot 0x98): the music from its start
	void restartMusic();
	// 0x40e850 (slot 0x9c)
	void setMusicOn(bool theOn);
	// 0x40e7b0
	void setMusicVolume(float theVolume);

	// 0x40f6b0: the loading screen; the sprites of theTable take 3/4 of theTotal, the music and sounds the rest
	bool loadAssets(PGSpriteEntry* theTable, int theCount, const char* theMusic, int theLoopStart,
		const char** theSounds, const char* theBackground, const char* theBottomLeft, const char* theBottomRight,
		const char* theProgress, int theX, int theY, float theTotal, const char* theText, HFont* theFont);

	// 0x4105a0 PGApp::messageDialog (slot 0x40): a background with OK and, when given, Cancel; theX or theY -1
	// centres it; with theTimeout the OK button shows only after it. 1 for OK, 2 when the window was closed, else 0
	int messageDialog(int theX, int theY, int theTimeout, const char* theBackground, const char* theOk,
		const char* theOkLit, const char* theCancel, const char* theCancelLit);
	// 0x40fe90 (slot 0x44): pages shown one at a time, with buttons to close and to go back and forth
	// 0x4100e0 lays the slideshow out
	bool slideshow(int theX, int theY, const char* theClose, const char* theCloseLit, const char* theNext,
		const char* theNextLit, const char* thePrev, const char* thePrevLit, const char** thePages, int theCount);

private:
	// 0x40f530: the loading screen's sprites
	bool loadScreenSprites(const char* theBackground, const char* theBottomLeft, const char* theBottomRight,
		const char* theProgress, int theX, int theY);
	// 0x40f600
	bool loadSprites(PGSpriteEntry* theTable, int theCount, float theTotal, const char* theText, HFont* theFont);
	// 0x40f750
	bool loadSounds(const char* theMusic, int theLoopStart, const char** theSounds, float theFrom, float theTo);
	// 0x40f210: the loading screen at theProgress (0 draws the whole screen)
	void drawLoadProgress(float theProgress, const char* theText, HFont* theFont);
	// 0x40f4f0
	void releaseScreenSprites();
};

// 0x4c1d3c: the app (set by its constructor)
PGApp* GetApp();

} // namespace pg

#endif
