// BoardEvents.cpp: the Board's event loops (job B1). Each creature queues small event codes in Enemy::mEvents (docs/BOARD.md,
// part 5); Update calls one of these for every creature of a list and the handler drains the queue. A handler returns true
// when it removed the creature from its list and deleted it (the creature is then gone: Update does not advance its index),
// false when the queue ran empty.
//
// The lists hold the Enemy part of each creature (Board.h); a Shark or Piranha is reached with dynamic_cast, the way the
// original reads the virtual-base offset. The original's inlined vector<T*>::erase loop ("copy the tail down by one") is
// std::vector::erase, and the deleting-destructor call through the GameObject part is a plain delete.

#include <algorithm>
#include "Board.h"
#include "Enemy.h"
#include "Shark.h"
#include "Piranha.h"
#include "Clam.h"
#include "WSTimer.h"
#include "LessonManager.h"
#include "UserProfile.h"
#include "UserProfileMgr.h"
#include "GameText.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// The creatures of the lists ask their own slot 0 (Shark::IsSwimming, Piranha::IsSwimming) through the Shark or Piranha
// part of the original's object.
static bool IsSharkSwimming(Enemy* theEnemy)
{
	return dynamic_cast<Shark*>(theEnemy)->IsSwimming();
}

static bool IsPiranhaSwimming(Enemy* theEnemy)
{
	return dynamic_cast<Piranha*>(theEnemy)->IsSwimming();
}

// 0x457480: the sharks' event loop. 2 the diver is bitten, 7 the shark turns at the left edge, 8 a stealth shark shows up
// (30 bubbles), 4 the other swimming sharks are chased off, 5 the shark left the screen (removed and deleted, returns
// true), 6 it died (the group may be over), 1 its word was finished (points, the shock state)
bool Board::HandleSharkEvents(Enemy* theEnemy)
{
	int anEvent = theEnemy->PopEvent();
	while (anEvent != 0)
	{
		if (anEvent == 2)
		{
			int aState = mPlayer->mState;
			if (aState != 3 && aState != 2)
			{
				mPlayer->mFlag144 = true;
				mPlayer->SetState(3);
			}
		}
		else if (anEvent == 7)
		{
			theEnemy->SetY(mPlayer->mY);
		}
		else if (anEvent == 8)
		{
			int aCount = 0x1e;
			do
			{
				double aX = theEnemy->mX;
				double aY = theEnemy->mY;
				unsigned long aRandY = gGameRand->NextNoAssert() % 10;
				int aBubbleY = (int)((double)aRandY + aY + 15.0);
				unsigned long aRandX = gGameRand->NextNoAssert() % 0x28;
				int aBubbleX = (int)((double)aRandX + aX);
				AddBubble(aBubbleX, aBubbleY);
				aCount = aCount - 1;
			} while (aCount != 0);
			mApp->PlaySample(SOUND_DIVERENTERS);
		}
		else if (anEvent == 4)
		{
			unsigned int i = 0;
			while (i < mSharks.size())
			{
				Enemy* aShark = mSharks[i];
				if (aShark != theEnemy && IsSharkSwimming(aShark))
					aShark->SetState(4);
				i++;
			}
		}
		else if (anEvent == 5)
		{
			unsigned int i = 0;
			while (i < mSharks.size())
			{
				Enemy* aShark = mSharks[i];
				if (aShark == theEnemy)
				{
					if (aShark == mTarget)
					{
						mTyped = "";
						if (aShark->HasEvent(1))
							aShark->RemoveEvent(1);
						mTarget = NULL;
					}
					delete aShark;
					mSharks.erase(mSharks.begin() + i);
					return true;
				}
				i++;
			}
			return true;	// not in the list: the original returns true all the same
		}
		else if (anEvent == 6)
		{
			// the group is over once every shark has finished dying (and none is still waiting to report its own death)
			bool aStillDying = false;
			unsigned int i = 0;
			while (i < mSharks.size())
			{
				Enemy* aShark = mSharks[i];
				if (!aShark->IsDeathFinished())
				{
					aStillDying = true;
					break;
				}
				aShark = mSharks[i];
				if (aShark->mState == 3 && aShark->HasEvent(6))
				{
					aStillDying = true;
					break;
				}
				i++;
			}

			if (!aStillDying)
			{
				mFlag0f80 = true;
				i = 0;
				while (i < mSharks.size())
				{
					mSharks[i]->RemoveEvent(6);
					i++;
				}

				if (CountActiveLists() == 1)
				{
					mFlag0f80 = true;
					if (!mFlag0eb5 && !mRating->mActive)
					{
						mRatingTimer->SetDisabled(true);
						ShowRating();
					}
					else
						mRatingTimer->Reset();

					UserProfile* aUser = mApp->mUser;
					if (!aUser->mFlag1dc)
					{
						aUser->mFlag1dc = true;
						mApp->mProfileMgr->WriteUserProfileToRegistry(aUser, false, false);
						mPlayer->SetFlag118(true);
					}
				}
			}
		}
		else if (anEvent == 1)
		{
			// the word is finished: it is worth the points of the word, times score / 600 (at least 2) in an abyss game
			int aPoints = theEnemy->mField54;
			if (mLessonManager->mAbyssMode)
			{
				int aFactor = mScore / 600;
				if (aFactor < 3)
					aFactor = 2;
				aPoints = aFactor * aPoints;
			}

			if (!mFlag0eb5)
				mTypeTime = mTypeTime + mTypeTimer->Elapsed();

			mRating->mPoints += aPoints;
			mPlayer->AddScore(aPoints);
			theEnemy->SetState(5);
			if (!IsSharkSwimming(theEnemy))
				mApp->mUser->CountSharkKilled(mOceanDepth == -1);

			AddBonusText(StrFormat(GTC(0x00530c5c), aPoints), (int)theEnemy->mX, (int)theEnemy->mY);
			ResetPiranhaSpeed();

			// back to the quiet music once no swimming shark is near the diver any more
			if (mApp->mMusicOrder != 0)
			{
				mApp->mFlag671 = true;
				bool aNear = false;
				unsigned int i = 0;
				while (i < mSharks.size())
				{
					Enemy* aShark = mSharks[i];
					if (IsSharkSwimming(aShark) && (int)aShark->mX <= 0xa0)
					{
						aNear = true;
						break;
					}
					i++;
				}
				if (!aNear)
					mApp->ChangeMusic(0);
			}

			if (theEnemy == mTarget)
			{
				mTyped = "";
				mTarget = NULL;
			}
		}

		anEvent = theEnemy->PopEvent();
	}
	return false;
}

// 0x457ad0: the piranhas' event loop. The same codes as the sharks'; the differences: the diver's byte at +0x144 is cleared
// (not set), code 4 chases off the piranhas in state 0, code 5 does not clear the typing target, code 6 only asks whether
// each piranha has finished dying, and code 1 adds nothing to mTypeTime and counts a piranha killed
bool Board::HandlePiranhaEvents(Enemy* theEnemy)
{
	int anEvent = theEnemy->PopEvent();
	while (anEvent != 0)
	{
		if (anEvent == 2)
		{
			int aState = mPlayer->mState;
			if (aState != 3 && aState != 2)
			{
				mPlayer->mFlag144 = false;
				mPlayer->SetState(3);
			}
		}
		else if (anEvent == 7)
		{
			theEnemy->SetY(mPlayer->mY);
		}
		else if (anEvent == 8)
		{
			int aCount = 0x1e;
			do
			{
				double aX = theEnemy->mX;
				double aY = theEnemy->mY;
				unsigned long aRandY = gGameRand->NextNoAssert() % 10;
				int aBubbleY = (int)((double)aRandY + aY + 15.0);
				unsigned long aRandX = gGameRand->NextNoAssert() % 0x28;
				int aBubbleX = (int)((double)aRandX + aX);
				AddBubble(aBubbleX, aBubbleY);
				aCount = aCount - 1;
			} while (aCount != 0);
			mApp->PlaySample(SOUND_DIVERENTERS);
		}
		else if (anEvent == 4)
		{
			unsigned int i = 0;
			while (i < mPiranhas.size())
			{
				Enemy* aPiranha = mPiranhas[i];
				if (aPiranha != theEnemy && aPiranha->mState == 0)
					aPiranha->SetState(4);
				i++;
			}
		}
		else if (anEvent == 5)
		{
			unsigned int i = 0;
			while (i < mPiranhas.size())
			{
				Enemy* aPiranha = mPiranhas[i];
				if (aPiranha == theEnemy)
				{
					if (aPiranha->HasEvent(1))
						aPiranha->RemoveEvent(1);
					delete aPiranha;
					mPiranhas.erase(mPiranhas.begin() + i);
					return true;
				}
				i++;
			}
			return true;	// not in the list: the original returns true all the same
		}
		else if (anEvent == 6)
		{
			bool aStillDying = false;
			unsigned int i = 0;
			while (i < mPiranhas.size())
			{
				if (!mPiranhas[i]->IsDeathFinished())
				{
					aStillDying = true;
					break;
				}
				i++;
			}

			if (!aStillDying)
			{
				mFlag0f80 = true;
				i = 0;
				while (i < mPiranhas.size())
				{
					mPiranhas[i]->RemoveEvent(6);
					i++;
				}

				if (CountActiveLists() == 1)
				{
					mFlag0f80 = true;
					if (!mFlag0eb5 && !mRating->mActive)
					{
						mRatingTimer->SetDisabled(true);
						ShowRating();
					}
					else
						mRatingTimer->Reset();
				}
			}
		}
		else if (anEvent == 1)
		{
			int aPoints = theEnemy->mField54;
			if (mLessonManager->mAbyssMode)
			{
				int aFactor = mScore / 600;
				if (aFactor < 3)
					aFactor = 2;
				aPoints = aFactor * aPoints;
			}

			mRating->mPoints += aPoints;
			mPlayer->AddScore(aPoints);
			theEnemy->SetState(5);
			if (!IsPiranhaSwimming(theEnemy))
				mApp->mUser->CountPiranhaKilled(mOceanDepth == -1);

			AddBonusText(StrFormat(GTC(0x00530c5c), aPoints), (int)theEnemy->mX, (int)theEnemy->mY);
			ResetPiranhaSpeed();

			if (mApp->mMusicOrder != 0)
			{
				mApp->mFlag671 = true;
				bool aNear = false;
				unsigned int i = 0;
				while (i < mPiranhas.size())
				{
					Enemy* aPiranha = mPiranhas[i];
					if (IsPiranhaSwimming(aPiranha) && (int)aPiranha->mX <= 0xa0)
					{
						aNear = true;
						break;
					}
					i++;
				}
				if (!aNear)
					mApp->ChangeMusic(0);
			}

			if (theEnemy == mTarget)
			{
				mTyped = "";
				mTarget = NULL;
			}
		}

		anEvent = theEnemy->PopEvent();
	}
	return false;
}

// 0x458030: the clams' event loop (always returns false). 2 a clam closed (state 4); 3 a clam with a pearl began to open
// and pairs with the clam that was opened before (+100000 points for the first pair, +200000 for the next ones; the
// KeyChar side sets mOpenClam); 1 a clam is open: an empty one (type 0) closes again at once with the "wrong" sound
// and takes the open clam with it, and after the second pair the third one shows the treasure
bool Board::HandleClamEvents(Enemy* theEnemy)
{
	int anEvent = theEnemy->PopEvent();
	while (anEvent != 0)
	{
		if (anEvent == 2)
		{
			theEnemy->SetState(4);
		}
		else if (anEvent == 3)
		{
			Enemy* anOpenClam = mOpenClam;
			if (anOpenClam != NULL && theEnemy != anOpenClam
				&& static_cast<Clam*>(anOpenClam)->mType == static_cast<Clam*>(theEnemy)->mType)
			{
				mPairsFound = mPairsFound + 1;
				std::string aText;
				if (mPairsFound == 1)
				{
					mApp->PlaySample(SOUND_WAVE_BONUS3);
					mPlayer->AddScore(100000);
					aText = StrFormat(GTC(0x00530c5c), 100000);
				}
				else
				{
					mApp->PlaySample(SOUND_WINGEM);
					mPlayer->AddScore(200000);
					aText = StrFormat(GTC(0x00530c5c), 200000);
				}
				AddBonusText(aText, (int)theEnemy->mX, (int)theEnemy->mY);
				mOpenClam = NULL;
			}
		}
		else if (anEvent == 1)
		{
			if (static_cast<Clam*>(theEnemy)->mType == 0)
			{
				mApp->PlaySample(SOUND_BAD);
				theEnemy->SetState(2);
				if (mOpenClam != NULL)
				{
					mOpenClam->SetState(2);
					mOpenClam = NULL;
				}
			}
			else if (mPairsFound == 2)
				ShowTreasure();
		}

		anEvent = theEnemy->PopEvent();
	}
	return false;
}

// 0x4582c0: the bonus creatures' event loop. 5 the creature left the screen (removed and deleted; unlike the shark loops
// it returns false when the creature is not in the list), 6 it died (the group may be over), 1 its word was finished:
// 200 points for the first, 400 for the second... (mField0d64 counts them), a rising tone, the EMP charge grows
bool Board::HandleBonusCreatureEvents(Enemy* theEnemy)
{
	int anEvent = theEnemy->PopEvent();
	while (anEvent != 0)
	{
		if (anEvent == 5)
		{
			if (mTarget == theEnemy)
			{
				if (theEnemy->HasEvent(1))
					theEnemy->RemoveEvent(1);
				mTarget = NULL;
			}

			unsigned int i = 0;
			while (i < mBonusCreatures.size())
			{
				Enemy* aCreature = mBonusCreatures[i];
				if (aCreature == theEnemy)
				{
					delete aCreature;
					mBonusCreatures.erase(mBonusCreatures.begin() + i);
					return true;
				}
				i++;
			}
		}
		else if (anEvent == 6)
		{
			if (theEnemy == mTarget)
			{
				mTyped = "";
				mTarget = NULL;
			}

			bool aStillDying = false;
			unsigned int i = 0;
			while (i < mBonusCreatures.size())
			{
				if (!mBonusCreatures[i]->IsDeathFinished())
				{
					aStillDying = true;
					break;
				}
				i++;
			}

			if (!aStillDying)
			{
				mFlag0f80 = true;
				i = 0;
				while (i < mBonusCreatures.size())
				{
					mBonusCreatures[i]->RemoveEvent(6);
					i++;
				}

				if (CountActiveLists() == 1)
				{
					mFlag0f80 = true;
					if (!mFlag0eb5 && !mRating->mActive)
					{
						mRatingTimer->SetDisabled(true);
						ShowRating();
					}
					else
						mRatingTimer->Reset();
				}
			}
		}
		else if (anEvent == 1)
		{
			if (!mFlag0eb5)
				mTypeTime = mTypeTime + mTypeTimer->Elapsed();

			mField0d64 = mField0d64 + 1;
			int aCount = mField0d64;
			int aPoints = aCount * 200;
			if (mLessonManager->mAbyssMode)
			{
				int aFactor = mScore / 600;
				if (aFactor < 3)
					aFactor = 2;
				aPoints = aFactor * aPoints;
			}

			int aSound = aCount + 0xf;
			if (aSound > 0x1b)
				aSound = 0x1b;
			mApp->PlaySample(aSound);

			mRating->mPoints += aPoints;
			mPlayer->AddScore(aPoints);
			if (theEnemy->mState != 3)
				theEnemy->SetState(3);

			if (theEnemy == mTarget)
			{
				mTyped = "";
				mTarget = NULL;
			}

			mPlayer->mEmpCharge = mPlayer->mEmpCharge + 0.01;
			if (mPlayer->mEmpCharge >= 1.0)
				mPlayer->mEmpCharge = 1.0;

			AddBonusText(StrFormat(GTC(0x00530c5c), aPoints), (int)theEnemy->mX, (int)theEnemy->mY);
		}

		anEvent = theEnemy->PopEvent();
	}
	return false;
}

// 0x4586b0: the boss's event loop (the boss is mBoss; theEnemy is the creature whose queue is drained, in practice the boss
// itself). 1 a word of the boss's shots was typed (the typing target's points, the letters it used are free again),
// 0x13 a shot hit the diver, 6 the boss died (the diver's state 2), 5 the boss left (deleted, returns true), 0x15 a shot
// is gone, 0xc the boss is beaten (the profile counts it, GetBossBonus points, the level ends when the diver is in
// state 9), 0xa the boss exists (the music of order 0x30)
bool Board::HandleBossEvents(Enemy* theEnemy)
{
	int anEvent = theEnemy->PopEvent();
	while (anEvent != 0)
	{
		if (anEvent == 1)
		{
			if (mTarget != NULL)
			{
				int aPoints = mTarget->mField54;
				if (mLessonManager->mAbyssMode)
				{
					int aFactor = mScore / 600;
					if (aFactor < 3)
						aFactor = 2;
					aPoints = aFactor * aPoints;
				}

				mPlayer->AddScore(aPoints);
				mTyped = "";

				std::string aWord = mTarget->mWord;		// GetWord() 0x4070c0 is a copy of the word
				mApp->RemoveLetter(aWord[0]);

				mTypeTime = mTypeTime + mTypeTimer->Elapsed();

				AddBonusText(StrFormat(GTC(0x00530c5c), aPoints), (int)mTarget->mX, (int)mTarget->mY);
				if (mTarget->mX <= 160.0)
					mApp->mFlag671 = true;
				mTarget = NULL;
			}
		}
		else if (anEvent == 0x13)
		{
			if (theEnemy->HasEvent(1))
				theEnemy->RemoveEvent(1);
			if (theEnemy->HasEvent(0x13))
				theEnemy->RemoveEvent(0x13);

			WinWSApp* anApp = mApp;
			anApp->mUsedLetters = "";
			anApp->mUsedLetterCount = 0;
			anApp->mUsedUpperCount = 0;
			mTarget = NULL;
			mTyped = "";
			mTypeTimer->Reset();
			mPlayer->SetState(10);
		}
		else if (anEvent == 6)
		{
			mPlayer->SetState(2);
		}
		else if (anEvent == 5)
		{
			if (mBoss != NULL)
				delete mBoss;
			mBoss = NULL;
			return true;
		}
		else if (anEvent == 0x15)
		{
			if (mTarget != NULL)
			{
				// a shot that is not all on the screen any more is let go: the target is deleted, its letters are free
				bool aRelease = true;
				double aX = mTarget->mX;
				int aWidth = mTarget->GetFrameWidth();
				if ((double)(-aWidth) < aX)
				{
					double aX2 = mTarget->mX;
					int aWidth2 = mTarget->GetFrameWidth();
					double aRight = (double)aWidth2 + aX2;
					if ((double)mApp->mWidth > aRight)
						aRelease = false;
				}

				if (aRelease)
				{
					std::string aWord = mTarget->mWord;
					mApp->RemoveLetter(aWord[0]);

					if (mTarget != NULL)
					{
						// the shot is on the retired list too (see Board::Update): it is freed here, so it must not be freed again
						mRetired.erase(std::remove(mRetired.begin(), mRetired.end(), mTarget), mRetired.end());
						delete mTarget;
					}
					mTarget = NULL;
					mTyped = "";
					mTypeTimer->Reset();
				}
			}
		}
		else if (anEvent == 0xc)
		{
			if (!mLessonManager->mAbyssMode)
				mApp->mUser->mBossesKilled = mApp->mUser->mBossesKilled + 1;
			else
				mApp->mUser->CountKill(mBoss->mField0e4);

			mTarget = NULL;
			mTyped = "";
			mTypeTimer->Reset();

			AddBonusText(StrFormat(GTC(0x00530c5c), GetBossBonus()), 0x136, 0xfa);

			if (mPlayer->mState == 9)
			{
				if (!mLessonManager->mAbyssMode)
					mFlag0ebc = true;
				else
					mApp->ChangeMusic(0);

				mFlag0f80 = true;
				mPlayer->SetState(0);
				mPlayer->AddScore(GetBossBonus());
				mPlayer->SetFlag118(true);
			}
		}
		else if (anEvent == 10)
		{
			mApp->ChangeMusic(0x30);
		}

		anEvent = theEnemy->PopEvent();
	}
	return false;
}
