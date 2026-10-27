// The game (see NingPo.h): start-up, the events and the flow of a game.

#include "NingPo.h"
#include "GameText.h"
#include "HDisplay.h"
#include "HFileSystem.h"
#include "PGControl.h"
#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

#include <cstdio>
#include <cstring>

namespace ningpo
{

static NingPo* gNingPo = nullptr;

NingPo* GetNingPo()
{
	return gNingPo;
}

// 0x401330's names (SpriteTable.inc); sprites 8 and 117 get theirs from the player's choices
static const char* const SPRITE_NAMES[NUM_SPRITES] =
{
#include "SpriteTable.inc"
};

// 0x467110: the layouts' files
static const char* const LAYOUT_FILES[NUM_LAYOUTS] =
{
	"classic.nml", "coliseum.nml", "pyramid.nml", "quin.nml", "temple.nml", "hurricane.nml", "pyro.nml",
	"pyrogon0.nml", "pyrogon1.nml", "pyrogon2.nml", "pyrogon3.nml", "pyrogon4.nml", "pyrogon5.nml", "pyrogon6.nml",
	"pyrogon7.nml", "pyrogon8.nml", "pyrogon9.nml", "pyrogon10.nml",
};

// 0x4671a0: the tile sets' sheets
static const char* const TILESET_FILES[NUM_TILESETS] =
{
	"tileset_1.tga", "tileset_2.tga", "tileset_popcap.tga", "tileset_3.tga", "tileset_4.tga", "tileset_5.tga",
	"tileset_6.tga", "tileset_7.tga", "tileset_8.tga",
};

// 0x4671e8: the arenas' backgrounds
static const char* const BACKGROUND_FILES[NUM_BACKGROUNDS] =
{
	"arena10_np.jpg", "arena1_np.jpg", "arena2_np.jpg", "arena3_np.jpg", "arena4_np.jpg", "arena5_np.jpg",
	"arena6_np.jpg", "arena7_np.jpg", "arena8_np.jpg", "arena9_np.jpg",
};

// 0x462280: how far the "show moves" button lights the free tiles, by tile set
static const float SHOW_MOVES_LIGHT[NUM_TILESETS] = {0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f};

// 0x4670a0: the sounds loaded with the sprites (the game over and splash sounds load when they first play)
static const char* PRELOADED_SOUNDS[] =
{
	"nm_guiclick.wav", "nm_click.wav", "nm_unclick.wav", "nm_match.ogg", "nm_blocked.ogg", "nm_youwin.ogg",
	"nm_hint.ogg", nullptr,
};

// 0x4684f0..0x4684b8: the best times' default names, by rank (GameText)
static const uint32_t DEFAULT_NAMES[NUM_BEST_TIMES] =
{
	GTA(0x4684f0), GTA(0x4684e8), GTA(0x4684e0), GTA(0x4684d8), GTA(0x4684d0), GTA(0x4684c8), GTA(0x4684c4),
	GTA(0x4684b8),
};

// 0x467a24: the name of an empty best time
#define NOBODY GT(0x467a24)

// 0x4010a0
NingPo::NingPo()
{
	gNingPo = this;
	mFont = nullptr;
	mShowMovesButton = nullptr;
	mPauseButton = nullptr;
	mHints = 0;
	mUndos = 0;
	mShuffles = 0;
	mFinalTime = 0;
	mFinalRawTime = 0;
	mLayout = 0;
	mBackground = 0;
	mTileset = 0;
	mAutohint = 1;
	mSoundSetting = 1.0f;
	mMusicSetting = 1.0f;
	mFullscreen = 0;
	for (int i = 0; i < NUM_SPRITES; i++)
	{
		mSpriteTable[i].mSprite = nullptr;
		mSpriteTable[i].mName = nullptr;
	}
	for (int aLayout = 0; aLayout < NUM_LAYOUTS; aLayout++)
		for (int aRank = 0; aRank < NUM_BEST_TIMES; aRank++)
		{
			BestTime* aTime = &mBestTimes[aLayout][aRank];
			// port: the name's field is zeroed first (the original is a global, zeroed before its constructor)
			memset(aTime->mName, 0, sizeof(aTime->mName));
			pg::HStrncpy(aTime->mName, NOBODY, 0x20);
			aTime->mTilesLeft = 0x90;
			aTime->mTime = 5999;
			aTime->mRawTime = 5999;
			aTime->mVersion = 0x300;
		}
	mLoadedTileset = -1;
	mLoadedBackground = -1;
	mMovesRect.top = 0x1c2;
	mTilesLeftRect.top = 0x1c2;
	mClockRect.top = 0x1c2;
	mRegistered = false;
	mState = STATE_TITLE;
	mUndoButton = nullptr;
	mShuffleButton = nullptr;
	mHintButton = nullptr;
	mNewGameButton = nullptr;
	mBuyButton = nullptr;
	mNoMatchTime = 0;
	mHintStart = 0;
	mSpaceHeld = false;
	mFadeStart = 0;
	mHintTiles[0] = nullptr;
	mHintTiles[1] = nullptr;
	mHintTiles[2] = nullptr;
	mHintTiles[3] = nullptr;
	mBoardRect = {0x7d, 0, 0x280, 0x1e0};
	mPreviewRect = {6, 0xe1, 0x7b, 0x101};
	mTilesLeftRect.left = 0x23e;
	mTilesLeftRect.right = 0x280;
	mTilesLeftRect.bottom = 0x1e0;
	mMovesRect.left = 0x1a1;
	mMovesRect.right = 0x1d6;
	mMovesRect.bottom = 0x1e0;
	mClockRect.left = 0xd3;
	mClockRect.right = 0x13a;
	mClockRect.bottom = 0x1e0;
	// port: the fields the constructor leaves to the zeroed global
	mLastAction = 0;
	mFade = 0.0f;
	mTimesPlayed = 0;
	mGameStart = 0;
	mPenalty = 0;
	mCheat = false;
	mNoMatchRect = {0, 0, 0, 0};
	initSpriteTable();
}

// 0x4012c0
NingPo::~NingPo()
{
	if (gNingPo == this)
		gNingPo = nullptr;
}

// 0x401330
void NingPo::initSpriteTable()
{
	for (int i = 0; i < NUM_SPRITES; i++)
	{
		mSpriteTable[i].mSprite = &mSprites[i];
		mSpriteTable[i].mName = SPRITE_NAMES[i][0] != '\0' ? SPRITE_NAMES[i] : nullptr;
	}
}

// 0x405af0
PGSprite* NingPo::getSprite(int theIndex)
{
	if (-1 < theIndex && theIndex < NUM_SPRITES)
	{
		if (!mSprites[theIndex].isValid())
		{
			pg::PGAlert("getSprite: sprite %d is not loaded", theIndex);
			return nullptr;
		}
		return &mSprites[theIndex];
	}
	pg::PGAlert("getSprite: sprite %d is out of range", theIndex);
	return nullptr;
}

// 0x404010
void NingPo::releaseSprites()
{
	PGSprite* aSprite = mSprites;
	int i = NUM_SPRITES;
	do
	{
		aSprite->release();
		aSprite++;
		i--;
	} while (i != 0);
}

// 0x401a40
bool NingPo::init(const char* theCmdLine, const char* theResourceDir)
{
	// HGlobal::setResourceDir(theResourceDir)
	mCvars.registerVariable("nm_pyrogonRegistration", "0", 0);
	mCvars.registerVariable("nm_maxTimesPlayed", "20", 0);
	mCvars.registerVariable("companyName", "Pyrogon", 0);
	mCvars.registerVariable("productName", "NingPo", 0);
	PGApp::init(theCmdLine, theResourceDir);
	char anArchive[0x100];
	pg::HStrncpy(anArchive, "ningpo.pyz", 0x100);
	char aPath[0x400];
	pg::HSnprintf(aPath, 0x400, "%s%s", theResourceDir, anArchive);
	if (openResourceFile(aPath))
	{
		loadSettings();
#ifdef __EMSCRIPTEN__
		SiteStart();	// the web page's players and volumes (framework/WebProfiles.h): the settings are read, before the splash waits for a click
#endif
		// the cvar c_soundDriver chose DirectSound (port: the platform's mixer either way)
		if (initDevices(mFullscreen != 0, 0x280, 0x1e0, 2, 0x10, 0x5622))
		{
			pg::PGLog("subsystems initialized\n");
			int aVoice = -1;
			mSounds.play(&aVoice, "nm_splash.ogg", 0, 0xc0, 0, false);
			mRegistered = checkRegistration();
			pg::PGLog("loading the assets\n");
			bool aLoaded = loadAssets();
			mFont = mDisplay->createFont(getSprite(SPRITE_FONT), 8, 0xc);
			loadBestTimes(false);
			if (aLoaded && startup())
			{
				createControls();
				toTitle(false);
#ifdef __EMSCRIPTEN__
				SiteReady();	// the web page's volumes (framework/WebSettings.h): the music is there to take its volume
#endif
				return true;
			}
		}
	}
	return false;
}

// 0x401c40
void NingPo::shutdown()
{
	if (mDisplay != nullptr && mFont != nullptr)
		mDisplay->freeFont(mFont);
	mFont = nullptr;
	releaseSprites();
	PGApp::shutdown();
}

// 0x401950: the window was closed (asked first while a game is on)
void NingPo::onExit()
{
	if (mState == STATE_PLAYING)
	{
		beginPause();
		int anAnswer = messageDialog(0xdc, -1, 0, "restart_areyousure.jpg", "besttimes_areyousure_yes.tga",
			"besttimes_areyousure_yes_lit.tga", "besttimes_areyousure_no.tga", "besttimes_areyousure_no_lit.tga");
		if (anAnswer == 0)
		{
			endPause();
			return;
		}
		endPause();
	}
	mState = STATE_EXIT;
	setMusicOn(false);
	// the trial asked to buy the game here (exit_np.jpg)
}

// 0x401a00
void NingPo::openPurchasePage()
{
	if (mFullscreen != 0)
		toggleFullscreen();
	openPurchaseURL(nullptr, "Pyrogon", "NingPo");
}

// 0x405b60
void NingPo::openPartnerSite()
{
	if (mFullscreen != 0)
		toggleFullscreen();
	openPartnerURL(nullptr, "Pyrogon", "NingPo");
}

// 0x4059e0: the original switched the display's mode in place (or restarted the devices and reloaded the
// sprites); port: the host switches the window
void NingPo::toggleFullscreen()
{
	mFullscreen = mFullscreen == 0;
	invalidateAll();
	pg::GetHost()->SetFullscreen(mFullscreen != 0);
}

// 0x405cd0
bool NingPo::checkRegistration()
{
	if (mCvars.getVariableInt("nm_pyrogonRegistration") == 1)
		return PGApp::checkRegistration("Pyrogon", "NingPo", 0x3ea, false);
	return PGApp::checkRegistration("Pyrogon", "NingPo", 0x3ea, true);
}

// 0x405c70: the loading screen; the trial's progress bar stopped at a quarter
bool NingPo::loadAssets()
{
	return PGApp::loadAssets(mSpriteTable, NUM_SPRITES, "theme_np.ogg", 0, PRELOADED_SOUNDS, "loading_np.jpg", nullptr,
		"loading_partner_np.jpg", "progress_np.tga", 0x78, 0x150, mRegistered ? 1.0f : 0.25f, nullptr, nullptr);
}

// 0x405960
bool NingPo::startup()
{
	mRegistered = checkRegistration();
	// the trial counted the games played here (the registry's InstallFlags, against nm_maxTimesPlayed)
	return startupDialog(mTimesPlayed);
}

// 0x405e10
void NingPo::playSound(const char* theName, int theVolume, int thePan, bool theOverlap)
{
	mSounds.play(nullptr, theName, 0, (uint8_t)(int)((float)theVolume * mSoundSetting), (uint8_t)thePan, theOverlap);
}

// 0x405940
void NingPo::setActive(bool theActive)
{
	if (!theActive)
		setPaused(true);
	PGApp::setActive(theActive);
}

// 0x403cd0
bool NingPo::handleEvent(const PGEvent* theEvent)
{
	// 0x475538: where the mouse was at the last move
	static int sLastX = 0;
	static int sLastY = 0;
	switch (theEvent->mType)
	{
	case pg::EV_QUIT:
		onExit();
		break;
	case pg::EV_PASTE:
	case pg::EV_PAINT:
	case pg::EV_ACTIVATE:
	case pg::EV_HIDDEN:
	case pg::EV_WHEEL:
	case pg::EV_TICK:
		break;
	case pg::EV_TOGGLE_SCREEN:
		toggleFullscreen();
		break;
	case pg::EV_UP:
		if (theEvent->mChar == ' ')
			mSpaceHeld = false;
		break;
	case pg::EV_DOWN:
		if (theEvent->mCode == pg::BUTTON_LEFT)
			onLeftClick(theEvent);
		else if (theEvent->mCode == pg::BUTTON_RIGHT)
			onRightClick(theEvent);
		else
			handleKey(theEvent);
		break;
	case pg::EV_MOUSE_MOVE:
		if (theEvent->mX != sLastX || theEvent->mY != sLastY)
		{
			sLastX = theEvent->mX;
			sLastY = theEvent->mY;
			onMouseMove(theEvent);
		}
		break;
	default:
		break;
	}
	if ((theEvent->mType == pg::EV_TICK || theEvent->mType == pg::EV_QUIT) && !tick())
		return true;
	return false;
}

// 0x403e20: the keys
void NingPo::handleKey(const PGEvent* theEvent)
{
	// 0x475548: Backspace, q, Backspace toggles the cheat
	static int sCheatKeys = 0;
	if ((theEvent->mChar == '\b' && (sCheatKeys & 1) == 0) || (theEvent->mChar == 'q' && (sCheatKeys & 1) != 0))
		sCheatKeys = sCheatKeys + 1;
	if (sCheatKeys == 3)
	{
		mCheat = !mCheat;
		sCheatKeys = 0;
		return;
	}
	if (mCheat)
	{
		if (theEvent->mChar == 'm')
			cheatMatch();
		if (theEvent->mChar == 'n')
			mNoMatchTime = mEventTime;
	}
	char aChar = (char)theEvent->mChar;
	if (aChar == ' ' && !mSpaceHeld)
	{
		showMoves();
		mSpaceHeld = true;
		return;
	}
	if (aChar == 's')
	{
		shuffle();
		return;
	}
	if (aChar == 'f' || aChar == 'h')
		hint(true);
	else if (aChar == 'u')
	{
		undo();
		return;
	}
}

// 0x403f10
void NingPo::cycleBackground(int theStep)
{
	int aBackground = mBackground + theStep;
	mBackground = aBackground;
	if (aBackground < 0)
	{
		mBackground = NUM_BACKGROUNDS - 1;
		invalidateAll();
		return;
	}
	if (NUM_BACKGROUNDS - 1 < aBackground)
		mBackground = 0;
	invalidateAll();
}

// 0x403f50
void NingPo::cycleLayout(int theStep)
{
	int aLayout = mLayout + theStep;
	mLayout = aLayout;
	if (aLayout < 0)
	{
		mLayout = NUM_LAYOUTS - 1;
		invalidateAll();
		return;
	}
	if (NUM_LAYOUTS - 1 < aLayout)
		mLayout = 0;
	invalidateAll();
}

// 0x403f90
void NingPo::cycleTileset(int theStep)
{
	int aTileset = mTileset + theStep;
	mTileset = aTileset;
	if (aTileset < 0)
	{
		mTileset = NUM_TILESETS - 1;
		invalidateAll();
		return;
	}
	if (NUM_TILESETS - 1 < aTileset)
		mTileset = 0;
	invalidateAll();
}

// 0x403fd0: the cheat's 'm' plays the first move
void NingPo::cheatMatch()
{
	Tile* aTile;
	Tile* anOther;
	if (mBoard.findMove(&aTile, &anOther, 0))
		match(aTile, anOther);
}

// 0x404030: the hint's tiles redrawn while it shows, and forgotten after
void NingPo::expireHint()
{
	PGRect aRect;
	Tile** aHint = mHintTiles;
	int i = 4;
	if (pg::PyTimeSub(mEventTime, mHintStart) < 0xd7b)
	{
		do
		{
			if (*aHint != nullptr)
			{
				(*aHint)->getRect(&aRect);
				invalidateRect(&aRect);
			}
			aHint++;
			i--;
		} while (i != 0);
		return;
	}
	do
	{
		if (*aHint != nullptr)
		{
			(*aHint)->getRect(&aRect);
			invalidateRect(&aRect);
			*aHint = nullptr;
		}
		aHint++;
		i--;
	} while (i != 0);
}

// 0x4040e0: the "no match" sign redrawn while it shows (3 seconds)
void NingPo::updateNoMatch()
{
	if ((uint64_t)mEventTime < (uint64_t)mNoMatchTime)
		mNoMatchTime = 0;
	if (mNoMatchTime != 0)
	{
		if (3000 < pg::PyTimeSub(mEventTime, mNoMatchTime))
		{
			invalidateRect(&mNoMatchRect);
			mNoMatchTime = 0;
			return;
		}
		invalidateRect(&mNoMatchRect);
	}
}

// 0x4041b0: the removed tiles redrawn while they fade (500 ms)
void NingPo::updateRemovedTiles()
{
	Tile* aTile = mBoard.mTiles;
	int i = Board::NUM_TILES;
	do
	{
		if (aTile->mRemovedTime != 0)
		{
			int64_t anElapsed = pg::PyTimeSub(mEventTime, aTile->mRemovedTime);
			PGRect aRect;
			aTile->getDirtyRect(&aRect);
			invalidateRect(&aRect);
			if (-1 < anElapsed && 500 < anElapsed)
				aTile->mRemovedTime = 0;
		}
		aTile++;
		i--;
	} while (i != 0);
}

// 0x404250: the tick (and the quit event): false quits
bool NingPo::tick()
{
	// 0x475544: the clock's seconds at the last tick
	static int sClockSeconds = 0;
	if (mState == STATE_EXIT)
		return false;
	if (mAutohint != 0 && mState == STATE_PLAYING)
	{
		PyTime aNow = mEventTime;
		PyTime aPaused = getTime();
		PyTime anIdle = pg::PyTimeSub(aNow, mLastAction);
		// a hint after 45 seconds without a match (the time paused not counted)
		if (45000 < pg::PyTimeDelta(anIdle, aPaused))
		{
			mLastAction = mEventTime;
			hint(false);
		}
	}
	if (mFadeStart != 0)
	{
		float anOld = mFade;
		uint32_t aTime = (uint32_t)pg::PyTimeDelta(mEventTime, mFadeStart);
		// the free tiles light up over half a second, stay lit for 3.5 seconds (or while Space is held) and go
		// back
		if (aTime < 500)
			mFade = (float)aTime * 0.002f;
		else if (aTime < 4000 || mSpaceHeld)
			mFade = 1.0f;
		else
		{
			mFade = (float)(aTime - 4000) * 0.002f;
			if (1.0f < (float)(aTime - 4000) * 0.002f)
			{
				mFade = 1.0f;
				mFadeStart = 0;
			}
			mFade = 1.0f - mFade;
		}
		if (1.0f < mFade)
			mFade = 1.0f;
		float aLight = SHOW_MOVES_LIGHT[mTileset];
		float aFade = mFade;
		mFade = aLight * aFade;
		if (anOld != aLight * aFade)
		{
			Tile* aTile = mBoard.mTiles;
			int i = Board::NUM_TILES;
			do
			{
				if (mBoard.isFree(aTile) && aTile->mActive)
				{
					PGRect aRect;
					aTile->getDirtyRect(&aRect);
					invalidateRect(&aRect);
				}
				aTile++;
				i--;
			} while (i != 0);
		}
	}
	expireHint();
	updateRemovedTiles();
	updateNoMatch();
	PyTime aPlayed = pg::PyTimeSub(mEventTime, mGameStart);
	PyTime aPaused = getTime();
	int aSeconds = ((int)aPlayed - (int)aPaused) / 1000 + mPenalty;
	aSeconds = aSeconds % 0x3c;
	if (aSeconds != sClockSeconds)
	{
		sClockSeconds = aSeconds;
		invalidateRect(&mClockRect);
	}
	return true;
}

// 0x4044f0: the right button drops the selection (or ends the pause)
void NingPo::onRightClick(const PGEvent* theEvent)
{
	if (mPaused)
	{
		if (mBoardRect.left <= theEvent->mX && theEvent->mX <= mBoardRect.right && mBoardRect.top <= theEvent->mY &&
			theEvent->mY <= mBoardRect.bottom)
			setPaused(false);
		playSound("nm_guiclick.wav", 0x80, 0, false);
		return;
	}
	if (mState != STATE_TITLE && mState != STATE_GAME_OVER && mBoard.mSelected != nullptr)
	{
		PGRect aRect;
		mBoard.mSelected->getRect(&aRect);
		invalidateRect(&aRect);
		deselect();
		invalidateRect(&mPreviewRect);
		expireHint();
		mHintTiles[0] = nullptr;
		mHintTiles[1] = nullptr;
		mHintTiles[2] = nullptr;
		mHintTiles[3] = nullptr;
	}
}

// 0x4045c0: the left button selects a free tile, and a second one of its kind matches the pair
void NingPo::onLeftClick(const PGEvent* theEvent)
{
	if (mPaused)
	{
		if (mBoardRect.left <= theEvent->mX && theEvent->mX <= mBoardRect.right && mBoardRect.top <= theEvent->mY &&
			theEvent->mY <= mBoardRect.bottom)
			setPaused(false);
		playSound("nm_guiclick.wav", 0x80, 0, false);
		return;
	}
	if (mState == STATE_TITLE)
		return;
	if (mState == STATE_GAME_OVER)
		return;
	PGPoint aPoint = {theEvent->mX, theEvent->mY};
	Tile* aTile = mBoard.tileAt(&aPoint, true);
	Tile* aWasSelected = mBoard.mSelected;
	const char* aSound;
	bool anOverlap;
	if (aTile == nullptr)
		goto done;
	if (!mBoard.isFree(aTile))
	{
	blocked:
		anOverlap = true;
		aSound = "nm_blocked.ogg";
	}
	else
	{
		expireHint();
		mHintTiles[0] = nullptr;
		mHintTiles[1] = nullptr;
		mHintTiles[2] = nullptr;
		mHintTiles[3] = nullptr;
		Tile* aSelected = mBoard.mSelected;
		if (aSelected == nullptr)
		{
			deselect();
			select(aTile);
			anOverlap = false;
			aSound = "nm_click.wav";
		}
		else
		{
			if (!aTile->mSelected)
			{
				if (aTile->mType == aSelected->mType)
				{
					match(aTile, aSelected);
					mLastAction = mEventTime;
					goto done;
				}
				PGRect aRect;
				aSelected->getRect(&aRect);
				invalidateRect(&aRect);
				deselect();
				goto blocked;
			}
			aTile->mSelected = false;
			deselect();
			PGRect aRect;
			aTile->getRect(&aRect);
			invalidateRect(&aRect);
			anOverlap = false;
			aSound = "nm_unclick.wav";
		}
	}
	playSound(aSound, 0x80, 0, anOverlap);
done:
	if (mBoard.mSelected != aWasSelected)
		invalidateRect(&mPreviewRect);
}

// 0x4047b0: the tile under the mouse gets its outline
void NingPo::onMouseMove(const PGEvent* theEvent)
{
	PGPoint aPoint = {theEvent->mX, theEvent->mY};
	Tile* aTile = mBoard.tileAt(&aPoint, true);
	if (aTile != mBoard.mHover)
	{
		PGRect aRect;
		if (aTile != nullptr)
		{
			aTile->getDirtyRect(&aRect);
			invalidateRect(&aRect);
		}
		if (mBoard.mHover != nullptr)
		{
			mBoard.mHover->getDirtyRect(&aRect);
			invalidateRect(&aRect);
		}
		mBoard.mHover = aTile;
	}
}

// 0x404820..0x404a00: a control enabled or not, and redrawn
static void EnableControl(NingPo* theApp, PGControl* theControl, bool theEnabled)
{
	if (theControl != nullptr)
	{
		theControl->setEnabled(theEnabled);
		PGRect aRect = *theControl->getRect();
		theApp->invalidateRect(&aRect);
	}
}

void NingPo::enablePause(bool theEnabled)      // 0x404820
{
	EnableControl(this, mPauseButton, theEnabled);
}

void NingPo::enableNewGame(bool theEnabled)    // 0x404880
{
	EnableControl(this, mNewGameButton, theEnabled);
}

void NingPo::enableShuffle(bool theEnabled)    // 0x4048e0
{
	EnableControl(this, mShuffleButton, theEnabled);
}

void NingPo::enableShowMoves(bool theEnabled)  // 0x404940
{
	EnableControl(this, mShowMovesButton, theEnabled);
}

void NingPo::enableHint(bool theEnabled)       // 0x4049a0
{
	EnableControl(this, mHintButton, theEnabled);
}

void NingPo::enableUndo(bool theEnabled)       // 0x404a00
{
	EnableControl(this, mUndoButton, theEnabled);
}

// 0x404a60: the chosen tile set's sheet, when it changed
void NingPo::loadTileset()
{
	PGSprite* aSprite = &mSprites[SPRITE_TILE_SHEET];
	if (aSprite->isValid())
	{
		if (mTileset == mLoadedTileset)
			return;
		aSprite->release();
	}
	aSprite->load(mDisplay, mFileSystem, TILESET_FILES[mTileset]);
	mLoadedTileset = mTileset;
}

// 0x404ad0: the chosen arena, when it changed
void NingPo::loadBackground()
{
	PGSprite* aSprite = &mSprites[SPRITE_BACKGROUND];
	if (aSprite->isValid())
	{
		if (mBackground == mLoadedBackground)
			return;
		aSprite->release();
	}
	aSprite->load(mDisplay, mFileSystem, BACKGROUND_FILES[mBackground]);
	mLoadedBackground = mBackground;
}

// 0x404b40: the title's start button
void NingPo::startGame()
{
	saveSettings();
	if (newGame())
	{
		mFadeStart = 0;
		mFade = 0.0f;
		mPenalty = 0;
		mState = STATE_PLAYING;
		loadTileset();
		loadBackground();
		enablePause(true);
		enableUndo(false);
		enableShuffle(true);
		enableHint(true);
		enableShowMoves(true);
		enableNewGame(true);
		invalidateAll();
	}
}

// 0x404bd0: back to the title (asked first with theAsk)
void NingPo::toTitle(bool theAsk)
{
	if (theAsk)
	{
		beginPause();
		int anAnswer = messageDialog(0xdc, -1, 0, "restart_areyousure.jpg", "besttimes_areyousure_yes.tga",
			"besttimes_areyousure_yes_lit.tga", "besttimes_areyousure_no.tga", "besttimes_areyousure_no_lit.tga");
		if (anAnswer == 0)
		{
			endPause();
			return;
		}
		endPause();
	}
	setPaused(false);
	setMusicOn(false);
	enablePause(false);
	enableUndo(false);
	enableShuffle(false);
	enableHint(false);
	enableShowMoves(false);
	enableNewGame(false);
	mState = STATE_TITLE;
	invalidateAll();
}

// 0x404c90: a new board (the trial refused its last layouts and tile sets here)
bool NingPo::newGame()
{
	restartMusic();
	setMusicOn(true);
	setMusicVolume(mMusicSetting);
	mBoard.newGame(LAYOUT_FILES[mLayout]);
	enableUndo(false);
	enableShuffle(true);
	invalidateAll();
	mGameStart = mEventTime;
	mLastAction = mEventTime;
	// slot 0xcc: the time paused starts again from 0
	mPausedTime = 0;
	return true;
}

// 0x404db0
void NingPo::addPenalty(int theSeconds)
{
	mPenalty = mPenalty + theSeconds;
	invalidateRect(&mClockRect);
}

// 0x404ee0: the last pair back, for a minute's penalty
void NingPo::undo()
{
	Tile* aTile = nullptr;
	Tile* anOther = nullptr;
	PGRect aRect;
	mBoard.undo(&aTile, &anOther);
	if (aTile != nullptr)
	{
		aTile->getDirtyRect(&aRect);
		invalidateRect(&aRect);
	}
	if (anOther != nullptr)
	{
		anOther->getDirtyRect(&aRect);
		invalidateRect(&aRect);
	}
	mUndos = mUndos + 1;
	invalidateRect(&mMovesRect);
	invalidateRect(&mTilesLeftRect);
	if (mBoard.mSelected != nullptr)
	{
		mBoard.mSelected->mSelected = false;
		mBoard.mSelected->getRect(&aRect);
		invalidateRect(&aRect);
		deselect();
		invalidateRect(&mPreviewRect);
		expireHint();
		mHintTiles[0] = nullptr;
		mHintTiles[1] = nullptr;
		mHintTiles[2] = nullptr;
		mHintTiles[3] = nullptr;
	}
	enableUndo(-1 < mBoard.mUndoIndex);
	enableShuffle(2 < mBoard.mTilesLeft);
	addPenalty(0x3c);
}

// 0x405010: the tiles shuffled, for two minutes' penalty
void NingPo::shuffle()
{
	if (mState == STATE_PLAYING && !mPaused)
	{
		playSound("nm_hint.ogg", 0x80, 0, true);
		mLastAction = mEventTime;
		mBoard.shuffle();
		mShuffles = mShuffles + 1;
		enableUndo(false);
		deselect();
		expireHint();
		mHintTiles[0] = nullptr;
		mHintTiles[1] = nullptr;
		mHintTiles[2] = nullptr;
		mHintTiles[3] = nullptr;
		invalidateAll();
		addPenalty(0x78);
	}
}

// 0x4050c0: the hint button shows "hint" again
void NingPo::deselect()
{
	mBoard.deselect();
	mHintButton->setSprites(getSprite(0x50), getSprite(0x51), nullptr);
}

// 0x405100: the hint button shows "find match"
void NingPo::select(Tile* theTile)
{
	mBoard.mSelected = theTile;
	theTile->mSelected = true;
	PGRect aRect;
	mBoard.mSelected->getRect(&aRect);
	invalidateRect(&aRect);
	invalidateRect(&mPreviewRect);
	mHintButton->setSprites(getSprite(0x52), getSprite(0x53), nullptr);
}

// 0x405170
void NingPo::showRules()
{
	const char* aPages[2] = {"rules_0_np.jpg", "rules_1_np.jpg"};
	beginPause();
	slideshow(0x7d, 0, "rules_close_np.tga", "rules_close_lit_np.tga", "rules_next_np.tga", "rules_next_lit_np.tga",
		"rules_prev_np.tga", "rules_prev_lit_np.tga", aPages, 2);
	playSound("nm_guiclick.wav", 0x80, 0, false);
	endPause();
}

// 0x4051f0: a hint, asked for (theManual: 30 seconds' penalty) or the autohint's. Without a selection a move (each
// hint the next one), with one the selected tile's matches that can be seen; "no match" when it has none
void NingPo::hint(bool theManual)
{
	if (mState != STATE_PLAYING || mPaused)
		return;
	if (theManual)
		playSound("nm_hint.ogg", 0x80, 0, true);
	expireHint();
	if (theManual)
		mHints = mHints + 1;
	if (mBoard.mSelected == nullptr)
	{
		if (mBoard.findMove(&mHintTiles[0], &mHintTiles[1], mHints))
		{
			mHintStart = mEventTime;
			PGRect aRect;
			mHintTiles[0]->getRect(&aRect);
			invalidateRect(&aRect);
			mHintTiles[1]->getRect(&aRect);
			invalidateRect(&aRect);
			mHintTiles[3] = nullptr;
			mHintTiles[2] = nullptr;
		}
	}
	else
	{
		Tile* aMatches[3] = {nullptr, nullptr, nullptr};
		int aCount = mBoard.getMatches(mBoard.mSelected, aMatches);
		Tile** aNext = &mHintTiles[1];
		mHintTiles[3] = nullptr;
		mHintTiles[2] = nullptr;
		*aNext = nullptr;
		mHintTiles[0] = nullptr;
		int i = 0;
		if (0 < aCount)
		{
			do
			{
				if (mBoard.isVisible(aMatches[i]))
				{
					mHintTiles[0] = mBoard.mSelected;
					*aNext = aMatches[i];
					aNext++;
				}
				i++;
			} while (i < aCount);
		}
		Tile** aHint = mHintTiles;
		if (mHintTiles[0] == nullptr)
		{
			if (!theManual)
				return;
			mNoMatchTime = mEventTime;
		}
		else
		{
			mHintStart = mEventTime;
			int j = 4;
			do
			{
				if (*aHint != nullptr)
				{
					PGRect aRect;
					(*aHint)->getRect(&aRect);
					invalidateRect(&aRect);
				}
				aHint++;
				j--;
			} while (j != 0);
		}
	}
	if (theManual)
		addPenalty(0x1e);
}

// 0x405410: the end of a game: won, or no more moves (the player may still shuffle or undo instead)
void NingPo::endGame(bool theWon, int theTime, int theRawTime)
{
	if (!theWon && !noMoreMoves())
		return;
	setMusicOn(false);
	enablePause(false);
	enableUndo(false);
	enableShuffle(false);
	enableHint(false);
	enableShowMoves(false);
	enableNewGame(false);
	if (!theWon)
	{
		playSound("nm_gameover.ogg", 0x80, 0, false);
		mState = STATE_GAME_OVER;
	}
	else
	{
		playSound("nm_youwin.ogg", 0x80, 0, false);
		mState = STATE_WON;
	}
	mFinalTime = theTime;
	mNoMatchTime = 0;
	mFinalRawTime = theRawTime;
	mBoard.clearUndo();
	invalidateAll();
}

// 0x4054f0: a pair matched
void NingPo::match(Tile* theTile, Tile* theOther)
{
	theTile->mActive = false;
	mBoard.removePair(theTile, theOther);
	invalidateRect(&mTilesLeftRect);
	invalidateRect(&mMovesRect);
	PGRect aRect;
	theTile->getDirtyRect(&aRect);
	invalidateRect(&aRect);
	theOther->getDirtyRect(&aRect);
	invalidateRect(&aRect);
	bool anUndo = false;
	// the seconds played: an unsigned division of the 64-bit time
	uint64_t aPlayed = (uint64_t)(pg::PyTimeSub(mEventTime, mGameStart) - getTime());
	int aSeconds = (int)(aPlayed / 1000);
	if (mBoard.mTilesLeft == 0)
		endGame(true, mPenalty + aSeconds, aSeconds);
	else if (!mBoard.findMove(nullptr, nullptr, 0))
	{
		endGame(false, mPenalty + aSeconds, aSeconds);
		anUndo = -1 < mBoard.mUndoIndex;
	}
	else
	{
		playSound("nm_match.ogg", 0x80, 0, true);
		anUndo = true;
	}
	enableUndo(anUndo);
	enableShuffle(2 < mBoard.mTilesLeft && mState != STATE_GAME_OVER);
	mHintButton->setSprites(getSprite(0x50), getSprite(0x51), nullptr);
}

// 0x405d20: the "show moves" button (and Space): the free tiles light up for a while
void NingPo::showMoves()
{
	if (mFadeStart == 0 && !mPaused && mState == STATE_PLAYING)
	{
		mFadeStart = mEventTime;
		invalidateAll();
	}
}

// 0x407c40: the settings from the registry
void NingPo::loadSettings()
{
	pg::HRegistry* aRegistry = pg::GetRegistry();
	if (aRegistry->open("Pyrogon", "NingPo"))
	{
		int aSize;
		aRegistry->readData("Layout", &mLayout, 4, &aSize);
		aRegistry->readData("Background", &mBackground, 4, &aSize);
		aRegistry->readData("Tileset", &mTileset, 4, &aSize);
		aRegistry->readData("Autohint", &mAutohint, 4, &aSize);
		float* aVolume = &mSoundSetting;
		if (!aRegistry->readData("Sound Volume", aVolume, 4, &aSize))
			*aVolume = 1.0f;
		if (*aVolume < 0.0f)
			*aVolume = 0.0f;
		if (1.0f < *aVolume)
			*aVolume = 1.0f;
		aVolume = &mMusicSetting;
		if (!aRegistry->readData("Music Volume", aVolume, 4, &aSize))
			*aVolume = 0.25f;
		if (*aVolume < 0.0f)
			*aVolume = 0.0f;
		if (1.0f < *aVolume)
			*aVolume = 1.0f;
		if (!aRegistry->readData("Fullscreen", &mFullscreen, 4, &aSize))
			mFullscreen = 0;
		aRegistry->close();
	}
	cycleBackground(0);
	cycleTileset(0);
	cycleLayout(0);
}

// 0x407dd0
void NingPo::saveSettings()
{
	pg::HRegistry* aRegistry = pg::GetRegistry();
	if (aRegistry->open("Pyrogon", "NingPo"))
	{
		aRegistry->writeData("Autohint", &mAutohint, 4);
		aRegistry->writeData("Sound Volume", &mSoundSetting, 4);
		aRegistry->writeData("Music Volume", &mMusicSetting, 4);
		aRegistry->writeData("Tileset", &mTileset, 4);
		aRegistry->writeData("Background", &mBackground, 4);
		aRegistry->writeData("Layout", &mLayout, 4);
		aRegistry->writeData("Fullscreen", &mFullscreen, 4);
		aRegistry->close();
	}
}

// 0x407ea0: the pause (none while the cheat is on)
void NingPo::setPaused(bool thePaused)
{
	if (!mCheat && thePaused != mPaused)
	{
		if (thePaused)
		{
			setMusicOn(false);
			beginPause();
			mPaused = true;
			invalidateAll();
			return;
		}
		setMusicOn(true);
		endPause();
		mPaused = false;
		invalidateAll();
	}
}

// 0x407f10
void NingPo::resetBestTimes(int theLayout)
{
	BestTime* aTime = mBestTimes[theLayout];
	int i = NUM_BEST_TIMES;
	do
	{
		pg::HStrncpy(aTime->mName, NOBODY, 0x20);
		aTime->mTime = 5999;
		aTime->mRawTime = 5999;
		aTime->mTilesLeft = 0x90;
		aTime->mVersion = 0x300;
		aTime++;
		i--;
	} while (i != 0);
}

// 0x407f70
void NingPo::saveBestTimes()
{
	pg::HRegistry* aRegistry = pg::GetRegistry();
	if (!aRegistry->open("Pyrogon", "NingPo"))
	{
		pg::PGMessageBox(0x31, "Warning", "Could not open the registry to save the best times.");
		return;
	}
	BestTime* aTime = &mBestTimes[0][0];
	int aLayout = 0;
	do
	{
		int aRank = 0;
		do
		{
			char aName[0x100];
			pg::HSnprintf(aName, 0x100, "HS:%dx%d", aLayout, aRank);
			aRegistry->writeData(aName, aTime, 0x30);
			aRank++;
			aTime++;
		} while (aRank < NUM_BEST_TIMES);
		aLayout++;
	} while (aLayout < NUM_LAYOUTS);
	aRegistry->close();
}

// 0x408010: the best times from the registry; with theReset (and for a time that is not there) the defaults
void NingPo::loadBestTimes(bool theReset)
{
	int aLayout = 0;
	do
	{
		resetBestTimes(aLayout);
		aLayout++;
	} while (aLayout < NUM_LAYOUTS);
	pg::HRegistry* aRegistry = pg::GetRegistry();
	if (aRegistry->open("Pyrogon", "NingPo"))
	{
		aLayout = 0;
		int aRank = 0;
		while (true)
		{
			do
			{
				BestTime aTime;
				memset(&aTime, 0, sizeof(aTime));
				pg::HStrncpy(aTime.mName, NOBODY, 0x20);
				aTime.mTime = 5999;
				aTime.mRawTime = 5999;
				aTime.mTilesLeft = 0x90;
				aTime.mVersion = 0x300;
				int aSize = 0;
				char aName[0x400];
				pg::HSnprintf(aName, 0x400, "HS:%dx%d", aLayout, aRank);
				if (!theReset && aRegistry->readData(aName, &aTime, 0x30, &aSize))
				{
					if (aSize == 0x30 && aTime.mVersion == 0x300)
						mBestTimes[aLayout][aRank] = aTime;
				}
				else
				{
					// the original copies with a size of 0x30, past the name's field; HStrncpy stops at the end
					pg::HStrncpy(mBestTimes[aLayout][aRank].mName, GT(DEFAULT_NAMES[aRank]), 0x30);
				}
				aRank++;
			} while (aRank < NUM_BEST_TIMES);
			aLayout++;
			if (NUM_LAYOUTS - 1 < aLayout)
				break;
			aRank = 0;
		}
		aRegistry->close();
		return;
	}
	pg::PGMessageBox(0x31, "Warning", "Could not open the registry to load the best times.");
}

// 0x4081d0: a won (or ended) game's time into the layout's best times, with the player's name
void NingPo::recordTime()
{
	BestTime* aTime = mBestTimes[mLayout];
	int i = 0;
	while (aTime->mTilesLeft <= mBoard.mTilesLeft &&
		(mBoard.mTilesLeft != aTime->mTilesLeft ||
			(aTime->mTime <= mFinalTime && (mFinalTime != aTime->mTime || aTime->mRawTime <= mFinalRawTime))))
	{
		i++;
		aTime++;
		if (7 < i)
			return;
	}
	if (i != -1)
	{
		char aName[0x100];
		memmove(&mBestTimes[mLayout][i + 1], &mBestTimes[mLayout][i], (NUM_BEST_TIMES - 1 - i) * sizeof(BestTime));
		enterName(aName, sizeof(aName));
		mBestTimes[mLayout][i].mTime = mFinalTime;
		mBestTimes[mLayout][i].mRawTime = mFinalRawTime;
		mBestTimes[mLayout][i].mTilesLeft = mBoard.mTilesLeft;
		pg::HStrncpy(mBestTimes[mLayout][i].mName, aName, 0x20);
		saveBestTimes();
		showBestTimes(mLayout);
	}
}

} // namespace ningpo
