#include "ScoreBoard.h"

#include "Anim.h"
#include "Color.h"
#include "Emitter.h"
#include "FireBall.h"
#include "GameApp.h"
#include "Graphics.h"
#include "HighScore.h"
#include "ObjString.h"
#include "Pending.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The board's object type (ObjBase::mType), set by its constructor
const int OBJ_TYPE_SCORE_BOARD = 13;

// The key events the board's own tracks raise (AnimKey::mEventId); the icons' tracks raise 5, 6 and 7
enum BoardEvent
{
	BOARD_EVENT_ENTER_SOUND = 1,   // the board is rising into place
	BOARD_EVENT_ENTERED = 2,       // it is in place
	BOARD_EVENT_LEAVE_SOUND = 3,   // it is going away
	BOARD_EVENT_LEFT = 4,          // it has gone
	ICON_EVENT_POPPED = 5,         // an icon has popped in: its star ring and swell burst
	ICON_EVENT_SWELLED = 6,        // nothing listens for it
	ICON_EVENT_GONE = 7,           // an icon has gone away (the track that clears it, or the one that throws it)
};

// The tracks an icon holds (the order they are added in)
enum IconTrack
{
	ICON_TRACK_POP_IN = 0,
	ICON_TRACK_CLEAR = 1,
	ICON_TRACK_THROW = 2,
};

// What MarkLastIcon sets in an icon's button id (an ObjBase's spare word) so the next call passes it by
const int ICON_MARKED = 0x43;

// The sound a banana icon makes as it pops in and as it is thrown away
const int ICON_SOUND = SOUND_POPCORN;

// A whole number as the x87 maths takes it, a double
static double ToDouble(int theValue)
{
	return (double)theValue;
}

// 0x464320: the picture, the dirt, leaf and banana pictures, the place it rests at (further right on a wide screen),
// the sparks, tracks and counters, one MonkeyLives for each monkey the second upgrade adds, one fire ball for each
// the tenth gives, the 25 banana icons and the marks of the progress bar. It starts away and hidden. The original also
// names the object "ScoreBoard" in a field nothing reads.
ScoreBoard::ScoreBoard()
{
	mAnim = Anim::Get("images/scoreboard01", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mDirtAnim = Anim::Get("images/dirt01", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mLeafAnim = Anim::Get("images/tinyleaf", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mBananaIconAnim = Anim::Get("images/bananabunch01", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mGemsAnim = Anim::Get("images/scoreboard_gems", ANIM_SCALE_SMOOTH, false, 0xa1, 0x42, 4, false);
	mPlaceX = gApp->mWidescreen ? 664.0f : 710.0f;
	mPlaceY = (float)(600.0 - (double)mAnim->GetSourceCelHeight() * 0.5);
	SetState(STATE_X, mPlaceX);
	SetState(STATE_Y, mPlaceY);
	SetStateInt(STATE_Z, 0x17);
	ObjBase::Activate(this, false);
	mAway = true;
	mType = OBJ_TYPE_SCORE_BOARD;
	BuildDirtFall();
	BuildSprays();
	BuildHud();
	BuildEnterTrack();
	BuildLeaveTrack();
	BuildGems();
	mUnused290 = (float)((double)mPlaceX - (double)mAnim->GetCelWidth() * (double)gApp->mDesignScaleX * 0.5);
	mUnused294 = (float)((double)mPlaceY - (double)mAnim->GetCelHeight() * (double)gApp->mDesignScaleY * 0.5);

	// The sparks that fly from the two corners of the picture as the level begins: glowing flecks that fade. Two
	// emitters share the three keys; each waits for the intro (1500 frames the first time, 300 once a game is going)
	AnimKey* aFlare = new AnimKey();
	aFlare->SetDuration(10);
	aFlare->mDrawMode = 1;
	aFlare->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aFlare->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFlare->AddValue(STATE_SCALE_X, 0.5f, MOD_TOWARD_NEXT);
	aFlare->AddValue(STATE_SCALE_Y, 0.5f, MOD_TOWARD_NEXT);
	aFlare->AddValue(STATE_RED, 0.6f, MOD_TOWARD_NEXT);
	aFlare->AddValue(STATE_GREEN, 0.5f, MOD_TOWARD_NEXT);
	aFlare->AddValue(STATE_BLUE, 0.5f, MOD_TOWARD_NEXT);
	aFlare->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aGlow = new AnimKey();
	aGlow->SetDuration(100);
	aGlow->mDrawMode = 1;
	aGlow->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aGlow->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aGlow->AddValue(STATE_SCALE_Y, 1.3f, MOD_TOWARD_NEXT);
	aGlow->AddValue(STATE_RED, 0.6f, MOD_TOWARD_NEXT);
	aGlow->AddValue(STATE_GREEN, 0.5f, MOD_TOWARD_NEXT);
	aGlow->AddValue(STATE_BLUE, 0.5f, MOD_TOWARD_NEXT);
	aGlow->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);

	AnimKey* aFlareEnd = new AnimKey();
	aFlareEnd->SetDuration(0);
	aFlareEnd->AddValue(STATE_Y, -100.0f, MOD_HOLD);
	aFlareEnd->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aFlareEnd->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aFlareEnd->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aFlareEnd->AddValue(STATE_GREEN, 0.6f, MOD_HOLD);
	aFlareEnd->AddValue(STATE_BLUE, 0.2f, MOD_HOLD);
	aFlareEnd->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);

	AnimTrack* aFirstTrack = new AnimTrack();
	AnimTrack* aSecondTrack = new AnimTrack();
	aFirstTrack->AddKey(aFlare, nullptr);
	aSecondTrack->AddKey(aFlare, nullptr);
	aFirstTrack->AddKey(aGlow, nullptr);
	aSecondTrack->AddKey(aGlow, nullptr);
	aFirstTrack->AddKey(aFlareEnd, nullptr);
	aSecondTrack->AddKey(aFlareEnd, nullptr);

	// Each waits for the intro before it starts: 0x5dc frames (0x62c for the second) the first time, 0x12c (0x17c) once
	// a game is in progress
	bool aInProgress = Player::sCurrent->mSavedProgress != 0;

	Emitter* aLeftFlare = new Emitter(this, false);
	aLeftFlare->SetAdditive();
	aLeftFlare->mAnim = Emitter::sParticlesAnim;
	Emitter::sParticlesAnim->AddRef();
	aLeftFlare->SetStateInt(STATE_FRAME, 0);
	aLeftFlare->AddTrack(aFirstTrack, nullptr);
	aLeftFlare->SetStateInt(STATE_Z, 2);
	int aFlareY = 0x2ca - (int)mAnim->GetSourceCelHeight() / 2;
	aLeftFlare->Setup(10, 10, 1, 0x3c - (int)mAnim->GetSourceCelWidth() / 2, aFlareY, 2, 0, 0xb4);
	aLeftFlare->mDelay = aInProgress ? 0x12c : 0x5dc;
	aLeftFlare->SetSoundAtStart();

	Emitter* aRightFlare = new Emitter(this, false);
	aRightFlare->SetAdditive();
	aRightFlare->mAnim = Emitter::sParticlesAnim;
	Emitter::sParticlesAnim->AddRef();
	aRightFlare->SetStateInt(STATE_FRAME, 0);
	aRightFlare->AddTrack(aSecondTrack, nullptr);
	aRightFlare->SetStateInt(STATE_Z, 2);
	aFlareY = 0x2ca - (int)mAnim->GetSourceCelHeight() / 2;
	aRightFlare->Setup(12, 9, 1, 0x146 - (int)mAnim->GetSourceCelWidth() / 2, aFlareY, 2, 0, 0xb4);
	aRightFlare->mDelay = aInProgress ? 0x17c : 0x62c;
	aRightFlare->SetSoundAtStart();

	// the three counters, in the red font, at their places in the design space
	mScoreText = new ObjString();
	mScoreText->SetParent(this, nullptr);
	mScoreText->SetFont(gApp->mRedFont);
	mScoreText->SetStateInt(STATE_Z, 1);
	mScoreText->SetStateInt(STATE_SCALE_X, 1);
	mScoreText->SetStateInt(STATE_SCALE_Y, 1);
	mScoreText->Create("00000000000", false, 0);
	mScoreText->SetDesignY(306.0f);

	mPercentText = new ObjString();
	mPercentText->SetParent(this, nullptr);
	mPercentText->SetFont(gApp->mRedFont);
	mPercentText->SetStateInt(STATE_Z, 1);
	mPercentText->SetStateInt(STATE_SCALE_X, 1);
	mPercentText->SetStateInt(STATE_SCALE_Y, 1);
	mPercentText->Create("000", false, 0);
	mPercentText->SetDesignX(258.0f);
	mPercentText->SetDesignY(410.0f);

	mLevelText = new ObjString();
	mLevelText->SetParent(this, nullptr);
	mLevelText->SetFont(gApp->mRedFont);
	mLevelText->SetStateInt(STATE_Z, 1);
	mLevelText->SetStateInt(STATE_SCALE_X, 1);
	mLevelText->SetStateInt(STATE_SCALE_Y, 1);
	mLevelText->Create("000", false, 0);
	mLevelText->SetDesignX(139.0f);
	mLevelText->SetDesignY(410.0f);

	// a monkey for each the second upgrade added, along the bottom
	for (int i = 0; i < SCOREBOARD_MONKEYS; i++)
		mMonkeys[i] = nullptr;
	const int MONKEY_FIRST_X = -0x60;
	const int MONKEY_SPACING = 0x40;
	const int MONKEY_Y = 0x3a;
	int aMonkeyX = MONKEY_FIRST_X;
	for (int i = 0; i < Player::sCurrent->mSecondUpgradeValue; i++) {
		MonkeyLives* aMonkey = new MonkeyLives();
		mMonkeys[i] = aMonkey;
		aMonkey->mWholePixels = true;
		aMonkey->SetParent(this, nullptr);
		aMonkey->SetStateInt(STATE_X, aMonkeyX);
		aMonkey->SetStateInt(STATE_Y, MONKEY_Y);
		aMonkey->SetStateInt(STATE_Z, 1); // the original passes the channel number SetState returned, 1
		aMonkey->Activate(aMonkey, false);
		aMonkeyX += MONKEY_SPACING;
	}

	// a fire ball for each the tenth upgrade gives, under the root, waiting out of play
	for (int i = 0; i < SCOREBOARD_FIREBALLS; i++)
		mFireBalls[i] = nullptr;
	for (int i = 0; i < Player::sCurrent->mTenthUpgradeValue; i++) {
		FireBall* aBall = new FireBall();
		mFireBalls[i] = aBall;
		aBall->SetParent(nullptr, nullptr);
		aBall->SetStateInt(STATE_Z, 300);
		Deactivate(aBall);
	}

	// port: the original leaves the icon slots after the 25th and the count to the allocator's memory until the loop sets them
	for (int i = 0; i < SCOREBOARD_ICON_SLOTS; i++)
		mIcons[i] = nullptr;
	BuildBananaIcons();
	BuildBarMarks();
	ShowScore(Player::sCurrent->mScore);
	ShowLevel(Player::sCurrent->mSavedProgress + 1);
	Deactivate(this);
}

// 0x464ed0: the fire balls go with it
ScoreBoard::~ScoreBoard()
{
	for (int i = 0; i < SCOREBOARD_FIREBALLS; i++) {
		if (mFireBalls[i] != nullptr) {
			mFireBalls[i]->Delete();
			mFireBalls[i] = nullptr;
		}
	}
}

// 0x464f50: the bar of the world's progress behind the picture (a dark bar the whole length, a yellow one as far as
// the progress), with the gems' cel set to the fire balls the player has
void ScoreBoard::Draw(Graphics* g)
{
	if (mGems != nullptr)
		mGems->SetStateInt(STATE_FRAME, Player::sCurrent->mTenthUpgradeValue);
	float aFullWidth = (float)((double)mBarTopRight->mDrawn[STATE_X] - (double)mBarTopLeft->mDrawn[STATE_X] + 1.0);
	int aHeight = (int)((double)mBarBottomLeft->mDrawn[STATE_Y] - (double)mBarTopLeft->mDrawn[STATE_Y] + 1.0);
	int aLeft = (int)mBarTopLeft->mDrawn[STATE_X];
	int aTop = (int)mBarTopLeft->mDrawn[STATE_Y];
	g->SetColor(Color(40, 40, 0, 255));
	g->FillRect(aLeft, aTop, (int)aFullWidth, aHeight);
	float aProgressWidth = (float)((double)Player::sCurrent->mWorldProgress * (double)aFullWidth);
	g->SetColor(Color(245, 230, 16, 255));
	g->FillRect(aLeft, aTop, (int)aProgressWidth, aHeight);
	ObjBase::Draw(g);
}

// 0x465080: the board slides up and shows
void ScoreBoard::Show()
{
	StartTrack(mEnterTrack);
	Activate(this, false);
}

// 0x4650a0: every icon shown plays its clearing track, and the count starts again at nothing. The original has no null
// check; the icons are all made.
void ScoreBoard::ClearBananaIcons()
{
	for (int i = 0; i < SCOREBOARD_ICONS; i++) {
		ObjBase* anIcon = mIcons[i];
		if (anIcon->IsActive())
			anIcon->StartTrack(ICON_TRACK_CLEAR);
	}
	mIconCount = 0;
	ShowBananaTotal(0);
}

// 0x4650f0: the last icon still shown and not yet marked is marked and sent flying
void ScoreBoard::MarkLastIcon()
{
	for (int i = mIconCount - 1; i >= 0; i--) {
		ObjBase* anIcon = mIcons[i];
		if (anIcon != nullptr && anIcon->IsActive() && anIcon->mButtonId == 0) {
			anIcon->mButtonId = ICON_MARKED;
			anIcon->StartTrack(ICON_TRACK_THROW);
			return;
		}
	}
}

// 0x465140: the next icon pops in (until all 25 show)
void ScoreBoard::ShowBananaIcon()
{
	if (mIconCount < SCOREBOARD_ICONS) {
		ObjBase* anIcon = mIcons[mIconCount];
		anIcon->mButtonId = 0;
		anIcon->StartTrack(ICON_TRACK_POP_IN);
		anIcon->Activate(anIcon, false);
		mIconCount++;
		ShowBananaTotal(Player::GetBananaValue() * mIconCount);
	}
}

// 0x4651a0: the fire ball of that number leaves 300 pixels above the board's place; none made past the upgrade's value
void ScoreBoard::LaunchFireBall(int theIndex)
{
	if (mFireBalls[theIndex] != nullptr)
		mFireBalls[theIndex]->Throw(mScreenX, (float)((double)mScreenY - 300.0));
}

// 0x4651e0: 25 icons in 5 rows of 5, the first row at the top and each a little smaller than the one before, each
// with a ring of stars to burst and three tracks (see IconTrack)
void ScoreBoard::BuildBananaIcons()
{
	float aFade = 1.0f;
	mIconCount = 0;
	for (int i = 0; i < SCOREBOARD_ICONS; i++) {
		float aX = (float)((double)aFade * ((double)(i % 5) * 35.0 - 70.0));
		float aY = (float)((double)(4 - i / 5) * 28.0 + 160.0);
		aFade = (float)((double)aFade - 0.025);

		ObjBase* anIcon = new ObjBase();
		mIcons[i] = anIcon;
		anIcon->mEmitters = new EmitterSet(anIcon);
		anIcon->mEmitters->MakeSlowSwell(1.1f);
		anIcon->mEmitters->MakeStarRing(1.1f, 0x28);
		mBananaIconAnim->AddRef();
		anIcon->mAnim = mBananaIconAnim;
		anIcon->SetParent(this, nullptr);
		anIcon->SetState(STATE_X, aX);
		anIcon->SetState(STATE_Y, aY);
		anIcon->SetState(STATE_Z, (float)(ToDouble(i) * 0.019999999552965164 + 2.9000000953674316));
		anIcon->SetState(STATE_ROTATION, (float)(ToDouble((int)(GameRandomNext() % 0x168)) * PI_F / 180.0));

		// Popping in: from nothing, 5 degrees round, then a sound and a wait
		AnimKey* aGrow = new AnimKey();
		aGrow->SetDuration(0x10);
		aGrow->AddValue(STATE_X, 0.0f, MOD_HOLD);
		aGrow->AddValue(STATE_Y, 0.0f, MOD_HOLD);
		aGrow->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
		aGrow->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
		aGrow->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
		aGrow->AddValue(STATE_ROTATION, (float)((double)anIcon->mState[STATE_ROTATION] + 0.0872664675116539),
			MOD_TOWARD_NEXT);

		AnimKey* aPop = new AnimKey();
		aPop->mEventId = ICON_EVENT_POPPED;
		aPop->SetDuration(1);
		aPop->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		aPop->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
		aPop->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
		aPop->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
		aPop->mEndCondition = KEY_SOUND;
		aPop->mEndChannel = ICON_SOUND;
		aPop->mEndValue = 0.0f;

		AnimKey* aRest = new AnimKey();
		aRest->SetDuration(100);

		AnimTrack* aPopInTrack = new AnimTrack();
		aPopInTrack->AddKey(aGrow, nullptr);
		aPopInTrack->AddKey(aPop, nullptr);
		aPopInTrack->AddKey(aRest, aRest);
		anIcon->AddTrack(aPopInTrack, nullptr);

		// Clearing: a short wait, then it shrinks and drops away
		AnimKey* aWait = new AnimKey();
		aWait->SetDuration(3);
		aWait->AddValue(STATE_X, 0.0f, MOD_HOLD);
		aWait->AddValue(STATE_Y, 0.0f, MOD_HOLD);
		aWait->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);

		AnimKey* aSwell = new AnimKey();
		aSwell->mEventId = ICON_EVENT_SWELLED;
		aSwell->SetDuration(0x14);
		aSwell->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aSwell->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aSwell->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aSwell->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);

		AnimKey* aDrop = new AnimKey();
		aDrop->SetDuration(5);
		aDrop->AddValue(STATE_Y, -4.0f, MOD_HOLD);
		aDrop->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
		aDrop->AddValue(STATE_SCALE_X, -0.1f, MOD_HOLD);
		aDrop->AddValue(STATE_SCALE_Y, -0.1f, MOD_HOLD);

		AnimKey* aGone = new AnimKey();
		aGone->mEventId = ICON_EVENT_GONE;
		aGone->SetDuration(1);
		aGone->AddValue(STATE_Y, 0.0f, MOD_HOLD);
		aGone->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
		aGone->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
		aGone->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);

		AnimTrack* aClearTrack = new AnimTrack();
		aClearTrack->AddKey(aWait, nullptr);
		aClearTrack->AddKey(aSwell, nullptr);
		aClearTrack->AddKey(aDrop, nullptr);
		aClearTrack->AddKey(aGone, nullptr);
		anIcon->AddTrack(aClearTrack, nullptr);

		// Thrown: it flies off upward at 70 to 110 degrees with a sound, falling under gravity, until it is past the
		// foot of the design space, then fades
		float aThrowAngle = (float)(ToDouble((int)(GameRandomNext() % 0x29 + 0x46)) * PI_F / 180.0);
		AnimKey* aThrowSound = new AnimKey();
		aThrowSound->SetDuration(1);
		aThrowSound->mEndCondition = KEY_SOUND;
		aThrowSound->mEndChannel = ICON_SOUND;
		aThrowSound->mEndValue = 2.0f;

		AnimKey* aFly = new AnimKey();
		aFly->SetDuration(400);
		aFly->mEventId = ICON_EVENT_GONE;
		aFly->AddDecay(STATE_X, 0.0f, (float)(ToDouble((int)(GameRandomNext() % 0x29 + 0x32)) / 5.0),
			FloatCos(aThrowAngle), 0.0f, 1.0f);
		aFly->AddDecay(STATE_Y, 0.0f, (float)(ToDouble((int)(GameRandomNext() % 0x29 + 0x32)) / 5.0),
			FloatSin(aThrowAngle), -0.26f, 1.0f);
		aFly->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aFly->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
		aFly->mEndCondition = KEY_END_ABOVE;
		aFly->mEndChannel = STATE_Y;
		aFly->mEndValue = 1030.0f;

		AnimKey* aFlyFade = new AnimKey();
		aFlyFade->SetDuration(10);
		aFlyFade->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aFlyFade->AddValue(STATE_ROTATION, aThrowAngle, MOD_TOWARD_NEXT);

		AnimKey* aFlyGone = new AnimKey();
		aFlyGone->SetDuration(1);
		aFlyGone->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);

		AnimTrack* aThrowTrack = new AnimTrack();
		aThrowTrack->AddKey(aThrowSound, nullptr);
		aThrowTrack->AddKey(aFly, nullptr);
		aThrowTrack->AddKey(aFlyFade, nullptr);
		aThrowTrack->AddKey(aFlyGone, nullptr);
		anIcon->AddTrack(aThrowTrack, nullptr);
		anIcon->SetEventTarget(this);
	}
}

// 0x465bf0: from 1100 below its place, slowly up for 100 frames, and hold 200
void ScoreBoard::BuildEnterTrack()
{
	AnimKey* aStart = new AnimKey();
	aStart->SetDuration(1);
	aStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_Y, 1100.0f, MOD_TOWARD_NEXT);

	AnimKey* aRise = new AnimKey();
	aRise->mEventId = BOARD_EVENT_ENTER_SOUND;
	aRise->SetDuration(100);
	aRise->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_Y, 1100.0f, MOD_TOWARD_NEXT);
	aRise->mEasing = EASE_COSINE_TOO;

	AnimKey* aSettled = new AnimKey();
	aSettled->mEventId = BOARD_EVENT_ENTERED;
	aSettled->SetDuration(200);
	aSettled->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aSettled->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aStart, nullptr);
	aTrack->AddKey(aRise, nullptr);
	aTrack->AddKey(aSettled, aSettled);
	mEnterTrack = AddTrack(aTrack, nullptr);
}

// 0x465db0: still, then 100 frames easing in toward 200 more frames down to 1100 below its place, and out to 1200
void ScoreBoard::BuildLeaveTrack()
{
	AnimKey* aStart = new AnimKey();
	aStart->SetDuration(1);
	aStart->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aStart->AddValue(STATE_Y, 0.0f, MOD_HOLD);

	AnimKey* aWait = new AnimKey();
	aWait->mEventId = BOARD_EVENT_LEAVE_SOUND;
	aWait->SetDuration(100);
	aWait->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aWait->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aWait->mEasing = EASE_IN_CUBE;

	AnimKey* aDrop = new AnimKey();
	aDrop->SetDuration(200);
	aDrop->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aDrop->AddValue(STATE_Y, 1100.0f, MOD_TOWARD_NEXT);

	AnimKey* aGone = new AnimKey();
	aGone->mEventId = BOARD_EVENT_LEFT;
	aGone->SetDuration(1);
	aGone->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_Y, 1200.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aStart, nullptr);
	aTrack->AddKey(aWait, nullptr);
	aTrack->AddKey(aDrop, nullptr);
	aTrack->AddKey(aGone, aGone);
	mLeaveTrack = AddTrack(aTrack, nullptr);
}

// 0x465fe0: a leaf under the board's parent, at a random place within 128 of the board's resting x and a random
// turn, thrown up and sideways at 80 to 100 degrees, falling, then sent far away. It waits theDelay frames.
void ScoreBoard::SpawnLeaf(int theDelay)
{
	ObjBase* aLeaf = new ObjBase();
	aLeaf->SetParent(mParent, nullptr);
	mLeafAnim->AddRef();
	aLeaf->mAnim = mLeafAnim;
	aLeaf->SetState(STATE_X, (float)((double)((int)(GameRandomNext() % 0x101) - 0x80) + (double)mPlaceX));
	aLeaf->SetStateInt(STATE_Y, 600);
	aLeaf->SetStateInt(STATE_Z, 2);
	aLeaf->SetState(STATE_ROTATION, (float)(ToDouble((int)(GameRandomNext() % 0x169)) * PI_F / 180.0));
	float anAngle = (float)(ToDouble((int)(GameRandomNext() % 0x15 + 0x50)) * PI_F / 180.0);

	AnimKey* aFall = new AnimKey();
	aFall->SetDuration(0x15e);
	aFall->AddDecay(STATE_X, 0.0f, (float)(ToDouble((int)(GameRandomNext() % 0x24 + 0x14)) / 10.0), FloatCos(anAngle),
		0.0f, 1.0f);
	aFall->AddDecay(STATE_Y, 0.0f, (float)(ToDouble((int)(GameRandomNext() % 0x24 + 0x14)) / 10.0), FloatSin(anAngle),
		-0.06f, 1.0f);
	// the original gives the blue a whole number of 245 to 255, not a fraction (a bug: the colour is clamped)
	aFall->AddValue(STATE_BLUE, (float)ToDouble((int)(GameRandomNext() % 0xb + 0xf5)), MOD_HOLD);

	AnimKey* aAway = new AnimKey();
	aAway->SetDuration(100);
	aAway->AddValue(STATE_X, 1800.0f, MOD_HOLD);
	aAway->AddValue(STATE_Y, 600.0f, MOD_HOLD);

	AnimKey* aEnd = new AnimKey();
	aEnd->SetDuration(1);
	aEnd->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aEnd->AddValue(STATE_Y, -700.0f, MOD_HOLD);
	aEnd->SetRemoveAtEnd();

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFall, nullptr);
	aTrack->AddKey(aAway, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	aLeaf->AddTrack(aTrack, nullptr);
	aLeaf->Activate(aLeaf, false);
	aTrack->SetDelay(theDelay, false);
}

// 0x468780: the board slides away and 16 leaves are thrown, one more every 4 frames from 5
void ScoreBoard::Leave()
{
	StartTrack(mLeaveTrack);
	for (int aDelay = 5; aDelay < 0x45; aDelay += 4)
		SpawnLeaf(aDelay);
}

// 0x466350 (its eight copies in the original are written out): an additive spark emitter with its own track of the
// two keys given, at a place on the picture, kept below the board's text in depth; made hidden
Emitter* ScoreBoard::MakeHudEmitter(AnimKey* theFirst, AnimKey* theSecond, int theZ, int theCount, int theRate,
	int theX, int theY, int theJitterX, float theDepth)
{
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(theFirst, nullptr);
	aTrack->AddKey(theSecond, nullptr);
	Emitter* anEmitter = new Emitter(this, true);
	anEmitter->SetAdditive();
	anEmitter->mAnim = Emitter::sParticlesAnim;
	Emitter::sParticlesAnim->AddRef();
	anEmitter->SetStateInt(STATE_FRAME, 0);
	anEmitter->SetStateInt(STATE_Z, theZ);
	anEmitter->AddTrack(aTrack, nullptr);
	int aHeight = (int)mAnim->GetSourceCelHeight();
	int aWidth = (int)mAnim->GetSourceCelWidth();
	anEmitter->Setup(theCount, theRate, 1, theX - aWidth / 2, theY - aHeight / 2, theJitterX, 0, 0xb4);
	Emitter::SetDepth(anEmitter, theDepth);
	return anEmitter;
}

// 0x466350: four pairs of emitters (two of them with sparks that orbit out), all made hidden
void ScoreBoard::BuildHud()
{
	const int HUD_SPARK_Z = 10;
	const int HUD_COUNTER_Y = 0x6a;
	const int HUD_BOTTOM_Z = 0xf;
	const int HUD_BOTTOM_Y = 0x2c8;

	// Pair 0 and 1 (+0x2c4, +0x2c8): small pale gold sparks flying out, fading at the end
	AnimKey* aSparkOut = new AnimKey();
	aSparkOut->SetDuration(0x82);
	aSparkOut->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aSparkOut->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aSparkOut->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aSparkOut->AddValue(STATE_GREEN, 0.9f, MOD_TOWARD_NEXT);
	aSparkOut->AddValue(STATE_BLUE, 0.9f, MOD_TOWARD_NEXT);
	aSparkOut->AddValue(STATE_ALPHA, 0.8f, MOD_TOWARD_NEXT);
	aSparkOut->AddValue(STATE_ORBIT_RADIUS, 0.3f, MOD_TOWARD_NEXT);
	aSparkOut->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aSparkOut->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);

	AnimKey* aSparkFade = new AnimKey();
	aSparkFade->SetDuration(1);
	aSparkFade->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aSparkFade->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aSparkFade->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aSparkFade->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aSparkFade->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aSparkFade->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSparkFade->AddValue(STATE_ORBIT_RADIUS, 1.0f, MOD_TOWARD_NEXT);
	aSparkFade->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aSparkFade->AddValue(STATE_Y, 8.0f, MOD_ORBIT_SIN);

	mHud[0] = MakeHudEmitter(aSparkOut, aSparkFade, HUD_SPARK_Z, 16, 5, 0xb6, HUD_COUNTER_Y, 0, 98.0f);
	mHud[1] = MakeHudEmitter(aSparkOut, aSparkFade, HUD_SPARK_Z, 16, 5, 0xd4, HUD_COUNTER_Y, 0, 98.0f);

	// Pair 2 and 3 (+0x2cc, +0x2d0): additive embers going from yellow to red
	AnimKey* aEmberStart = new AnimKey();
	aEmberStart->SetDuration(0x96);
	aEmberStart->mDrawMode = 1;
	aEmberStart->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aEmberStart->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aEmberStart->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aEmberStart->AddValue(STATE_GREEN, 0.2f, MOD_TOWARD_NEXT);
	aEmberStart->AddValue(STATE_BLUE, 0.01f, MOD_TOWARD_NEXT);
	aEmberStart->AddValue(STATE_ALPHA, 0.4f, MOD_TOWARD_NEXT);

	AnimKey* aEmberEnd = new AnimKey();
	aEmberEnd->SetDuration(1);
	aEmberEnd->mDrawMode = 1;
	aEmberEnd->AddValue(STATE_SCALE_X, 0.3f, MOD_HOLD);
	aEmberEnd->AddValue(STATE_SCALE_Y, 0.2f, MOD_HOLD);
	aEmberEnd->AddValue(STATE_RED, 0.9f, MOD_HOLD);
	aEmberEnd->AddValue(STATE_GREEN, 0.1f, MOD_HOLD);
	aEmberEnd->AddValue(STATE_BLUE, 0.0f, MOD_HOLD);
	aEmberEnd->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);

	mHud[2] = MakeHudEmitter(aEmberStart, aEmberEnd, HUD_SPARK_Z, 5, 20, 0xb3, HUD_COUNTER_Y, 2, 101.0f);
	mHud[3] = MakeHudEmitter(aEmberStart, aEmberEnd, HUD_SPARK_Z, 5, 20, 0xcd, HUD_COUNTER_Y, 2, 101.0f);

	// Pair 4 and 5 (+0x2d4, +0x2d8): white sparks that orbit out and shrink, additive
	AnimKey* aWhiteOut = new AnimKey();
	aWhiteOut->SetDuration(0x5f);
	aWhiteOut->mDrawMode = 1;
	aWhiteOut->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aWhiteOut->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aWhiteOut->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aWhiteOut->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aWhiteOut->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aWhiteOut->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
	aWhiteOut->AddValue(STATE_ORBIT_RADIUS, 0.5f, MOD_TOWARD_NEXT);
	aWhiteOut->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aWhiteOut->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);

	AnimKey* aWhiteEnd = new AnimKey();
	aWhiteEnd->SetDuration(1);
	aWhiteEnd->mDrawMode = 1;
	aWhiteEnd->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aWhiteEnd->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aWhiteEnd->AddValue(STATE_RED, 0.2f, MOD_TOWARD_NEXT);
	aWhiteEnd->AddValue(STATE_GREEN, 0.2f, MOD_TOWARD_NEXT);
	aWhiteEnd->AddValue(STATE_BLUE, 0.2f, MOD_TOWARD_NEXT);
	aWhiteEnd->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aWhiteEnd->AddValue(STATE_ORBIT_RADIUS, 0.01f, MOD_TOWARD_NEXT);
	aWhiteEnd->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aWhiteEnd->AddValue(STATE_Y, 8.0f, MOD_ORBIT_SIN);

	mHud[4] = MakeHudEmitter(aWhiteOut, aWhiteEnd, HUD_SPARK_Z, 35, 2, 0xb3, HUD_COUNTER_Y, 0, 100.0f);
	mHud[5] = MakeHudEmitter(aWhiteOut, aWhiteEnd, HUD_SPARK_Z, 35, 2, 0xb3, HUD_COUNTER_Y, 0, 100.0f);

	// Pair 6 and 7 (+0x2dc, +0x2e0): flecks falling from the foot of the picture at its two corners, additive
	AnimKey* aFleckStart = new AnimKey();
	aFleckStart->SetDuration(0x46);
	aFleckStart->mDrawMode = 1;
	aFleckStart->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFleckStart->AddValue(STATE_SCALE_X, 0.9f, MOD_TOWARD_NEXT);
	aFleckStart->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aFleckStart->AddValue(STATE_RED, 0.9f, MOD_TOWARD_NEXT);
	aFleckStart->AddValue(STATE_GREEN, 0.8f, MOD_TOWARD_NEXT);
	aFleckStart->AddValue(STATE_BLUE, 0.7f, MOD_TOWARD_NEXT);
	aFleckStart->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);

	AnimKey* aFleckEnd = new AnimKey();
	aFleckEnd->SetDuration(1);
	aFleckEnd->mDrawMode = 1;
	aFleckEnd->AddValue(STATE_Y, -180.0f, MOD_HOLD);
	aFleckEnd->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aFleckEnd->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aFleckEnd->AddValue(STATE_RED, 0.5f, MOD_HOLD);
	aFleckEnd->AddValue(STATE_GREEN, 0.4f, MOD_HOLD);
	aFleckEnd->AddValue(STATE_BLUE, 0.3f, MOD_HOLD);
	aFleckEnd->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);

	mHud[6] = MakeHudEmitter(aFleckStart, aFleckEnd, HUD_BOTTOM_Z, 25, 3, 0x3c, HUD_BOTTOM_Y, 1, 99.0f);
	mHud[7] = MakeHudEmitter(aFleckStart, aFleckEnd, HUD_BOTTOM_Z, 25, 3, 0x146, HUD_BOTTOM_Y, 1, 99.0f);

	for (int i = 0; i < SCOREBOARD_HUD_EMITTERS; i++)
		mHud[i]->Deactivate(mHud[i]);
}

// 0x4676e0: the sparks of the counters (not the white pair, +0x2d4 and +0x2d8) start
void ScoreBoard::ActivateHud()
{
	mHud[0]->Activate(mHud[0], false);
	mHud[1]->Activate(mHud[1], false);
	mHud[2]->Activate(mHud[2], false);
	mHud[3]->Activate(mHud[3], false);
	mHud[6]->Activate(mHud[6], false);
	mHud[7]->Activate(mHud[7], false);
}

// 0x467750: all eight stop
void ScoreBoard::DeactivateHud()
{
	for (int i = 0; i < SCOREBOARD_HUD_EMITTERS; i++)
		mHud[i]->Deactivate(mHud[i]);
}

// 0x4677d0: dirt falls from above the board for 250 frames, growing and speeding up, until it is past 1200
void ScoreBoard::BuildDirtFall()
{
	mDirtFall = new Emitter(this, false);
	mDirtFall->SetStateInt(STATE_Z, 10);

	AnimKey* aFalling = new AnimKey();
	aFalling->SetDuration(0xfa);
	aFalling->mAnim = mDirtAnim;
	mDirtAnim->AddRef();
	aFalling->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFalling->AddValue(STATE_SCALE_X, 0.28f, MOD_TOWARD_NEXT);
	aFalling->AddValue(STATE_SCALE_Y, 0.28f, MOD_TOWARD_NEXT);
	aFalling->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aFalling->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aFalling->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aFalling->AddValue(STATE_ALPHA, 0.9f, MOD_HOLD);
	aFalling->mEndCondition = KEY_END_ABOVE;
	aFalling->mEndChannel = STATE_Y;
	aFalling->mEndValue = 1200.0f;
	aFalling->mEasing = EASE_IN_SQUARE;

	AnimKey* aFallen = new AnimKey();
	aFallen->SetDuration(1);
	aFallen->mAnim = mDirtAnim;
	mDirtAnim->AddRef();
	aFallen->AddValue(STATE_Y, 1200.0f, MOD_HOLD);
	aFallen->AddValue(STATE_SCALE_X, 0.4f, MOD_HOLD);
	aFallen->AddValue(STATE_SCALE_Y, 0.4f, MOD_HOLD);
	aFallen->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aFallen->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aFallen->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aFallen->AddValue(STATE_ALPHA, 0.9f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFalling, nullptr);
	aTrack->AddKey(aFallen, nullptr);
	mDirtFall->AddTrack(aTrack, nullptr);
	mDirtFall->Setup(0x190, 9, 1, 0, -200, 0x7d, 200, 0xb4);
}

// 0x467b30: two sprays of dirt thrown up from the sides (at 158 and -158 from the board's middle), each a key that
// flies out for 500 frames until it is off the screen and a key that lets it fall at 16 pixels' radius
void ScoreBoard::BuildSprays()
{
	mRightSpray = new Emitter(this, false);
	mLeftSpray = new Emitter(this, false);

	Emitter* aSprays[2] = { mRightSpray, mLeftSpray };
	const int aSprayX[2] = { 0x9e, -158 };
	for (int i = 0; i < 2; i++) {
		AnimKey* aFlyOut = new AnimKey();
		aFlyOut->SetDuration(500);
		aFlyOut->mAnim = mDirtAnim;
		mDirtAnim->AddRef();
		aFlyOut->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
		aFlyOut->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
		aFlyOut->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
		aFlyOut->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
		aFlyOut->AddValue(STATE_ORBIT_RADIUS, 1.0f, MOD_TOWARD_NEXT);
		aFlyOut->AddValue(STATE_RED, 1.0f, MOD_HOLD);
		aFlyOut->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
		aFlyOut->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
		aFlyOut->AddValue(STATE_ALPHA, 0.9f, MOD_HOLD);
		aFlyOut->mEndCondition = KEY_END_OFF_SCREEN;
		aFlyOut->mEndChannel = STATE_X;
		aFlyOut->mEndValue = 0.0f;

		AnimKey* aFall = new AnimKey();
		aFall->SetDuration(1);
		aFall->mAnim = mDirtAnim;
		mDirtAnim->AddRef();
		aFall->AddValue(STATE_SCALE_X, 0.3f, MOD_HOLD);
		aFall->AddValue(STATE_SCALE_Y, 0.3f, MOD_HOLD);
		aFall->AddValue(STATE_ORBIT_RADIUS, 16.0f, MOD_TOWARD_NEXT);
		aFall->AddValue(STATE_RED, 1.0f, MOD_HOLD);
		aFall->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
		aFall->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
		aFall->AddValue(STATE_ALPHA, 0.9f, MOD_HOLD);

		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aFlyOut, nullptr);
		aTrack->AddKey(aFall, nullptr);
		aSprays[i]->AddTrack(aTrack, nullptr);
		aSprays[i]->Setup(0x190, 4, 1, aSprayX[i], -270, 2, 2, 0xb4);
		aSprays[i]->SetState(STATE_ORBIT_ANGLE, 4.71239f); // straight up
		aSprays[i]->SetStateInt(STATE_Z, -1);
		aSprays[i]->SetAngleOwn();
	}
}

// 0x4682a0: three marks that give the progress bar its corners (the drawing reads where they ended up)
void ScoreBoard::BuildBarMarks()
{
	ObjBase** aMarks[3] = { &mBarTopLeft, &mBarTopRight, &mBarBottomLeft };
	const int aMarkX[3] = { -146, 0x95, -146 };
	const int aMarkY[3] = { -32, -32, -12 };
	for (int i = 0; i < 3; i++) {
		ObjBase* aMark = new ObjBase();
		*aMarks[i] = aMark;
		aMark->SetStateInt(STATE_X, aMarkX[i]);
		aMark->SetStateInt(STATE_Y, aMarkY[i]);
		aMark->SetStateInt(STATE_Z, 10);
		aMark->SetParent(this, nullptr);
		Activate(aMark, false);
	}
}

// 0x468410: the gems' picture, at the foot of the board
void ScoreBoard::BuildGems()
{
	mGems = new ObjBase();
	mGemsAnim->AddRef();
	mGems->mAnim = mGemsAnim;
	mGems->SetParent(this, nullptr);
	mGems->SetStateInt(STATE_X, -2);
	mGems->SetStateInt(STATE_Y, -466);
	mGems->SetStateInt(STATE_Z, 1); // the original passes the channel number SetState returned, 1
	mGems->Activate(mGems, false);
}

// 0x4684a0: the score with commas, the zeros before it blanked. The original blanks the formatting's own buffer.
void ScoreBoard::ShowScore(int theScore)
{
	char aText[32];
	strcpy(aText, FormatScore((unsigned int)theScore));
	int aPlace = 0;
	if ((int)strlen(aText) > 0) {
		do {
			if (aText[aPlace] != '0')
				break;
			aText[aPlace] = ' ';
			aPlace++;
		} while (aPlace < (int)strlen(aText));
	}
	mScoreText->SetText(aText);
}

// 0x468500
void ScoreBoard::ShowPercent(int thePercent)
{
	char aText[16];
	snprintf(aText, sizeof(aText), "%02d", thePercent);
	mPercentText->SetText(aText);
}

// 0x468550
void ScoreBoard::ShowLevel(int theLevel)
{
	char aText[16];
	snprintf(aText, sizeof(aText), "%02d", theLevel);
	mLevelText->SetText(aText);
}

// 0x4685a0: the bananas the icons are worth, formatted and then thrown away (a bug of the original: no text shows
// them, and the function is as long as the icons' counts change)
void ScoreBoard::ShowBananaTotal(int theTotal)
{
	char aText[16];
	snprintf(aText, sizeof(aText), "%02d", theTotal);
}

// 0x4685e0: past the base's update, once the board is at its resting place (its y no larger than the resting y) the
// three dirt emitters are set to emit only when told, which they never are; while it is lower (sliding), one frame in
// four it jolts to a pixel either side of the resting x
bool ScoreBoard::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mState[STATE_Y] <= mPlaceY) {
		mDirtFall->SetEmitOnCall();
		mRightSpray->SetEmitOnCall();
		mLeftSpray->SetEmitOnCall();
	} else if ((GameRandomNext() & 3) == 0) {
		mBase[STATE_X] = (float)((int)(GameRandomNext() % 3) - 1) + mPlaceX;
	}
	return true;
}

// 0x468660: what the board's own tracks and its icons' say
void ScoreBoard::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case BOARD_EVENT_ENTER_SOUND:
	case BOARD_EVENT_LEAVE_SOUND:
		gApp->mSoundQueue->Request(SOUND_SCOREBOARD, 0, false);
		break;
	case BOARD_EVENT_ENTERED:
		mAway = false;
		break;
	case BOARD_EVENT_LEFT:
		mAway = true;
		break;
	case ICON_EVENT_POPPED:
		theObj->mEmitters->BurstStarRing(0.0f, 0.0f);
		theObj->mEmitters->BurstSlowSwell(0.0f, 0.0f);
		break;
	case ICON_EVENT_GONE:
		if (mIconCount > 0)
			mIconCount--;
		ShowBananaTotal(Player::GetBananaValue() * mIconCount);
		break;
	default:
		break;
	}
}

// 0x468740: each monkey that is not shown yet and is idle wakes
void ScoreBoard::WakeMonkeys()
{
	int aCount = Player::sCurrent->mSecondUpgradeValue;
	for (int i = 0; i < aCount; i++)
		mMonkeys[i]->Wake();
}

// 0x4687b0: each monkey is dismissed
void ScoreBoard::DismissMonkeys()
{
	for (int i = 0; i < Player::sCurrent->mSecondUpgradeValue; i++)
		mMonkeys[i]->Dismiss();
}
