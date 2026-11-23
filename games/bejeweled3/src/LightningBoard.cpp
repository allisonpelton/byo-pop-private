// Sexy::SpeedBoard (the port's LightningBoard): see the header. Every function says its slot and address in the exe.
#include "LightningBoard.h"
#include "TimeBonusEffect.h"

#include "Bej3App.h"
#include "EndLevelDialog.h"
#include "CrystalBall.h"
#include "PopAnim.h"
#include "Res.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Sexy {

// 0x41c090 (bejeweled3): the counter goes up by theStep and the chance of a gem of the length follows the curve: with the curve's value g
// above 0 the chance is g times the 1.2 power of (1.5 counter g), held between 0.2 and 2.5, else 0
void TimeGemChance::Step(float theStep)
{
	mCounter = (float)((double)theStep + (double)mCounter);
	double aCurve = mCurve.GetOutVal();
	if (aCurve <= 0.0) {
		mChance = 0.0f;
		return;
	}
	float aPower = (float)std::pow((double)mCounter * 1.5 * aCurve, 1.2);
	if (aPower < 0.2f)
		aPower = 0.2f;
	if (aPower > 2.5f)
		aPower = 2.5f;
	mChance = (float)(aCurve * (double)aPower);
}

// the setting's text as the exe reads it (a missing one is the empty text, which is 0)
int LightningBoard::SettingInt(const std::map<std::string, std::string>& theSettings, const char* theName)
{
	auto anIter = theSettings.find(theName);
	return anIter == theSettings.end() ? 0 : atoi(anIter->second.c_str());
}

double LightningBoard::SettingDouble(const std::map<std::string, std::string>& theSettings, const char* theName)
{
	auto anIter = theSettings.find(theName);
	return anIter == theSettings.end() ? 0.0 : atof(anIter->second.c_str());
}

// 0x718090 (bejeweled3): Board's constructor 0x74ae30, then the vftable and the Title "Lightning" (the quest's own settings replace the
// map before the game is reset: Bej3App::StartLightning)
LightningBoard::LightningBoard()
{
	mSettings["Title"] = "Lightning";
	// 0x741840, the case of the Lightning modes: the Lightning pictures in place of Classic's
	for (ButtonWidget* aButton : {mHintButton, mMenuButton, mResetButton}) {
		RemoveWidget(aButton);
		delete aButton;
	}
	mHintButton = nullptr;
	mMenuButton = nullptr;
	mResetButton = nullptr;
	MakeButtons(IMAGE_INGAMEUI_HINT_BUTTON_LIGHTNING, IMAGE_INGAMEUI_HINT_BUTTON_LIGHTNING_ID, IMAGE_INGAMEUI_MENU_BUTTON_LIGHTNING,
		IMAGE_INGAMEUI_MENU_BUTTON_LIGHTNING_ID, IMAGE_INGAMEUI_RESET_BUTTON_LIGHTNING, IMAGE_INGAMEUI_RESET_BUTTON_LIGHTNING_ID);
}

// slot 85 (0x154), 0x718500 (bejeweled3): Board's reset (0x74a880), then the mode's state and the quest's settings are read. port: the
// other curves it sets (three constants at +0x3380 and after), the "unpowered" text and the three flags of the Last Hurrah's look
// (+0x30e0 and the two after it, nothing translated reads them) are left out
void LightningBoard::ResetGame()
{
	Board::ResetGame();
	mPenalty = 0.0;
	mLastHurrahPoints = 0;
	mRoundNumber = 0;
	mUsePoints = false;
	mTimeUpShown = false;
	mEndingAge = 0;
	mBonusSeconds = 0;
	mBonusSecondsTotal = 0;
	mTicksPlayed = 0;
	mGemSpacing = 0;
	mStartDelay = 0xfa;
	mSpecialsStarted = false;
	mLastHurrah = false;
	mEnding = false;
	mUsePoints = SettingInt(mSettings, "UsePM") != 0;
	mPointsGoalStart = SettingInt(mSettings, "PointsGoalStart");
	mPointsGoalPerLevel = SettingInt(mSettings, "AddPointsGoalPerLevel");
	mTimeStart = SettingInt(mSettings, "TimeStart");
	// the three chances: each a counter of its length with its curve (an empty setting is a curve of 0)
	TimeGemChance* aChances[3] = {&mChance5, &mChance10, &mChance15};
	const char* aCurveNames[3] = {"5SecChanceCurve", "10SecChanceCurve", "15SecChanceCurve"};
	for (int i = 0; i < 3; i++) {
		auto anIter = mSettings.find(aCurveNames[i]);
		std::string aCurve = anIter == mSettings.end() ? std::string() : anIter->second;
		if (aCurve.size() >= 2 && aCurve.front() == '"' && aCurve.back() == '"')
			aCurve = aCurve.substr(1, aCurve.size() - 2);
		if (aCurve.empty())
			aChances[i]->mCurve.SetConstant(0.0);
		else
			aChances[i]->mCurve.SetCurve(aCurve);
		aChances[i]->mCounter = 0.0f;
		aChances[i]->mChance = 0.0f;
	}
	mTimeStep = (float)SettingDouble(mSettings, "TimeStep");
	mLevelTimeStep = (float)SettingDouble(mSettings, "LevelTimeStep");
	mHasGoalAndTime = mPointsGoalStart > 0 && mTimeStart > 0;
	mTimeLimitTicks = 6000;
	mClock = 0.0f;
	mClockScaleOverride = 0.0f;
	StartHudLabel("unpowered", 0);   // 0x718500 ends with 0x718400
}

// slot 187 (0x2ec), 0x713fa0 (bejeweled3): the game ticks left of the round, 100 for each second of the time limit: the limit less the
// clock (which starts 250 late, held between 0 and the limit), and never more than the round's own length (+0x30f4: 6000, or the Last
// Hurrah's). -1 where no limit runs. port: the exe first returns +0x1308 when +0x12f8 is set (a replay: not made)
int LightningBoard::GetTimeLeft() const
{
	int aLimit = GetTimeLimit();
	if (aLimit == 0)
		return -1;
	double aLimitTicks = (double)(float)(aLimit * 100);
	double aPassed = (double)mClock - 250.0;
	if (aPassed < 0.0)
		aPassed = 0.0;
	if (aPassed > aLimitTicks)
		aPassed = aLimitTicks;
	int aLeft = Ftol(aLimitTicks - aPassed);
	return mTimeLimitTicks < aLeft ? mTimeLimitTicks : aLeft;
}

// slot 188 (0x2f0), 0x716f70 (bejeweled3): where the bar is going; and the round ends here: with the time out (the share of it left is 0),
// or in the form with a points goal when the bar's target is out or the time is up, and the board is at rest, the end is asked for
// (slot 161, with the flag +0xe28 set). The return value is the share of the time left (0 once the time-up is shown). port: the exe's
// bar in the form with a goal weights it by half and the time by half (0x8d3370); only the music and the look read it, since the level
// bar's size is 0 for the shipped quest. The music's volume steps (0x76aaf0), the minute-mark sound (0x76ab60), the text of
// slot 82 and 0x54f810 every 20th update are left out
float LightningBoard::GetBarTarget()
{
	int aLevelMax = GetLevelMax();
	if (aLevelMax < 1) {
		int aLimit = GetTimeLimit();
		float aShare = 0.0f;
		if (aLimit != 0) {
			int aLeft = GetTimeLeft();
			float aFraction = (float)aLeft / ((float)aLimit * 100.0f);
			aShare = aFraction >= 0.0f ? aFraction : 0.0f;
			if (aShare <= 0.0f && IsSettled() && mGameOverTicks == 0) {
				mEnding = true;
				NoMoreMoves(true);
			}
		}
		if (aShare > 1.0f)
			aShare = 1.0f;
		return mTimeUpShown ? 0.0f : aShare;
	}
	// the form with a goal: the bar is the progress over the goal, a half of the way
	float aTarget = (float)(0.5 + ((double)GetLevelProgress() / (double)aLevelMax) * 0.5);
	if (aTarget > 1.0f)
		aTarget = 1.0f;
	if (aTarget < 0.0f)
		aTarget = 0.0f;
	if ((mTimeUpShown || aTarget <= 0.0f) && IsSettled() && mGameOverTicks == 0) {
		mEnding = true;
		NoMoreMoves(true);
	}
	return aTarget;
}

// slot 22 (0x58), 0x7173f0 (bejeweled3): the board's update (0x759170), then, when the game's own tick went on: the clock goes up (by its
// scale: the time's slowing at the end of a round, or the override that a big explosion sets), the three chances step by the setting
// TimeStep over 100, the ticks are counted, and every 150th tick the aging gems count down. Every 400 ticks the spacing is lowered by
// draws of the board's own generator (two when the first leaves it above 5). The end of the Last Hurrah counts its age; the bar's
// target is asked for (which ends the round); and while a big explosion runs (+0x1188 above 0) the clock slows so that no more than
// 800 times it of the time passes. port: the exe's other work here (the curves of the time and score displays at +0x3450 and after, the
// level bar's colours, the 20 songs' filters, the sprites' sizes, the board's shake in the last 100 ticks of the end, which draws the
// app's generator twice every third update, the points goal's bookkeeping when it is on, the round's bonus display) is not translated
void LightningBoard::Update()
{
	int aTicksBefore = mGameTicks;
	Board::Update();
	if (mGameTicks != aTicksBefore) {
		float aStep;
		if (mClockScaleOverride == 0.0f) {
			aStep = 1.0f;
			float aSlowed = ((float)GetTimeLeft() / 600.0f) * 0.3f + 0.7f;
			if (aSlowed <= 1.0f)
				aStep = aSlowed;
		}
		else
			aStep = mClockScaleOverride;
		mClock = (float)((double)aStep + (double)mClock);
		float aChanceStep = (float)((double)mTimeStep / 100.0);
		mChance5.Step(aChanceStep);
		mChance10.Step(aChanceStep);
		mChance15.Step(aChanceStep);
		mTicksPlayed++;
		if (mGameTicks % 0x96 == 0) {
			// 0x7173f0: the gems with bit 0x40000 count down their seconds and lose the bits when they are out (nothing translated sets it)
			for (int aRow = 0; aRow < ROWS; aRow++) {
				for (int aCol = 0; aCol < COLS; aCol++) {
					Piece* aPiece = mGrid[aRow][aCol];
					if (aPiece != nullptr && (aPiece->mFlags & Piece::FLAG_TIME_GEM_AGING) != 0) {
						aPiece->mTimeGemSeconds--;
						if (aPiece->mTimeGemSeconds < 1)
							aPiece->mFlags &= ~(Piece::FLAG_TIME_GEM | Piece::FLAG_TIME_GEM_AGING);
					}
				}
			}
		}
	}
	// 0x7173f0: the four animations step, in the board's alpha
	{
		PopAnim* aHud[] = {POPANIM_ANIMS_GEM_MEGA, POPANIM_ANIMS_LIGHTNINGUIBOTTOM, POPANIM_ANIMS_LIGHTNINGUI, POPANIM_ANIMS_LIGHTNINGUITOP};
		for (PopAnim* anAnim : aHud) {
			anAnim->Update();
			anAnim->SetAnimColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
		}
	}
	if (mTimeUpShown)
		mEndingAge++;
	GetBarTarget();   // 0x7173f0 asks for it for the bar (and the round ends in it)
	if (mGemSpacing == 0)
		mGemSpacing = 0x14;
	if (mGameTicks % 400 == 0 && aTicksBefore != mGameTicks) {
		if ((int)((unsigned)mGemSpacing - mRand.Next() % 5) < 5)
			mGemSpacing = 5;
		else
			mGemSpacing = (int)((unsigned)mGemSpacing - mRand.Next() % 5);
	}
	// 0x717e45 (bejeweled3): the screen shakes in the first 99 updates of the ending (+0x30a4 above 0 and below 100) in every third update of
	// the board: the board's own place (+0x40, +0x44) is moved by a draw of the app's Rand() each, as far as 12 of 1200 of the art size
	// (a draw from -0.5 to 0.5 over 100 less the ending's age, the share of the 100 that is left)
	if (mEndingAge > 0 && mEndingAge < 100 && mUpdateCnt % 3 == 0) {
		const double anArtShare = (double)gApp->mArtRes * 12.0 / 1200.0;
		int aShakeX = 0;
		int aShakeY = 0;
		for (int* aShake : {&aShakeX, &aShakeY}) {
			float aDraw = (float)((double)(Rand() % 1000000 - 500000) / 500000.0);
			*aShake = Ftol(anArtShare * (((double)(100 - mEndingAge) * (double)aDraw) / 100.0));
		}
		mX = aShakeX;
		mY = aShakeY;
	}
	// the end of the update: a big explosion running slows the clock
	if (mCheckerFlash > 0.0f) {
		if (mClockScaleOverride != 0.0f)
			return;
		int aLeft = GetTimeLeft();
		int aScaled = Ftol((double)mCheckerFlash * 800.0);
		if ((double)aScaled <= (double)aLeft * 1.3)
			return;
		mClockScaleOverride = (float)((double)aLeft / (double)aScaled);
		return;
	}
	mClockScaleOverride = 0.0f;
}

// slot 185 (0x2e4), 0x716c60 (bejeweled3): the fill's new pieces are lifted (by 120 of the 1200 of the screen's height, above where the
// fill put them), and, once the game is past its first 100 ticks and the end is not being played, the three chances are stepped by 1 for
// each new piece and each gives a draw that may make the batch's gem a time gem of 10 seconds (the draw is under the 10-second chance)
// or of 5 (the first is not, and the second is under the 5-second chance); then one new piece of the batch, drawn, becomes it if it is a plain gem.
// Every draw is of the board's own generator, the draws of one piece in the order of the code: the 10-second one, then the 5-second one
// when needed. The first time gem queues its tutorial box (bit 9, 0x716c60). port: the sound is left out
bool LightningBoard::OnFillAccepted(std::vector<Piece*>& theNewPieces)
{
	const int aLift = (gApp->mArtRes * 0x78) / 0x4b0;
	for (size_t i = 0; i < theNewPieces.size(); i++)
		theNewPieces[i]->mY = (float)aLift + theNewPieces[i]->mY;
	if (mGameTicks > 100 && !mEnding && !theNewPieces.empty()) {
		int aSeconds = 0;
		for (size_t i = 0; i < theNewPieces.size(); i++) {
			mChance5.Step(1.0f);
			mChance10.Step(1.0f);
			mChance15.Step(1.0f);
			unsigned long aRoll = mRand.Next();
			if ((double)mChance10.mChance <= (double)(float)(aRoll % 100000) / 100000.0) {
				aRoll = mRand.Next();
				if ((double)(float)(aRoll % 100000) / 100000.0 < (double)mChance5.mChance) {
					aSeconds = SHORT_GEM_SECONDS;
					mChance5.mCounter = 0.0f;
				}
			}
			else {
				aSeconds = LONG_GEM_SECONDS;
				mChance10.mCounter = 0.0f;
			}
		}
		if (aSeconds != 0) {
			unsigned long aPick = mRand.Next() % (unsigned long)theNewPieces.size();
			Piece* aGem = theNewPieces[aPick];
			if (aGem->mFlags == 0) {
				aGem->mFlags = Piece::FLAG_TIME_GEM;
				aGem->mTimeGemSeconds = aSeconds;
				TimeBonusEffect::MakeFor(this, aGem);   // 0x741130: its glow, bolts and number
				if (CanShowHint(HINT_TIME_GEM))
					QueueHint(aGem, HINT_TIME_GEM);
			}
		}
	}
	return true;
}

// slot 143 (0x23c), 0x715360 (bejeweled3): a time gem that is cleared (the exe's slot 142, 0x73d2b0, calls it for every piece the grid
// lets go): the match counts it (counter 0x22), its seconds are put to the Last Hurrah's account and the game's, and from the 60th second
// of the account on each time gem is worth 50 points a second. port: the effects (the time gem's burst with the app's draws in 0x714520
// and its sibling, the flying number, the sounds 0x76aaf0 for 5 and 10 seconds and the shine at the board's corner) are left out
void LightningBoard::OnPieceCleared(Piece* thePiece)
{
	if ((thePiece->mFlags & Piece::FLAG_TIME_GEM) != 0) {
		AddCounter(COUNTER_TIME_GEMS, 1, thePiece->mColumnTag);
		mBonusSeconds += thePiece->mTimeGemSeconds;
		mBonusSecondsTotal += thePiece->mTimeGemSeconds;
		if (mBonusSeconds - 0x3c > 0)
			AddPoints(thePiece->mTimeGemSeconds * 0x32, thePiece->mColumnTag);
		// 0x719150: the HUD pulses, the harder the more seconds are in the account (at most "pulse5")
		int aPulse = mBonusSeconds * 5 / 60 + 1;
		StartHudLabel(StrFormat("pulse%d", aPulse > 5 ? 5 : aPulse).c_str(), 12);
	}
}

// slot 161 (0x284), 0x719350 (bejeweled3): the round is out (or no move is left): when no level change is going, the time gems left on
// the board are made into a Flame Gem (under 10 seconds) or a Star Gem (more) when there is a Last Hurrah to come, else they only
// go (and give 50 points a second). With seconds in the account the Last Hurrah begins: the chances step by the setting LevelTimeStep
// and move on their curves, the multiplier (+0xe0c) goes up by one, and the round's length is the account's seconds, the clock being
// put where that many seconds are left. With none the game ends: the score is noted (+0x3080), then every special gem (bits 0x17, 0x800,
// 0x1000, 0x20000, 0x80000) gets a busy count of 25 for each one in the order of the grid (plus 150 on the first update) and goes off by
// it; when none is left the game ends (0x7602c0), at the end of the wait that the curve at +0x33e0 shortens to 200. port: the round's
// record (slot 153, 0x745f10, which only the end dialog's statistics read), the banners, texts, sounds and the dialog at +0x309c, the
// call of slot 206 for the effects at +0x1080, the bits 0x400 (no gem has them here) and the exe's curve at +0x33e0 (held by
// mSpecialsStarted: it is above 0 once the specials have been started) are not translated
void LightningBoard::NoMoreMoves(bool theAnnounce)
{
	(void)theAnnounce;
	if (mLevelUpPending || mHyperspace != nullptr)
		return;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr || (aPiece->mFlags & Piece::FLAG_TIME_GEM) == 0)
				continue;
			if (mBonusSeconds == 0)
				AddPoints(aPiece->mTimeGemSeconds * 0x32, aPiece->mColumnTag);
			else if (aPiece->mTimeGemSeconds < 10)
				MakeFlameGem(aPiece);   // slot 166 (0x298)
			else
				MakeStarGem(aPiece);    // slot 168 (0x2a0)
			aPiece->mFlags &= ~Piece::FLAG_TIME_GEM;
			aPiece->mTimeGemSeconds = 0;
		}
	}
	if (mBonusSeconds < 1) {
		if (mCheckerFlash > 0.0f)
			return;
		if (!mTimeUpShown) {
			mLastHurrahPoints = mScorePoints;
			mTimeUpShown = true;
			mLastHurrah = false;
			StartHudLabel("death", 15);   // 0x719350
		}
		const unsigned kSpecialBits = 0x17 | 0xa1800;
		int aBusy = 0;
		int aCount = 0;
		for (int aRow = 0; aRow < ROWS; aRow++) {
			for (int aCol = 0; aCol < COLS; aCol++) {
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == nullptr || ((unsigned)aPiece->mFlags & kSpecialBits) == 0)
					continue;
				int aWait = (aCount + 1) * 0x19;
				aPiece->mBusyCount = mEndingAge != 0 ? aWait : aWait + 0x96;
				aCount++;
				aBusy++;
			}
		}
		if (aBusy > 0)
			mSpecialsStarted = true;
		if (aBusy == 0) {
			// 0x714430: the end dialog's wait is counted down and the score is handed in
			mEndDialogWait--;
			if (mEndDialogWait < 1) {
				mEndDialogWait = 0x113;
				SubmitScore(mScorePoints);
			}
			EndGame();
			if (mSpecialsStarted)
				mGameOverTicks = 200;
		}
		return;
	}
	// the Last Hurrah: the chances move on, the multiplier goes up, and the account's seconds are the round
	mChance5.Step(mLevelTimeStep);
	mChance10.Step(mLevelTimeStep);
	mChance15.Step(mLevelTimeStep);
	mLastHurrah = true;
	StartHudLabel("energize", 15);   // 0x719350
	mChance5.mCurve.IncInVal();
	mChance10.mCurve.IncInVal();
	mChance15.mCurve.IncInVal();
	mEnding = false;
	mLevelNumber++;
	int aClock = (0x3c - mBonusSeconds) * 100;
	mTimeLimitTicks = mBonusSeconds * 100;
	mBonusSeconds = 0;
	if (aClock < 0)
		aClock = 0;
	mGameTicks = aClock;
	mClock = (float)aClock;
	mClockScaleOverride = 0.0f;
}

// slot 233 (0x3a4), 0x716ab0 (bejeweled3): the Lightning end dialog (SpeedEndLevelDialog), given the seconds of all the time gems matched (+0x30b0)
ClassicEndLevelDialog* LightningBoard::MakeEndLevelDialog()
{
	return new LightningEndLevelDialog(this, mBonusSecondsTotal);
}


namespace {

int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

// the left panel's middle and the shift the panel's pictures have in resources.xml (0x741440, 0x741840)
const int PANEL_MIDDLE_X = 0xfa;
const int PANEL_PICTURE_SHIFT = -400;
// the HUD animations are drawn this far left of the board's own origin (1200ths: 0x714840, 0x715cb0)
const int HUD_SHIFT_X = -0xa0;
// 0x73b7c0 (bejeweled3): the clock warns in the last 1000 ticks (10 of the 60 seconds), 1500 for a round of more than 60 seconds
const int WARNING_TICKS = 1000;
const int WARNING_TICKS_LONG = 1500;

// 0x713850 (bejeweled3) at a scale of 1: the picture drawn with its middle at the point
void DrawCentered(Graphics* g, Image* theImage, int theX, int theY)
{
	if (theImage != nullptr)
		g->DrawImage(theImage, theX - theImage->GetWidth() / 2, theY - theImage->GetHeight() / 2);
}

}  // namespace

// 0x718400 (bejeweled3): the four animations go to the label; the exe's order is the bottom, the middle, the top and the mega gem's.
// The top one gets "death" once the game is over, else "unpowered"
void LightningBoard::StartHudLabel(const char* theLabel, int theTime)
{
	POPANIM_ANIMS_LIGHTNINGUIBOTTOM->TransitionToLabel(theLabel, theTime);
	POPANIM_ANIMS_LIGHTNINGUI->TransitionToLabel(theLabel, theTime);
	POPANIM_ANIMS_LIGHTNINGUITOP->TransitionToLabel(std::string(theLabel) == "death" ? "death" : "unpowered", theTime);
	POPANIM_ANIMS_GEM_MEGA->TransitionToLabel(theLabel, theTime);
	mHudLabel = theLabel;
}

// slot 251 (0x3ec), 0x741440 (bejeweled3): the bottom widget of Lightning
void LightningBoard::DrawBottomWidget(Graphics* g)
{
	if (IMAGE_INGAMEUI_BOTTOM_WIDGET_LIGHTNING != nullptr)
		g->DrawImage(IMAGE_INGAMEUI_BOTTOM_WIDGET_LIGHTNING,
			Scaled(PANEL_MIDDLE_X + PANEL_PICTURE_SHIFT + GetImageOffsetX(IMAGE_INGAMEUI_BOTTOM_WIDGET_LIGHTNING_ID)),
			Scaled(GetImageOffsetY(IMAGE_INGAMEUI_BOTTOM_WIDGET_LIGHTNING_ID)));
}

// slots 238 (0x714840) and 250 (0x713f90) (bejeweled3): Lightning has no top widget, level number, frames or level bar: the bottom
// widget (slot 250 goes to slot 251) and the bottom animation, drawn 0xa0 left of the origin
void LightningBoard::DrawScreenFrame(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	DrawBottomWidget(g);
	g->PushState();
	g->Translate(Scaled(HUD_SHIFT_X), 0);
	POPANIM_ANIMS_LIGHTNINGUIBOTTOM->Draw(g);
	g->PopState();
}

// slot 255 (0x3fc), 0x714660 (bejeweled3): the score at (0xf2, 0xd2), in the board's alpha. port: the font is the score's (the exe draws
// with the one the overlay set)
void LightningBoard::DrawScoreText(Graphics* g)
{
	g->SetFont(FONT_SCORE);
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	g->WriteString(CommaSeperate(mScore), Scaled(0xf2), Scaled(0xd2), -1, 0, true, 0, -1, -1);
}

// slot 218 (0x714130): no warning with an account of seconds; else 0x73b7c0: a round with a length whose time left is under the warning's
bool LightningBoard::IsWarning() const
{
	if (mBonusSeconds != 0)
		return false;
	const int aLimit = GetTimeLimit();
	return aLimit > 0 && GetTimeLeft() < (aLimit <= 60 ? WARNING_TICKS : WARNING_TICKS_LONG);
}

// 0x73d9f0 (bejeweled3): the warning's colour is white and its alpha this: the board's alpha (slot 175) times 127 (1 + sin(0.15 of the
// board's update count)) times how far into the warning the clock is, (window - time left) / window; once the time is out that fades
// over 450 more game ticks of the round's length (a flag at +0x12f8 that would make it 0 is not translated: it is not set)
int LightningBoard::GetWarningAlpha(float theBoardAlpha) const
{
	const int aLimit = GetTimeLimit();
	const int aWindow = aLimit <= 60 ? WARNING_TICKS : WARNING_TICKS_LONG;
	const int aLeft = GetTimeLeft();
	float aFraction = (float)((double)(aWindow - aLeft) / (double)aWindow);
	if (aLeft == 0) {
		int aFade = aLimit * 100 - mGameTicks + 450;
		if (aFade < 0)
			aFade = 0;
		aFraction = (float)((double)aFade / 200.0 * (double)aFraction);
	}
	const float aSine = (float)std::sin((double)(float)((double)mUpdateCnt * (double)0.15f));
	const int anAlpha = Ftol((double)theBoardAlpha * (127.0 * (double)aSine + 127.0) * (double)aFraction);
	return anAlpha < 0 ? 0 : (anAlpha > 255 ? 255 : anAlpha);
}

// slot 43 (0x2c), 0x716ef0 (bejeweled3): the mode's overlay 0x715cb0 (the order below is the exe's), then the top animation 0xa0 left
// of the origin. port: not translated: the PIEffects of the mega gem's shard (PIEFFECT_LIGHTNING_POWERED_MEGASHARD), the side bolts
// (..._LEFTRIGHT) and the countdown's glow (PIEFFECT_COUNTDOWNBAR), which the exe edits keyframe by keyframe in its update; the
// time gem's flying number, the pulse and the screen's shake (Oddities in docs/NOTES.md)
void LightningBoard::DrawModeOverlay(Graphics* g)
{
	const float aAlpha = (float)mCoverCurve.GetOutVal();
	const int aAlphaByte = Ftol((double)aAlpha * 255.0);
	const int aGridX = GetGridX();
	const int aGridY = GetGridY();
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, aAlphaByte));

	// the mega gem rises with the seconds in the account: the 60 of them are the full height (0x715cb0). The exe's curve at +0x3290
	// (which eases to it) is the plain share here
	float aShare = (float)mBonusSeconds / 60.0f;
	if (aShare > 1.0f)
		aShare = 1.0f;
	g->PushState();
	g->ClipRect(0, Scaled(300), mWidth, mHeight);
	double aLift = aShare > 0.0f ? std::pow((double)aShare, 0.5) : 0.0;
	g->Translate(Scaled(0x38), Ftol((double)Scaled(0x118) * aLift) + Scaled(-0x16));
	POPANIM_ANIMS_GEM_MEGA->Draw(g);
	// 0x715cb0: after the gem (and the shard's effect, not translated) the red shard fades in with the warning, in the same translation
	if (IsWarning() && IMAGE_INGAMEUI_MEGASHARD_RED_LIGHTNING != nullptr) {
		g->SetColor(Color(255, 255, 255, GetWarningAlpha(aAlpha)));
		g->DrawImage(IMAGE_INGAMEUI_MEGASHARD_RED_LIGHTNING, Scaled(0x2d), Scaled(0x22));
	}
	g->PopState();
	if (mBonusSeconds > 0) {
		g->PushState();
		g->ClipRect(0, Scaled(300), mWidth, mHeight);
		g->SetFont(FONT_SCORE);
		g->SetColor(Color(255, 255, 255, aAlphaByte));
		g->WriteString(StrFormat(gApp->GetString("481", "+%d:%02d").c_str(), mBonusSeconds / 60, mBonusSeconds % 60), Scaled(0xba),
			Scaled(0x1ba), -1, 0, true, 0, -1, -1);
		g->PopState();
	}

	// the board of the top, the bar's rectangle and the timer
	g->SetColor(Color(255, 255, 255, aAlphaByte));
	Image* aTopBack = IMAGE_INGAMEUI_TOP_BACK_LIGHTNING;
	int aCellW = aTopBack != nullptr ? aTopBack->GetCelWidth() : 0;
	int aCellH = aTopBack != nullptr ? aTopBack->GetCelHeight() : 0;
	// slot 86 (0x7141b0): the rectangle of the bar
	Rect aBar(Scaled(aGridX + 0x200) - aCellW / 2, Scaled(aGridY - 0x37) - aCellH / 2, aCellW - Scaled(0x78), aCellH);
	auto aDrawAt = [&](Image* theImage, int theImageId) {
		if (theImage != nullptr)
			g->DrawImage(theImage, gApp->mArtRes * (GetImageOffsetX(theImageId) + aGridX - 0x2ab) / 1200,
				gApp->mArtRes * (GetImageOffsetY(theImageId) + aGridY - 0x82) / 1200);
	};
	aDrawAt(aTopBack, IMAGE_INGAMEUI_TOP_BACK_LIGHTNING_ID);

	// slot 241 (0x715940): the bar's colour over what is left of the time
	{
		double aFade = std::pow((double)aAlpha, 4.0);
		g->SetColor(Color(180, 120, 70, Ftol(aFade * 255.0)));
		g->FillRect(aBar.mX, aBar.mY, Ftol((double)mLevelBar * (double)aBar.mWidth), aBar.mHeight);
	}

	g->PushState();
	g->Translate(Scaled(HUD_SHIFT_X), 0);
	POPANIM_ANIMS_LIGHTNINGUI->Draw(g);
	g->PopState();

	if (mLastHurrah && !mTimeUpShown) {
		g->SetFont(FONT_SPEED_TEXT);
		Color aOldGlow, aOldOutline;
		bool aGlowSet = SetLayerColor(FONT_SPEED_TEXT, "GLOW", Color(0, 0, 0, 0), &aOldGlow);
		bool anOutlineSet = SetLayerColor(FONT_SPEED_TEXT, "OUTLINE", Color(0, 0, 0, 0), &aOldOutline);
		g->SetColor(Color(255, 255, 255, aAlphaByte));
		g->WriteString(gApp->GetString("482", "Last Hurrah"), Scaled(aGridX + 0x200), Scaled(0x62), -1, 0, true, 0, -1, -1);
		if (aGlowSet)
			RestoreLayerColor(FONT_SPEED_TEXT, "GLOW", aOldGlow);
		if (anOutlineSet)
			RestoreLayerColor(FONT_SPEED_TEXT, "OUTLINE", aOldOutline);
	}

	// slot 105 (0x714150): the timer's marker rides the bar's end
	int aMarkerX = aBar.mX + Ftol((double)mLevelBar * (double)aBar.mWidth) + Scaled(0x40) - Scaled(2);
	g->SetColor(Color(255, 255, 255, aAlphaByte));
	DrawCentered(g, IMAGE_INGAMEUI_TIMER_LIGHTNING, aMarkerX, Scaled(0x4c));

	// slot 218 (0x714130): without an account in the last ten seconds the screen's edge and the timer glow red
	if (IsWarning()) {
		const int aRed = GetWarningAlpha(aAlpha);
		g->PushState();
		g->SetColor(Color(255, 255, 255, aRed));
		DrawCentered(g, IMAGE_INGAMEUI_TIMER_RED_LIGHTNING, aMarkerX, Scaled(0x4c));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		aDrawAt(IMAGE_INGAMEUI_TOP_RED_LIGHTNING, IMAGE_INGAMEUI_TOP_RED_LIGHTNING_ID);
		aDrawAt(IMAGE_INGAMEUI_BOTTOM_RED_LIGHTNING, IMAGE_INGAMEUI_BOTTOM_RED_LIGHTNING_ID);
		g->PopState();
	}

	DrawScoreText(g);

	// 0x716ef0 after 0x715cb0: the top animation
	g->PushState();
	g->Translate(Scaled(HUD_SHIFT_X), 0);
	POPANIM_ANIMS_LIGHTNINGUITOP->Draw(g);
	g->PopState();
}

}  // namespace Sexy
