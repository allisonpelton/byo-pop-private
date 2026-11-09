// The game's dialogs (0x408320..0x40bcc0): the name entry, the question, the best times, the loading screen, the
// "no more moves" choice and the options. Each is a PGDialog subclass on the stack of the function that runs it,
// with its sprites loaded from the archive for the time it is up. The original passes the app as doModal()'s last
// argument in some of them; the framework never reads it, and the port passes 0.

#include <cstdio>
#include <cstring>
#include <string>

#include "HDisplay.h"
#include "NingPo.h"
#include "PGDialog.h"
#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

namespace ningpo
{

using pg::PGDialog;
using pg::PGDialogButton;
using pg::PGDialogImage;
using pg::PGDialogItem;
using pg::PGDialogSlider;
using pg::PGDialogText;

namespace
{

const float YELLOW[4] = {1.0f, 1.0f, 0.0f, 1.0f};

// vtables 0x462c18 (the name entry) and 0x462d08 (the question): any click ends them
class ClickDialog : public PGDialog
{
public:
	// 0x408d80
	bool onItemClicked(PGDialogItem* theItem) override
	{
		GetNingPo()->playSound("nm_guiclick.wav", 0x80, 0, true);
		return true;
	}
};

// vtable 0x462d50 (0xce98 bytes): the best times of a layout, with arrows to the others
class BestTimesDialog : public PGDialog
{
public:
	int mLayout;                                       // +0x44
	BestTime mTimes[NUM_LAYOUTS][NUM_BEST_TIMES];      // +0x48 a copy of the game's
	PGSprite mBackgroundSprite;                        // +0x1b48
	PGSprite mOkLit;                                   // +0x1b9c
	PGSprite mClearLit;                                // +0x1bf0
	PGDialogButton mOk;                                // +0x1c44
	PGDialogButton mClear;                             // +0x1d88
	PGDialogButton mNext;                              // +0x1ecc
	PGDialogButton mPrev;                              // +0x2010
	PGDialogButton mTitle;                             // +0x2154 the layout's name
	PGDialogText mNames[NUM_BEST_TIMES];               // +0x2298
	PGDialogText mTotals[NUM_BEST_TIMES];              // +0x4d98 with the penalties
	PGDialogText mRaws[NUM_BEST_TIMES];                // +0x7898 without
	PGDialogText mTiles[NUM_BEST_TIMES];               // +0xa398 the tiles left

	void setup(pg::HDisplay* theDisplay, const pg::HFileSystem* theFileSystem, const BestTime* theTimes, int theLayout);
	void fillRows(const BestTime* theTimes);
	bool onItemClicked(PGDialogItem* theItem) override;
};

// 0x409a30: theTimes copied, and the rows of the shown layout written from them
void BestTimesDialog::fillRows(const BestTime* theTimes)
{
	memcpy(mTimes, theTimes, sizeof(mTimes));
	char aBuf[0x100];
	int aY = 0x94;
	for (int i = 0; i < NUM_BEST_TIMES; i++)
	{
		BestTime& aTime = mTimes[mLayout][i];
		pg::HSnprintf(aBuf, 0x100, "%s", aTime.mName);
		PGDialogText& aName = mNames[i];
		aName.setColor(YELLOW);
		aName.setEditable(false);
		aName.setText(aBuf);
		PGPoint aPos = {0x3c, aY};
		aName.setPosition(&aPos);
		aName.setFont(GetNingPo()->mFont);

		int aSeconds = aTime.mTime;
		if (5999 < aSeconds)
			aSeconds = 5999;
		pg::HSnprintf(aBuf, 0x100, "%02d:%02d", aSeconds / 60, aSeconds % 60);
		PGDialogText& aTotal = mTotals[i];
		aTotal.setColor(YELLOW);
		aTotal.setEditable(false);
		aTotal.setText(aBuf);
		aTotal.setFont(GetNingPo()->mFont);
		aTotal.setAlign(1);
		aPos = {0x159, aY};
		aTotal.setPosition(&aPos);

		aSeconds = aTime.mRawTime;
		if (5999 < aSeconds)
			aSeconds = 5999;
		pg::HSnprintf(aBuf, 0x100, "%02d:%02d", aSeconds / 60, aSeconds % 60);
		PGDialogText& aRaw = mRaws[i];
		aRaw.setColor(YELLOW);
		aRaw.setEditable(false);
		aRaw.setText(aBuf);
		aRaw.setFont(GetNingPo()->mFont);
		aRaw.setAlign(1);
		aPos = {0x1b8, aY};
		aRaw.setPosition(&aPos);

		pg::HSnprintf(aBuf, 0x100, "%ld", (long)aTime.mTilesLeft);
		PGDialogText& aTiles = mTiles[i];
		aTiles.setColor(YELLOW);
		aTiles.setEditable(false);
		aTiles.setText(aBuf);
		aTiles.setFont(GetNingPo()->mFont);
		aTiles.setAlign(1);
		aPos = {0x104, aY};
		aTiles.setPosition(&aPos);

		aY += 0x23;
	}
}

// 0x409020 (the original writes the rows inline, as 0x409a30 does)
void BestTimesDialog::setup(pg::HDisplay* theDisplay, const pg::HFileSystem* theFileSystem, const BestTime* theTimes,
	int theLayout)
{
	NingPo* anApp = GetNingPo();
	mLayout = theLayout;
	mClearLit.load(theDisplay, theFileSystem, "besttimes_clear_lit_np.tga");
	mOkLit.load(theDisplay, theFileSystem, "ok_lit_np.tga");
	mBackgroundSprite.load(theDisplay, theFileSystem, "besttimes_np.jpg");
	PGPoint aPos = {0x7d, 0};
	setPosition(&aPos);
	setBackground(&mBackgroundSprite);

	mOk.setSprites(nullptr, &mOkLit, nullptr);
	aPos = {0x149, 0x1a5};
	mOk.setPosition(&aPos);
	mClear.setSprites(nullptr, &mClearLit, nullptr);
	aPos = {0x6d, 0x1a1};
	mClear.setPosition(&aPos);
	mNext.setSprites(anApp->getSprite(SPRITE_ARROW_RIGHT), anApp->getSprite(SPRITE_ARROW_RIGHT_LIT), nullptr);
	aPos = {0x152, 0x3c};
	mNext.setPosition(&aPos);
	mPrev.setSprites(anApp->getSprite(SPRITE_ARROW_LEFT), anApp->getSprite(SPRITE_ARROW_LEFT_LIT), nullptr);
	aPos = {0x91, 0x3c};
	mPrev.setPosition(&aPos);
	mTitle.setSprites(anApp->getSprite(mLayout + SPRITE_TITLE_LAYOUT), anApp->getSprite(mLayout + SPRITE_TITLE_LAYOUT),
		nullptr);
	aPos = {(mBackgroundSprite.width() - anApp->getSprite(SPRITE_TITLE_LAYOUT)->width()) / 2, 0x44};
	mTitle.setPosition(&aPos);
	addItem(&mOk);
	addItem(&mClear);
	addItem(&mNext);
	addItem(&mPrev);
	addItem(&mTitle);

	fillRows(theTimes);
	for (int i = 0; i < NUM_BEST_TIMES; i++)
	{
		addItem(&mNames[i]);
		addItem(&mTotals[i]);
		addItem(&mRaws[i]);
		addItem(&mTiles[i]);
	}
}

// 0x4095d0: OK (or any item but the buttons) ends it; Clear asks first
bool BestTimesDialog::onItemClicked(PGDialogItem* theItem)
{
	NingPo* anApp = GetNingPo();
	if (theItem == &mOk)
	{
		anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
		return true;
	}
	if (theItem == &mClear)
	{
		anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
		if (anApp->askYesNo("besttimes_areyousure.jpg", "besttimes_areyousure_yes.tga",
				"besttimes_areyousure_yes_lit.tga", "besttimes_areyousure_no.tga", "besttimes_areyousure_no_lit.tga"))
		{
			anApp->resetBestTimes(mLayout);
			// the original repeats 0x409a30 inline here
			fillRows(&anApp->mBestTimes[0][0]);
			anApp->saveBestTimes();
			return false;
		}
	}
	else
	{
		if (theItem != &mNext && theItem != &mPrev)
			return true;
		anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
		if (theItem == &mPrev)
		{
			mLayout--;
			if (mLayout < 0)
				mLayout = NUM_LAYOUTS - 1;
		}
		else if (theItem == &mNext)
		{
			mLayout++;
			if (NUM_LAYOUTS - 1 < mLayout)
				mLayout = 0;
		}
		fillRows(&anApp->mBestTimes[0][0]);
		mTitle.setSprites(anApp->getSprite(mLayout + SPRITE_TITLE_LAYOUT), anApp->getSprite(mLayout + SPRITE_TITLE_LAYOUT),
			nullptr);
		invalidate(&mRect);
	}
	return false;
}

// vtable 0x462e18 (0xea0 bytes): the loading screen, which ends with a click on its button. The trial filled its
// progress bar over the seconds of play it had left (0x40a670), wrote how often it had been played (the draw
// 0x40a170 sets that line's font) and had a button to buy the game; the port has the registered game's screen.
class StartupDialog : public PGDialog
{
public:
	PGSprite mBackgroundSprite;  // +0x44
	PGSprite mClick;             // +0x98
	PGSprite mPartner;           // +0x140
	PGSprite mProgressSprite;    // +0x1e8
	PGSprite mClickLit;          // +0x23c
	PGDialogButton mPartnerItem; // +0x428 hidden: a picture
	PGDialogImage mProgress;     // +0x56c
	PGDialogButton mClickItem;   // +0xc08
	bool mQuit = false;          // +0xe9c the window was closed

	void setup(pg::HDisplay* theDisplay, const pg::HFileSystem* theFileSystem);

	// 0x40a670
	bool handleEvent(const PGEvent* theEvent) override
	{
		if (theEvent->mType == pg::EV_QUIT)
		{
			mQuit = true;
			return false;
		}
		return PGDialog::handleEvent(theEvent);
	}

	// 0x40a7f0
	bool onItemClicked(PGDialogItem* theItem) override
	{
		if (theItem == &mClickItem)
		{
			GetNingPo()->playSound("nm_guiclick.wav", 0x80, 0, true);
			return true;
		}
		return false;
	}
};

// 0x40a1a0; port: the thanks for registering (demo_menu_thanks_np.tga, a hidden picture) is left out with the trial
void StartupDialog::setup(pg::HDisplay* theDisplay, const pg::HFileSystem* theFileSystem)
{
	mBackgroundSprite.load(theDisplay, theFileSystem, "loading_np.jpg");
	PGRect aRect;
	mBackgroundSprite.getRect(&aRect);
	mClick.load(theDisplay, theFileSystem, "demo_menu_click_np.tga");
	mClickLit.load(theDisplay, theFileSystem, "demo_menu_click_lit_np.tga");
	mProgressSprite.load(theDisplay, theFileSystem, "progress_np.tga");
	mPartner.load(theDisplay, theFileSystem, "loading_partner_np.jpg");

	PGPoint aPos = {(aRect.right - mClick.width()) / 2, 0x19f};
	mClickItem.setPosition(&aPos);
	aPos = {0x78, 0x150};
	mProgress.setPosition(&aPos);
	mProgress.setSprite(&mProgressSprite);
	mProgress.setProgress(1.0f);
	mClickItem.setSprites(&mClick, &mClickLit, nullptr);
	if (mPartner.isValid())
	{
		aPos = {aRect.right - mPartner.width() - 1, aRect.bottom - mPartner.height() - 1};
		mPartnerItem.setPosition(&aPos);
		mPartnerItem.setSprites(&mPartner, &mPartner, &mPartner);
		mPartnerItem.setVisible(false);
	}

	aPos = {0, 0};
	setPosition(&aPos);
	addItem(&mPartnerItem);
	addItem(&mProgress);
	addItem(&mClickItem);
	setBackground(&mBackgroundSprite);
}

// vtable 0x462e70: shuffle, undo or end the game
class NoMovesDialog : public PGDialog
{
public:
	PGDialogButton* mShuffle;  // +0x518
	PGDialogButton* mUndo;     // +0x3d4
	PGDialogButton* mEndGame;  // +0x290

	// 0x40ae70
	bool onItemClicked(PGDialogItem* theItem) override
	{
		NingPo* anApp = GetNingPo();
		if (theItem == mEndGame)
		{
			anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
			return true;
		}
		if (theItem == mShuffle)
		{
			anApp->shuffle();
			return true;
		}
		if (theItem == mUndo)
			anApp->undo();
		return true;
	}
};

// vtable 0x462f00: the sound's slider
class SoundSlider : public PGDialogSlider
{
public:
	// 0x40bc50
	bool handleEvent(const PGEvent* theEvent) override
	{
		float anOld = getValue();
		bool aResult = PGDialogSlider::handleEvent(theEvent);
		if (getValue() != anOld)
			GetNingPo()->mSoundSetting = getValue();
		return aResult;
	}
};

// vtable 0x462f70: the music's slider, which also sets the playing music's volume
class MusicSlider : public PGDialogSlider
{
public:
	// 0x40bcc0
	bool handleEvent(const PGEvent* theEvent) override
	{
		float anOld = getValue();
		bool aResult = PGDialogSlider::handleEvent(theEvent);
		if (getValue() != anOld)
		{
			NingPo* anApp = GetNingPo();
			anApp->mMusicSetting = getValue();
			anApp->setMusicVolume(anApp->mMusicSetting);
		}
		return aResult;
	}
};

// vtable 0x462eb8 (0xf08 bytes): the options. The trial had a button to register here, and every copy one to visit the
// partner's site, between Best Times and Done (both left out: docs/README.md, "Left out").
class OptionsDialog : public PGDialog
{
public:
	NingPo* mSettings;           // +0x44 the game's settings (+0x8240)
	PGDialogButton mOk;          // +0x48
	PGSprite mBackgroundSprite;  // +0x18c
	PGSprite mDone;              // +0x1e0
	PGSprite mDoneLit;           // +0x234
	SoundSlider mSound;          // +0x288
	MusicSlider mMusic;          // +0x3d0
	PGDialogButton mFullscreen;  // +0x518
	PGDialogButton mAutohint;    // +0x65c
	PGDialogButton mBestTimes;   // +0x7a0
	PGSprite mBestTimesLit;      // +0xc14
	PGSprite mBestTimesSprite;   // +0xc68
	PGSprite mTab;               // +0xcbc the sliders' knob
	PGSprite mOff;               // +0xdb8
	PGSprite mOffLit;            // +0xe0c
	PGSprite mOn;                // +0xe60
	PGSprite mOnLit;             // +0xeb4

	void setup(pg::HDisplay* theDisplay, const pg::HFileSystem* theFileSystem, NingPo* theSettings);
	void releaseSprites();
	bool onItemClicked(PGDialogItem* theItem) override;
};

// 0x40b3d0
// port: paints out the options' empty bottom frame (x 61 to 462, y 393 to 436 in options_np.jpg) with the wood beside
// it. Its grooved edges and their shadows, 12 pixels across, take the 12 pixels just outside them: the left and right
// edges first, then the top and bottom across the whole width, so the corners take wood too.
static void PaintOutFrame(pg::HImage& theImage)
{
	const int aBytes = theImage.bytesPerPixel();
	if (theImage.mWidth < 480 || theImage.mHeight < 460 || aBytes != 4)
		return;
	uint8_t* aBits = theImage.mBits.data();
	const int aPitch = theImage.pitch();
	auto aColumns = [&](int theX, int theCount, int theFrom)
	{
		for (int y = 390; y < 444; y++)
			memcpy(aBits + y * aPitch + theX * aBytes, aBits + y * aPitch + theFrom * aBytes, theCount * aBytes);
	};
	auto aRows = [&](int theY, int theCount, int theFrom)
	{
		for (int i = 0; i < theCount; i++)
			memcpy(aBits + (theY + i) * aPitch + 55 * aBytes, aBits + (theFrom + i) * aPitch + 55 * aBytes,
				(466 - 55) * aBytes);
	};
	aColumns(55, 12, 43);
	aColumns(456, 10, 466);
	aRows(390, 12, 378);
	aRows(433, 11, 444);
}

void OptionsDialog::setup(pg::HDisplay* theDisplay, const pg::HFileSystem* theFileSystem, NingPo* theSettings)
{
	mSettings = theSettings;
	mTab.load(theDisplay, theFileSystem, "options_slider_tab_np.tga");
	mDone.load(theDisplay, theFileSystem, "options_done_np.tga");
	mDoneLit.load(theDisplay, theFileSystem, "options_done_lit_np.tga");
	// port: in the third row, where the visit to the partner's site was (left out: docs/README.md, "Left out"); the
	// original's Done was at (0x3f, 0x18c), in the fourth
	PGPoint aPos = {0x3f, 0x14e};
	mOk.setPosition(&aPos);
	mOk.setSprites(&mDone, &mDoneLit, nullptr);
	mBestTimesSprite.load(theDisplay, theFileSystem, "options_besttimes_np.tga");
	mBestTimesLit.load(theDisplay, theFileSystem, "options_besttimes_lit_np.tga");
	aPos = {0x3f, 0x10d};
	mBestTimes.setPosition(&aPos);
	mBestTimes.setSprites(&mBestTimesSprite, &mBestTimesLit, nullptr);
	mOn.load(theDisplay, theFileSystem, "options_on_np.tga");
	mOnLit.load(theDisplay, theFileSystem, "options_on_lit_np.tga");
	mOff.load(theDisplay, theFileSystem, "options_off_np.tga");
	mOffLit.load(theDisplay, theFileSystem, "options_off_lit_np.tga");
	// port: the background has a frame for each button; the fourth row's, empty now that Done is in the third, is
	// painted out (PaintOutFrame)
	pg::HImage aBackground;
	if (pg::LoadImage(theFileSystem, "options_np.jpg", aBackground))
	{
		PaintOutFrame(aBackground);
		mBackgroundSprite.create(theDisplay, aBackground);
	}
	setBackground(&mBackgroundSprite);

	aPos = {0xfa, 0x77};
	mSound.setPosition(&aPos);
	mSound.setKnob(&mTab);
	mSound.setRange(0, 0xa0);
	mSound.setValue(GetNingPo()->mSoundSetting);
	aPos = {0xfa, 0xa6};
	mMusic.setPosition(&aPos);
	mMusic.setKnob(&mTab);
	mMusic.setRange(0, 0xa0);
	mMusic.setValue(GetNingPo()->mMusicSetting);

	aPos = {0xaf, 0xda};
	mFullscreen.setPosition(&aPos);
	if (theSettings->mFullscreen == 0)
		mFullscreen.setSprites(&mOff, &mOffLit, nullptr);
	else
		mFullscreen.setSprites(&mOn, &mOnLit, nullptr);
	aPos = {0x1b2, 0xda};
	mAutohint.setPosition(&aPos);
	if (theSettings->mAutohint == 0)
		mAutohint.setSprites(&mOff, &mOffLit, nullptr);
	else
		mAutohint.setSprites(&mOn, &mOnLit, nullptr);

	aPos = {0x7d, 0};
	setPosition(&aPos);
	// the trial added its register button first
	addItem(&mBestTimes);
	addItem(&mSound);
	addItem(&mMusic);
	addItem(&mFullscreen);
	addItem(&mAutohint);
	addItem(&mOk);
}

void OptionsDialog::releaseSprites()
{
	mOn.release();
	mOff.release();
	mOnLit.release();
	mOffLit.release();
	mBestTimesSprite.release();
	mBestTimesLit.release();
	mDone.release();
	mDoneLit.release();
	mTab.release();
	mBackgroundSprite.release();
}

// 0x40b890: Done and Visit end it; the switches flip their setting
bool OptionsDialog::onItemClicked(PGDialogItem* theItem)
{
	NingPo* anApp = GetNingPo();
	if (theItem == &mOk)
	{
		anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
		return true;
	}
	if (theItem == &mFullscreen)
	{
		mSettings->mFullscreen = mSettings->mFullscreen == 0;
		if (mSettings->mFullscreen == 0)
			mFullscreen.setSprites(&mOff, &mOffLit, nullptr);
		else
			mFullscreen.setSprites(&mOn, &mOnLit, nullptr);
	}
	if (theItem == &mBestTimes)
		anApp->showBestTimes(anApp->mLayout);
	if (theItem == &mAutohint)
	{
		mSettings->mAutohint = mSettings->mAutohint == 0;
		anApp->mLastAction = anApp->mEventTime;
		if (mSettings->mAutohint == 0)
			mAutohint.setSprites(&mOff, &mOffLit, nullptr);
		else
			mAutohint.setSprites(&mOn, &mOnLit, nullptr);
	}
	if (anApp->mFullscreen == 0)
		mFullscreen.setSprites(&mOff, &mOffLit, nullptr);
	return false;
}

} // namespace

// 0x408320: the name for a best time, into theName; the account's name to start with
void NingPo::enterName(char* theName, int theSize)
{
	ClickDialog aDialog;
	PGSprite aBackground, anOk, anOkLit;
	PGDialogButton aButton;
	PGDialogText anEdit, aScore;
	anEdit.setMaxLength(0x10);
	aScore.setMaxLength(0x10);
	aBackground.load(mDisplay, mFileSystem, "entername_np.jpg");
	anOk.load(mDisplay, mFileSystem, "register_ok_np.tga");
	anOkLit.load(mDisplay, mFileSystem, "register_ok_lit_np.tga");
	aButton.setSprites(&anOk, &anOkLit, nullptr);
	PGPoint aPos = {(aBackground.width() - anOk.width()) / 2, aBackground.height() - (anOk.height() * 3) / 2};
	aButton.setPosition(&aPos);

	// GetUserNameA into 0x18 characters, which fails for a longer name
	std::string aUser = pg::GetHost()->UserName();
	if (aUser.size() + 1 > 0x18)
		aUser.clear();
	anEdit.setFont(mFont);
	anEdit.setMaxLength(0xc);
	anEdit.setText(aUser.c_str());
#ifdef __EMSCRIPTEN__
	{
		// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with the site's player
		std::string aSite = SiteScoreName();
		if (!aSite.empty())
			anEdit.setText(aSite.c_str());
	}
#endif
	aPos = {0x1e, 0xa4};
	anEdit.setPosition(&aPos);
	anEdit.mRect = {0x1e, 0xa4, 0x82, 0xb0};
	anEdit.setEditable(true);
	anEdit.setColor(YELLOW);

	char aBuf[0x100];
	pg::HSnprintf(aBuf, 0x100, "%02d:%02d", mFinalTime / 60, mFinalTime % 60);
	static const float GREEN[4] = {0.0f, 0.7f, 0.0f, 1.0f};
	aScore.setColor(GREEN);
	aScore.setText(aBuf);
	aScore.setAlign(2);
	aScore.setFont(mFont);
	aPos = {aBackground.width() / 2, 0x55};
	aScore.setPosition(&aPos);
	aScore.setEditable(false);

	aDialog.setBackground(&aBackground);
	aPos = {(mDisplay->mWidth - aBackground.width() - 0x7d) / 2 + 0x7d, (mDisplay->mHeight - aBackground.height()) / 2};
	aDialog.setPosition(&aPos);
	aDialog.addItem(&aButton);
	aDialog.addItem(&aScore);
	aDialog.addItem(&anEdit);
	aDialog.doModal(this, mDisplay, &mInput, 0, 0);
	pg::HStrncpy(theName, anEdit.getText(), theSize);
	aBackground.release();
	anOk.release();
	anOkLit.release();
	aDialog.removeAll();
}

// 0x408990: true when the player clicked the first button
bool NingPo::askYesNo(const char* theBackground, const char* theOk, const char* theOkLit, const char* theCancel,
	const char* theCancelLit)
{
	beginPause();
	ClickDialog aDialog;
	PGDialogButton anOkItem, aCancelItem;
	PGSprite aBackground, anOk, anOkLit, aCancel, aCancelLit;
	aBackground.load(mDisplay, mFileSystem, theBackground);
	anOk.load(mDisplay, mFileSystem, theOk);
	aCancel.load(mDisplay, mFileSystem, theCancel);
	anOkLit.load(mDisplay, mFileSystem, theOkLit);
	aCancelLit.load(mDisplay, mFileSystem, theCancelLit);
	aDialog.setBackground(&aBackground);
	PGPoint aPos = {(0x280 - aBackground.width()) / 2 + 0x3e, (0x1e0 - aBackground.height()) / 2 + 0x1e};
	aDialog.setPosition(&aPos);
	anOkItem.setSprites(&anOk, &anOkLit, nullptr);
	aCancelItem.setSprites(&aCancel, &aCancelLit, nullptr);
	int aY = aBackground.height() - (anOk.height() * 3) / 2;
	aPos = {(aBackground.width() - anOk.width()) / 4, aY};
	anOkItem.setPosition(&aPos);
	aPos = {((aBackground.width() - aCancel.width()) * 3) / 4, aY};
	aCancelItem.setPosition(&aPos);
	aDialog.addItem(&anOkItem);
	aDialog.addItem(&aCancelItem);
	aDialog.setDefault(&aCancelItem);
	PGDialogItem* aClicked = aDialog.doModal(this, mDisplay, &mInput, 0, 0);
	anOkLit.release();
	aCancelLit.release();
	anOk.release();
	aCancel.release();
	aBackground.release();
	aDialog.removeAll();
	endPause();
	return aClicked == &anOkItem;
}

// 0x408da0
void NingPo::showBestTimes(int theLayout)
{
	// 0xce98 bytes, on the heap rather than the stack
	BestTimesDialog* aDialog = new BestTimesDialog();
	beginPause();
	aDialog->setup(mDisplay, mFileSystem, &mBestTimes[0][0], theLayout);
	aDialog->doModal(this, mDisplay, &mInput, 0, 0);
	aDialog->mBackgroundSprite.release();
	aDialog->mOkLit.release();
	aDialog->mClearLit.release();
	aDialog->removeAll();
	endPause();
	delete aDialog;
}

// 0x409e90: the loading screen; false when the window was closed
bool NingPo::startupDialog()
{
	StartupDialog aDialog;
	beginPause();
	aDialog.setup(mDisplay, mFileSystem);
	aDialog.doModal(this, mDisplay, &mInput, 0, 0);
	bool anOk = !aDialog.mQuit;
	aDialog.mBackgroundSprite.release();
	aDialog.mClick.release();
	aDialog.mClickLit.release();
	aDialog.mPartner.release();
	aDialog.mProgressSprite.release();
	aDialog.removeAll();
	endPause();
	return anOk;
}

// 0x40aa60: true ends the game; no shuffle is offered with a single free tile
bool NingPo::noMoreMoves()
{
	NoMovesDialog aDialog;
	PGSprite aBackground, aShuffle, anUndo, anEndGame, aShuffleLit, anUndoLit, anEndGameLit;
	PGDialogButton anEndGameItem, anUndoItem, aShuffleItem;
	aDialog.mShuffle = &aShuffleItem;
	aDialog.mUndo = &anUndoItem;
	aDialog.mEndGame = &anEndGameItem;
	beginPause();
	int aFree = mBoard.countFree();
	aBackground.load(mDisplay, mFileSystem, "no_more_moves.jpg");
	PGRect aRect;
	aBackground.getRect(&aRect);
	aShuffle.load(mDisplay, mFileSystem, "no_more_moves_shuffle.tga");
	aShuffleLit.load(mDisplay, mFileSystem, "no_more_moves_shuffle_lit.tga");
	anUndo.load(mDisplay, mFileSystem, "no_more_moves_undo.tga");
	anUndoLit.load(mDisplay, mFileSystem, "no_more_moves_undo_lit.tga");
	anEndGame.load(mDisplay, mFileSystem, "no_more_moves_endgame.tga");
	anEndGameLit.load(mDisplay, mFileSystem, "no_more_moves_endgame_lit.tga");
	PGPoint aPos = {(aRect.right - aShuffle.width()) / 2, 0x7d};
	aShuffleItem.setPosition(&aPos);
	aPos = {(aRect.right - anUndo.width()) / 2 - 2, 0xa0};
	anUndoItem.setPosition(&aPos);
	aPos = {(aRect.right - anEndGame.width()) / 2, 0xc3};
	anEndGameItem.setPosition(&aPos);
	aShuffleItem.setSprites(&aShuffle, &aShuffleLit, nullptr);
	anUndoItem.setSprites(&anUndo, &anUndoLit, nullptr);
	anEndGameItem.setSprites(&anEndGame, &anEndGameLit, nullptr);
	aPos = {(0x280 - aRect.right) / 2 + 0x3f, (0x1e0 - aRect.bottom) / 2 + 0x2d};
	aDialog.setPosition(&aPos);
	if (aFree != 1)
		aDialog.addItem(&aShuffleItem);
	aDialog.addItem(&anUndoItem);
	aDialog.addItem(&anEndGameItem);
	aDialog.setBackground(&aBackground);
	PGDialogItem* aClicked = aDialog.doModal(this, mDisplay, &mInput, 0, 0);
	aBackground.release();
	aShuffle.release();
	anUndo.release();
	anEndGame.release();
	aShuffleLit.release();
	anUndoLit.release();
	anEndGameLit.release();
	aDialog.removeAll();
	endPause();
	return aClicked == &anEndGameItem;
}

// 0x40aff0: the options; a change of the full-screen box takes effect once they close
void NingPo::showOptions()
{
	OptionsDialog aDialog;
	beginPause();
	int aWasFullscreen = mFullscreen;
	aDialog.setup(mDisplay, mFileSystem, this);
	aDialog.doModal(this, mDisplay, &mInput, 0, 0);
	aDialog.releaseSprites();
	aDialog.removeAll();
	if (aWasFullscreen != mFullscreen)
	{
		mFullscreen = mFullscreen == 0;
		toggleFullscreen();
	}
	saveSettings();
	endPause();
}

} // namespace ningpo
