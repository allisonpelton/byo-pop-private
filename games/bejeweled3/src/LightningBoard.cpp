// Sexy::SpeedBoard (the port's LightningBoard): see the header. Every function says its slot and address in the exe.
#include "LightningBoard.h"

#include "Bej3App.h"
#include "EndLevelDialog.h"

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
// when needed. port: the time gem's help box (the exe's slot 131 with 9, which says the help of the gem has not been seen: every help is
// seen here), its glow (0x741130) and sound are left out
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

}  // namespace Sexy
