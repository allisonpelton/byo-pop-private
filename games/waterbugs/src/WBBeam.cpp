// The ship's beams and the beamriders that run back along them (0x402830..0x4035d0)

#include "WBLevel.h"

#include <cmath>

namespace wb
{

void Beam_Setup(Beam* theBeam, Shooter* theOwner, float theX, float theY, float theVx, float theVy)
{
	theBeam->mStartX = theX;
	theBeam->mOwner = theOwner;
	theBeam->mDepth = 6;
	theBeam->mStartY = theY;
	theBeam->mDrawKind = 1;
	theBeam->mHit = 0;
	theBeam->mVx = theVx;
	theBeam->mVy = theVy;
	const char* aName = nullptr;
	if (0.0f < theVx)
		aName = "shooter_beam_right";
	else if (theVx < 0.0f)
		aName = "shooter_beam_left";
	else if (0.0f < theVy)
		aName = "shooter_beam_down";
	else if (theVy < 0.0f)
		aName = "shooter_beam_up";
	if (aName != nullptr)
		theBeam->mAnim = Animation_Find(aName, &theBeam->mFrameCount, &theBeam->mFrameTime);
	// Water Bugs' beam has a harpoon at its head, going the same way
	const char* aTip = nullptr;
	if (0.0f < theVx)
		aTip = "harpoon_right";
	else if (theVx < 0.0f)
		aTip = "harpoon_left";
	else if (0.0f < theVy)
		aTip = "harpoon_down";
	else if (theVy < 0.0f)
		aTip = "harpoon_up";
	if (aTip != nullptr)
		theBeam->mHarpoon.mAnim = Animation_Find(aTip, &theBeam->mHarpoon.mFrameCount, &theBeam->mHarpoon.mFrameTime);
	theBeam->mHarpoon.mDrawKind = 1;
	theBeam->mHarpoon.mX = theBeam->mX;
	theBeam->mHarpoon.mMidHandle = 1;
	theBeam->mHarpoon.mDirty = 1;
	theBeam->mHarpoon.mY = theBeam->mY;
	theBeam->mHarpoon.mDepth = 7;
	theBeam->mHarpoon.mVx = theBeam->mVx;
	theBeam->mHarpoon.mVy = theBeam->mVy;
}

// It starts on the beam level with the enemy and runs the other way at twice the beam's speed
void Rider_Setup(Rider* theRider, Beam* theBeam, float theX, float theY, Enemy* theEnemy)
{
	if (theBeam->mVx == 0.0f)
	{
		theRider->mX = theBeam->mX;
		theRider->mY = theY;
	}
	else
	{
		theRider->mY = theBeam->mY;
		theRider->mX = theX;
	}
	theRider->mFirstImage = Bank_IndexOf(&gArtBank, "particle_0");
	theRider->mLastImage = Bank_IndexOf(&gArtBank, "particle_0");
	theRider->mVx = -theBeam->mVx * 2.0f;
	theRider->mDrawKind = 1;
	theRider->mBeam = theBeam;
	theRider->mEnemy = theEnemy;
	theRider->mVy = -theBeam->mVy * 2.0f;
	theRider->mImage = Bank_ImageAt(&gArtBank, Bank_IndexOf(&gArtBank, "beamrider"));
	Image_SetMidHandle(theRider->mImage);
	theRider->mDirty = 1;
}

int Rider_Update(Rider* theRider)
{
	int aResult = 1;
	Obj_Update(theRider);
	if (0.0f <= theRider->mX && theRider->mX < (float)BOARD_WIDTH && 0.0f <= theRider->mY &&
		theRider->mY <= (float)BOARD_HEIGHT)
	{
		int aCol = (int)theRider->mX;
		int aRow = (int)theRider->mY;
		if (gLevels.mCurrent->mColumns[aCol / CELL_SIZE][aRow / CELL_SIZE].mKind != CELL_SOLID)
		{
			if (RandRange(1, 10) == 1)
			{
				ParticleSystem_New();
				int aLifetime = RandRange(400, 500);
				int aLast = theRider->mLastImage;
				int aFirst = theRider->mFirstImage;
				float aVy = (float)((double)RandRange(-50, 50) / 500.0);
				float aVx = (float)((double)RandRange(-50, 50) / 500.0);
				float aY = (float)RandRange(-5, 5) + theRider->mY;
				float aX = (float)RandRange(-5, 5) + theRider->mX;
				ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
					0xff, 0xff, 0xff, 0, 400, 10, aLifetime, 0.999f, 0.0002f, 5);
			}
			Shooter* aShooter = theRider->mBeam->mOwner;
			double aDy = (double)theRider->mY - (double)aShooter->mY;
			int aRadius = aShooter->mImage->mW / 2;
			double aDx = (double)theRider->mX - (double)aShooter->mX;
			double aDx2 = aDx * aDx;
			double aDy2 = aDy * aDy;
			if (aDx2 + aDy2 < (double)(aRadius * aRadius))
			{
				aShooter = theRider->mBeam->mOwner;
				aResult = 0;
				if ((aShooter->mFlags & 0x10) != 0)
				{
					// (the original works out a pan from x for a play call that takes none)
					Sound_Play(Sound_Find("audio\\invincibubble.wav"));
					return 0;
				}
				float aHitY = (float)((double)theRider->mVy / 12.0);
				float aHitX = (float)((double)theRider->mVx / 6.0);
				Shooter_Hit(aShooter, aHitX, aHitY);
				if (gLives != 0)
					gLives = gLives - 1;
				gProfiles[gProfile].mRiderHits = gProfiles[gProfile].mRiderHits + 1;
			}
			return aResult;
		}
	}
	return 0;
}

// From the start to the head, a frame of the beam's image every image length, clipped to the beam's rectangle
void Beam_DrawTo(Beam* theBeam, uint8_t* theDst, int theDPitch)
{
	int aRect[4];
	float aPos;
	Image* anImage;
	if (theBeam->mX <= theBeam->mStartX)
	{
		if (theBeam->mStartX <= theBeam->mX)
		{
			if (theBeam->mStartY < theBeam->mY)
			{
				// down
				theBeam->mImage->mHandleY = 0;
				theBeam->mImage->mHandleX = theBeam->mImage->mOrigW / 2;
				anImage = theBeam->mImage;
				aRect[0] = (int)((double)theBeam->mStartX - (double)(anImage->mOrigW / 2));
				aRect[1] = (int)theBeam->mStartY;
				aRect[2] = (int)((double)(anImage->mOrigW / 2) + (double)theBeam->mStartX);
				aRect[3] = (int)theBeam->mY;
				for (aPos = theBeam->mStartY; aPos <= theBeam->mY; aPos = (float)anImage->mH + aPos)
				{
					Image_BlitBlend(anImage, theDst, theDPitch, &gBackBuffer.mFmt, aRect, (int)theBeam->mX, (int)aPos);
					anImage = theBeam->mImage;
				}
			}
			else
			{
				if (theBeam->mStartY <= theBeam->mY)
					goto riders;
				// up
				theBeam->mImage->mHandleY = theBeam->mImage->mOrigH + -1;
				theBeam->mImage->mHandleX = theBeam->mImage->mOrigW / 2;
				anImage = theBeam->mImage;
				aRect[0] = (int)((double)theBeam->mStartX - (double)(anImage->mOrigW / 2));
				aRect[1] = (int)theBeam->mY;
				aRect[2] = (int)((double)(anImage->mOrigW / 2) + (double)theBeam->mStartX);
				aRect[3] = (int)theBeam->mStartY;
				for (aPos = theBeam->mStartY; theBeam->mY <= aPos; aPos = aPos - (float)anImage->mH)
				{
					Image_BlitBlend(anImage, theDst, theDPitch, &gBackBuffer.mFmt, aRect, (int)theBeam->mX, (int)aPos);
					anImage = theBeam->mImage;
				}
			}
		}
		else
		{
			// left
			theBeam->mImage->mHandleX = theBeam->mImage->mOrigW + -1;
			theBeam->mImage->mHandleY = theBeam->mImage->mOrigH / 2;
			aRect[0] = (int)theBeam->mX;
			anImage = theBeam->mImage;
			aRect[1] = (int)((double)theBeam->mStartY - (double)(anImage->mOrigH / 2));
			aRect[2] = (int)theBeam->mStartX;
			aRect[3] = (int)((double)(anImage->mOrigH / 2) + (double)theBeam->mStartY);
			for (aPos = theBeam->mStartX; theBeam->mX <= aPos; aPos = aPos - (float)anImage->mW)
			{
				Image_BlitBlend(anImage, theDst, theDPitch, &gBackBuffer.mFmt, aRect, (int)aPos, (int)theBeam->mY);
				anImage = theBeam->mImage;
			}
		}
	}
	else
	{
		// right
		theBeam->mImage->mHandleX = 0;
		theBeam->mImage->mHandleY = theBeam->mImage->mOrigH / 2;
		aRect[0] = (int)theBeam->mStartX;
		anImage = theBeam->mImage;
		aRect[1] = (int)((double)theBeam->mStartY - (double)(anImage->mOrigH / 2));
		aRect[2] = (int)theBeam->mX;
		aRect[3] = (int)((double)(anImage->mOrigH / 2) + (double)theBeam->mStartY);
		for (aPos = theBeam->mStartX; aPos <= theBeam->mX; aPos = (float)anImage->mW + aPos)
		{
			Image_BlitBlend(anImage, theDst, theDPitch, &gBackBuffer.mFmt, aRect, (int)aPos, (int)theBeam->mY);
			anImage = theBeam->mImage;
		}
	}
	Dirty_AddClippedRect(&gDirty, aRect, SCREEN_CLIP);
riders:
	List_Each(&theBeam->mRiders, [](Rider* theARider) { Obj_Draw(theARider); });
	if (theBeam->mHit == 0)
		Obj_Draw(&theBeam->mHarpoon);
}

void Beam_Draw(Beam* theBeam)
{
	Beam_DrawTo(theBeam, gScreenBits, gScreenPitch);
}

static int Sign(float theValue)
{
	if (theValue == 0.0f)
		return 0;
	if (theValue <= 0.0f)
		return -1;
	return 1;
}

// 0x4031f0: the head moves on while the cell ahead is open; the first time it is not, the beam pops in a ring of
// particles; the harpoon follows it, four pixels behind
int Beam_Update(Beam* theBeam)
{
	int aResult = 1;
	Obj_Update(theBeam);
	int aHit = theBeam->mHit;
	if (aHit != 0)
		theBeam->mFrameTimer = 0;
	// back to where it was, to look at the cell it moved into
	float aX = theBeam->mX - theBeam->mVx;
	theBeam->mX = aX;
	float aY = theBeam->mY - theBeam->mVy;
	theBeam->mY = aY;
	int aCol = (int)((double)theBeam->mVx + (double)aX);
	int aRow = (int)((double)aY + (double)theBeam->mVy);
	if (gLevels.mCurrent->mColumns[aCol / CELL_SIZE][aRow / CELL_SIZE].mKind == CELL_OPEN)
	{
		theBeam->mX = aX + theBeam->mVx;
		theBeam->mY = aY + theBeam->mVy;
		// (Cosmic Bugs' beam sometimes shed a sparkle here; this one only takes the number)
		if (RandMod(0x28) == 2)
			RandRange(-50, 50);
	}
	else
	{
		if (aHit == 0)
		{
			Sound_Play(theBeam->mOwner->mHitSound);
			for (int anAngle = 0; anAngle < 0x168; anAngle = anAngle + 0x14)
			{
				float aVx = (float)((double)CosDeg(anAngle) * 0.11999999731779099);
				float aVy = (float)((double)SinDeg(anAngle) * 0.11999999731779099);
				ParticleSystem_New();
				Cell* aCell = gLevels.mCurrent->mColumns[0];
				ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, theBeam->mX, theBeam->mY, aVx, aVy,
					aCell->mFirstParticle, aCell->mLastParticle, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x21c, 10, 400,
					0.999f, 0.0f, 5);
			}
		}
		theBeam->mHit = 1;
		aResult = 0;
	}
	List<Rider>* aRiders = &theBeam->mRiders;
	List_Each(aRiders, [&](Rider* theARider) {
		if (Rider_Update(theARider) == 0)
			List_RemoveCurrent(aRiders);
	});
	// the tip follows the head, four pixels behind
	if (theBeam->mHit == 0)
		Obj_Update(&theBeam->mHarpoon);
	theBeam->mHarpoon.mX = (float)((double)theBeam->mX - (double)Sign(theBeam->mVx) * 4.0);
	theBeam->mHarpoon.mY = (float)((double)theBeam->mY - (double)Sign(theBeam->mVy) * 4.0);
	return aResult;
}

// The ship's last beam going gives the ship back its moves and its beams' speed (at least the start's for the
// difficulty)
void Beam_Destroy(Beam* theBeam)
{
	if (theBeam->mNext == nullptr && theBeam->mPrev == nullptr)
	{
		gLevels.mCurrent->mShooters.mCurrent->mFlags = 0;
		Shooter* aShooter = gLevels.mCurrent->mShooters.mCurrent;
		double aSpeed = (double)gBeamSpeed - (double)gDifficulty * 0.05;
		if (aSpeed <= (double)aShooter->mBeamSpeed)
			aSpeed = aShooter->mBeamSpeed;
		aShooter->mBeamSpeed = (float)aSpeed;
	}
	Obj_Destroy(&theBeam->mHarpoon);
	List_Clear(&theBeam->mRiders);
	Obj_Destroy(theBeam);
}

void List_DeleteNode(Beam* theBeam)
{
	Beam_Destroy(theBeam);
	::operator delete(theBeam);
}

Beam* Beam_Init(Beam* theBeam)
{
	Obj_Init(theBeam);
	List_Init(&theBeam->mRiders);
	Obj_Init(&theBeam->mHarpoon);
	return theBeam;
}

void Rider_Append(List<Rider>* theList)
{
	Rider* aRider = (Rider*)::operator new(sizeof(Rider));
	Obj_Init(aRider);
	List_Append(theList, aRider);
}

// The ship's beam an object touches: a box twice its image's size over the divisors overlaps the beam's, and the
// beam passes within the image's width over theWDiv of it (the walk stops on that beam)
Beam* Obj_TouchedBeam(Obj* theObj, int theWDiv, int theHDiv)
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	if (aShip != nullptr && aShip->mBeams.mCurrent != nullptr)
	{
		aShip->mBeams.mCurrent = aShip->mBeams.mHead;
		while (true)
		{
			Beam* aBeam = gLevels.mCurrent->mShooters.mCurrent->mBeams.mCurrent;
			if (BoxOverlap(theObj->mX, theObj->mY, (float)((theObj->mImage->mW * 2) / theWDiv),
					(float)((theObj->mImage->mH * 2) / theHDiv), (float)(((double)aBeam->mStartX + aBeam->mX) * 0.5),
					(float)(((double)aBeam->mStartY + aBeam->mY) * 0.5), std::fabs(aBeam->mStartX - aBeam->mX),
					std::fabs(aBeam->mStartY - aBeam->mY)) != 0 &&
				SegmentDist(theObj->mX, theObj->mY, aBeam->mStartX, aBeam->mStartY, aBeam->mX, aBeam->mY) <
					(float)(theObj->mImage->mW / theWDiv))
				return aBeam;
			aShip = gLevels.mCurrent->mShooters.mCurrent;
			Beam* aNode = aShip->mBeams.mCurrent;
			if (aNode == nullptr)
				return nullptr;
			if (aNode->mNext == nullptr)
				break;
			aShip->mBeams.mCurrent = Node_Next(aNode);
		}
		aShip->mBeams.mCurrent = aShip->mBeams.mHead;
	}
	return nullptr;
}

} // namespace wb
