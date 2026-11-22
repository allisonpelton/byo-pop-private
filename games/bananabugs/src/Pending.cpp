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

// The preloaded pictures, each in the global the original keeps it in; they move to their classes with them
static Anim* sSlotMachineAnim;       // 0x670210
static Anim* sSlotSymbolsAnim;       // 0x670214
static Anim* sSlotWinAnim;           // 0x670218
static Anim* sSlotSpinAnim;          // 0x67021c
static Anim* sSlotNumbersAnim;       // 0x670220
static Anim* sSlotShineAnim;         // 0x670224
Anim* Sexy::gMissileAnim = nullptr;           // 0x60e1ac
Anim* Sexy::gParrotFlyAnim = nullptr;         // 0x60e1b8
Anim* Sexy::gParrotWalkAnim = nullptr;        // 0x60e1bc
Anim* Sexy::gParrotPeckAnim = nullptr;        // 0x60e1c0
Anim* Sexy::gDiscoBallAnim = nullptr;         // 0x60e1c4
Anim* Sexy::gMilkDropAnim = nullptr;          // 0x60e1c8
Anim* Sexy::gDropAnim = nullptr;              // 0x60e1cc
Anim* Sexy::gTarzanAnim = nullptr;            // 0x60e1d0
Anim* Sexy::gPaperAnim = nullptr;             // 0x60e1d4
Anim* Sexy::gZapperAnim = nullptr;            // 0x60e1d8
Anim* Sexy::gSwatterAnim = nullptr;           // 0x60e1dc
Anim* Sexy::gMineAnim = nullptr;              // 0x60e1e0
Anim* Sexy::gZumaAnim = nullptr;              // 0x60e1e4
Anim* Sexy::gZumaBallAnim = nullptr;          // 0x60e1e8

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

void Sexy::PreloadSlots()
{
	sSlotMachineAnim = Load("images/slot/slot_machine", 0, 0, 1);
	sSlotSymbolsAnim = Load("images/slot/slot_symbols", 0xe0, 0xc0, 7);
	sSlotWinAnim = Load("images/slot/slot_win", 0x3c3, 0xc9, 2);
	sSlotSpinAnim = Load("images/slot/slot_spin", 0x11b, 0x11b, 4);
	sSlotNumbersAnim = Load("images/slot/slot_numbers", 0x3f, 0x49, 10);
	sSlotShineAnim = Load("images/slot/slot_shine", 0x100, 0xc0, 4);
}

// The effect of a bonus on the monkey shows nothing
void BonusEffect::Play()
{
}

// port: the stand-in bonus does nothing when it is won: it is put away at once (Bonus::Reset)
void PendingBonus::Start()
{
	Reset();
}

BonusShotSpeed::BonusShotSpeed() { mKind = BONUS_SHOT_SPEED; }
BonusFlyPaper::BonusFlyPaper() { mKind = BONUS_FLY_PAPER; }
BonusZapper::BonusZapper() { mKind = BONUS_ZAPPER; }
BonusSwat::BonusSwat() { mKind = BONUS_SWAT; }
BonusSpearCraft::BonusSpearCraft() { mKind = BONUS_SPEAR_CRAFT; }
BonusMagicArrows::BonusMagicArrows() { mKind = BONUS_MAGIC_ARROWS; }
BonusMissile::BonusMissile() { mKind = BONUS_MISSILE; }
BonusRain::BonusRain() { mKind = BONUS_RAIN; }
BonusHurricane::BonusHurricane() { mKind = BONUS_HURRICANE; }
BonusParrot::BonusParrot() { mKind = BONUS_PARROT; }
BonusTarzan::BonusTarzan() { mKind = BONUS_TARZAN; }
BonusZuma::BonusZuma() { mKind = BONUS_ZUMA; }
BonusMine::BonusMine() { mKind = BONUS_MINE; }
