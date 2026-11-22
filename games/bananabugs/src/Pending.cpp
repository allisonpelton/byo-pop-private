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
static Anim* sInGameButtonsAnim;     // 0x670188
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

void Sexy::PreloadInGameButtons()
{
	sInGameButtonsAnim = Load("images/menu/ingame_buttons", 0x167, 0x67, 3);
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

// port: the stand-in monkey shows nothing
MonkeyLives::MonkeyLives()
{
	mPhase = 0;
}

void MonkeyLives::Wake()
{
	if (!IsActive() && mPhase == 0)
		mPhase = 1;
}

void MonkeyLives::Dismiss()
{
	mPhase = 2;
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
