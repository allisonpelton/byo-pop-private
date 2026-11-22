#include "Stats.h"

#include "Anim.h"
#include "BBFont.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "ObjString.h"
#include "Player.h"
#include "Res.h"
#include "ScoreBoard.h"
#include "SoundQueue.h"
#include <cstdio>

using namespace Sexy;

// The tracks of the screen itself
enum StatsTrack
{
	STATS_TRACK_STAY = 0,       // changes the size by a thousandth, for ever: it runs from the start
	STATS_TRACK_LEAVE = 1,      // shrinks away, turning three times, then removes the screen (started by Continue)
};

// The tracks of a figure
enum FigureTrack
{
	FIGURE_TRACK_COUNT = 0,     // swells in after its delay; its last key raises the figure's event, over and over
	FIGURE_TRACK_SETTLE = 1,    // a hold, then a small swell, once the figure is done
};

// The key events a figure's counting key raises (AnimKey::mEventId); each is the number of the figure's row
enum StatsEvent
{
	STATS_EVENT_NONE = 0,       // the "Click To Continue" line: nothing to count
	STATS_EVENT_BANANAS = 1,
	STATS_EVENT_PERCENT = 2,
	STATS_EVENT_SHOTS = 3,
	STATS_EVENT_OVERFILL = 4,
	STATS_EVENT_BONUS = 5,
	STATS_EVENT_SPARE = 6,      // nothing raises it
};

// Where the first caption stands, how far apart the captions are and how far below its caption a figure stands
// (design pixels from the screen's own place)
const int STATS_FIRST_CAPTION_Y = -232;
const int STATS_ROW_SPACING = 96;
const int STATS_FIGURE_OFFSET = 40;

// The screen stands 32 above the place it is given
const int STATS_RAISE = 32;

// The frames before the first caption appears, the step to the next, where the steps stop, and the frames the
// original adds to each as it makes the caption
const int STATS_CAPTION_DELAY = 100;
const int STATS_CAPTION_DELAY_STEP = 10;
const int STATS_CAPTION_DELAY_END = 150;
const int STATS_CAPTION_DELAY_EXTRA = 50;

// The frames before the first figure and before the "Click To Continue" line appear
const int STATS_FIRST_FIGURE_DELAY = 175;
const int STATS_CLICK_DELAY = 255;

// The rows' captions; the fourth is spelled as the original spells it
static const char* const sCaptions[STATS_ROWS] = {
	"Number Of Bananas", "Percent Filled", "Arrows Shot", "OverFill", "Bonus Points"
};

// 0x46d2b0: at depth 5, a child of the level, 32 above the place given; the rings, the tracks, the five captions, the
// bananas' figure, the sparks about the score board's counters, the "Click To Continue" line, and the board's monkeys
// stepping aside. The original also formats the player's banana total ("%03d") into a buffer it never reads.
Stats::Stats(LevelObj* theLevel, int theX, int theY)
{
	mCounter = 0;
	mReady = false;
	mUpdateLevel = 2;
	SetStateInt(STATE_Z, 5);
	SetParent(theLevel, nullptr);
	SetStateInt(STATE_X, theX);
	SetStateInt(STATE_Y, theY - STATS_RAISE);
	BuildBack();
	BuildTracks();

	int aCaptionY = STATS_FIRST_CAPTION_Y;
	int aDelay = STATS_CAPTION_DELAY;
	int aRow = 0;
	do {
		AddLabel(sCaptions[aRow], 0.0f, (float)aCaptionY, aDelay, 0.8f, aRow);
		mRowY[aRow] = (float)(aCaptionY + STATS_FIGURE_OFFSET);
		aCaptionY += STATS_ROW_SPACING;
		aDelay += STATS_CAPTION_DELAY_STEP;
		aRow++;
	} while (aDelay < STATS_CAPTION_DELAY_END);

	AddFigure("000", 0.0f, mRowY[0], STATS_FIRST_FIGURE_DELAY, 0.9f, 1.0f, 1.0f, 1.0f, STATS_EVENT_BANANAS, true);
	mTallyKey->SetDuration(5);
	Player::sCurrent->mScoreBoard->ActivateHud();
	AddFigure("Click To Continue", 0.0f, (float)aCaptionY, STATS_CLICK_DELAY, 0.9f, 0.0f, 1.0f, 0.5f,
		STATS_EVENT_NONE, false);
	mShowing = true;
	Player::sCurrent->mScoreBoard->DismissMonkeys();
	Activate(this, false);
}

// 0x46d600: a figure's counting key ended (theObj is the figure). The bananas count up a step at a time, each step
// marking the last banana icon on the board; once they are counted the figure settles, and each other figure is set
// at once. Each figure, once shown, makes the next.
void Stats::Notify(ObjBase* theObj)
{
	Player* aPlayer = Player::sCurrent;
	if (theObj->mCurrentKey == nullptr)
		return;
	char aText[16];
	ObjString* aFigure = static_cast<ObjString*>(theObj);
	switch (theObj->mCurrentKey->mEventId) {
	case STATS_EVENT_BANANAS:
		if (mCounter >= aPlayer->mBananasEarned) {
			// counted: this figure settles, the percent's comes next, and the banana figure shows the total
			theObj->StartTrack(FIGURE_TRACK_SETTLE);
			AddFigure("000", 0.0f, mRowY[1], 0, 0.9f, 1.0f, 1.0f, 1.0f, STATS_EVENT_PERCENT, true);
			mTallyKey->SetDuration(1);
			mCounter = 0;
			static_cast<LevelObj*>(sLevel)->ShowBananas(true);
			sApp->mSoundQueue->Request(SOUND_STATSCLICK, 0, false);
			return;
		}
		mReady = true;
		mCounter = mCounter + 1;
		sprintf(aText, "%02d", Player::GetBananaValue() * mCounter);
		aFigure->SetText(aText);
		Player::MarkLastBananaIcon();
		sApp->mSoundQueue->Request(SOUND_STATSCLICK, 0, false);
		return;
	case STATS_EVENT_PERCENT:
		sprintf(aText, "%02d", aPlayer->mPercent);
		aFigure->SetText(aText);
		sApp->mSoundQueue->Request(SOUND_STATSCLICK, 0, false);
		theObj->StartTrack(FIGURE_TRACK_SETTLE);
		AddFigure("000", 0.0f, mRowY[2], 0, 0.9f, 1.0f, 1.0f, 1.0f, STATS_EVENT_SHOTS, true);
		mTallyKey->SetDuration(5);
		mCounter = 0;
		return;
	case STATS_EVENT_SHOTS:
		mReady = true;
		sprintf(aText, "%02d", aPlayer->mShotsFired);
		aFigure->SetText(aText);
		sApp->mSoundQueue->Request(SOUND_STATSCLICK, 0, false);
		theObj->StartTrack(FIGURE_TRACK_SETTLE);
		AddFigure("000", 0.0f, mRowY[3], 0, 0.9f, 1.0f, 1.0f, 1.0f, STATS_EVENT_OVERFILL, true);
		mTallyKey->SetDuration(5);
		return;
	case STATS_EVENT_OVERFILL:
		mReady = true;
		sprintf(aText, "%02d", aPlayer->mExtraPercent);
		aFigure->SetText(aText);
		sApp->mSoundQueue->Request(SOUND_STATSCLICK, 0, false);
		theObj->StartTrack(FIGURE_TRACK_SETTLE);
		AddFigure("0000", 0.0f, mRowY[4], 0, 0.9f, 1.0f, 1.0f, 1.0f, STATS_EVENT_BONUS, true);
		mTallyKey->SetDuration(0);
		return;
	case STATS_EVENT_BONUS:
		// The last figure never leaves its counting track and its key is 0 frames long, so this runs every frame: the
		// text is set again and the sparks put out again each time (kept).
		mReady = true;
		sprintf(aText, "%02d", aPlayer->mLevelBonus);
		aFigure->SetText(aText);
		aPlayer->mScoreBoard->DeactivateHud();
		return;
	case STATS_EVENT_SPARE:
		mReady = true;
		return;
	default:
		return;
	}
}

// 0x46d9e0: the click. The board's banana icons go away, the screen spins off, the sparks about the counters are put
// out, the banana figure shows the total again without its sound, and the level's fireworks are stopped.
void Stats::Continue()
{
	Player::sCurrent->mScoreBoard->ClearBananaIcons();
	StartTrack(STATS_TRACK_LEAVE);
	Player::sCurrent->mScoreBoard->DeactivateHud();
	static_cast<LevelObj*>(sLevel)->ShowBananas(false);
	mShowing = false;
	static_cast<LevelObj*>(sLevel)->mFireworksOn = false;
}

// 0x46e5c0: the picture of rings, a child of the screen, invisible and tiny at first: 85 frames turning in from nothing,
// then breathing between alpha 0.4 and 0.41 at 1.2 times its size
void Stats::BuildBack()
{
	mBack = new ObjBase();
	mBack->mUpdateLevel = 2;
	mBack->SetParent(this, nullptr);
	mBack->SetState(STATE_ALPHA, 0.0f);
	mBack->SetStateInt(STATE_SCALE_X, 0);
	mBack->SetStateInt(STATE_SCALE_Y, 0);
	mBack->mAnim = Anim::Get("images/stat_back", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mBack->Activate(mBack, false);

	AnimKey* aTurnIn = new AnimKey();
	aTurnIn->SetDuration(85);
	aTurnIn->AddValue(STATE_ALPHA, -0.0f, MOD_TOWARD_NEXT);
	aTurnIn->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aTurnIn->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aTurnIn->AddValue(STATE_ROTATION, 12.566371f, MOD_TOWARD_NEXT);

	AnimKey* aBreatheIn = new AnimKey();
	aBreatheIn->SetDuration(100);
	aBreatheIn->AddValue(STATE_ALPHA, 0.4f, MOD_TOWARD_NEXT);
	aBreatheIn->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aBreatheIn->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	aBreatheIn->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimKey* aBreatheOut = new AnimKey();
	aBreatheOut->SetDuration(100);
	aBreatheOut->AddValue(STATE_ALPHA, 0.41f, MOD_TOWARD_NEXT);
	aBreatheOut->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aBreatheOut->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	aBreatheOut->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aTurnIn, nullptr);
	aTrack->AddKey(aBreatheIn, nullptr);
	aTrack->AddKey(aBreatheOut, aBreatheIn);
	mBack->AddTrack(aTrack, nullptr);
}

// 0x46e320: the screen's own two tracks (a key's values come on top of the object's own, so a scale of 0 keeps its
// size). The first, running from the start, only changes the size by up to a thousandth. The second, started by
// Continue, shrinks it away over 75 frames, then turns it three times in one more and removes it.
void Stats::BuildTracks()
{
	AnimKey* aStayA = new AnimKey();
	aStayA->mEventId = 0;
	aStayA->SetDuration(100);
	aStayA->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aStayA->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aStayB = new AnimKey();
	aStayB->mEventId = 0;
	aStayB->SetDuration(100);
	aStayB->AddValue(STATE_SCALE_X, 0.001f, MOD_TOWARD_NEXT);
	aStayB->AddValue(STATE_SCALE_Y, 0.001f, MOD_TOWARD_NEXT);

	AnimTrack* aStayTrack = new AnimTrack();
	aStayTrack->AddKey(aStayA, nullptr);
	aStayTrack->AddKey(aStayB, aStayA);
	AddTrack(aStayTrack, nullptr);

	AnimKey* aShrink = new AnimKey();
	aShrink->mEventId = 0;
	aShrink->SetDuration(75);
	aShrink->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aGone = new AnimKey();
	aGone->mEventId = 0;
	aGone->SetDuration(1);
	aGone->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_ROTATION, -18.849556f, MOD_HOLD);
	aGone->SetRemoveAtEnd();

	AnimTrack* aLeaveTrack = new AnimTrack();
	aLeaveTrack->AddKey(aShrink, nullptr);
	aLeaveTrack->AddKey(aGone, nullptr);
	AddTrack(aLeaveTrack, nullptr);
}

// 0x46df90: a caption in the game's font, red, at its place, at depth 200 and tiny. After its delay (theDelay and 50
// more) it swells to twice theScale in 12 frames while turning 80 degrees (one way for the even rows, the other for
// the odd), steadies at theScale with a drum sound (sound 3), and settles a little below it.
void Stats::AddLabel(const char* theText, float theX, float theY, int theDelay, float theScale, int theIndex)
{
	theDelay += STATS_CAPTION_DELAY_EXTRA;
	ObjString* aLabel = new ObjString();
	aLabel->mUpdateLevel = 2;
	aLabel->SetParent(this, nullptr);
	aLabel->SetFont(sApp->mFont);
	aLabel->Create(theText, false, 0);
	aLabel->SetState(STATE_X, theX);
	aLabel->SetState(STATE_Y, theY);
	aLabel->SetStateInt(STATE_Z, 200);
	aLabel->SetStateInt(STATE_SCALE_X, 0);
	aLabel->SetStateInt(STATE_SCALE_Y, 0);
	aLabel->SetStateInt(STATE_RED, 1);
	aLabel->SetStateInt(STATE_GREEN, 0);
	aLabel->SetStateInt(STATE_BLUE, 0);
	aLabel->SetStateInt(STATE_ALPHA, 0);

	AnimKey* aSwell = new AnimKey();
	aSwell->SetDuration(12);
	aSwell->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_SCALE_X, theScale + theScale, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_SCALE_Y, theScale + theScale, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_ROTATION, (theIndex & 1) ? 1.3962635f : -1.3962635f, MOD_TOWARD_NEXT);

	AnimKey* aSteady = new AnimKey();
	aSteady->mEndCondition = KEY_SOUND;
	aSteady->mEndChannel = SOUND_DRUM_C;
	aSteady->mEndValue = 2.0f;
	aSteady->SetDuration(1);
	aSteady->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aSteady->AddValue(STATE_SCALE_X, theScale, MOD_TOWARD_NEXT);
	aSteady->AddValue(STATE_SCALE_Y, theScale, MOD_TOWARD_NEXT);
	aSteady->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimKey* aSettle = new AnimKey();
	aSettle->SetDuration(25);
	aSettle->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aSettle->AddValue(STATE_SCALE_X, (float)((double)theScale - 0.1), MOD_HOLD);
	aSettle->AddValue(STATE_SCALE_Y, (float)((double)theScale - 0.1), MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->SetDelay(theDelay, false);
	aTrack->AddKey(aSwell, nullptr);
	aTrack->AddKey(aSteady, nullptr);
	aTrack->AddKey(aSettle, aSettle);
	aLabel->AddTrack(aTrack, nullptr);
}

// 0x46da50: a figure in the game's font, in the colour given, at its place, at depth 200 and tiny; its text starts as
// "00" when theStartAtZero, and its key events go to the screen. The event number is kept in the spare button-id word.
// Track 0: after theDelay frames it swells in with a wobble, rings (sound 78) as it reaches theScale, and its last
// key, 5 frames long, is held and raises theEventId each time it ends; the screen keeps that key (mTallyKey) to
// change its length. Track 1 (the settle): a hold, then a small swell.
void Stats::AddFigure(const char* theText, float theX, float theY, int theDelay, float theScale, float theRed,
	float theGreen, float theBlue, int theEventId, bool theStartAtZero)
{
	ObjString* aFigure = new ObjString();
	aFigure->mButtonId = theEventId;
	aFigure->mUpdateLevel = 2;
	aFigure->SetParent(this, nullptr);
	aFigure->SetFont(sApp->mFont);
	aFigure->Create(theText, false, 0);
	if (theStartAtZero)
		aFigure->SetText("00");
	aFigure->SetState(STATE_X, theX);
	aFigure->SetState(STATE_Y, theY);
	aFigure->SetStateInt(STATE_Z, 200);
	aFigure->SetStateInt(STATE_SCALE_X, 0);
	aFigure->SetStateInt(STATE_SCALE_Y, 0);
	aFigure->SetState(STATE_RED, theRed);
	aFigure->SetState(STATE_GREEN, theGreen);
	aFigure->SetState(STATE_BLUE, theBlue);
	aFigure->SetStateInt(STATE_ALPHA, 0);
	aFigure->SetEventTarget(this);

	AnimKey* aSwellIn = new AnimKey();
	aSwellIn->SetDuration(12);
	aSwellIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSwellIn->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aSwellIn->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSwellIn->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aSwellIn->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);

	AnimKey* aRing = new AnimKey();
	aRing->mEndCondition = KEY_SOUND;
	aRing->mEndChannel = SOUND_STATS;
	aRing->mEndValue = 2.0f;
	aRing->SetDuration(10);
	aRing->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aRing->AddValue(STATE_SCALE_X, theScale, MOD_TOWARD_NEXT);
	aRing->AddValue(STATE_SCALE_Y, theScale, MOD_TOWARD_NEXT);

	AnimKey* aCount = new AnimKey();
	aCount->mEventId = theEventId;
	aCount->SetDuration(5);
	aCount->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aCount->AddValue(STATE_SCALE_X, theScale, MOD_TOWARD_NEXT);
	aCount->AddValue(STATE_SCALE_Y, theScale, MOD_TOWARD_NEXT);
	mTallyKey = aCount;

	AnimTrack* aCountTrack = new AnimTrack();
	aCountTrack->SetDelay(theDelay, false);
	aCountTrack->AddKey(aSwellIn, nullptr);
	aCountTrack->AddKey(aRing, nullptr);
	aCountTrack->AddKey(aCount, aCount);
	aFigure->AddTrack(aCountTrack, nullptr);

	AnimKey* aHold = new AnimKey();
	aHold->SetDuration(40);
	aHold->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aHold->AddValue(STATE_SCALE_X, theScale, MOD_TOWARD_NEXT);
	aHold->AddValue(STATE_SCALE_Y, theScale, MOD_TOWARD_NEXT);

	AnimKey* aSwell = new AnimKey();
	aSwell->SetDuration(10);
	aSwell->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_SCALE_X, (float)((double)theScale - 0.1), MOD_HOLD);
	aSwell->AddValue(STATE_SCALE_Y, (float)((double)theScale - 0.1), MOD_HOLD);

	AnimTrack* aSettleTrack = new AnimTrack();
	aSettleTrack->AddKey(aHold, nullptr);
	aSettleTrack->AddKey(aSwell, aSwell);
	aFigure->AddTrack(aSettleTrack, nullptr);
}
