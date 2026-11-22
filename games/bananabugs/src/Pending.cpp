// port: a temporary file; the pictures it loads are used once their classes are translated
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-but-set-global"
#endif
#include "Pending.h"

#include "Anim.h"

#include <cstdio>

using namespace Sexy;

Player* Player::sCurrent = nullptr;
bool PauseWin::sEnabled = false;

// The preloaded pictures, each in the global the original keeps it in; they move to their classes with them
static Anim* sNetFlyAnim;            // 0x60e508
static Anim* sBananaBunchAnim;       // 0x60e1a0
static Anim* sCoronaAnim;            // 0x60e1a4
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
static Anim* sMapPlayAnim;           // 0x60e64c
static Anim* sMapAnim;               // 0x60e650
static Anim* sMapDotAnim;            // 0x60e654
static Anim* sMapXAnim;              // 0x60e658
static Anim* sMapIconAnims[13];      // 0x60e65c
static Anim* sFireBallAnim;          // 0x60e50c
static Anim* sPipesAnim;             // 0x670194
static Anim* sRopeArrowAnims[4];     // 0x6701f4
static Anim* sRopePartAnim;          // 0x670204
static Anim* sArrowAuraAnim;         // 0x670208
static Anim* sRopeSnapAnim;          // 0x67020c
static Anim* sCockroachWalkAnim;     // 0x60e29c

static Anim* Load(const char* theName, int theCelWidth, int theCelHeight, int theNumCels, bool uniform = false)
{
	return Anim::Get(theName, ANIM_SCALE_SMOOTH, false, theCelWidth, theCelHeight, theNumCels, uniform);
}

// a track that holds, for StartLeaving
static int AddHoldTrack(ObjBase* theObj)
{
	AnimKey* aKey = new AnimKey();
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey, aKey);
	return theObj->AddTrack(aTrack, nullptr);
}

Board::Board()
{
	SetParent(nullptr, nullptr);
	mLevel = new LevelObj();
	mLeaveTrack = AddHoldTrack(this);
	Activate(this, false);
}

MainMenu::MainMenu()
{
	SetParent(nullptr, nullptr);
	mChoice = MENU_CHOICE_NONE;
	mLeaveTrack = AddHoldTrack(this);
	Activate(this, false);
}

MainMenu::~MainMenu()
{
}

Board::~Board()
{
	delete mLevel;
}

MetaGame::MetaGame()
{
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

Player::Player()
{
	mMetaGameStarted = false;
}

Player::~Player()
{
}

// port: the stand-in has no board to click
void Player::BoardClick(int theButton)
{
	(void)theButton;
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

// port: the stand-ins of GameApp's globals do nothing but make one player
void Sexy::InitHighScores()
{
}

void Sexy::LoadProfiles()
{
	Player::sCurrent = new Player();
}

void Sexy::SaveProfiles()
{
}

void Sexy::PreloadNetFly()
{
	sNetFlyAnim = Load("images/netfly", 0x21, 0x21, 2);
}

void Sexy::PreloadBananas()
{
	sBananaBunchAnim = Load("images/bananabunch01", 0, 0, 1);
	sCoronaAnim = Load("images/corona1", 0, 0, 1);
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

void Sexy::PreloadMap()
{
	sMapPlayAnim = Load("images/map/map_play", 0, 0, 1);
	sMapDotAnim = Load("images/map/map_dot", 0x23, 0x23, 3);
	sMapAnim = Load("images/map/map", 0, 0, 1);
	sMapXAnim = Load("images/map/map_X", 0, 0, 1);
	for (int i = 0; i < 13; i++) {
		char aName[32];
		snprintf(aName, sizeof(aName), "images/map/map_icon%02d", i + 1);
		sMapIconAnims[i] = Load(aName, 0, 0, 1);
	}
}

void Sexy::PreloadFireBall()
{
	sFireBallAnim = Load("images/fireball", 0, 0, 1);
}

void Sexy::PreloadPipes()
{
	sPipesAnim = Load("images/pipes01", 0x21, 0x21, 0x10);
}

void Sexy::PreloadRope()
{
	sRopeArrowAnims[0] = Load("images/rope_arrow_0", 0x40, 0x40, 0x10);
	sRopeArrowAnims[1] = Load("images/rope_arrow_1", 0x40, 0x40, 0x10);
	sRopeArrowAnims[2] = Load("images/rope_arrow_2", 0x40, 0x40, 0x10);
	sRopeArrowAnims[3] = Load("images/rope_arrow_3", 0x40, 0x40, 0x10);
	sArrowAuraAnim = Load("images/arrow_aura", 0, 0, 1);
	sRopePartAnim = Load("images/rope_part", 0x19, 0x19, 2);
	sRopeSnapAnim = Load("images/rope_snap", 0, 0, 1);
}

// port: the levels' table is not translated yet
void Sexy::InitLevelTable()
{
}

void Sexy::PreloadCockroach()
{
	sCockroachWalkAnim = Load("images/cockroach_walk", 0x60, 0x40, 0x1c);
}

// port: the stand-in has no level taking the mouse
bool Sexy::LevelAcceptsMouse()
{
	return true;
}
