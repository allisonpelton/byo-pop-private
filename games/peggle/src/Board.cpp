// Board.cpp: see Board.h. Only the making, the emptying and the putting of a level on the board are translated so far.
//
// The original keeps its tweakable numbers in ModVal calls (each returns the number it was compiled with); they are named
// constants here with the place of the call in the original's source (file, line, id).

#include "Board.h"

#include "AIMgr.h"
#include "Ball.h"
#include "CharacterMgr.h"
#include "CollisionMgr.h"
#include "Common.h"
#include "DebugMgr.h"
#include "EffectMgr.h"
#include "FloatingTextMgr.h"
#include "GameMode.h"
#include "ImageMgr.h"
#include "InterfaceMgr.h"
#include "LevelFile.h"
#include "LogicMgr.h"
#include "MusicInterface.h"
#include "NotTranslated.h"
#include "PlayerInfo.h"
#include "Poly.h"
#include "Res.h"
#include "SDL.h"
#include "SoundMgr.h"
#include "ThunderButton.h"
#include "ThunderCommon.h"
#include "ThunderballApp.h"
#include "TrophyMgr.h"
#include "WidgetManager.h"

using namespace Sexy;

static const int ORIGIN_X = 75;					// where the level's own places start in the window
static const int ORIGIN_Y = 40;
static const int BOARD_WIDTH = 650;
static const int BOARD_HEIGHT = 560;
static const int GUN_SCREEN_X = 400;			// the cannon is put at these less the origin (Board.cpp 2 and 3, 147: ModValInt 400 and 70)
static const int GUN_SCREEN_Y = 70;
static const int MENU_BUTTON_X = 12;			// Board.cpp 4 and 5, 153 (ModValInt 0xc and 0x21c)
static const int MENU_BUTTON_Y = 540;
static const int REPLAY_BUTTON_X = 736;			// Board.cpp 6 and 7, 158 (ModValInt 0x2e0 and 0x21c)
static const int REPLAY_BUTTON_Y = 540;
static const int DEFAULT_SLOW_MO_RATE = 250;
static const int SLOW_MO_DEFAULT_TIMER = 20;
static const int TWO_PLAYER_BALLS = 6;			// Board.cpp 22, 1658 (ModValInt)
static const int ONE_PLAYER_BALLS = 10;
static const int DEFAULT_NUM_GOAL_PEGS = 25;
static const int BIG_BACKGROUND_WIDTH = 646;	// a background picture of this size is the usual one (0x286 by 0x21f)
static const int BIG_BACKGROUND_HEIGHT = 543;
static const int BIG_BACKGROUND_X = 325;		// where it is put (ModValInt 0x145 and 0x11c, Board.cpp)
static const int BIG_BACKGROUND_Y = 284;
static const int BACKGROUND_SCREEN_X = 400;		// the other pictures are centred on this point, less the origin
static const int BACKGROUND_SCREEN_Y = 300;
static const int BACKGROUND_MIN_SIZE = 400;		// CheckForBackground: a picture bigger than this both ways is the background
static const char* const LEVEL_FOLDER = "levels/";
static const char* const BASE_LEVEL_FILE = "levels\\_base.dat";

// the original's global at 0x649d7c: set while a level is being put on the board (a brick makes its pictures when it is not)
bool gLoadingLevel = false;

// 0x4238a0
Board::Board(ThunderballApp* theApp)
{
	mApp = theApp;
	mGameMode = GAME_MODE_NONE;
	mUnknownC0 = true;
	mSlowMo = false;
	mUnknownC2 = false;
	mUnknownC3 = false;
	mFileNameUnderscore = false;
	mUnknownC5 = false;
	mUnknownC6 = false;
	mUnknownC7 = false;
	mUnknownC8 = 0;
	mUnknownCC = 0;
	mSlowMoRate = DEFAULT_SLOW_MO_RATE;
	mUnknownD4 = 0;
	mUnknownD8 = SLOW_MO_DEFAULT_TIMER;
	mLives = 1;
	mUnknownE0 = 0;
	mUnknownE4 = 0;
	mUnknownE8 = false;
	mUnknownE9 = false;
	mUnknownEA = false;
	mResetCount = 0;
	mUnknownF0 = 0;
	mUnknownF4 = 0;
	mUnknownF8 = 0;
	mUnknown104 = false;
	mUnknown11C = false;
	mUnknown11D = true;
	mUnknown11E = false;
	mUnknown11F = false;
	mUnknown120 = true;
	mUnknown121 = false;
	mUnknown122 = false;
	mEasterEgg = false;
	mEasterEggKind = 0; // (not set by the original's constructor)
	mChallenge = nullptr;
	mOriginX = ORIGIN_X;
	mOriginY = ORIGIN_Y;
	mUnknown1B0 = BOARD_WIDTH;
	mUnknown1B4 = BOARD_HEIGHT;
	mUnknown1C4 = false;
	mBackgroundImage = nullptr;

	mAIMgr.reset(new AIMgr(this));
	mCharacterMgr = &theApp->mCharacterMgr;
	mCollisionMgr.reset(new CollisionMgr());
	mDebugMgr.reset(new DebugMgr(this));
	mEffectMgr.reset(new EffectMgr());
	mFloatingTextMgr.reset(new FloatingTextMgr());
	mInterfaceMgr.reset(new InterfaceMgr(this));
	mLogicMgr.reset(new LogicMgr(this));
	mSoundMgr.reset(new SoundMgr());
	// (the original makes the level editor, the replay dialog, the end of level dialog and the slot machine here)

	mGun = SharedPtr<Gun>(new Gun(this));
	mGun->SetPos((float)(GUN_SCREEN_X - mOriginX), (float)(GUN_SCREEN_Y - mOriginY));

	mMenuButton = MakeEmbeddedButton(0, this, IMAGE_DLG_MENU, false);
	mMenuButton->Move(MENU_BUTTON_X, MENU_BUTTON_Y);
	mMenuButton->mDrawEnabled = false;
	AddWidget(mMenuButton);
	mInstantReplayButton = MakeEmbeddedButton(1, this, IMAGE_DLG_INSTANTREPLAY, false);
	mInstantReplayButton->Move(REPLAY_BUTTON_X, REPLAY_BUTTON_Y);
	mInstantReplayButton->mDrawEnabled = false;
	mInstantReplayButton->mHasTransparencies = true;
	mInstantReplayButton->mStopBlinkOnHover = true;
	AddWidget(mInstantReplayButton);

	static const char* const aWords[] = {"kathy", "steve", "shanon", "zoom", "scott", "seattle", "piano", "pitcher", "tennis"};
	for (int i = 0; i < 9; i++)
	{
		TypingCheck aCheck;
		aCheck.mWord = aWords[i];
		aCheck.mTyped = 0;
		mTypingChecks.push_back(aCheck);
	}
	Clear(true);
}

// 0x4277f0
Board::~Board()
{
}

// 0x40d7d0
void Board::Clear(bool theNewGame)
{
	mApp->mMusicInterface->StopMusic(0);
	mAIMgr->Clear();
	mApp->mImageMgr.Clear();
	mCollisionMgr->Clear();
	mDebugMgr->Clear();
	mEffectMgr->Clear();
	mFloatingTextMgr->Clear();
	mInterfaceMgr->Clear();
	mSoundMgr->Clear();
	mGun->Reset();
	mLogicMgr->Clear(theNewGame, false);
	mCharacterMgr->Clear(false);
	mCharacterMgr->CalcLookPos(this, true);
	mObjects.clear();
	mBalls.clear();
	mLevelName.erase();
	gPhysObjCount = 0;
	mUnknownE4 = 0;
	mUnknownE8 = false;
	mUnknownE9 = false;
	mFileNameUnderscore = false;
	SetSlowMo(false, DEFAULT_SLOW_MO_RATE);
	DeleteReplays(false);
}

// 0x42da00
void Board::Reset()
{
	NOT_TRANSLATED("Board::Reset");
}

// 0x4026f0
void Board::SetSlowMo(bool theSlowMo, int theRate)
{
	mSlowMo = theSlowMo;
	mUnknownC3 = false;
	mUnknownC8 = 0;
	mUnknownC2 = false;
	mUnknownCC = 0;
	mSlowMoRate = theRate;
	mUnknownD4 = 0;
	if (!theSlowMo)
	{
		if (mLogicMgr->mUnknownFA)
		{
			mLogicMgr->KillSlowMoSound();
			mLogicMgr->mUnknownFA = false;
		}
		mUnknown11C = false;
	}
}

// 0x40cf70: lets go of what the replay recorded. The board does not record any replay data yet.
void Board::DeleteReplays(bool theKeep)
{
}

// 0x422a90 (0x421070)
void Board::AddObj(const SharedPtr<PhysObj>& theObj)
{
	mObjects.push_back(theObj);
	mCollisionMgr->AddObj(theObj.get());
	if (theObj->mKind == KIND_SHOT_BALL && !static_cast<Ball*>(theObj.get())->mIsHat)
	{
		// the ball takes the first number (0 or 1) that none of the balls in play has
		bool aUsed[2] = {false, false};
		for (std::list<SharedPtr<PhysObj> >::iterator anIter = mBalls.begin(); anIter != mBalls.end(); ++anIter)
		{
			int anId = static_cast<Ball*>(anIter->get())->mBallId;
			if (anId >= 0 && anId < 2)
				aUsed[anId] = true;
		}
		for (int i = 0; i < 2; i++)
		{
			if (!aUsed[i])
			{
				static_cast<Ball*>(theObj.get())->mBallId = i;
				break;
			}
		}
		mBalls.push_back(theObj);
	}
	theObj->AddedToGame();
}

// 0x40ffe0 and friends: where the mouse is, in the board's own places (the window's less the board's origin)
int Board::GetBoardMouseX() const
{
	return mApp->mWidgetManager->mLastMouseX - mOriginX;
}

int Board::GetBoardMouseY() const
{
	return mApp->mWidgetManager->mLastMouseY - mOriginY;
}

// 0x41bb80: the window's title names the version and, in a test level (the game's mode is none), the level
void Board::SetWindowTitle(const std::string& theLevelName)
{
	mApp->mTitle = "Peggle " + mApp->mProductVersion; // (the original's title says "Peggle Deluxe", which the site does not)
	if (!theLevelName.empty())
		mApp->mTitle += " - " + theLevelName;
	if (mApp->mHWnd != nullptr)
		SDL_SetWindowTitle(mApp->mHWnd, mApp->mTitle.c_str());
}

// 0x42aae0
bool Board::LoadLevel(const std::string& theName)
{
	gLoadingLevel = false;
	bool aLoaded = LoadLevel2(theName);
	gLoadingLevel = true;
	return aLoaded;
}

// 0x429d10: puts a level on the board: the pieces of its file, the base pieces every level shares (unless the file's name starts
// with an underscore), and the picture behind them, which is the level's file name's in the levels folder
bool Board::LoadLevel2(const std::string& thePath)
{
	mLevelName = thePath;
	Clear(false);
	mUnknownE9 = true;
	mLevelName = "";
	std::string aFileName = GetFileName(thePath, false);

	PieceList aLevelPieces;
	if (!ReadLevelFile(thePath, aLevelPieces))
	{
		Clear(true);
		return false;
	}

	PieceList aBasePieces;
	if (!aFileName.empty() && aFileName[0] == '_')
		mFileNameUnderscore = true;
	else
	{
		// the base file may be missing (the original passes a flag that makes a missing file a success)
		ReadLevelFile(BASE_LEVEL_FILE, aBasePieces);
	}

	for (PieceList::iterator anIter = aBasePieces.begin(); anIter != aBasePieces.end(); ++anIter)
	{
		(*anIter)->mBaseObject = true;
		AddObj(*anIter);
	}

	std::string anImageName = std::string(LEVEL_FOLDER) + GetFileName(thePath, true);
	bool aWasCached = false;
	Image* aBackgroundImage = mApp->mImageMgr.GetImage(anImageName, &aWasCached, true);
	if (mEasterEgg && aBackgroundImage == nullptr)
		NOT_TRANSLATED("Board::LoadLevel2: the picture made for an easter egg's level");
	if (aBackgroundImage != nullptr)
	{
		SharedPtr<Poly> aBackground(new Poly());
		aBackground->mImageName = anImageName;
		aBackground->LoadImage();
		aBackground->mCollidable = false;
		aBackground->mBackground = true;
		aBackground->mImageCached = aWasCached;
		if (aBackground->mImage != nullptr)
		{
			mBackgroundImage = aBackground->mImage;
			aBackground->MakeRect(0, 0, (float)aBackground->mImage->mWidth, (float)aBackground->mImage->mHeight);
			aBackground->mNormalDir = -1;
			float aX;
			float aY;
			if (aBackground->mImage->mWidth == BIG_BACKGROUND_WIDTH && aBackground->mImage->mHeight == BIG_BACKGROUND_HEIGHT)
			{
				mUnknownE9 = false;
				aX = (float)BIG_BACKGROUND_X;
				aY = (float)BIG_BACKGROUND_Y;
			}
			else
			{
				aX = (float)(BACKGROUND_SCREEN_X - mOriginX);
				aY = (float)(BACKGROUND_SCREEN_Y - mOriginY);
			}
			aBackground->SetPos(aX, aY);
		}
		aBackground->mBaseObject = true;
		AddObj(SharedPtr<PhysObj>(aBackground.get()));
	}

	for (PieceList::iterator anIter = aLevelPieces.begin(); anIter != aLevelPieces.end(); ++anIter)
	{
		(*anIter)->mBaseObject = false;
		AddObj(*anIter);
	}

	mLevelName = thePath;
	SetWindowTitle(mApp->mGameMode == GAME_MODE_NONE ? GetFileName(mLevelName, true) : std::string());
	SetupLevel();
	mApp->ClearUpdateBacklog(false);
	CheckForBackground();
	return true;
}

// 0x409920: the picture behind the level, when it is a big one, is the one the board draws (and the board's is then drawn shaded)
void Board::CheckForBackground()
{
	mUnknownE8 = false;
	mInterfaceMgr->SyncStageNum();
	for (std::list<SharedPtr<PhysObj> >::iterator anIter = mObjects.begin(); anIter != mObjects.end(); ++anIter)
	{
		PhysObj* anObject = anIter->get();
		if (!anObject->mBackground || anObject->mImage == nullptr)
			continue;
		if (anObject->mImage->mWidth > BACKGROUND_MIN_SIZE && anObject->mImage->mHeight > BACKGROUND_MIN_SIZE)
		{
			mUnknownE8 = mUnknownC0;
			anObject->mVisible = mUnknownC0;
			if (!anObject->mImageCached && anObject->mImageName.find("levels/") != std::string::npos)
				NOT_TRANSLATED_DRAWING("Board::MakeShadow for the level's picture");
		}
	}
}

// 0x41ea50: the rules' start of the level: the challenge's balls, lives and scores, then LogicMgr::InitLevel
void Board::SetupLevel()
{
	mUnknown1C4 = false;
	LogicMgr* aLogic = mLogicMgr.get();
	if (mChallenge != nullptr)
	{
		if (mResetCount == 1)
		{
			aLogic->mNumGoalPegs = mChallenge->mNumOrange > 0 ? mChallenge->mNumOrange : DEFAULT_NUM_GOAL_PEGS;
			mLives = mChallenge->mLives > 0 ? mChallenge->mLives : 1;
		}
		if (mChallenge->mBalls == 0)
		{
			int aNumBalls = aLogic->mTwoPlayer ? TWO_PLAYER_BALLS : ONE_PLAYER_BALLS;
			aLogic->mNumBalls[0] = aNumBalls;
			aLogic->mNumBalls[1] = aNumBalls;
		}
		else if (mResetCount == 1)
			aLogic->mNumBalls[0] = mChallenge->mBalls;
		else if (aLogic->mNumBalls[0] < 1)
			aLogic->mNumBalls[0] = 1;
		if (mResetCount == 1 || mChallenge->mScoreReset)
		{
			aLogic->mScore[1] = 0;
			aLogic->mScore[0] = 0;
		}
		aLogic->mUnknownFB = mChallenge->mClearLevel;
	}
	aLogic->InitLevel(mChallenge == nullptr, true, false);
	mInterfaceMgr->InitLevel();
	mDebugMgr->InitLevel();
	if (mGun->mAngle != mGun->mTargetAngle)
		mGun->SetAngle(mGun->mTargetAngle, true);
	mCharacterMgr->Resync();
	mInterfaceMgr->SyncStageNum();
	if (mApp->mCurrentPlayer != nullptr && !aLogic->mTwoPlayer)
	{
		// a level the player has cleared before
		if (mApp->mCurrentPlayer->mLevelsCleared.count(GetFileName(mLevelName, true)) != 0)
			mUnknownC7 = true;
	}
	NOT_TRANSLATED_DRAWING("Board::MakeThumbnail"); // (draws the level once and keeps it in levels/cached_thumbnails/)
}
