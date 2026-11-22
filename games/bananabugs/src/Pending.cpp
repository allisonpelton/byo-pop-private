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
static Anim* sMothDanceAnim;         // 0x60e294
static Anim* sMothAnim;              // 0x60e298
static Anim* sCentipedeHeadAnim;     // 0x60e280
static Anim* sCentipedeBodyAnim;     // 0x60e284
static Anim* sCentipedeTailAnim;     // 0x60e288
static Anim* sWaspAnim;              // 0x60e268
static Anim* sCaterpillarHeadAnim;   // 0x60e278
static Anim* sCaterpillarBodyAnim;   // 0x60e27c
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
static Anim* sRolyPolyAnim;          // 0x60e2a0
static Anim* sCockroachWalkAnim;     // 0x60e29c

static Anim* Load(const char* theName, int theCelWidth, int theCelHeight, int theNumCels, bool uniform = false)
{
	return Anim::Get(theName, ANIM_SCALE_SMOOTH, false, theCelWidth, theCelHeight, theNumCels, uniform);
}

// port: three one-frame tracks stand for the original's, the last of which (the end of the game) removes it
Slots::Slots(MetaGame* theMetaGame)
{
	(void)theMetaGame;
	for (int i = 0; i < 3; i++) {
		AnimKey* aKey = new AnimKey();
		aKey->SetDuration(1);
		aKey->SetRemoveAtEnd();
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aKey, aKey);
		AddTrack(aTrack, nullptr);
	}
}

void Slots::Start()
{
}

// port: the stand-in is done at once
bool Slots::IsDone()
{
	return true;
}

PathTrace::PathTrace()
{
	mCount = 0;
	for (int i = 0; i < 5008; i++)
		mX[i] = 0.0f;
	for (int i = 0; i < 15155; i++)
		mY[i] = 0.0f;
}

void PathTrace::Load(const char* theName)
{
	(void)theName;
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

void Sexy::PreloadMoth()
{
	sMothDanceAnim = Load("images/moth_dance", 0x60, 0x60, 0x1e);
	sMothAnim = Load("images/moth", 0x60, 0x60, 2);
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

void Sexy::PreloadRolyPoly()
{
	sRolyPolyAnim = Load("images/rolypoly", 0x60, 0x4f, 0x3c);
}

void Sexy::PreloadCockroach()
{
	sCockroachWalkAnim = Load("images/cockroach_walk", 0x60, 0x40, 0x1c);
}

// port: the stand-in bonuses, bugs and bolts do nothing
// port: the stand-in bonus is not drawn or kept anywhere (the original sets up its place and its looks)
void BonusCoin::Place(int theColumn, int theRow)
{
	(void)theColumn;
	(void)theRow;
}

void HiddenBonus::Uncover()
{
}

// The kinds of bug: stand-ins (see Pending.h). The kind each sets is BugKind; the moth sets the fly's number 7 in the
// original too (a bug of the original: nothing reads the difference in what is translated)
BugBrainDragonfly::BugBrainDragonfly() // 0x418880
{
	SetKind(2, 4, 2);
}

BugBrainRolly::BugBrainRolly() // 0x41d0d0
{
	SetKind(3, 1, 1);
}

BugBrainRoach::BugBrainRoach() // 0x41c4f0
{
	SetKind(5, 2, 2);
}

BugBrainCaterpillar::BugBrainCaterpillar() // 0x416ea0
{
	SetKind(6, 3, 3);
}

BugBrainFly::BugBrainFly() // 0x4198c0
{
	SetKind(7, 2, 2);
}

BugBrainCentipede::BugBrainCentipede() // 0x417840
{
	SetKind(8, 4, 4);
}

BugBrainMoth::BugBrainMoth() // 0x41af60
{
	SetKind(7, 2, 1);
}

BugBrainBee::BugBrainBee() // 0x414fa0
{
	SetKind(10, 2, 1);
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

// port: the stand-in walk-through shows no hints
void WalkThru::OnBugZapped()
{
}

// port: the stand-in bonuses have none running
void Bonus::OnAllBugsDead()
{
}
