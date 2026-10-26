#include "NcApp.h"
#include "Image.h"
#include "DDImage.h"
#include "ImageFont.h"
#include "Dialog.h"
#include "SoundManager.h"
#include "MusicInterface.h"
#include <SDL.h>

using namespace Sexy;

NcApp::NcApp()
{
	mImageNames = NULL;
	mImageGroups = NULL;
	mNumImages = 0;
	mNumFonts = 0;
	mPauseCount = 0;
	mImages = NULL;
	mFonts = NULL;
	mHintsDisabled = false;
	mQuitRequested = false;
}

NcApp::~NcApp()
{
	if (mImages != NULL)
	{
		for (int i = 0; i < mNumImages; i++)
			delete mImages[i];
		delete[] mImages;
	}
	if (mFonts != NULL)
	{
		for (int i = 0; i < mNumFonts; i++)
			delete mFonts[i];
		delete[] mFonts;
	}
}

// 0x4e36c0 slot 17: the base class's Init, then the trial bookkeeping (0x4e3350: MaxExecutions, MaxPlays and
// MaxTime from the properties and the NoReg/NoPopLink flags; only the full version is ported)
void NcApp::Init()
{
	SexyApp::Init();
}

// 0x4e3740: the names of the songs, loaded one after the other (music\name.ogg)
void NcApp::LoadMusicNames(const char* const* theNames, int theCount)
{
	for (int i = 0; i < theCount; i++)
	{
		mMusicInterface->LoadMusic(i, theNames[i]);
		mCompletedLoadingThreadTasks++;
	}
}

// 0x4e3820
void NcApp::LoadFonts(const char* const* theNames, int theCount)
{
	mNumFonts = theCount;
	mFonts = new ImageFont*[theCount];
	for (int i = 0; i < theCount; i++)
		mFonts[i] = NULL;
	for (int i = 0; i < theCount; i++)
	{
		mFonts[i] = new ImageFont(this, theNames[i]);
		mCompletedLoadingThreadTasks++;
	}
}

// 0x4e40a0: the image is stored, even when it did not load
bool NcApp::LoadImage(int theIndex)
{
	SexyString aPath = SexyString("images\\") + mImageNames[theIndex];
	mImages[theIndex] = GetImage(aPath);
	if (mImages[theIndex] == NULL)
	{
		Popup("Failed to load " + aPath);
		mLoadingFailed = true;
		return false;
	}
	return true;
}

// 0x4e4250
void NcApp::LoadImages(const char* const* theNames, const unsigned char* theGroups, int theCount)
{
	mImageNames = theNames;
	mImageGroups = theGroups;
	mNumImages = theCount;
	mImages = new Image*[theCount];
	for (int i = 0; i < theCount; i++)
		mImages[i] = NULL;
	for (int i = 0; i < theCount; i++)
	{
		if (!LoadImage(i))
		{
			Popup(SexyString("Failed to load images\\") + theNames[i]);
			mLoadingFailed = true;
			return;
		}
		mCompletedLoadingThreadTasks++;
	}
}

// 0x4e4410: without a sound device the tasks are only counted
void NcApp::LoadSounds(const char* const* theNames, int theCount)
{
	if (!mSoundManager->Initialized())
	{
		mCompletedLoadingThreadTasks += theCount;
		return;
	}
	for (int i = 0; i < theCount; i++)
	{
		if (!mSoundManager->LoadSound(i, SexyString("sounds\\") + theNames[i]))
		{
			Popup(SexyString("Failed to load sounds\\") + theNames[i]);
			mLoadingFailed = true;
			return;
		}
		mCompletedLoadingThreadTasks++;
	}
}

// 0x4e2c10: the four cursors, images by index
void NcApp::SetCursorImages(int thePointer, int theHand, int theDragging, int theText)
{
	SetCursorImage(CURSOR_POINTER, mImages[thePointer]);
	SetCursorImage(CURSOR_HAND, mImages[theHand]);
	SetCursorImage(CURSOR_DRAGGING, mImages[theDragging]);
	SetCursorImage(CURSOR_TEXT, mImages[theText]);
}

// 0x4e2d50: the dialog's centre on the screen's
void NcApp::CenterDialog(Dialog* theDialog)
{
	if (theDialog != NULL)
		theDialog->Resize(mWidth / 2 - theDialog->mWidth / 2, mHeight / 2 - theDialog->mHeight / 2,
			theDialog->mWidth, theDialog->mHeight);
}

// 0x4e3bb0: Pause(true) and Pause(false) nest; an unpaired Pause(false) does nothing. The game's handler
// (slot 84) hears the change: was it paused, is it now, the count.
void NcApp::Pause(bool thePause)
{
	int aCount = mPauseCount;
	if ((aCount == 0) && (!thePause))
		return;
	mPauseCount = thePause ? aCount + 1 : aCount - 1;
	HandlePaused(aCount > 0, mPauseCount > 0, mPauseCount);
}

// 0x4e3aa0 / 0x4e3b20: the hint switch is a registry boolean
void NcApp::SetHintsDisabled(bool theDisabled)
{
	RegistryWriteBoolean("HintsDisabled", theDisabled);
}

bool NcApp::GetHintsDisabled()
{
	bool aValue = false;
	if (!RegistryReadBoolean("HintsDisabled", &aValue))
		return false;
	return aValue;
}

// 0x4e2e00 slot 4: a dialog's yes/no press. 2000 + id and 3000 + id are the Dialog2004 buttons; only dialog 5
// (the registration box) answers a press, with its tick sound; not ported
void NcApp::ButtonPress(int theId)
{
}

// 0x4e4920 slot 5: the buttons of the dialogs every Nuclide game has. Translated with the dialogs in
// NcDialogs.cpp; the game's own ButtonDepress handles its dialogs first and calls this for the rest.
void NcApp::ButtonDepress(int theId)
{
	int anId = theId % 10000;
	if ((anId >= 2000) && (anId < 3000))
	{
		anId = theId - 2000;
		switch (theId)
		{
		case 2004:		// the quit question's yes
		case 2014:
			KillDialog(anId);
			mQuitRequested = true;
			return;
		}
		KillDialog(anId);
		return;
	}
	if ((anId >= 3000) && (anId < 4000))
	{
		anId = theId - 3000;
		KillDialog(anId);
		return;
	}
}
