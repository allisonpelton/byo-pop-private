#include "Launcher.h"
#include "AngleTable.h"
#include "Dynomite.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "PortCompat.h"
#include "Sprite.h"
#include <cmath>

using namespace Sexy;

// 0x401050
Launcher::Launcher(FossilizedApp* theApp, Dynomite* theBoard)
{
	mApp = theApp;
	mBoard = theBoard;
	mAngle = 0;
	mLoaded = NULL;
	mFlying = NULL;
	mField1c = 0;
	mRecoilX = 0;
	mRecoilY = 0;
	mRand.SRand(GetTickCount());
	mField18 = 0;
	mFieldA5c = 0x32;
	int x = mBoard->mField158 + mBoard->mField70;
	mAimX = x;
	mFieldA60 = 0;
	mFieldA64 = 0;
	mFieldA68 = 0;
	mFieldA70 = 0;
	mFrozen = false;
	mHeld = NULL;
	mFieldAac = 0;
	mSlingX = x;
	mSlingY = 0x1b0;
	mAimY = 0x17e;
	mFieldA6c = 0xff;
}

// 0x401160
Launcher::~Launcher()
{
	if (mLoaded != NULL)
	{
		delete mLoaded;
		mLoaded = NULL;
	}
	if (mHeld != NULL)
	{
		delete mHeld;
		mHeld = NULL;
	}
}

// 0x4011f0
void Launcher::SetLoaded(LoadedEgg* theEgg)
{
	if (mLoaded == NULL)
		mLoaded = theEgg;
}

// 0x401210
void Launcher::Load(int theColor)
{
	if (mLoaded == NULL)
	{
		mLoaded = new LoadedEgg();
		mLoaded->Init(mApp, mBoard, theColor);
		mLoaded->mX = 100.0f;
		mLoaded->mY = 100.0f;
	}
}

// 0x4012a0: the loaded egg flies along the launcher's angle (a pixel a step, ten steps a frame) and the launcher
// recoils 20 pixels the other way
void Launcher::Fire()
{
	if ((mLoaded != NULL) && (mFrozen != true))
	{
		mBoard->mField160 = mLoaded->mColor;
		mBoard->mField15c = mBoard->mField160;
		mApp->mSndRubbershot->Play();
		AngleTable* anAngles = mApp->mAngles;
		float aDX = anAngles->mDX[mAngle];
		float aDY = anAngles->mDY[mAngle];
		mRecoilX = -(aDX * 20.0f);
		mRecoilY = -(aDY * 20.0f);
		FlyingEgg* anEgg = new FlyingEgg();
		anEgg->mX = mLoaded->mX;
		anEgg->mY = mLoaded->mY;
		anEgg->mColor = mLoaded->mColor;
		anEgg->mApp = mApp;
		anEgg->mBoard = mBoard;
		anEgg->mVY = aDY;
		anEgg->mVX = aDX;
		anEgg->mKind = mLoaded->mKind;
		mFlying = anEgg;
		mBoard->mFlyingEggs.Add(anEgg);
		if (mLoaded != NULL)
			delete mLoaded;
		mLoaded = NULL;
		// TODO: the board's +0x84 (0x437aa0) hears of it (0x437b20)
	}
}

// 0x4013f0
// port: the x87 code keeps the step unrounded, so the sums work in double
void Launcher::Extend(PathLine* theLine, float theDist)
{
	float* aTo = theLine->mTo;
	float* aFrom = theLine->mFrom;
	int anAngle = mApp->mAngles->AngleTo((int) aFrom[0], (int) aFrom[1], (int) aTo[0], (int) aTo[1]);
	AngleTable* anAngles = mApp->mAngles;
	double aStepX = (double) theDist * (double) anAngles->mDX[anAngle];
	double aStepY = (double) theDist * (double) anAngles->mDY[anAngle];
	theLine->mTo[0] = (float) (aStepX + (double) theLine->mTo[0]);
	theLine->mTo[1] = (float) (aStepY + (double) theLine->mTo[1]);
}

// 0x401470
void Launcher::Retract(PathLine* theLine, float theDist)
{
	float* aTo = theLine->mTo;
	float* aFrom = theLine->mFrom;
	int anAngle = mApp->mAngles->AngleTo((int) aFrom[0], (int) aFrom[1], (int) aTo[0], (int) aTo[1]);
	AngleTable* anAngles = mApp->mAngles;
	double aStepX = (double) theDist * (double) anAngles->mDX[anAngle];
	double aStepY = (double) theDist * (double) anAngles->mDY[anAngle];
	theLine->mFrom[0] = (float) ((double) theLine->mFrom[0] - aStepX);
	theLine->mFrom[1] = (float) ((double) theLine->mFrom[1] - aStepY);
}

// 0x4014f0
void Launcher::Freeze(LoadedEgg* theEgg)
{
	mFrozen = true;
	mHeld = theEgg;
	mFieldAac = 0;
}

// 0x401940: the slingshot and its loaded egg, drawn lower by the ceiling's depth plus half the slingshot's height
// once the ceiling comes past it
void Launcher::DrawSling(Graphics* g)
{
	Point anOffset;
	anOffset.mX = 0;
	anOffset.mY = 0;
	Pic* aPic = mApp->mPicSlingshot;
	float aLimit = (float) (aPic->mHeight / 2) + mBoard->mGrid->mCeiling;
	if ((float) mSlingY < aLimit)
		anOffset.mY = (int) aLimit;
	AddDrawOffset(anOffset);
	aPic = mApp->mPicSlingshot;
	aPic->Draw(mSlingX - (aPic->mWidth >> 1), mSlingY - (aPic->mHeight >> 1));
	if (mLoaded != NULL)
		mLoaded->Draw();
	anOffset.mY = -anOffset.mY;
	AddDrawOffset(anOffset);
}

// 0x401a10: the rubber band from the slingshot's two posts, round the loaded egg (and back to the pull) or the
// flying one until it is above 388, and the hand; moved down like the slingshot
void Launcher::DrawBand()
{
	Point anOffset;
	anOffset.mX = 0;
	anOffset.mY = 0;
	float aLimit = (float) (mApp->mPicSlingshot->mHeight / 2) + mBoard->mGrid->mCeiling;
	if ((float) mSlingY < aLimit)
		anOffset.mY = (int) aLimit;
	AddDrawOffset(anOffset);
	int aMiddle = mBoard->mField158 + mBoard->mField70;
	if ((mLoaded == NULL) && (mFlying == NULL))
		mApp->mPicRubberband->DrawLine(aMiddle - 0x36, 0x18c, aMiddle + 0x36, 0x18c);
	Cell* aHand;
	if (mLoaded != NULL)
	{
		mApp->mPicRubberband->DrawLine(aMiddle - 0x36, 0x18c, (int) (mLoaded->mX - 16.0f), (int) mLoaded->mY);
		mApp->mPicRubberband->DrawLine(aMiddle + 0x36, 0x18c, (int) (mLoaded->mX + 16.0f), (int) mLoaded->mY);
		mApp->mPicRubberband->DrawLine((int) (mLoaded->mX + 16.0f), (int) mLoaded->mY,
			(int) ((float) mField38[0] + mLoaded->mX), (int) ((float) mField38[1] + mLoaded->mY));
		mApp->mPicRubberband->DrawLine((int) (mLoaded->mX - 16.0f), (int) mLoaded->mY,
			(int) ((float) mField38[0] + mLoaded->mX), (int) ((float) mField38[1] + mLoaded->mY));
		mField20[0] = (int) ((float) (mField38[0] * 2) + mLoaded->mX);
		mField20[1] = (int) ((mLoaded->mY - 10.0f) + (float) mField38[1] + 50.0f);
		aHand = mApp->mHandCells[8 - mField18 / 8];
		aHand->Draw(mField20[0] - (aHand->mWidth >> 1), mField20[1] - (aHand->mHeight >> 1));
	}
	else
	{
		aHand = mApp->mHandCells[8 - mField18 / 8];
		aHand->Draw(mField20[0] - (aHand->mWidth >> 1), (mField20[1] - (aHand->mHeight >> 1)) + 0x1e);
	}
	if (mFlying != NULL)
	{
		mApp->mPicRubberband->DrawLine(aMiddle - 0x36, 0x18c, (int) (mFlying->mX - 16.0f), (int) mFlying->mY);
		mApp->mPicRubberband->DrawLine(aMiddle + 0x36, 0x18c, (int) (mFlying->mX + 16.0f), (int) mFlying->mY);
		int y = (int) mFlying->mY;
		mApp->mPicRubberband->DrawLine((int) (mFlying->mX + 16.0f), y, (int) (mFlying->mX - 16.0f), y);
		if (mFlying->mY < 388.0f)
			mFlying = NULL;
	}
	anOffset.mY = -anOffset.mY;
	AddDrawOffset(anOffset);
}

// 0x401d30: the egg pulled back 25 pixels against the aim, and the band's points
// port: the original takes the egg's +0x18 as a float[2]; the x87 code works in 53 bits and keeps the offsets
// unrounded, so this works in double
void Launcher::PlaceEgg(float& theX, float& theY)
{
	AngleTable* anAngles = mApp->mAngles;
	double aX = (double) anAngles->mDY[mAngle] * 0.0 - (double) anAngles->mDX[mAngle] * 25.0;
	float aY = (float) ((double) anAngles->mDY[mAngle] * 25.0 + (double) anAngles->mDX[mAngle] * 0.0);
	theX = (float) ((double) mSlingX + aX);
	double aBack = (double) mSlingY - (double) aY;
	theY = (float) aBack;
	theY = (float) ((double) mFieldAac + aBack);
	mField30[0] = (int) ((double) mSlingX - aX * 0.4);
	mField30[1] = (int) ((double) aY * 0.4 + (double) mSlingY);
	int anAngle = mAngle;
	if (anAngle >= 0x168)
		anAngle = anAngle - 0x168;
	if (anAngle < 0)
		anAngle = anAngle + 0x168;
	mField38[0] = (int) ((double) anAngles->mDY[anAngle] * 0.0 - (double) anAngles->mDX[anAngle] * 10.0);
	mField38[1] = mFieldAac - (int) ((double) anAngles->mDX[anAngle] * 0.0 + (double) anAngles->mDY[anAngle] * 10.0);
}

// 0x401e40: aims at a point no lower than 10 above the slingshot's middle; the hand turns with the aim
void Launcher::Aim(int theX, int theY)
{
	int aTop = mSlingY - 10;
	mAimX = theX;
	mAimY = theY;
	if (aTop < theY)
		theY = aTop;
	int anAngle = mApp->mAngles->AngleTo(mSlingX, mSlingY, theX, theY);
	mAngle = anAngle;
	anAngle = anAngle & ~1;
	mField18 = anAngle;
	if (anAngle > 0x5a)
		anAngle = (anAngle - 0x11e) >> 1;
	else
		anAngle = (anAngle >> 1) + 0x25;
	mField18 = anAngle;
	if (anAngle < 0)
		mField18 = 0;
	if (mField18 > 0x47)
		mField18 = 0x47;
	if (mLoaded != NULL)
		PlaceEgg(mLoaded->mX, mLoaded->mY);
}

// 0x401ed0: the aiming arrow while the aim is at least 10 above the slingshot's middle: a shaft 24 wide from 20
// behind the middle to 20 short of the aim, a head 36 wide and its tip 10 past the aim
// port: the x87 code keeps some sums unrounded (in 53 bits) where the floats are not stored; those work in double
void Launcher::DrawArrow(Graphics* g)
{
	if (mAimY <= mSlingY - 10)
	{
		AngleTable* anAngles = mApp->mAngles;
		int anAngle = mAngle;
		float aBackX = anAngles->mDX[anAngle] * 20.0f;
		float aBackY = anAngles->mDY[anAngle] * 20.0f;
		float aTipX = aBackX * 0.5f + (float) mAimX;
		float aTipY = aBackY * 0.5f + (float) mAimY;
		double aTailX = (double) mSlingX - (double) aBackX;
		float aTailRightY = (float) mSlingY - aBackY;
		float aTailLeftX = (float) mSlingX - aBackX;
		float aTailLeftY = (float) mSlingY - aBackY;
		int aSide = anAngle + 0x5a;
		if (aSide >= 0x168)
			aSide = aSide - 0x168;
		if (aSide < 0)
			aSide = aSide + 0x168;
		double aSideX = (double) anAngles->mDX[aSide] * 12.0;
		double aSideY = (double) anAngles->mDY[aSide] * 12.0;
		float aShaftRightX = (float) (((double) mAimX + aSideX) - (double) aBackX);
		float aShaftRightY = (float) (((double) mAimY + aSideY) - (double) aBackY);
		float aTailRightX = (float) (aSideX + aTailX);
		aTailRightY = (float) ((double) aTailRightY + aSideY);
		float aHeadRightX = (float) (aSideX * 0.5 + (double) aShaftRightX);
		float aHeadRightY = (float) (aSideY * 0.5 + (double) aShaftRightY);
		aSide = anAngle - 0x5a;
		if (aSide >= 0x168)
			aSide = aSide - 0x168;
		if (aSide < 0)
			aSide = aSide + 0x168;
		aSideX = (double) anAngles->mDX[aSide] * 12.0;
		aSideY = (double) anAngles->mDY[aSide] * 12.0;
		float aShaftLeftX = (float) (((double) mAimX + aSideX) - (double) aBackX);
		float aShaftLeftY = (float) (((double) mAimY + aSideY) - (double) aBackY);
		aTailLeftX = (float) (aSideX + (double) aTailLeftX);
		aTailLeftY = (float) ((double) aTailLeftY + aSideY);
		float aHeadLeftX = (float) (aSideX * 0.5 + (double) aShaftLeftX);
		float aHeadLeftY = (float) (aSideY * 0.5 + (double) aShaftLeftY);
		g->SetColor(Color(mFieldA6c, mFieldA70, mFieldA70, 0x78));
		Point aPoints[7];
		aPoints[0].mX = (int) aTailRightX;
		aPoints[0].mY = (int) aTailRightY;
		aPoints[1].mX = (int) aShaftRightX;
		aPoints[1].mY = (int) aShaftRightY;
		aPoints[2].mX = (int) aHeadRightX;
		aPoints[2].mY = (int) aHeadRightY;
		aPoints[3].mX = (int) aTipX;
		aPoints[3].mY = (int) aTipY;
		aPoints[4].mX = (int) aHeadLeftX;
		aPoints[4].mY = (int) aHeadLeftY;
		aPoints[5].mX = (int) aShaftLeftX;
		aPoints[5].mY = (int) aShaftLeftY;
		aPoints[6].mX = (int) aTailLeftX;
		aPoints[6].mY = (int) aTailLeftY;
		g->PolyFill(aPoints, 7);
	}
}

// 0x4021e0
void Launcher::Widen(PathLine* theLine, float* theFrom, float* theTo, float theWidth)
{
	theFrom[0] = theLine->mFrom[0];
	theFrom[1] = theLine->mFrom[1];
	theFrom[2] = theLine->mFrom[0];
	theFrom[3] = theLine->mFrom[1];
	theTo[0] = theLine->mTo[0];
	theTo[1] = theLine->mTo[1];
	theTo[2] = theLine->mTo[0];
	theTo[3] = theLine->mTo[1];
	float* aTo = theLine->mTo;
	float* aFrom = theLine->mFrom;
	int anAngle = mApp->mAngles->AngleTo((int) aFrom[0], (int) aFrom[1], (int) aTo[0], (int) aTo[1]) + 0x5a;
	if (anAngle >= 0x168)
		anAngle = anAngle - 0x168;
	if (anAngle < 0)
		anAngle = anAngle + 0x168;
	AngleTable* anAngles = mApp->mAngles;
	double aSideX = (double) theWidth * (double) anAngles->mDX[anAngle];
	double aSideY = (double) theWidth * (double) anAngles->mDY[anAngle];
	theFrom[0] = (float) (aSideX + (double) theFrom[0]);
	theFrom[1] = (float) (aSideY + (double) theFrom[1]);
	theFrom[2] = (float) ((double) theFrom[2] - aSideX);
	theFrom[3] = (float) ((double) theFrom[3] - aSideY);
	theTo[0] = (float) (aSideX + (double) theTo[0]);
	theTo[1] = (float) (aSideY + (double) theTo[1]);
	theTo[2] = (float) ((double) theTo[2] - aSideX);
	theTo[3] = (float) ((double) theTo[3] - aSideY);
}

// 0x4022f0: a line of the aim's path as a band 30 wide, its sides mitred with the lines before and after, the
// right side added to the right list and the left side put in front of the left list; the first line starts 10
// further back, and the last ends 25 short with a head 40 wide and its tip 30 further
void Launcher::AddPath(Graphics* g, PathLine* thePrev, PathLine* theLine, PathLine* theNext)
{
	float aFrom[4];
	float aTo[4];
	float aNextFrom[4];
	float aNextTo[4];
	float aPrevFrom[4];
	float aPrevTo[4];
	float aLine1[4];
	float aLine2[4];
	float aCross1[2];
	float aCross2[2];
	float aCross3[2];
	float aCross4[2];
	if (thePrev == NULL)
		Retract(theLine, 10.0f);
	if (theNext == NULL)
		Extend(theLine, -25.0f);
	if (thePrev != NULL)
	{
		if (theNext != NULL)
		{
			Widen(theLine, aFrom, aTo, 15.0f);
			Widen(theNext, aNextFrom, aNextTo, 15.0f);
			Widen(thePrev, aPrevFrom, aPrevTo, 15.0f);
			aLine1[0] = aFrom[0];
			aLine1[1] = aFrom[1];
			aLine1[2] = aTo[0];
			aLine1[3] = aTo[1];
			aLine2[0] = aNextFrom[0];
			aLine2[1] = aNextFrom[1];
			aLine2[2] = aNextTo[0];
			aLine2[3] = aNextTo[1];
			AngleTable::Intersect(aLine1, aLine2, aCross1);
			aLine1[0] = aFrom[0];
			aLine1[1] = aFrom[1];
			aLine1[2] = aTo[0];
			aLine1[3] = aTo[1];
			aLine2[0] = aPrevFrom[0];
			aLine2[1] = aPrevFrom[1];
			aLine2[2] = aPrevTo[0];
			aLine2[3] = aPrevTo[1];
			AngleTable::Intersect(aLine1, aLine2, aCross2);
			aLine1[0] = aFrom[2];
			aLine1[1] = aFrom[3];
			aLine1[2] = aTo[2];
			aLine1[3] = aTo[3];
			aLine2[0] = aNextFrom[2];
			aLine2[1] = aNextFrom[3];
			aLine2[2] = aNextTo[2];
			aLine2[3] = aNextTo[3];
			AngleTable::Intersect(aLine1, aLine2, aCross3);
			aLine1[0] = aFrom[2];
			aLine1[1] = aFrom[3];
			aLine1[2] = aTo[2];
			aLine1[3] = aTo[3];
			aLine2[0] = aPrevFrom[2];
			aLine2[1] = aPrevFrom[3];
			aLine2[2] = aPrevTo[2];
			aLine2[3] = aPrevTo[3];
			AngleTable::Intersect(aLine1, aLine2, aCross4);
			mPathRight.Add(new Point((int) aCross2[0], (int) aCross2[1]));
			mPathRight.Add(new Point((int) aCross1[0], (int) aCross1[1]));
			mPathLeft.InsertAt(new Point((int) aCross4[0], (int) aCross4[1]), 0);
			mPathLeft.InsertAt(new Point((int) aCross3[0], (int) aCross3[1]), 0);
		}
		else
		{
			Widen(theLine, aFrom, aTo, 15.0f);
			Widen(thePrev, aPrevFrom, aPrevTo, 15.0f);
			aLine1[0] = aFrom[0];
			aLine1[1] = aFrom[1];
			aLine1[2] = aTo[0];
			aLine1[3] = aTo[1];
			aLine2[0] = aPrevFrom[0];
			aLine2[1] = aPrevFrom[1];
			aLine2[2] = aPrevTo[0];
			aLine2[3] = aPrevTo[1];
			AngleTable::Intersect(aLine1, aLine2, aCross1);
			aLine1[0] = aFrom[2];
			aLine1[1] = aFrom[3];
			aLine1[2] = aTo[2];
			aLine1[3] = aTo[3];
			aLine2[0] = aPrevFrom[2];
			aLine2[1] = aPrevFrom[3];
			aLine2[2] = aPrevTo[2];
			aLine2[3] = aPrevTo[3];
			AngleTable::Intersect(aLine1, aLine2, aCross2);
			mPathRight.Add(new Point((int) aCross1[0], (int) aCross1[1]));
			mPathRight.Add(new Point((int) aTo[0], (int) aTo[1]));
			mPathLeft.InsertAt(new Point((int) aCross2[0], (int) aCross2[1]), 0);
			mPathLeft.InsertAt(new Point((int) aTo[2], (int) aTo[3]), 0);
		}
	}
	else
	{
		if (theNext != NULL)
		{
			Widen(theLine, aFrom, aTo, 15.0f);
			Widen(theNext, aNextFrom, aNextTo, 15.0f);
			aLine1[0] = aFrom[0];
			aLine1[1] = aFrom[1];
			aLine1[2] = aTo[0];
			aLine1[3] = aTo[1];
			aLine2[0] = aNextFrom[0];
			aLine2[1] = aNextFrom[1];
			aLine2[2] = aNextTo[0];
			aLine2[3] = aNextTo[1];
			AngleTable::Intersect(aLine1, aLine2, aCross1);
			aLine1[0] = aFrom[2];
			aLine1[1] = aFrom[3];
			aLine1[2] = aTo[2];
			aLine1[3] = aTo[3];
			aLine2[0] = aNextFrom[2];
			aLine2[1] = aNextFrom[3];
			aLine2[2] = aNextTo[2];
			aLine2[3] = aNextTo[3];
			AngleTable::Intersect(aLine1, aLine2, aCross2);
			mPathRight.Add(new Point((int) aFrom[0], (int) aFrom[1]));
			mPathRight.Add(new Point((int) aCross1[0], (int) aCross1[1]));
			mPathLeft.InsertAt(new Point((int) aFrom[2], (int) aFrom[3]), 0);
			mPathLeft.InsertAt(new Point((int) aCross2[0], (int) aCross2[1]), 0);
		}
		else
		{
			Widen(theLine, aFrom, aTo, 15.0f);
			mPathRight.Add(new Point((int) aFrom[0], (int) aFrom[1]));
			mPathRight.Add(new Point((int) aTo[0], (int) aTo[1]));
			mPathLeft.InsertAt(new Point((int) aFrom[2], (int) aFrom[3]), 0);
			mPathLeft.InsertAt(new Point((int) aTo[2], (int) aTo[3]), 0);
		}
	}
	if (theNext == NULL)
	{
		float aHead[4];
		Widen(theLine, aFrom, aTo, 15.0f);
		Widen(theLine, aFrom, aHead, 20.0f);
		Extend(theLine, 30.0f);
		mPathRight.Add(new Point((int) aHead[0], (int) aHead[1]));
		mPathRight.Add(new Point((int) theLine->mTo[0], (int) theLine->mTo[1]));
		mPathRight.Add(new Point((int) aHead[2], (int) aHead[3]));
	}
}

// 0x402c10: a frozen launcher holding an egg loads it after 56 frames, else thaws as its count runs down; the
// arrow turns white while the board's bonus bit 1 is on and red otherwise; the slingshot follows the board, and the
// recoil dies away
void Launcher::Update()
{
	if (mFrozen == true)
	{
		if (mHeld != NULL)
		{
			mFieldAac++;
			if (mFieldAac > 0x37)
			{
				if (mLoaded != NULL)
					delete mLoaded;
				mLoaded = mHeld;
				mHeld = NULL;
			}
		}
		else
		{
			mFieldAac--;
			if (mFieldAac < 1)
				mFrozen = false;
		}
	}
	Dynomite* aBoard = mBoard;
	mFieldA64 = 0;
	if ((aBoard->mBonusBits & 1) != 0)
	{
		mFieldA6c--;
		if (mFieldA6c < 0)
			mFieldA6c = 0;
		mFieldA70++;
		if (mFieldA70 > 0xff)
			mFieldA70 = 0xff;
		mFieldA64 = -600;
	}
	else
	{
		mFieldA70--;
		if (mFieldA70 < 0)
			mFieldA70 = 0;
		mFieldA6c++;
		if (mFieldA6c > 0xff)
			mFieldA6c = 0xff;
		if (mFieldA60 < mFieldA68)
			mFieldA60 = mFieldA68;
	}
	if (mFieldA60 > mFieldA64)
		mFieldA60--;
	if (mFieldA60 < mFieldA64)
		mFieldA60++;
	mSlingX = aBoard->mField158 + aBoard->mField70;
	mFieldA5c--;
	mSlingY = 0x1b0;
	if (mFieldA5c < 0)
		mFieldA5c = 0x18;
	if (mLoaded != NULL)
		PlaceEgg(mLoaded->mX, mLoaded->mY);
	mRecoilX = mRecoilX * 0.85f;
	mRecoilY = mRecoilY * 0.85f;
}

// port: the original computes this in line, in 53 bits
static double LineLength(const PathLine& theLine)
{
	double aDY = (double) theLine.mTo[1] - (double) theLine.mFrom[1];
	double aDX = (double) theLine.mTo[0] - (double) theLine.mFrom[0];
	return sqrt(aDY * aDY + aDX * aDX);
}

// 0x402d90: the aim's path, found by flying a ghost egg from the slingshot: its points are the start, two points
// back along the path at each bounce, and 25 back from where it stops; drawn as a band when it stops more than
// 50 above the slingshot's middle (lines under 50 long left out), then the aim marker on the cell it lands in
// port: the x87 code keeps the products unrounded, so the sums work in double
void Launcher::DrawPath(Graphics* g)
{
	AngleTable* anAngles = mApp->mAngles;
	float aLastVX = anAngles->mDX[mAngle];
	float aVY = anAngles->mDY[mAngle];
	PtrList<float> aPoints;
	aPoints.Clear();
	GridPoint aMarker;
	// port: the original leaves the marker's place unset when the ghost lands on its first step
	aMarker.mX = 0;
	aMarker.mY = 0;
	FlyingEgg* anEgg = new FlyingEgg();
	PlaceEgg(anEgg->mX, anEgg->mY);
	anEgg->mGhost = true;
	anEgg->mColor = 1000;
	anEgg->mApp = mApp;
	anEgg->mBoard = mBoard;
	anEgg->mVY = aVY;
	anEgg->mVX = aLastVX;
	anEgg->mKind = 0;
	if (anEgg->Update())
	{
		float* aPoint = new float[2];
		aPoint[0] = (float) mSlingX;
		aPoint[1] = (float) mSlingY;
		aPoints.Add(aPoint);
		aLastVX = anEgg->mVX;
		anEgg->mCol = -1;
		while (anEgg->Update() && ((float) (mFieldA60 + mAimY - 0x10) <= anEgg->mY))
		{
			if (anEgg->mVX != aLastVX)
			{
				aLastVX = anEgg->mVX;
				aPoint = new float[2];
				aPoint[0] = (float) ((double) anEgg->mVX * 25.0 + (double) anEgg->mX);
				aPoint[1] = (float) ((double) anEgg->mY - (double) anEgg->mVY * 25.0);
				aPoints.Add(aPoint);
				aPoint = new float[2];
				aPoint[0] = anEgg->mVX + anEgg->mX;
				aPoint[1] = anEgg->mY - anEgg->mVY;
				aPoints.Add(aPoint);
			}
		}
		GridPoint aCell;
		mBoard->mGrid->ScreenToGrid((int) anEgg->mX, (int) anEgg->mY, &aCell);
		GridPoint aPixel;
		EggGrid::PixelToCell(aCell.mX, aCell.mY, &aPixel);
		EggGrid::CellToPixel(aPixel.mX, aPixel.mY, &aCell);
		mBoard->mGrid->GridToScreen(aCell.mX, aCell.mY, &aMarker);
		aMarker.mX = aMarker.mX + 0x10;
		aPoint = new float[2];
		aPoint[0] = (float) ((double) anEgg->mX - (double) anEgg->mVX * 25.0);
		aPoint[1] = (float) ((double) anEgg->mY - (double) anEgg->mVY * 25.0);
		aPoints.Add(aPoint);
		mFieldA68 = (int) (anEgg->mY - (float) mAimY);
		if ((float) (mSlingY - 0x32) > anEgg->mY)
		{
			int aLast = aPoints.mCount - 1;
			mPathRight.Clear();
			mPathLeft.Clear();
			if (0 < aLast)
			{
				int i = -1;
				do
				{
					PathLine aPrev;
					aPrev.mFrom = aPoints.Get(i - 1);
					aPrev.mTo = aPoints.Get(i);
					if ((aPrev.mFrom != NULL) &&
						(LineLength(aPrev) < 50.0))
						aPrev.mFrom = NULL;
					PathLine aLine;
					aLine.mFrom = aPoints.Get(i + 1);
					aLine.mTo = aPoints.Get(i + 2);
					if ((aLine.mFrom != NULL) &&
						(LineLength(aLine) < 50.0))
						aLine.mFrom = NULL;
					PathLine aNext;
					aNext.mFrom = aPoints.Get(i + 3);
					aNext.mTo = aPoints.Get(i + 4);
					if ((aNext.mFrom != NULL) &&
						(LineLength(aNext) < 50.0))
						aNext.mFrom = NULL;
					if (aPrev.mFrom == NULL)
					{
						if (aLine.mFrom != NULL)
						{
							if (aNext.mFrom == NULL)
								AddPath(g, NULL, &aLine, NULL);
							else
								AddPath(g, NULL, &aLine, &aNext);
						}
					}
					else if (aLine.mFrom != NULL)
					{
						if (aNext.mFrom == NULL)
							AddPath(g, &aPrev, &aLine, NULL);
						else
							AddPath(g, &aPrev, &aLine, &aNext);
					}
					i = i + 2;
				} while (i + 1 < aLast);
			}
			Point* aPolygon = new Point[mPathLeft.mCount + mPathRight.mCount];
			Point* aDest = aPolygon;
			mPathRight.ResetIter();
			Point* aCorner = mPathRight.Next();
			while (aCorner != NULL)
			{
				*aDest = *aCorner;
				aDest++;
				delete aCorner;
				aCorner = mPathRight.Next();
			}
			mPathLeft.ResetIter();
			aCorner = mPathLeft.Next();
			while (aCorner != NULL)
			{
				*aDest = *aCorner;
				aDest++;
				delete aCorner;
				aCorner = mPathLeft.Next();
			}
			g->SetColor(Color(mFieldA6c, mFieldA70, mFieldA70, 0x78));
			g->PolyFill(aPolygon, mPathLeft.mCount + mPathRight.mCount);
			delete[] aPolygon;
			mPathRight.Clear();
			mPathLeft.Clear();
		}
	}
	if (anEgg->mCol != -1)
	{
		Pic* aPic = mApp->mPicAimx;
		aPic->Draw(aMarker.mX - (aPic->mWidth >> 1), aMarker.mY - (aPic->mHeight >> 1));
	}
	delete anEgg;
	aPoints.ResetIter();
	float* aPoint = aPoints.Next();
	while (aPoint != NULL)
	{
		delete[] aPoint;
		aPoint = aPoints.Next();
	}
}

// 0x4033a0: the aim, while the mouse is over the board and nothing else holds it: the path (clipped to the board)
// while +0xa60 is away from 0 and the aim is above 421, else the arrow
void Launcher::Draw(Graphics* g)
{
	Dynomite* aBoard = mBoard;
	if ((aBoard->mField130 != true) && (aBoard->mField131 != true) && (aBoard->mIsOver == true) &&
		(aBoard->mField244 == false) && ((mApp->mInTutorial != true) || (aBoard->mPauseCount < 1)))
	{
		if ((mAimY <= 0x1a4) && (mFieldA60 != 0))
		{
			Graphics aClipG(*g);
			aBoard = mBoard;
			aClipG.ClipRect(aBoard->mField70 - aBoard->mField6c, 0, aBoard->mField68, 0x1e0);
			DrawPath(&aClipG);
			return;
		}
		DrawArrow(g);
	}
}
