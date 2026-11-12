// The Board's widget shell: construction, the update (0x426f40), the end of a level and of a game, and the manager hooks (the
// input handlers are in BoardInput.cpp, the saved games in BoardSave.cpp, the drawing in BoardDraw.cpp, the rules in
// BoardLogic.cpp and BoardLogic2.cpp). See docs/GAME_NOTES.md for the map.

#include "Board.h"
#include "RocketApp.h"
#include "MusicInterface.h"
#include "Compat2004.h"
#include "Dialogs.h"
#include "BuyRockets.h"
#include "TalkingHead.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include "MTRand.h"
#include "WidgetManager.h"

#include <SDL.h>
#include <cstring>
#include <cstdio>

using namespace Sexy;

// port: the original's float to int conversion (__ftol): truncation
static int Ftol(double theValue)
{
	return (int) theValue;
}

// 0x41ae80: a new board (its pieces are made through Piece(int), 0x41ad90)
Board::Board(RocketApp* theApp, bool theContinuing)
{
	mApp = theApp;
	mCols = 6;
	mRows = 9;
	mRand = new MTRand();
	SeedMTRand2004(*mRand, (unsigned long) GlobalRand());
	mGen = new PieceGenerator(theApp, this, mRand);
	mEffects = new EffectManager(mApp);
	mTip = new HoverTip(mApp);      // 0x413aa0
	// every plain field of the original starts as zero; the lists stay as they are
	memset(&mState, 0, (char*) &mObjects - (char*) &mState);
	for (int aRow = 0; aRow < 9; aRow++)
		for (int aColumn = 0; aColumn < 6; aColumn++)
			mGrid[aRow][aColumn] = NULL;
	for (int i = 0; i < 6; i++)
		mTop[i] = NULL;
	mSpare = NULL;
	for (int i = 0; i < 9; i++)
	{
		mLeft[i] = NULL;
		mRight[i] = NULL;
	}
	mPressedPiece = NULL;
	mNightLength = 100.0;
	mLevel = 0;
	mLaunchesNeeded = 10;
	mLaunchesNeededCopy = 10;
	mLeftCount = 3;
	mRightCount = 3;
	// 0x41b04e: the arcade mode's first fall delay, or the timed modes' two updates between dropped tiles
	// (the original also stores -1 in the two fields that are not zero)
	mDealStart = -1;
	mBurningPath = -1;
	// the game-over kind starts at 3 (none of the kinds 0 to 2; nothing reads it before GameOver sets it, but the save file holds it)
	mGameOverKind = 3;
	if (mApp->mMode == 0)
		mFallPeriod = mGen->Rows20();
	else if (mApp->mMode == 1 || mApp->mMode == 2)
		mTimedFallPeriod = 2;
	mState = 0;
	ResetState(theContinuing);
	SetupBoard(true, theContinuing);
	mMoved = mApp->mNoHints;
	// the head says its first lines a moment into the first new game of the run (the countdown is in Update)
	if (!mApp->mNoHints && !theContinuing && !mApp->mFirstGameHint)
	{
		mApp->mFirstGameHint = true;
		mApp->mHeadCountdown = 0x96;
	}
	// 0x41ae80: the game's song fades in
	mApp->mMusicInterface->FadeIn(SONG_GAME, -1, 0.666, false);
}

// 0x41b2a0
Board::~Board()
{
	ClearBoard(false);
	for (std::list<Points*>::iterator anItr = mTexts.begin(); anItr != mTexts.end(); ++anItr)
		(*anItr)->mWidgetManager->RemoveWidget(*anItr);
	mTexts.clear();
	delete mGen;
	delete mEffects;
	delete mTip;
	delete mRand;
}

// 0x426f40: the board's update. (The original also blinks the colour of the widget at +0x51c, which the shipped game never
// creates, so there is nothing to blink.)
void Board::Update()
{
	if (0 < mPingCooldown)
		mPingCooldown--;
	if (0 < mApp->mHeadCountdown)
	{
		mApp->mHeadCountdown--;
		if (mApp->mHeadCountdown == 0)
		{
			mApp->mHead->Say(SOUND_VOICE_CLICK_ON_THE_TILE_TO_ROTATE_IT_01);
			mApp->mHead->SetMood(3);
		}
	}
	if (mState == 3)
		return;
	if (mPause < 1)
		mPauseFade = 0;
	else if (mPauseFade != 1.0)
	{
		mPauseFade = mPauseFade + 0.002;
		if (1.0 < mPauseFade)
			mPauseFade = 1.0;
		MarkDirty();
	}
	if (1 <= mPause)
		return;
	Widget::Update();
	mPlayedMs = (int) SDL_GetTicks() - mStartTicks;
	if (!mStarted)
		mStarted = true;
	mTip->Update(mPointerX, mPointerY);
	MarkDirty();
	if (mBurning)
	{
		mBurnCountdown--;
		if (mBurnCountdown < 1)
		{
			if (mBurnCountdown == 0)
				FinishBurn();
			else if (mFlameCount == 0)
				Launch();
		}
	}
	if (mApp->mMode == 2 && mState == 0)
	{
		if (mMatchesLeft == 0)
		{
			mEndCountdown--;
			if (mEndCountdown < 1 && CountBurning() == 0)
				GameOver(2);
		}
		else
			mEndCountdown = 0xfa;
	}
	SmoothFraction();
	Timers();
	UpdatePieces();
	if (mRefilling && AllTiles())
	{
		mRefilling = false;
		mJustRefilled = true;
	}
	Connections();
	mJustRefilled = false;
	for (int i = 0; i < 10; i++)
	{
		if (mLanternPuff[i] == 1)
		{
			MeterFilledEffect(i);
			if (i == 7)
			{
				Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO22_BOLD], GT(0x004e66f0),
					CellX(3) + mX, mY + 0x1e, 400, 0);
				aPoints->mStatic = true;
				mTexts.push_back(aPoints);
				mWidgetManager->AddWidget(aPoints);
				mHintsQuiet = true;
			}
			mLanternPuff[i] = 2;
		}
	}
	if (mScoreShown != mScore)
	{
		MarkDirty();
		int aDiff = mScore - mScoreShown;
		if (mScoreShown < mScore)
		{
			mScoreShown = mScoreShown + Ftol((double) aDiff * 0.02 + 2.0);
			if (mScore < mScoreShown)
				mScoreShown = mScore;
		}
		else if (mScore < mScoreShown)
		{
			mScoreShown = mScoreShown + Ftol((double) aDiff * 0.025 - 2.0);
			if (mScoreShown < mScore)
				mScoreShown = mScore;
		}
	}
	for (std::list<Points*>::iterator anItr = mTexts.begin(); anItr != mTexts.end(); )
	{
		Points* aPoints = *anItr;
		aPoints->MarkDirty();
		aPoints->mPhase = (aPoints->mPhase + 7) % 0xff;
		if (mUpdateCnt % 3 == 0 && 0 < aPoints->mTop && !aPoints->mStatic)
		{
			aPoints->Move(aPoints->mLeft, aPoints->mTop - 1);   // (nothing lowers mTop: the text jumps once and stays)
		}
		aPoints->mLife--;
		if (aPoints->mLife < 1)
		{
			anItr = mTexts.erase(anItr);
			mWidgetManager->RemoveWidget(aPoints);
			mApp->SafeDeleteWidget(aPoints);
		}
		else
			++anItr;
	}
	if (mState != 2)
	{
		if (mState == 4)
		{
			if (mLevelEndCountdown < 300)
				mApp->mHead->Follow(mEffects);
			if (mLevelEndCountdown < 1)
				EndLevelB();
			else
			{
				mLevelEndCountdown--;
				if (mLevelEndCountdown == 0x168)
					mApp->mHead->Say(SOUND_VOICE_LEVEL_UP_02);
				else if (mLevelEndCountdown == 300)
					mApp->mHead->FlyIn(mEffects);
			}
		}
		else if (mState == 5)
		{
			mState = 6;
			ShowLevelUp(LevelUpDialog::KIND_LEVEL);
			mSecondsPlayed = mSecondsPlayed + ((int) SDL_GetTicks() - mStartTicks) / 1000;
			ClearRowsAbove(1000);
			mApp->mHead->FlyOut(mEffects);
			mWidgetManager->BringToFront(mApp->mHead);
		}
	}
	if (mState == 0 && 0x32 < mUpdateCnt - mSettledAt && !mSettleFlag)
		SettleAll(true);
	mEffects->Update();
	if (mSpecialsDelay != 0 && --mSpecialsDelay == 0)
		AttachLaunchBonuses();
	if (mState == 0 && mApp->mMode == 0 && mUpdateCnt % 100 == 0 && 0 < mCols)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			if (mGrid[0][aColumn] != NULL && mGrid[0][aColumn]->mType == 1)
			{
				mApp->PlaySample(5);
				break;
			}
		}
	}
	if (mState == 1 && mGameOverKind == 1)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
			for (int aColumn = 0; aColumn < mCols; aColumn++)
				Bounce(mGrid[aRow][aColumn]);
		for (int aColumn = 0; aColumn < mCols; aColumn++)
			Bounce(mTop[aColumn]);
		Bounce(mSpare);
	}
}

// ---------------------------------------------------------------------------------------------------------
// the originals' small helpers (their addresses are their names; what is left out is said at each)

// 0x4282a0: a burning tile is moved: the burn is cancelled and its tiles are plain again
void Board::CancelBurn(int thePath)
{
	mChainBurn = false;
	for (int aRow = mRows - 1; aRow >= 0; aRow--)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mType == 4 && aPiece->mPath == thePath)
			{
				aPiece->mType = 1;
				aPiece->mPath = 0;
				aPiece->mPathFrom = -1;
			}
		}
	}
	mBurning = false;
	mBurningPath = -1;
}

// 0x428090: takes the first special of kind 9 (type 6) out of the moving pieces
void Board::RemoveSetOffSpecial()
{
	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece != NULL && aPiece->mType == 6 && aPiece->mKind == 9)
		{
			EraseMover(aPiece);
			RemovePiece(aPiece, false);
			return;
		}
	}
}

// 0x421de0: the game is over (theKind 0 the night ran out, 1 the top line is full, 2 strategy mode ran out of moves).
// The talking head says "game over" (kinds 1 and 2) or "out of time" (the night). The buy-rockets screen goes up in its
// game-over form: the coins turn into points.
void Board::GameOver(int theKind)
{
	mGameOverKind = theKind;
	mState = 1;
	mGameOverDelay = 300;
	mApp->mMusicInterface->FadeOut(SONG_GAME, true, 0.02);
	mApp->mMusicInterface->FadeIn(SONG_GAMEOVER, -1, 0.02, false);
	mApp->PlaySample(0x24);
	if (theKind == 1 || theKind == 2)
	{
		mApp->mHead->Say(SOUND_VOICE_GAME_OVER_02);
		mApp->mHead->SetMood(13);
	}
	else
	{
		mApp->mHead->Say(SOUND_VOICE_OUT_OF_TIME_02);
		mApp->mHead->SetMood(6);
	}
	BuyRocketsWidget* aWidget = new BuyRocketsWidget(mApp, true);
	aWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(aWidget);
}

// 0x429050, 0x429060, 0x429080: the count of the flames that were lit for one launch; when the launch is done
// MakeMarker puts that many on the board
void Board::AddLaunchFlame()
{
	mLaunchFlames++;
}

void Board::StartLaunchFlames(int theRow)
{
	mLaunchFlames = 0;
	mLaunchRow = theRow;
}

void Board::FinishLaunchFlames()
{
	MakeMarker(mLaunchRow, mLaunchFlames + 1);
	mLaunchRow = -1;
}

// 0x41f280: the effect of the meter i filling up
void Board::MeterFilledEffect(int theIndex)
{
	Effect* anEffect = mEffects->Add(0xe, true);
	anEffect->mLife = 1;
	anEffect->mDelay = 0;
	anEffect->mY = 44.0;
	anEffect->mX = (double) (theIndex * 0x16 + 0x108);
	mApp->PlaySample(4);
}

// 0x428620: the buy-rockets screen goes up over the board; the board keeps it (mBuyRocketsScreen) and takes it away when the next level
// begins (NextLevel) or when it goes itself
void Board::ShowBuyRockets()
{
	BuyRocketsWidget* aWidget = new BuyRocketsWidget(mApp, false);
	mBuyRocketsScreen = aWidget;
	mBuyRocketsScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBuyRocketsScreen);
}

// 0x428bc0: the next level. The rank is counted again (0x428cf0 keeps it in mRank: the level-up screen says a new rank was
// made when it differs), the board is made ready and plays.
void Board::NextLevel()
{
	int aRank;
	int aPointsToNext;
	RankName(&aRank, &aPointsToNext);       // (0x428ca0 once more before the level number goes up: its results only go to a log call, 0x42d4d0, that is a bare return in the shipped game)
	mLevel++;
	RankName(&mRank, &aPointsToNext);         // 0x428cf0
	mStartTicks = (int) SDL_GetTicks();
	if (mBuyRocketsScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mBuyRocketsScreen);
		mApp->SafeDeleteWidget(mBuyRocketsScreen);
		mBuyRocketsScreen = NULL;
	}
	ResetState(false);
	SetupBoard(false, false);
	mState = 0;
	mSpecialsDelay = 200;
}

// 0x424260: the widgets that the board owns go with it (the original deletes the buy-rockets screen at once, not later)
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	if (mBuyRocketsScreen != NULL)
	{
		theWidgetManager->RemoveWidget(mBuyRocketsScreen);
		delete mBuyRocketsScreen;
		mBuyRocketsScreen = NULL;
	}
	for (std::list<Points*>::iterator anItr = mTexts.begin(); anItr != mTexts.end(); ++anItr)
	{
		theWidgetManager->RemoveWidget(*anItr);
		mApp->SafeDeleteWidget(*anItr);
	}
	mTexts.clear();
}

// 0x424340: when the board is moved in the order of the widgets its buy-rockets screen follows it, in front
void Board::OrderInManagerChanged()
{
	if (mBuyRocketsScreen != NULL)
		mWidgetManager->PutInfront(mBuyRocketsScreen, this);
}

// 0x4286b0: after a level's screen. The first levels bring the hint of an item (level 1 the coins,
// level 3 the gems, level 5 the bombs and, in the strategy mode, level 7 the clocks) and then the head climbs out of its
// hole to watch (0x416520 with 1); otherwise, or when the hint is turned off, the buy-rockets screen follows. The next
// level's launches are counted. (The original ends with 0x42f770, the clock of the trial game's minutes mode: the
// registered game has none.)
void Board::AfterLevelScreen()
{
	int aHint = -1;
	if (mLevel == 0)
		aHint = HintDialog::HINT_COIN;
	else if (mLevel == 2)
		aHint = HintDialog::HINT_GEM;
	else if (mLevel == 4)
		aHint = HintDialog::HINT_BOMB;
	else if (mLevel == 6)
	{
		if (mApp->mMode == 1)
			aHint = HintDialog::HINT_CLOCK;
	}
	if (aHint == -1 || !mApp->ShowModeHint(aHint, false))
		ShowBuyRockets();
	else
		mApp->mHead->GoToRest(true);
	mNightLeftShown = 0.0;
	mFrozenTime = 0;
	mLaunchesNeeded = mGen->RocketsForLevel(mLevel + 2);
	mLaunchesNeededCopy = mLaunchesNeeded;
	mLaunchesDone = 0;
}

// 0x428ca0: the rank of the score (0 to 26) by the table at 0x4c7848, its name, and how many points the next rank is away
// (0 at the top rank)
static const int sRankScores[27] =
{
	0, 6000, 12000, 18000, 24000, 30000, 36000, 42000, 48000, 54000, 60000, 69000, 78000, 90000, 120000, 180000, 240000,
	300000, 360000, 420000, 480000, 600000, 750000, 900000, 1200000, 1800000, 3000000
};

// the names, by rank: the table at 0x4e62e8
static const unsigned int sRankNames[27] =
{
	0x004e6560, 0x004e6550, 0x004e653c, 0x004e6528, 0x004e6514, 0x004e6508, 0x004e6500, 0x004e64f4, 0x004e64dc, 0x004e64d0,
	0x004e64c8, 0x004e64bc, 0x004e64b0, 0x004e64a4, 0x004e6494, 0x004e6480, 0x004e6468, 0x004e6450, 0x004e6438, 0x004e6420,
	0x004e6408, 0x004e63f8, 0x004e63e0, 0x004e63cc, 0x004e63bc, 0x004e63b0, 0x004e639c
};

SexyString Board::RankName(int* theRank, int* thePointsToNext)
{
	int anIndex = 0x1a;
	int aNext = 0;
	int aRank;
	do
	{
		int aScore = sRankScores[anIndex];
		aRank = anIndex;
		if (aScore <= mScore)
			break;
		anIndex--;
		aNext = aScore;
		aRank = 0;
	} while (-1 < anIndex);
	*theRank = aRank;
	int aToNext = aNext - mScore;
	*thePointsToNext = (aToNext < 0) ? 0 : aToNext;
	return GT(sRankNames[aRank]);
}

// the level-up dialog as the board opens it (0x426860 at the end of a game with kind 5, 0x426f40 after a level with kind 4):
// white, green and green for the dialog's colours 0, 1 and 3, the whole board's size
void Board::ShowLevelUp(int theKind)
{
	LevelUpDialog* aDialog = new LevelUpDialog(mApp, theKind, mRand);
	aDialog->SetColor(0, Color(0xff, 0xff, 0xff));
	aDialog->SetColor(1, Color(0, 0xff, 0));
	aDialog->SetColor(3, Color(0, 0xff, 0));
	aDialog->mContentInsets = Insets(0x24, 0x24, 0x24, 0x24);
	aDialog->Resize(0, 0, mWidth, mHeight);
	mApp->AddDialog(DIALOG_LEVEL_UP, aDialog);
}

// 0x428d10: the specials that the launches have earned are put on the board
// (the first matches special that is attached opens the matches hint)
void Board::AttachLaunchBonuses()
{
	if (mState != 0)
		return;
	if (mApp->mMode == 2)
	{
		while (0 < mGen->mLaunchBonus)
		{
			bool aWasAttached = mMatchAttached;
			AttachSpecial10();
			if (mMatchAttached != aWasAttached)
				mApp->ShowModeHint(HintDialog::HINT_MATCHES, false);
			mGen->mLaunchBonus--;
		}
	}
	else
	{
		int aLeft = mGen->mLaunchBonus;
		while (0 < aLeft)
		{
			int aIndex;
			if (aLeft < 10)
			{
				if (4 < aLeft)
				{
					aLeft -= 5;
					aIndex = 2;
				}
				else if (1 < aLeft)
				{
					aLeft -= 2;
					aIndex = 1;
				}
				else
				{
					aLeft -= 1;
					aIndex = 0;
				}
			}
			else
			{
				aLeft -= 10;
				aIndex = 3;
			}
			mGen->mLaunchBonus = aLeft;
			AttachBomb(aIndex);
			aLeft = mGen->mLaunchBonus;
		}
	}
}

// 0x428de0: the rows above the height theValue (pixels, from the top of the board) flash and go
void Board::ClearRowsAbove(int theValue)
{
	int aRows = (theValue - 30) / 42;
	if (aRows < 0)
		aRows = 0;
	if (mRows < aRows)
		aRows = mRows;
	for (int aRow = 0; aRow < aRows; aRow++)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL)
			{
				AddFlash(aPiece, 2);
				RemovePiece(aPiece, false);
			}
		}
	}
}
