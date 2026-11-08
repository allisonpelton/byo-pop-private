// PGApp (see PGApp.h).

#include "PGApp.h"
#include "HDisplay.h"
#include "HFileSystem.h"
#include "PGControl.h"
#include "PGDialog.h"

#include "platform/Audio.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace pg
{

static PGApp* gApp = nullptr;

PGApp* GetApp()
{
	return gApp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGRandom

void PGRandom::seed(uint32_t theSeed)
{
	mIndex = 1;
	mState[0] = theSeed;
	do
	{
		mState[mIndex] = mState[mIndex - 1] * 0x10dcd;
		mIndex++;
	} while (mIndex < 0x270);
	mSeed = theSeed;
}

float PGRandom::nextFloat()
{
	return (float)((double)next(0xfffffffe) * 2.3283064e-10);
}

uint32_t PGRandom::next(uint32_t theMax)
{
	static const uint32_t kMag01[2] = {0, 0x9908b0df};  // 0x4692c8
	if (mIndex > 0x26f)
	{
		uint32_t* p = mState;
		int i = 0xe3;
		do
		{
			uint32_t y = ((p[1] ^ p[0]) & 0x7fffffff) ^ p[0];
			i--;
			p[0] = kMag01[y & 1] ^ (y >> 1) ^ p[0x18d];
			p++;
		} while (i != 0);
		i = 0x18c;
		p = mState + 0xe3;
		do
		{
			uint32_t y = ((p[1] ^ p[0]) & 0x7fffffff) ^ p[0];
			i--;
			p[0] = kMag01[y & 1] ^ (y >> 1) ^ p[-0xe3];
			p++;
		} while (i != 0);
		uint32_t y = ((mState[0] ^ mState[0x26f]) & 0x7fffffff) ^ mState[0x26f];
		mIndex = 0;
		mState[0x26f] = kMag01[y & 1] ^ mState[0x18c] ^ (y >> 1);
	}
	uint32_t y = mState[mIndex];
	mIndex++;
	y ^= y >> 0xb;
	y ^= (y & 0xff3a58ad) << 7;
	y ^= (y & 0xffffdf8c) << 0xf;
	return ((y >> 0x12) ^ y) % (theMax + 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HRegistry

static HRegistry gRegistry;

HRegistry* GetRegistry()
{
	return &gRegistry;
}

bool HRegistry::readString(const char* theName, char* theBuf, int theSize)
{
	theBuf[0] = 0;
	std::string aValue;
	if (!GetHost()->RegistryReadString(theName, aValue))
		return false;
	HStrncpy(theBuf, aValue.c_str(), theSize);
	return true;
}

bool HRegistry::writeString(const char* theName, const char* theValue)
{
	char aBuf[0x100];
	HStrncpy(aBuf, theValue, 0x100);
	return GetHost()->RegistryWriteString(theName, aBuf);
}

bool HRegistry::readData(const char* theName, void* theBuf, int theSize, int* theSizeOut)
{
	std::vector<uint8_t> aData;
	// RegQueryValueEx fails when the value is larger than the buffer
	if (!GetHost()->RegistryReadData(theName, aData) || (int)aData.size() > theSize)
		return false;
	if (!aData.empty())
		memcpy(theBuf, aData.data(), aData.size());
	*theSizeOut = (int)aData.size();
	return true;
}

bool HRegistry::writeData(const char* theName, const void* theData, int theSize)
{
	return GetHost()->RegistryWriteData(theName, theData, theSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGApp

PGApp::PGApp()
{
	mLoadAt = {0, 0};
	mDisplay = nullptr;
	mHasAudio = false;
	mFileSystem = nullptr;
	mLinksDisabled = false;
	mPauseCount = 0;
	mPauseStart = 0;
	mPauseEnd = 0;
	mPausedTime = 0;
	mActive = false;
	mMinimized = false;
	mPaused = false;
	mRandomSeeded = false;
	mEventTime = 0;
	mMusicOn = false;
	mMusicVoice = -1;
	gApp = this;
	mMusicVolume = 0.25f;
}

PGApp::~PGApp()
{
	if (gApp == this)
		gApp = nullptr;
}

PyTime PGApp::getTime() const
{
	if (mPauseCount == 0)
		return mPausedTime;
	return PyTimeSub(mEventTime, mPauseStart) + mPausedTime;
}

void PGApp::beginPause()
{
	if (mPauseCount == 0)
		mPauseStart = mEventTime;
	mPauseCount++;
}

void PGApp::endPause()
{
	mPauseCount--;
	if (mPauseCount == 0)
	{
		mPauseEnd = mEventTime;
		if (mPauseStart != 0)
			mPausedTime += PyTimeSub(mPauseEnd, mPauseStart);
		mPauseStart = 0;
	}
}

bool PGApp::mainLoop()
{
	bool aQuit = false;
	invalidateAll();
	while (true)
	{
		PGEvent anEvent;
		memset(&anEvent, 0, sizeof(anEvent));
		bool aWait = mPaused || !isRealtime();
		bool aGot = mInput.getEvent(&anEvent, aWait);
		if (aGot)
		{
			do
			{
				mEventTime = anEvent.mTime;
				if (!mRandomSeeded && anEvent.mTime != 0)
				{
					mRandom.seed((uint32_t)anEvent.mTime);
					mRandomSeeded = true;
				}
				if (!preHandleEvent(&anEvent))
				{
					aQuit = handleEvent(&anEvent);
					if (aQuit)
						break;
				}
				aGot = mInput.getEvent(&anEvent, aWait);
			} while (aGot);
			if (aQuit)
				return true;
		}
		renderDirty(false);
		presentDirty();
		endFrame();
	}
}

bool PGApp::preHandleEvent(const PGEvent* theEvent)
{
	switch (theEvent->mType)
	{
	case EV_ACTIVATE:
		// the original also told the display and DirectInput (slots 0x48/0x44 and 0x14/0x10)
		if (theEvent->mValue != 0)
		{
			setActive(true);
			return dispatchToControls(theEvent);
		}
		setActive(false);
		// fall through
	case EV_PAINT:
		invalidateAll();
		return dispatchToControls(theEvent);
	case EV_HIDDEN:
		if (theEvent->mValue != (int)mMinimized)
			mMinimized = theEvent->mValue != 0;
		break;
	}
	// a mouse move redrew the sprite cursor; port: Ning Po has none
	return dispatchToControls(theEvent);
}

bool PGApp::dispatchToControls(const PGEvent* theEvent)
{
	for (PGControl* aControl : mControls)
	{
		if (aControl->isEnabled() && aControl->handleEvent(theEvent))
			return true;
	}
	return false;
}

void PGApp::drawControls(const PGRect* theClip, bool theUnderDialog)
{
	for (PGControl* aControl : mControls)
	{
		if (!theUnderDialog || aControl->isShownUnderDialogs())
			aControl->draw(theClip);
	}
}

void PGApp::invalidateRect(const PGRect* theRect)
{
	int aWidth = mDisplay->mWidth;
	int aHeight = mDisplay->mHeight;
	PGRect aRect = *theRect;
	if (aRect.left < 0)
		aRect.left = 0;
	else if (aWidth < aRect.left)
		aRect.left = aWidth;
	if (aRect.right < 0)
		aRect.right = 0;
	else if (aWidth < aRect.right)
		aRect.right = aWidth;
	if (aRect.top < 0)
		aRect.top = 0;
	else if (aHeight < aRect.top)
		aRect.top = aHeight;
	if (aRect.bottom < 0)
		aRect.bottom = 0;
	else if (aHeight < aRect.bottom)
		aRect.bottom = aHeight;
	mDirty.add(aRect);
}

void PGApp::invalidateAll()
{
	if (mDisplay != nullptr)
		mDirty.add(0, 0, mDisplay->mHeight, mDisplay->mWidth);
}

void PGApp::renderDirty(bool theUnderDialog)
{
	if (!canDraw())
		return;
	int aCount = mDirty.count();
	if (aCount > 0)
	{
		for (int i = 0; i < aCount; i++)
			render(mDirty.get(i), theUnderDialog);
		for (int i = 0; i < aCount; i++)
			drawControls(mDirty.get(i), theUnderDialog);
	}
	// slot 0x70 (0x427d00) does nothing
}

bool PGApp::canDraw() const
{
	if (mMinimized)
		return false;
	return mDisplay != nullptr;
}

void PGApp::endFrame()
{
	if (mDisplay != nullptr)
		mDisplay->flip();
	mDirty.clear();
}

void PGApp::addControl(PGControl* theControl)
{
	if (theControl != nullptr)
	{
		theControl->setOwner(this);
		mControls.push_front(theControl);
	}
}

void PGApp::removeControl(PGControl* theControl)
{
	if (theControl != nullptr)
		mControls.remove(theControl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Start and end

void PGApp::run(const char* theCmdLine, const char* theResourceDir)
{
	if (init(theCmdLine, theResourceDir))
		mainLoop();
	shutdown();
	cleanup();
}

bool PGApp::init(const char* theCmdLine, const char* theResourceDir)
{
	(void)theCmdLine;
	(void)theResourceDir;
	// 0x40dfb0 PGApp::registerCvars (slot 0x2c)
	mCvars.registerVariable("c_soundDriver", "default", 0);
	mCvars.registerVariable("c_dxversion", "0", 0);
	mCvars.registerVariable("pg_trace", "0", 0);
	mCvars.registerVariable("pg_journal", "0", 0);
	mCvars.registerVariable("pg_demoFile", "", 0);
	mCvars.registerVariable("pg_spriteCursor", "1", 0);
	mCvars.registerVariable("c_hklm", "0", 0);
	// the original parsed the command line into cvars and opened the trace, journal and demo files here
	HRegistry* aRegistry = GetRegistry();
	if (aRegistry->open(mCvars.getVariableString("companyName"), mCvars.getVariableString("productName")))
	{
		char aBuf[0x400];
		if (aRegistry->readString("disableLinks", aBuf, sizeof(aBuf)))
			mLinksDisabled = true;
		aRegistry->close();
	}
	return true;
}

void PGApp::shutdown()
{
	// 0x40dd30 (slot 0xa0): the sound
	setMusicOn(false);
	freeMusic();
	mMixer.stopAll();
	mSounds.freeAll();
	mHasAudio = false;
	// slot 0xa8 shut the input device; slot 0xa4 (0x40dbd0) the display
	if (mDisplay != nullptr)
	{
		mDisplay->shutdown();
		delete mDisplay;
	}
	mDisplay = nullptr;
}

void PGApp::cleanup()
{
	// 0x40e990: the controls are deleted
	while (!mControls.empty())
	{
		PGControl* aControl = mControls.front();
		mControls.pop_front();
		delete aControl;
	}
	closeResourceFile();
}

bool PGApp::openResourceFile(const char* theName)
{
	if (theName == nullptr)
		return false;
	// a name without ".pyz" was a folder (0x427350); port: always the archive
	mFileSystem = new HFileSystem();
	if (!mFileSystem->mount(theName))
	{
		PGMessageBox(0x31, "Missing Resource File", "Could not load the resource file '%s'", theName);
		return false;
	}
	return true;
}

void PGApp::closeResourceFile()
{
	if (mFileSystem != nullptr)
	{
		delete mFileSystem;
		mFileSystem = nullptr;
	}
}

bool PGApp::initDevices(bool theFullscreen, int theWidth, int theHeight, int theChannels, int theBits, int theRate)
{
	PGLog("...Creating display device\n");
	// 0x40db70 (slot 0x7c)
	mDisplay = new HDisplay();
	if (!mDisplay->init(theWidth, theHeight, theFullscreen))
	{
		delete mDisplay;
		mDisplay = nullptr;
		return false;
	}
	PGLog("...Creating input device\n");
	// 0x40dc00 (slot 0x80) made the DirectInput device; port: the host posts the input
	SetInput(&mInput);
	PGLog("...Creating audio device\n");
	// 0x40dc70 (slot 0x78) and 0x40e650 (slot 0x84 m_createAudioStream)
	mHasAudio = true;
	mSounds.setFormat(theRate, theChannels, theBits);
	mSounds.setFileSystem(mFileSystem);
	mSounds.setMixer(&mMixer);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Music

bool PGApp::loadMusic(const char* theName, int theLoopStart)
{
	// port: the platform's voices loop from the start, which is where the game loops (theLoopStart 0)
	(void)theLoopStart;
	if (!mHasAudio)
		return true;
	std::vector<uint8_t> aFile;
	if (mFileSystem == nullptr || !mFileSystem->readFile(theName, aFile))
	{
		PGLog("...***could not open theme music***\n");
		return false;
	}
	mMusic = Platform::Mixer::Get().LoadSoundData(aFile, theName);
	return mMusic != nullptr;
}

void PGApp::freeMusic()
{
	setMusicOn(false);
	mMusic.reset();
}

void PGApp::restartMusic()
{
	if (mMusic)
		mMixer.restartMusic();
}

void PGApp::setMusicOn(bool theOn)
{
	if (mHasAudio && mMusicOn != theOn && mMusic)
	{
		if (theOn)
		{
			mMixer.setMusic(mMusic, (uint8_t)(int)(mMusicVolume * 255.0f));
			mMusicVoice = 0;
			mMusicOn = theOn;
			return;
		}
		mMixer.stopMusic();
		mMusicOn = false;
	}
}

void PGApp::setMusicVolume(float theVolume)
{
	mMusicVolume = theVolume;
	if (0.0f <= theVolume)
	{
		if (1.0f < theVolume)
			mMusicVolume = 1.0f;
	}
	else
		mMusicVolume = 0.0f;
	if (mMusicVoice != -1)
		mMixer.setMusicVolume((uint8_t)(int)(mMusicVolume * 255.0f));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The loading screen

// 0x4c1d28: how much of the progress sprite is shown
static PGRect gLoadShown = {0, 0, 0, 0};

void PGApp::drawLoadProgress(float theProgress, const char* theText, HFont* theFont)
{
	if (theProgress < 0.0f || theProgress == 0.0f)
	{
		PGPoint anAt = {0, 0};
		mLoadBackground.drawAt(&anAt);
		if (mLoadBottomLeft.isValid())
		{
			anAt.y = (mDisplay->mHeight - mLoadBottomLeft.height()) - 1;
			anAt.x = 0;
			mLoadBottomLeft.drawAt(&anAt);
		}
		if (mLoadBottomRight.isValid())
		{
			anAt.x = (mDisplay->mWidth - mLoadBottomRight.width()) - 1;
			anAt.y = (mDisplay->mHeight - mLoadBottomRight.height()) - 1;
			mLoadBottomRight.drawAt(&anAt);
		}
		if (theText != nullptr && theFont != nullptr)
		{
			static const float kWhite[4] = {1.0f, 1.0f, 1.0f, 1.0f};
			PGRect aClip = {0, 0, mDisplay->mWidth, mDisplay->mHeight};
			mDisplay->drawText(theText, 10, 100, &aClip, 0, theFont, kWhite);
		}
		gLoadShown.left = mLoadAt.x;
		gLoadShown.bottom = mLoadProgress.height() + mLoadAt.y;
		gLoadShown.top = mLoadAt.y;
		gLoadShown.right = gLoadShown.left;
	}
	PGRect aDest = {gLoadShown.right, gLoadShown.top, (int)(mLoadProgress.width() * theProgress) + mLoadAt.x,
		gLoadShown.bottom};
	PGRect aSrc;
	mLoadProgress.getRect(&aSrc);
	aSrc.left = gLoadShown.right - mLoadAt.x;
	aSrc.right = (int)(mLoadProgress.width() * theProgress);
	if (aSrc.left < aSrc.right && aDest.left < aDest.right)
	{
		mLoadProgress.blitEx(&aDest, &aSrc, BLEND_ALPHA);
		gLoadShown = aDest;
	}
	mDisplay->flip();
}

void PGApp::releaseScreenSprites()
{
	mLoadBottomLeft.release();
	mLoadBackground.release();
	mLoadProgress.release();
	mLoadBottomRight.release();
}

bool PGApp::loadScreenSprites(const char* theBackground, const char* theBottomLeft, const char* theBottomRight,
	const char* theProgress, int theX, int theY)
{
	if (theBottomLeft != nullptr && !mLoadBottomLeft.load(mDisplay, mFileSystem, theBottomLeft))
		return false;
	if (theBottomRight != nullptr)
		mLoadBottomRight.load(mDisplay, mFileSystem, theBottomRight);
	if (!mLoadBackground.load(mDisplay, mFileSystem, theBackground))
		return false;
	if (!mLoadProgress.load(mDisplay, mFileSystem, theProgress))
		return false;
	mLoadAt.x = theX;
	mLoadAt.y = theY;
	return true;
}

bool PGApp::loadSprites(PGSpriteEntry* theTable, int theCount, float theTotal, const char* theText, HFont* theFont)
{
	float aStep = theTotal / (float)theCount;
	float aProgress = 0.0f;
	for (int i = 0; i < theCount; i++)
	{
		if (theTable[i].mName != nullptr)
		{
			drawLoadProgress(aProgress, theText, theFont);
			if (!theTable[i].mSprite->load(mDisplay, mFileSystem, theTable[i].mName))
			{
				PGLog("...could not load %s\n", theTable[i].mName);
				return false;
			}
		}
		aProgress = aProgress + aStep;
	}
	return true;
}

bool PGApp::loadSounds(const char* theMusic, int theLoopStart, const char** theSounds, float theFrom, float theTo)
{
	PGLog("...loading sounds\n");
	drawLoadProgress(theFrom, nullptr, nullptr);
	if (theMusic != nullptr)
		loadMusic(theMusic, theLoopStart);
	int aCount = 0;
	if (theSounds[0] != nullptr)
	{
		do
			aCount++;
		while (theSounds[aCount] != nullptr);
		if (aCount != 0)
		{
			float aRange = theTo - theFrom;
			std::shared_ptr<Platform::SoundData> aData;
			do
			{
				theFrom = theFrom + aRange / (float)aCount;
				drawLoadProgress(theFrom, nullptr, nullptr);
				mSounds.load(*theSounds, &aData);
				theSounds++;
			} while (*theSounds != nullptr);
			return true;
		}
	}
	return false;
}

bool PGApp::loadAssets(PGSpriteEntry* theTable, int theCount, const char* theMusic, int theLoopStart,
	const char** theSounds, const char* theBackground, const char* theBottomLeft, const char* theBottomRight,
	const char* theProgress, int theX, int theY, float theTotal, const char* theText, HFont* theFont)
{
	loadScreenSprites(theBackground, theBottomLeft, theBottomRight, theProgress, theX, theY);
	float aSprites = theTotal * 0.75f;
	if (!loadSprites(theTable, theCount, aSprites, theText, theFont))
	{
		PGMessageBox(0x31, "Error", "Failed to load bitmaps");
		return false;
	}
	loadSounds(theMusic, theLoopStart, theSounds, aSprites, theTotal);
	releaseScreenSprites();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The framework's dialogs

namespace
{

// vtable 0x463298
class MessageDialog : public PGDialog
{
public:
	PGDialogButton mOk;      // +0x44
	PGDialogButton mCancel;  // +0x188
	PGSprite mBackground;    // +0x2cc
	PGSprite mOkSprite;      // +0x320
	PGSprite mCancelSprite;  // +0x374
	PGSprite mOkLit;         // +0x3c8
	PGSprite mCancelLit;     // +0x41c

	// 0x4109d0
	bool onItemClicked(PGDialogItem* theItem) override { return true; }
	// 0x4109e0: the OK button shows once the timeout has passed
	bool onTimeout() override
	{
		mOk.setVisible(true);
		invalidate(mOk.getRect());
		return false;
	}
	// 0x410970
	void releaseAll()
	{
		mBackground.release();
		mCancelLit.release();
		mOkSprite.release();
		mCancelSprite.release();
		mOkLit.release();
		removeAll();
	}
};

// vtable 0x463250
class Slideshow : public PGDialog
{
public:
	PGDialogButton mClose;   // +0x44
	PGDialogButton mPrev;    // +0x188
	PGDialogButton mNext;    // +0x2cc
	PGSprite mCloseSprite;   // +0x410
	PGSprite mCloseLit;      // +0x464
	PGSprite mNextSprite;    // +0x4b8
	PGSprite mNextLit;       // +0x50c
	PGSprite mPrevSprite;    // +0x560
	PGSprite mPrevLit;       // +0x5b4
	std::vector<PGSprite> mPages;  // +0x608, the count at +0x60c
	int mPage;               // +0x610

	// 0x410430
	bool onItemClicked(PGDialogItem* theItem) override
	{
		if (theItem == &mClose)
			return true;
		if (theItem == &mNext)
			mPage = mPage + 1;
		else if (theItem == &mPrev)
			mPage = mPage - 1;
		int aCount = (int)mPages.size();
		if (aCount <= mPage)
			mPage = aCount - 1;
		if (mPage < 0)
			mPage = 0;
		mNext.setVisible(mPage != aCount - 1);
		mPrev.setVisible(mPage != 0);
		setBackground(&mPages[mPage]);
		invalidate(&mRect);
		return false;
	}
	// 0x410410: closing the window does not close it
	bool handleEvent(const PGEvent* theEvent) override
	{
		if (theEvent->mType != EV_QUIT)
			return PGDialog::handleEvent(theEvent);
		return true;
	}
};

} // namespace

int PGApp::messageDialog(int theX, int theY, int theTimeout, const char* theBackground, const char* theOk,
	const char* theOkLit, const char* theCancel, const char* theCancelLit)
{
	int aResult = 0;
	MessageDialog aDialog;
	HDisplay* aDisplay = mDisplay;
	aDialog.mBackground.load(aDisplay, mFileSystem, theBackground);
	aDialog.mOkSprite.load(aDisplay, mFileSystem, theOk);
	if (theCancel != nullptr)
		aDialog.mCancelSprite.load(aDisplay, mFileSystem, theCancel);
	if (theOkLit != nullptr)
		aDialog.mOkLit.load(aDisplay, mFileSystem, theOkLit);
	if (theCancelLit != nullptr)
		aDialog.mCancelLit.load(aDisplay, mFileSystem, theCancelLit);
	aDialog.setBackground(&aDialog.mBackground);
	if (theX == -1)
		theX = (aDisplay->mWidth - aDialog.mBackground.width()) / 2;
	if (theY == -1)
		theY = (aDisplay->mHeight - aDialog.mBackground.height()) / 2;
	PGPoint anAt = {theX, theY};
	aDialog.setPosition(&anAt);
	aDialog.mOk.setName("Are you sure, ok");
	aDialog.mOk.setSprites(&aDialog.mOkSprite, theOkLit == nullptr ? &aDialog.mOkSprite : &aDialog.mOkLit, nullptr);
	if (theTimeout != 0)
		aDialog.mOk.setVisible(false);
	if (theCancel != nullptr)
	{
		aDialog.mCancel.setName("Are you sure, cancel");
		aDialog.mCancel.setSprites(&aDialog.mCancelSprite,
			theCancelLit == nullptr ? &aDialog.mCancelSprite : &aDialog.mCancelLit, nullptr);
	}
	int aY = aDialog.mBackground.height() - (aDialog.mOkSprite.height() * 3) / 2;
	if (theOk != nullptr)
	{
		int aX = aDialog.mBackground.width() - aDialog.mOkSprite.width();
		if (theCancel == nullptr)
			aX = aX / 2;
		else
			aX = aX / 4;
		anAt = {aX, aY};
		aDialog.mOk.setPosition(&anAt);
		aDialog.addItem(&aDialog.mOk);
	}
	if (theCancel != nullptr)
	{
		int aX = ((aDialog.mBackground.width() - aDialog.mCancelSprite.width()) * 3) / 4;
		anAt = {aX, aY};
		aDialog.mCancel.setPosition(&anAt);
		aDialog.addItem(&aDialog.mCancel);
		aDialog.setDefault(&aDialog.mCancel);
	}
	PGDialogItem* anItem = aDialog.doModal(this, mDisplay, &mInput, theTimeout, 0);
	if (anItem == &aDialog.mOk)
		aResult = 1;
	else if (anItem == PGDialog::CLOSED)
		aResult = 2;
	aDialog.releaseAll();
	invalidateAll();
	return aResult;
}

bool PGApp::slideshow(int theX, int theY, const char* theClose, const char* theCloseLit, const char* theNext,
	const char* theNextLit, const char* thePrev, const char* thePrevLit, const char** thePages, int theCount)
{
	Slideshow aDialog;
	HDisplay* aDisplay = mDisplay;
	HFileSystem* aFileSystem = mFileSystem;
	// 0x4100e0: the layout
	aDialog.mPages.resize(theCount);
	aDialog.mPage = 0;
	for (int i = 0; i < theCount; i++)
		aDialog.mPages[i].load(aDisplay, aFileSystem, thePages[i]);
	PGPoint anAt = {theX, theY};
	aDialog.setPosition(&anAt);
	aDialog.setBackground(&aDialog.mPages[0]);
	aDialog.mCloseSprite.load(aDisplay, aFileSystem, theClose);
	aDialog.mCloseLit.load(aDisplay, aFileSystem, theCloseLit);
	aDialog.mNextSprite.load(aDisplay, aFileSystem, theNext);
	aDialog.mNextLit.load(aDisplay, aFileSystem, theNextLit);
	aDialog.mPrevSprite.load(aDisplay, aFileSystem, thePrev);
	aDialog.mPrevLit.load(aDisplay, aFileSystem, thePrevLit);
	PGSprite* aPage = &aDialog.mPages[0];
	aDialog.mClose.setSprites(&aDialog.mCloseSprite, &aDialog.mCloseLit, nullptr);
	anAt.x = aPage->width() / 4;
	anAt.y = aPage->height() - (aDialog.mCloseSprite.height() * 5) / 4;
	aDialog.mClose.setPosition(&anAt);
	aDialog.mPrev.setSprites(&aDialog.mPrevSprite, &aDialog.mPrevLit, nullptr);
	anAt.x = (aPage->width() * 2) / 3;
	anAt.y = aPage->height() - (aDialog.mPrevSprite.height() * 3) / 2;
	aDialog.mPrev.setPosition(&anAt);
	aDialog.mNext.setSprites(&aDialog.mNextSprite, &aDialog.mNextLit, nullptr);
	anAt.x = (aPage->width() * 2) / 3 + aDialog.mPrevSprite.width() * 2;
	anAt.y = aPage->height() - (aDialog.mNextSprite.height() * 3) / 2;
	aDialog.mNext.setPosition(&anAt);
	aDialog.mPrev.setVisible(false);
	if (theCount == 1)
		aDialog.mNext.setVisible(false);
	aDialog.addItem(&aDialog.mClose);
	aDialog.addItem(&aDialog.mPrev);
	aDialog.addItem(&aDialog.mNext);
	aDialog.doModal(this, mDisplay, &mInput, 0, 0);
	for (PGSprite& aSprite : aDialog.mPages)
		aSprite.release();
	aDialog.mCloseSprite.release();
	aDialog.mCloseLit.release();
	aDialog.mNextSprite.release();
	aDialog.mNextLit.release();
	aDialog.mPrevSprite.release();
	aDialog.mPrevLit.release();
	aDialog.removeAll();
	return true;
}

} // namespace pg
