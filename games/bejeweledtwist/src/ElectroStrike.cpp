// The electro gems' lightning (ElectroStrike.h).
#include "ElectroStrike.h"

#include "Board.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Piece.h"
#include "Res.h"
#include "TriVertex.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

namespace {

// 0x85d850: the bolts' colours by the gem's colour. Nothing in the exe writes this table, so every bolt is
// Color(0, 0, 0, 0): its glow (additive black) shows nothing and its lightning is the mid grey of half white.
const Color kBoltColors[10] = {};

}  // namespace

ElectroBolt::ElectroBolt(int theX1, int theY1, int theX2, int theY2, const Color& theColor, float theDuration,
	bool theStar)
{
	mProgress = 0;
	mDuration = theDuration;
	mColor = theColor;
	mTime = 0;
	mStar = theStar;
	mX1 = (float)theX1;
	mY1 = (float)theY1;
	mX2 = (float)theX2;
	mDone = false;
	mY2 = (float)theY2;
	float aDY = mY2 - mY1;
	float aDX = mX2 - mX1;
	mAngle = (float)atan2((double)aDY, (double)aDX);  // 0x4c1b70
	mLength = sqrtf(aDY * aDY + aDX * aDX);
	Piece* aStart = gBoard->GetPieceAtXY(theX1, theY1);
	mCel = 0;
	mCount = 0;
	mStart = aStart;
	Update();
}

void ElectroBolt::Update()
{
	mTime = (float)((double)mTime + 0.10000000149011612);
	mCount++;
	float aDX = mX2 - mX1;
	float aDY = mY2 - mY1;
	float aSq = (float)((double)aDX * (double)aDX + (double)aDY * (double)aDY);
	float aLen = sqrtf(aSq) < 1.0 ? 1.0f : sqrtf(aSq);
	// (the original picks between two equal widths by mStar)
	float aWidth = (float)((double)gApp->mScale * 80.0);
	float aNX = (float)((double)aWidth * (double)aDY / (double)aLen);
	float aNY = (float)((double)aWidth * (double)aDX / (double)aLen);
	if (mCount % 5 == 0) {
		// Every fifth update the line is made anew, jagged by Rand except at its two ends
		mLeft.clear();
		mRight.clear();
		int aCels = IMAGE_LIGHTNING->mNumRows * IMAGE_LIGHTNING->mNumCols;
		mCel = (int)Rand() % aCels;
		double aSegs = (double)mLength * 0.5 / (double)IMAGE_LIGHTNING->GetCelHeight();
		if (!(1.0 <= aSegs))
			aSegs = 1.0;
		int aCount = Ftol(aSegs + 1.0);
		int i = 0;
		if (0 < aCount) {
			float aLast = (float)(aCount - 1);
			do {
				float aFrac = (float)((double)i / (double)aLast);
				int aJitter = 1;
				if (i != 0 && i < aCount - 1) {
					float aWide = (float)((double)gApp->mScale * 160.0);
					int aGrown = Ftol((double)aWide * (double)mTime / (double)mDuration);
					if (aGrown < Ftol((double)gApp->mScale * 80.0)) {
						aJitter = Ftol(80.0 * (double)gApp->mScale);
					} else {
						aWide = (float)(160.0 * (double)gApp->mScale);
						aJitter = Ftol((double)aWide * (double)mTime / (double)mDuration);
					}
				}
				float aHalf = (float)(aJitter / 2);
				double aX = (double)aFrac * (double)aDX + (double)mX1 + (double)aHalf;
				int aRoll = (int)Rand() % aJitter;
				float aPX = (float)(aX - (double)aRoll);
				double aY = (double)aFrac * (double)aDY + (double)mY1 + (double)aHalf;
				aRoll = (int)Rand() % aJitter;
				float aPY = (float)(aY - (double)aRoll);
				Point aLeft = {aPX - aNX, aPY + aNY};
				Point aRight = {aPX + aNX, aPY - aNY};
				mLeft.push_back(aLeft);
				mRight.push_back(aRight);
				i++;
			} while (i < aCount);
		}
	}
	float aProgress = mTime / mDuration;
	mProgress = aProgress;
	if (1.0f <= aProgress)
		mDone = true;
}

void ElectroBolt::Draw(Graphics* g)
{
	float aCel = (float)((double)mCel * 0.20000000298023224);
	float aU1 = (float)(0.20000000298023224 + (double)aCel);
	float aU0 = (float)((double)aCel + 0.019999999552965164);
	aU1 = (float)((double)aU1 - 0.019999999552965164);
	double aFade = (1.0 - (double)mProgress) * 8.0;
	double aClamped = aFade < 1.0 ? aFade : 1.0;
	float anAlpha;
	if (aClamped < 0.0)
		anAlpha = 0;
	else if (1.0 <= aFade)
		anAlpha = 1.0f;
	else
		anAlpha = (float)aFade;
	// With Direct3D the strips are drawn by the 3D interface with the z buffer switched off: NotPorted (Direct3D)
	int aLast = (int)mLeft.size() - 1;
	int i = 0;
	if (0 < aLast) {
		int anAlpha255 = Ftol((double)anAlpha * 800.0);
		int aShown = 255 < anAlpha255 ? 255 : anAlpha255;
		int j;
		do {
			j = i + 1;
			float aDX = (float)((((double)mLeft[i].mX - (double)mLeft[j].mX) + (double)mRight[i].mX) -
				(double)mRight[j].mX);
			float aDY = (float)((((double)mLeft[i].mY - (double)mLeft[j].mY) + (double)mRight[i].mY) -
				(double)mRight[j].mY);
			float aSq = (float)((double)aDX * (double)aDX + (double)aDY * (double)aDY);
			float aLen = sqrtf(aSq) < 1.0 ? 1.0f : sqrtf(aSq);
			// (the strip's widening is multiplied by 0, so the quad runs edge to edge)
			float anOX = (float)((double)aDY * 0.0 / (double)aLen);
			float anOY = (float)(0.0 * (double)aDX / (double)aLen);
			float aX0 = mLeft[i].mX - anOX;
			float aY0 = mLeft[i].mY - anOY;
			float aX1 = mRight[i].mX + anOX;
			float aY1 = mRight[i].mY + anOY;
			float aX2 = mRight[j].mX + anOX;
			float aY2 = mRight[j].mY + anOY;
			float aX3 = mLeft[j].mX - anOX;
			float aY3 = mLeft[j].mY - anOY;
			// The glow in the bolt's colour
			TriVertex aGlow[2][3] = {
				{TriVertex(aX0, aY0, 0, 0), TriVertex(aX1, aY1, 1, 0), TriVertex(aX2, aY2, 1, 1)},
				{TriVertex(aX2, aY2, 1, 1), TriVertex(aX3, aY3, 0, 1), TriVertex(aX0, aY0, 0, 0)},
			};
			g->SetColor(Color(mColor.mRed, mColor.mGreen, mColor.mBlue, aShown));
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawTrianglesTex(IMAGE_GRITTYBLURRY, aGlow, 2);
			g->DrawTrianglesTex(IMAGE_GRITTYBLURRY, aGlow, 2);
			// The lightning's cel, half way to white
			TriVertex aBolt[2][3] = {
				{TriVertex(aX0, aY0, aU0, 0), TriVertex(aX1, aY1, aU1, 0), TriVertex(aX2, aY2, aU1, 1)},
				{TriVertex(aX2, aY2, aU1, 1), TriVertex(aX3, aY3, aU0, 1), TriVertex(aX0, aY0, aU0, 0)},
			};
			Color aWhite(255, 255, 255);
			g->SetColor(Color((mColor.mRed + aWhite.mRed) / 2, (mColor.mGreen + aWhite.mGreen) / 2,
				(mColor.mBlue + aWhite.mBlue) / 2, aShown));
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawTrianglesTex(IMAGE_LIGHTNING, aBolt, 2);
			g->DrawTrianglesTex(IMAGE_LIGHTNING, aBolt, 2);
			i = j;
		} while (j < aLast);
	}
}

ElectroStrike::ElectroStrike(Piece* thePiece)
{
	mColor = thePiece->mColor;
	mPointsId = thePiece->mUnknown2EC;
	mIds.push_back(thePiece->mId);
	if (thePiece->IsFlagSet(Piece::FLAG_POWER)) {
		mNovaScale.SetCurve(GTS(0x7e6750));
		mNovaAlpha.SetCurve(GTS(0x7e672c), &mNovaScale);
		mNukeScale.SetCurve(GTS(0x7e66f8));
		mNukeAlpha.SetCurve(GTS(0x7e66d0), &mNukeScale);
		mKind = 1;
	} else {
		mKind = 0;
	}
	mPieceId = thePiece->mId;
	mX = gBoard->GetColX(thePiece->mCol) + gGemWidth / 2;
	mY = gBoard->GetRowY(thePiece->mUnknown30) + gGemHeight / 2;
	thePiece->mGlow = 0.9f;
	thePiece->mUnknown38 = true;
	thePiece->mUnknown51D = true;
	mTimer = 0;
	mStep = 0;
	mFade = 1.0f;
	// A bolt along the gem's row and one down its column; a power electro gem's also along the rows and columns on
	// either side
	int i = (mKind != 1) - 1;
	if (i <= (int)(mKind == 1)) {
		do {
			int aY = Ftol((double)(gGemHeight / 2) + (double)thePiece->mY + (double)(gGemHeight * i));
			int aRow = gBoard->GetRowAt(aY);
			if (-1 < aRow && aRow < gBoard->mRows) {
				ElectroBolt* aBolt = new ElectroBolt(gBoard->GetColX(0), aY, gBoard->GetColX(gBoard->mCols), aY,
					kBoltColors[mColor], 10.0f, mKind == 1);
				mBolts.push_back(aBolt);
			}
			int aX = Ftol((double)(gGemWidth / 2) + (double)thePiece->mX + (double)(gGemWidth * i));
			int aCol = gBoard->GetColAt(aX);
			if (-1 < aCol && aCol < gBoard->mCols) {
				ElectroBolt* aBolt = new ElectroBolt(aX, gBoard->GetRowY(0), aX, gBoard->GetRowY(gBoard->mRows),
					kBoltColors[mColor], 10.0f, mKind == 1);
				mBolts.push_back(aBolt);
			}
			i++;
		} while (i <= (int)(mKind == 1));
	}
}

ElectroStrike::~ElectroStrike()
{
	for (size_t i = 0; i < mBolts.size(); i++) {
		if (mBolts[i] != nullptr)
			delete mBolts[i];
	}
}

void ElectroStrike::Update()
{
	mCount++;
	mNovaScale.IncInVal();
	mNukeScale.IncInVal();
	if (mKind == 1 && mNukeScale.CheckInThreshold(1.6))
		gApp->PlaySoundEx(SOUND_BOMB_EXPLODE, 0, -2.0f, 0.0f, true, true);
	for (int i = 0; i < (int)mBolts.size(); i++)
		mBolts[i]->Update();
	float aLeft = (float)((double)mUnknown210 - 0.009999999776482582);
	if (aLeft < 0.0f)
		aLeft = 0;
	mUnknown210 = aLeft;
}

void ElectroStrike::Draw(Graphics* g)
{
	// A power electro gem's nova and blast, by their curves
	if (mNovaAlpha.GetOutVal() != 0.0) {
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mNovaAlpha.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		float aScale = (float)mNovaScale.GetOutVal();
		DrawImageScaledCentered(g, IMAGE_BOOM_NOVA, (float)mX, (float)mY, aScale, aScale);
	}
	if (mNukeAlpha.GetOutVal() != 0.0) {
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mNukeAlpha.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		float aScale = (float)mNukeScale.GetOutVal();
		DrawImageScaledCentered(g, IMAGE_BOOM_NUKE, (float)mX, (float)mY, aScale, aScale);
	}
	for (unsigned int i = 0; i < mBolts.size(); i++)
		mBolts[i]->Draw(g);
}
