#include "LevelObj.h"
#include "Board.h"

#include "BananaBonus.h"
#include "BugScore.h"
#include "Compat2004.h"
#include "FireWorks.h"

#include "Cursor.h"
#include "GameApp.h"
#include "HighScore.h"
#include "DDImage.h"
#include "MenuButton.h"
#include "ObjString.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"
#include "TextFade.h"
#include "Transition.h"

#include <cstdio>
#include <cstdlib>

using namespace Sexy;

// The original's tables, as the level's code lays them out
static const int WORLD_COUNT_FOR_MUSIC = 4;        // the four level songs
static const int LAST_WORLD_WITH_OWN_SONG = 11;    // the world that has MUSIC_LEVEL_5
static const int LAST_WORLD = 13;                  // the map level past which the game is won
static const int VICTORY_PROGRESS = 0x1b8;         // the levels finished past which the victory scene is made
static const int INTRO_DELAY = 0x4b0;              // StartLevel's delay with the intro
static const int PLAIN_DELAY = 300;                // ... without it
static const int SHOT_COOLDOWN_START = 10;       // frames the first shot waits on the first level
static const int REPLAY_DELAY = 10;                // ... for the next level after the stats
static const int FIREBALL_FIRST_FRAME = 0x19a;     // frames after the level starts that the fire balls are first shown
static const int FIREBALL_SECOND = 61;             // the countdown values that launch the three fire balls
static const int FIREBALL_THIRD = 31;
static const int FIREBALL_LAST = 1;
static const int FIREWORK_PERIOD = 30;             // frames between the fireworks of the victory
static const int FIREWORK_SPREAD_PLAIN = 0x709;    // the range of a firework's x, to which 100 is taken
static const int FIREWORK_SPREAD_WIN = 0x6a5;
static const int FIREWORK_X_MARGIN = 100;
static const float FIREWORK_Y = 900.0f;
static const int WIN_FIREWORKS = 5;                // the fireworks the win launches at once
static const int WIN_FIREWORK_DELAY = 250;
static const int STATS_WAIT_FRAMES = 200;          // how long the level shows its win before the stats come
static const int STATS_PAUSE_FRAMES = 20;
static const int GAME_OVER_FRAMES = 300;
static const int MESSAGE_DELAY_AFTER_START = 100;  // the level's messages start this long after StartLevel's delay
static const float LEVEL_X = -844.0f;              // where the level's origin is in the design space
static const float LEVEL_Y_BASE = 16.0f;
static const float LEVEL_Z = 10.0f;
static const float LAYER_OFFSET = 15.5f;           // the layers sit half a cell in
static const float ROPE_Z = -0.01f;
static const float FIREWORK_Z = -0.5f;
static const float YOU_WIN_X = 620.0f;
static const float YOU_WIN_Y = 496.0f;
static const int PANEL_BANANAS_BONUS = 20;
static const int TEXT_Z_FIRST = 0xda;              // the depth of the first of the texts that take turns; each next is one lower
static const int TEXT_PERSISTENT_Z = 0xd1;
static const int BUG_SCORE_Z_FIRST = 0xd2;
static const int OBJECT_LAYER_Z = 0xd;
static const int FILL_LAYER_Z = 1;
static const int START_MARK_INSIDE = -1;           // FillObj::SetMark's value for a cell inside the field

// 0x4733f0: theCount whole numbers in a row, then mixed twice by swapping each with one picked at random by the C
// library's rand (CrtRand, the emulation of MSVC's), not the game's own generator
static void ShuffleNumbers(int* theNumbers, int theCount)
{
	for (int i = 0; i < theCount; i++)
		theNumbers[i] = i;
	for (int aPass = 0; aPass < 2; aPass++) {
		for (int i = 0; i < theCount; i++) {
			int aPick = CrtRand() % theCount;
			int aTemp = theNumbers[i];
			theNumbers[i] = theNumbers[aPick];
			theNumbers[aPick] = aTemp;
		}
	}
}

LevelObj::LevelObj()
{
	Player::EndMetaGame();
	SetParent(nullptr, nullptr);
	Cursor::Hide();
	mMorphImage = sApp->GetImage("images/morph01", true);
	mMorphStatsImage = sApp->GetImage("images/morph_stats", true);
	mOrderPos = 0;
	// (the original stores the name "LevelObj" at +0x1f0, which this port's ObjBase has no room for)
	mCenterX = (float)LEVEL_CENTER_X;
	mCenterY = (float)LEVEL_CENTER_Y;
	mType = 2;
	mUnused8fec = 0;
	mCellsCut = 0;
	mFrame = 0;
	mCellsReady = false;
	mPreviewShown = false;
	mPreviewSide2Valid = false;
	mPreviewSide3Valid = false;
	mQuitConfirmed = false;
	mGroundCleared = false;
	mTextRing = 0;
	mBugScoreRing = 0;
	mFireworksOn = false;
	mMenu = nullptr;
	mGameOverTimer = -1;
	mVictory = nullptr;
	mVictoryShown = false;
	mState = LEVEL_STATE_INTRO;
	mTimer = 0;
	mStats = nullptr;
	mFireBallCountdown = 0;
	mFireworksDelay = 0;
	mSpawnTimer = 0;
	mSpawnTimer2 = 0;
	mMenuButtonLive = false;
	SetState(STATE_X, LEVEL_X);
	SetState(STATE_Y, LEVEL_Y_BASE - mCenterY);
	SetStateInt(STATE_Z, (int)LEVEL_Z);
	ComputeDrawn();
	mScreenCenterX = mScreenX + mCenterX;
	mScreenCenterY = mScreenY + mCenterY;
	BuildMessages();
	BuildHud();
	BuildMenuButton();
	BuildYouWin();
	ShowBananas(false);
	InitSpawnTable();
	mBananaBonus = new BananaBonus();

	mFillLayer = new ObjBase();
	mFillLayer->SetStateInt(STATE_Z, FILL_LAYER_Z);
	mFillLayer->SetParent(this, nullptr);
	mFillLayer->SetState(STATE_X, LAYER_OFFSET);
	mFillLayer->SetState(STATE_Y, LAYER_OFFSET);
	mFillLayer->Activate(mFillLayer, false);
	mObjectLayer = new ObjBase();
	mObjectLayer->SetStateInt(STATE_Z, OBJECT_LAYER_Z);
	mObjectLayer->SetParent(this, nullptr);
	mObjectLayer->Activate(mObjectLayer, false);
	Activate(this, false);
	SetLevel(this);
	BuildMorphTable();
	ShuffleNumbers(mColumnOrder, FILL_GRID_COLUMNS);
	ShuffleNumbers(mRowOrder, FILL_GRID_ROWS);
	ShuffleNumbers(mCellOrder, LEVEL_CELL_LIST_MAX);
	mOrderPos = 0;
	mRope = new RopeObj();
	mRope->SetState(STATE_Z, ROPE_Z);
	mRope->SetParent(mObjectLayer, nullptr);
	InitCells();
	Player* aPlayer = Player::sCurrent;
	aPlayer->mMapLevel = aPlayer->MapPosition(aPlayer->mSavedProgress, nullptr, nullptr, nullptr, nullptr,
		&aPlayer->mWorldProgress);
	mBonusCoin = new BonusCoin();
	mBonusCoin->SetParent(this, nullptr);
	for (int i = 0; i < LEVEL_FIREWORK_COUNT; i++) {
		mFireWorks[i] = new FireWorks();
		mFireWorks[i]->SetParent(this, nullptr);
		mFireWorks[i]->SetState(STATE_Z, FIREWORK_Z);
	}
	if (!HasIntro()) {
		mIntro = nullptr;
		StartLevel(PLAIN_DELAY);
		Player::AppearAtMouse();
		PlayLevelMusic();
	} else {
		mIntro = new Intro();
		StartLevel(INTRO_DELAY);
		sApp->ChangeMusic(MUSIC_INTRO);
	}
	if (Player::sCurrent->mSavedProgress <= VICTORY_PROGRESS)
		mVictory = nullptr;
	else
		mVictory = new Victory();
}

// 0x439d00 (0x439ce0 is the deleting wrapper)
LevelObj::~LevelObj()
{
	if (mIntro != nullptr) {
		mIntro->Finish();
		mIntro = nullptr;
	}
	if (mVictory != nullptr) {
		mVictory->Delete();
		mVictory = nullptr;
	}
	WalkThru::Dismiss();
	mBananaPanel->Delete();
	mBananaPanel = nullptr;
	mMenuButtonLive = false;
	if (mMenuButton != nullptr) {
		mMenuButton->Delete();
		mMenuButton = nullptr;
	}
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			FillGridCell& aPlace = gFillGrid[aColumn][aRow];
			aPlace.mCell->SetParent(nullptr, nullptr);
			aPlace.mPipe->SetParent(nullptr, nullptr);
			Deactivate(aPlace.mCell);
			Deactivate(aPlace.mPipe);
		}
	}
	SetLevel(nullptr);
	// port (leak): the original never frees the two pictures it loads in its constructor
	delete mMorphImage;
	delete mMorphStatsImage;
}

// 0x4396b0: the first level of a player who has not seen the tutorial
bool LevelObj::HasIntro()
{
	Player* aPlayer = Player::sCurrent;
	return aPlayer != nullptr && aPlayer->mTutorialDone == 0 && aPlayer->mSavedProgress == 0;
}

// 0x439c42..0x439c67: the world's song; the 12th world has the fifth, the others one of the four
void LevelObj::PlayLevelMusic()
{
	int aWorld = Player::sCurrent->mMapLevel;
	int aSong;
	if (aWorld == LAST_WORLD_WITH_OWN_SONG)
		aSong = MUSIC_LEVEL_5;
	else
		aSong = aWorld % WORLD_COUNT_FOR_MUSIC;
	sApp->ChangeMusic(aSong);
}

// 0x43c4a0: the bug kinds' table as a new level starts it: how many are wanted, the most, the first level, where
// they go and how likely they are
void LevelObj::InitSpawnTable()
{
	for (int i = 0; i < LEVEL_SPAWN_KINDS; i++)
		mSpawn[i] = SpawnEntry();
	static const SpawnEntry aStart[LEVEL_SPAWN_KINDS] = {
		{0, 5, 0, SPAWN_IN_OPEN, 1},       // Skeeter
		{0, 8, 0, SPAWN_ON_WALLS, 1},      // Beetle
		{0, 2, 8, SPAWN_IN_OPEN, 2},       // DragonFly
		{0, 2, 0x10, SPAWN_ON_EDGES, 1},   // Rolly
		{0, 1, 0x39, SPAWN_ON_EDGES, 5},   // Ant
		{0, 5, 0x18, SPAWN_ON_WALLS, 1},   // Roach
		{0, 1, 0x14, SPAWN_ON_EDGES, 1},   // Caterpillar
		{0, 2, 0x24, SPAWN_IN_OPEN, 1},    // Fly
		{0, 1, 0x48, SPAWN_ON_EDGES, 4},   // Centipede
		{0, 2, 100, SPAWN_IN_OPEN, 1},     // Moth
		{0, 2, 0x78, SPAWN_IN_OPEN, 2},    // Bee
	};
	for (int i = 0; i < LEVEL_SPAWN_KINDS; i++)
		mSpawn[i] = aStart[i];
}

// 0x43e530: the texts that take turns (the fifth stays up until it is told to go) and the numbers that float up where a
// bug was caught
void LevelObj::BuildMessages()
{
	int aDepth = TEXT_Z_FIRST;
	for (int i = 0; i <= LEVEL_TEXT_COUNT; i++) {
		mTexts[i] = new TextFade();
		mTexts[i]->SetParent(this, nullptr);
		mTexts[i]->SetStateInt(STATE_Z, aDepth);
		aDepth--;
	}
	mTexts[LEVEL_PERSISTENT_TEXT]->SetStateInt(STATE_Z, TEXT_PERSISTENT_Z);
	for (int i = 0; i < LEVEL_BUG_SCORE_COUNT; i++) {
		mBugScores[i] = new BugScore();
		mBugScores[i]->SetParent(this, nullptr);
		mBugScores[i]->SetStateInt(STATE_Z, BUG_SCORE_Z_FIRST + i);
	}
}

// 0x43e650: the in-level menu's button
void LevelObj::BuildMenuButton()
{
	mMenuButton = new MenuButton();
	mMenuButton->SetParent(this, nullptr);
	Deactivate(mMenuButton);
}

// 0x43c1d0: a cell is on the field when its place has ground, off it when not
void LevelObj::ShowOrHideCell(FillObj* theCell)
{
	if ((unsigned int)gFillGrid[theCell->mColumn][theCell->mRow].mKind < FILL_KIND_CHECKED)
		theCell->Deactivate(theCell);
	else
		theCell->Activate(theCell, false);
}

// 0x43c200 (a place has ground once its kind is 10 or more; the inside, -1, counts as the original's unsigned compare says)
bool LevelObj::IsGroundKind(int theColumn, int theRow)
{
	return (unsigned int)gFillGrid[theColumn][theRow].mKind >= FILL_KIND_CHECKED;
}

// 0x43c0f0: the pipe's look from its place's kind (10 or more is taken as the kind less 10): a post has one of three
// pictures, a wall across or down one of four, and anything else the empty picture
void LevelObj::ChoosePipeLook(PipeObj* thePipe)
{
	int aKind = gFillGrid[thePipe->mColumn][thePipe->mRow].mKind;
	if (aKind >= FILL_KIND_CHECKED)
		aKind -= FILL_KIND_CHECKED;
	if (aKind == FILL_KIND_CORNER) {
		unsigned int aRandom = GameRandomNext();
		thePipe->mVariant = (int)(aRandom % 3);
		thePipe->SetStateInt(STATE_FRAME, thePipe->mVariant);
		thePipe->SetKind(FILL_KIND_CORNER);
	} else if (aKind == FILL_KIND_WALL_ACROSS) {
		unsigned int aRandom = GameRandomNext();
		thePipe->mVariant = (int)(aRandom & 3) + 3;
		thePipe->SetStateInt(STATE_FRAME, thePipe->mVariant);
		thePipe->SetKind(FILL_KIND_WALL_ACROSS);
	} else if (aKind == FILL_KIND_WALL_DOWN) {
		unsigned int aRandom = GameRandomNext();
		thePipe->mVariant = (int)(aRandom & 3) + 7;
		thePipe->SetStateInt(STATE_FRAME, thePipe->mVariant);
		thePipe->SetKind(FILL_KIND_WALL_DOWN);
	} else {
		thePipe->mVariant = 11;
		thePipe->SetStateInt(STATE_FRAME, 11);
		thePipe->Activate(thePipe, false);
		thePipe->SetKind(FILL_KIND_EMPTY);
	}
}

// 0x439e30: every cell and pipe goes onto its layer, cells showing where there is ground, and each pipe takes its look
void LevelObj::SetupCells()
{
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			FillGridCell& aPlace = gFillGrid[aColumn][aRow];
			aPlace.mCell->SetParent(mFillLayer, nullptr);
			ShowOrHideCell(aPlace.mCell);
			aPlace.mCell->SetMark(IsGroundKind(aColumn, aRow) ? START_MARK_INSIDE : 0);
			aPlace.mPipe->SetParent(mObjectLayer, nullptr);
			ChoosePipeLook(aPlace.mPipe);
			aPlace.mPipe->BuildFlames();
			aPlace.mPipe->RandomizeDuration();
			aPlace.mPipe->Activate(aPlace.mPipe, false);
		}
	}
}

// 0x43d5b0: every pipe has settled (the second of its two flags)
bool LevelObj::AllPipesSettledB()
{
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			if (!gFillGrid[aColumn][aRow].mPipe->IsSettledB())
				return false;
		}
	}
	return true;
}

// 0x43d5f0: every pipe has settled (the first flag); when they have, each is initialised once more
bool LevelObj::AllPipesSettledA()
{
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			if (!gFillGrid[aColumn][aRow].mPipe->IsSettledA())
				return false;
		}
	}
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++)
			gFillGrid[aColumn][aRow].mPipe->Init();
	}
	return true;
}

// 0x43d660: each pipe that is part of a wall links to the nearest post on each of its four sides, looking along its
// row and column while the pipes it passes are on the field
void LevelObj::LinkPipes()
{
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			PipeObj* aPipe = gFillGrid[aColumn][aRow].mPipe;
			aPipe->mUpLink = nullptr;
			aPipe->mRightLink = nullptr;
			aPipe->mDownLink = nullptr;
			aPipe->mLeftLink = nullptr;
			if (aPipe->mKind == 0)
				continue;
			for (int aNear = aRow - 1; aNear >= 0; aNear--) {
				PipeObj* aOther = gFillGrid[aColumn][aNear].mPipe;
				if (!aOther->IsActive())
					break;
				if (aOther->mKind == FILL_KIND_CORNER) {
					aPipe->mUpLink = aOther;
					break;
				}
			}
			for (int aNear = aColumn + 1; aNear < FILL_GRID_COLUMNS; aNear++) {
				PipeObj* aOther = gFillGrid[aNear][aRow].mPipe;
				if (!aOther->IsActive())
					break;
				if (aOther->mKind == FILL_KIND_CORNER) {
					aPipe->mRightLink = aOther;
					break;
				}
			}
			for (int aNear = aRow + 1; aNear < FILL_GRID_ROWS; aNear++) {
				PipeObj* aOther = gFillGrid[aColumn][aNear].mPipe;
				if (!aOther->IsActive())
					break;
				if (aOther->mKind == FILL_KIND_CORNER) {
					aPipe->mDownLink = aOther;
					break;
				}
			}
			for (int aNear = aColumn - 1; aNear >= 0; aNear--) {
				PipeObj* aOther = gFillGrid[aNear][aRow].mPipe;
				if (!aOther->IsActive())
					break;
				if (aOther->mKind == FILL_KIND_CORNER) {
					aPipe->mLeftLink = aOther;
					break;
				}
			}
		}
	}
}

// 0x43f3e0 (the level table the game builds as it starts): a cell and a pipe for every place of the grid
void Sexy::InitLevelTable()
{
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			gFillGrid[aColumn][aRow].mCell = FillObj::Create(aColumn, aRow);
			gFillGrid[aColumn][aRow].mPipe = PipeObj::Create(aColumn, aRow);
		}
	}
}

// 0x43c000: clears the grid, then lays the level out: a picture for the first five levels (the first world's
// shapes), a random one from the sixth on, and a plain one for the first level of the later worlds
void LevelObj::InitCells()
{
	Player* aPlayer = Player::sCurrent;
	int aLevel = aPlayer->mSavedProgress;
	if (aLevel < 0)
		aLevel = 0;
	PipeObj::sSoundPlayed = false;
	mOrderPos = aLevel % LEVEL_MASK_COLUMNS;
	GameRandomSeed((unsigned int)aLevel + 0x58d2764);
	mEdgeCount = 0;
	mCellCount = 0;
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			FillGridCell& aPlace = gFillGrid[aColumn][aRow];
			aPlace.mColumn = aColumn;
			aPlace.mRow = aRow;
			aPlace.mKind = FILL_KIND_EMPTY;
			aPlace.mUnused5 = 0;
			aPlace.mFirstBug = nullptr;
			aPlace.mLastBug = nullptr;
		}
	}
	if (aPlayer->mSavedProgress < 5) {
		MakeLayoutFromPicture(aPlayer->mSavedProgress);
		HollowOut();
		ClassifyCells();
		return;
	}
	if (aPlayer->mWorldProgress == 0.0f) {
		MakeLayoutFromPicture(-1);
		HollowOut();
		ClassifyCells();
		return;
	}
	MakeRandomLayout();
	HollowOut();
	ClassifyCells();
}

// 0x43cdf0: no bug is left on the field or in the grid
void LevelObj::ClearBugs()
{
	BugObj::KillAll();
	BugObj::ResetCounters();
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			gFillGrid[aColumn][aRow].mFirstBug = nullptr;
			gFillGrid[aColumn][aRow].mLastBug = nullptr;
		}
	}
}

// 0x43ce30: the ground comes back, with the sound of leaves: every pipe goes and every cell is replayed
void LevelObj::ClearCells()
{
	sApp->mSoundQueue->Request(SOUND_LEAVES, 0, false);
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			gFillGrid[aColumn][aRow].mPipe->Retire();
			gFillGrid[aColumn][aRow].mCell->Replay();
		}
	}
}

// 0x43d470: the next level's ground is laid out again while the stats are shown
void LevelObj::ReloadLevel()
{
	ClearBugs();
	mCellsReady = false;
	InitCells();
	mCellsCut = 0;
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			FillGridCell& aPlace = gFillGrid[aColumn][aRow];
			aPlace.mPipe->Refresh();
			aPlace.mCell->Replay();
			aPlace.mCell->SetMark(IsGroundKind(aColumn, aRow) ? START_MARK_INSIDE : 0);
		}
	}
}

// 0x43cc60: the bugs of the later levels: those of each list, the open-ground ones first
void LevelObj::FillKindLists()
{
	if (mOtherKindCount > LEVEL_KIND_LIST_MAX - 1)
		mOtherKindCount = LEVEL_KIND_LIST_MAX - 1;
	if (mOpenKindCount > LEVEL_KIND_LIST_MAX - 1)
		mOpenKindCount = LEVEL_KIND_LIST_MAX - 1;
	for (int i = 0; i < mOpenKindCount; i++)
		SpawnBug(mOpenKinds[i]);
	for (int i = 0; i < mOtherKindCount; i++)
		SpawnBug(mOtherKinds[i]);
}

// 0x43ccf0: the bugs of the level: the first four levels have a fixed few (more once the tutorial is done), the later
// ones the kinds PickBugKinds chose; then they are put on the field and sorted
void LevelObj::SpawnBugs()
{
	mSpawnTimer = 0;
	mSpawnTimer2 = 0;
	BugObj::Reset();
	Player* aPlayer = Player::sCurrent;
	switch (aPlayer->mSavedProgress) {
	case 0:
		if (aPlayer->mTutorialDone != 0)
			SpawnBug(0);
		break;
	case 1:
		if (aPlayer->mTutorialDone != 0)
			SpawnBug(1);
		SpawnBug(0);
		break;
	case 2:
		if (aPlayer->mTutorialDone != 0)
			SpawnBug(1);
		SpawnBug(0);
		SpawnBug(0);
		break;
	case 3:
		if (aPlayer->mTutorialDone != 0)
			SpawnBug(0);
		SpawnBug(0);
		SpawnBug(1);
		SpawnBug(1);
		break;
	default:
		FillKindLists();
		break;
	}
	PlaceBugs();
	BugObj::Shuffle();
}

// 0x43d4f0: the pipes have settled: the cells stop their settling, the bugs and the bonus go in, and the tutorial's
// hints start on the first three levels
void LevelObj::StartPlay()
{
	FillObj::ClearBonuses();
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++)
			gFillGrid[aColumn][aRow].mCell->Settle();
	}
	PipeObj::sSoundPlayed = false;
	SpawnBugs();
	PlaceBonus();
	if (HasIntro()) {
		Player::AppearAtMouse();
		Player::SetShotCooldown(SHOT_COOLDOWN_START);
	}
	if (Player::sCurrent->mWalkThruDone == 0) {
		int aLevel = Player::sCurrent->mSavedProgress;
		if (aLevel == 0) {
			WalkThru::ShowFirstStep();
		} else if (aLevel == 1) {
			WalkThru::ShowSecondStep();
		} else if (aLevel == 2) {
			WalkThru::ShowThirdStep();
			Player::sCurrent->mWalkThruDone = 1;
		}
	}
}

// 0x43a560: a level starts, theDelay frames before its first message: the button takes the mouse, the bugs are
// chosen, the player's counters reset and the level's name and target shown
void LevelObj::StartLevel(int theDelay)
{
	if (mMenuButton != nullptr) {
		mMenuButton->Reset();
		mMenuButtonLive = true;
	}
	PickBugKinds();
	Player::StartLevel();
	mFireworksOn = false;
	int aLevel = Player::sCurrent->mSavedProgress;
	if (aLevel != 0) {
		char aText[32];
		sprintf(aText, "LEVEL %d", aLevel + 1);
		ShowMessage(aText, (float)LEVEL_CENTER_X, 432.0f, theDelay + MESSAGE_DELAY_AFTER_START, 2.0f, 1.0f, 1.0f, 0.2f);
		sprintf(aText, "FILL %d%c TO WIN", Player::GetTargetPercent(), '%');
		ShowMessage(aText, (float)LEVEL_CENTER_X, 560.0f, theDelay + MESSAGE_DELAY_AFTER_START, 1.9f, 0.5f, 1.0f, 0.2f);
	}
	Player::sCurrent->mScoreBoard->ShowLevel(Player::sCurrent->mSavedProgress + 1);
	Player::StartLevelPlay();
	if (Player::sCurrent->mTenthUpgradeValue < 1)
		mFireBallCountdown = -1;
	else
		mFireBallCountdown = theDelay + FIREBALL_FIRST_FRAME;
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++)
			gFillGrid[aColumn][aRow].mCell->ShowPicture();
	}
}

// 0x43a710: the target is reached (or the ground cleared): the level is won
void LevelObj::EndLevel()
{
	if (mMenuButton != nullptr) {
		mMenuButtonLive = false;
		Deactivate(mMenuButton);
		if (Cursor::sCursor->IsActive())
			Cursor::Hide();
	}
	Player::sCurrent->mTutorialDone = 1;
	mGroundCleared = false;
	mTimer = STATS_WAIT_FRAMES;
	mState = LEVEL_STATE_CELEBRATE;
	WalkThru::Dismiss();
	Bonus::ClearAll();
	ClearBugs();
	mRope->Init();
	FillObj::ClearBonuses();
	HidePersistentMessage();
	if (BugObj::sAlive == 0 && Player::sCurrent->mSavedProgress > 0) {
		ShowMessage("ALL BUGS ELIMINATED!", (float)LEVEL_CENTER_X, 692.0f, 0x32, 1.0f, 1.0f, 1.0f, 0.2f, -1, 100);
		ShowMessage("20 BANANA BONUS!", (float)LEVEL_CENTER_X, 796.0f, 0x3c, 1.1f, 1.0f, 0.5f, 0.2f, -1, 100);
		Player::AddBananas(PANEL_BANANAS_BONUS);
	}
	Player::FinishLevel();
	if (Player::TakeMapEvent(false) != 0) {
		mFireworksOn = true;
		mFireworksDelay = WIN_FIREWORK_DELAY;
		mYouWin->SetStateInt(STATE_FRAME, 1);
		for (int i = 0; i < WIN_FIREWORKS; i++) {
			if (!mFireWorks[i]->IsActive()) {
				unsigned int aRandom = GameRandomNext();
				mFireWorks[i]->Launch((float)(int)(aRandom % FIREWORK_SPREAD_WIN - FIREWORK_X_MARGIN), FIREWORK_Y);
			}
		}
		sApp->ChangeMusic(MUSIC_STATS);
	} else {
		mFireworksOn = false;
		mYouWin->SetStateInt(STATE_FRAME, 0);
		sApp->ChangeMusic(MUSIC_STATS_2);
	}
	sApp->mSoundQueue->Request(SOUND_WINFLUTE, 0, false);
	Activate(mYouWin, false);
	mYouWin->SetStateInt(STATE_X, (int)YOU_WIN_X);
	mYouWin->SetStateInt(STATE_Y, (int)YOU_WIN_Y);
	mYouWin->SetStateInt(STATE_SCALE_X, 0);
	mYouWin->SetStateInt(STATE_SCALE_Y, 0);
	mYouWin->ComputeDrawn();
	mYouWin->StartTrack(0);
}

// 0x43ce80: the level is left: the intro (if still there), the bugs, the bonuses and the rope go, and the button;
// addScore puts the score in the high scores, clearCells brings the ground back
void LevelObj::Leave(bool addScore, bool clearCells)
{
	if (mIntro != nullptr) {
		if (!Intro::IsFinished())
			mIntro->Finish();
		mIntro = nullptr;
	}
	if (addScore)
		Player::AddHighScoreOfCurrent();
	WalkThru::Dismiss();
	ClearBugs();
	Bonus::ClearAll();
	FillObj::ClearBonuses();
	mRope->Init();
	if (mMenuButton != nullptr) {
		mMenuButtonLive = false;
		Deactivate(mMenuButton);
	}
	Player::sCurrent->mScoreBoard->ClearBananaIcons();
	Player::EndLevelPlay();
	if (clearCells)
		ClearCells();
}

// 0x43a990: the cut's last cell has gone
void LevelObj::OnFillDone()
{
	sApp->mSoundQueue->Request(SOUND_AREAFILLED, 0, false);
	WalkThru::OnAreaFilled();
}

// 0x43dc40: a cell has gone: the percent cut and the score of the cell
void LevelObj::OnCellCut()
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	aLevel->mCellsCut = aLevel->mCellsCut + 1;
	// port (crash): the original divides by the cell count whatever it is; a level with no ground cells makes 0 here
	int aPercent = 0;
	if (aLevel->mCellCount != 0)
		aPercent = (int)((double)aLevel->mCellsCut / (double)aLevel->mCellCount * 100.0);
	Player::SetPercent(aPercent);
	Player::AddScore(10);
}

// 0x43a9b0: what the level has ended with, once the board's figures have stopped counting
int LevelObj::GetResult()
{
	ScoreBoard* aBoard = Player::sCurrent->mScoreBoard;
	if (mState == LEVEL_STATE_DONE) {
		if (aBoard->IsDone() && BugObj::sCount == 0)
			return mVictoryShown ? LEVEL_RESULT_LOST : LEVEL_RESULT_WON;
	} else if (mState == LEVEL_STATE_LEAVING) {
		if (!mQuitConfirmed) {
			if (aBoard->IsDone())
				return LEVEL_RESULT_LOST;
		} else {
			if (aBoard->IsDone())
				return LEVEL_RESULT_QUIT;
		}
	}
	return LEVEL_RESULT_NONE;
}

// 0x43aa30: the in-level menu's button goes (the monkey cannot open the menu)
void LevelObj::HideMenuButton()
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	if (aLevel->mMenuButton != nullptr && aLevel->mMenuButton->IsActive()) {
		aLevel->Deactivate(aLevel->mMenuButton);
		aLevel->mMenuButtonLive = false;
	}
}

// 0x43aa60
void LevelObj::ShowGameOver()
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	ShowMessage("GAME OVER", (float)LEVEL_CENTER_X, 432.0f, 100, 2.0f, 1.0f, 1.0f, 0.2f);
	aLevel->mGameOverTimer = GAME_OVER_FRAMES;
	aLevel->mState = LEVEL_STATE_GAME_OVER;
	HideMenuButton();
}

// 0x43aad0
bool LevelObj::IsPlaying()
{
	// port (crash): the original reads sLevel without looking whether there is one
	if (sLevel == nullptr)
		return false;
	return static_cast<LevelObj*>(sLevel)->mState == LEVEL_STATE_PLAYING;
}

// 0x43aae0: a text of the level; the next of the four that take turns unless theSlot says which
void LevelObj::ShowMessage(const char* theText, float theX, float theY, int theDelay, float theScale, float theRed,
	float theGreen, float theBlue, int theSlot, int theExtraHold)
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	if (theSlot < 0)
		theSlot = aLevel->mTextRing;
	TextFade* aText = aLevel->mTexts[theSlot];
	aText->SetExtraHold(theExtraHold);
	aText->SetMode(0);
	aText->Start(theText, theX, theY, theScale, theRed, theGreen, theBlue, theDelay);
	aLevel->mTextRing = (aLevel->mTextRing + 1) % LEVEL_TEXT_COUNT;
}

// 0x43ab70
void LevelObj::HidePersistentMessage()
{
	static_cast<LevelObj*>(sLevel)->mTexts[LEVEL_PERSISTENT_TEXT]->Hide();
}

// 0x43ab80: the number a caught bug is worth floats up from where it was
void LevelObj::ShowBugScore(int theScore, float theX, float theY)
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	char aText[16];
	sprintf(aText, "%02d", theScore);
	aLevel->mBugScores[aLevel->mBugScoreRing]->Show(aText, theX, theY);
	aLevel->mBugScoreRing = (aLevel->mBugScoreRing + 1) % LEVEL_BUG_SCORE_COUNT;
}

// 0x43ac00: the banana figure shows the player's bananas, with the money sound when asked
void LevelObj::ShowBananas(bool thePlaySound)
{
	// port: the stand-in HUD (BuildHud) makes no figure
	if (mBananasText != nullptr) {
		mBananasText->SetText(FormatScore((unsigned int)Player::sCurrent->mBananas));
		mBananasText->StartTrack(0);
	}
	if (thePlaySound)
		sApp->mSoundQueue->Request(SOUND_MONEY, 0, false);
}

// 0x43ac60: the lives figure
void LevelObj::ShowLives(int theLives)
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	char aText[16];
	sprintf(aText, "%d", theLives);
	// port: the stand-in HUD (BuildHud) makes no figure
	if (aLevel->mScoreText != nullptr) {
		aLevel->mScoreText->SetText(aText);
		aLevel->mScoreText->StartTrack(0);
	}
}

// 0x439f00
bool LevelObj::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mFireBallCountdown >= 1) {
		// the three fire balls go at 61, 31 and 1 frames; any other value just counts down
		if (mFireBallCountdown == FIREBALL_SECOND)
			Player::sCurrent->mScoreBoard->LaunchFireBall(0);
		else if (mFireBallCountdown == FIREBALL_THIRD)
			Player::sCurrent->mScoreBoard->LaunchFireBall(1);
		else if (mFireBallCountdown == FIREBALL_LAST)
			Player::sCurrent->mScoreBoard->LaunchFireBall(2);
		mFireBallCountdown = mFireBallCountdown - 1;
	}
	if (Transition::GetPhase() != TRANSITION_OPEN)
		return true;
	if (mMenu != nullptr) {
		int aResult = mMenu->GetResult();
		if (aResult == IN_GAME_MENU_RESUME) {
			mMenu->Close();
			mMenu = nullptr;
			if (mMenuButtonLive)
				mMenuButton->Reset();
		} else if (aResult == IN_GAME_MENU_QUIT) {
			Player::RestoreLevelStart();
			Leave(false, true);
			mQuitConfirmed = true;
			mState = LEVEL_STATE_LEAVING;
			mMenu->Close();
			mMenu = nullptr;
		}
	}
	if (mMenu == nullptr && mMenuButton->TakeClicked()) {
		mMenu = new InGameMenu();
		Deactivate(mMenuButton);
	}
	if (mFireworksOn) {
		if (mFireworksDelay < 1) {
			if (sFrame % FIREWORK_PERIOD == 0) {
				for (int i = 0; i < LEVEL_FIREWORK_COUNT; i++) {
					if (!mFireWorks[i]->IsActive()) {
						unsigned int aRandom = GameRandomNext();
						mFireWorks[i]->Launch((float)(int)(aRandom % FIREWORK_SPREAD_PLAIN - FIREWORK_X_MARGIN),
							FIREWORK_Y);
						break;
					}
				}
			}
		} else {
			mFireworksDelay = mFireworksDelay - 1;
		}
	}
	switch (mState) {
	case LEVEL_STATE_INTRO:
		if (Intro::IsReady()) {
			mState = LEVEL_STATE_SETTLING;
			Player::sCurrent->mScoreBoard->Show();
			SetupCells();
			if (mMenuButton != nullptr)
				mMenuButton->Reset();
		}
		break;
	case LEVEL_STATE_SETTLING:
		if (!mCellsReady) {
			bool aSettled = AllPipesSettledA();
			if (aSettled != mCellsReady) {
				mCellsReady = aSettled;
				StartPlay();
				mState = LEVEL_STATE_PLAYING;
			}
		}
		break;
	case LEVEL_STATE_PLAYING:
		if (FillObj::IsIdle() &&
			(mGroundCleared || Player::GetTargetPercent() <= Player::sCurrent->mPercent))
			EndLevel();
		break;
	case LEVEL_STATE_STATS_WAIT:
		if (mTimer < 1)
			mState = LEVEL_STATE_STATS;
		else
			mTimer = mTimer - 1;
		break;
	case LEVEL_STATE_CELEBRATE:
		if (sUpdateLevel != 0)
			break;
		if (mTimer < 1) {
			mState = LEVEL_STATE_STATS_WAIT;
			mTimer = STATS_PAUSE_FRAMES;
			mStats = new Stats(this, (int)mCenterY, (int)mCenterX);
			ReloadLevel();
			Bonus::ClearAll();
			break;
		}
		mTimer = mTimer - 1;
		break;
	case LEVEL_STATE_STATS:
		if (!AllPipesSettledB())
			break;
		if (mStats == nullptr || !mStats->IsReady() || Player::sCurrent->mHidden) {
			sMouseHeld = false;
			break;
		}
		if (!sMouseHeld)
			break;
		sMouseHeld = false;
		mStats->Continue();
		if (Player::TakeMapEvent(true) == 0) {
			PlayLevelMusic();
			mState = LEVEL_STATE_SETTLING;
			StartLevel(REPLAY_DELAY);
			for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
				for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
					gFillGrid[aColumn][aRow].mPipe->Rebuild();
					gFillGrid[aColumn][aRow].mCell->SetMark(IsGroundKind(aColumn, aRow) ? START_MARK_INSIDE : 0);
				}
			}
			break;
		}
		if (Player::sCurrent->mMapLevel > LAST_WORLD) {
			ClearCells();
			mVictory->Show();
			mState = LEVEL_STATE_VICTORY;
			Player::AddHighScoreOfCurrent();
			Player::ResetCurrent();
			Player::SaveAll();
			mVictoryShown = true;
			break;
		}
		Leave(false, true);
		mState = LEVEL_STATE_DONE;
		return true;
	case LEVEL_STATE_GAME_OVER:
		if (mGameOverTimer < 1) {
			Leave(true, true);
			mState = LEVEL_STATE_LEAVING;
		} else {
			mGameOverTimer = mGameOverTimer - 1;
		}
		break;
	case LEVEL_STATE_VICTORY:
		// (the original goes on to leave when there is no victory scene; the scene is made only after level 0x1b8)
		if (mVictory != nullptr) {
			if (!mVictory->IsDone())
				break;
			Leave(false, false);
		} else {
			Leave(false, true);
		}
		mState = LEVEL_STATE_DONE;
		return true;
	default:
		break;
	}
	mFrame = mFrame + 1;
	return true;
}

// Whether the level being played takes the mouse
bool Sexy::LevelAcceptsMouse()
{
	return static_cast<LevelObj*>(ObjBase::sLevel)->mMenuButtonLive;
}

// 0x461320's object: the level's rope (+0x9000 of the level)
RopeObj* RopeObj::Current()
{
	// port (crash): the original reads sLevel without looking whether there is one
	if (sLevel == nullptr)
		return nullptr;
	return static_cast<LevelObj*>(sLevel)->mRope;
}

// The stand-ins ----------------------------------------------------------------------------------------------------

// port: the stand-in HUD has no banana panel or figures; the original (0x43e6d0) makes the panel "images/panel_bananas", the
// banana and lives figures (each with a track that pops it when the figure changes) and their keys
void LevelObj::BuildHud()
{
	mBananaPanel = new ObjBase();
	mBananaPanel->SetParent(this, nullptr);
	mBananasText = nullptr;
	mScoreText = nullptr;
}

// port: the stand-in has no "you win" picture beyond an empty object with one track (0x43edd0 builds its keys)
void LevelObj::BuildYouWin()
{
	mYouWin = new ObjBase();
	mYouWin->SetParent(this, nullptr);
	AnimKey* aKey = new AnimKey();
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey, aKey);
	mYouWin->AddTrack(aTrack, nullptr);
}

// port: 0x43d2b0 builds the two morph tables from the pictures; the stand-in leaves them unused
void LevelObj::BuildMorphTable()
{
	for (int anImage = 0; anImage < LEVEL_MORPH_IMAGES; anImage++) {
		mMorphWidth[anImage] = 0;
		mMorphHeight[anImage] = 0;
		for (int aRow = 0; aRow < LEVEL_MORPH_SIZE; aRow++) {
			for (int aColumn = 0; aColumn < LEVEL_MORPH_SIZE; aColumn++)
				mMorph[anImage][aColumn][aRow] = MorphCell();
		}
	}
}

// port: 0x43cb00 (with the budgets of 0x43c900) picks the bug kinds the level will have; the stand-in picks none
void LevelObj::PickBugKinds()
{
	InitSpawnTable();
	mOpenKindCount = 0;
	mOtherKindCount = 0;
}

// port: 0x43c600 makes a bug of the kind (a jump table over the eleven bug classes, none translated yet)
void LevelObj::SpawnBug(int theKind)
{
	(void)theKind;
}

// port: 0x43b370 puts the bugs made onto the field (with 0x43acc0, 0x43ae10, 0x43b170 and 0x43b2b0)
void LevelObj::PlaceBugs()
{
}

// The layout -----------------------------------------------------------------------------------------------------------

static const int GRID_PLACES = FILL_GRID_COLUMNS * FILL_GRID_ROWS;   // the places in the grid (0x500)
static const int PICTURE_NAME_SIZE = 56;           // the original's name buffer
static const int SHAPE_CENTER_COLUMN = 10;         // where the random shapes grow from, in the mask
static const int SHAPE_CENTER_ROW = 8;
static const int SHAPE_RIGHT_CENTER_COLUMN = 11;   // ... and the second blob's start
static const int MASK_BOX_MIN = 2;                 // the hole's smallest half-size
static const int MASK_BOX_NARROW = 9;              // a clear square this wide gets a hole cut in it
static const int MASK_BOX_MAX_WIDTH = 0x26;
static const int MASK_BOX_MAX_HEIGHT = 0x1e;
static const int MASK_FILL_CLEAR = 0;              // FillMask's value 3 clears the inside and fills a ring round it with 1
static const int MASK_FILL_RING = 3;
static const int MASK_MIRROR_ROWS = 10;            // the top ten rows of the mask are copied, turned half way round, to the bottom
static const int MASK_SQUARE_STEPS = 16;
static const int BONUS_MIN_PROGRESS = 4;           // the level the hidden bonus starts to appear on
static const int BONUS_FREE_RADIUS = 2;            // the bonus needs ground this far all round

// 0x43b630: the kind of a place of the grid, or 0 (no ground) outside it
static int GetPlaceKind(int theColumn, int theRow)
{
	if (theColumn < 0 || theRow < 0 || theColumn >= FILL_GRID_COLUMNS || theRow >= FILL_GRID_ROWS)
		return 0;
	return gFillGrid[theColumn][theRow].mKind;
}

// 0x43b660: the place has ground on all eight sides (the place itself is not looked at)
static bool IsSurrounded(int theColumn, int theRow)
{
	if (GetPlaceKind(theColumn - 1, theRow - 1) == 0)
		return false;
	if (GetPlaceKind(theColumn, theRow - 1) == 0)
		return false;
	if (GetPlaceKind(theColumn + 1, theRow - 1) == 0)
		return false;
	if (GetPlaceKind(theColumn - 1, theRow) == 0)
		return false;
	if (GetPlaceKind(theColumn + 1, theRow) == 0)
		return false;
	if (GetPlaceKind(theColumn - 1, theRow + 1) == 0)
		return false;
	if (GetPlaceKind(theColumn, theRow + 1) == 0)
		return false;
	return GetPlaceKind(theColumn + 1, theRow + 1) != 0;
}

// 0x43b6e0 (ECX the row, EDX the column): every place within two of this one has ground, and the whole square is inside the
// grid
static bool HasGroundAround(int theColumn, int theRow)
{
	int aFirstColumn = theColumn - BONUS_FREE_RADIUS;
	int aLastColumn = theColumn + BONUS_FREE_RADIUS;
	int aFirstRow = theRow - BONUS_FREE_RADIUS;
	int aLastRow = theRow + BONUS_FREE_RADIUS;
	if (aFirstColumn < 0 || aFirstRow < 0 || aLastColumn >= FILL_GRID_COLUMNS || aLastRow >= FILL_GRID_ROWS)
		return false;
	for (int aRow = aFirstRow; aRow <= aLastRow; aRow++) {
		for (int aColumn = aFirstColumn; aColumn <= aLastColumn; aColumn++) {
			if (gFillGrid[aColumn][aRow].mKind == 0)
				return false;
		}
	}
	return true;
}

// 0x43b9d0 (ESI the column, EDX the row): what a place of the ground's edge is. 2 when ground lies to its left and right
// only, 3 when it lies above and below only, 1 (a post) otherwise
static int GetWallShape(int theColumn, int theRow)
{
	bool aAbove = false;
	bool aBelow = false;
	bool aRight = false;
	bool aLeft = false;
	if (GetPlaceKind(theColumn, theRow - 1) > 0)
		aAbove = true;
	if (GetPlaceKind(theColumn, theRow + 1) > 0)
		aBelow = true;
	if (GetPlaceKind(theColumn + 1, theRow) > 0)
		aRight = true;
	if (GetPlaceKind(theColumn - 1, theRow) > 0)
		aLeft = true;
	if (!aAbove) {
		if (!aBelow && aRight && aLeft)
			return FILL_KIND_WALL_ACROSS;
	} else if (aBelow && !aRight && !aLeft) {
		return FILL_KIND_WALL_DOWN;
	}
	return FILL_KIND_CORNER;
}

// 0x43b530: the box of half-width theHalfWidth and half-height theHalfHeight round (theCenterColumn, theCenterRow) in the
// layout mask takes theValue (3 first fills a box two bigger with 1, then clears the inside, which leaves a ring of 1).
// Gives -1 when the box lies wholly outside the mask, 1 when it was cut short at an edge, 0 when it fits
int LevelObj::FillMask(int theCenterColumn, int theCenterRow, int theHalfWidth, int theHalfHeight, int theValue)
{
	if (theValue == MASK_FILL_RING) {
		FillMask(theCenterColumn, theCenterRow, theHalfWidth + 2, theHalfHeight + 2, 1);
		theValue = MASK_FILL_CLEAR;
	}
	int aFirstColumn = theCenterColumn - theHalfWidth;
	int aFirstRow = theCenterRow - theHalfHeight;
	int aLastRow = theCenterRow + theHalfHeight;
	int aLastColumn = theCenterColumn + theHalfWidth;
	if (aFirstColumn >= LEVEL_MASK_COLUMNS || aFirstRow >= LEVEL_MASK_ROWS || aLastColumn < 0 || aLastRow < 0)
		return -1;
	int aCutShort = 0;
	if (aFirstColumn < 0) {
		aCutShort = 1;
		aFirstColumn = 0;
	}
	if (aFirstRow < 0) {
		aCutShort = 1;
		aFirstRow = 0;
	}
	if (aLastColumn >= LEVEL_MASK_COLUMNS) {
		aCutShort = 1;
		aLastColumn = LEVEL_MASK_COLUMNS - 1;
	}
	if (aLastRow >= LEVEL_MASK_ROWS) {
		aLastRow = LEVEL_MASK_ROWS - 1;
		aCutShort = 1;
	}
	for (int aRow = aFirstRow; aRow <= aLastRow; aRow++) {
		for (int aColumn = aFirstColumn; aColumn <= aLastColumn; aColumn++)
			mMask[aColumn][aRow] = theValue;
	}
	return aCutShort;
}

// 0x43ba70: how wide and high the square round the mask's middle (10, 8) is that has ground in every place, growing a
// ring at a time from 3 by 3 (to at most 33 by 33). Both come out the same; they are 0 when even the 3 by 3 has a hole.
// Gives whether the loop ran to the end
bool LevelObj::MeasureMaskSquare(int* theWidth, int* theHeight)
{
	*theWidth = 0;
	*theHeight = 0;
	int aFirstColumn = 9;
	int aFirstRow = 7;
	int aLastColumn = 11;
	int aLastRow = 9;
	const int* aMask = &mMask[0][0];
	for (int aStep = 0; aStep < MASK_SQUARE_STEPS; aStep++) {
		for (int aRow = aFirstRow; aRow <= aLastRow; aRow++) {
			for (int aColumn = aFirstColumn; aColumn <= aLastColumn; aColumn++) {
				// port (crash): the square grows past the mask's edge (rows after seven steps, columns after ten), and the
				// original reads the memory that lies beyond it; a place outside the mask counts as empty here
				int anIndex = aColumn * LEVEL_MASK_ROWS + aRow;
				if (anIndex < 0 || anIndex >= LEVEL_MASK_COLUMNS * LEVEL_MASK_ROWS || aMask[anIndex] == 0)
					return false;
			}
		}
		*theWidth = aLastColumn - aFirstColumn + 1;
		*theHeight = aLastRow - aFirstRow + 1;
		aFirstColumn--;
		aFirstRow--;
		aLastColumn++;
		aLastRow++;
	}
	return true;
}

// 0x43be60: the ground of the first levels is the green of a picture of the game's own: "images/levels/level_NN" (NN the
// level counted from one) or, for theLevel below 0, the map's world's "images/levels/lsection_NN". A pixel with any green
// is ground, one place of the grid for a pixel
void LevelObj::MakeLayoutFromPicture(int theLevel)
{
	char aName[PICTURE_NAME_SIZE];
	if (theLevel < 0)
		snprintf(aName, sizeof(aName), "images/levels/lsection_%02d", Player::sCurrent->mMapLevel + 1);
	else
		snprintf(aName, sizeof(aName), "images/levels/level_%02d", theLevel + 1);
	DDImage* aPicture = sApp->GetImage(aName, true);
	if (aPicture == nullptr)
		return;
	if (aPicture->GetBits() != nullptr && aPicture->GetHeight() > 0) {
		for (int aRow = 0; aRow < aPicture->GetHeight(); aRow++) {
			for (int aColumn = 0; aColumn < aPicture->GetWidth(); aColumn++) {
				// (the original makes a Color of the pixel, 0x479020, and tests its green)
				ulong aPixel = aPicture->GetBits()[aPicture->GetWidth() * aRow + aColumn];
				int aGreen = (int)(aPixel >> 8) & 0xff;
				if (aGreen > 0) {
					// port (crash): the original does not look whether the picture is larger than the grid
					if (aColumn < FILL_GRID_COLUMNS && aRow < FILL_GRID_ROWS)
						gFillGrid[aColumn][aRow].mKind = 1;
				}
			}
		}
	}
	delete aPicture;
}

// 0x43bb50: a random shape. Two blobs of boxes are drawn from (10, 8) and (11, 8) in the half-size mask, the top ten rows are
// mirrored into the bottom, and the levels finished (the remainder of four) decide which bars cross the middle; if the clear
// square in the middle is wide, a hole with a ring is cut in it. Each mask place then makes a block of two by two places of
// the grid
void LevelObj::MakeRandomLayout()
{
	int aProgress = Player::sCurrent->mSavedProgress;
	memset(mMask, 0, sizeof(mMask));

	// the first blob: boxes that walk up and to the left until one is cut short by the mask's edge
	int aCenterColumn = SHAPE_CENTER_COLUMN;
	int aCenterRow = SHAPE_CENTER_ROW;
	unsigned int aHalfWidth = GameRandomNext() % 5 + 3;
	unsigned int aHalfHeight = (GameRandomNext() & 3) + 3;
	int aCutShort;
	do {
		aCutShort = FillMask(aCenterColumn, aCenterRow, (int)aHalfWidth, (int)aHalfHeight, 1);
		aCenterColumn += -1 - (int)(GameRandomNext() % aHalfWidth);
		aCenterRow += -1 - (int)(GameRandomNext() % aHalfHeight);
		aHalfWidth = GameRandomNext() % 3 + 2;
		aHalfHeight = (GameRandomNext() & 1) + 2;
	} while (aCutShort == 0);

	// the second blob: boxes that walk up and to the right
	aCenterColumn = SHAPE_RIGHT_CENTER_COLUMN;
	aCenterRow = SHAPE_CENTER_ROW;
	aHalfWidth = (GameRandomNext() & 3) + 3;
	aHalfHeight = GameRandomNext() % 3 + 3;
	do {
		aCutShort = FillMask(aCenterColumn, aCenterRow, (int)aHalfWidth, (int)aHalfHeight, 1);
		aHalfWidth = aHalfWidth - 1;
		aCenterColumn += (int)(GameRandomNext() % aHalfWidth) + 2;
		aHalfHeight = aHalfHeight - 1;
		aCenterRow += -2 - (int)(GameRandomNext() % aHalfHeight);
		aHalfWidth = GameRandomNext() % 3 + 2;
		aHalfHeight = (GameRandomNext() & 1) + 2;
	} while (aCutShort == 0);

	// the top ten rows turned half way round into the bottom ones
	for (int aRow = 0; aRow < MASK_MIRROR_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < LEVEL_MASK_COLUMNS; aColumn++)
			mMask[LEVEL_MASK_COLUMNS - 1 - aColumn][LEVEL_MASK_ROWS - 1 - aRow] = mMask[aColumn][aRow];
	}

	// the bars through the middle: a wide one for 1 and 3, a tall one for 2 and 3, as the signed remainder of four gives
	int aShape = aProgress % 4;
	if (aShape == 3)
		FillMask(SHAPE_CENTER_COLUMN, SHAPE_CENTER_ROW, 11, 1, 1);
	if (aShape == 3 || aShape == 2)
		FillMask(SHAPE_CENTER_COLUMN, SHAPE_CENTER_ROW, 1, 9, 1);
	else if (aShape == 1)
		FillMask(SHAPE_CENTER_COLUMN, SHAPE_CENTER_ROW, 11, 1, 1);

	// a clear square in the middle that is wide gets a hole with a ring round it
	int aSquareWidth;
	int aSquareHeight;
	MeasureMaskSquare(&aSquareWidth, &aSquareHeight);
	if (aSquareWidth >= MASK_BOX_NARROW) {
		int aBoxWidth = aSquareWidth;
		if (FILL_GRID_COLUMNS - aBoxWidth < 2)
			aBoxWidth = MASK_BOX_MAX_WIDTH;
		int aBoxHeight = aSquareHeight;
		if (FILL_GRID_ROWS - aBoxHeight < 2)
			aBoxHeight = MASK_BOX_MAX_HEIGHT;
		else if (aBoxHeight < 4)
			aBoxHeight = (int)(GameRandomNext() % 5) + 4;
		int aHalfOfWidth = aBoxWidth / 2;
		int aHoleHalfWidth = ((int)(GameRandomNext() % (unsigned int)(aBoxWidth - aHalfOfWidth + 1)) + aHalfOfWidth) / 2;
		int aHalfOfHeight = aBoxHeight / 2;
		int aHoleHalfHeight = ((int)(GameRandomNext() % (unsigned int)(aBoxHeight - aHalfOfHeight + 1)) + aHalfOfHeight) / 2;
		if (aHoleHalfWidth < MASK_BOX_MIN)
			aHoleHalfWidth = MASK_BOX_MIN;
		if (aHoleHalfHeight < MASK_BOX_MIN)
			aHoleHalfHeight = MASK_BOX_MIN;
		FillMask(SHAPE_CENTER_COLUMN, SHAPE_CENTER_ROW, aHoleHalfWidth, aHoleHalfHeight, 1);
		FillMask(SHAPE_CENTER_COLUMN, SHAPE_CENTER_ROW, aHoleHalfWidth - 1, aHoleHalfHeight - 1, MASK_FILL_RING);
	}

	// every place of the mask is a block of two by two places of the grid
	for (int aRow = 0; aRow < LEVEL_MASK_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < LEVEL_MASK_COLUMNS; aColumn++) {
			int aValue = mMask[aColumn][aRow];
			gFillGrid[aColumn * 2][aRow * 2].mKind = aValue;
			gFillGrid[aColumn * 2 + 1][aRow * 2].mKind = aValue;
			gFillGrid[aColumn * 2][aRow * 2 + 1].mKind = aValue;
			gFillGrid[aColumn * 2 + 1][aRow * 2 + 1].mKind = aValue;
		}
	}
}

// 0x43b800: ground with ground on all eight sides becomes inside ground (-1); one place at a time, in order of the rows
void LevelObj::HollowOut()
{
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			if (gFillGrid[aColumn][aRow].mKind != 0 && IsSurrounded(aColumn, aRow))
				gFillGrid[aColumn][aRow].mKind = FILL_KIND_INSIDE;
		}
	}
}

// 0x43b850: every place that has ground is sorted: inside places go to the open list; the others become a post (1), a
// wall across (2) or a wall down (3) by what is next to them, and the walls go to the wall list. A place whose
// neighbours (below and to the right, as its kind needs) have ground counts as checked (10 more) and is one of the
// level's cells. Inside places count as cells too
void LevelObj::ClassifyCells()
{
	mEdgeCount = 0;
	mCellCount = 0;
	mWallCellCount = 0;
	mOpenCellCount = 0;
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			FillGridCell& aPlace = gFillGrid[aColumn][aRow];
			if (aPlace.mKind == 0)
				continue;
			if (aPlace.mKind < 1) {
				mOpenCells[mOpenCellCount] = aColumn + aRow * FILL_GRID_COLUMNS;
				mOpenCellCount++;
			} else {
				bool aChecked = false;
				int aShape = GetWallShape(aColumn, aRow);
				if (aShape == FILL_KIND_CORNER) {
					aPlace.mKind = FILL_KIND_CORNER;
					aChecked = true;
					if (GetPlaceKind(aColumn, aRow + 1) == 0)
						aChecked = false;
					if (GetPlaceKind(aColumn + 1, aRow + 1) == 0)
						aChecked = false;
					if (GetPlaceKind(aColumn + 1, aRow) == 0)
						aChecked = false;
					mEdgeCount++;
				} else if (aShape == FILL_KIND_WALL_ACROSS) {
					aPlace.mKind = FILL_KIND_WALL_ACROSS;
					if (GetPlaceKind(aColumn, aRow + 1) != 0)
						aChecked = true;
					mEdgeCount++;
					mWallCells[mWallCellCount] = aColumn + aRow * FILL_GRID_COLUMNS;
					mWallCellCount++;
				} else if (aShape == FILL_KIND_WALL_DOWN) {
					aPlace.mKind = FILL_KIND_WALL_DOWN;
					if (GetPlaceKind(aColumn + 1, aRow) != 0)
						aChecked = true;
					mEdgeCount++;
					mWallCells[mWallCellCount] = aColumn + aRow * FILL_GRID_COLUMNS;
					mWallCellCount++;
				} else {
					aPlace.mKind = 0;
					continue;
				}
				if (!aChecked)
					continue;
				aPlace.mKind = aPlace.mKind + FILL_KIND_CHECKED;
			}
			mCellCount++;
		}
	}
}

// 0x43b750: from the fourth level on, the hidden bonus goes under the first place in the shuffled order that has ground
// within two places on every side (as many as the grid has places are tried, the order's position moving on each time)
void LevelObj::PlaceBonus()
{
	if (Player::sCurrent->mSavedProgress < BONUS_MIN_PROGRESS)
		return;
	for (int aTry = 0; aTry < GRID_PLACES; aTry++) {
		unsigned int aPlace = (unsigned int)mCellOrder[mOrderPos];
		mOrderPos = (mOrderPos + 1) % LEVEL_CELL_LIST_MAX;
		if (aPlace < (unsigned int)LEVEL_CELL_LIST_MAX) {
			int aRow = (int)aPlace / FILL_GRID_COLUMNS;
			int aColumn = (int)aPlace - aRow * FILL_GRID_COLUMNS;
			if (HasGroundAround(aColumn, aRow)) {
				mBonusCoin->Place(aColumn, aRow);
				gFillGrid[aColumn][aRow].mCell->SetBonus(mBonusCoin);
				return;
			}
		}
	}
}

// The rope's preview and the cut ----------------------------------------------------------------------------------

static const int PIPE_GROW_FRAMES = 15;            // the walls' first key: grows in from big and faint
static const int PIPE_SETTLE_FRAMES = 100;         // the second key
static const float PIPE_GROW_SCALE = 3.6f;
static const int PIPE_ROTATION_RANGE = 0x5b;       // the walls start turned by up to 45 degrees either way
static const int PIPE_ROTATION_BIAS = 0x2d;
static const int PIPE_DELAY_PER_PLACE = 7;         // frames a wall waits for each place it is from the shot's
static const int PIPE_POST_VARIANTS = 3;
static const int PIPE_WALL_VARIANTS = 4;
static const int PIPE_ACROSS_FIRST_VARIANT = 3;
static const int PIPE_DOWN_FIRST_VARIANT = 7;
static const int PREVIEW_FREE = 1;                 // the preview map: a cell that can be cut
static const int PREVIEW_WALL = 2;                 // a wall or a post
static const int PREVIEW_FILLED = 9;               // a cell the flood fill has been to
static const int PREVIEW_SIDE_A = 2;               // the sides of the rope the flood fills mark the cells with (FillObj::mSide)
static const int PREVIEW_SIDE_B = 3;
static const int PREVIEW_SMALL_AREA = 0x20;        // both sides are cut when neither has more cells than this

// One wall of the cut (the body of both of 0x43d800's loops): the pipe at thePlace starts again, as a post at the
// rope's two ends and as a wall (theWallKind, its picture theFirstVariant and up) between them, growing in after as many
// frames as it is places from the shot
static void StartWallPipe(FillGridCell& thePlace, int theDistance, bool theIsEnd, int theWallKind, int theFirstVariant)
{
	PipeObj* aPipe = thePlace.mPipe;
	aPipe->SetStateInt(STATE_ALPHA, 0);
	aPipe->SetStateInt(STATE_SCALE_X, 0);
	aPipe->SetStateInt(STATE_SCALE_Y, 0);
	AnimKey* aGrowKey = aPipe->mAppearKey;
	aGrowKey->SetDuration(PIPE_GROW_FRAMES);
	aGrowKey->mValue[STATE_SCALE_X] = PIPE_GROW_SCALE;
	aGrowKey->mValue[STATE_SCALE_Y] = PIPE_GROW_SCALE;
	aGrowKey->mValue[STATE_RED] = 1.0f;
	aGrowKey->mValue[STATE_GREEN] = 1.0f;
	aGrowKey->mValue[STATE_BLUE] = 1.0f;
	aGrowKey->mValue[STATE_ALPHA] = 0.0f;
	int aTurn = (int)(GameRandomNext() % PIPE_ROTATION_RANGE) - PIPE_ROTATION_BIAS;
	aGrowKey->mValue[STATE_ROTATION] = (float)((double)aTurn * 3.1415927410125732 / 180.0);
	AnimKey* aSettleKey = aPipe->mSettleKey;
	aSettleKey->SetDuration(PIPE_SETTLE_FRAMES);
	aSettleKey->mValue[STATE_SCALE_X] = 1.0f;
	aSettleKey->mValue[STATE_SCALE_Y] = 1.0f;
	aSettleKey->mValue[STATE_RED] = 1.0f;
	aSettleKey->mValue[STATE_GREEN] = 1.0f;
	aSettleKey->mValue[STATE_BLUE] = 1.0f;
	aSettleKey->mValue[STATE_ALPHA] = 1.0f;
	aSettleKey->mValue[STATE_ROTATION] = 0.0f;
	int aDistance = theDistance < 0 ? -theDistance : theDistance;
	aPipe->mAppearTrack->SetDelay(aDistance * PIPE_DELAY_PER_PLACE, false);
	aPipe->StartTrack(aPipe->mAppearTrackIndex);
	int aVariant;
	if (!theIsEnd) {
		thePlace.mKind = theWallKind;
		aPipe->SetKind(theWallKind);
		aVariant = (int)(GameRandomNext() & (PIPE_WALL_VARIANTS - 1)) + theFirstVariant;
	} else {
		// the ends keep their size from the start, and wait for nothing
		aGrowKey->mValue[STATE_SCALE_X] = 1.0f;
		aGrowKey->mValue[STATE_SCALE_Y] = 1.0f;
		aGrowKey->mValue[STATE_ALPHA] = 1.0f;
		aGrowKey->mValue[STATE_ROTATION] = 0.0f;
		aPipe->mAppearTrack->SetDelay(0, false);
		thePlace.mKind = FILL_KIND_CORNER;
		aPipe->SetKind(FILL_KIND_CORNER);
		aVariant = (int)(GameRandomNext() % PIPE_POST_VARIANTS);
	}
	aPipe->mVariant = aVariant;
	aPipe->SetStateInt(STATE_FRAME, aVariant);
	aPipe->Activate(aPipe, false);
}

// 0x43d800 (ECX the shot's column, EAX its row, then the direction and the two ends): the pipes from theFrom to theTo
// along the rope's row (or column when theVertical) start over as posts at the ends and walls between, and are linked
void LevelObj::StartWallPipes(int theCellX, int theCellY, int theVertical, int theFrom, int theTo)
{
	if (theVertical == 0) {
		int aDistance = theCellX - theFrom;
		for (int aColumn = theFrom; aColumn <= theTo; aColumn++) {
			StartWallPipe(gFillGrid[aColumn][theCellY], aDistance, aColumn == theFrom || aColumn == theTo,
				FILL_KIND_WALL_ACROSS, PIPE_ACROSS_FIRST_VARIANT);
			aDistance--;
		}
		LinkPipes();
		return;
	}
	if (theFrom <= theTo) {
		int aDistance = theCellY - theFrom;
		for (int aRow = theFrom; aRow <= theTo; aRow++) {
			StartWallPipe(gFillGrid[theCellX][aRow], aDistance, aRow == theFrom || aRow == theTo,
				FILL_KIND_WALL_DOWN, PIPE_DOWN_FIRST_VARIANT);
			aDistance--;
		}
	}
	LinkPipes();
}

// 0x43dc80: the flood fill of the preview map: the free places (1) joined to (theX, theY) take 9, and the grid cell
// each lies in (a place of the map is half a cell, and the map starts a place in) is marked with theSide. Counts them in
// mFloodCount. The last of its four directions is the loop
void LevelObj::FloodPreview(int theX, int theY, int theSide)
{
	if (theX < 0)
		return;
	while (theY >= 0 && theX < LEVEL_PREVIEW_COLUMNS && theY < LEVEL_PREVIEW_ROWS
		&& mPreviewMap[theX][theY] == PREVIEW_FREE) {
		// (C's division, which gives 0 for -1)
		int aCellColumn = (theX - 1) / 2;
		int aCellRow = (theY - 1) / 2;
		if (aCellColumn >= 0 && aCellRow >= 0)
			gFillGrid[aCellColumn][aCellRow].mCell->mSide = theSide;
		mFloodCount++;
		mPreviewMap[theX][theY] = PREVIEW_FILLED;
		FloodPreview(theX + 1, theY, theSide);
		FloodPreview(theX - 1, theY, theSide);
		FloodPreview(theX, theY + 1, theSide);
		theY = theY - 1;
	}
}

// 0x43dd60: the preview map (two places for each cell of the grid, and one more at the top and left) is drawn afresh: 1 for
// each cell that is on the field and not cut, 2 for each pipe and the walls it joins to, and every cell's mSide is cleared
void LevelObj::BuildPreviewMap()
{
	memset(mPreviewMap, 0, sizeof(mPreviewMap));
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			FillObj* aCell = gFillGrid[aColumn][aRow].mCell;
			PipeObj* aPipe = gFillGrid[aColumn][aRow].mPipe;
			aCell->mUnused288 = 0;
			aCell->mSide = 0;
			if (aCell->IsActive() && !aCell->mCut) {
				aCell->mUnused288 = 1;
				mPreviewMap[aColumn * 2 + 1][aRow * 2 + 1] = PREVIEW_FREE;
				mPreviewMap[aColumn * 2 + 2][aRow * 2 + 1] = PREVIEW_FREE;
				mPreviewMap[aColumn * 2 + 1][aRow * 2 + 2] = PREVIEW_FREE;
				mPreviewMap[aColumn * 2 + 2][aRow * 2 + 2] = PREVIEW_FREE;
			}
			if (!aPipe->IsActive())
				continue;
			mPreviewMap[aColumn * 2][aRow * 2] = PREVIEW_WALL;
			if (aPipe->mKind == FILL_KIND_CORNER) {
				PipeObj* anUp = nullptr;
				if (aRow - 1 >= 0) {
					anUp = gFillGrid[aColumn][aRow - 1].mPipe;
					if (!anUp->IsActive())
						anUp = nullptr;
				}
				PipeObj* aLeft = nullptr;
				if (aColumn - 1 >= 0) {
					aLeft = gFillGrid[aColumn - 1][aRow].mPipe;
					if (!aLeft->IsActive())
						aLeft = nullptr;
				}
				PipeObj* aDown = nullptr;
				if (aRow + 1 < FILL_GRID_ROWS) {
					aDown = gFillGrid[aColumn][aRow + 1].mPipe;
					if (!aDown->IsActive())
						aDown = nullptr;
				}
				PipeObj* aRight = nullptr;
				if (aColumn + 1 < FILL_GRID_COLUMNS) {
					aRight = gFillGrid[aColumn + 1][aRow].mPipe;
					if (!aRight->IsActive())
						aRight = nullptr;
				}
				if (anUp != nullptr && anUp->mKind != FILL_KIND_WALL_ACROSS)
					mPreviewMap[aColumn * 2][aRow * 2 - 1] = PREVIEW_WALL;
				if (aDown != nullptr && aDown->mKind != FILL_KIND_WALL_ACROSS)
					mPreviewMap[aColumn * 2][aRow * 2 + 1] = PREVIEW_WALL;
				if (aRight != nullptr && aRight->mKind != FILL_KIND_WALL_DOWN)
					mPreviewMap[aColumn * 2 + 1][aRow * 2] = PREVIEW_WALL;
				if (aLeft != nullptr && aLeft->mKind != FILL_KIND_WALL_DOWN)
					mPreviewMap[aColumn * 2 - 1][aRow * 2] = PREVIEW_WALL;
			} else if (aPipe->mKind == FILL_KIND_WALL_ACROSS) {
				mPreviewMap[aColumn * 2 + 1][aRow * 2] = PREVIEW_WALL;
			} else if (aPipe->mKind == FILL_KIND_WALL_DOWN) {
				mPreviewMap[aColumn * 2][aRow * 2 + 1] = PREVIEW_WALL;
			}
		}
	}
}

// 0x43df60 (ECX theTo, EDX the row or EAX the column): the rope's own line goes into the preview map as walls, two places
// to each place of the grid, and the cells it crosses have their (unused) mark cleared
void LevelObj::MarkPreviewLine(int theCellX, int theCellY, int theVertical, int theFrom, int theTo)
{
	if (theVertical == 0) {
		for (int aColumn = theFrom; aColumn <= theTo; aColumn++) {
			gFillGrid[aColumn][theCellY].mCell->mUnused288 = 0;
			mPreviewMap[aColumn * 2][theCellY * 2] = PREVIEW_WALL;
			mPreviewMap[aColumn * 2 + 1][theCellY * 2] = PREVIEW_WALL;
		}
	} else {
		for (int aRow = theFrom; aRow <= theTo; aRow++) {
			gFillGrid[theCellX][aRow].mCell->mUnused288 = 0;
			mPreviewMap[theCellX * 2][aRow * 2] = PREVIEW_WALL;
			mPreviewMap[theCellX * 2][aRow * 2 + 1] = PREVIEW_WALL;
		}
	}
}

// 0x43e020: no cell glows
void LevelObj::ClearRopePreview()
{
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++)
			gFillGrid[aColumn][aRow].mCell->Unhighlight();
	}
}

// 0x43e060: the cells that the rope from (theCellX, theCellY) between theFrom and theTo would cut glow. The preview map is
// built and the two sides of the rope are flooded (side 3 from the places below or to the right of it, side 2 from those
// above or to the left); the side with the fewer cells (side 2 when they are as many) is the one that would go, or both
// when neither has more than 32, or when no bug is alive and the tutorial is done or a level has been played. A side that
// cannot go does not glow
void LevelObj::ShowRopePreview(int theCellX, int theCellY, int theVertical, int theFrom, int theTo)
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	aLevel->BuildPreviewMap();
	aLevel->MarkPreviewLine(theCellX, theCellY, theVertical, theFrom, theTo);
	aLevel->mFloodCount = 0;
	if (theVertical == 0) {
		for (int aColumn = theFrom; aColumn <= theTo; aColumn++)
			aLevel->FloodPreview(aColumn * 2, theCellY * 2 + 1, PREVIEW_SIDE_B);
	} else {
		for (int aRow = theFrom; aRow <= theTo; aRow++)
			aLevel->FloodPreview(theCellX * 2 + 1, aRow * 2, PREVIEW_SIDE_B);
	}
	int aSideBCells = aLevel->mFloodCount - theFrom + 1 + theTo;
	aLevel->mFloodCount = 0;
	if (theVertical == 0) {
		for (int aColumn = theFrom; aColumn <= theTo; aColumn++)
			aLevel->FloodPreview(aColumn * 2, theCellY * 2 - 1, PREVIEW_SIDE_A);
	} else {
		for (int aRow = theFrom; aRow <= theTo; aRow++)
			aLevel->FloodPreview(theCellX * 2 - 1, aRow * 2, PREVIEW_SIDE_A);
	}
	int aSideACells = aLevel->mFloodCount;

	// which sides would be cut
	bool aSideA = false;
	bool aSideB = false;
	aLevel->mPreviewSide2Valid = false;
	aLevel->mPreviewSide3Valid = false;
	if (aSideBCells > 0 && aSideACells > 0) {
		if (aSideBCells < aSideACells) {
			aSideB = true;
			aLevel->mPreviewSide3Valid = true;
		} else {
			aSideA = true;
			aLevel->mPreviewSide2Valid = true;
		}
		if (aSideBCells <= PREVIEW_SMALL_AREA && aSideACells <= PREVIEW_SMALL_AREA) {
			aSideB = true;
			aSideA = true;
			aLevel->mPreviewSide2Valid = true;
			aLevel->mPreviewSide3Valid = true;
		}
	}

	// with no bug alive the monkey may cut either side once the game is on its way
	Player* aPlayer = Player::sCurrent;
	bool aEitherSide = false;
	if (aPlayer->mTutorialDone != 0) {
		if (BugObj::sAlive < 1)
			aEitherSide = true;
	} else if (BugObj::sAlive < 1 && aPlayer->mSavedProgress > 0) {
		aEitherSide = true;
	}
	bool aShowSideB;
	if (aEitherSide) {
		// (the original leaves the flag of the second side as it was here; the valid bytes say both)
		aSideA = true;
		aLevel->mPreviewSide2Valid = true;
		aLevel->mPreviewSide3Valid = true;
		aShowSideB = true;
	} else if (aSideA) {
		aShowSideB = aSideB;
	} else if (aSideB) {
		aShowSideB = true;
	} else {
		ClearRopePreview();
		return;
	}
	if (aShowSideB) {
		aLevel->mPreviewShown = true;
		for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
			for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
				FillObj* aCell = gFillGrid[aColumn][aRow].mCell;
				if (aCell->mSide == PREVIEW_SIDE_B)
					aCell->Highlight(theCellX, theCellY);
				else
					aCell->Unhighlight();
			}
		}
		if (theVertical == 0) {
			for (int aColumn = theFrom; aColumn <= theTo; aColumn++)
				gFillGrid[aColumn][theCellY].mCell->Highlight(theCellX, theCellY);
		} else {
			for (int aRow = theFrom; aRow <= theTo; aRow++)
				gFillGrid[theCellX][aRow].mCell->Highlight(theCellX, theCellY);
		}
	}
	if (aSideA) {
		if (!aLevel->mPreviewSide3Valid)
			aLevel->mPreviewShown = false;
		for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
			for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
				FillObj* aCell = gFillGrid[aColumn][aRow].mCell;
				if (aCell->mSide == PREVIEW_SIDE_A)
					aCell->Highlight(theCellX, theCellY);
				else
					aCell->Unhighlight();
			}
		}
	}
}

// 0x43e3a0: the rope has reached both ends. A new wave of the cut starts, the pipes along the rope start over as walls,
// and the sides the preview found (the cells of the sides, then the rope's own line less its last place) are cut; the
// level waits on the cut's last cell
void LevelObj::CutGround(int theCellX, int theCellY, int theVertical, int theFrom, int theTo)
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	FillObj::StartWave();
	aLevel->StartWallPipes(theCellX, theCellY, theVertical, theFrom, theTo);
	if (aLevel->mPreviewSide3Valid) {
		for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
			for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
				FillObj* aCell = gFillGrid[aColumn][aRow].mCell;
				if (aCell->mSide == PREVIEW_SIDE_B)
					aCell->Cut(theCellX, theCellY);
			}
		}
		// (the rope's line is cut from theFrom up to but not with theTo, as the original does)
		if (theVertical == 0) {
			for (int aColumn = theFrom; aColumn < theTo; aColumn++)
				gFillGrid[aColumn][theCellY].mCell->Cut(theCellX, theCellY);
		} else {
			for (int aRow = theFrom; aRow < theTo; aRow++)
				gFillGrid[theCellX][aRow].mCell->Cut(theCellX, theCellY);
		}
	}
	if (aLevel->mPreviewSide2Valid) {
		for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
			for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
				FillObj* aCell = gFillGrid[aColumn][aRow].mCell;
				if (aCell->mSide == PREVIEW_SIDE_A)
					aCell->Cut(theCellX, theCellY);
			}
		}
	}
	if (aLevel->mPreviewSide2Valid || aLevel->mPreviewSide3Valid) {
		FillObj::AddBusy();
		FillObj::MarkFarthest();
	}
}
