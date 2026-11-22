#include "BoltObj.h"

#include "Anim.h"

using namespace Sexy;

// The marker for a step whose offset has not been worked out yet, and the largest place the fill looks at
const float BOLT_UNSET = 999.0f;
const float BOLT_UNSET_LIMIT = 998.0f;
const int BOLT_LAST_PLACE = 0x3e6;

// How the bolt is laid: a piece every 16 pixels, an offset's unit is 24 pixels, a piece's picture is 28 pixels long
const double BOLT_STEP = 16.0;
const double BOLT_OFFSET_UNIT = 24.0;
const double BOLT_PIECE_LENGTH = 28.0;

// 0x405fb0: the pieces are made, all hidden for now (a piece is an object of the level, drawn added to the picture)
BoltObj::BoltObj(int theRedrawFrames)
{
	ObjBase* aLevel = ObjBase::sLevel;
	for (int i = 0; i < BOLT_PIECES; i++) {
		ObjBase* aPiece = new ObjBase();
		mPieces[i] = aPiece;
		aPiece->SetStateInt(STATE_Z, 25);
		aPiece->SetParent(aLevel, nullptr);
		aPiece->mDrawMode = 1;
		aPiece->mAnim = Anim::Get("images/bolt", ANIM_SCALE_FAST, false, 0, 0, 1, false);
		aPiece->SetStateInt(STATE_SCALE_X, 1);
		aPiece->SetStateInt(STATE_SCALE_Y, 2);
		aPiece->SetStateInt(STATE_OFFSET_X, -12);
	}
	mHasEnds = false;
	mCountdown = 0;
	mRedrawFrames = theRedrawFrames;
	mShown = false;
	for (int i = 0; i < BOLT_PIECES + 2; i++)
		mOffsets[i] = 0.0f;
	mFromX = 0.0f;
	mFromY = 0.0f;
	mToX = 0.0f;
	mToY = 0.0f;
}

// 0x406280: the bolt goes out
void BoltObj::Stop()
{
	mShown = false;
	mHasEnds = false;
	for (int i = 0; i < BOLT_PIECES; i++)
		mPieces[i]->Deactivate(mPieces[i]);
}

// 0x4062b0
void BoltObj::SetEnds(int theFromX, int theFromY, int theToX, int theToY)
{
	mHasEnds = true;
	mFromX = (float)theFromX;
	mFromY = (float)theFromY;
	mToX = (float)theToX;
	mToY = (float)theToY;
}

// 0x406070 (EBX the bolt): the zigzag of theSteps places (made even). The two ends stay on the line; each place midway
// between two that are known is the mean of the two, moved by a random whole number of hundredths, up to the scale
// either way, the scale being 1 to start with and 15 percent smaller from one round of the list to the next, until every
// place has a value
void BoltObj::MakeShape(int theSteps)
{
	int aCount = theSteps;
	if (aCount % 2 == 1)
		aCount = aCount + 1;
	for (int i = 0; i < aCount; i++)
		mOffsets[i] = BOLT_UNSET;
	mOffsets[0] = 0.0f;
	// port: with no steps the original's write lands on the last piece's pointer, which nulls it (and Stop then crashes)
	if (aCount >= 1)
		mOffsets[aCount - 1] = 0.0f;
	float aScale = 1.0f;
	for (;;) {
		int aFrom = 0;
		bool aFinished = false;
		while (aFrom < aCount - 1 && !aFinished) {
			if (!(mOffsets[aFrom] < BOLT_UNSET)) {
				aFrom = aFrom + 1;
				continue;
			}
			int aTo = aFrom + 1;
			if (aTo >= aCount)
				break;
			while (!(mOffsets[aTo] < BOLT_UNSET)) {
				aTo = aTo + 1;
				if (aTo >= aCount) {
					aFinished = true;
					break;
				}
			}
			if (aFinished || aTo > BOLT_LAST_PLACE)
				break;
			float aMean;
			if (mOffsets[aFrom] < mOffsets[aTo])
				aMean = (float)(((double)mOffsets[aTo] - (double)mOffsets[aFrom]) * 0.5 + (double)mOffsets[aFrom]);
			else
				aMean = (float)(((double)mOffsets[aFrom] - (double)mOffsets[aTo]) * 0.5 + (double)mOffsets[aTo]);
			aScale = (float)((double)aScale * 100.0);
			int aLow = (int)(-(double)aScale);
			unsigned int aRandom = GameRandomNext();
			int aHigh = (int)aScale;
			int aChange = (int)(aRandom % (unsigned int)(aHigh - aLow + 1)) + aLow;
			mOffsets[(aTo - aFrom) / 2 + aFrom] = (float)((double)aChange / 100.0 + (double)aMean);
			aScale = (float)((double)aScale / 100.0);
			aFrom = aTo;
		}
		aScale = (float)((double)aScale / 1.15);
		bool aRepeat = false;
		for (int i = 0; i < aCount; i++) {
			if (mOffsets[i] > BOLT_UNSET_LIMIT) {
				aRepeat = true;
				break;
			}
		}
		if (!aRepeat)
			break;
	}
	mOffsets[0] = 0.0f;
	if (aCount >= 1)
		mOffsets[aCount - 1] = 0.0f;
	mOffsets[aCount] = 0.0f;
	mOffsets[aCount + 1] = 0.0f;
}

// 0x4062e0: every mRedrawFrames frames the bolt takes a new shape: the line's longer way is cut into 16 pixel pieces
// (the original does not stop at 160), each placed on its step, turned to the next step and as long as the way to it
void BoltObj::UpdateBolt()
{
	if (!mHasEnds)
		return;
	if (mCountdown > 0) {
		mCountdown = mCountdown - 1;
		return;
	}
	mShown = true;
	mCountdown = mRedrawFrames;
	double aDX = (double)mToX - (double)mFromX;
	double aDY = (double)mToY - (double)mFromY;
	float aAbsDX = FloatAbs((float)aDX);
	float aAbsDY = FloatAbs((float)aDY);
	if (aAbsDX == 0.0f && aAbsDY == 0.0f)
		return;
	int aPlaced = 0;
	if (aAbsDX < aAbsDY) {
		// Runs down: the pieces step along y and are pushed sideways in x
		MakeShape((int)((double)FloatAbs((float)(aDY + 1.0)) * 0.0625));
		float aY = mFromY;
		while (mToY > aY) {
			ObjBase* aPiece = mPieces[aPlaced];
			float aNextY = (float)((double)aY + BOLT_STEP);
			float aLeft = (float)((double)mOffsets[aPlaced] * BOLT_OFFSET_UNIT + (double)mFromX);
			float aRight = (float)(BOLT_OFFSET_UNIT * (double)mOffsets[aPlaced + 1] + (double)mFromX);
			double aAcross = (double)aLeft - (double)aRight;
			float anAngle = FloatAtan2((float)((double)aNextY - (double)aY), (float)aAcross);
			aPiece->SetState(STATE_ROTATION, anAngle);
			double anAlong = (double)aY - (double)aNextY;
			float aLength = FloatSqrt((float)(aAcross * aAcross + anAlong * anAlong));
			aPiece->SetState(STATE_SCALE_X, (float)((double)aLength / BOLT_PIECE_LENGTH));
			aPiece->SetState(STATE_X, (float)((double)mOffsets[aPlaced] * BOLT_OFFSET_UNIT + (double)mFromX));
			aPiece->SetState(STATE_Y, aY);
			aPiece->Activate(aPiece, false);
			aPiece->ComputeDrawn();
			aY = aNextY;
			aPlaced = aPlaced + 1;
		}
	} else {
		// Runs across: the pieces step along x and are pushed sideways in y
		MakeShape((int)((double)FloatAbs((float)(aDX + 1.0)) * 0.0625));
		float aX = mFromX;
		while (mToX > aX) {
			ObjBase* aPiece = mPieces[aPlaced];
			float aNextX = (float)((double)aX + BOLT_STEP);
			float aLeft = (float)((double)mOffsets[aPlaced] * BOLT_OFFSET_UNIT + (double)mFromY);
			float aRight = (float)(BOLT_OFFSET_UNIT * (double)mOffsets[aPlaced + 1] + (double)mFromY);
			double anAlong = (double)aX - (double)aNextX;
			float anAngle = FloatAtan2((float)((double)aRight - (double)aLeft), (float)anAlong);
			aPiece->SetState(STATE_ROTATION, anAngle);
			aPiece->SetState(STATE_X, aX);
			double aSide = (double)aLeft - (double)aRight;
			float aLength = FloatSqrt((float)(anAlong * anAlong + aSide * aSide));
			aPiece->SetState(STATE_SCALE_X, (float)((double)aLength / BOLT_PIECE_LENGTH));
			aPiece->SetState(STATE_Y, (float)((double)mOffsets[aPlaced] * BOLT_OFFSET_UNIT + (double)mFromY));
			aPiece->Activate(aPiece, false);
			aPiece->ComputeDrawn();
			aX = aNextX;
			aPlaced = aPlaced + 1;
		}
	}
	for (int i = aPlaced; i < BOLT_PIECES; i++)
		mPieces[i]->Deactivate(mPieces[i]);
}
