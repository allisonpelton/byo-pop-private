#include "TorpedoBoss.h"
#include "Torpedo.h"
#include "WinWSApp.h"
#include "WSTimer.h"
#include "WidgetManager.h"
#include "Board.h"
#include "LessonManager.h"
#include "Graphics.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and destructors): GameObject slots 0 (0x408df0, deleting destructor), 1 (0x40e0a0, to
// 0x40cdc0), 2 (0x408e00, to 0x405270), 3 and 4 (0x408dd0 and 0x408de0, to the Enemy level 0x402180 and 0x4021a0) and 7
// (0x40e0f0, to 0x40ca50)

// 0x40c630: ProjectileBoss (0x40b030) first, then the pictures, the second timer and the numbers of this boss. Its way
// runs from mMinX, a little left of the middle of the screen, to mMaxX, and bobs between two heights above the status
// bar. Outside the abyss mode the numbers of shots and their speeds are the first record's, in the abyss mode the
// boss gets hit points, word lengths and speeds by the score: the higher the score, the more of each. The boss starts
// at the right edge of the screen, at the lower end of its way
TorpedoBoss::TorpedoBoss(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig) :
	ProjectileBoss(theApp, thePlayer, theConfig),
	mMinWordLength(0),
	mMaxWordLength(0),
	mRotorFrame(0),
	mRotorFrames(0),
	mSlot1Taken(false),
	mSlot2Taken(false),
	mRotorImage(NULL),
	mRotorOffsetX(0),
	mRotorOffsetY(0),
	mRotorWidth(0),
	mSlot3Taken(false),
	mSonarTimer(NULL)
{
	mSlotOffsetY[0] = 0;
	mSlotOffsetY[1] = 0;
	mSlotOffsetY[2] = 0;

	mImages[0] = mApp->mImages[IMAGE_TORPEDO_BOSS_BOBBING];
	mImages[2] = mApp->mImages[IMAGE_TORPEDO_BOSS_DYING];
	mFrameWidth = mImages[0]->GetWidth();
	mFrameHeight = mImages[0]->GetHeight();
	mSonarTimer = new WSTimer();
	mApp->mWidgetManager->AddWidget(mSonarTimer);
	mSonarTimer->Reset();

	mMinX = (double)(mApp->mWidth - 0x109);
	mMaxX = (double)(mFrameWidth >> 1) + mMinX;
	mMinY = (double)(mApp->mHeight - mFrameHeight - mApp->mImages[IMAGE_STATUSBARX]->GetHeight() - 0xa0);
	mMaxY = (double)(mApp->mHeight - mFrameHeight - mApp->mImages[IMAGE_STATUSBARX]->GetHeight() - 0x32);

	if (mConfig->mAbyssMode == 0)
	{
		mField130 = mConfig->GetEnemyRecord(0)->mMinMissiles;
		mMaxShots = mConfig->GetEnemyRecord(0)->mMaxMissiles;
		mShotSpeedMin = mConfig->GetEnemyRecord(0)->mShotSpeedMin;
		mShotSpeedMax = mConfig->GetEnemyRecord(0)->mShotSpeedMax;
	}
	else
	{
		double aMore = 0.0;
		mMinWordLength = 3;
		mMaxWordLength = 4;
		int aScore = mApp->mBoard->mField1c4;	// +0x1c4 of the board: the score
		if (aScore >= 13000)
		{
			aMore = 1.7;
			mField10c = 400;
			mHitPoints = 400;
			mMinWordLength = 7;
			mMaxWordLength = 7;
		}
		else if (aScore >= 10000)
		{
			aMore = 1.3;
			mField10c = 300;
			mHitPoints = 300;
			mMinWordLength = 7;
			mMaxWordLength = 7;
		}
		else if (aScore >= 7000)
		{
			aMore = 0.9;
			mMaxWordLength = 7;
			mField10c = 300;
			mHitPoints = 300;
			mMinWordLength = 5;
		}
		else if (aScore >= 4000)
		{
			aMore = 0.5;
			mMaxWordLength = 6;
			mField10c = 200;
			mHitPoints = 200;
			mMinWordLength = 5;
		}
		mField130 = 2;
		mMaxShots = 3;
		mShotSpeedMin = 0.4 + aMore;
		mShotSpeedMax = aMore + 0.5;
	}

	mSlot3Taken = false;
	mSlot2Taken = false;
	mSlot1Taken = false;
	mFrameDelay = 10;
	mNumFrames = 1;
	mY = mMaxY;
	mX = (double)mApp->mWidth;
	mSlotOffsetY[0] = 0x41;
	mSlotOffsetY[1] = 0x65;
	mSlotOffsetY[2] = 0x87;
	mRotorImage = mApp->mImages[IMAGE_TORPEDO_BOSS_ROTOR];
	mRotorOffsetX = 0xe3;
	mRotorOffsetY = 0x36;
	mRotorWidth = 0x16;
	mRotorFrame = 0;
	mRotorFrames = mRotorImage->GetWidth() / 22;
}

// 0x408d00: ProjectileBoss's destructor first (shots, timer), then the second timer leaves the widget manager and is
// deleted, and the explosions are freed
TorpedoBoss::~TorpedoBoss()
{
	mApp->mWidgetManager->RemoveWidget(mSonarTimer);
	delete mSonarTimer;
	mExplosions.clear();
}

// 0x4055c0: the word of a shot, from the app (0x428d80)
std::string TorpedoBoss::GetWord(int theMinLength, int theMaxLength)
{
	return mApp->GetWord(theMinLength, theMaxLength);
}

// 0x40d5b0: a torpedo (new, 0x214 bytes, with the boss as the target it can hit and the player's cannon) 100 to the
// left of the boss, shown
Projectile* TorpedoBoss::CreateProjectile()
{
	Torpedo* aShot = new Torpedo(mApp, this, mPlayer);
	aShot->SetX(mX - 100.0);
	aShot->SetVisible(true);
	return aShot;
}

// 0x4054b0: whether a new shot (given its y and size, and the sizes of the shots flying right) is in the way of one of
// them: the shot's bottom (y plus height) is below the new shot's y and the shot's y is above the new shot's bottom.
// The x of the new shot is not looked at (the two x comparisons compare the old shot's x with itself plus a size)
bool TorpedoBoss::ShotOverlaps(Projectile* theNewShot)
{
	size_t i = 0;
	while (i < mShots.size())
	{
		Projectile* aShot = mShots[i];
		if (aShot->mState == 0)
		{
			int aShotX = (int)aShot->mX;
			int aShotY = (int)aShot->mY;
			int aShotWidth = aShot->GetFrameWidth();
			int aShotHeight = aShot->GetFrameHeight();
			int aNewY = (int)theNewShot->mY;
			int aNewWidth = theNewShot->GetFrameWidth();
			int aNewHeight = theNewShot->GetFrameHeight();
			if (aShotWidth + aShotX > aShotX)
			{
				if (aShotHeight + aShotY > aNewY)
				{
					if (aShotX < aNewWidth + aShotX)
					{
						if (aShotY < aNewHeight + aNewY)
							return true;
					}
				}
			}
		}
		i++;
	}
	return false;
}

// 0x40cba0: the events one shot has pushed, read until there are none: 0x13 (the shot hit the player's cannon) is
// passed on and sets every other shot to state 4, 0x14 (it hit the boss) takes the shot's hit points from the boss's and
// sets the boss to state 8 (which ends it when none are left), 5 (the shot is gone) frees its place and says true: the
// boss lets it go, 1 (its word was typed) is passed on and frees the place and turns the shot back (state 6), 0x15 is
// passed on, and 6 from a shot flying left too. A dying boss (state 3) passes on and sets nothing, and neither does
// one that has said 0xc (no hit points left)
bool TorpedoBoss::HandleShot(Projectile* theShot)
{
	int anEvent = theShot->PopEvent();
	while (anEvent != 0)
	{
		if (anEvent == 0x13)
		{
			if (mState != 3)
			{
				mEvents.push_back(0x13);
				size_t i = 0;
				while (i < mShots.size())
				{
					if (mShots[i] != theShot)
						mShots[i]->SetState(4);
					i++;
				}
			}
		}
		else if (anEvent == 0x14)
		{
			if (mState != 3)
			{
				mHitPoints -= theShot->mDamage;
				SetState(8);
			}
		}
		else if (anEvent == 5)
		{
			if (theShot->mSlot == 1)
				mSlot1Taken = false;
			else if (theShot->mSlot == 2)
				mSlot2Taken = false;
			else if (theShot->mSlot == 3)
				mSlot3Taken = false;
			return true;
		}
		else if (anEvent == 1)
		{
			if (!HasEvent(0xc))
			{
				if (mState != 3)
					mEvents.push_back(1);
			}
			if (theShot->mSlot == 1)
				mSlot1Taken = false;
			else if (theShot->mSlot == 2)
				mSlot2Taken = false;
			else if (theShot->mSlot == 3)
				mSlot3Taken = false;
			if (!HasEvent(0xc))
			{
				if (mState != 3)
					theShot->SetState(6);
			}
		}
		else if (anEvent == 0x15)
		{
			mEvents.push_back(0x15);
		}
		else if (anEvent == 6)
		{
			if (theShot->mVelX < 0.0)
				mEvents.push_back(6);
		}
		anEvent = theShot->PopEvent();
	}
	return false;
}

// 0x40ca50: ProjectileBoss::SetState (0x40b240), then the size of a frame: the whole of the boss picture in the states
// 0, 6 and 7, the whole of the dying picture in 3, which also puts the first explosion at a place within the boss's way
// (x between mMinX and the right edge, y between mMinY and mMaxY)
void TorpedoBoss::SetState(int theState)
{
	ProjectileBoss::SetState(theState);

	if (theState == 6 || theState == 0 || theState == 7)
	{
		mFrameWidth = mImages[0]->GetWidth();
		mFrameHeight = mImages[0]->GetHeight();
		mNumFrames = 1;
	}
	else if (theState == 3)
	{
		ExplosionRecord aRecord;
		aRecord.mFrame = 0;
		aRecord.mX = (double)(gGameRand->NextNoAssert() % (unsigned int)(int)((double)mApp->mWidth - mMinX)) + mMinX;
		aRecord.mY = (double)(gGameRand->NextNoAssert() % (unsigned int)(int)(mMaxY - mMinY)) + mMinY;
		mExplosions.push_back(aRecord);
		mFrameWidth = mImages[2]->GetWidth();
		mFrameHeight = mImages[2]->GetHeight();
		mNumFrames = 1;
	}
}

// 0x405270: GameObject::Draw (0x461270, the position history), then the boss: dying (3) it is the dying picture,
// coloured white with the alpha mAlpha (the picture is drawn with the colorize option on), and over it the explosions,
// each a frame of an eighth of the width of the torpedo explosion picture; otherwise the boss picture and the rotor
// (a frame of mRotorWidth pixels) at the stern. Last the shots
void TorpedoBoss::Draw(Graphics* g)
{
	GameObject::Draw(g);

	int aExplosionHeight = mApp->mImages[IMAGE_TORPEDO_EXPLODE]->GetHeight();
	int aExplosionWidth = mApp->mImages[IMAGE_TORPEDO_EXPLODE]->GetWidth() / 8;

	if (mState == 3)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mAlpha));
		g->DrawImage(mImages[2], (int)mLastX, (int)mLastY);
		g->SetColorizeImages(false);

		size_t i = 0;
		while (i < mExplosions.size())
		{
			ExplosionRecord* aRecord = &mExplosions[i];
			Rect aSrc(aRecord->mFrame * aExplosionWidth, 0, aExplosionWidth, aExplosionHeight);
			g->DrawImage(mApp->mImages[IMAGE_TORPEDO_EXPLODE], (int)aRecord->mX, (int)aRecord->mY, aSrc);
			i++;
		}
	}
	else
	{
		g->DrawImage(mImages[0], (int)mLastX, (int)mLastY);
		Rect aSrc(mRotorFrame * mRotorWidth, 0, mRotorWidth, mRotorImage->GetHeight());
		g->DrawImage(mRotorImage, (int)((double)mRotorOffsetX + mLastX), (int)((double)mRotorOffsetY + mLastY), aSrc);
	}

	size_t i = 0;
	while (i < mShots.size())
	{
		mShots[i]->Draw(g);
		i++;
	}
}

// 0x40cdc0: ProjectileBoss::Update (0x40b360), then the rotor turns (every mFrameDelay ticks, while the boss lives).
// Dying (3): every third tick each explosion goes on a frame and is dropped after the eighth, every tick it rises half
// a pixel, every seventh tick a new one starts at a random place (x in the boss's way, y within the boss's height, the
// first draw is x), every 33rd tick a bang plays. Alive, a sonar ping plays and the second timer restarts every 2
// seconds. Firing (7): the boss bobs; when the fire interval is over, the player's cannon is ready and a place is free,
// it counts the shots flying right (state 0) and fires as many as it may have in the air more (mMaxShots), each in the
// first free place where it is not in the way of another shot; the timer starts again. A launch plays the launch sound
// (the first only), makes 30 bubbles at the place, says 0xd, gives the shot a speed (a random one between the
// slowest and the fastest, the abyss mode adds more by the score and a pace of the frames by it) and a word of the
// lengths mMinWordLength to mMaxWordLength (outside the abyss mode those of the last record of the lesson manager)
void TorpedoBoss::Update()
{
	ProjectileBoss::Update();

	if (mTick % mFrameDelay == 0)
	{
		if (mState != 3)
		{
			mRotorFrame++;
			if (mRotorFrame >= mRotorFrames)
				mRotorFrame = 0;
		}
	}

	if (mState == 3)
	{
		size_t i = 0;
		while (i < mExplosions.size())
		{
			if (mTick % 3 == 0)
			{
				mExplosions[i].mFrame++;
				if (mExplosions[i].mFrame >= 8)
					mExplosions.erase(mExplosions.begin() + i);
			}
			if (i < mExplosions.size())	// port: the original writes the erased last slot, which nothing reads
				mExplosions[i].mY = mExplosions[i].mY - 0.5;
			i++;
		}

		if (mTick % 7 == 0)
		{
			ExplosionRecord aRecord;
			aRecord.mFrame = 0;
			aRecord.mX = (double)(gGameRand->NextNoAssert() % (unsigned int)(int)((double)mApp->mWidth - mMinX)) + mMinX;
			aRecord.mY = (double)(gGameRand->NextNoAssert() % (unsigned int)(mFrameHeight - (mApp->mImages[IMAGE_TORPEDO_EXPLODE]->GetHeight() >> 1))) + mY;
			mExplosions.push_back(aRecord);
		}
		if (mTick % 33 == 0)
			mApp->PlaySample(SOUND_EXPLOSION);
	}
	else
	{
		if (mSonarTimer->Elapsed() >= 2000)
		{
			mApp->PlaySample(SOUND_SONARPING1);
			mSonarTimer->Reset();
		}
	}

	if (mState == 7)
	{
		Move();
		if ((double)mTimer->Elapsed() >= mFireInterval)
		{
			if (mPlayer->mState == 9)
			{
				if (!(mSlot1Taken && mSlot2Taken && mSlot3Taken))
				{
					int anAlive = 0;
					size_t j = 0;
					while (j < mShots.size())
					{
						if (mShots[j]->mState == 0)
							anAlive++;
						j++;
					}
					if (anAlive < mMaxShots)
					{
						int aCount = mMaxShots - anAlive;
						int i = 0;
						while (i < aCount)
						{
							int aMinLength;
							int aMaxLength;
							if (mConfig->mAbyssMode == 0)
								aMinLength = mConfig->GetEnemyRecord(mConfig->GetEnemyRecordCount() - 1)->mWordMinLength;
							else
								aMinLength = mMinWordLength;
							if (mConfig->mAbyssMode == 0)
								aMaxLength = mConfig->GetEnemyRecord(mConfig->GetEnemyRecordCount() - 1)->mWordMaxLength;
							else
								aMaxLength = mMaxWordLength;

							Projectile* aShot = NULL;
							if (!mSlot1Taken)
							{
								aShot = CreateProjectile();
								aShot->SetY((double)mSlotOffsetY[0] + mY);
								if (ShotOverlaps(aShot))
								{
									delete aShot;
									aShot = NULL;
								}
								else
								{
									mSlot1Taken = true;
									aShot->mSlot = 1;
								}
							}
							if (!mSlot2Taken && aShot == NULL)
							{
								aShot = CreateProjectile();
								aShot->SetY((double)mSlotOffsetY[1] + mY);
								if (ShotOverlaps(aShot))
								{
									delete aShot;
									aShot = NULL;
								}
								else
								{
									mSlot2Taken = true;
									aShot->mSlot = 2;
								}
							}
							if (!mSlot3Taken && aShot == NULL)
							{
								aShot = CreateProjectile();
								aShot->SetY((double)mSlotOffsetY[2] + mY);
								if (ShotOverlaps(aShot))
								{
									delete aShot;
									aShot = NULL;
								}
								else
								{
									mSlot3Taken = true;
									aShot->mSlot = 3;
								}
							}

							if (aShot != NULL)
							{
								if (i == 0)
									mApp->PlaySample(SOUND_TORPEDOLAUNCH);

								double aShotX = aShot->mX;
								int aBubbleX = (int)((double)aShot->GetFrameWidth() + aShotX - 20.0);
								int aBubbleY = (int)aShot->mY;
								int aBubbles = 30;
								do
								{
									Board* aBoard = mApp->mBoard;
									int aY = (int)(gGameRand->NextNoAssert() % 10) + aBubbleY;
									int aX = (int)(gGameRand->NextNoAssert() % 40) + aBubbleX;
									aBoard->AddBubble(aX, aY);
									aBubbles--;
								}
								while (aBubbles != 0);

								mEvents.push_back(0xd);

								int aRange = (int)((mShotSpeedMax - mShotSpeedMin) * 1000.0);
								if (aRange == 0)
									aRange = 1;
								double aSpeed = (double)(gGameRand->NextNoAssert() % (unsigned int)aRange) * 0.001;
								if (mConfig->mAbyssMode == 0)
									aShot->SetVelX(-(aSpeed + mShotSpeedMin));
								else
									aShot->SetVelX(-((double)(mApp->mBoard->mField1c4 / 1000 - 1) * 0.08 + mShotSpeedMin + aSpeed));

								// the speed is negative, so the first comparison always holds
								if (aShot->mVelX <= 0.3)
									aShot->SetFrameDelay(6);
								else if (aShot->mVelX <= 0.4)
									aShot->SetFrameDelay(5);
								else if (aShot->mVelX <= 0.5)
									aShot->SetFrameDelay(4);
								else
									aShot->SetFrameDelay(3);

								aShot->SetWord(GetWord(aMinLength, aMaxLength));
								mShots.push_back(aShot);
							}
							i++;
						}
					}
					mTimer->Reset();
				}
			}
		}
	}
}
