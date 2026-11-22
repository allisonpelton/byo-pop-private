#include "Slots.h"

#include "Anim.h"
#include "Bonus.h"
#include "BonusCoin.h"
#include "Compat2004.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "MetaGame.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"
#include "TextFade.h"

using namespace Sexy;

Anim* Slots::sMachineAnim = nullptr;
Anim* Slots::sSymbolsAnim = nullptr;
Anim* Slots::sWinAnim = nullptr;
Anim* Slots::sSpinAnim = nullptr;
Anim* Slots::sNumbersAnim = nullptr;
Anim* Slots::sShineAnim = nullptr;
int Slots::sReelStrip[SLOTS_STRIP_ROWS][SLOTS_REELS];

// How many of each symbol each reel's strip holds (0x600010: a row for each symbol, a column for each reel). Each
// column adds up to the 32 places of a strip.
static const int STRIP_COUNTS[SLOTS_SYMBOLS][SLOTS_REELS] = {
	{ 1, 10, 18 },   // the 100 bananas: rare on the first reel
	{ 0, 5, 5 },     // flypaper
	{ 3, 4, 0 },     // zapper
	{ 4, 0, 6 },     // swatter
	{ 2, 7, 0 },     // swinging monkeys
	{ 16, 1, 3 },    // the jackpot: common on the first reel, rare on the others
	{ 6, 5, 0 },     // exploding mine
};

// The key events of the machine's own tracks (AnimKey::mEventId)
enum SlotsEvent
{
	SLOTS_EVENT_PULSED = 1,          // the pulse that repeats while the prize is counted in has ended
	SLOTS_EVENT_GREW = 2,            // the machine has swelled into place
};

// The tracks of the machine and its objects, numbered in the order their builders add them
enum SlotsTracks
{
	SLOTS_TRACK_GROW = 0,            // the machine swells in and holds (the first track runs at once; MetaGame starts none of them)
	SLOTS_TRACK_PULSE = 1,           // a pulse of 10 frames, repeated from Notify while the prize is counted
	SLOTS_TRACK_LEAVE = 2,           // shrinks away and takes the machine with it (MetaGame starts it)
	DONE_TRACK_HOVER = 1,            // the done button
	DONE_TRACK_UNHOVER = 2,
	DONE_TRACK_HIDE = 3,             // while the reels turn
	DONE_TRACK_SHOW = 4,
	SPIN_TRACK_HOVER = 1,            // the lever and its glow
	SPIN_TRACK_UNHOVER = 2,
	RING_TRACK_TURN = 0,             // the ring turns for 999 frames, while the reels do
	RING_TRACK_PULSE = 1,            // swells and shrinks again for ever
	GLOW_TRACK_BREATHE = 0,          // swells and shrinks for ever, started by the builder
	BUBBLE_TRACK_FLY = 0,
	BANNER_TRACK_SHOW = 0,
};

const int PAID_SPIN_DRAWS = 61;   // a paid spin draws from 0 to 60 ...
const int FREE_SPIN_DRAWS = 36;           // ... a free one from 0 to 35 (always a category with a prize)
const int ROLL_SPECIAL = 40;              // the draw that gives the 100 bananas or the jackpot
const int JACKPOT_AFTER_SPINS = 14;       // the special roll is the jackpot once more spins than this have passed since a big win
const int REEL_BASE_SPEED = 24;           // a reel turns this many pixels a frame, and one more for each reel after the first
const int STRIP_LENGTH = SLOTS_SYMBOLS * SLOTS_CELL_HEIGHT;
const int REEL_FIRST_TURNS = 3;           // whole turns the first reel makes before it looks for its place; two more for each next
const int REEL_PLACE_AT_START = 0x2a0;    // where the reels stand as they are made: a reel further on for each reel
const int PAYOUT_STEP = 20;               // the win sound plays once for each this many bananas, and the lights follow
const int WIN_SOUND_INTERVAL = 40;        // frames between plays of the win sound
const int LEAVE_DELAY = 10;               // frames after the done button
const int SEED_OFFSET = 0xd6f1;           // added to the clock to seed the C library's rand as the machine starts
const int GAME_SEED_RANGE = 0x40001;      // the game's own generator is seeded with one of its draws, taken modulo this

// The colour and size of the lines the machine says (TextFade::Start's arguments)
const float MESSAGE_SCALE = 2.0f;
const int MESSAGE_DELAY = 5;
const float WIN_RED = 0.28f;
const float WIN_GREEN = 1.0f;
const float WIN_BLUE = 0.2f;

// 0x4687e0: the two lines of text (the second never shows), the done button and the bubble of an unlocked bonus, and the
// reels' strips: each reel's column holds the symbols in the numbers the table gives, then shuffled three times by the C
// library's rand. Two lists of 28 numbers are shuffled and never read; each shuffle draws numbers, so they stay. The
// rest (the machine itself, the reels, the lever) is made by Start.
Slots::Slots(MetaGame* theMetaGame)
{
	mMessage = new TextFade();
	mMessage->SetParent(this, nullptr);
	mSpareMessage = new TextFade();
	mSpareMessage->SetParent(this, nullptr);
	mMetaGame = theMetaGame;
	mWinSoundsLeft = 0;
	mWinSoundId = 0;
	mWinSoundInterval = 0;
	mWinSoundTimer = 0;
	mSpinsSinceBigWin = 0;
	mWon = 0;
	mUnused2b4 = 0;
	mDone = false;
	mSpinRing = nullptr;
	mState = SLOTS_IDLE;
	mFreeSpin = false;
	mPayout = 0;                // port: the original reads it only after ScoreRoll has set it
	mSpinAfterSounds = 0;       // port: likewise, StartWinSounds sets it before it is read
	mLeaveDelay = 0;            // port: likewise (the done button sets it)
	for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
		mPosition[aReel] = 0;
		mSpinsLeft[aReel] = 0;
		mResult[aReel] = 0;
		mStopped[aReel] = 1;
		mTarget[aReel] = 0;
	}
	for (int aSymbol = 0; aSymbol < SLOTS_SYMBOLS; aSymbol++)
		mRolled[aSymbol] = false;
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, 200);
	mStripLength = STRIP_LENGTH;
	BuildDoneButton();
	BuildUnlockBubble();

	int aFilled[SLOTS_REELS] = { 0, 0, 0 };
	for (int aSymbol = 0; aSymbol < SLOTS_SYMBOLS; aSymbol++) {
		for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
			int aCount = STRIP_COUNTS[aSymbol][aReel];
			if (0 < aCount) {
				for (int i = 0; i < aCount; i++)
					sReelStrip[aFilled[aReel] + i][aReel] = aSymbol;
				aFilled[aReel] += aCount;
			}
		}
	}
	for (int aPass = 0; aPass < 3; aPass++) {
		for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
			for (int aRow = 0; aRow < SLOTS_STRIP_ROWS; aRow++) {
				int aPick = CrtRand() % SLOTS_STRIP_ROWS;
				int aTemp = sReelStrip[aRow][aReel];
				sReelStrip[aRow][aReel] = sReelStrip[aPick][aReel];
				sReelStrip[aPick][aReel] = aTemp;
			}
		}
	}
	// (the original then adds up each column, and nothing reads the sums)
	ShuffleNumbers(mUnusedOrder[0], SLOTS_SHUFFLED);
	ShuffleNumbers(mUnusedOrder[1], SLOTS_SHUFFLED);
	mFreeSpin = Player::TakeUpgradeBought() != 0;
}

// 0x468b10: the machine, the reels and the lever are built, the machine shown, and the generators seeded: the C
// library's by the clock, the game's own by one of its own draws
void Slots::Start()
{
	BuildCabinet();
	BuildReels();
	BuildSpinButton();
	BuildSpinRing();
	BuildSpinGlow();
	BuildWinBanner();
	Activate(this, false);
	ShowBananas();
	// the original reads timeGetTime(); GameTickCount is the same clock, and the one a scripted run repeats
	CrtSRand(GameTickCount() + SEED_OFFSET);
	GameRandomSeed(GameRandomNext() % GAME_SEED_RANGE);
}

// 0x468b80: event 1, a pulse of the machine ended: while the prize is counted in, it pulses again; event 2, the machine
// has swelled in: a free spin is said
void Slots::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr) {
		int anEvent = theObj->mCurrentKey->mEventId;
		if (anEvent == SLOTS_EVENT_PULSED) {
			if (mState == SLOTS_PAYING)
				StartTrack(SLOTS_TRACK_PULSE);
		} else if (anEvent == SLOTS_EVENT_GREW) {
			if (mFreeSpin)
				ShowMessage("YOU HAVE ONE FREE SPIN!", WIN_RED, WIN_GREEN, WIN_BLUE);
		}
	}
}

// 0x468c10: the win sound repeats while there are plays left. Then by state: idle, the lever and done button swell
// when the mouse is on them; rolling, each reel turns, stops on its place, and once all have the roll is scored and its
// prize shown; paying, a banana a frame is counted in; leaving, the machine waits ten frames and is done.
// The original counts every update in a global (0x6a5594) that nothing reads, and keeps the reel number of the loop in
// another (0x6a5598).
bool Slots::Update()
{
	if (!ObjBase::Update())
		return false;
	if (0 < mWinSoundsLeft) {
		if (mWinSoundTimer < 1) {
			mWinSoundsLeft--;
			if (mWinSoundsLeft == 0 && mSpinAfterSounds != 0)
				Spin();
			else
				gApp->mSoundQueue->Request(mWinSoundId, 0, false);
			mWinSoundTimer = mWinSoundInterval;
		}
		mWinSoundTimer--;
	}
	if (mState == SLOTS_ROLLING) {
		for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
			PlaceReel(mPosition[aReel], aReel);
			if (0 < mSpinsLeft[aReel]) {
				mPosition[aReel] += aReel + REEL_BASE_SPEED;
				if (mStripLength <= mPosition[aReel]) {
					mPosition[aReel] -= mStripLength;
					mSpinsLeft[aReel]--;
				}
				continue;
			}
			if (mStopped[aReel] != 0)
				continue;
			mPosition[aReel] += aReel + REEL_BASE_SPEED;
			if (mPosition[aReel] < mTarget[aReel])
				continue;
			mPosition[aReel] = mTarget[aReel];
			mStopped[aReel] = 1;
			gApp->mSoundQueue->Request(SOUND_SLOTROLLSTOP, 0, false);
			PlaceReel(mPosition[aReel], aReel);
			if (mStopped[0] == 0 || mStopped[1] == 0 || mStopped[2] == 0)
				continue;

			// the last reel has stopped: the done button comes back, the roll is scored and what it won is shown
			Activate(mDoneButton, false);
			mDoneButton->StartTrack(DONE_TRACK_SHOW);
			gApp->mSoundQueue->StopLoop(SOUND_SLOTROLL);
			mSpinRing->StartTrack(RING_TRACK_PULSE);
			int aSymbol;
			int aResult = ScoreRoll(&mPayout, &aSymbol);
			switch (aSymbol) {
			case SLOT_SYMBOL_BANANAS_100:
				ShowMessage("YOU WON 100 BANANAS", WIN_RED, WIN_GREEN, WIN_BLUE);
				mSpinsSinceBigWin = 0;
				break;
			case SLOT_SYMBOL_FLY_PAPER:
				Player::sCurrent->Unblock(BONUS_FLY_PAPER);
				ShowMessage("FLYPAPER UNLOCKED", WIN_RED, WIN_GREEN, WIN_BLUE);
				Activate(mUnlockBubble, false);
				mUnlockBubble->StartTrack(BUBBLE_TRACK_FLY);
				mUnlockBubble->SetStateInt(STATE_FRAME, BONUS_FLY_PAPER);
				break;
			case SLOT_SYMBOL_ZAPPER:
				Player::sCurrent->Unblock(BONUS_ZAPPER);
				ShowMessage("ZAPPER UNLOCKED", WIN_RED, WIN_GREEN, WIN_BLUE);
				Activate(mUnlockBubble, false);
				mUnlockBubble->StartTrack(BUBBLE_TRACK_FLY);
				mUnlockBubble->SetStateInt(STATE_FRAME, BONUS_ZAPPER);
				break;
			case SLOT_SYMBOL_SWAT:
				Player::sCurrent->Unblock(BONUS_SWAT);
				ShowMessage("SWATTER UNLOCKED", WIN_RED, WIN_GREEN, WIN_BLUE);
				Activate(mUnlockBubble, false);
				mUnlockBubble->StartTrack(BUBBLE_TRACK_FLY);
				mUnlockBubble->SetStateInt(STATE_FRAME, BONUS_SWAT);
				break;
			case SLOT_SYMBOL_TARZAN:
				Player::sCurrent->Unblock(BONUS_TARZAN);
				ShowMessage("SWINGING MONKEYS UNLOCKED", WIN_RED, WIN_GREEN, WIN_BLUE);
				Activate(mUnlockBubble, false);
				mUnlockBubble->StartTrack(BUBBLE_TRACK_FLY);
				mUnlockBubble->SetStateInt(STATE_FRAME, BONUS_TARZAN);
				break;
			case SLOT_SYMBOL_JACKPOT:
				ShowMessage("YOU WON 999 BANANAS", WIN_RED, WIN_GREEN, WIN_BLUE);
				mSpinsSinceBigWin = 0;
				break;
			case SLOT_SYMBOL_MINE:
				Player::sCurrent->Unblock(BONUS_MINE);
				ShowMessage("EXPLODING MINE UNLOCKED", WIN_RED, WIN_GREEN, WIN_BLUE);
				Activate(mUnlockBubble, false);
				mUnlockBubble->StartTrack(BUBBLE_TRACK_FLY);
				mUnlockBubble->SetStateInt(STATE_FRAME, BONUS_MINE);
				break;
			default:
				break;
			}
			if (aResult == SLOT_RESULT_NOTHING) {
				mState = SLOTS_IDLE;
			} else {
				if (aResult == SLOT_RESULT_UNLOCK) {
					gApp->mSoundQueue->Request(SOUND_SLOTWIN, 0, false);
				} else if (aResult == SLOT_RESULT_BANANAS) {
					mWinBanner->SetStateInt(STATE_FRAME, 0);
					StartWinSounds(mPayout / PAYOUT_STEP + 1);
					mWon = 0;
				} else if (aResult == SLOT_RESULT_JACKPOT) {
					mWinBanner->SetStateInt(STATE_FRAME, 1);
					gApp->mSoundQueue->Request(SOUND_SLOTWINBIG, 0, true);
					mWon = 0;
				}
				mWinBanner->StartTrack(BANNER_TRACK_SHOW);
				Activate(mWinBanner, false);
				mState = SLOTS_PAYING;
				StartTrack(SLOTS_TRACK_PULSE);
			}
			ShowBananas();
		}
		return true;
	}
	if (mState == SLOTS_PAYING) {
		UpdateShines();
		if (0 < mPayout) {
			mWon++;
			mPayout--;
			Player::sCurrent->mBananas++;
			ShowBananas();
		} else {
			mState = SLOTS_IDLE;
			gApp->mSoundQueue->StopLoop(SOUND_SLOTWINBIG);
		}
		return true;
	}
	if (mState == SLOTS_LEAVING) {
		if (!mDone) {
			if (0 < mLeaveDelay)
				mLeaveDelay--;
			else
				mDone = true;
		}
		return true;
	}

	// idle: the lever and its glow swell as the mouse comes onto the lever and shrink as it leaves; the done button the
	// same (not while the mouse is held down)
	ObjBase* aHover = sHover;
	if (mSpinButton == aHover) {
		if (mSpinButton->mHoverState == 0) {
			mSpinButton->mHoverState = 1;
			mSpinButton->StartTrack(SPIN_TRACK_HOVER);
			mSpinGlow->StartTrack(SPIN_TRACK_HOVER);
		}
	} else if (mSpinButton->mHoverState == 1) {
		mSpinButton->mHoverState = 0;
		mSpinButton->StartTrack(SPIN_TRACK_UNHOVER);
		mSpinGlow->StartTrack(SPIN_TRACK_UNHOVER);
	}
	if (mDoneButton == aHover) {
		if (!sMouseHeld && mDoneButton->mHoverState == 0) {
			mDoneButton->StartTrack(DONE_TRACK_HOVER);
			mDoneButton->mHoverState = 1;
			return true;
		}
	} else if (mDoneButton->mHoverState == 1) {
		mDoneButton->StartTrack(DONE_TRACK_UNHOVER);
		mDoneButton->mHoverState = 0;
		return true;
	}
	return true;
}

// 0x4699b0: the lever (button 0) spins; the done button (button 1) starts the leaving
bool Slots::MouseUp(ObjBase* theObj)
{
	if (theObj->mButtonId == SLOTS_BUTTON_SPIN) {
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		Spin();
	} else if (theObj->mButtonId == SLOTS_BUTTON_DONE) {
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		mState = SLOTS_LEAVING;
		mLeaveDelay = LEAVE_DELAY;
	}
	ShowBananas();
	return true;
}

// 0x4693b0: the win sound plays once for each 20 bananas and one more, 40 frames apart
void Slots::StartWinSounds(int theCount)
{
	mWinSoundId = SOUND_SLOTWIN;
	mWinSoundInterval = WIN_SOUND_INTERVAL;
	mWinSoundTimer = 0;
	mWinSoundsLeft = theCount;
	mSpinAfterSounds = 0;
}

// 0x4693e0: counts how many reels show each symbol; three of one is a win. The symbol is written to theSymbol (-1 for
// none) and the bananas it pays to thePayout; the result is what kind of prize
int Slots::ScoreRoll(int* thePayout, int* theSymbol)
{
	int aResult = SLOT_RESULT_NOTHING;
	*thePayout = 0;
	*theSymbol = SLOT_SYMBOL_NONE;
	for (int aSymbol = 0; aSymbol < SLOTS_SYMBOLS; aSymbol++)
		mSymbolCount[aSymbol] = 0;
	for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
		for (int aSymbol = 0; aSymbol < SLOTS_SYMBOLS; aSymbol++) {
			if (mResult[aReel] == aSymbol) {
				mSymbolCount[aSymbol]++;
				break;
			}
		}
	}
	for (int aSymbol = 0; aSymbol < SLOTS_SYMBOLS; aSymbol++) {
		if (2 < mSymbolCount[aSymbol]) {
			*theSymbol = mResult[0];
			switch (mResult[0]) {
			case SLOT_SYMBOL_BANANAS_100:
				*thePayout = 100;
				aResult = SLOT_RESULT_BANANAS;
				break;
			case SLOT_SYMBOL_FLY_PAPER:
			case SLOT_SYMBOL_ZAPPER:
			case SLOT_SYMBOL_SWAT:
			case SLOT_SYMBOL_TARZAN:
			case SLOT_SYMBOL_MINE:
				aResult = SLOT_RESULT_UNLOCK;
				break;
			case SLOT_SYMBOL_JACKPOT:
				*thePayout = 999;
				aResult = SLOT_RESULT_JACKPOT;
				break;
			}
		}
	}
	return aResult;
}

// 0x4694d0: a spin while the machine is idle. The first spin of a visit may be free; it draws from 0 to 35, a paid one
// from 0 to 60 (after the 100 bananas are taken; short of them, a line says so and the draw is not made). The draw
// picks what the reels will show: 0 to 8 the flypaper, 9 to 16 the zapper, 17 to 23 the swatter, 24 to 29 the monkeys,
// 30 to 35 the mine (each three of a kind the first time its number comes up, after that three places of the strips at
// random), 36 to 39 and 41 to 60 three places at random, and 40 the special roll: three of the 100-banana symbol, or
// the jackpot once more than 14 spins have gone by since a big win. The reels start only if all three are standing
// (the click and the done button's leaving are done in any case).
void Slots::Spin()
{
	if (mState != SLOTS_IDLE)
		return;
	unsigned int aDraw;
	if (mFreeSpin) {
		aDraw = GameRandomNext() % FREE_SPIN_DRAWS;
		mFreeSpin = false;
	} else {
		if (Player::SpendBananas() != 0) {
			ShowMessage("NOT ENOUGH BANANAS!", 1.0f, 1.0f, 0.1f);
			return;
		}
		aDraw = GameRandomNext() % PAID_SPIN_DRAWS;
	}
	gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
	mDoneButton->StartTrack(DONE_TRACK_HIDE);
	if (mStopped[0] == 0 || mStopped[1] == 0 || mStopped[2] == 0)
		return;
	if (mSpinRing != nullptr)
		mSpinRing->StartTrack(RING_TRACK_TURN);
	gApp->mSoundQueue->Request(SOUND_SLOTROLL, 0, true);
	mSpinsSinceBigWin++;
	mState = SLOTS_ROLLING;
	if (aDraw <= 8) {
		RollForced(SLOT_SYMBOL_FLY_PAPER);
	} else if (aDraw <= 16) {
		RollForced(SLOT_SYMBOL_ZAPPER);
	} else if (aDraw <= 23) {
		RollForced(SLOT_SYMBOL_SWAT);
	} else if (aDraw <= 29) {
		RollForced(SLOT_SYMBOL_TARZAN);
	} else if (aDraw <= 35) {
		RollForced(SLOT_SYMBOL_MINE);
	} else if (aDraw == ROLL_SPECIAL) {
		mSpinsLeft[0] = REEL_FIRST_TURNS;
		mStopped[0] = 0;
		mStopped[1] = 0;
		mSpinsLeft[2] = REEL_FIRST_TURNS + 4;
		mStopped[2] = 0;
		int aSymbol = JACKPOT_AFTER_SPINS < mSpinsSinceBigWin ? SLOT_SYMBOL_JACKPOT : SLOT_SYMBOL_BANANAS_100;
		for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
			mResult[aReel] = aSymbol;
			mTarget[aReel] = aSymbol * SLOTS_CELL_HEIGHT + SLOTS_CELL_MIDDLE;
		}
		mSpinsLeft[1] = REEL_FIRST_TURNS + 2;
		mSpinsSinceBigWin = 0;
		ShowBananas();
	} else {
		RollRandom();
	}
}

// 0x4694d0, the case of a symbol with a prize: the first time, all three reels are given the symbol; after that each
// takes a place of its strip at random (the C library's rand, one draw a reel, first reel first). Each reel turns
// 3, 5 and 7 times round before it looks for the place of its symbol
void Slots::RollForced(int theSymbol)
{
	int aTurns = REEL_FIRST_TURNS;
	for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
		if (!mRolled[theSymbol])
			mResult[aReel] = theSymbol;
		else
			mResult[aReel] = sReelStrip[CrtRand() % SLOTS_STRIP_ROWS][aReel];
		mSpinsLeft[aReel] = aTurns;
		mTarget[aReel] = mResult[aReel] * SLOTS_CELL_HEIGHT + SLOTS_CELL_MIDDLE;
		mStopped[aReel] = 0;
		aTurns += 2;
	}
	mRolled[theSymbol] = true;
	ShowBananas();
}

// 0x4694d0's default case: a place of each strip at random, the same draws
void Slots::RollRandom()
{
	int aTurns = REEL_FIRST_TURNS;
	for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
		int aSymbol = sReelStrip[CrtRand() % SLOTS_STRIP_ROWS][aReel];
		mSpinsLeft[aReel] = aTurns;
		mResult[aReel] = aSymbol;
		mTarget[aReel] = aSymbol * SLOTS_CELL_HEIGHT + SLOTS_CELL_MIDDLE;
		mStopped[aReel] = 0;
		aTurns += 2;
	}
	ShowBananas();
}

// 0x469a40: the base's count of the player's bananas (silently)
void Slots::ShowBananas()
{
	mMetaGame->UpdateBananasText(false);
}

// 0x469a50: the three lights over the reels, by how many 20s of bananas are left to pay in (a guess: the cels run from
// the lights out, to the lights on in turn). Up to one, all three out and additive; after that they chase round
void Slots::UpdateShines()
{
	int aSteps = mPayout / PAYOUT_STEP;
	if (1 < aSteps) {
		int aPhase = aSteps % 3;
		if (aPhase == 0) {
			mShines[0]->SetStateInt(STATE_FRAME, 3);
			mShines[0]->mDrawMode = 0;
			mShines[1]->SetStateInt(STATE_FRAME, 1);
			mShines[1]->mDrawMode = 0;
			mShines[2]->SetStateInt(STATE_FRAME, 0);
			mShines[2]->mDrawMode = 1;
		} else if (aPhase == 1) {
			mShines[0]->SetStateInt(STATE_FRAME, 1);
			mShines[0]->mDrawMode = 0;
			mShines[1]->SetStateInt(STATE_FRAME, 0);
			mShines[1]->mDrawMode = 1;
			mShines[2]->SetStateInt(STATE_FRAME, 2);
			mShines[2]->mDrawMode = 0;
		} else if (aPhase == 2) {
			mShines[0]->SetStateInt(STATE_FRAME, 0);
			mShines[0]->mDrawMode = 1;
			mShines[1]->SetStateInt(STATE_FRAME, 2);
			mShines[1]->mDrawMode = 0;
			mShines[2]->SetStateInt(STATE_FRAME, 3);
			mShines[2]->mDrawMode = 0;
		}
	} else {
		for (int i = 0; i < SLOTS_SHINES; i++) {
			mShines[i]->SetStateInt(STATE_FRAME, 0);
			mShines[i]->mDrawMode = 1;
		}
	}
}

// 0x46a3c0: the three symbols of a reel for the place it has turned to: the one the place is in, the one before it
// (wrapping from the first to the last) above, and the one after below, each 192 pixels apart
void Slots::PlaceReel(int thePosition, int theReel)
{
	int aCell = thePosition / SLOTS_CELL_HEIGHT;
	int aBefore = aCell - 1;
	if (aBefore < 0)
		aBefore = SLOTS_SYMBOLS - 1;
	int aAfter = aCell + 1;
	if (aAfter >= SLOTS_SYMBOLS)
		aAfter = 0;
	int aY = aCell * SLOTS_CELL_HEIGHT - thePosition + SLOTS_CELL_MIDDLE;
	mSymbols[theReel][0]->SetStateInt(STATE_Y, aY - SLOTS_CELL_HEIGHT);
	mSymbols[theReel][0]->SetStateInt(STATE_FRAME, aBefore);
	mSymbols[theReel][1]->SetStateInt(STATE_Y, aY);
	mSymbols[theReel][1]->SetStateInt(STATE_FRAME, aCell);
	mSymbols[theReel][2]->SetStateInt(STATE_Y, aY + SLOTS_CELL_HEIGHT);
	mSymbols[theReel][2]->SetStateInt(STATE_FRAME, aAfter);
}

// 0x46f740, as every line of the machine calls it: at the middle, twice the size, five frames late
void Slots::ShowMessage(const char* theText, float theRed, float theGreen, float theBlue)
{
	mMessage->Start(theText, 0.0f, 0.0f, MESSAGE_SCALE, theRed, theGreen, theBlue, MESSAGE_DELAY);
}

// 0x469bd0: the machine's own picture, and its three tracks: it swells in (the event 2 key), pulses (event 1, from
// Notify, again and again while a prize is counted in), and shrinks away at the end of the game, taking itself with it.
// The lights over the reels: three pictures side by side, 256 apart, additive.
void Slots::BuildCabinet()
{
	sMachineAnim->AddRef();
	mAnim = sMachineAnim;
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(20);
	aKey1->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aKey1->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aKey1->AddValue(STATE_ALPHA, 0.9f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(1);
	aKey2->mEventId = 2;
	aKey2->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(200);
	aKey3->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey3);
	AddTrack(aTrack1, nullptr);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(10);
	aKey4->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey4->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 0.05f, 0.0f, nullptr);
	aKey4->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aKey4->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 0.05f, 0.0f, nullptr);
	AnimKey* aKey5 = new AnimKey();
	aKey5->mEventId = 1;
	aKey5->SetDuration(1);
	aKey5->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey4, nullptr);
	aTrack2->AddKey(aKey5, aKey5);
	AddTrack(aTrack2, nullptr);
	AnimKey* aKey6 = new AnimKey();
	aKey6->SetDuration(10);
	aKey6->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey6->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	// original bug: this puts the alpha on the fifth key a second time (the sixth, which has none, was meant)
	aKey5->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey7 = new AnimKey();
	aKey7->SetDuration(1);
	aKey7->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey7->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey7->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aKey7->SetRemoveAtEnd();
	AnimTrack* aTrack3 = new AnimTrack();
	aTrack3->AddKey(aKey6, nullptr);
	aTrack3->AddKey(aKey7, nullptr);
	AddTrack(aTrack3, nullptr);
	for (int i = 0; i < SLOTS_SHINES; i++) {
		mShines[i] = new ObjBase();
		sShineAnim->AddRef();
		mShines[i]->mAnim = sShineAnim;
		mShines[i]->SetParent(this, nullptr);
		mShines[i]->SetState(STATE_ALPHA, 0.8f);
		mShines[i]->SetStateInt(STATE_Z, -1);
		mShines[i]->mDrawMode = 1;
		mShines[i]->SetDesignX((float)(300 + i * 256));
		mShines[i]->SetDesignY(464.0f);
		Activate(mShines[i], false);
	}
}

// 0x46a230: the reels' window (a child at the middle of the machine) and nine symbols in it, a reel to a column, 256
// apart across and 192 down, each reel set at its starting place
void Slots::BuildReels()
{
	mReelWindow = new ObjBase();
	mReelWindow->SetParent(this, nullptr);
	mReelWindow->SetStateInt(STATE_Z, -5);
	mReelWindow->SetDesignX(561.0f);
	mReelWindow->SetDesignY(588.0f);
	Activate(mReelWindow, false);
	const float REEL_X[SLOTS_REELS] = { -256.0f, 0.0f, 256.0f };
	const float ROW_Y[SLOTS_ROWS_SHOWN] = { -192.0f, 0.0f, 192.0f };
	for (int aReel = 0; aReel < SLOTS_REELS; aReel++) {
		for (int aRow = 0; aRow < SLOTS_ROWS_SHOWN; aRow++) {
			ObjBase* aSymbol = new ObjBase();
			mSymbols[aReel][aRow] = aSymbol;
			aSymbol->SetParent(mReelWindow, nullptr);
			sSymbolsAnim->AddRef();
			aSymbol->mAnim = sSymbolsAnim;
			aSymbol->SetState(STATE_X, REEL_X[aReel]);
			aSymbol->SetState(STATE_Y, ROW_Y[aRow]);
			aSymbol->SetStateInt(STATE_FRAME, aRow);
			Activate(aSymbol, false);
		}
		PlaceReel(REEL_PLACE_AT_START + aReel, aReel);
	}
}

// 0x46a490: the lever. Track 0 swells it in after 50 frames, 1 swells it while the mouse is on it (the lit cel), 2 back
void Slots::BuildSpinButton()
{
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(15);
	aKey1->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aKey1->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
	aKey1->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aKey1->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(100);
	aKey2->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, aKey2);
	aTrack1->SetDelay(50, false);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(5);
	aKey3->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(2);
	aKey4->AddValue(STATE_SCALE_X, 1.05f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 1.05f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_FRAME, 1.0f, MOD_HOLD);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey3, nullptr);
	aTrack2->AddKey(aKey4, aKey4);
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(5);
	aKey5->AddValue(STATE_SCALE_X, 1.05f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_SCALE_Y, 1.05f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_FRAME, 1.0f, MOD_HOLD);
	AnimKey* aKey6 = new AnimKey();
	aKey6->SetDuration(2);
	aKey6->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey6->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aKey6->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimTrack* aTrack3 = new AnimTrack();
	aTrack3->AddKey(aKey5, nullptr);
	aTrack3->AddKey(aKey6, aKey6);
	mSpinButton = new ObjBase();
	mSpinButton->SetMouseable(32.0f, 32.0f);
	mSpinButton->SetMouseListener(this);
	mSpinButton->mButtonId = SLOTS_BUTTON_SPIN;
	mSpinButton->SetParent(this, nullptr);
	sSpinAnim->AddRef();
	mSpinButton->mAnim = sSpinAnim;
	mSpinButton->AddTrack(aTrack1, nullptr);
	mSpinButton->AddTrack(aTrack2, nullptr);
	mSpinButton->AddTrack(aTrack3, nullptr);
	mSpinButton->mDrawMode = 1;
	mSpinButton->SetDesignX(560.0f);
	mSpinButton->SetDesignY(930.0f);
	mSpinButton->SetStateInt(STATE_Z, 0);
	mSpinButton->SetStateInt(STATE_SCALE_X, 0);
	mSpinButton->SetStateInt(STATE_SCALE_Y, 0);
	Activate(mSpinButton, false);
}

// 0x46aa40: the ring behind the lever: track 0 turns it, track 1 swells and shrinks it for ever
void Slots::BuildSpinRing()
{
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(999);
	aKey1->AddModifier(STATE_ROTATION, false, 0.0f, MOD_WRAP_DEGREES, 1.0f, 4.0f, 0.0f, nullptr);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(100);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, aKey2);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(200);
	aKey3->AddValue(STATE_SCALE_X, 1.1f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 1.1f, MOD_TOWARD_NEXT);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(200);
	aKey4->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(1);
	aKey5->AddValue(STATE_SCALE_X, 1.1f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_SCALE_Y, 1.1f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey3, nullptr);
	aTrack2->AddKey(aKey4, nullptr);
	aTrack2->AddKey(aKey5, aKey3);
	mSpinRing = new ObjBase();
	mSpinRing->SetParent(this, nullptr);
	sSpinAnim->AddRef();
	mSpinRing->mAnim = sSpinAnim;
	mSpinRing->AddTrack(aTrack1, nullptr);
	mSpinRing->AddTrack(aTrack2, nullptr);
	mSpinRing->SetDesignX(560.0f);
	mSpinRing->SetDesignY(930.0f);
	mSpinRing->SetStateInt(STATE_SCALE_X, 0);
	mSpinRing->SetStateInt(STATE_SCALE_Y, 0);
	mSpinRing->SetState(STATE_Z, -0.1f);
	mSpinRing->SetStateInt(STATE_FRAME, 2);
	mSpinRing->StartTrack(RING_TRACK_PULSE);
	Activate(mSpinRing, false);
}

// 0x46adb0: the glow behind the lever: track 0 breathes for ever, 1 and 2 swell and shrink it as the lever is hovered
void Slots::BuildSpinGlow()
{
	mSpinGlow = new ObjBase();
	mSpinGlow->SetParent(this, nullptr);
	sSpinAnim->AddRef();
	mSpinGlow->mAnim = sSpinAnim;
	mSpinGlow->SetDesignX(560.0f);
	mSpinGlow->SetDesignY(930.0f);
	mSpinGlow->SetState(STATE_SCALE_X, 0.0f);
	mSpinGlow->SetState(STATE_SCALE_Y, 0.0f);
	mSpinGlow->SetState(STATE_Z, 0.1f);
	mSpinGlow->SetStateInt(STATE_FRAME, 3);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(300);
	aKey1->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(300);
	aKey2->AddValue(STATE_SCALE_X, 1.1f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.1f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(0);
	aKey3->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey1);
	mSpinGlow->AddTrack(aTrack1, nullptr);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(5);
	aKey4->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(2);
	aKey5->AddValue(STATE_SCALE_X, 1.06f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_SCALE_Y, 1.06f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey4, nullptr);
	aTrack2->AddKey(aKey5, aKey5);
	mSpinGlow->AddTrack(aTrack2, nullptr);
	AnimKey* aKey6 = new AnimKey();
	aKey6->SetDuration(5);
	aKey6->AddValue(STATE_SCALE_X, 1.06f, MOD_TOWARD_NEXT);
	aKey6->AddValue(STATE_SCALE_Y, 1.06f, MOD_TOWARD_NEXT);
	AnimKey* aKey7 = new AnimKey();
	aKey7->SetDuration(2);
	aKey7->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey7->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack3 = new AnimTrack();
	aTrack3->AddKey(aKey6, nullptr);
	aTrack3->AddKey(aKey7, aKey7);
	mSpinGlow->AddTrack(aTrack3, nullptr);
	mSpinGlow->StartTrack(GLOW_TRACK_BREATHE);
	Activate(mSpinGlow, false);
}

// 0x46b2a0: the banner over the machine, "you win" or the jackpot's (its second cel), swelling in and spinning
void Slots::BuildWinBanner()
{
	mWinBanner = new ObjBase();
	sWinAnim->AddRef();
	mWinBanner->mAnim = sWinAnim;
	mWinBanner->SetStateInt(STATE_X, 0);
	mWinBanner->SetStateInt(STATE_Y, -300);
	mWinBanner->SetStateInt(STATE_Z, 20);
	mWinBanner->SetParent(this, nullptr);
	mWinBanner->SetStateInt(STATE_SCALE_X, 0);
	mWinBanner->SetStateInt(STATE_SCALE_Y, 0);
	mWinBanner->SetStateInt(STATE_ALPHA, 0);
	mWinBanner->SetStateInt(STATE_FRAME, 0);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(40);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aKey1->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aKey1->AddValue(STATE_ROTATION, 12.566371f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_CUBE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(130);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(25);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(25);
	aKey4->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(0);
	aKey5->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, nullptr);
	aTrack1->AddKey(aKey4, nullptr);
	aTrack1->AddKey(aKey5, nullptr);
	mWinBanner->AddTrack(aTrack1, nullptr);
	Deactivate(mWinBanner);
}

// 0x46b7d0: the picture of the unlocked bonus (a cel of the bonuses' picture), swelling in and out; shown by Update
void Slots::BuildUnlockBubble()
{
	mUnlockBubble = new ObjBase();
	BonusCoin::sBonusesAnim->AddRef();
	mUnlockBubble->mAnim = BonusCoin::sBonusesAnim;
	mUnlockBubble->SetStateInt(STATE_Z, 20);
	mUnlockBubble->SetParent(this, nullptr);
	mUnlockBubble->SetStateInt(STATE_X, -8);
	mUnlockBubble->SetDesignY(930.0f);
	mUnlockBubble->SetStateInt(STATE_SCALE_X, 0);
	mUnlockBubble->SetStateInt(STATE_SCALE_Y, 0);
	mUnlockBubble->SetStateInt(STATE_ALPHA, 0);
	mUnlockBubble->SetStateInt(STATE_FRAME, 0);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(25);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aKey1->AddValue(STATE_ROTATION, -0.06981317f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(150);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_X, 4.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_Y, 4.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ROTATION, -0.034906585f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(25);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ROTATION, 0.034906585f, MOD_TOWARD_NEXT);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(1);
	aKey4->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ROTATION, 3.1415927f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, nullptr);
	aTrack1->AddKey(aKey4, nullptr);
	mUnlockBubble->AddTrack(aTrack1, nullptr);
	Deactivate(mUnlockBubble);
}

// 0x46bc80: the done button, as the base's (MetaGame::BuildDoneButton): its own child here, and its mouse calls come
// to the machine. Track 0 turns it in after 100 frames with the drum's sound, 1 and 2 swell it and shrink it as the
// mouse comes and goes, 3 takes it away (while the reels turn) and 4 brings it back.
void Slots::BuildDoneButton()
{
	mDoneButton = new ObjBase();
	mDoneButton->mAnim = Anim::Get("images/meta/panel_done", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mDoneButton->SetParent(this, nullptr);
	mDoneButton->SetStateInt(STATE_X, 650);
	mDoneButton->SetStateInt(STATE_Z, 7);
	mDoneButton->SetState(STATE_Y, 508.0f);
	mDoneButton->SetStateInt(STATE_ALPHA, 0);
	mDoneButton->SetMouseable(32.0f, 32.0f);
	mDoneButton->SetMouseListener(this);
	mDoneButton->mButtonId = SLOTS_BUTTON_DONE;
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(25);
	aKey1->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ROTATION, -1.3962635f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_CUBE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->mEndCondition = KEY_SOUND;
	aKey2->mEndChannel = SOUND_DRUM_C;
	aKey2->mEndValue = 2.0f;
	aKey2->SetDuration(1);
	aKey2->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mEventId = 0;
	aKey3->SetDuration(5);
	aKey3->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->SetDelay(100, false);
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey3);
	mDoneButton->AddTrack(aTrack1, nullptr);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(5);
	aKey4->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(2);
	aKey5->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey5->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey4, nullptr);
	aTrack2->AddKey(aKey5, aKey5);
	mDoneButton->AddTrack(aTrack2, nullptr);
	AnimKey* aKey6 = new AnimKey();
	aKey6->SetDuration(5);
	aKey6->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey6->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey6->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey6->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	AnimKey* aKey7 = new AnimKey();
	aKey7->SetDuration(2);
	aKey7->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey7->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey7->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey7->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack3 = new AnimTrack();
	aTrack3->AddKey(aKey6, nullptr);
	aTrack3->AddKey(aKey7, aKey7);
	mDoneButton->AddTrack(aTrack3, nullptr);
	AnimKey* aKey8 = new AnimKey();
	aKey8->SetDuration(5);
	aKey8->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey8->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey8->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey8->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey9 = new AnimKey();
	aKey9->SetDuration(1);
	aKey9->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey9->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey9->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aKey9->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack4 = new AnimTrack();
	aTrack4->AddKey(aKey8, nullptr);
	aTrack4->AddKey(aKey9, nullptr);
	mDoneButton->AddTrack(aTrack4, nullptr);
	AnimKey* aKey10 = new AnimKey();
	aKey10->SetDuration(5);
	aKey10->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey10->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey10->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aKey10->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey11 = new AnimKey();
	aKey11->SetDuration(1);
	aKey11->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey11->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey11->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey11->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack5 = new AnimTrack();
	aTrack5->AddKey(aKey10, nullptr);
	aTrack5->AddKey(aKey11, aKey11);
	mDoneButton->AddTrack(aTrack5, nullptr);
	Activate(mDoneButton, false);
}

// 0x46c710
void Sexy::PreloadSlots()
{
	Slots::sMachineAnim = Anim::Get("images/slot/slot_machine", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	Slots::sSymbolsAnim = Anim::Get("images/slot/slot_symbols", ANIM_SCALE_SMOOTH, false, 0xe0, 0xc0, 7, false);
	Slots::sWinAnim = Anim::Get("images/slot/slot_win", ANIM_SCALE_SMOOTH, false, 0x3c3, 0xc9, 2, false);
	Slots::sSpinAnim = Anim::Get("images/slot/slot_spin", ANIM_SCALE_SMOOTH, false, 0x11b, 0x11b, 4, false);
	Slots::sNumbersAnim = Anim::Get("images/slot/slot_numbers", ANIM_SCALE_SMOOTH, false, 0x3f, 0x49, 10, false);
	Slots::sShineAnim = Anim::Get("images/slot/slot_shine", ANIM_SCALE_SMOOTH, false, 0x100, 0xc0, 4, false);
}
