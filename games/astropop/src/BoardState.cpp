// Board: the state that is reset for each game, the per-update tick of the effects, and the score.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// a test's weapon charge for a new board and for ResetState (a test command, `set chargestart`; below 0: none, the charge starts at 0)
float Sexy::gTestChargeAtReset = -1.0f;

// 0x4346c0: the numbers a new game (or a new level) starts from
void Board::ResetState()
{
	m8e0 = false;
	m1b48 = -IMAGE_SUPA_BUTTON_ALERT->mWidth;
	m1bd8 = gTestChargeAtReset < 0.0f ? 0.0f : gTestChargeAtReset;
	m92c = 0;
	m928 = 0;
	m1abc = 0;
	m1ab8 = 4;
	m1ac0 = PickSpawnDelay();
	m1af4 = 0;
	m1af0 = 0;
	m90b = false;
	m19b8 = 0;
	m1b00 = 0;
	m1afc = 0;
	m974 = 0;
	mScore = 0;
	m1970 = 0;
	m354 = NULL;
	m198c = 0xa6;
	m920 = false;
	m1954 = 2;
	m8ea = false;
	m1a34 = 0;
	m8fc = false;
	m1a7c = -1;
	mSirenAnim.SetNumFrames(IMAGE_SIREN->mNumCols);
	mSirenAnim.SetDelay(8);
	mSirenAnim.mHoldLast = false;
	mSirenAnim.SetStopped(true);
}

// 0x439c20: one update's worth of ticking for the fades: the alpha at +0x1b38 climbs, the overlay's colour fades out
// (and the overlay goes when the colour is gone and nothing stops it), the streaks slow down, and so on
void Board::UpdateEffects()
{
	m1b38 += 2;
	if (m1b38 > 0xff)
		m1b38 = 0xff;

	m3d0.mAlpha -= 2;
	if (m3d0.mAlpha < 0)
	{
		if (m230.empty() || m230[0]->m1c == 0 || -m7ec < m7e8)
			SetOverlayHidden(true);
		m3d0.mAlpha = 0;
	}
	mOverlay.SetFill(m3d0.mRed, m3d0.mGreen, m3d0.mBlue, m3d0.mAlpha);

	int anOld = m77c.mPeriod;
	m77c.mPeriod = anOld + 1;
	if (1 < anOld)
		m77c.mPeriod = 1;

	int aNumDone = 0;
	for (int i = 0; i < 100; i++)
	{
		StreakRec* aStreak = &m170[i];
		aStreak->mMargin -= 3;
		if (aStreak->mMargin < 0)
		{
			aStreak->mMargin = 0;
			aNumDone++;
		}
		aStreak->mSpeed = aStreak->mSpeed - 0.04f;
		if (aStreak->mSpeed < aStreak->mMinSpeed)
			aStreak->mSpeed = aStreak->mMinSpeed;
	}
	if (aNumDone == 100)
		m8ea = false;

	int aPhase = m1954;
	m19b0++;
	if (aPhase != 2)
	{
		if (mUpdateCnt % 8 == 0 && -1 < m19b0)
		{
			if (aPhase < 3)
				m1954 = 2;
			else
				m1954 = aPhase - 1;
		}
	}

	if (0.0f < m92c)
	{
		float aValue = m92c - 0.0076f;
		m92c = aValue;
		if (aValue < 0.0f)
			m92c = 0;
		m928 = m92c;
	}
}

// 0x4344d0: the score the next level needs, by the level
void Board::SetNextLevelScore()
{
	int aLevel = m1940;
	if (aLevel < 4)
		m960 = 10000;
	else if (aLevel < 8)
		m960 = 25000;
	else if (aLevel < 0xc)
		m960 = 50000;
	else if (aLevel < 0x10)
		m960 = 75000;
	else if (aLevel < 0x14)
		m960 = 100000;
	else if (aLevel < 0x47)
		m960 = (((aLevel - 0x13) / 4) + 2) * 50000;
	else
		m960 = 500000;
	if (500000 < m960)
		m960 = 500000;
}

// 0x433b90: points are added to the score; the best score of the mode is kept in the profile, and the points are
// counted for the stats (m7bc while m910 is off, else m7b8 unless m916 holds them back)
void Board::AddScore(int thePoints, bool theForce)
{
	mScore += thePoints;
	int aBest = 0;
	mApp->mUser->GetInt(mMode, &aBest);
	if (aBest < mScore)
		mApp->mUser->SetInt(mMode, mScore);
	if (!m910)
	{
		m7bc += thePoints;
		return;
	}
	if (!m916 || theForce)
		m7b8 += thePoints;
}

// the best of a stat is kept in the profile (setting 0x5d)
static void SaveBestStat(UserProfile* theUser, int theValue)
{
	int aBest = 0;
	theUser->GetInt(0x5d, &aBest);
	if (aBest < theValue)
		theUser->SetInt(0x5d, theValue);
}

// 0x43a9a0: when nothing moves, the field and the counters are written to the profile (so that a game can be
// continued): the type of every cell (and a number for steel and jammed bricks) in the profile's table of scores, the
// rest in its settings 0x56 .. 0x65. Which settings are written depends on the mode the game is in.
void Board::SaveGameState()
{
	if (m1a34 != 0)
		return;
	if (m1990 != 0)
		return;
	if (!m70.empty())
		return;
	if (m8e4)
		return;
	if (!m94.empty())
		return;
	if (!m7c.empty())
		return;
	if (CountMovingBricks(false) != 0)
		return;
	if (!m1a0.empty())
		return;
	if (!ma0.empty())
		return;
	if (!mc8.empty())
		return;
	if (!mf8.empty())
		return;

	UserProfile* aUser = mApp->mUser;
	gFlag5A5100 = true;
	bool aIdle = !m8eb && !m8ec && !m8ed && !m8ef && !m8f4;
	if (!m916 && aIdle && !m917 && !m912)
	{
		if (!m314.empty())
		{
			for (unsigned int aRow = 0; aRow < m314.size(); aRow++)
			{
				for (int aCol = 0; aCol < 10; aCol++)
				{
					Brick* aBrick = m314[aRow][aCol];
					int aType;
					if (aBrick == NULL)
						aType = -1;
					else
						aType = aBrick->GetEffectiveType();
					int aValue = -1;
					if (aBrick == NULL || !aBrick->IsJammed())
					{
						if (aType == 9)
							aValue = static_cast<SpecialBrick*>(aBrick)->mStage;
					}
					else
						aValue = 500000000;
					aUser->mScores[aRow][aCol].mValue = aType;
					aUser->mScores[aRow][aCol].mAux = aValue;
				}
			}
		}
	}

	// which of the four sets of settings is written: the classic game (A: nothing running, no 912/917), the timed
	// mode (Y, m910 on), the others (W), or the game's end (Z, m917 or m912). The original tests the same flags in a
	// tangle of jumps (and a return that no combination of them reaches); this is the same decision.
	if (!m917 && !m912)
	{
		if (!m910 && !m916 && aIdle)
		{
			StoreProgress();
			aUser->SetInt(0x56, m1a14);
			aUser->SetInt(0x57, 0);
			aUser->SetInt(0x61, m1978);
			aUser->SetInt(0x58, 0);
			aUser->SetInt(0x62, 0);
			aUser->SetInt(0x5b, 0);
			aUser->SetInt(0x60, m19b4);
			aUser->SetInt(0x5c, m7bc);
			SaveBestStat(aUser, m7c4);
			if (m200.empty())
				aUser->SetInt(0x59, -1);
			else
			{
				aUser->SetInt(0x59, m200[0].mType);
				aUser->SetInt(0x5a, m200[0].mCount);
			}
			aUser->SetInt(0x65, m19fc);
			return;
		}
		if (m910 && !m916 && aIdle)
		{
			StoreProgress();
			aUser->SetInt(0x57, m1b70);
			aUser->SetInt(0x58, 0);
			aUser->SetInt(0x5b, 0);
			aUser->SetInt(0x62, m7b8);
			aUser->SetInt(0x61, m1978);
			aUser->SetInt(0x5c, m7bc);
			aUser->SetInt(0x60, m19b4);
			SaveBestStat(aUser, m7c4);
			if (m200.empty())
			{
				aUser->SetInt(0x59, -1);
				aUser->SetInt(0x5a, 0);
			}
			else
			{
				aUser->SetInt(0x59, m200[0].mType);
				aUser->SetInt(0x5a, m200[0].mCount);
			}
			aUser->SetInt(0x65, m19fc);
			return;
		}
		StoreProgress();
		aUser->SetInt(0x58, 1);
		aUser->SetInt(0x5b, 0);
		aUser->SetInt(0x57, 0);
		aUser->SetInt(0x62, m7b8);
		aUser->SetInt(0x61, m1978);
		aUser->SetInt(0x5c, m7bc);
		aUser->SetInt(0x60, m19b4);
		SaveBestStat(aUser, m7c4);
		if (m1b2c != 1)
			return;
		aUser->SetInt(0x5f, (m1ba4 - m1b9c) + m960);
		return;
	}

	StoreProgress();
	aUser->SetInt(0x5b, 1);
	aUser->SetInt(0x57, 0);
	aUser->SetInt(0x62, 0);
	aUser->SetInt(0x61, m1978);
	aUser->SetInt(0x5c, m7bc);
	aUser->SetInt(0x60, m19b4);
	SaveBestStat(aUser, m7c4);
}

bool Sexy::gFlag5A5100 = false;   // 0x5a5100: set by SaveGameState, cleared by the callback at +0x60 and others (meaning not found yet)
