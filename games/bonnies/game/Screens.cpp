// The screens of the app's states other than the game's, and the modal screens (see Screens.h).

#include "Screens.h"
#include "App.h"
#include "BonniesApp.h"
#include "Book.h"
#include "Dialog.h"
#include "Draw.h"
#include "Events.h"
#include "File.h"
#include "Font.h"
#include "Game.h"
#include "GameScreen.h"
#include "Globals.h"
#include "Graphics.h"
#include "HighScores.h"
#include "Host.h"
#include "Input.h"
#include "Language.h"
#include "Music.h"
#include "Options.h"
#include "Platform.h"
#include "Resources.h"
#include "Sound.h"
#include "Widget.h"

#include <cmath>
#include <cstring>
#ifdef __EMSCRIPTEN__
#include "Start.h"
#endif

namespace bonnies
{

static uint32_t ticks()
{
	return host()->getTicks();
}

// ---------------------------------------------------------------------------------------------------------------------
// The credits (state 6)

int gCreditsReturn = -1;          // 0x473e28: the state the credits go back to
static int gCreditsPage;          // 0x4780ec

// The credits' table at 0x457f30: 16 entries of 0x14 bytes
static const uint32_t CREDITS = 0x457f30;
static const int CREDITS_SIZE = 0x14;

// 0x40e590: the pages (kind 99 ends one)
static int creditsPages()
{
	int aPages = 1;
	uint32_t anOffset = 0;
	do
	{
		if (exeI32(CREDITS + 8 + anOffset) == 99)
			aPages++;
		if (exeI32(CREDITS + 8 + 0x14 + anOffset) == 99)
			aPages++;
		if (exeI32(CREDITS + 8 + 0x28 + anOffset) == 99)
			aPages++;
		if (exeI32(CREDITS + 8 + 0x3c + anOffset) == 99)
			aPages++;
		anOffset += 0x50;
	} while (anOffset < 0x140);
	return aPages;
}

// 0x40e5d0
void CreditsScreen::load()
{
	if (mLoaded == 0)
	{
		if (gBonnies->mLoadImages[5] == 0)
		{
			gBonnies->mLoadImages[7] = 1;
			return;
		}
		gBonnies->mLoadImages[7] = 0;
		mFont1 = gResources->getFont(exeFont(0x4781ac));
		mFont2 = gResources->getFont(exeFont(0x478188));
		mFont3 = gResources->getFont(exeFont(0x478140));
		mImage = gResources->getImage(Str(lit(0x458070)));
		mLoaded = 1;
	}
}

// 0x40e670
void CreditsScreen::init()
{
	gCreditsPage = 0;
	gCreditsScreen = this;
	Screen::init();
	mFont1 = nullptr;
	mFont2 = nullptr;
	mFont3 = nullptr;
	mImage = nullptr;
	mLoaded = 0;
	load();
}

// 0x40e6a0
void CreditsScreen::shutdown()
{
	removeAll();
	gResources->release(mImage);
	gResources->release(mFont1);
	gResources->release(mFont2);
	gResources->release(mFont3);
	gCreditsScreen = nullptr;
}

// 0x40e6f0
void CreditsScreen::draw()
{
	uint32_t anOffset = 0;
	if (mLoaded != 0)
	{
		drawFrame(mImage->mFrames, 0.0f, 0.0f, 1.0f, 1);
		float aY = 0.085f;
		int aPage = 0;
		do
		{
			uint32_t anEntry = CREDITS + anOffset;
			int aKind = exeI32(anEntry + 8);
			if (aKind == 99)
				aPage++;
			else if (aPage == gCreditsPage)
			{
				Font* aFont = nullptr;
				bool aLine = false;
				if (aKind == 1)
				{
					aFont = mFont1;
					aLine = true;
				}
				else if (aKind == 3)
				{
					Str aText = lit(0x457426);
					aText += getText(exeI32(anEntry + 0x10));
					float aBelow = mFont3->drawText(aText, 0.01875f, aY, 0.9625f, 0.033333335f, 0xff8a0851, 0x5fffffdf,
						0x10a, 1.0f, 0.0f);
					double aNext = aBelow;
					if (aNext == aY)
						aNext = mFont3->lineHeight(1) * 0.65f + aBelow;
					aY = (float)aNext;
				}
				else if (gPortalConfig == nullptr || gPortalConfig->mQuote != 0)
				{
					// The exe also tests a buffer here (0x475508) that nothing fills
					aFont = mFont2;
					aLine = true;
				}
				if (aLine)
				{
					float aSpacing = exeFloat(anEntry + 0xc);
					if (aSpacing <= 0.0f)
					{
						Str aText = lit(0x457426);
						aText += getText(exeI32(anEntry + 0x10));
						aY = aFont->drawText(aText, 0.1875f, aY, 0.625f, 20.0f, exeU32(anEntry), exeU32(anEntry + 4),
							0x10a, 1.0f, 0.0f);
					}
					else
						aY = (float)(aFont->lineHeight(1) * aSpacing + aY);
				}
			}
			anOffset += CREDITS_SIZE;
		} while (anOffset < 0x140);
		if (gCreditsPage == 0)
		{
			Str aText = lit(0x457426);
			aText += lit(0x457424);
			aText += lit(0x458104);
			aText += lit(0x4580a8);
			aText += lit(0x458078);
			mFont3->drawText(aText, 0.1f, 0.74166667f, 0.8f, 0.25f, 0xffbfbf7f, 0xaf000000, 0x10c, 1.0f, 0.0f);
		}
		Screen::draw();
	}
}

// 0x40e9f0
void CreditsScreen::update()
{
	Screen::update();
	load();
}

// 0x40ea00
int CreditsScreen::handleEvent(Event* theEvent)
{
	if (mLoaded != 0)
	{
		switch (theEvent->mType)
		{
		case 3:
		case 5:
		case 7:
		{
			int aPage = gCreditsPage + 1;
			if (creditsPages() <= aPage)
			{
				gBonnies->setState(gCreditsReturn, 1, 0);
				return 0;
			}
			gCreditsPage = aPage;
			return 0;
		}
		case 8:
		{
			int aKey = theEvent->mA;
			if (aKey < 0x133 || (0x136 < aKey && aKey != 0x13c))
			{
				gBonnies->setState(gCreditsReturn, 1, 0);
				return 0;
			}
		}
		}
	}
	return Screen::handleEvent(theEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
// The help (state 4): three pages

// The page's arrows
static Button* helpArrow(Screen* theScreen, float theX, uint32_t theFlags, int theId, uint32_t theNormal,
	uint32_t theHover, uint32_t theDown)
{
	Button* aButton = new Button();
	aButton->init();
	theScreen->addWidget(aButton);
	aButton->mX = theX;
	aButton->mY = 0.905f;
	aButton->mWidth = 0.075f;
	aButton->mHeight = 0.08f;
	aButton->mFlags = theFlags;
	aButton->setUpImage(Str(lit(theNormal)), 0);
	aButton->setFocusImage(Str(lit(theHover)), 0);
	aButton->setDownImage(Str(lit(theDown)), 0);
	aButton->mId = theId;
	aButton->setSound(0, Str(lit(0x458518)));
	aButton->setSound(1, Str(lit(0x458504)));
	return aButton;
}

// 0x415090
void HelpScreen::load()
{
	if (mLoaded == 0)
	{
		if (gBonnies->mLoadImages[3] == 0)
		{
			gBonnies->mLoadImages[7] = 1;
			return;
		}
		gBonnies->mLoadImages[7] = 0;
		Str aName = lit(0x458578);
		aName += intToStr(mPage + 1);
		mImage = gResources->getImage(aName);
		helpArrow(this, 0.9125f, 0x40, 0x44e, 0x45855c, 0x458544, 0x45852c);
		helpArrow(this, 0.0125f, 0x41, 0x44d, 0x4584ec, 0x4584d4, 0x4584bc);
		mLoaded = 1;
	}
}

// 0x415350
void HelpScreen::init()
{
	gState4Screen = this;
	Screen::init();
	mPage = 0;
	mLoaded = 0;
	load();
}

// 0x415370
void HelpScreen::shutdown()
{
	removeAll();
	gResources->release(mImage);
	gState4Screen = nullptr;
}

// 0x4153a0
void HelpScreen::draw()
{
	if (mLoaded != 0)
	{
		drawFrame(mImage->mFrames, 0.0f, 0.0f, 1.0f, 1);
		if (mPage == 2)
		{
			// The tiles of the last page: a wobbling A, a gold G and a fading C
			const Frame* aGlow = gBonnies->mImages[30]->mFrames;
			drawFrame(gBonnies->mImages[31]->mFrames, 0.535f, 0.175f, 1.0f, 0);
			drawFrameGlow(aGlow, 0.535f, 0.175f, 1.0f, 0x55, 1, 2, 0);
			Font* aFont = gBonnies->mFonts[6];
			Str aText = lit(0x458594);
			int aTime = (int)gLastActiveTime;
			double aWobbleX = std::sin((double)(aTime % 800) * 0.00125f * 3.14159f);
			double aWobbleY = std::sin((double)(aTime % 0x514) * 0.00076923077f * 3.14159f);
			aFont->drawText(aText, (float)((aWobbleX - 0.5f) * 4.0f * 0.00125f + 0.79f),
				(float)((aWobbleY - 0.5f) * 4.0f * 0.0016666667f + 0.175f), 0.055f, 0.073333330f, 0xff48252c, 0x6fffffff,
				0x100, 1.0f, 0.0f);
			aText = lit(0x458588);
			drawSweep(0.47375f, 0.64333332f, 0.055f, 0.073333330f, 0x7887fb87);
			aFont->drawText(aText, 0.47375f, 0.645f, 0.055f, 0.073333330f, 0xff48252c, 0x6fffffff, 0x100, 1.0f, 0.0f);
			Str aText2 = lit(0x458584);
			aTime = (int)gLastActiveTime;
			uint32_t aColor = 0xffffffff;
			uint32_t aShadow = 0xaf000000;
			double aLevel = std::sin((double)(aTime % 0x708) * 0.00055555557f * 3.14159f) * 0.4f + 0.5f;
			if (aLevel < 1.0f)
			{
				aColor = (uint32_t)ftol(255.0f * aLevel) << 24 | 0xffffff;
				aShadow = (uint32_t)ftol(aLevel * 175.0f) << 24;
			}
			aFont->drawText(aText2, 0.78875f, 0.645f, 0.055f, 0.073333330f, aColor, aShadow, 0x100, 1.0f, 0.0f);
		}
		Screen::draw();
	}
}

// 0x4156a0
void HelpScreen::update()
{
	load();
	Screen::update();
}

// 0x4156b0: theStep pages on; past the last, back to the game or the menu
void HelpScreen::turn(int theStep)
{
	int aPage = mPage + theStep;
	mPage = aPage;
	if (aPage < 3)
	{
		Widget* aBack = findWidget(0x44d);
		if (aPage == 0)
			aBack->mFlags |= 1;
		else
			aBack->mFlags &= ~1u;
		Str aName = lit(0x458578);
		aName += intToStr(mPage + 1);
		gResources->release(mImage);
		mImage = gResources->getImage(aName);
		return;
	}
	if (gBonnies->mLastGameState == 3)
	{
		gBonnies->setState(3, 1, 0);
		return;
	}
	gBonnies->setState(2, 1, 0);
}

// 0x4157f0
int HelpScreen::handleEvent(Event* theEvent)
{
	if (Screen::handleEvent(theEvent) == 0)
		return 0;
	if (mLoaded != 0)
	{
		switch (theEvent->mType)
		{
		case 3:
			// Not over the back arrow
			if (theEvent->mX < 0.125f && 0.8333333f < theEvent->mY)
				return 0;
			// fall through
		case 5:
		case 7:
			turn(1);
			return 0;
		case 8:
			if (theEvent->mA == 0xd)
			{
				turn(1);
				return 0;
			}
			if (theEvent->mA == 0x1b)
			{
				turn(99);
				return 0;
			}
			break;
		case 0xd:
			if (theEvent->mA == 0x44d)
			{
				turn(-1);
				return 0;
			}
			if (theEvent->mA == 0x44e)
			{
				turn(1);
				return 0;
			}
			break;
		}
	}
	return Screen::handleEvent(theEvent);
}


// ---------------------------------------------------------------------------------------------------------------------
// The high scores (state 7): the timed game's table and the other's, and the player's record

int gHighScoresToGame;            // 0x478d9c: the screen goes back to the game, not the menu

// The tables' columns: the rank, the name, the longest word and the score
static const float HIGH_SCORE_WIDTHS[5] = {0.0375f, 0.17375f, 0.15625f, 0.1375f, 0.0f};

// 0x4158d0
void HighScoresScreen::init()
{
	if (gGame->mInProgress == 0 && !gOptions->mName.empty() && playerSaveExists(gOptions->mName) != 0)
		gGame->load(gOptions->mName, 1);
	gState7Screen = this;
	mFont1 = gResources->getFont(exeFont(0x478f10));
	mFont2 = gResources->getFont(exeFont(0x478eec));
	mFont3 = gResources->getFont(exeFont(0x478ec8));
	if (gHighScores.mLoaded == 0)
		gHighScores.load();
	Screen::init();
	mImage = gResources->getImage(Str(lit(0x4585d0)));
	mStart = ticks();
	mTable = gHighScoreTable & -(int)(0 <= gHighScoreTable);
	int i = 0;
	do
	{
		Button* aButton = new Button();
		aButton->init();
		addWidget(aButton);
		aButton->mId = (i != 0) + 0x385;
		aButton->mX = i == 0 ? 0.22125f : 0.71125f;
		aButton->mY = 0.051666666f;
		aButton->mWidth = 0.0575f;
		aButton->mHeight = 0.056666669f;
		aButton->mFlags = 0x40;
		aButton->setUpImage(Str(lit(0x4585b4)), i * 3);
		aButton->setFocusImage(Str(lit(0x4585b4)), i * 3 + 1);
		aButton->setDownImage(Str(lit(0x4585b4)), i * 3 + 2);
		aButton->setSound(0, Str(lit(0x458518)));
		aButton->setSound(1, Str(lit(0x458504)));
		i++;
	} while (i < 2);
	update();
}

// 0x415b20
void HighScoresScreen::shutdown()
{
	removeAll();
	gResources->release(mFont1);
	gResources->release(mFont2);
	gResources->release(mFont3);
	gResources->release(mImage);
	gState7Screen = nullptr;
}

// 0x415b70
void HighScoresScreen::draw()
{
	drawFrame(mImage->mFrames, 0.0f, 0.0f, 1.0f, 1);
	Str aTitle = getText(0x4b5);
	aTitle += lit(0x4585f0);
	aTitle += getText(mTable == 0 ? 0x4aa : 0x4ab);
	aTitle += lit(0x4585f4);
	mFont1->drawText(aTitle, 0.0f, 0.043333333f, 1.0f, 0.05f, 0xffcfcfff, 0xaf000000, 0x8a, 1.0f, 0.0f);
	float aY = 0.215f;
	int aRow = -1;
	do
	{
		float aX = 0.2375f;
		int aColumn = 0;
		do
		{
			uint32_t aFlags = (aColumn != 1 ? 0x20 : 0x10);
			if (aRow == -1)
				drawCText(mFont2, getText(exeI32(0x473e40 + aColumn * 4)), aX, aY, HIGH_SCORE_WIDTHS[aColumn], 0.05f,
					0xffcfcfff, 0xaf000000, aFlags | 0x84, 1.0f, 0.0f);
			else
			{
				HighScore& anEntry = gHighScores.mScores[aRow + mTable * 5];
				if (anEntry.mUsed != 0)
				{
					Str aText = lit(0x457426);
					switch (aColumn)
					{
					case 0:
						aText += intToStr(aRow + 1);
						aText += lit(0x4585f4);
						break;
					case 1:
						aText = anEntry.mName;
						break;
					case 2:
						aText = anEntry.mWord;
						break;
					case 3:
						aText += intToStr(anEntry.mScore);
						break;
					}
					uint32_t aColor = 0xffffff00;
					if (mTable == gHighScoreTable && gHighScoreRank == aRow)
					{
						// The last game's score pulses
						int aTime = (int)ticks() % 800;
						double aWave = std::sin((double)aTime * 0.00125f * 3.14159f);
						int aRed = ftol(20.0f * aWave);
						int aGreen = ftol(aWave * -10.0f);
						aColor = addRGB(0xff8fff8f, aRed, aGreen, aRed);
					}
					float aWidth = HIGH_SCORE_WIDTHS[aColumn];
					float aLeft = aX;
					if (aColumn == 2 || aColumn == 3)
						aLeft = (float)(aX - (double)0.05f);
					if (aColumn == 1 || aColumn == 2 || aColumn == 3)
						aWidth = (float)(aWidth + (double)0.05f);
					mFont2->drawText(aText, aLeft, aY, aWidth, 0.05f, aColor, 0xaf00004f, aFlags | 0x84, 1.0f, 0.0f);
				}
			}
			aColumn++;
			aX = (float)((aX + (double)HIGH_SCORE_WIDTHS[aColumn - 1]) + 0.0075f);
		} while (aColumn < 4);
		aY = (float)(aY + (double)(aRow == -1 ? 0.063333333f : 0.055f));
		aRow++;
	} while (aRow < 5);
	if (gGame != nullptr && 0 < gGame->mRecord.mField0C)
	{
		// The player's record
		Str aHeading = lit(0x457426);
		aY = (float)(aY + (double)0.023333333f);
		Str aName = gOptions->mName;
		aHeading += getText(0x4af);
		aHeading += aName;
		mFont3->drawText(aHeading, 0.0f, aY, 1.0f, 0.05f, 0xffcfcfff, 0xaf000000, 0x8a, 1.0f, 0.0f);
		aY = (float)(aY + (double)0.043333333f);
		int aLine = 0;
		do
		{
			Str aLabel = lit(0x457426);
			Str aValue = lit(0x457426);
			switch (aLine)
			{
			case 0:
				aLabel += getText(0x4b0);
				aValue += intToStr(gGame->mRecord.mGames);
				break;
			case 1:
			{
				aLabel += getText(0x4b1);
				int aCount = 1;
				if (1 <= gGame->mRecord.mField0C)
					aCount = gGame->mRecord.mField0C;
				appendFloat(aValue, (float)((double)gGame->mRecord.mField10 / aCount));
				break;
			}
			case 2:
				aLabel += getText(0x4b2);
				aValue += gGame->mRecord.mStr14;
				break;
			case 3:
				aLabel += getText(0x4b3);
				aValue += intToStr(gGame->mRecord.mField1C);
				break;
			}
			mFont3->drawText(aLabel, 0.12499997f, aY, 0.4625f, 0.05f, 0xffffff00, 0xaf00004f, 0xa1, 1.0f, 0.0f);
			mFont3->drawText(aValue, 0.59999996f, aY, 0.25f, 0.05f, 0xffffff00, 0xaf00004f, 0x91, 1.0f, 0.0f);
			aY = (float)(aY + (double)0.035f);
			aLine++;
		} while (aLine < 4);
	}
	Str aFooter = getText(0x4b4);
	mFont2->drawText(aFooter, 0.19999999f, 0.90833336f, 0.6f, 1.0f, 0xffcfcfff, 0xaf000000, 0x28a, 1.0f, 0.0f);
	Screen::draw();
}

// 0x4162b0: the arrows of the other table
void HighScoresScreen::update()
{
	Widget* aLeft = findWidget(0x385);
	if (aLeft != nullptr)
	{
		if (mTable < 1)
			aLeft->mFlags |= 1;
		else
			aLeft->mFlags &= ~1u;
	}
	Widget* aRight = findWidget(0x386);
	if (aRight != nullptr)
	{
		if (mTable < 1)
			aRight->mFlags &= ~1u;
		else
			aRight->mFlags |= 1;
	}
	Screen::update();
}

// 0x416310
void HighScoresScreen::turn(int theStep)
{
	int aTable = mTable + theStep;
	mTable = aTable;
	if (aTable < 0)
		mTable = 0;
	if (1 < mTable)
		mTable = 1;
}

// 0x416340
int HighScoresScreen::handleEvent(Event* theEvent)
{
	if (Screen::handleEvent(theEvent) == 0)
		return 0;
	switch (theEvent->mType)
	{
	case 3:
		// Not on the title's arrows
		if (0.25f <= theEvent->mX && theEvent->mX <= 0.75f && theEvent->mY <= 0.2f)
			return 0;
		// fall through
	case 5:
	case 7:
	leave:
		if (gHighScoresToGame == 0)
		{
			gBonnies->setState(2, 1, 0);
			return 0;
		}
		gHighScoresToGame = 0;
		gBonnies->setState(3, 1, 0);
		return 0;
	case 8:
		switch (theEvent->mA)
		{
		case 0x104:
		case 0x114:
			turn(-1);
			break;
		case 0x106:
		case 0x113:
			turn(1);
			return 1;
		case 0x133:
		case 0x134:
		case 0x135:
		case 0x136:
		case 0x13c:
			break;
		default:
			goto leave;
		}
		break;
	case 0xd:
		if (theEvent->mA == 0x385)
		{
			turn(-1);
			return 0;
		}
		if (theEvent->mA == 0x386)
		{
			turn(1);
			return 0;
		}
		break;
	}
	return 1;
}


// ---------------------------------------------------------------------------------------------------------------------
// The logos (state 1): the game's logo of nine falling letters, then PopCap's and the distributor's

static int gLogoLoaded;           // 0x4791b4: the app's resources are loaded
static int gLogoSkip;             // 0x4791b8: a click or key hurries the logos
bool gFixSkipLogos = false;
static int gLogoFade = 0x96;      // 0x473e50: a logo's fade, in ms
static int gPopCapTime = 0x5dc;   // 0x473e54: PopCap's logo shows this long
static int gDistributorTime = 0x5dc;  // 0x473e58

// The letters' positions (0x458608: x, y in pixels of 800x600)
static int letterX(int theLetter)
{
	return exeI32(0x458608 + theLetter * 8);
}

static int letterY(int theLetter)
{
	return exeI32(0x45860c + theLetter * 8);
}

// 0x4164c0
void LogoScreen::init()
{
	gLogoScreen = this;
	Screen::init();
	mPopCap = nullptr;
	mDistributor = nullptr;
	if (imageExists(Str(lit(0x45869c))) != 0)
		mPopCap = gResources->getImage(Str(lit(0x45869c)));
	if (imageExists(Str(lit(0x458690))) != 0)
		mDistributor = gResources->getImage(Str(lit(0x458690)));
	mGames = gResources->getImage(Str(lit(0x458680)));
	mTM = gResources->getImage(Str(lit(0x458674)));
	mCrayon = gResources->getImage(Str(lit(0x458664)));
	int i = 0;
	do
	{
		Str aName = lit(0x457426);
		aName += lit(0x458650);
		aName += intToStr(i);
		mLetters[i] = gResources->getImage(aName);
		i++;
	} while (i < 9);
	mHoldUntil = 0;
	mFallStart = 0;
	mLastUpdate = 0;
	mShown = 0;
	mDone = 0;
	mFadeEnd = 0;
	mField12C = 0;
	mShowPopCap = 0;
	mShowDistributor = 0;
	mShowWhite = 0;
	mShowLetters = 1;
	mPopCapShown = 0;
	mDistributorShown = 0;
	mEarly = 1;
	i = 0;
	do
	{
		int aHeight = mLetters[i]->mFrames->mHeight;
		int aY = letterY(i);
		int aScreen = screenHeight();
		mSpeeds[i] = 0.0f;
		mBounces[i] = 0;
		float aTop = (float)((double)(-10 - aY) * 0.0016666667f);
		float aSize = (float)aHeight;
		mOffsets[i] = (float)(aTop - (double)aSize / (double)aScreen);
		mLanded = 0;
		mLandTime = 0;
		i++;
	} while (i < 9);
	mCursorHidden = 0;
	if (isFullScreen() != 0)
	{
		hideCursor();
		mCursorHidden = 1;
	}
}

// 0x4167a0
void LogoScreen::shutdown()
{
	removeAll();
	gResources->release(mGames);
	gResources->release(mCrayon);
	gResources->release(mTM);
	Image** aLetter = mLetters;
	int i = 9;
	do
	{
		gResources->release(*aLetter);
		aLetter++;
		i--;
	} while (i != 0);
	if (mPopCap != nullptr)
		gResources->release(mPopCap);
	if (mDistributor != nullptr)
		gResources->release(mDistributor);
	if (mCursorHidden != 0)
		showCursor();
	gLogoScreen = nullptr;
}

// 0x416840
void LogoScreen::draw()
{
	if (isFullScreen() != 0 && (int)ticks() < (int)(gGraphics->mModeTime + 0x514))
	{
		fillRect(0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff);
		return;
	}
	if (mEarly != 0)
	{
		// The distributor's logo first, when Portal.cfg asks for it
		if (gPortalConfig->mDistributorLogoFirst != 0 && mDistributor != nullptr)
		{
			if (mFadeEnd == 0)
			{
				Str aPath = soundDir(true);
				aPath += lit(0x4586a4);
				if (fileExists(aPath))
					playSound(Str(lit(0x458690)), 1.0f, 1, 0x14);
				mFadeEnd = ticks();
			}
			uint32_t aNow = ticks();
			uint32_t aStart = mFadeEnd;
			if ((int)(aNow - aStart) <= gLogoFade + 200 + gDistributorTime)
			{
				drawFrame(mDistributor->mFrames, 0.0f, 0.0f, 1.0f, 1);
				double aLevel = (double)(int)(((aNow - aStart) - gDistributorTime) - 200) / (double)gLogoFade;
				if (aLevel <= 0.01f)
					return;
				fillRect(0.0f, 0.0f, 1.0f, 1.0f, (uint32_t)ftol(aLevel * 255.0f) << 24 | 0xffffff);
				return;
			}
			mFadeEnd = 0;
		}
		mEarly = 0;
		mShowLetters = 1;
	}
	if (mFallStart == 0)
		mFallStart = ticks();
	ticks();
	float aFade = 0.0f;
	if (mShown != 0 && mFadeEnd != 0 && 0 < gLogoFade)
		aFade = (float)(1.0 - (double)(int)(mFadeEnd - ticks()) / (double)gLogoFade);
	if (mShowLetters != 0)
	{
		fillRect(0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff);
		int i = 0;
		do
		{
			if (i == 9)
			{
				if (mLandTime != 0)
				{
					int aSince = (int)(ticks() + (-2 - mLandTime));
					if (0 < aSince)
					{
						// The crayon, "Games" and the TM fade in once the letters have landed
						double aLevel = (double)aSince * 0.004f;
						int aFull;
						if (aLevel <= 1.0f)
							aFull = aLevel < 1.0f ? 0 : 1;
						else
						{
							aFull = 1;
							aLevel = 1.0f;
						}
						float anAlpha = (float)aLevel;
						mShown = aFull;
						drawFrame(mCrayon->mFrames, 0.195f, 0.28666666f, anAlpha, 1);
						drawFrame(mGames->mFrames, 0.72125f, 0.68666667f, anAlpha, 1);
						drawFrame(mTM->mFrames, 0.9125f, 0.47333333f, anAlpha, 1);
					}
				}
			}
			else
				drawFrame(mLetters[i]->mFrames, (float)((double)letterX(i) * 0.00125f),
					(float)((double)letterY(i) * 0.0016666667f + mOffsets[i]), 1.0f, 0);
			i++;
		} while (i < 10);
	}
	if (mPopCapShown == 0)
	{
		if (mShowPopCap != 0 && 0.01f < aFade)
			drawFrame(mPopCap->mFrames, 0.0f, 0.0f, aFade, 1);
	}
	else
		drawFrame(mPopCap->mFrames, 0.0f, 0.0f, 1.0f, 1);
	if (mDistributorShown == 0)
	{
		if (mShowDistributor != 0 && 0.01f < aFade)
			drawFrame(mDistributor->mFrames, 0.0f, 0.0f, aFade, 1);
	}
	else
		drawFrame(mDistributor->mFrames, 0.0f, 0.0f, 1.0f, 1);
	if ((mShowWhite != 0 || mDone != 0) && (mDone != 0 || 0.01f < aFade))
		fillRect(0.0f, 0.0f, 1.0f, 1.0f, (uint32_t)ftol(aFade * 255.0f) << 24 | 0xffffff);
	Screen::draw();
	if (mDone != 0)
		gBonnies->setState(2, 1, 0);
}

// 0x416d70
void LogoScreen::update()
{
	if (isFullScreen() != 0 && (int)ticks() < (int)(gGraphics->mModeTime + 0x514))
		return;
	if (gLogoSkip != 0)
	{
		gLogoFade = 0x50;
		gPopCapTime = 600;
		gDistributorTime = 600;
	}
	Screen::update();
	if (mEarly != 0)
		return;
	if (mLastUpdate == 0)
	{
		playSound(Str(lit(0x4586c4)), 1.0f, 1, 0x14);
		mLastUpdate = ticks();
	}
	if (0 < (int)mFallStart)
	{
		// The letters fall in one after another and bounce
		int anElapsed = (int)(ticks() - mFallStart);
		int aDelta = (int)(ticks() - mLastUpdate);
		if (100 <= aDelta)
			aDelta = 100;
		float aSeconds = (float)((double)aDelta * 0.001f);
		int i = 0;
		do
		{
			if (0 < anElapsed - i * 0x50)
			{
				if (mSpeeds[i] == 0.0f)
					mSpeeds[i] = 0.42f;
				mSpeeds[i] = (float)((double)(aDelta * 0x5aa) * 0.001f * 0.0016666667f + mSpeeds[i]);
			}
			float aY = (float)((double)aSeconds * mSpeeds[i] + mOffsets[i]);
			mOffsets[i] = aY;
			if (0.0f < aY)
			{
				int aBounces = mBounces[i];
				if (aBounces < 4)
				{
					mBounces[i] = aBounces + 1;
					mOffsets[i] = (float)(aY * (double)-0.35f);
					mSpeeds[i] = (float)(mSpeeds[i] * (double)-0.35f);
				}
				else
				{
					if (i == 5 && mLanded == 0)
					{
						mLanded = 1;
						mLandTime = ticks();
					}
					mOffsets[i] = 0.0f;
				}
			}
			i++;
		} while (i < 9);
		if (mLanded == 0 && 5000 < (int)(ticks() - mFallStart))
		{
			// The exe sets the nine one by one
			for (int j = 0; j < 9; j++)
			{
				mOffsets[j] = 0.0f;
				mSpeeds[j] = 0.0f;
				mBounces[j] = 99;
			}
			mLanded = 1;
			mLandTime = ticks();
		}
	}
	if (gLogoLoaded == 0 && mShown != 0)
	{
		gLogoLoaded = 1;
		gBonnies->loadResources();
	}
	// port (mod): the fixes mod's "skipLogos": on to the menu once the resources are in, as a finished logo goes
	if (gFixSkipLogos && gLogoLoaded != 0)
		mDone = 1;
	int aWait = 0xdac;
	if (gLogoSkip != 0)
		aWait = 0x5dc;
	if (mFadeEnd != 0 && (int)mFadeEnd < (int)ticks())
	{
		// The fade out is over: the next logo
		mFadeEnd = 0;
		mShowLetters = 0;
		mPopCapShown = 0;
		mDistributorShown = 0;
		if (mShowPopCap == 0)
		{
			if (mShowDistributor == 0)
				mDone = 1;
			else
			{
				mDistributorShown = 1;
				Str aPath = soundDir(true);
				aPath += lit(0x4586a4);
				if (fileExists(aPath))
					playSound(Str(lit(0x458690)), 1.0f, 1, 0x14);
			}
		}
		else
		{
			mPopCapShown = 1;
			Str aPath = soundDir(true);
			aPath += lit(0x4586b4);
			if (fileExists(aPath))
				playSound(Str(lit(0x45869c)), 1.0f, 1, 0x14);
		}
		mShowWhite = 0;
		mShowDistributor = 0;
		mShowPopCap = 0;
		mHoldUntil = ticks() + 1000;
		if (mPopCapShown != 0)
			mHoldUntil = ticks() + gPopCapTime;
		if (mDistributorShown != 0)
			mHoldUntil = ticks() + gDistributorTime;
	}
	if (gLogoLoaded != 0 && (int)(mFallStart + aWait) < (int)ticks() && (int)mHoldUntil < (int)ticks())
	{
		if (mFadeEnd == 0)
		{
			// The fade into the next
			mFadeEnd = ticks() + gLogoFade;
			if (mShowLetters == 0)
			{
				if (mPopCapShown == 0)
				{
					if (mDistributorShown != 0)
						mShowWhite = 1;
				}
				else if (mDistributor == nullptr || gPortalConfig->mDistributorLogoFirst != 0)
					mShowWhite = 1;
				else
					mShowDistributor = 1;
			}
			else if (mPopCap == nullptr)
			{
				if (mDistributor == nullptr || gPortalConfig->mDistributorLogoFirst != 0)
					mShowWhite = 1;
				else
					mShowDistributor = 1;
			}
			else
				mShowPopCap = 1;
		}
		if (gLogoFade == 0)
			mDone = 1;
	}
	mLastUpdate = ticks();
}

// 0x417340
int LogoScreen::handleEvent(Event* theEvent)
{
	int aType = theEvent->mType;
	if (1 < aType && (aType < 8 || aType == 8))
	{
		gLogoSkip = 1;
		return 0;
	}
	return Screen::handleEvent(theEvent);
}


// ---------------------------------------------------------------------------------------------------------------------
// The modal dialogs on the "Options" panel

// 0x418760
void OptionsPanel::init()
{
	Screen::init();
	mDrawsPanel = 0;
	mImage = gResources->getImage(Str(lit(0x4586e4)));
}

// 0x419380
void OptionsPanel::shutdown()
{
	removeAll();
	gResources->release(mImage);
}

// 0x417370
void OptionsPanel::draw()
{
	if (isTop() != 0)
		drawVignette(0.6f);
	drawFrame(mImage->mFrames, mX, mY, 1.0f, 0);
	Screen::draw();
}

// A text on the panel (a text field that is not edited)
static TextField* panelText(Screen* theScreen, float theX, float theY, float theW, float theH, uint32_t theColor,
	Str theText, uint32_t theFlags, uint32_t theFont)
{
	TextField* aText = new TextField();
	aText->init();
	theScreen->addWidget(aText);
	aText->mX = theX;
	aText->mY = theY;
	aText->mWidth = theW;
	aText->mHeight = theH;
	aText->mColor = theColor;
	aText->mColor2 = 0x7f000000;
	aText->mText = theText;
	aText->mTextFlags = theFlags;
	aText->setFont(exeFont(theFont));
	return aText;
}

// The image of a panel button and the box of its text
static void panelButtonImage(Button* theButton, int theFrame)
{
	theButton->mTextWidth = (float)(theButton->mWidth - (double)0.0325f);
	theButton->mTextX = 0.01625f;
	theButton->mTextY = 0.016666668f;
	theButton->mTextHeight = 0.035f;
	theButton->setUpImage(Str(lit(0x458708)), theFrame);
	theButton->setFocusImage(Str(lit(0x458708)), theFrame + 1);
	theButton->setDownImage(Str(lit(0x458708)), theFrame + 2);
}

// 0x4173c0
int NamePanel::handleEvent(Event* theEvent)
{
	int aResult = 0;
	int aCommand;
	if (theEvent->mType == 0xd)
		aCommand = theEvent->mA - 0x321;
	else
		aCommand = theEvent->mType - 0x10;
	if (aCommand == 0)
	{
		// OK: the name, unless a player has it already
		aResult = 1;
		Str aText = findWidget(0x323)->mText;
		Str aName = trim(aText, false);
		if (!aName.empty())
		{
			if (playerSaveExists(aName) != 0)
			{
				messageDialog(1, Str(getText(0x4f6)), Str(), Str(), Str(), Str(), 0, 0, 0, 0, 0);
				static_cast<TextField*>(findWidget(0x323))->beginEdit();
				return 0;
			}
			saveGameAs(aName);
			gOptions->mName = aName;
			gOptions->save();
#ifdef __EMSCRIPTEN__
			// port (web): the site's players (Start.h)
			if (gSitePlayersChanged != nullptr)
				gSitePlayersChanged();
#endif
		}
	}
	else if (aCommand != 1)
		return Screen::handleEvent(theEvent);
	gApp->endModal(aResult, 0);
	return 0;
}

// 0x4175f0: asks for a new player's name (theCancel: with a cancel button); whether one was given
int askName(int theCancel)
{
	NamePanel* aPanel = new NamePanel();
	aPanel->init();
	aPanel->mX = 0.2825f;
	aPanel->mY = 0.114f;
	aPanel->mWidth = 0.435f;
	aPanel->mHeight = 0.74666667f;
	aPanel->mColor = 0xff153661;
	aPanel->mDraggable = 1;
	panelText(aPanel, 0.0f, 0.09f, 0.435f, 0.055f, addRGB(0xffdcb970, 0xf, 0xf, 0xf), Str(getText(0x4f7)), 0x108,
		0x479668);
	panelText(aPanel, 0.075f, 0.16f, 0.285f, 0.13833334f, 0xffdcb970, Str(getText(0x4f8)), 0x109, 0x479644);
	TextField* aField = new TextField();
	aField->init();
	aPanel->addWidget(aField);
	aField->mX = 0.075f;
	aField->mY = 0.32666668f;
	aField->mWidth = 0.285f;
	aField->mHeight = 0.055f;
	aField->mColor = 0xffdcb970;
	aField->mColor2 = 0x7f000000;
	aField->mText = lit(0x457426);
#ifdef __EMSCRIPTEN__
	// port (web): the site player's name, under the site's "fill" and "create" (Start.h; src/SiteProfiles.cpp)
	if (gSiteNameFor != nullptr)
		aField->mText = gSiteNameFor();
#endif
	aField->mEditable = 1;
	aField->mNamesOnly = 1;
	aField->mEscapeCancels = theCancel;
	aField->mMaxLength = 0x14;
	aField->mPanelColor = 0xff3f3f3f;
	aField->mTextFlags = 0x108;
	aField->mId = 0x323;
	aField->setFont(exeFont(0x479644));
	aField->beginEdit();
	Button* anOK = new Button();
	anOK->init();
	aPanel->addWidget(anOK);
	anOK->mX = theCancel != 0 ? 0.05625f : 0.11875f;
	anOK->mY = 0.47166666f;
	anOK->mWidth = theCancel != 0 ? 0.16125f : 0.19375f;
	anOK->mHeight = 0.075f;
	anOK->mColor = 0xffebc87f;
	anOK->mColor2 = 0x7f000000;
	anOK->mTextFlags = 0x100;
	anOK->mFlags = 0x2c0;
	anOK->mText = getText(0x4f9);
	anOK->mId = 0x321;
	anOK->setFont(exeFont(0x4797d0));
	anOK->setSound(0, Str(lit(0x45872c)));
	anOK->setSound(1, Str(lit(0x45871c)));
	panelButtonImage(anOK, -(int)(theCancel != 0) & 3);
	if (theCancel != 0)
	{
		Button* aCancel = new Button();
		aCancel->init();
		aPanel->addWidget(aCancel);
		aCancel->mX = 0.21625f;
		aCancel->mY = 0.47166666f;
		aCancel->mWidth = 0.16125f;
		aCancel->mHeight = 0.075f;
		aCancel->mColor = 0xffebc87f;
		aCancel->mColor2 = 0x7f000000;
		aCancel->mTextFlags = 0x100;
		aCancel->mFlags = 0x2c0;
		aCancel->mText = getText(0x4fa);
		aCancel->mKeys[0] = 0x1b;
		aCancel->mId = 0x322;
		aCancel->setFont(exeFont(0x4797d0));
		panelButtonImage(aCancel, 3);
	}
	int aResult = gApp->runModal(1, aPanel);
	if (gMenuScreen != nullptr)
		static_cast<MenuScreen*>(gMenuScreen)->refresh();
	return aResult;
}

// ---------------------------------------------------------------------------------------------------------------------
// The options (the menu's and the game's): the volumes, full screen and the buttons

static Screen* gOptionsMenu;      // 0x4799e0: while it is being made

// 0x417b40: the widgets, once the menu's images are loaded
void OptionsMenu::load()
{
	if (mLoaded != 0)
		return;
	if (gBonnies->mLoadImages[4] == 0)
	{
		gBonnies->mLoadImages[7] = 1;
		return;
	}
	gBonnies->mLoadImages[7] = 0;
	mImage = gResources->getImage(Str(lit(0x4586e4)));
	int anInGame = gBonnies->mState == 3;
	panelText(this, 0.0f, 0.09f, 0.435f, 0.055f, addRGB(0xffdcb970, 0xf, 0xf, 0xf), Str(getText(0x514)), 0x108,
		0x479a78);
	int aButtons = 2;
	if (anInGame != 0)
		aButtons = 4;
	float aY = 0.16f;
	int i = 0;
	do
	{
		// The sound's and the music's volumes
		Slider* aSlider = new Slider();
		aSlider->init();
		addWidget(aSlider);
		aSlider->mY = (float)(aY + (double)0.0016666667f);
		aSlider->mX = 0.195f;
		aSlider->mWidth = 0.18f;
		aSlider->mHeight = 0.055f;
		aSlider->mKnobColor = 0xffffff00;
		aSlider->mId = (i != 0) * 2 + 0x26d;
		if (i == 0)
			aSlider->mValue = gOptions->mSoundVolume;
		else
			aSlider->mValue = gOptions->mMusicVolume;
		aSlider->setKnob(Str(lit(0x458770)), 0);
		TextField* aLabel = new TextField();
		aLabel->init();
		addWidget(aLabel);
		aLabel->mX = 0.0125f;
		aLabel->mY = aY;
		aLabel->mWidth = 0.195f;
		aLabel->mHeight = 0.055f;
		aLabel->mColor2 = 0x7f000000;
		aLabel->mColor = 0xffdcb970;
		aLabel->mText = getText(i == 0 ? 0x515 : 0x516);
		aLabel->mTextFlags = 0x120;
		aLabel->mId = (i != 0) * 2 + 0x26e;
		aLabel->setFont(exeFont(0x479a54));
		aY = (float)(aY + (double)0.07f);
		i++;
	} while (i < 2);
	aY = (float)(aY + (double)0.0066666668f);
	CheckBox* aFull = new CheckBox();
	aFull->init();
	addWidget(aFull);
	aFull->mX = 0.12875f;
	aFull->mY = aY;
	aFull->mWidth = 0.17875f;
	aFull->mHeight = 0.055f;
	aFull->mBoxWidth = 0.035f;
	aFull->mBoxHeight = 0.046666667f;
	aFull->mColor = 0xffdcb970;
	aFull->mColor2 = 0x7f000000;
	aFull->mFlags = 0x90;
	aFull->setFont(exeFont(0x479a54));
	aFull->mText = getText(0x517);
	aY = (float)(aY + (double)0.086666666f);
	aFull->mTextFlags = 0x100;
	aFull->mId = 0x271;
	aFull->mChecked = gOptions->mFullScreen;
	// No window on a desktop of 800x600 or less, nor on one of 256 colours
	if (isFullScreen() != 0 && (gBonnies->mDesktopWidth <= 800 || gBonnies->mDesktopHeight <= 600))
		aFull->mFlags |= 1;
	if (isLowColour() != 0)
		aFull->mFlags |= 1;
	aY = (float)(aY + (double)0.02f);
	int aFirst = 4 - aButtons;
	int aButton = 0;
	do
	{
		// The menu has only the last three
		if (aFirst <= aButton)
		{
			Button* aWidget = new Button();
			aWidget->init();
			addWidget(aWidget);
			aWidget->mFlags = 0x2c0;
			aWidget->setFont(exeFont(0x479be0));
			aWidget->mTextFlags = 0x100;
			aWidget->setSound(0, Str(lit(0x45872c)));
			aWidget->setSound(1, Str(lit(0x45871c)));
			if (aButton == 0)
			{
				aWidget->mText = getText(0x518);
				aWidget->mTooltip = getText(0x519);
				aWidget->mId = 0x25c;
			}
			else if (aButton == 1)
			{
				aWidget->mText = getText(0x51a);
				aWidget->mTooltip = getText(0x51b);
				aWidget->mId = 0x259;
			}
			else if (aButton == 2)
			{
				aWidget->mText = getText(0x611);
				aWidget->mTooltip = getText(0x612);
				aWidget->mId = 0x25b;
			}
			else if (aButton == 3)
			{
				aWidget->mText = getText(0x51c);
				aWidget->mTooltip = getText(0x51d);
				aWidget->mId = 0x25a;
			}
			else if (aButton == 4)
			{
				aY = (float)(aY + (double)0.016666668f);
				aWidget->mText = getText(anInGame != 0 ? 0x51e : 0x51f);
				aWidget->mTooltip = getText(anInGame != 0 ? 0x520 : 0x521);
				aWidget->mId = 0x25d;
				aWidget->mKeys[0] = 0x1b;
				aWidget->mKeys[1] = 0xd;
			}
			aWidget->mColor = 0xffebc87f;
			aWidget->mColor2 = 0x7f000000;
			if (aButtons == 4 && aButton <= 3)
			{
				// In the game, two to a row
				aWidget->setUpImage(Str(lit(0x458708)), 3);
				aWidget->setFocusImage(Str(lit(0x458708)), 4);
				aWidget->setDownImage(Str(lit(0x458708)), 5);
				if (aButton == 0 || aButton == 2)
					aWidget->mX = 0.05625f;
				else
					aWidget->mX = 0.21875f;
				aWidget->mWidth = 0.16125f;
			}
			else
			{
				aWidget->setUpImage(Str(lit(0x458708)), 0);
				aWidget->setFocusImage(Str(lit(0x458708)), 1);
				aWidget->setDownImage(Str(lit(0x458708)), 2);
				aWidget->mX = 0.11875f;
				aWidget->mWidth = 0.19375f;
			}
			aWidget->mY = aY;
			aWidget->mHeight = 0.075f;
			aWidget->mTextWidth = (float)(aWidget->mWidth - (double)0.03f);
			aWidget->mTextX = 0.015f;
			aWidget->mTextY = 0.016666668f;
			aWidget->mTextHeight = 0.035f;
			if (aButtons != 4 || (aButton & 1) != 0)
				aY = (float)(aY + (double)0.076666668f);
		}
		aButton++;
	} while (aButton < 5);
	mLoaded = 1;
}

// 0x4182d0
void OptionsMenu::init()
{
	int aState = gBonnies->mState;
	Screen::init();
	mDrawsPanel = 0;
	mLoaded = 0;
	mX = 0.2825f;
	mY = (float)((aState == 3 ? 0.0f : -0.041666668f) + (double)0.11399999f);
	mWidth = 0.435f;
	mHeight = 0.74666667f;
	mColor = 0xff153661;
	mDraggable = 1;
	mField4C = 0;
	mField50 = 0;
	load();
}

// 0x418350
void OptionsMenu::shutdown()
{
	removeAll();
	gResources->release(mImage);
}

// 0x418370
void OptionsMenu::update()
{
	load();
	Widget* aFull = findWidget(0x271);
	if (aFull != nullptr)
		static_cast<CheckBox*>(aFull)->mChecked = isFullScreen();
	if (mField4C != 0)
	{
		Screen* aTop = gApp->mDepth == 0 ? nullptr : gApp->mModal[gApp->mDepth];
		if (aTop == this)
		{
			mField4C = 0;
			if (gApp->mLastResult != 1)
				return;
			showInterstitial(0, 0);
			mField50 = 1;
		}
	}
	if (mField50 != 0)
	{
		Screen* aTop = gApp->mDepth == 0 ? nullptr : gApp->mModal[gApp->mDepth];
		if (aTop == this)
		{
			mField50 = 0;
			if (gGameScreen != nullptr && gGame != nullptr)
			{
				gGameScreen->mField58 = 0x11;
				gGame->mFieldC44 = 1;
				gApp->endModal(0, 0);
			}
		}
	}
}

// 0x418450
void OptionsMenu::draw()
{
	if (mLoaded != 0)
	{
		if (isTop() != 0)
			drawVignette(0.6f);
		drawFrame(mImage->mFrames, mX, mY, 1.0f, 0);
		Screen::draw();
	}
}

// 0x4184a0
int OptionsMenu::handleEvent(Event* theEvent)
{
	if (mLoaded == 0)
		return Screen::handleEvent(theEvent);
	int aType = theEvent->mType;
	if (aType != 0xd)
	{
		if (aType == 0xe)
		{
			if (theEvent->mA == 0x271)
			{
				gGraphics->setVideoMode(screenWidth(), screenHeight(), isFullScreen() == 0, 0);
				gOptions->mFullScreen = isFullScreen();
				gOptions->save();
				return 0;
			}
		}
		else if (aType == 0xf)
		{
			if (theEvent->mA == 0x26d)
			{
				gOptions->mSoundVolume = static_cast<Slider*>(findWidget(0x26d))->mValue;
				gSound->play(gApp->mSounds[2], 1.0f, 1, 0, 1.0f);
				gSound->pauseMusicOnly();
				gOptions->save();
				return 0;
			}
			if (theEvent->mA == 0x26f)
			{
				gOptions->mMusicVolume = static_cast<Slider*>(findWidget(0x26f))->mValue;
				gMusic->setVolume(1.0f);
				gOptions->save();
				return 0;
			}
		}
		return Screen::handleEvent(theEvent);
	}
	int aState;
	switch (theEvent->mA)
	{
	case 0x259:
		aState = 4;
		break;
	case 0x25a:
		gCreditsReturn = gBonnies->mState;
		aState = 6;
		break;
	case 0x25b:
		if (gBonnies->mState == 3)
		{
			gHighScoresToGame = 1;
			aState = 7;
		}
		else
		{
			if (gBonnies->mState == 2 && !gOptions->mName.empty() && playerSaveExists(gOptions->mName) != 0)
				gGame->load(gOptions->mName, 1);
			aState = 7;
		}
		break;
	case 0x25c:
	{
		Str aName = gOptions->mName;
		gGame->save(aName);
		aState = 2;
		break;
	}
	case 0x25d:
		gApp->endModal(0, 0);
		return 0;
	// port: the original's 0x25e opened the game's web site; no button has the id, and the port takes out every way out
	// of a game (docs/DEVIATIONS.md, "Loading and the registration")
	default:
		return Screen::handleEvent(theEvent);
	}
	gBonnies->setState(aState, 1, 0);
	gApp->endModal(0, 0);
	return 0;
}

// 0x418710
void showGameMenu()
{
	gOptionsMenu = new OptionsMenu();
	gOptionsMenu->init();
	gApp->runModal(0, gOptionsMenu);
	gOptionsMenu = nullptr;
}


// ---------------------------------------------------------------------------------------------------------------------
// The players (the vtable 0x4587bc): the saved players, a new one, deleting one

// 0x47a1ec, a string the original never sets: its Delete removed the name without its ".gam", so no player was ever
// deleted. The port removes the saved game, by the path playerSaveExists checks (docs/DEVIATIONS.md)

// 0x4187a0: the player selected ("" none)
Str PlayersPanel::selected()
{
	ListBox* aList = static_cast<ListBox*>(findWidget(0x2c3));
	if (-1 < aList->mSelected)
		return aList->mItems[aList->mSelected].mText;
	return Str(lit(0x457426));
}

// 0x4187f0: the scroll buttons when the list has more than five
void PlayersPanel::layout()
{
	ListBox* aList = static_cast<ListBox*>(findWidget(0x2c3));
	if (aList != nullptr)
	{
		if (5 < aList->mCount)
		{
			aList->mX = 0.0625f;
			aList->mWidth = 0.26f;
			findWidget(0x2c1)->mFlags &= ~1u;
			findWidget(0x2c2)->mFlags &= ~1u;
			aList->mRowWidth = aList->mWidth;
			return;
		}
		aList->mX = 0.075f;
		aList->mWidth = 0.285f;
		findWidget(0x2c1)->mFlags |= 1;
		findWidget(0x2c2)->mFlags |= 1;
		aList->mRowWidth = aList->mWidth;
	}
}

// 0x4188a0: the saved players, the current one selected
void PlayersPanel::refill()
{
	ListBox* aList = static_cast<ListBox*>(findWidget(0x2c3));
	aList->clear();
	int aCount;
	Str* aNames;
	listFiles(savedGamesDir(false), Str(lit(0x4575cc)), &aCount, &aNames);
	int i = 0;
	int aSelected = 0;
	if (0 < aCount)
	{
		do
		{
			aList->insert(aNames[i], 0, 999999);
			if (equalsNoCase(aNames[i], gOptions->mName))
				aSelected = i;
			i++;
		} while (i < aCount);
	}
	if (aNames != nullptr)
		delete[] aNames;
	aList->select(aSelected);
	layout();
}

// 0x4189b0
int PlayersPanel::handleEvent(Event* theEvent)
{
	int aType = theEvent->mType;
	if (aType == 0xd)
	{
		switch (theEvent->mA)
		{
		case 0x2bd:
			if (askName(1) != 0)
				refill();
			return 0;
		case 0x2be:
		{
			Str aName = selected();
			if (!aName.empty())
			{
				Str aText = lit(0x457426);
				aText = format(Str(getText(0x50a)), aName, Str(lit(0x457426)), Str(lit(0x457426)));
				int aButton = messageDialog(1, aText, Str(getText(0x50b)), Str(getText(0x50c)), Str(lit(0x457426)),
					Str(lit(0x457426)), 0xd, 0x1b, 0, 0, 0);
				if (aButton != 2)
				{
					Str aPath = lit(0x457426);
					aPath += savedGamesDir(true);
					aPath += aName;
					aPath += exeGlobalString(0x475a54);
					fileRemove(aPath);
					refill();
				}
			}
			return 0;
		}
		case 0x2bf:
		case 0x2c0:
		choose:
		{
			if (selected().empty())
				askName(0);
			else
			{
				gOptions->mName = selected();
				if (theEvent->mA == 0x2c0 && !mOriginal.empty() && playerSaveExists(mOriginal) != 0)
					gOptions->mName = mOriginal;
				gOptions->save();
			}
#ifdef __EMSCRIPTEN__
			if (gSitePlayersChanged != nullptr)
				gSitePlayersChanged();
#endif
			gApp->endModal(0, 0);
			return 0;
		}
		case 0x2c1:
		case 0x2c2:
			static_cast<ListBox*>(findWidget(0x2c3))->scroll(((theEvent->mA != 0x2c1) * 2 - 1) * 4);
			return 0;
		}
	}
	else if (aType == 0x14)
	{
		gOptions->mName = selected();
		if (gMenuScreen != nullptr)
			static_cast<MenuScreen*>(gMenuScreen)->refresh();
#ifdef __EMSCRIPTEN__
		if (gSitePlayersChanged != nullptr)
			gSitePlayersChanged();
#endif
	}
	else if (aType == 0x15 && theEvent->mA == 0x2c3)
		goto choose;
	return Screen::handleEvent(theEvent);
}

// 0x418e10: the players
void choosePlayer()
{
	PlayersPanel* aPanel = new PlayersPanel();
	aPanel->mOriginal = gOptions->mName;
	aPanel->init();
	aPanel->mX = 0.2825f;
	aPanel->mY = 0.114f;
	aPanel->mWidth = 0.435f;
	aPanel->mHeight = 0.74666667f;
	aPanel->mColor = 0xff153661;
	aPanel->mDraggable = 1;
	panelText(aPanel, 0.0f, 0.09f, 0.435f, 0.055f, addRGB(0xffdcb970, 0xf, 0xf, 0xf), Str(getText(0x50d)), 0x108,
		0x479e88);
	ListBox* aList = new ListBox();
	aList->init();
	aPanel->addWidget(aList);
	aList->mX = 0.075f;
	aList->mY = 0.16f;
	aList->mWidth = 0.285f;
	aList->mHeight = 0.30833334f;
	aList->mRows = 5;
	aList->mRowWidth = 0.285f;
	aList->mRowHeight = 0.061666667f;
	aList->mColor = 0xffdcb970;
	aList->mColor2 = 0x7f000000;
	aList->mTextFlags = 0x108;
	aList->setFont(exeFont(0x479e64));
	aList->mId = 0x2c3;
	float aY = 0.50666666f;
	int i = 0;
	do
	{
		Button* aButton = new Button();
		aButton->init();
		aPanel->addWidget(aButton);
		aButton->mColor = 0xffebc87f;
		aButton->mColor2 = 0x7f000000;
		aButton->mFlags = 0x2c0;
		aButton->mTextFlags = 0x100;
		aButton->setFont(exeFont(0x479ff0));
		aButton->setSound(0, Str(lit(0x45872c)));
		aButton->setSound(1, Str(lit(0x45871c)));
		if (i == 0)
		{
			aButton->mText = getText(0x50e);
			aButton->mId = 0x2bd;
		}
		if (i == 1)
		{
			aButton->mText = getText(0x50f);
			aButton->mId = 0x2be;
		}
		if (i == 2)
		{
			aButton->mText = getText(0x510);
			aButton->mId = 0x2bf;
			aButton->mKeys[0] = 0xd;
		}
		if (i == 3)
		{
			aButton->mText = getText(0x511);
			aButton->mId = 0x2c0;
			aButton->mKeys[0] = 0x1b;
		}
		int anOdd = i & 1;
		aButton->mX = anOdd == 0 ? 0.05625f : 0.21625f;
		aButton->mY = aY;
		aButton->mWidth = 0.16125f;
		aButton->mHeight = 0.075f;
		aButton->mTextX = 0.01625f;
		aButton->mTextY = 0.016666668f;
		aButton->mTextWidth = 0.12875f;
		aButton->mTextHeight = 0.035f;
		aButton->setUpImage(Str(lit(0x458708)), 3);
		aButton->setFocusImage(Str(lit(0x458708)), 4);
		aButton->setDownImage(Str(lit(0x458708)), 5);
		if (anOdd != 0)
			aY = (float)(aY + (double)0.081666663f);
		i++;
	} while (i < 4);
	i = 0;
	do
	{
		// The list's scroll buttons
		Button* anArrow = new Button();
		anArrow->init();
		aPanel->addWidget(anArrow);
		anArrow->mFlags = 0x40;
		anArrow->setSound(0, Str(lit(0x45872c)));
		anArrow->setSound(1, Str(lit(0x45871c)));
		anArrow->mX = 0.33125f;
		anArrow->mId = (i != 0) + 0x2c1;
		anArrow->mKeys[0] = (i != 0) + 0x111;
		anArrow->mY = i == 0 ? 0.16666667f : 0.375f;
		anArrow->mWidth = 0.045f;
		anArrow->mHeight = 0.09f;
		anArrow->setUpImage(Str(lit(0x4587d8)), i * 3);
		anArrow->setFocusImage(Str(lit(0x4587d8)), i * 3 + 1);
		anArrow->setDownImage(Str(lit(0x4587d8)), i * 3 + 2);
		i++;
	} while (i < 2);
	aPanel->refill();
	gApp->runModal(1, aPanel);
}


// ---------------------------------------------------------------------------------------------------------------------
// The menu (state 2): the bookstore with its animations, the five buttons and the player's

static int gNameTwoLines;         // 0x47a614: the player's name takes two lines
static uint32_t gBlinkTime;       // 0x47a618: Bonnie's blink
static int gBlinkStep;            // 0x47a61c
static int gBlinkTwice;           // 0x47a620
static uint32_t gWaveStart;       // 0x47a624: the wave
static uint32_t gWaveTime;        // 0x47a628
static int gWaveStep;             // 0x47a62c
static uint32_t gSignTime;        // 0x47a630: the sign swings
static int gSignStep;             // 0x47a634
static uint32_t gLeafStart;       // 0x47a638: the leaf falls
static uint32_t gLeafTime;        // 0x47a63c
static int gLeafStep;             // 0x47a640
[[maybe_unused]] static int gMenuShown;  // 0x47a644 (nothing reads it)
static int gChoosingLevel;        // 0x47aa58: the dialog shows the level chooser
static int gLevelMax = 1;         // 0x473e60: the last level that may be chosen
static int gLevelChosen = 1;      // 0x473e64
static int gStoryEnd;             // 0x47ae74: the story is the one after the last book

// 0x419d40
void MenuScreen::refresh()
{
	Str aTooltip = lit(0x457426);
	if (gOptions->mName.empty())
		aTooltip += getText(0x40f);
	else
		aTooltip = format(Str(getText(0x410)), gOptions->mName, Str(lit(0x457426)), Str(lit(0x457426)));
	gNameTwoLines = 0;
	if (findWidget(0x132) == nullptr)
		return;
	int aLines = 0;
	if (!gOptions->mName.empty())
	{
		Str aName = lit(0x457426);
		aName += toUpperStr(gOptions->mName);
		aName += lit(0x458894);
		aLines = mFont78->countLines(aName, 0, 0.225f, nullptr, 1.0f);
		if (1 < aLines)
			gNameTwoLines = 1;
	}
	// A name of two lines moves the button down
	findWidget(0x132)->mY = 1 < aLines ? 0.25833333f : 0.24166666f;
	findWidget(0x132)->mTooltip = aTooltip;
	if (gOptions->mName.empty())
		findWidget(0x132)->mFlags |= 1;
	else
		findWidget(0x132)->mFlags &= ~1u;
}

// 0x419f70
void MenuScreen::shutdown()
{
	removeAll();
	gBonnies->setStatusFont(exeFont(0x47a6bc));
	gResources->release(mSetup);
	gResources->release(mBlink);
	gResources->release(mLeaf);
	gResources->release(mWave);
	gResources->release(mSign);
	if (mFont70 != nullptr)
		gResources->release(mFont70);
	if (mFont74 != nullptr)
		gResources->release(mFont74);
	if (mFont78 != nullptr)
		gResources->release(mFont78);
	Image** aBook = mBooks;
	int i = 5;
	do
	{
		gResources->release(*aBook);
		aBook++;
		i--;
	} while (i != 0);
	gMenuScreen = nullptr;
}

// 0x41a040
void MenuScreen::draw()
{
	drawFrame(mSetup->mFrames, 0.0f, 0.0f, 1.0f, 1);
	int i = 0;
	Image** aBook = mBooks;
	do
	{
		// The book of the button in focus lights up
		Widget* aButton = findWidget(i + 0x12d);
		if (aButton != nullptr && aButton->hasFocus() != 0)
			drawFrame((*aBook)->mFrames, 0.035f, 0.42666668f, 1.0f, 0);
		i++;
		aBook++;
	} while (i < 5);
	int aStill;
	if (gBonnies->mPaused == 0 && isInactive() == 0 && gBonnies->inModal() == 0)
		aStill = 0;
	else
	{
		// Nothing moves while paused, inactive or under a dialog
		aStill = 1;
		gBlinkTime = ticks();
		gWaveTime = gBlinkTime;
		gLeafStart = gBlinkTime;
	}
	if (gBlinkStep == 0)
	{
		if (6000 < (int)(ticks() - gBlinkTime))
		{
			gBlinkTwice = randomRange(0, 100) < 0x19;
			gBlinkStep = 1;
			gBlinkTime = ticks();
		}
	}
	else
	{
		gBlinkStep++;
		if (gBlinkTwice == 0)
		{
			if (gBlinkStep == 4)
				gBlinkStep = 0;
		}
		else if (gBlinkStep == 0xc)
			gBlinkStep = 0;
	}
	int aFrame = 0;
	if (gBlinkStep != 1 && gBlinkStep != 3)
	{
		if (gBlinkStep == 2)
		{
			aFrame = 2;
			goto blinked;
		}
		if (gBlinkStep != 9 && gBlinkStep != 0xb)
		{
			if (gBlinkStep == 10)
				aFrame = 2;
			goto blinked;
		}
	}
	aFrame = 1;
blinked:
	if (aStill != 0)
		aFrame = 0;
	drawFrame(&mBlink->mFrames[aFrame], 0.49250001f, 0.21333334f, 1.0f, 1);
	if (gWaveStep == 0)
	{
		if (16000 < (int)(ticks() - gWaveStart))
		{
			gWaveStep = 1;
			gWaveStart = ticks();
			gWaveTime = ticks();
		}
	}
	else
	{
		if (0x32 < (int)(ticks() - gWaveTime))
		{
			gWaveStep++;
			gWaveTime = ticks();
		}
		if (gWaveStep == 4)
			gWaveStep = 0;
	}
	aFrame = 1;
	if (gWaveStep == 1)
		aFrame = 2;
	else if (gWaveStep == 3)
		aFrame = 0;
	if (aStill != 0)
		aFrame = 1;
	drawFrame(&mWave->mFrames[aFrame], 0.61124998f, 0.30666667f, 1.0f, 1);
	if (gLeafStep == 0)
	{
		if (8000 < (int)(ticks() - gLeafStart))
		{
			gLeafStep = 1;
			gLeafStart = ticks();
			gLeafTime = ticks();
		}
	}
	else
	{
		if (0x50 < (int)(ticks() - gLeafTime))
		{
			gLeafStep++;
			gLeafTime = ticks();
		}
		if (gLeafStep == 6)
			gLeafStep = 0;
	}
	if (aStill != 0)
		gLeafStep = 0;
	else if (0 < gLeafStep)
	{
		// The leaf's five positions, in pixels of 800x600
		int aPos[10] = {0x41, -3, 0x75, 0xd, 0xd8, 0x16, 0x139, 0x18, 0x174, 0x28};
		drawFrame(&mLeaf->mFrames[gLeafStep - 1], (float)((double)aPos[gLeafStep * 2 - 2] * 0.00125f),
			(float)((double)aPos[gLeafStep * 2 - 1] * 0.0016666667f), 1.0f, 0);
	}
	// The sign swings back and forth over eight steps, slower at the ends
	int aStep = gSignStep % 8;
	if (4 < aStep)
		aStep = 9 - aStep;
	float aSlow = 1.0f;
	if (aStep == 0 || aStep == 4)
		aSlow = 1.4f;
	else if (aStep == 1 || aStep == 3)
		aSlow = 1.2f;
	int anElapsed = (int)(ticks() - gSignTime);
	if ((double)aSlow * 130.0f < (double)anElapsed)
	{
		gSignStep++;
		gSignTime = ticks();
	}
	aStep = gSignStep % 8;
	if (4 < aStep)
		aStep = 8 - aStep;
	if (aStill != 0)
		aStep = 2;
	drawFrame(&mSign->mFrames[aStep], 0.19374999f, 0.0f, 1.0f, 0);
	Str aVersion = getText(0x4d8);
	aVersion += floatToStr(1.01f, 2);
	mFont70->drawText(aVersion, 0.005f, 0.91500002f, 0.25f, 0.083333336f, 0xffafafaf, 0x4f000000, 0x114, 1.0f, 0.0f);
	Str aGreeting = getText(0x4d9);
	float aY;
	if (gOptions->mName.empty())
	{
		aGreeting += lit(0x458894);
		aY = 0.005f;
	}
	else
		aY = -0.005f;
	if (gNameTwoLines != 0)
		aY = (float)(aY - (double)0.021666666f);
	mFont74->drawText(aGreeting, 0.72500002f, (float)(aY + (double)0.055f), 0.25f, 0.16666667f, 0xffffffcf, 0x9f000000,
		0x8c, 1.0f, 0.0f);
	if (!gOptions->mName.empty())
	{
		Str aName = lit(0x457426);
		aName += toUpperStr(gOptions->mName);
		aName += lit(0x458894);
		mFont78->drawText(aName, 0.73750001f, (float)(aY + (double)0.22f), 0.225f, 0.30000001f, 0xffffffcf,
			0x9f000000, 0x8a, 1.0f, 0.0f);
	}
	Screen::draw();
}

// 0x41a690: the difficulty, in a dialog that does not block (update reads its answer)
void MenuScreen::chooseDifficulty()
{
	Str aText = lit(0x457426);
	aText += lit(0x457424);
	aText += getText(0x4db);
	aText += lit(0x457c64);
	if (mStory == 0)
		gApp->showStatus(Str(getText(0x4dd)));
	else
		gApp->showStatus(Str(getText(0x4dc)));
	messageDialog(0, aText, Str(getText(0x4de)), Str(getText(0x4df)), Str(getText(0x4e0)), Str(getText(0x4e1)), 0, 0,
		0, 0x1b, 0);
	mWaiting = 1;
}

// 0x41a810
int MenuScreen::handleEvent(Event* theEvent)
{
	if (theEvent->mType == 0xc)
	{
		// The bell, the cash register and the typewriter
		int anId = theEvent->mA;
		if (anId == 0x140)
		{
			playSound(Str(lit(0x4588a4)), 1.0f);
			return 0;
		}
		if (anId == 0x141)
		{
			playSound(Str(lit(0x4588ac)), 1.0f);
			return 0;
		}
		if (anId == 0x142)
		{
			Str aName = lit(0x457426);
			int aType = randomRange(1, 3);
			aName += lit(0x458294);
			appendInt(aName, aType);
			playSound(aName, 0.85000002f);
			return 0;
		}
		return Screen::handleEvent(theEvent);
	}
	if (theEvent->mType != 0xd)
		return Screen::handleEvent(theEvent);
	switch (theEvent->mA)
	{
	case 0x12d:
	case 0x12e:
	{
		// The two ways to play: the player's game, or a new one at a chosen level
		mStory = theEvent->mA == 0x12d;
		if (!gOptions->mName.empty() && playerSaveExists(gOptions->mName) == 0)
			gOptions->mName = lit(0x457426);
		Str aText = lit(0x457426);
		if (gOptions->mName.empty())
		{
			askName(0);
			if (gOptions->mName.empty())
			{
				gOptions->mName = getText(0x4e6);
				saveGameAs(gOptions->mName);
				gOptions->save();
#ifdef __EMSCRIPTEN__
				if (gSitePlayersChanged != nullptr)
					gSitePlayersChanged();
#endif
				if (gOptions->mName.empty())
					return 0;
			}
		}
		else
			gGame->load(gOptions->mName, mStory);
		if (gGame->mScore != 0 && gGame->mFieldC44 == 0)
		{
			// A game to go on with
			aText = lit(0x457426);
			aText += getText(0x4e2);
			aText += lit(0x457424);
			aText += gGame->currentBook()->title();
			aText += lit(0x457424);
			aText += getText(0x4e3);
			aText += lit(0x4582d0);
			appendInt(aText, gGame->currentBook()->mChapter + 1);
			aText += lit(0x457c64);
			aText += getText(0x4e4);
			if (gApp->mFocus != nullptr)
				gApp->mFocus->unfocus();
			gApp->showStatus(Str(getText(0x4e5)));
			int aButton = messageDialog(1, aText, Str(getText(0x4e9)), Str(getText(0x4ea)), Str(getText(0x4eb)),
				Str(lit(0x457426)), 0xd, 0, 0x1b, 0, 0);
			gApp->showStatus(Str(lit(0x457426)));
			if (aButton == 3)
			{
				refresh();
				return 0;
			}
			if (aButton != 2)
			{
				if (aButton == 1)
				{
					if (gGame->mFieldC5C != mStory)
					{
						if (mStory == 0)
							gGame->mFieldC94 = 0.05f <= gGame->mFieldC94 ? gGame->mFieldC94 : 0.05f;
						else
						{
							int aGoal = gGame->mGoal;
							if (aGoal < 4)
								aGoal = 4;
							gGame->mGoal = aGoal;
						}
					}
					gGame->mFieldC5C = mStory;
					gBonnies->setState(3, 1, 0);
				}
				refresh();
				return 0;
			}
		}
		// A new game: its level, when the player has been further than the first
		mStartLevel = 1;
		if (0 < gGame->mRecord.mBestLevel)
		{
			gChoosingLevel = 1;
			gLevelMax = gGame->mRecord.mBestLevel + 1;
			if (0x31 < gLevelMax)
				gLevelMax = 0x32;
			gLevelChosen = gGame->mRecord.mLastLevel + 1;
			if (gLevelChosen < 1 || 0x32 < gLevelChosen)
				gLevelChosen = 1;
			int aButton = messageDialog(1, Str(lit(0x457426)), Str(getText(0x4e8)), Str(getText(0x4eb)),
				Str(lit(0x457426)), Str(lit(0x457426)), 0xd, 0x1b, 0, 0, 0);
			gChoosingLevel = 0;
			if (aButton == 2)
			{
				refresh();
				return 0;
			}
			mStartLevel = gLevelChosen;
		}
		chooseDifficulty();
		refresh();
		return 0;
	}
	case 0x12f:
		showGameMenu();
		return 0;
	case 0x130:
		gBonnies->setState(4, 1, 0);
		return 0;
	case 0x131:
		gQuit = 1;
		return 0;
	case 0x132:
		if (gOptions->mName.empty())
		{
			askName(1);
			refresh();
		}
		else
		{
			choosePlayer();
			refresh();
		}
		return 0;
	}
	return Screen::handleEvent(theEvent);
}

// 0x41af20
void MenuScreen::init()
{
#ifdef __EMSCRIPTEN__
	// port (web): the site's players at the first menu, before it shows the player (Start.h; src/SiteProfiles.cpp)
	if (gSiteStart != nullptr)
		gSiteStart();
#endif
	gMenuScreen = this;
	Screen::init();
	mStory = 1;
	mStartLevel = 1;
	mField7C = 0;
	mWaiting = 0;
	gApp->mStatusFlags = 0x8c;
	gBonnies->setStatusFont(exeFont(0x47a890));
	gMusic->select(2, 0);
	mSetup = gResources->getImage(Str(lit(0x458914)));
	mBlink = gResources->getImage(Str(lit(0x458908)));
	mLeaf = gResources->getImage(Str(lit(0x4588fc)));
	mWave = gResources->getImage(Str(lit(0x4588f0)));
	mSign = gResources->getImage(Str(lit(0x4588e4)));
	mFont74 = gResources->getFont(exeFont(0x47a890));
	mFont70 = gResources->getFont(exeFont(0x47a86c));
	mFont78 = gResources->getFont(exeFont(0x47a8b4));
	int i = 0;
	Image** aBook = mBooks;
	do
	{
		// The five books are one image with five mattes
		Str aMatte = lit(0x457426);
		aMatte += lit(0x4588d4);
		i++;
		aMatte += intToStr(i);
		aMatte += lit(0x4588c8);
		gMatteName = aMatte;
		gResources->mNoSharing = 1;
		*aBook = gResources->getImage(Str(lit(0x4588bc)));
		gResources->mNoSharing = 0;
		gMatteName = lit(0x457426);
		aBook++;
	} while (i < 5);
	float aY[5] = {0.46166667f, 0.55333334f, 0.64499998f, 0.73666668f, 0.82833332f};
	i = 0;
	do
	{
		Button* aButton = new Button();
		aButton->init();
		addWidget(aButton);
		aButton->mX = 0.1075f;
		aButton->mY = aY[i];
		aButton->mWidth = 0.27250001f;
		aButton->mHeight = 0.095f;
		aButton->mTextX = 0.025f;
		aButton->mTextWidth = 0.2475f;
		aButton->mTextFlags = 0x109;
		aButton->mColor = 0xffffffff;
		aButton->mColor2 = 0x9f000000;
		aButton->mFocusColor = 0xffffff8f;
		aButton->mFocusColor2 = 0xaf000000;
		aButton->setFont(exeFont(0x47a9b0));
		aButton->mFlags = 0x80;
		if (i == 0)
		{
			aButton->mText = getText(0x404);
			aButton->mTooltip = getText(0x405);
			aButton->mId = 0x12d;
		}
		else if (i == 1)
		{
			aButton->mText = getText(0x406);
			aButton->mTooltip = getText(0x407);
			aButton->mId = 0x12e;
		}
		else if (i == 2)
		{
			aButton->mText = getText(0x408);
			aButton->mTooltip = getText(0x409);
			aButton->mId = 0x12f;
		}
		else if (i == 3)
		{
			aButton->mText = getText(0x40a);
			aButton->mTooltip = getText(0x40b);
			aButton->mId = 0x130;
		}
		else if (i == 4)
		{
			aButton->mText = getText(0x40c);
			aButton->mTooltip = getText(0x40d);
			aButton->mId = 0x131;
		}
		i++;
	} while (i < 5);
	// The player's name
	Button* aPlayer = new Button();
	aPlayer->init();
	addWidget(aPlayer);
	aPlayer->mX = 0.73750001f;
	aPlayer->mY = 0.24166666f;
	aPlayer->mWidth = 0.22f;
	aPlayer->mHeight = 0.075f;
	aPlayer->mTextFlags = 0x89;
	aPlayer->mColor = 0xffffffff;
	aPlayer->mColor2 = 0x9f000000;
	aPlayer->mFocusColor = 0xffffff8f;
	aPlayer->mFocusColor2 = 0xaf000000;
	aPlayer->setFont(exeFont(0x47a86c));
	aPlayer->mText = getText(0x40e);
	aPlayer->mId = 0x132;
	aPlayer->mFlags = 0x80;
	// The bell, the cash register and the typewriter
	Area* anArea = new Area();
	anArea->init();
	addWidget(anArea);
	anArea->mX = 0.57499999f;
	anArea->mY = 0.7816667f;
	anArea->mWidth = 0.1525f;
	anArea->mHeight = 0.14f;
	anArea->mId = 0x140;
	anArea = new Area();
	anArea->init();
	addWidget(anArea);
	anArea->mX = 0.8675f;
	anArea->mY = 0.41833332f;
	anArea->mWidth = 0.12875f;
	anArea->mHeight = 0.16833334f;
	anArea->mId = 0x141;
	anArea = new Area();
	anArea->init();
	addWidget(anArea);
	anArea->mX = 0.64249998f;
	anArea->mY = 0.51833332f;
	anArea->mWidth = 0.0875f;
	anArea->mHeight = 0.10666667f;
	anArea->mId = 0x142;
	gBlinkTime = ticks();
	gWaveStart = ticks() - 0x3c8c;
	gSignTime = ticks();
	gLeafTime = ticks();
	gLeafStart = ticks();
	gLeafStep = 0;
	refresh();
}

// 0x41b580
void MenuScreen::update()
{
	if (gMenuShown == 0 && gQuit == 0)
		gMenuShown = 1;
	Screen::update();
	if (mField7C != 0 && gBonnies->inModal() == 0)
	{
		mField7C = 0;
		chooseDifficulty();
	}
	if (mWaiting != 0 && gBonnies->inModal() == 0)
	{
		gApp->showStatus(Str(lit(0x457426)));
		mWaiting = 0;
		if (gBonnies->mLastResult == 4)
		{
			refresh();
			return;
		}
		gApp->showStatus(Str(lit(0x457426)));
		gGame->prepareSave(0);
		gGame->mFieldC5C = mStory;
		gGame->setDifficulty(gBonnies->mLastResult - 1);
		gGame->mLevel = mStartLevel;
		gGame->newBoard();
		if (gGame->mLevel == 1 && gBonnies->mLoadImages[2] != 0)
		{
			gStoryEnd = 0;
			gBonnies->setState(5, 1, 0);
			return;
		}
		gBonnies->setState(3, 1, 0);
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// The game's dialog (the vtable 0x458994): a text and up to four buttons on Bonnie's notepad, with the level chooser
// of a new game

static int gLevelCount = 0x32;    // 0x473e5c: the levels the chooser has

// 0x41b6f0: the chosen level's book and chapter, and the first button only for a level the player has reached
void GameDialog::showLevel()
{
	Str aText = lit(0x457426);
	aText = lit(0x457426);
	Book* aBook = &gBooks[gLevelChosen - 1];
	aText += aBook->title();
	aText += lit(0x457424);
	aText += getText(0x403);
	aText += lit(0x4582d0);
	aText += intToStr(aBook->mChapter + 1);
	Widget* aWidget = findWidget(0x70);
	if (aWidget != nullptr)
		aWidget->mText = aText;
	int aLocked = gLevelMax < gLevelChosen;
	aWidget = findWidget(0x65);
	if (aWidget != nullptr)
	{
		if (aLocked)
			aWidget->mFlags |= 1;
		else
			aWidget->mFlags &= ~1u;
	}
}

// 0x41b840
void GameDialog::shutdown()
{
	removeAll();
	gResources->release(mNotepad);
	if (mBonnie != nullptr)
		gResources->release(mBonnie);
	if (mThumbs != nullptr)
		gResources->release(mThumbs);
	if (mThumbsFrame != nullptr)
		gResources->release(mThumbsFrame);
}

// 0x41b8a0
void GameDialog::draw()
{
	if (isTop() != 0)
		drawVignette(0.6f);
	drawFrame(mNotepad->mFrames, mX, mY, 1.0f, 0);
	if (mBonnie != nullptr)
		drawFrame(mBonnie->mFrames, (float)(mX - (double)0.155f), (float)(mY - (double)0.1f), 1.0f, 0);
	if (gChoosingLevel != 0)
	{
		drawFrame(&mThumbs->mFrames[(gLevelChosen - 1) % 0x32], (float)(mX + (double)0.08625f),
			(float)(mY + (double)0.13166666f), 1.0f, 1);
		drawFrame(mThumbsFrame->mFrames, (float)(mX + (double)0.08625f), (float)(mY + (double)0.13166666f), 1.0f, 0);
		if (gLevelMax < gLevelChosen)
			drawCText(gBonnies->mFonts[5], getText(0x60e), (float)(mX + (double)0.0875f),
				(float)(mY + (double)0.13166666f), 0.22f, 0.22f, 0xffcc0101, 0xafffffff, 0x109, 1.0f, 0.0f);
	}
	Screen::draw();
}

// 0x41ba20
int GameDialog::handleEvent(Event* theEvent)
{
	int aType = theEvent->mType;
	if (aType == 8)
	{
		int aKey = theEvent->mA;
		if (aKey == 0x104 || aKey == 0x106 || aKey == 0x113 || aKey == 0x114)
		{
			// The arrows move the chooser by a level
			int aDown = aKey == 0x114 || aKey == 0x104;
			Slider* aSlider = static_cast<Slider*>(findWidget(0x6f));
			if (aSlider != nullptr)
			{
				gLevelChosen += (aDown == 0) * 2 - 1;
				if (gLevelChosen < 1)
					gLevelChosen = 1;
				if (gLevelCount <= gLevelChosen)
					gLevelChosen = gLevelCount;
				int aRange = 1;
				if (1 <= gLevelCount - 1)
					aRange = gLevelCount - 1;
				double aValue = (double)(gLevelChosen - 1) / (double)aRange + (double)0.001f;
				aSlider->mValue = (float)aValue;
				if (aValue <= 0.0f)
					aSlider->mValue = 0.0f;
				if (!(aSlider->mValue < 1.0f))
					aSlider->mValue = 1.0f;
				showLevel();
			}
			return 0;
		}
	}
	else if (aType == 0xd)
	{
		switch (theEvent->mA)
		{
		case 0x65:
			gApp->endModal(1, 0);
			return 0;
		case 0x66:
			gApp->endModal(2, 0);
			return 0;
		case 0x67:
			gApp->endModal(3, 0);
			return 0;
		case 0x68:
			gApp->endModal(4, 0);
			return 0;
		}
	}
	else if (aType == 0xf && theEvent->mA == 0x6f)
	{
		Slider* aSlider = static_cast<Slider*>(findWidget(0x6f));
		gLevelChosen = ftol((double)(gLevelCount - 1) * aSlider->mValue + (double)0.01f + 1.0f);
		showLevel();
		return 0;
	}
	return Screen::handleEvent(theEvent);
}

// 0x41bc30
void GameDialog::init()
{
	Screen::init();
	mDrawsPanel = 0;
	mNotepad = gResources->getImage(Str(lit(0x45897c)));
	mBonnie = gResources->getImage(Str(lit(0x458970)));
	mThumbs = nullptr;
	mThumbsFrame = nullptr;
	if (gChoosingLevel != 0)
	{
		mThumbs = gResources->getImage(Str(lit(0x458960)));
		mThumbsFrame = gResources->getImage(Str(lit(0x45894c)));
		Slider* aSlider = new Slider();
		aSlider->init();
		addWidget(aSlider);
		double aWider = 0.0f;
		if (0x13 < gLevelCount)
			aWider = 0.125f;
		aSlider->mY = 0.35333332f;
		aSlider->mHeight = 0.055f;
		aSlider->mKnobColor = 0xff4f4f6f;
		aSlider->mX = (float)((double)0.0975f - aWider * 0.5f);
		aSlider->mWidth = (float)(aWider + (double)0.195f);
		aSlider->mSteps = gLevelCount;
		aSlider->mId = 0x6f;
		int aRange = 1;
		if (1 <= gLevelCount - 1)
			aRange = gLevelCount - 1;
		aSlider->mMaxStep = 1;
		aSlider->mValue = (float)((double)(gLevelChosen - 1) / (double)aRange);
		TextField* aText = new TextField();
		aText->init();
		addWidget(aText);
		aText->mX = 0.036249998956918716f;
		aText->mY = 0.39833334f;
		aText->mWidth = 0.32499999f;
		aText->mHeight = 0.055f;
		aText->mColor = 0xff151515;
		aText->mText = lit(0x457426);
		aText->mTextFlags = 0xa;
		aText->mId = 0x70;
		aText->setFont(exeFont(0x47ac18));
		aText = new TextField();
		aText->init();
		addWidget(aText);
		aText->mX = 0.050000001f;
		aText->mY = 0.073333338f;
		aText->mWidth = 0.29749998450279236f;
		aText->mHeight = 0.059999999f;
		aText->mColor = 0xff151515;
		aText->mText = getText(0x402);
		aText->mTextFlags = 0xc;
		aText->mId = 0x71;
		aText->setFont(exeFont(0x47ac18));
		showLevel();
	}
}

// 0x41bf10: theText and up to four buttons (each with its key; a lone button takes Esc and Enter); with bit 0 of
// theFlags2 the registration's reminder (port: taken out with the registration; no caller asks for it). When
// theBlocking, the button pressed (1-4).
int gameDialog(int theBlocking, Str theText, Str theButton1, Str theButton2, Str theButton3, Str theButton4,
	int theKey1, int theKey2, int theKey3, int theKey4, int theFlags2)
{
	int aCount = 0;
	if (!theButton4.empty())
		aCount = 4;
	else if (!theButton3.empty())
		aCount = 3;
	else if (!theButton1.empty())
		aCount = 1;
	GameDialog* aDialog = new GameDialog();
	aDialog->init();
	aDialog->mX = 0.3675f;
	aDialog->mY = 0.16049998998641968f;
	aDialog->mWidth = 0.38999999f;
	aDialog->mHeight = 0.64333332f;
	aDialog->mColor = 0xff153661;
	aDialog->mDraggable = 1;
	TextField* aLabel = new TextField();
	aLabel->init();
	aDialog->addWidget(aLabel);
	aLabel->mX = 0.038750001f;
	aLabel->mY = 0.098333336f;
	aLabel->mWidth = 0.3125f;
	aLabel->mHeight = 0.64333332f;
	aLabel->mColor = 0xff151515;
	aLabel->mTextFlags = 2;
	aLabel->mText = theText;
	aLabel->setFont(exeFont(0x47ac18));
	float aY = (float)((double)0.64333332f - ((double)aCount * 0.081666663f + (double)0.071666665f));
	if (aCount <= 1)
		aY = (float)(aY - (double)0.023333333f);
	int i = 0;
	do
	{
		Str aText = lit(0x457426);
		int aKey = 0;
		switch (i)
		{
		case 0:
			aText = theButton1;
			aKey = theKey1;
			break;
		case 1:
			aText = theButton2;
			aKey = theKey2;
			break;
		case 2:
			aText = theButton3;
			aKey = theKey3;
			break;
		case 3:
			aText = theButton4;
			aKey = theKey4;
			break;
		}
		if (aText.empty())
			break;
		// Two buttons share a row; a lone one is the dialog's Esc and Enter
		int aPair = !theButton1.empty() && !theButton2.empty() && theButton3.empty();
		int aLone = !theButton1.empty() && theButton2.empty() && theButton3.empty();
		double aWidth = aPair != 0 ? 0.0975f : 0.15f;
		float aX = (float)(((double)0.38999999f - aWidth) * 0.5f);
		if (aPair != 0)
		{
			if (i == 0)
				aX = (float)((((double)0.38999999f - (aWidth + aWidth)) - (double)0.11f) * 0.5f);
			else
				aX = (float)((((double)0.38999999f - (aWidth + aWidth)) - (double)0.11f) * 0.5f + aWidth +
					(double)0.05625f);
		}
		Button* aButton = new Button();
		aButton->init();
		aDialog->addWidget(aButton);
		aButton->mId = i + 0x65;
		aButton->mText = aText;
		aButton->mX = aX;
		aButton->mY = aY;
		aButton->mWidth = 0.175f;
		aButton->mHeight = 0.13333334f;
		aButton->mHitX = 0.029999999f;
		aButton->mHitY = 0.036666665f;
		aButton->mHitWidth = 0.115f;
		aButton->mHitHeight = 0.065f;
		aButton->mColor = 0xff151515;
		aButton->mColor2 = 0xff96540b;
		aButton->mTextFlags = 0;
		aButton->mFlags = 0xc0;
		aButton->setFont(exeFont(0x47ac3c));
		aButton->setUpImage(Str(lit(0x458860)), i * 3);
		aButton->setFocusImage(Str(lit(0x458860)), i * 3 + 1);
		aButton->setDownImage(Str(lit(0x458860)), i * 3 + 2);
		aButton->setSound(0, Str(lit(0x458518)));
		aButton->setSound(1, Str(lit(0x458504)));
		aButton->mKeys[0] = aKey;
		if (aLone != 0 && aKey == 0)
		{
			aButton->mKeys[0] = 0x1b;
			aButton->mKeys[1] = 0xd;
		}
		if (aPair == 0)
			aY = (float)(aY + (double)0.081666663f);
		i++;
	} while (i < 4);
	return gApp->runModal(theBlocking, aDialog);
}

// 0x41c410: gameDialog with the first button's text "OK" (text 3) when it has none
int messageDialog(int theBlocking, Str theText, Str theButton1, Str theButton2, Str theButton3, Str theButton4,
	int theKey1, int theKey2, int theKey3, int theKey4, int theFlags2)
{
	if (theButton1.empty())
		theButton1 = getText(3);
	return gameDialog(theBlocking, theText, theButton1, theButton2, theButton3, theButton4, theKey1, theKey2, theKey3,
		theKey4, theFlags2);
}


// ---------------------------------------------------------------------------------------------------------------------
// The story (state 5): Bonnie's three pages before the first level, or the one after the last

// The page's image: "Story_", the story and the page's letter
static Str storyImageName(int theStory, int thePage)
{
	Str aName = lit(0x458a0c);
	aName += intToStr(theStory);
	aName += (char)(thePage + 'A');
	return aName;
}

// 0x41c550
void StoryScreen::init()
{
	gState5Screen = this;
	Screen::init();
	mPage = 0;
	mStory = (gStoryEnd != 0) + 1;
	mPages = (gStoryEnd == 0) * 2 + 1;
	mFont = gResources->getFont(exeFont(0x47b204));
	mImage = gResources->getImage(storyImageName(mStory, mPage));
	mImage2 = nullptr;
	if (mStory == 1)
	{
		// The button that skips to the next page
		Button* aButton = new Button();
		aButton->init();
		addWidget(aButton);
		aButton->mX = 0.882499993f;
		aButton->mY = 0.899999976f;
		aButton->mWidth = 0.100000001f;
		aButton->mHeight = 0.0799999982f;
		aButton->mFlags = 0x40;
		aButton->setUpImage(Str(lit(0x4589f8)), 0);
		aButton->setFocusImage(Str(lit(0x4589e4)), 0);
		aButton->setDownImage(Str(lit(0x4589d0)), 0);
		aButton->mId = 0x1f5;
		aButton->setSound(0, Str(lit(0x458518)));
		aButton->setSound(1, Str(lit(0x458504)));
	}
}

// 0x41c750
void StoryScreen::shutdown()
{
	removeAll();
	gResources->release(mImage);
	if (mImage2 != nullptr)
		gResources->release(mImage2);
	if (mFont != nullptr)
		gResources->release(mFont);
	gState5Screen = nullptr;
}

// 0x41c7a0
void StoryScreen::draw()
{
	drawFrame(mImage->mFrames, 0.0f, 0.0f, 1.0f, 1);
	if (mImage2 != nullptr)
		drawFrame(mImage2->mFrames, 0.0f, 0.0f, 1.0f, 0);
	if (mPage == 0 && mStory == 1)
	{
		drawCText(mFont, getText(0x3f9), 0.0474999994f, 0.0533333346f, 0.417499989f, 0.0533333346f, 0xfffdfafa, 0, 1,
			1.0f, 0.0f);
		drawCText(mFont, getText(0x3fa), 0.519999981f, 0.0533333346f, 0.432500005f, 0.0733333305f, 0xfffdfafa, 0, 1,
			1.0f, 0.0f);
		drawCText(mFont, getText(0x3fb), 0.208749995f, 0.86833334f, 0.566250026f, 0.0566666685f, 0xfffdfafa, 0, 1,
			1.0f, 0.0f);
	}
	if (mPage == 1 && mStory == 1)
	{
		drawCText(mFont, getText(0x3fc), 0.0387500003f, 0.0500000007f, 0.33375001f, 0.0816666633f, 0xfffdfafa, 0, 1,
			1.0f, 0.0f);
		drawCText(mFont, getText(0x3fd), 0.467500001f, 0.0733333305f, 0.405000001f, 0.0816666633f, 0xfffdfafa, 0, 1,
			1.0f, 0.0f);
		drawCText(mFont, getText(0x3fe), 0.100000001f, 0.754999995f, 0.270000011f, 0.12166667f, 0xff010101, 0, 1,
			1.0f, 0.0f);
	}
	if (mPage == 2 && mStory == 1)
		drawCText(mFont, getText(0x3ff), 0.0887499973f, 0.0833333358f, 0.483749986f, 0.0616666675f, 0xfffdfafa, 0, 1,
			1.0f, 0.0f);
	if (mPage == 0 && mStory == 2)
	{
		drawCText(mFont, getText(0x400), 0.0175000001f, 0.0199999996f, 0.328750014f, 0.0949999988f, 0xfffdfafa, 0, 1,
			1.0f, 0.0f);
		drawCText(mFont, getText(0x401), 0.542500019f, 0.0599999987f, 0.38499999f, 0.0966666639f, 0xfffdfafa, 0, 1,
			1.0f, 0.0f);
	}
	Screen::draw();
}

// 0x41ca40: the next page, or the game (the high scores after the last book)
void StoryScreen::turn()
{
	mPage++;
	if (mPage < mPages)
	{
		Str aName = storyImageName(mStory, mPage);
		gResources->release(mImage);
		mImage = gResources->getImage(aName);
		return;
	}
	if (mStory == 2)
	{
		gBonnies->setState(7, 1, 0);
		return;
	}
	gBonnies->setState(3, 1, 0);
}

// 0x41cb60: a click, Enter, Esc or the button turns the page
int StoryScreen::handleEvent(Event* theEvent)
{
	if (Screen::handleEvent(theEvent) != 0)
	{
		switch (theEvent->mType)
		{
		case 3:
		case 5:
		case 7:
			break;
		case 8:
			if (theEvent->mA != 0xd && theEvent->mA != 0x1b)
				return Screen::handleEvent(theEvent);
			break;
		case 0xd:
			if (theEvent->mA != 0x1f5)
				return Screen::handleEvent(theEvent);
			break;
		default:
			return Screen::handleEvent(theEvent);
		}
		turn();
	}
	return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// A book's end (the vtable 0x458a4c): its summary fades in; a click goes on to the next book, or ends the game after
// the last

// 0x41cbe0
void BookEndScreen::init()
{
	Screen::init();
	mBackground = gResources->getImage(Str(lit(0x458a38)));
	gMatteName = lit(0x458a24);
	mSummary = gResources->getImage(gGame->currentBook()->summaryName());
	gMatteName = lit(0x457426);
	mStart = 0;
	mEnd = 0;
	mShown = 0;
	mAdvanced = 0;
	mFont = gResources->getFont(exeFont(0x47b41c));
}

// 0x41cc80
void BookEndScreen::shutdown()
{
	removeAll();
	gResources->release(mBackground);
	gResources->release(mSummary);
	if (mFont != nullptr)
		gResources->release(mFont);
}

// 0x41ccc0
void BookEndScreen::draw()
{
	if (mStart == 0)
		mStart = ticks();
	double aLevel = (double)(int)(ticks() - mStart) * 0.0033333334f;
	float anAlpha = (float)aLevel;
	if (!(aLevel < 1.0f))
		mShown = 1;
	if (mShown == 0)
	{
		if (0 < (int)mEnd)
			mEnd = ticks();
	}
	else if (0 < (int)mEnd)
	{
		// The fade out after the click
		double anOut = 1.0f - (double)(int)(ticks() - mEnd) * 0.0033333334f;
		if (anOut <= anAlpha)
			anAlpha = (float)anOut;
	}
	if (anAlpha <= 1.0f)
	{
		if (anAlpha < 0.01f)
			return;
	}
	else
		anAlpha = 1.0f;
	drawFrame(mBackground->mFrames, 0.0f, 0.0f, anAlpha, 1);
	drawFrame(mSummary->mFrames, 0.036249999f, 0.0833333358f, anAlpha, 0);
	Str aText = lit(0x457426);
	aText += getText(0x585);
	int aBook = gBooks[gGame->mLevel - 2].mBook;
	if (aBook < 0)
		aBook = 0;
	else if (0xb < aBook)
		aBook = 0xb;
	aText += getText(exeI32(0x473e68 + aBook * 4));
	uint32_t aColor = (uint32_t)ftol(anAlpha * 255.0f) << 24 | 0x5f5f5f;
	mFont->drawText(aText, 0.60374999f, 0.236666664f, 0.306250006f, 0.36500001f, aColor, 0, 0x12, 1.0f, 0.0f);
	drawCText(mFont, getText(0x586), 0.758750021f, 0.605000019f, 0.152500004f, 0.143333331f, aColor, 0, 0x22, 1.0f,
		0.0f);
	Screen::draw();
}

// 0x41cef0
void BookEndScreen::update()
{
	if (mShown != 0 && mAdvanced == 0)
	{
		mAdvanced = 1;
		if (gGame != nullptr)
			gGame->nextLevel();
	}
	if (0 < (int)mEnd && 300 < (int)(ticks() - mEnd))
	{
		gApp->endModal(0, 0);
		return;
	}
	Screen::update();
}

// 0x41cf50
int BookEndScreen::handleEvent(Event* theEvent)
{
	switch (theEvent->mType)
	{
	case 3:
	case 5:
	case 7:
		if (mShown == 0)
			return 0;
		break;
	case 8:
		if (mShown == 0)
			return 0;
		if (theEvent->mA == 0x13c)
			return Screen::handleEvent(theEvent);
		break;
	default:
		return Screen::handleEvent(theEvent);
	}
	if (gGame->mLevel != 0x33)
	{
		gGameScreen->reset();
		gGameScreen->mField58 = 9;
		if (mEnd == 0)
			mEnd = ticks();
		return 0;
	}
	// The last book: the game is over, into the high scores, and the last story
	gGame->mFieldC44 = 1;
	gGame->save(gOptions->mName);
	gHighScoreTable = gGame->mFieldC5C == 0;
	gHighScoreRank = gHighScores.add(gGame->mFieldC5C == 0, gOptions->mName, gGame->mStrCD0, gGame->mScore);
	gStoryEnd = 1;
	gBonnies->setState(5, 1, 0);
	gApp->endModal(0, 0);
	return Screen::handleEvent(theEvent);
}

// 0x41d110
void showBookEnd()
{
	BookEndScreen* aScreen = new BookEndScreen();
	aScreen->init();
	gApp->runModal(1, aScreen);
}

// ---------------------------------------------------------------------------------------------------------------------
// The overlay (the vtable 0x4574bc), above every screen: "Downloading..." while the menu's images load, and the pause

// 0x41d150
void OverlayScreen::init()
{
	gOverlayScreen = this;
	Screen::init();
	mPriority = 99;
}

// 0x41d170
void OverlayScreen::shutdown()
{
	removeAll();
	gOverlayScreen = nullptr;
}

// 0x41d180
void OverlayScreen::draw()
{
	Screen::draw();
	if (gBonnies != nullptr && gBonnies->mLoadImages[7] != 0)
	{
		drawVignette(0.6f);
		Str aText = lit(0x458a80);
		int aTime = (int)ticks() % 3000;
		int aDots = ftol((double)aTime * 0.000333222270f * 6.0f);
		if (0 < aDots)
		{
			do
			{
				aText += lit(0x458a78);
				aDots--;
			} while (aDots != 0);
		}
		gBonnies->mFonts[1]->drawText(aText, 0.25833333f, 0.0f, 0.6875f, 1.0f, 0xffa8f2ff, 0xbf000000, 0x50, 1.0f, 0.0f);
		return;
	}
	if (gBonnies->mFonts[1] != nullptr && gGameScreen != nullptr &&
		(gBonnies->mPaused != 0 || (isInactive() != 0 && gGame != nullptr && gGame->mFieldC5C == 0)))
	{
		// Paused (or the timed game's window inactive)
		drawVignette(0.6f);
		drawCText(gBonnies->mFonts[1], getText(0x3f8), 0.125f, 0.0f, 0.75f, 1.0f, 0xffa8f2ff, 0xbf000000, 0x40, 1.0f,
			0.0f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The interstitial (the vtable 0x458ad4): the portal's page between levels, after a win and after a loss

static int gInterstitialKind;     // 0x473e98: 1 and 2 by turns between levels, 3 a win, 4 a loss
[[maybe_unused]] static Screen* gInterstitial;  // 0x47baa8 (nothing reads it)

// 0x41d330
void InterstitialScreen::init()
{
	gInterstitial = this;
	Screen::init();
	mField4C = 0;
	Str aName = lit(0x457426);
	if (gInterstitialKind == 3)
		aName += lit(0x458ac8);
	else if (gInterstitialKind == 4)
		aName += lit(0x458abc);
	else
	{
		aName += lit(0x458ab0);
		aName += intToStr(gInterstitialKind);
	}
	mImage = gResources->getImage(aName);
	int i = 0;
	do
	{
		float aX = 0.0020833334f;
		if (gInterstitialKind != 2)
			aX = 0.46458334f;
		if (i == 1)
			aX = (float)(aX + (double)0.2625f);
		Button* aButton = new Button();
		aButton->init();
		addWidget(aButton);
		aButton->mId = i + 0x4b1;
		aButton->mX = aX;
		aButton->mY = 0.858333349f;
		aButton->mWidth = 0.24166666f;
		aButton->mHeight = 0.13333334f;
		aButton->mFlags = 0x40;
		int aFrame = i * 3;
		if (i == 1 && (gInterstitialKind == 3 || gInterstitialKind == 4))
			aFrame = 6;
		aButton->setUpImage(Str(lit(0x458a9c)), aFrame);
		aButton->setFocusImage(Str(lit(0x458a9c)), aFrame + 1);
		aButton->setDownImage(Str(lit(0x458a9c)), aFrame + 2);
		aButton->setSound(0, Str(lit(0x458518)));
		aButton->setSound(1, Str(lit(0x458504)));
		i++;
	} while (i < 2);
}

// 0x41d560
void InterstitialScreen::draw()
{
	drawFrame(mImage->mFrames, 0.0f, 0.0f, 1.0f, 1);
	Screen::draw();
}

// 0x41d5a0: Enter, Esc or the second button closes it (the first does nothing)
int InterstitialScreen::handleEvent(Event* theEvent)
{
	if (Screen::handleEvent(theEvent) != 0)
	{
		if (theEvent->mType == 8)
		{
			if (theEvent->mA != 0xd && theEvent->mA != 0x1b)
				return Screen::handleEvent(theEvent);
		}
		else if (theEvent->mType != 0xd || theEvent->mA != 0x4b2)
			return Screen::handleEvent(theEvent);
		gApp->endModal(0, 0);
	}
	return 0;
}

// 0x41d600: the interstitial, when the portal has one (theWin: after a won game; theLoss: after a lost one)
void showInterstitial(int theWin, int theLoss)
{
	if (gBonnies->mLoadImages[6] == 0)
		return;
	if (theWin == 0)
	{
		if (theLoss == 0)
		{
			int aLast = gOptions->mInterstitial;
			gOptions->mInterstitial = (aLast + 1) % 2;
			gOptions->save();
			gInterstitialKind = aLast + 1;
		}
		else
			gInterstitialKind = 4;
	}
	else
		gInterstitialKind = 3;
	InterstitialScreen* aScreen = new InterstitialScreen();
	aScreen->init();
	aScreen->mX = 0.0f;
	aScreen->mY = 0.0f;
	aScreen->mWidth = 1.0f;
	aScreen->mHeight = 1.0f;
	gApp->runModal(0, aScreen);
}

} // namespace bonnies
