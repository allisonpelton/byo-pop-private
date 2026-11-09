// Board: the end of a level. The bonus for a run of combos is paid, the pieces on the field are handed to the
// deleting, and the next phase starts (the bonus round, or the level-up screen).

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "SmallObjects.h"
#include "SoundInstance.h"
#include "UserProfile.h"
#include "Common.h"
#include "Particle.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

// 0x452d30: a run of more than one combo pays a bonus of its points times its length; it floats up as a text and a
// sample (one for each length, up to 8) plays. (The original takes a parameter that it never reads.)
void Board::PayComboBonus()
{
	if (m1980 > 1 && (!m910 || m917))
	{
		int aPoints = m196c * m1980;
		AddScore(aPoints, true);

		FloatText* aText = new FloatText(StrFormat(GTC(0x5652d0), aPoints), FONT_POINTS, (float) m6e8, (float) (m6ec + 0x19), false);
		m140.push_back(aText);

		if (m1a14 < m1a18 && !m916)
		{
			int aSound = m1980 - 2 + SOUND_VCOMBO1;
			if (aSound >= SOUND_VCOMBO1 + 8)
				aSound = SOUND_VCOMBO1 + 7;
			mApp->PlaySample(aSound);
		}
	}
	m196c = 0;
	m1980 = 0;
	m1984 = 0;
}

// 0x465290: the level is complete (the Board's Update calls it when the goal is reached). The best level of the mode is
// kept in the profile every fourth level. Everything the player had in hand or on the field goes to the bricks to
// delete. In the survival mode's bonus rounds (m1b2c is 1) the bonus round starts at once; otherwise the level-up text
// comes and the level-up phase is set up.
void Board::LevelComplete()
{
	if (m900)
		return;

	SoundInstance* aKlaxon = mApp->mLoopSounds[7];
	if (aKlaxon->IsPlaying())
		aKlaxon->Stop();

	if (m1940 % 4 == 0 && m1940 > 1)
	{
		int aLevelStart = 0;
		mApp->mUser->GetInt(mMode + 8, &aLevelStart);
		if ((aLevelStart / 4) * 4 <= m1940)
		{
			int aBest = 0;
			mApp->mUser->GetInt(mMode + 0x6b, &aBest);
			if (aBest < mScore)
				mApp->mUser->SetInt(mMode + 0x6b, mScore);
		}
	}

	m1b64 = 0;
	for (std::list<Brick*>::iterator anItr = m70.begin(); anItr != m70.end(); ++anItr)
		m128.insert(*anItr);
	m70.clear();
	m923 = true;
	std::vector<MarkRec>().swap(m200);
	for (unsigned int i = 0; i < m108.size(); i++)
	{
		m108[i]->mGone = true;
		m128.insert(m108[i]);
	}
	std::vector<Brick*>().swap(m108);
	std::vector<DrawnText>().swap(m270);
	if (m354 != NULL)
	{
		m128.insert(m354);
		m354 = NULL;
	}
	m3c0 = Color(100, 100, 100);
	m1990 = 0;
	m92c = 0.0f;
	m8e5 = true;
	if (m1ad4 != 0)
		m1ad4 = 6;
	m1b1c++;
	gFlag5A5100 = false;
	if (m1980 > 1)
		PayComboBonus();

	if (m1a60 == 1 && m1b2c == 1)
	{
		mApp->PlaySample(SOUND_BONUS_ROUND);
		m90f = true;
		m910 = true;
		SetNextLevelScore();
		Fire(0x28, 1);
		mFlag8E9 = false;
		m19fc = 0;
		m1aa8 = 0;
		m1aa4 = 0;
		m1ab8 = 10;
		m1ac0 = PickSpawnDelay();
		m1abc = 400;
		mSirenAnim.SetStopped(true);
		mApp->PlaySong(0x29);
		m1b88 = 0;
		m1b8c = 0;
		m1b94 = 0;
		m1b98 = 0;
		m1b9c = 0;
		m1ba0 = 0;
		m1ba4 = 0;
		m1ba8 = 0;
		m1bb0 = 0;
		m1bac = 0;
		m916 = false;
		m912 = false;
		m914 = false;
		m915 = false;
		m1b74 = 0;
		m1b7c = 0;
		m1b78 = 200;
		m1b70 = -1;
		m918 = false;
		m917 = true;
		m1bc0 = 1;
		m1bc4 = 0;
		// (the original also reads the volume of the hyperspace loop and drops the number)
		mApp->mHyperspaceLoop->Play(true, false);
		mApp->mEngineLoop->Stop();
		SetOverlayHidden(false);
		mOverlay.SetFill(0xff, 0xff, 0xff, 0);
		for (unsigned int aRow = 0; aRow < m314.size(); aRow++)
		{
			for (int aCol = 0; aCol < 10; aCol++)
			{
				if (m314[aRow][aCol] != NULL)
					m314[aRow][aCol]->StartShake();
			}
		}
		aKlaxon = mApp->mLoopSounds[7];
		if (aKlaxon->IsPlaying())
			aKlaxon->Stop();
		SaveGameState();
		return;
	}

	m8de = true;
	delete m778;
	m778 = new LevelUpText(IMAGE_LEVELUP_TEXT, 0);
	mFlag8E9 = true;
	m1944 = 0;
	mApp->PlaySample(SOUND_LEVEL_COMPLETE);
	mApp->PlaySample(SOUND_WORDSWOOSH);
	mApp->PlaySong(0x29);
	StartLevelUp();
}

// A row of the table of upgrades (one table for each mode, rows by the group of four levels): the profile setting that
// counts the upgrade, its step and the text that announces it (the address of the original's literal).
struct UpgradeRec
{
	int				mSetting;
	int				mValue;
	unsigned int	mTextVA;
};

static const UpgradeRec kUpgrades[4][9] =
{
	{	// mode 0 (the original's table at 0x5a53e4)
		{61, 4, 0x56a098}, {51, 1, 0x569ce0}, {59, 1, 0x569c98}, {55, 1, 0x569c40}, {51, 2, 0x569bf0},
		{55, 2, 0x569c40}, {59, 2, 0x569c98}, {55, 3, 0x569c40}, {51, 3, 0x569ba0}
	},
	{	// mode 1 (0x5a55b4; row 0 is not used)
		{0, 0, 0}, {52, 1, 0x569f80}, {52, 2, 0x569f18}, {60, 1, 0x569ed0}, {52, 3, 0x569e68}, {52, 4, 0x569e00},
		{60, 2, 0x569ed0}, {52, 5, 0x569d98}, {52, 6, 0x569d30}
	},
	{	// mode 2 (0x5a52c4)
		{58, 3, 0x56a218}, {53, 1, 0x56a148}, {53, 2, 0x56a0e0}, {61, 1, 0x56a098}, {53, 3, 0x56a050},
		{61, 2, 0x56a098}, {53, 4, 0x569fe8}, {61, 3, 0x56a098}, {61, 4, 0x56a098}
	},
	{	// mode 3 (0x5a51a4; row 0 is not used)
		{0, 0, 0}, {54, 1, 0x56a2b8}, {62, 1, 0x56a270}, {58, 1, 0x56a218}, {62, 2, 0x56a270}, {58, 2, 0x56a218},
		{54, 2, 0x56a1c8}, {62, 3, 0x56a270}, {58, 3, 0x56a218}
	}
};

// 0x433130: what the upgrade screen of the next level offers (the argument is the level that is about to begin). 3: the
// level opens a mode (level 8 of mode 0 unlocks mode 3, level 16 of mode 1 unlocks mode 2) that is still locked;
// 4 or 2: every fourth level, while the profile's count of the upgrade is at its step (4 with the mode to unlock); 1: the
// levels before a multiple of four, unless the screen shown now was kind 1; else 0.
int Board::Fn433130(int theLevel)
{
	int aUnlock = 0;
	if ((mMode == 0 && theLevel == 8 && !mApp->mUser->HasModeUnlocked(3)) ||
		(mMode == 1 && theLevel == 0x10 && !mApp->mUser->HasModeUnlocked(2)))
		aUnlock = 3;

	if (theLevel % 4 == 0 && theLevel >= 0 && theLevel <= 0x20)
	{
		const UpgradeRec& aRow = kUpgrades[(mMode >= 0 && mMode <= 2) ? mMode : 3][theLevel / 4];
		int aCount = 0;
		mApp->mUser->GetInt(aRow.mSetting, &aCount);
		if (aCount <= aRow.mValue)
			return aUnlock == 3 ? 4 : 2;
	}
	if (aUnlock == 3)
		return 3;
	if ((theLevel - 3) % 4 == 0 || theLevel == 3)
		return m1b2c == 1 ? 0 : 1;
	return 0;
}

// 0x4330a0: the profile setting that the upgrade of the level counts (the row of the level's group of four in the mode's
// table), -1 for a mode that has no table
int Board::GetUpgradeSetting()
{
	if (mMode < 0 || mMode > 3)
		return -1;
	return kUpgrades[mMode][m1940 / 4].mSetting;
}

// 0x441850: the words of that row that name the upgrade (empty with no table)
std::string Board::GetUpgradeText()
{
	if (mMode < 0 || mMode > 3 || kUpgrades[mMode][m1940 / 4].mTextVA == 0)
		return std::string();
	return GT(kUpgrades[mMode][m1940 / 4].mTextVA);
}

// 0x43e220: a number with a comma after each three digits (counted from the right)
std::string Board::FormatNumber(int theNumber)
{
	std::string aString = StrFormat("%d", theNumber);
	int aCount = 1;
	int anIndex = (int) aString.length();
	while (--anIndex >= 0)
	{
		if (aCount % 3 == 0 && anIndex > 0)
			aString.insert(anIndex, ",");
		aCount++;
	}
	return aString;
}

// 0x434200: the picture of the upgrade screen is set up: the head and the arm, each with a frame animation
void Board::SetupUpgradeAnims()
{
	m87c = 0x294;
	m880 = 0x1a0;
	m884 = 0x1cf;
	m888 = 0x1a0;
	m894 = IMAGE_UPGRADE_HEAD;
	m890 = IMAGE_UPGRADE_ARM;
	mAnim898.Reset();
	mAnim898.SetNumFrames(IMAGE_UPGRADE_HEAD->mNumCols);
	mAnim898.SetDelay(6);
	mAnim898.mHoldLast = false;
	mAnim898.SetRange(0x10, 0x15);

	m820 = -IMAGE_UPGRADE_ARM->mWidth;
	m824 = 0x1a0;
	m828 = 0x82;
	m82c = 0x1a0;
	m830 = true;
	mAnim83c.Reset();
	mAnim83c.SetNumFrames(IMAGE_UPGRADE_HEAD->mNumCols);
	mAnim83c.SetDelay(6);
	mAnim83c.mHoldLast = false;
	mAnim83c.SetDirection(-1);
	mAnim83c.SetRange(0, 5);
}

// 0x432b50: the weapon levels of the player (profile settings 0x33 + mode, 0x3b + mode, and 0x37 or 0x3a) set how fast
// the weapon charges (m1be4, a factor) and how long it lasts (m1be0); the laser of mode 0 gets its animation. (The
// original computes the factor in x87 arithmetic.)
void Board::ApplyWeaponLevels()
{
	mApp->mUser->GetInt(mMode + 0x33, &m1a8c);
	mApp->mUser->GetInt(mMode + 0x3b, &m1a94);
	if (mMode == 0)
		mApp->mUser->GetInt(0x37, &m1a90);
	else if (mMode == 3)
		mApp->mUser->GetInt(0x3a, &m1a90);
	else
		m1a90 = -1;

	m1bdc = 18000.0f;
	if (mMode == 1)
	{
		if (m1a94 == 1)
			m1be4 = 1.2f;
		else if (m1a94 == 2)
			m1be4 = 1.7142857f;
		else if (m1a94 == 3)
			m1be4 = 3.0f;
		m1be0 = (float) ((double) 18000.0f * (double) 0.002f);
		return;
	}
	if (mMode == 2)
	{
		switch (m1a94)
		{
		case 1:		m1be4 = 1.2f;		break;
		case 2:		m1be4 = 1.3333334f;	break;
		case 3:		m1be4 = 1.5f;		break;
		case 4:		m1be4 = 1.7142857f;	break;
		case 5:		m1be4 = 2.0f;		break;
		}
		m1be0 = (float) ((double) 18000.0f * (double) 0.002f);
		return;
	}
	if (mMode != 0 && mMode != 3)
		return;

	m1be4 = (float) (18000.0 / (18000.0 - ((double) m1a94 - 1.0) * 3000.0));
	if (mMode == 0)
	{
		m420.SetNumFrames(IMAGE_PLAYER1_LASER->mNumCols);
		m420.SetDelay(2);
		m420.mHoldLast = true;
		m420.SetStopped(true);
	}
	switch (m1a90)
	{
	case 1:		m1be0 = 22.5f;			break;
	case 2:		m1be0 = 20.0f;			break;
	case 3:		m1be0 = 18.0f;			break;
	case 4:		m1be0 = 16.363636f;		break;
	}
}

// 0x43b1b0: the five layers of the backdrop (the warp's layers: one picture, then a left and a right one for each of the
// next two) start at the top of their pictures
void Board::SetupWarpLayers()
{
	struct WarpLayer
	{
		Image*	mImage;
		float	mSpeed;
		float	mRangeStart;
	};
	WarpLayer aLayers[5] =
	{
		{IMAGE_WARP_LAYER_1, 0.01f, 229.0f},
		{IMAGE_WARP_LAYER_2_LEFT, 0.25f, 133.0f},
		{IMAGE_WARP_LAYER_2_RIGHT, 0.25f, 436.0f},
		{IMAGE_WARP_LAYER_3_LEFT, 1.0f, 0.0f},
		{IMAGE_WARP_LAYER_3_RIGHT, 1.0f, 517.0f}
	};
	for (int i = 0; i < 5; i++)
	{
		PlanetRec& aLayer = m160[i];
		Image* anImage = aLayers[i].mImage;
		aLayer.mImage = anImage;
		aLayer.mPeriod = 1;
		aLayer.mSpeed = aLayers[i].mSpeed;
		aLayer.mF24 = 0.0f;
		aLayer.mF28 = -1.0f;
		aLayer.mRangeStart = aLayers[i].mRangeStart;
		aLayer.mCur[0] = 0.0f;
		aLayer.mCur[1] = (float) (anImage->mHeight - 0x1e0);
		aLayer.mCur[2] = (float) anImage->mWidth;
		aLayer.mCur[3] = 480.0f;
		aLayer.mNext[0] = 0.0f;
		aLayer.mNext[1] = 0.0f;
		aLayer.mNext[2] = (float) anImage->mWidth;
		aLayer.mNext[3] = 0.0f;
	}
}

// 0x45dd70: the level-up screen. The counters of the screen and of the warp start, the best level of the mode is kept,
// the upgrade of the level (every fourth, for the modes that have them) is counted in the profile, the weapon levels
// are read, and the panel of text (points, biggest combo, time, bonus, next rank) is made.
void Board::StartLevelUp()
{
	m1aa8 = 0;
	m1aa4 = 0;
	m8ec = true;
	m8ed = false;
	m19a4 = 0;
	m1954 = 0;
	m8ef = false;
	m3d0 = Color(0xff, 0xff, 0xff, 0);
	mOverlay.SetFill(0xff, 0xff, 0xff, 0);
	SetOverlayHidden(false);
	m1998 = 6;
	m19ac = 0xc;
	m19a8 = 0;
	m199c = 0;
	m19f0 = 0;
	unsigned int aRand = AstroRand();
	m19a0 = 0;
	m938 = -52.0f;
	m19f4 = (((int) aRand % 500000000) % 200) + 100;
	m3c0 = Color(100, 100, 100);
	m1b3c = 0;

	int aBestLevel = 0;
	mApp->mUser->GetInt(mMode + 8, &aBestLevel);
	if (aBestLevel < m1940)
		mApp->mUser->SetInt(mMode + 8, m1940);
	SetupUpgradeAnims();

	if (m1b2c == 2 || m1b2c == 4)
	{
		int aCount = 0;
		int aSetting = aBestLevel;
		if (mMode >= 0 && mMode <= 3)
			aSetting = kUpgrades[mMode][m1940 / 4].mSetting;
		mApp->mUser->GetInt(aSetting, &aCount);
		aCount++;
		mApp->mUser->SetInt(aSetting, aCount);
		ApplyWeaponLevels();
	}

	if (m1b2c == 3 || m1b2c == 4)
	{
		int aValue = 0x13;
		bool aKnown = true;
		if (mMode == 0)
		{
			mApp->mUser->GetInt(0x30, &aValue);
			aValue = (((aValue & 4) != 0) ? 3 : 0) + 0x41;
			mApp->mUser->SetInt(0x30, aValue);
		}
		else if (mMode == 1)
		{
			mApp->mUser->GetInt(0x30, &aValue);
			aValue = (((aValue & 0x40) != 0) ? 0x2e : 0) + 0x16;
			mApp->mUser->SetInt(0x30, aValue);
		}
		else
			aKnown = false;
		if (aKnown)
		{
			aValue += 0x193;
			mApp->mUser->SetInt(0x32, aValue);
		}
	}

	m19fc = 0;
	mSirenAnim.SetStopped(true);
	m1a98 = -500000000;
	m1a9c = -500000000;
	m1ab8 = 10;
	m1ac0 = PickSpawnDelay();
	m1abc = 700;
	mAnim460.SetStopped(false);

	// the lines of the panel
	m7e4 = 0;
	m7e0 = 0;
	std::vector<std::string>().swap(m7cc);
	m7cc.push_back(GT(0x565728) + FormatNumber(m7bc));
	m7cc.push_back(StrFormat(GTC(0x565714), m7c4));
	int aSeconds = (m19b4 / 100) % 0x3c;
	int aMinutes = m19b4 / 6000;
	if (aSeconds < 10)
		m7cc.push_back(StrFormat(GTC(0x5656f8), aMinutes, aSeconds));
	else
		m7cc.push_back(StrFormat(GTC(0x5656dc), aMinutes, aSeconds));

	mApp->GetRankName(mScore, &m1bd4);
	delete m30c;
	m30c = mApp->MakeRankImage(m1bd4, 0x50);

	if (m1b2c == 1)
		m7cc.push_back(StrFormat(GTC(0x5656c8), FormatNumber(m1ba4 - m1b9c + m960).c_str()));

	int aPointsToNext = mApp->PointsToNextRank(mScore);
	if (aPointsToNext == -1)
		m7cc.push_back(GT(0x56569c));
	else
		m7cc.push_back(StrFormat(GTC(0x5656b4), FormatNumber(aPointsToNext + mScore).c_str()));

	// the streaks of the backdrop take the colour of their kind
	for (unsigned int i = 0; i < m170.size() && i < 100; i++)
	{
		StreakRec& aStreak = m170[i];
		aStreak.mFlag28 = false;
		if (aStreak.mKind == 0)
			aStreak.mColor = Color(0x50, 0x50, 0x50);
		else if (aStreak.mKind == 1)
			aStreak.mColor = Color(0x96, 0x96, 0x96);
		else if (aStreak.mKind == 2)
			aStreak.mColor = Color(0xfa, 0xfa, 0xfa);
	}

	m7dc = 5;
	m800 = 0x19;
	m804 = true;
	m7f4 = 0;
	m7fc = 2;
	m7ec = 0xce;
	m7f0 = -206;
	m7f8 = 10;
	m7e8 = -206;
	SetupWarpLayers();
}
