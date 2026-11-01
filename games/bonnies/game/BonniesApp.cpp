// The game's application (see BonniesApp.h).

#include "BonniesApp.h"
#include "Book.h"
#include "Dialog.h"
#include "Dictionary.h"
#include "Draw.h"
#include "Events.h"
#include "File.h"
#include "Font.h"
#include "Game.h"
#include "GameScreen.h"
#include "Globals.h"
#include "Graphics.h"
#include "Host.h"
#include "Language.h"
#include "Music.h"
#include "Options.h"
#include "Platform.h"
#include "Resources.h"
#include "Screens.h"
#include "Sound.h"
#include "Widget.h"

namespace bonnies
{

static int gImagesLoaded;       // 0x475640
static int gFullFrame;          // 0x475644: the last frame that had to be drawn whole
static int gBoardFrame;         // 0x475648: the last frame the board's score changed
static int gLastFullScreen;     // 0x47564c
static uint32_t gFullDrawTime;  // 0x475650: when the screen was last drawn whole
int gExited;                    // 0x47563c

static uint32_t ticks()
{
	return host()->getTicks();
}

// 0x401070: plays theName at theVolume
int playSound(const Str& theName, float theVolume)
{
	return gSound->play(theName, theVolume, 1, 0, 1.0f);
}

// 0x4023a0: whether the player theName has a saved game
int playerSaveExists(const Str& theName)
{
	Str aPath = "";
	aPath += savedGamesDir(true);
	aPath += exeGlobalString(0x475a54);
	aPath += theName;
	return fileExists(aPath);
}

// 0x4024a0: saves the game in progress as theName's
void saveGameAs(const Str& theName)
{
	gGame->prepareSave(1);
	gGame->save(theName);
}

// 0x401160
BonniesApp::BonniesApp()
{
	mInited = 0;
}

// 0x4015c0
BonniesApp::~BonniesApp()
{
}

// 0x401630
void BonniesApp::loadImages()
{
	if (gImagesLoaded == 0)
	{
		gImagesLoaded = 1;
		uint32_t anEntry = 0x4662c0;
		Image** anImage = mImages;
		do
		{
			if (*anImage == nullptr)
			{
				int aKind = exeI32(anEntry);
				if ((aKind != 2 || mLoadImages[0] != 0) && (aKind != 7 || mLoadImages[6] != 0) &&
					(aKind != 4 || mLoadImages[3] != 0))
					*anImage = gResources->getImage(exeGlobalString(anEntry + 4));
				if (*anImage == nullptr)
					gImagesLoaded = 0;
			}
			anEntry += 0xc;
			anImage++;
		} while (anEntry < 0x466470);
	}
}

// 0x4016d0
void BonniesApp::draw()
{
	if (0 < mSkipDraws)
	{
		mSkipDraws--;
		return;
	}
	Widget* aScore = nullptr;
	if (gGameScreen != nullptr)
		aScore = gGameScreen->findWidget(0xc9);
	if (mSpecialMode != 0 || inModal() || gGameScreen == nullptr || gGameScreen->isBusy() ||
		(gBonnies != nullptr && gBonnies->mPaused != 0) || gGameScreen->mField58 != 1 ||
		gLastFullScreen != isFullScreen() || !hasInputFocus() || !gBonnies->mStatus.empty() ||
		gGameScreen->mFlashTime != 0 || 3999 < (int)(ticks() - gFullDrawTime) ||
		(aScore != nullptr && (aScore->mFlags & 6) != 0) || mFullRedraw != 0 || (int)ticks() < mRedrawUntil)
		gFullFrame = frameCount();
	if (gGameScreen != nullptr && gGame->mShownScore != gGame->mScore)
		gBoardFrame = frameCount();
	gLastFullScreen = isFullScreen();

	int aCount;
	float aRects[8];
	if (gFullFrame + 2 < frameCount())
	{
		float aX0;
		float aY0;
		float aX1;
		float aY1;
		gGameScreen->getRect(&aX0, &aY0, &aX1, &aY1, 1);
		if (gGame == nullptr || gGame->mFieldC5C == 0)
		{
			if (aY1 <= 0.98f)
				aY1 = 0.98f;
		}
		else
		{
			if (0.48624998f <= aX0)
				aX0 = 0.48624998f;
			if (aX1 <= 0.97124994f)
				aX1 = 0.97124994f;
			if (aY1 <= 0.97333336f)
				aY1 = 0.97333336f;
		}
		if (gGame != nullptr && frameCount() <= gGame->mFieldC80 + 1)
		{
			aY0 = 0.0f;
			aY1 = 1.0f;
		}
		if (mShowFPS != 0 && 0.096666664f <= aY0)
			aY0 = 0.096666664f;
		if (0.15f <= aX0)
		{
			aRects[2] = aX1 - aX0;
			aRects[1] = aY0;
			aCount = 1;
			aRects[0] = aX0;
			aRects[3] = aY1 - aY0;
			if (frameCount() <= gBoardFrame + 1)
			{
				aRects[4] = 0.0175f;
				aRects[5] = 0.09833334f;
				aRects[6] = 0.295f;
				aRects[7] = 0.043333333f;
				aCount = 2;
			}
			goto drawRects;
		}
	}
	aCount = 1;
	aRects[0] = 0.0f;
	aRects[1] = 0.0f;
	aRects[2] = 1.0f;
	aRects[3] = 1.0f;
	gFullDrawTime = ticks();
drawRects:
	if (0 < aCount)
	{
		float* aRect = aRects;
		int aLeft = aCount;
		do
		{
			pushClip(aRect[0], aRect[1], (float)(aRect[2] + aRect[0]), (float)(aRect[3] + aRect[1]));
			App::draw();
			Str aText = "";
			Str aLine;
			if (mShowFPS != 0)
			{
				aLine = getText(0x4a0);
				aLine += floatToStr((float)framesPerSecond(), 1);
				aLine += lit(0x457424);
				aText += aLine;
			}
			if (!mDebugText.empty())
			{
				aText += mDebugText;
				aText += lit(0x457424);
			}
			if (mSpecialMode != 0)
			{
				aText += lit(0x457400);
				if (0x40 < mSpecialMode && mSpecialMode < 0x5b)
					aText += (char)mSpecialMode;
				const char* aName = nullptr;
				switch (mSpecialMode)
				{
				case 0x21:
					aName = lit(0x4573f0);
					break;
				case 0x23:
					aName = lit(0x4573c4);
					break;
				case 0x26:
					aName = lit(0x4573a8);
					break;
				case 0x28:
					aName = lit(0x4573bc);
					break;
				case 0x29:
					aName = lit(0x4573b0);
					break;
				case 0x2a:
					aName = lit(0x4573d4);
					break;
				case 0x3f:
					aName = lit(0x4573e4);
					break;
				case 0x5e:
					aName = lit(0x4573cc);
					break;
				}
				if (aName != nullptr)
					aText += aName;
				aText += lit(0x457380);
			}
			if (!aText.empty() && mFonts[8] != nullptr)
				mFonts[8]->drawText(aText, 0.0125f, 0.1f, 0.875f, 0.8f, 0xffffffff, 0, 0x62, 1.0f, 0.0f);
			if (mFlashRects != 0)
				drawRect((float)(aRect[0] + 0.0012499999f), (float)(aRect[1] + 0.0016666667f),
					(float)(aRect[2] - 0.003125), (float)(aRect[3] - 0.004166666666666667),
					((int)ticks() % 0xfa) << 8 | 0xffff00ff, 1);
			popClip();
			aRect += 4;
			aLeft--;
		} while (aLeft != 0);
	}
	gGraphics->present(aCount, aRects);
}

// 0x401d00
int BonniesApp::isIdle()
{
	if (gGraphics != nullptr && mPaused != 0 && isInactive() != 0 && gGraphics->mFocusFrame + 1 < frameCount())
		return 1;
	return 0;
}

// 0x401d40
void BonniesApp::updateMenu()
{
	checkMenuItem(0x2711, mShowFPS);
	checkMenuItem(0x2715, mDebugTexts);
	checkMenuItem(0x2717, mDebugScreens);
	checkMenuItem(0x2716, mFlashRects);
	checkMenuItem(0x2718, mFullRedraw);
	checkMenuItem(0x2afa, mDebug1EC);
	checkMenuItem(0x2af9, mDebug1E8);
	checkMenuItem(0x2afd, mDebug1FC);
	checkMenuItem(0x2afc, mPauseInactive);
	unsigned int anId = 0x277e;
	int aLeft = 7;
	do
	{
		checkMenuItem(anId, mDebugRects[anId - 0x277e]);
		anId++;
		aLeft--;
	} while (aLeft != 0);
}

// 0x401e00
void BonniesApp::menuCommand(int theCommand)
{
	if (0x2bc0 < theCommand && theCommand < 0x2bf3 && gGameScreen != nullptr)
	{
		gGame->mLevel = theCommand - 0x2bc1;
		gGameScreen->refresh();
	}
	if (0x2c24 < theCommand && theCommand < 0x2c2b)
	{
		static const int aLimits[6] = {0, 30, 15, 10, 5, 2};
		mFrameLimit = aLimits[theCommand - 0x2c25];
		int i = 0;
		do
		{
			// As the original has it: the command, not its index, is compared, so no item is checked
			checkMenuItem(i + 0x2c25, theCommand == i);
			i++;
		} while (i < 7);
	}
	if (0x2b0a < theCommand && theCommand < 0x2b25)
		mSpecialMode = theCommand - 0x2aca;
	if (theCommand < 0x2afd)
	{
		if (theCommand == 0x2afc)
		{
			mPauseInactive = mPauseInactive == 0;
			updateMenu();
			return;
		}
		if (theCommand < 0x2776)
		{
			bool anAll = theCommand == 0x2774;
			if (0x2773 < theCommand)
			{
				for (int i = 0; i < 7; i++)
					mDebugRects[i] = anAll;
				updateMenu();
				return;
			}
			switch (theCommand)
			{
			case 0x2711:
				mShowFPS = mShowFPS == 0;
				updateMenu();
				return;
			case 0x2715:
				mDebugTexts = mDebugTexts == 0;
				updateMenu();
				return;
			case 0x2716:
				mFlashRects = mFlashRects == 0;
				updateMenu();
				return;
			case 0x2717:
				mDebugScreens = mDebugScreens == 0;
				updateMenu();
				return;
			case 0x2718:
				mFullRedraw = mFullRedraw == 0;
				updateMenu();
				return;
			}
		}
		else if (theCommand < 0x2afb)
		{
			if (theCommand == 0x2afa)
			{
				mDebug1EC = mDebug1EC == 0;
				updateMenu();
				return;
			}
			if (0x277d < theCommand)
			{
				if (theCommand < 0x2785)
				{
					mDebugRects[theCommand - 0x277e] = mDebugRects[theCommand - 0x277e] == '\0';
					updateMenu();
					return;
				}
				if (theCommand == 0x2af9)
				{
					mDebug1E8 = mDebug1E8 == 0;
					updateMenu();
					return;
				}
			}
		}
		else if (theCommand == 0x2afb && gGameScreen != nullptr)
		{
			gGameScreen->refresh();
			updateMenu();
			return;
		}
	}
	else
	{
		switch (theCommand)
		{
		case 0x2afd:
			mDebug1FC = mDebug1FC == 0;
			updateMenu();
			return;
		case 0x2b02:
			mSpecialMode = 0;
			updateMenu();
			return;
		case 0x2b03:
			mSpecialMode = '*';
			updateMenu();
			return;
		case 0x2b04:
			mSpecialMode = '?';
			updateMenu();
			return;
		case 0x2b05:
			mSpecialMode = '!';
			updateMenu();
			return;
		case 0x2b06:
			mSpecialMode = '(';
			updateMenu();
			return;
		case 0x2b07:
			mSpecialMode = '#';
			updateMenu();
			return;
		case 0x2b08:
			mSpecialMode = '^';
			updateMenu();
			return;
		case 0x2b09:
			mSpecialMode = ')';
			updateMenu();
			return;
		case 0x2b0a:
			mSpecialMode = '&';
			break;
		}
	}
	updateMenu();
}

// 0x402230
int BonniesApp::isPaused()
{
	if (mPaused == 0 && !inModal())
		return 0;
	return 1;
}

// 0x402250
int BonniesApp::messageBox(int theFlags, const Str& theText1, const Str& theText2, const Str& theText3,
	const Str& theText4, const Str& theText5)
{
	return gameDialog(theFlags, theText1, theText2, theText3, theText4, theText5, 0, 0, 0, 0, 0);
}

// 0x402360
float BonniesApp::getSoundVolume()
{
	if (gOptions != nullptr)
		return gOptions->mSoundVolume;
	return 1.0f;
}

// 0x402380
float BonniesApp::getMusicVolume()
{
	if (gOptions != nullptr)
		return gOptions->mMusicVolume;
	return 1.0f;
}

// 0x402510
void BonniesApp::balloon(const Str& theText, float theX, float theY, int theSide, int theTall, float* theRect,
	int theDraw)
{
	float aWidth = 0.4375f;
	if (theSide == 5)
		aWidth = 0.50625f;
	else if (theSide == 6)
	{
		aWidth = 0.4f;
		float aX0 = 0.5f;
		float aY0 = 0.5f;
		float aX1 = 0.5f;
		float aY1 = 0.5f;
		if (gGameScreen != nullptr)
			gGameScreen->getRect(&aX0, &aY0, &aX1, &aY1, 0);
		double aMargin = ((double)aX0 - 0.4375f) * 0.5f;
		if (aMargin < 0.03f)
		{
			double aShort = 0.03f - aMargin;
			aWidth = (float)(0.4f - (aShort + aShort));
		}
	}
	else if (theSide == 2 && theX < 0.5875f)
	{
		double aShift = 0.5875f - (double)theX;
		if (0.15f < aShift)
			aShift = 0.15f;
		aWidth = (float)(0.4375f - aShift);
	}
	float aSize[4];
	mFonts[11]->countLines(theText, 0, aWidth, aSize, 1.0f);
	float aPad = 0.0f;
	double aLines = (double)aSize[3] + 0.008333334f;
	float aBoxWidth = (float)(aWidth + 0.0375f);
	float aHeight = (float)(aLines + 0.07333333f);
	float aMin = 0.19333333f < aHeight ? aHeight : 0.19333333f;
	if (aHeight < aMin)
	{
		aPad = (float)(((double)aMin - aHeight) * 0.5f);
		aHeight = aMin;
	}
	if (theTall != 0)
		aHeight = (float)(aHeight + 0.09166667f);
	float aX;
	float aY;
	if (theSide == 0)
	{
		aX = (float)(((double)theX - 0.03375f) - aWidth);
		aY = (float)(theY + 0.17f);
	}
	else if (theSide == 1)
	{
		aX = (float)(((double)theX - 0.03375f) - aWidth);
		aY = (float)(((double)theY - aHeight) - 0.15f);
	}
	else if (theSide == 2)
	{
		aX = (float)(((double)theX - 0.15f) - aWidth);
		aY = (float)(((double)theY - aHeight) - 0.04f);
	}
	else
	{
		if (theSide == 3)
			aX = (float)(((double)theX - 0.07125f) - aWidth);
		else if (theSide == 4)
			aX = (float)(theX + 0.04125f);
		else if (theSide == 6)
		{
			float aX0 = 0.5f;
			float aY0 = 0.5f;
			float aX1 = 0.5f;
			float aY1 = 0.5f;
			if (gGameScreen != nullptr)
				gGameScreen->getRect(&aX0, &aY0, &aX1, &aY1, 0);
			double aCentre = ((double)aX0 - aBoxWidth) * 0.5f;
			aX = (float)aCentre;
			if (aCentre < 0.03f)
			{
				double aShort = 0.03f - (double)aX;
				aBoxWidth = (float)(aBoxWidth - aShort);
				aWidth = (float)(aWidth - aShort);
			}
			aY = (float)(((double)aY1 + aY0) * 0.5f - (double)aHeight * 0.5f);
			goto placed;
		}
		else
			aX = (float)(theX - (double)aBoxWidth * 0.5f);
		aY = (float)(theY - (double)aHeight * 0.5f);
	}
placed:
	if (!((double)aY + aHeight < 0.96f))
	{
		aY = (float)(0.96f - (double)aHeight);
		if (theSide == 0)
		{
			theSide = 3;
			aX = (float)(((double)theX - 0.07125f) - aWidth);
		}
	}
	if (aX < 0.02f)
		aX = 0.02f;
	if (theRect != nullptr)
	{
		theRect[0] = aX;
		theRect[1] = aY;
		theRect[2] = aBoxWidth;
		theRect[3] = aHeight;
	}
	if (theDraw != 0)
	{
		uint32_t anEdge = addColor(0xff242f92, 0x1e, -0x1e, -0x1e, -0x1e);
		float aTailX0 = 0.0f;
		float aTailY0 = 0.0f;
		float aTailX1 = 0.0f;
		float aTailY1 = 0.0f;
		if (theSide == 0)
		{
			aTailY0 = aY;
			double aLeft = ((double)aX + aBoxWidth) - 0.15625f;
			aTailX0 = (float)aLeft;
			aTailX1 = (float)(aLeft + 0.09125f);
			aTailY1 = (float)(aY + 0.016666668f);
		}
		else if (theSide == 1)
		{
			double aLeft = ((double)aX + aBoxWidth) - 0.15625f;
			aTailX0 = (float)aLeft;
			aTailX1 = (float)(aLeft + 0.09125f);
			double aBottom = (double)aY + aHeight;
			aTailY0 = (float)(aBottom - 0.016666668f);
			aTailY1 = (float)aBottom;
		}
		else if (theSide == 2)
		{
			double aRight = (double)aX + aBoxWidth;
			aTailX0 = (float)(aRight - 0.0125f);
			aTailX1 = (float)aRight;
			double aTop = ((double)aY + aHeight) - 0.14666666f;
			aTailY0 = (float)aTop;
			aTailY1 = (float)(aTop + 0.11f);
		}
		drawPanel(aX, aY, aBoxWidth, aHeight, 0x12, 0xff242f92, 0xfffdfebd, 1, anEdge, 3, aTailX0, aTailY0, aTailX1,
			aTailY1);
		if (theSide == 0)
			drawFrame(mImages[7]->mFrames, (float)(((double)aX + aBoxWidth) - 0.15875f),
				(float)(aY - 0.108333334f), 1.0f, 0);
		else if (theSide == 1)
			drawFrame(mImages[8]->mFrames, (float)(((double)aX + aBoxWidth) - 0.15875f),
				(float)(((double)aY + aHeight) - 0.006666667f), 2.0f, 0);
		else if (theSide == 2)
			drawFrameTurned(mImages[7]->mFrames, (float)(((double)aX + aBoxWidth) - 0.005f),
				(float)(((double)aY + aHeight) - 0.15833333f), 1);
		mFonts[11]->drawText(theText, (float)(aX + 0.01875f), (float)(((double)aY + aPad) + 0.036666665f), aWidth,
			1.0f, 0xfffcff29, 0x9f010101, 0x10a, 1.0f, 0.0f);
	}
}

// 0x402b30
void BonniesApp::windowMoved(int theX, int theY, int theClientX, int theClientY)
{
	if (gOptions != nullptr && theX != 0 && theY != 0)
	{
		gOptions->mWindowX = theX;
		gOptions->mWindowY = theY;
		gOptions->mClientX = theClientX;
		gOptions->mClientY = theClientY;
		gOptions->save();
	}
}

// 0x402b80
void BonniesApp::getWindowPos(int* theX, int* theY)
{
	if (gOptions != nullptr)
	{
		*theX = gOptions->mWindowX;
		*theY = gOptions->mWindowY;
		return;
	}
	*theX = 0x1e;
	*theY = 0x32;
}

// 0x402bc0
void BonniesApp::getWindowPos2(int* theX, int* theY)
{
	if (gOptions != nullptr)
	{
		*theX = gOptions->mClientX;
		*theY = gOptions->mClientY;
		return;
	}
	*theX = 0x1e;
	*theY = 0x32;
}

// 0x402c00
bool BonniesApp::isRunning()
{
	return mState == 3 && isPaused() == 0;
}

// 0x402c20
void BonniesApp::loadResources()
{
	gApp->setDefaultSound(0, lit(0x4574f8));
	gApp->setDefaultSound(1, lit(0x4574e8));
	gApp->setDefaultSound(2, lit(0x4574d8));
	loadImages();
	setStatusFont(exeFont(0x4756cc));
	mFonts[0] = gResources->getFont(exeFont(0x475a08));
	mFonts[1] = gResources->getFont(exeFont(0x475660));
	mFonts[6] = gResources->getFont(exeFont(0x475684));
	mFonts[3] = gResources->getFont(exeFont(0x475954));
	mFonts[4] = gResources->getFont(exeFont(0x475978));
	mFonts[5] = gResources->getFont(exeFont(0x47599c));
	mFonts[9] = gResources->getFont(exeFont(0x475738));
	mFonts[10] = gResources->getFont(exeFont(0x47590c));
	mFonts[11] = gResources->getFont(exeFont(0x4758e8));
	mFonts[8] = gResources->getFont(exeFont(0x4756cc));
	mFonts[7] = gResources->getFont(exeFont(0x4756a8));
	mFonts[2] = gResources->getFont(exeFont(0x475930));
	gGame = new Game();
	gDictionary = new Dictionary();
	gDictionary->init();
	int aChapter = 0;
	int aBook = 0;
	Book* anEntry = gBooks;
	do
	{
		int aLast = 0;
		if (aChapter == 4 || (aChapter == 3 && aBook != 5 && aBook != 0xb))
			aLast = 1;
		anEntry->init(aBook, aChapter, aLast);
		if (aLast == 0)
			aChapter++;
		else
		{
			aChapter = 0;
			aBook++;
		}
		anEntry++;
	} while (anEntry < gBooks + 50);
}

// 0x402ee0
void bonniesExit()
{
	if (gGame != nullptr && gGame->mInProgress != 0 && gOptions != nullptr && !gOptions->mName.empty())
		gGame->save(gOptions->mName);
	if (gBonnies != nullptr)
	{
		gBonnies->shutdown();
		delete gOptions;
		gPortalConfig->unload();
		delete gPortalConfig;
		delete gBonnies;
		gOptions = nullptr;
		gPortalConfig = nullptr;
		gBonnies = nullptr;
	}
	gExited = 1;
	closeRunning();
}

// 0x402fc0
void BonniesApp::setState(int theState, int theLater, int theForce)
{
	if (theState == mState)
		return;
	if (theLater != 0)
	{
		mNextState = theState;
		return;
	}
	if (inModal() && theForce == 0)
		return;
	if (mState == 3 && gGameScreen != nullptr && gGame != nullptr)
	{
		gGameScreen->leave(1);
		gGame->clearTileMarks();
	}
	mNextState = -1;
	gApp->mStatusX = 0.5f;
	gApp->mStatusColor = 0xffefefef;
	gApp->mStatusFlags = 0x10c;
	int aLast = mState;
	switch (theState)
	{
	case 1:
		mScreen = new LogoScreen();
		mScreen->init();
		addScreen(mScreen);
		gLogoScreen = mScreen;
		break;
	case 2:
		mScreen = new MenuScreen();
		mScreen->init();
		addScreen(mScreen);
		gMenuScreen = mScreen;
		break;
	case 3:
		mScreen = new GameScreen();
		mScreen->init();
		addScreen(mScreen);
		gGameScreen = (GameScreen*)mScreen;
		break;
	case 4:
		mScreen = new HelpScreen();
		mScreen->init();
		addScreen(mScreen);
		gState4Screen = mScreen;
		break;
	case 5:
		mScreen = new StoryScreen();
		mScreen->init();
		addScreen(mScreen);
		gState5Screen = mScreen;
		break;
	case 6:
		mScreen = new CreditsScreen();
		mScreen->init();
		addScreen(mScreen);
		gCreditsScreen = mScreen;
		break;
	case 7:
		mScreen = new HighScoresScreen();
		mScreen->init();
		addScreen(mScreen);
		gState7Screen = mScreen;
		break;
	}
	mScreen->mX = 0.0f;
	mScreen->mY = 0.0f;
	mScreen->mWidth = 1.0f;
	mScreen->mHeight = 1.0f;
	if (mState == 3 || mState == 2)
		mLastGameState = mState;
	mPrevState = mState;
	mState = theState;
	switch (aLast)
	{
	case 1:
		deleteScreen(gLogoScreen);
		gLogoScreen = nullptr;
		return;
	case 2:
		deleteScreen(gMenuScreen);
		gMenuScreen = nullptr;
		return;
	case 3:
		deleteScreen(gGameScreen);
		gGameScreen = nullptr;
		return;
	case 4:
		deleteScreen(gState4Screen);
		gState4Screen = nullptr;
		return;
	case 5:
		deleteScreen(gState5Screen);
		gState5Screen = nullptr;
		return;
	case 6:
		deleteScreen(gCreditsScreen);
		gCreditsScreen = nullptr;
		break;
	case 7:
		deleteScreen(gState7Screen);
		gState7Screen = nullptr;
		return;
	}
}

// 0x403440
void BonniesApp::init(int theWidth, int theHeight, int theFullScreen)
{
	mInited = 0;
	mField218 = 0;
	for (int i = 0; i < 8; i++)
		mLoadImages[i] = 0;
	gCaption = getText(0x49b);
	gIconPath = exeGlobalString(0x475a5c);
	mRedrawUntil = 0;
	if (gOptions->mName.empty() || playerSaveExists(gOptions->mName) == 0)
	{
		gOptions->mName = "";
		int aCount;
		Str* aNames = nullptr;
		listFiles(savedGamesDir(false), lit(0x4575cc), &aCount, &aNames);
		if (0 < aCount)
			gOptions->mName = aNames[0];
		delete[] aNames;
	}
	App::init(theWidth, theHeight, theFullScreen);
	gMusic = new Music();
	mNextState = -1;
	mPrevState = -1;
	mLastGameState = 2;
	mState = 0;
	mDebugText = Str();
	mDebug1E8 = 0;
	mDebug1EC = 0;
	mPaused = 0;
	mAutoPaused = 0;
	mSpecialMode = 0;
	mDebug1FC = 0;
	for (int i = 0; i < 12; i++)
		mFonts[i] = nullptr;
	mScreen = nullptr;
	mPauseInactive = 1;
	gGameScreen = nullptr;
	gLogoScreen = nullptr;
	gDictionary = nullptr;
	gGame = nullptr;
	for (int i = 0; i < 36; i++)
		mImages[i] = nullptr;
	gOverlayScreen = new OverlayScreen();
	gOverlayScreen->init();
	addScreen(gOverlayScreen);
	// setState(1, 0, 0), inlined
	setState(1, 0, 0);
	mInited = 1;
	updateMenu();
}

// 0x403980
void BonniesApp::shutdown()
{
	if (mInited != 0)
	{
		mInited = 0;
		deleteScreen(gOverlayScreen);
		gOverlayScreen = nullptr;
		setState(0, 0, 0);
		if (gDictionary != nullptr)
			delete gDictionary;
		gDictionary = nullptr;
		if (gGame != nullptr)
		{
			delete gGame;
			gGame = nullptr;
		}
		for (int i = 0; i < 36; i++)
		{
			if (mImages[i] != nullptr)
				gResources->release(mImages[i]);
		}
		for (int i = 0; i < 12; i++)
		{
			if (mFonts[i] != nullptr)
				gResources->release(mFonts[i]);
		}
		if (gMusic != nullptr)
		{
			delete gMusic;
			gMusic = nullptr;
		}
		App::shutdown();
	}
}

// 0x403da0
void BonniesApp::update()
{
	bool anActive;
	if ((int)ticks() < 0x4e21)
		anActive = true;
	else if (frameCount() <= gGraphics->mFocusFrame + 1 || mPauseInactive == 0)
		anActive = true;
	else
	{
		anActive = false;
		if (mPaused == 0)
		{
			mPaused = 1;
			mAutoPaused = 1;
		}
	}
	if (anActive)
	{
		if (isInactive() == 0 && mPaused != 0 && mAutoPaused != 0)
			mPaused = 0;
		if (mPaused == 0)
			mAutoPaused = 0;
	}
	mLoadImages[0] = 1;
	mLoadImages[1] = 1;
	mLoadImages[2] = 1;
	mLoadImages[3] = 1;
	mLoadImages[4] = 1;
	mLoadImages[5] = 1;
	mLoadImages[6] = 1;
	App::update();
	gMusic->update();
	if (mNextState != -1)
		setState(mNextState, 0, 0);
}

// 0x403e90
int BonniesApp::handleEvent(Event* theEvent)
{
	switch (theEvent->mType)
	{
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		if (gBonnies->mPaused != 0)
		{
			gBonnies->mPaused = 0;
			if (!gBonnies->inModal())
				return 0;
		}
		if ((!hasInputFocus() || gGraphics->mFocusFrame + 1 <= frameCount()) && !gBonnies->inModal())
			return 0;
		break;
	case 8:
	{
		int aKey = theEvent->mA;
		if (aKey < 0x67)
		{
			if (aKey == 0x66)
			{
				if (theEvent->mShift && theEvent->mCtrl)
				{
					mShowFPS = mShowFPS == 0;
					return 0;
				}
			}
			else if (aKey == 0xd)
			{
				if (theEvent->mAlt)
				{
					if (isFullScreen() == 0 || (800 < gBonnies->mDesktopWidth && 600 < gBonnies->mDesktopHeight))
					{
						int aLowColour = isLowColour();
						if (aLowColour == 0)
						{
							gGraphics->setVideoMode(screenWidth(), screenHeight(), isFullScreen() == 0, aLowColour);
							gOptions->mFullScreen = isFullScreen();
							gOptions->save();
							return 0;
						}
					}
					else
						playSound(lit(0x457620), 1.0f);
				}
			}
			else if (aKey == 0x1b && 0 < mSpecialMode)
			{
				mSpecialMode = 0;
				return 0;
			}
		}
		else
		{
			if (aKey == 0x11a)
			{
				if (gBonnies->mState != 1 && gBonnies->mState != 5 && !gBonnies->inModal() &&
					(gGameScreen == nullptr || (gGameScreen->isPlaying() != 0 && !gGameScreen->isBusy())) &&
					gBonnies->mState != 4)
					gBonnies->mNextState = 4;
				return 0;
			}
			if (aKey == 0x11d && theEvent->mAlt)
			{
				gQuit = 1;
				return 0;
			}
		}
		break;
	}
	case 10:
		menuCommand(theEvent->mA);
		return 0;
	case 0xb:
		gQuit = 1;
		break;
	}
	if (App::handleEvent(theEvent) == 0)
		return 0;
	// 0x40e250
	return 1;
}

// 0x403b80
int bonniesMain()
{
	loadLanguage();
	if (checkRunning(lit(0x457608), getText(0x49f), getText(0x49e)) != 0)
		return -1;
	gBonnies = new BonniesApp();
	gPortalConfig = new PortalConfig();
	gPortalConfig->load();
	gOptions = new Options();
	gBonnies->init(800, 600, gOptions->mFullScreen);
	while (gQuit == 0)
	{
		uint32_t aStart = ticks();
		gBonnies->update();
		gBonnies->draw();
		waitUntil(aStart + 0x10, 0);
	}
	bonniesExit();
	return 0;
}

} // namespace bonnies
