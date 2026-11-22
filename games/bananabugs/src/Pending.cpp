// port: a temporary file; the pictures it loads are used once their classes are translated
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-but-set-global"
#endif
#include "Pending.h"

#include "Anim.h"
#include "LevelObj.h"
#include "Player.h"

#include <cstdio>

using namespace Sexy;

float Sexy::gBugBoxX = 0.0f;
float Sexy::gBugBoxY = 0.0f;
float Sexy::gBugBoxWidth = 0.0f;
float Sexy::gBugBoxHeight = 0.0f;
bool PauseWin::sEnabled = false;
bool MetaGame::sFirstVisit = false;

// The preloaded pictures, each in the global the original keeps it in; they move to their classes with them
static Anim* sWalkThruTextAnims[9];  // 0x67022c
static Anim* sSlotMachineAnim;       // 0x670210
static Anim* sSlotSymbolsAnim;       // 0x670214
static Anim* sSlotWinAnim;           // 0x670218
static Anim* sSlotSpinAnim;          // 0x67021c
static Anim* sSlotNumbersAnim;       // 0x670220
static Anim* sSlotShineAnim;         // 0x670224
static Anim* sFlyDanceAnim;          // 0x60e28c
static Anim* sFlyFlyAnim;            // 0x60e290
static Anim* sInGameButtonsAnim;     // 0x670188
static Anim* sAntWalkAnim;           // 0x60e25c
static Anim* sAntTurnAnim;           // 0x60e260
static Anim* sAntDanceAnim;          // 0x60e264
static Anim* sMothDanceAnim;         // 0x60e294
static Anim* sMothAnim;              // 0x60e298
static Anim* sBeetleWalkAnim;        // 0x60e26c
static Anim* sBeetleTurnAnim;        // 0x60e270
static Anim* sBeetleDanceAnim;       // 0x60e274
static Anim* sCentipedeHeadAnim;     // 0x60e280
static Anim* sCentipedeBodyAnim;     // 0x60e284
static Anim* sCentipedeTailAnim;     // 0x60e288
static Anim* sWaspAnim;              // 0x60e268
static Anim* sCaterpillarHeadAnim;   // 0x60e278
static Anim* sCaterpillarBodyAnim;   // 0x60e27c
static Anim* sSkeeterFlyAnim;        // 0x60e2a4
static Anim* sSkeeterDanceAnim;      // 0x60e2a8
static Anim* sCocoBombAnim;          // 0x60e1a8
static Anim* sMissileAnim;           // 0x60e1ac
static Anim* sCannonAnim;            // 0x60e1b0
static Anim* sCannonBallAnim;        // 0x60e1b4
static Anim* sParrotFlyAnim;         // 0x60e1b8
static Anim* sParrotWalkAnim;        // 0x60e1bc
static Anim* sParrotPeckAnim;        // 0x60e1c0
static Anim* sDiscoBallAnim;         // 0x60e1c4
static Anim* sMilkDropAnim;          // 0x60e1c8
static Anim* sDropAnim;              // 0x60e1cc
static Anim* sTarzanAnim;            // 0x60e1d0
static Anim* sPaperAnim;             // 0x60e1d4
static Anim* sZapperAnim;            // 0x60e1d8
static Anim* sSwatterAnim;           // 0x60e1dc
static Anim* sMineAnim;              // 0x60e1e0
static Anim* sZumaAnim;              // 0x60e1e4
static Anim* sZumaBallAnim;          // 0x60e1e8
static Anim* sSpinStarsAnim;         // 0x60e254
static Anim* sMonkeyShockRingAnim;   // 0x60e258
static Anim* sRolyPolyAnim;          // 0x60e2a0
static Anim* sPipesAnim;             // 0x670194
static Anim* sCockroachWalkAnim;     // 0x60e29c

static Anim* Load(const char* theName, int theCelWidth, int theCelHeight, int theNumCels, bool uniform = false)
{
	return Anim::Get(theName, ANIM_SCALE_SMOOTH, false, theCelWidth, theCelHeight, theNumCels, uniform);
}

MetaGame::MetaGame()
{
	sFirstVisit = true;
	SetParent(nullptr, nullptr);
	mWantsMenu = false;
	mLeaving = false;
	Activate(this, false);
}

MetaGame::~MetaGame()
{
}

// port: the stand-in never chooses a level
bool MetaGame::TakeLevelChosen()
{
	return false;
}

// port: the stand-in never pauses
void PauseWin::PauseForFocusLoss()
{
}

bool PauseWin::IsBlocking()
{
	return false;
}

void PauseWin::ResumeOnRelease()
{
}

void Sexy::PreloadWalkThru()
{
	for (int i = 0; i < 9; i++) {
		char aName[32];
		snprintf(aName, sizeof(aName), "images/walkthru/text%02d", i + 1);
		sWalkThruTextAnims[i] = Load(aName, 0, 0, 1);
	}
}

void Sexy::PreloadSlots()
{
	sSlotMachineAnim = Load("images/slot/slot_machine", 0, 0, 1);
	sSlotSymbolsAnim = Load("images/slot/slot_symbols", 0xe0, 0xc0, 7);
	sSlotWinAnim = Load("images/slot/slot_win", 0x3c3, 0xc9, 2);
	sSlotSpinAnim = Load("images/slot/slot_spin", 0x11b, 0x11b, 4);
	sSlotNumbersAnim = Load("images/slot/slot_numbers", 0x3f, 0x49, 10);
	sSlotShineAnim = Load("images/slot/slot_shine", 0x100, 0xc0, 4);
}

void Sexy::PreloadFly()
{
	sFlyDanceAnim = Load("images/fly_dance", 0x60, 0x60, 0x1e);
	sFlyFlyAnim = Load("images/fly_fly", 0x40, 0x40, 6);
}

void Sexy::PreloadInGameButtons()
{
	sInGameButtonsAnim = Load("images/menu/ingame_buttons", 0x167, 0x67, 3);
}

void Sexy::PreloadAnt()
{
	sAntWalkAnim = Load("images/ant_walk", 0x60, 0x49, 0x1e);
	sAntTurnAnim = Load("images/ant_turn", 0x60, 0x49, 0x1e);
	sAntDanceAnim = Load("images/ant_dance", 0x60, 0x60, 0x1e);
}

void Sexy::PreloadMoth()
{
	sMothDanceAnim = Load("images/moth_dance", 0x60, 0x60, 0x1e);
	sMothAnim = Load("images/moth", 0x60, 0x60, 2);
}

void Sexy::PreloadBeetle()
{
	sBeetleWalkAnim = Load("images/beetle_walk", 0x60, 0x4f, 0x1e);
	sBeetleTurnAnim = Load("images/beetle_turn", 0x60, 0x57, 0x1e);
	sBeetleDanceAnim = Load("images/beetle_dance", 0x60, 0x60, 0x1e);
}

void Sexy::PreloadCentipede()
{
	sCentipedeHeadAnim = Load("images/centipede_head", 0x41, 0x3f, 0x5a);
	sCentipedeBodyAnim = Load("images/centipede_body", 0x2b, 0x31, 0x5a);
	sCentipedeTailAnim = Load("images/centipede_tail", 0x25, 0x21, 0x5a);
}

void Sexy::PreloadWasp()
{
	sWaspAnim = Load("images/wasp", 0x5f, 0x5f, 0x1e);
}

void Sexy::PreloadCaterpillar()
{
	sCaterpillarHeadAnim = Load("images/caterpillar_head", 0x24, 0x2d, 0x1e);
	sCaterpillarBodyAnim = Load("images/caterpillar_body", 0x24, 0x24, 0x1e);
}

// port: the flies' paths (two "images/path_fly" tables) are not translated yet
void Sexy::PreloadFlyPaths()
{
}

// port: the moths' paths (two "images/path_moth" tables) are not translated yet
void Sexy::PreloadMothPaths()
{
}

void Sexy::PreloadSkeeter()
{
	sSkeeterFlyAnim = Load("images/skeeter_fly", 0x60, 0x60, 0x1e);
	sSkeeterDanceAnim = Load("images/skeeter_dance", 0x60, 0x60, 0x1e);
}

void Sexy::PreloadBonusPictures()
{
	sCocoBombAnim = Load("images/cocobomb", 0, 0, 1, true);
	sMissileAnim = Load("images/missile", 0, 0, 1);
	sCannonBallAnim = Load("images/cannonball", 0, 0, 1, true);
	sCannonAnim = Load("images/cannon", 0, 0, 1, true);
	sParrotFlyAnim = Load("images/parrot_fly", 0x60, 0x60, 0xb);
	sParrotWalkAnim = Load("images/parrot_walk", 0x60, 0x60, 0x14);
	sParrotPeckAnim = Load("images/parrot_peck", 0x60, 0x60, 0x11);
	sDiscoBallAnim = Load("images/bonus_discoball", 0x60, 0x60, 6, true);
	sMilkDropAnim = Load("images/bonus_milkdrop", 0, 0, 1);
	sDropAnim = Load("images/drop", 0, 0, 1);
	sTarzanAnim = Load("images/bonus_tarzan", 0, 0, 1);
	sPaperAnim = Load("images/bonus_paper", 0, 0, 1);
	sZapperAnim = Load("images/bonus_zapper", 0x3f, 0x55, 2);
	sSwatterAnim = Load("images/bonus_swatter", 0, 0, 1);
	sMineAnim = Load("images/bonus_mine", 0, 0, 1, true);
	sZumaAnim = Load("images/bonus_zuma", 0x3f, 0x4b, 2);
	sZumaBallAnim = Load("images/bonus_zuma_ball", 0, 0, 1, true);
}

// port: the bonuses' table (the "images/bonuses" picture and each bonus's kind) is not translated yet
void Sexy::InitBonusTable()
{
}

void Sexy::PreloadSpinStars()
{
	sSpinStarsAnim = Load("images/spinstars", 0x3c, 0x1c, 0x18);
	sMonkeyShockRingAnim = Load("images/monkey_shock_ring", 0, 0, 1);
}

void Sexy::PreloadRolyPoly()
{
	sRolyPolyAnim = Load("images/rolypoly", 0x60, 0x4f, 0x3c);
}

void Sexy::PreloadPipes()
{
	sPipesAnim = Load("images/pipes01", 0x21, 0x21, 0x10);
}

void Sexy::PreloadCockroach()
{
	sCockroachWalkAnim = Load("images/cockroach_walk", 0x60, 0x40, 0x1c);
}

// port: the stand-in level has no bugs, ground or hints
bool BugObj::AnyNear(float theX, float theY, float theReach)
{
	(void)theX;
	(void)theY;
	(void)theReach;
	return false;
}

int BugObj::sCount = 0;
int BugObj::sAlive = 0;

void BugObj::OnShot()
{
}

void BugObj::Reset()
{
}

void BugObj::Shuffle()
{
}

void BugObj::KillAll()
{
}

void BugObj::ResetCounters()
{
}

void BugObj::SortList()
{
}

GridBug* BugObj::Get(int theIndex)
{
	(void)theIndex;
	return nullptr;
}

bool BugObj::Overlaps(int theColumn, int theRow, GridBug* theBug)
{
	(void)theColumn;
	(void)theRow;
	(void)theBug;
	return false;
}

void BugObj::AlertAlongRope(PipeObj* theStart, PipeObj* theEnd, int theVertical)
{
	(void)theStart;
	(void)theEnd;
	(void)theVertical;
}

// port: the stand-in pipes, bonuses, bugs and bolts do nothing
void PipeObj::SetShiftTarget(float theX, float theY)
{
	(void)theX;
	(void)theY;
}

void PipeObj::StartShift()
{
}

bool PipeObj::sSoundPlayed = false;

void PipeObj::SetKind(int theKind)
{
	mKind = theKind;
}

bool PipeObj::IsSettledA()
{
	return true;
}

bool PipeObj::IsSettledB()
{
	return true;
}

void PipeObj::Refresh()
{
}

void PipeObj::Retire()
{
}

void PipeObj::Rebuild()
{
}

void PipeObj::RandomizeDuration()
{
}

void PipeObj::BuildFlames()
{
}

// port: the stand-in pipe is an object of the right place that shows nothing
PipeObj* PipeObj::Create(int theColumn, int theRow)
{
	PipeObj* aPipe = new PipeObj();
	aPipe->SetParent(nullptr, nullptr);
	aPipe->mKind = 0;
	aPipe->mVariant = 0;
	aPipe->mColumn = theColumn;
	aPipe->mRow = theRow;
	aPipe->mUpLink = nullptr;
	aPipe->mRightLink = nullptr;
	aPipe->mDownLink = nullptr;
	aPipe->mLeftLink = nullptr;
	// the original's pipe has a track of two keys that grow it in (the level's cut sets them up and starts it)
	aPipe->mAppearKey = new AnimKey();
	aPipe->mSettleKey = new AnimKey();
	aPipe->mAppearTrack = new AnimTrack();
	aPipe->mAppearTrack->AddKey(aPipe->mAppearKey, aPipe->mSettleKey);
	aPipe->mAppearTrack->AddKey(aPipe->mSettleKey, aPipe->mSettleKey);
	aPipe->mAppearTrackIndex = aPipe->AddTrack(aPipe->mAppearTrack, nullptr);
	return aPipe;
}

// port: the stand-in bonus is not drawn or kept anywhere (the original sets up its place and its looks)
void BonusCoin::Place(int theColumn, int theRow)
{
	(void)theColumn;
	(void)theRow;
}

void HiddenBonus::Uncover()
{
}

// port: the stand-in bug is an object that shows nothing; the original's BugObj constructor (0x425150) also puts the bug in
// the list (sCount) and counts it alive (sAlive); the stand-in does neither, so no level waits on one
GridBug::GridBug(int theKind)
{
	mPlaced = false;
	mKind = theKind;
	mGroupSize = 0;
	mSegmentSteps = 0;
	mIsGroupLeader = true;
	mColumn = 0;
	mRow = 0;
	mNextInCell = nullptr;
	mPrevInCell = nullptr;
	mHalfWidth = 0;
	mHalfHeight = 0;
	SetParent(nullptr, nullptr);
}

void GridBug::Setup(int theRole, int theWaitFrames, GridBug* theHead, GridBug* thePrevious)
{
	(void)theRole;
	(void)theWaitFrames;
	(void)theHead;
	(void)thePrevious;
}

void GridBug::StepSegment()
{
}

// port: the original also sets the object's position to the place times 31 (0x100 and 0x104)
void GridBug::PlaceAt(int theColumn, int theRow)
{
	mColumn = theColumn;
	mRow = theRow;
	mPlaced = true;
}

void GridBug::Zap(int theFirst, int theSecond, int theThird)
{
	(void)theFirst;
	(void)theSecond;
	(void)theThird;
}

BoltObj::BoltObj(int theSegments)
{
	(void)theSegments;
}

void BoltObj::Stop()
{
}

void BoltObj::SetEnds(int theFromX, int theFromY, int theToX, int theToY)
{
	(void)theFromX;
	(void)theFromY;
	(void)theToX;
	(void)theToY;
}

void BoltObj::UpdateBolt()
{
}

void WalkThru::OnShoot()
{
}

void WalkThru::OnLoseLife()
{
}

void WalkThru::OnTurn()
{
}

void WalkThru::OnAreaFilled()
{
}

void WalkThru::Dismiss()
{
}

void WalkThru::ShowFirstStep()
{
}

void WalkThru::ShowSecondStep()
{
}

void WalkThru::ShowThirdStep()
{
}

// port: the stand-in has no bonus running; the original also ends what the running bonus changed, and lets go of what
// the monkey holds
void Bonus::Cancel()
{
	Player::Drop();
}

void Bonus::CancelIfDone()
{
}

void Bonus::ClearAll()
{
	Player::Drop();
}

// port: the stand-in board shows nothing
void ScoreBoard::ShowScore(int theScore)
{
	(void)theScore;
}

void ScoreBoard::ShowPercent(int thePercent)
{
	(void)thePercent;
}

void ScoreBoard::ShowBananaIcon()
{
}

void ScoreBoard::MarkLastIcon()
{
}

void ScoreBoard::ActivateHud()
{
}

void ScoreBoard::DeactivateHud()
{
}

void ScoreBoard::LaunchFireBall(int theIndex)
{
	(void)theIndex;
}

void ScoreBoard::ShowLevel(int theLevel)
{
	(void)theLevel;
}

void ScoreBoard::Show()
{
}

void ScoreBoard::ClearBananaIcons()
{
}

bool ScoreBoard::IsDone()
{
	return true;
}

// port: the stand-ins of what a level makes. The intro is always ready and finished, the in-level menu resumes at
// once, the stats are ready and the victory is done
bool Intro::IsReady()
{
	return true;
}

bool Intro::IsFinished()
{
	return true;
}

void Intro::Finish()
{
	Delete();
}

int InGameMenu::GetResult()
{
	return 2;
}

void InGameMenu::Close()
{
	Delete();
}

Stats::Stats(LevelObj* theLevel, int theFirst, int theSecond)
{
	(void)theLevel;
	(void)theFirst;
	(void)theSecond;
}

bool Stats::IsReady()
{
	return true;
}

void Stats::Continue()
{
}

void Victory::Show()
{
}

bool Victory::IsDone()
{
	return true;
}

void HeldObj::Rotate()
{
}

bool HeldObj::Place()
{
	return false;
}


GridBug* BugObj::PickPeaTarget()
{
	return nullptr;
}

void GridBug::MarkPeaHit()
{
}
