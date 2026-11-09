// BoardKeys.cpp: the keyboard side of the Board (job B3): the key handler, the test of one creature against a typed letter,
// and the banner message that the typing answers with.
//
// The draws of random numbers all come from the game's MTRand (the original's 0x463a10 on the object at 0x565384).
// KeyChar reads the creatures of the lists through their Enemy part, as the other Board files do (Board.h).

#include "Board.h"
#include "Enemy.h"
#include "Clam.h"
#include "Projectile.h"
#include "ProjectileBoss.h"
#include "WSTimer.h"
#include "LessonManager.h"
#include "UserProfile.h"
#include "GameText.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// 0x446aa0: shows a message on the screen: the text, a restart of the banner timer, and full opacity (Update fades it out
// once the timer has passed two seconds)
void Board::SetBannerText(const std::string& theText)
{
	mBannerText = theText;
	mBannerTimer->Reset();
	mBannerLife = 255;
}

// 0x443ae0: the creature takes the typed letter when it is swimming (state 0) or flashing (state 6) and the letter is the
// next one of its word after what the board has typed so far (mTyped, the letters of the creature the board has locked
// on). The board then locks on this creature, plays one of the three typing sounds, lets the creature count the letter
// (TypeLetter), and, unless the wave is the bonus wave (type 2), knocks the creature back by the wave's knock-back distance.
// Returns whether the letter was taken.
bool Board::TryLetter(char theChar, Enemy* theEnemy)
{
	if (theEnemy->mState != 0 && theEnemy->mState != 6)
		return false;

	std::string aWord = theEnemy->mWord;		// the original works on a copy of the word
	char aNext = '\0';
	if (mTyped.size() < aWord.size())			// port: the original indexes past the end of a finished word
		aNext = aWord[mTyped.size()];
	if (aNext != theChar)
		return false;

	mApp->PlaySample((int)(gGameRand->NextNoAssert() % 3) + SOUND_TYPE);
	mTarget = theEnemy;
	theEnemy->TypeLetter();
	mTyped.append(1, theChar);
	if (mLessonManager->GetCurrentWaveType() != 2)
	{
		double aX = theEnemy->mX;
		theEnemy->SetX((double)mLessonManager->GetWaveKnockBack() + aX);
	}
	return true;
}

// 0x454e40: a key typed. Lower case is made upper case first. The branches, in order:
//   the diver in state 7 (or in state 6 of a bonus level) and not paused: only the space bar does anything (back to the map);
//   the space bar pauses (ButtonDepress 200) when the profile says so;
//   a paused board, or a diver in a state that cannot type (only 0, 4, 5 and 9 can; 4 only while mFlag0f80 is clear) ignores it;
//   Return fires the EMP (states 0, 9, 4): a banner says why not when it cannot (no charge, already on, or a boss);
//   a bonus level (state 5): the letter opens the clam it is on (the memory game of pairs, three matches show the treasure);
//   the streak minigame (state 5 outside a bonus level) compares the letter with mString0f10;
//   otherwise the letter goes to the creature locked on, or to the first creature that takes it: sharks, then piranhas, then
//   bonus creatures, then the boss's shots; none of them: a typo.
void Board::KeyChar(SexyChar theChar)
{
	mApp->SetCursor(CURSOR_NONE);		// the 2004 cursor id 5 (docs/FRAMEWORK_2004.md)

	char aChar = (char)theChar;
	if (aChar >= 'a' && aChar <= 'z')
		aChar -= 0x20;

	int aState = mPlayer->mState;
	if (aState == 7 || (aState == 6 && mLessonManager->IsBonusLevel()))
	{
		if (!mPaused)
		{
			if (aChar == ' ')
				mApp->ReturnToMap();
			return;
		}
	}

	if (aChar == ' ')
	{
		if (mApp->mUser->mSpacePauses)
		{
			mApp->ButtonDepress(200);
			return;
		}
		aState = mPlayer->mState;
		if (aState != 7 && aState != 0xc)
			return;
	}

	if (mPaused)
		return;

	aState = mPlayer->mState;
	if (aState != 0 && aState != 5 && aState != 4 && aState != 9)
		return;
	if (aState == 4 && mFlag0f80)
		return;

	// the EMP
	if (aState == 0 || aState == 9 || (aState == 4 && !mFlag0f80))
	{
		if (aChar == '\n' || aChar == '\r')
		{
			if (mPlayer->mEmpCharge >= 1.0 && !mPlayer->mEmpActive)
			{
				if (aState == 9)
					SetBannerText(GT(0x00530ba0));
				else
				{
					mPlayer->mEmpActive = true;
					mFlashAlpha = 255;
					mFlag0eb4 = true;
					mApp->PlaySample(SOUND_EMP);
					mFlag0eb5 = true;
					SetBannerText(GT(0x00530bec));
					mApp->ChangeMusic(0);
				}
			}
			else
			{
				if (aState == 9)
					SetBannerText(GT(0x00530ba0));
				else
					SetBannerText(GT(0x00530bc4));
			}
			return;
		}
	}

	if (mLessonManager->IsBonusLevel())
	{
		// the bonus level: the clams
		if (mPlayer->mState != 5)
		{
			mApp->PlaySample(SOUND_WRONG);
			return;
		}

		int aFound = -1;
		size_t aClamIndex = 0;
		while (aClamIndex < mClams.size())
		{
			std::string aKey = StrFormat("%c", (int)aChar);
			Enemy* aClamEnemy = mClams[aClamIndex];
			if (aKey == aClamEnemy->mWord && aClamEnemy->mState == 4)
				aFound = (int)aClamIndex;
			if (aClamEnemy->mState == 1 && dynamic_cast<Clam*>(aClamEnemy)->mType == 0)
			{
				mApp->PlaySample(SOUND_WRONG);
				return;
			}
			aClamIndex++;
		}
		if (aFound < 0)
		{
			mApp->PlaySample(SOUND_WRONG);
			return;
		}

		Clam* aClam = dynamic_cast<Clam*>(mClams[aFound]);
		aClam->SetState(1);
		mApp->PlaySample(SOUND_TYPE);
		if (aClam->mType != 0 && mOpenClam == NULL)
		{
			mApp->PlaySample(SOUND_TREASURE);
			mOpenClam = aClam;
			return;
		}
		if (mOpenClam == NULL)
			return;
		if (dynamic_cast<Clam*>(mOpenClam)->mType != aClam->mType)
			return;

		mApp->PlaySample(SOUND_WINGEM);
		mPairsFound++;
		mOpenClam = NULL;
		std::string aText;
		if (mPairsFound == 1)
		{
			mApp->PlaySample(SOUND_WAVE_BONUS3);
			mPlayer->AddScore(100000);
			aText = StrFormat(GTC(0x00530c5c), 100000);			// "+%d PTS"
		}
		else
		{
			mApp->PlaySample(SOUND_WINGEM);
			mPlayer->AddScore(200000);
			aText = StrFormat(GTC(0x00530c5c), 200000);
		}
		AddBonusText(aText, (int)aClam->mX, (int)aClam->mY);
		return;
	}

	if (mPlayer->mState == 5)
	{
		// the streak minigame: type mString0f10 in one go
		unsigned int aPos = (unsigned int)mField0f2c;
		char aWanted = '\0';
		if (aPos < mString0f10.size())			// port: the original indexes past the end of the cleared string
			aWanted = mString0f10[aPos];
		if (aChar == aWanted)
		{
			if (aPos == 0)
				mTypeTimer->Reset();
			mApp->PlaySample((int)(gGameRand->NextNoAssert() % 3) + SOUND_TYPE);
			mField0f70++;
			mField0f68++;
			mField0f2c++;
			mFlag0d38 = true;
			if ((unsigned int)mField0f2c < mString0f10.size())
				return;

			// the whole string was typed
			mTypeTime += mTypeTimer->Elapsed();
			mPlayer->AddScore(GetStreakBonus());
			mField0f0c += GetStreakBonus();
			mString0f10 = "";
			int aThird = mWidth / 3;
			std::string aText = StrFormat(GTC(0x00530c5c), GetStreakBonus());		// "+%d PTS"
			AddBonusText(aText, (int)(gGameRand->NextNoAssert() % (unsigned int)(mWidth - aThird)) + aThird / 2, 240, NULL);

			mField0f30++;
			int aStreaks = mField0f30;
			if (aStreaks <= 2)
				mApp->PlaySample(SOUND_TONE1);
			else if (aStreaks <= 4)
				mApp->PlaySample(SOUND_TONE2);
			else if (aStreaks <= 6)
				mApp->PlaySample(SOUND_TONE3);
			else if (aStreaks <= 8)
				mApp->PlaySample(SOUND_TONE4);
			else if (aStreaks <= 10)
				mApp->PlaySample(SOUND_TONE5);
			else if (aStreaks <= 12)
				mApp->PlaySample(SOUND_TONE6);
			else if (aStreaks <= 14)
				mApp->PlaySample(SOUND_TONE7);
			else if (aStreaks <= 16)
				mApp->PlaySample(SOUND_TONE8);
			else if (aStreaks <= 18)
				mApp->PlaySample(SOUND_TONE9);
			else if (aStreaks <= 20)
				mApp->PlaySample(SOUND_TONE10);
			else if (aStreaks <= 22)
				mApp->PlaySample(SOUND_TONE11);
			else if (aStreaks <= 24)
				mApp->PlaySample(SOUND_TONE12);
			else
				mApp->PlaySample(SOUND_TONE13);

			int aBubbles = 50;
			do
			{
				AddBubble((int)(gGameRand->NextNoAssert() % 0xf0) + 200, 0x118);
				aBubbles--;
			}
			while (aBubbles != 0);
			mApp->PlaySample(SOUND_DIVERENTERS);
			return;
		}

		// a typo in the streak
		mApp->PlaySample(SOUND_WRONG);
		mField0f74++;
		mField0f6c++;
		std::string aMessage = StrFormat(GTC(0x00530c40), (int)aChar);
		if (mField0f38 > 5.0)
			mField0f38 = mField0f38 - 5.0;
		SetBannerText(aMessage);
		return;
	}

	// the normal play
	if (mFlag0f80 && mBoss == NULL)
		return;
	if (mBoss != NULL && dynamic_cast<ProjectileBoss*>(mBoss)->mHitPoints == 0)
		return;

	if (mTarget != NULL)
	{
		// a creature is locked on: only its next letter counts
		if (!TryLetter(aChar, mTarget))
		{
			mApp->PlaySample(SOUND_WRONG);
			if (mLessonManager->GetCurrentWaveType() != 2 && mBoss == NULL)
				mTarget->SetState(6);
			std::string aMessage = StrFormat(GTC(0x00530c18), (int)aChar);
			SetBannerText(aMessage);
			if (mLessonManager->GetCurrentWaveType() != 1 && (mBoss == NULL || mBoss->mField0e4 == 0xf || mBoss->mField0e4 == 0xc))
				mField0f6c++;
			mField0f64++;
			if (mLessonManager->GetCurrentWaveType() == 1)
			{
				mField0e94++;
				if (mField0e94 >= mLessonManager->GetLevelSettings()->mNumForPenalty)
				{
					mField0e94 = 0;
					AddPiranha();
				}
			}
			return;
		}
		if (mLessonManager->GetCurrentWaveType() != 1 && (mBoss == NULL || mBoss->mField0e4 == 0xf || mBoss->mField0e4 == 0xc))
			mField0f68++;
		mField0f60++;
		mFlag0d38 = true;
		return;
	}

	// nothing is locked on: the first creature that takes the letter
	size_t anIndex = 0;
	while (anIndex < mSharks.size())
	{
		Enemy* aShark = mSharks[anIndex];
		if ((double)mWidth > aShark->mX + 20.0 && aShark->mVisible && TryLetter(aChar, aShark))
		{
			mField0f60++;
			mField0f68++;
			mFlag0d38 = true;
			mTypeTimer->Reset();
			if (!mApp->mUser->mFlag1dc)
				mFlag0ea8 = true;
			break;
		}
		anIndex++;
	}

	anIndex = 0;
	while (anIndex < mPiranhas.size())
	{
		Enemy* aPiranha = mPiranhas[anIndex];
		if ((double)mWidth > aPiranha->mX + 20.0 && aPiranha->mVisible && TryLetter(aChar, aPiranha))
		{
			// a piranha takes one letter: it is not locked on
			mField0f60++;
			mFlag0d38 = true;
			mTarget = NULL;
			mTyped = "";
			return;
		}
		anIndex++;
	}

	anIndex = 0;
	while (anIndex < mBonusCreatures.size())
	{
		Enemy* aBonus = mBonusCreatures[anIndex];
		if ((double)mHeight > aBonus->mY + 20.0 && aBonus->mVisible && !aBonus->mFlag19 && TryLetter(aChar, aBonus))
		{
			mField0f60++;
			mField0f68++;
			mFlag0d38 = true;
			mTypeTimer->Reset();
			if (!mApp->mUser->mFlag1dc)
				mFlag0ea8 = true;
			break;
		}
		anIndex++;
	}

	if (mBoss != NULL)
	{
		ProjectileBoss* aBoss = dynamic_cast<ProjectileBoss*>(mBoss);
		int aShotIndex = 0;
		while (aShotIndex < (int)aBoss->mShots.size())
		{
			Projectile* aShot = aBoss->mShots[aShotIndex];
			if (aShot->mState == 0 && aShot->mVisible && TryLetter(aChar, aShot))
			{
				if (mBoss->mField0e4 == 0xf || mBoss->mField0e4 == 0xc)
					mField0f68++;
				mField0f60++;
				mFlag0d38 = true;
				mTypeTimer->Reset();
				break;
			}
			aShotIndex++;
		}
	}

	if (mTarget != NULL)
		return;

	// no creature took the letter: a typo
	mApp->PlaySample(SOUND_WRONG);
	if (mLessonManager->GetCurrentWaveType() != 1 && (mBoss == NULL || mBoss->mField0e4 == 0xf || mBoss->mField0e4 == 0xc))
		mField0f6c++;
	mField0f64++;
	std::string aMessage = StrFormat(GTC(0x00530c18), (int)aChar);
	SetBannerText(aMessage);
	if (mLessonManager->GetCurrentWaveType() == 1)
	{
		mField0e94++;
		if (mField0e94 >= mLessonManager->GetLevelSettings()->mNumForPenalty)
		{
			int aPlayerState = mPlayer->mState;
			if (aPlayerState == 9 || aPlayerState == 0 || aPlayerState == 4)
			{
				mField0e94 = 0;
				AddPiranha();
			}
		}
	}
}
