// VizRainbowGlyphs, SubAtomic, Cubes and RainDrops (Visualizers3D.h). As in Visualizers3D.cpp the original's float code
// runs on the x87 stack (rounded to float only where it stores, and to 53 bits, the CRT's default precision), so
// expressions are written in double where the original's are; a constant printed short in the export is a double, and a
// long one (0.800000011920929) is a float.
#include "Visualizers3D.h"

#include "Compat2004.h"
#include "DDImage.h"
#include "Graphics.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TriVertex.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdint>
#include <cstring>

using namespace Sexy;

// 0x67c64b: theValue held between theLow and theHigh
static float ClampF(float theValue, float theLow, float theHigh)
{
	if (theLow <= theValue) {
		if (theValue <= theHigh)
			return theValue;
		return theHigh;
	}
	return theLow;
}

// the colours' blend that the visualizers do by hand: theFrom to theTo by theFraction, a channel at a time, truncated
// part of VizRainbowGlyphs::Draw (0x68dd18): one channel of the colours' blend
static int BlendChannel(int theFrom, int theTo, float theFraction)
{
	return Ftol((double)(theTo - theFrom) * (double)theFraction + (double)theFrom);
}

// ---------------------------------------------------------------------------------------------------------------------
// VizRainbowGlyphs

// 0x68daa6: four of the game's Rand
VizRainbowGlyphs::Glyph::Glyph()
{
	mFlipCount = 0;
	mX = RandSigned();
	mY = RandSigned();
	mThirdDraw = RandSigned();
	mFourthDraw = RandSigned();
	mLife = 1.0f;
}

VizRainbowGlyphs::VizRainbowGlyphs(WinTwistApp* theApp) : Visualizer(theApp)
{
	mBackground = LoadVizImage(IMAGE_AZTEC_BG_ID);
	mStrip = LoadVizImage(IMAGE_AZTEC_STRIP_ID);
	mDrawWidth = theApp->mWidth;
	mBrightness = 0.2f;  // 0x3e4ccccd
	mDrawHeight = theApp->mHeight;
}

VizRainbowGlyphs::~VizRainbowGlyphs()
{
	DeleteVizImage(IMAGE_AZTEC_BG_ID);
	DeleteVizImage(IMAGE_AZTEC_STRIP_ID);
	// port: the original frees none of the glyphs
	for (int i = 0; i < 16; i++)
		for (Glyph* aGlyph : mColumns[i])
			delete aGlyph;
}

void VizRainbowGlyphs::Init()
{
	for (int i = 0; i < 16; i++) {
		mPrevX[i] = 0.0f;
		mPrevY[i] = 0.0f;
	}
	mCount = 0;
	mCel = 0;
	int aCount = 0x3c;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

void VizRainbowGlyphs::Update()
{
	mCount++;
	if (mCount % 2 != 0)
		return;
	int aPhaseX = 0;
	int aPhaseY = 0;
	int aPoint = 0;
	do {
		std::vector<Glyph*>& aGlyphs = mColumns[aPoint];
		// where the point is: round the screen on two sines of different speeds (its height scales x, its width y)
		float aSinArg = (float)((double)mCount * 0.0010000000474974513 + (double)aPhaseX);
		float aSin = sinf(aSinArg);
		float aX = (float)((double)mApp->mHeight * 0.7099999785423279 * (double)aSin);
		float aCosArg = (float)((double)mCount * 0.001500000013038516 + (double)aPhaseY);
		float aCos = cosf(aCosArg);
		float aY = (float)((double)mApp->mWidth * 0.7099999785423279 * (double)aCos);
		if (mCount == 0) {
			mPrevX[aPoint] = (float)((double)RandSigned() + (double)aX);  // 0x68eb0b
			mPrevY[aPoint] = (float)((double)RandSigned() + (double)aY);
		}
		if (mCount % 4 == 0) {
			Glyph* aGlyph = new Glyph();  // 0x68dd0c: four Rand
			float aSize = fabsf(RandSigned());  // 0x68eb5c
			double aRadius = (double)aSize * 70.0;
			aGlyph->mX = (float)((double)RandSigned() * aRadius + (double)aX);
			aGlyph->mY = (float)((double)RandSigned() * aRadius + (double)aY);
			aGlyph->mLife = (float)(1.100000023841858 - (double)aSize * (double)aSize);
			aGlyph->mCel = mCel;
			aGlyph->mAlpha = 0;
			mCel = (mCel + 1) % 0x17;
			aGlyph->mVX = (float)((double)RandSigned() * 0.4000000059604645 + 0.4000000059604645 * ((double)aX - (double)mPrevX[aPoint]));
			aGlyph->mVY = (float)((double)RandSigned() * 0.4000000059604645 + 0.4000000059604645 * ((double)aY - (double)mPrevY[aPoint]));
			aGlyphs.push_back(aGlyph);  // 0x68e9fa
		}
		int anIndex = 0;
		if (0 < (int)aGlyphs.size()) {
			do {
				Glyph* aGlyph = aGlyphs[anIndex];
				aGlyph->mVX = (float)(((double)aX - (double)mPrevX[aPoint]) * 0.0020000000949949026 + (double)aGlyph->mVX);
				aGlyph->mAlpha += 0x19;
				aGlyph->mVY = (float)(((double)aY - (double)mPrevY[aPoint]) * 0.0020000000949949026 + (double)aGlyph->mVY);
				aGlyph->mVX = (float)((double)aGlyph->mVX * 0.9900000095367432);
				aGlyph->mVY = (float)(0.9900000095367432 * (double)aGlyph->mVY);
				aGlyph->mX = (float)((double)aGlyph->mX + (double)aGlyph->mVX * 0.44999998807907104);
				aGlyph->mY = (float)(0.44999998807907104 * (double)aGlyph->mVY + (double)aGlyph->mY);
				aGlyph->mLife = (float)((double)aGlyph->mLife - 0.004999999888241291);
				if (0x78 < aGlyph->mAlpha)
					aGlyph->mAlpha = 0x78;
				if (aGlyph->mLife <= 0.0f) {
					// the glyph is replaced by the last one (0x68e5b0), which is taken off the end
					Glyph* aOld = aGlyphs[anIndex];
					aGlyphs[anIndex] = aGlyphs.back();
					delete aOld;
					if (!aGlyphs.empty())
						aGlyphs.pop_back();
					anIndex--;
				}
				anIndex++;
			} while (anIndex < (int)aGlyphs.size());
		}
		aPhaseX += 1000;
		aPhaseY += 0x5dc;
		mPrevX[aPoint] = aX;
		mPrevY[aPoint] = aY;
		aPoint++;
	} while (aPhaseX < 16000);
}

void VizRainbowGlyphs::Draw(Graphics* g)
{
	// the rainbow's seven colours (made once)
	static Color aColors[7];
	static bool aMade = false;
	if (!aMade) {
		aMade = true;
		aColors[0] = Color(0, 0xff, 0xff);
		aColors[1] = Color(0, 0, 0xff);
		aColors[2] = Color(0xff, 0, 0xff);
		aColors[3] = Color(0xff, 0, 0);
		aColors[4] = Color(0xff, 0x80, 0);
		aColors[5] = Color(0xff, 0xff, 0);
		aColors[6] = Color(0, 0xff, 0);
	}
	int aStep = mCount / 8;
	g->PushState();  // 0x41fd70
	g->SetColorizeImages(true);
	double aBase = (double)aStep * 0.004999999888241291;
	int aStrip = 0;
	do {
		// the strip's colour, a seventh of the way round the rainbow for each 0.15 of a strip, dimmed to a third
		float aT = (float)((double)(float)aStrip * 0.15000000596046448 + aBase);
		float aFraction = (float)((double)aT - (double)(float)std::floor((double)aT));  // 0x401b40
		int anIndex = Ftol((double)aT);
		const Color& aFrom = aColors[anIndex % 7];
		const Color& aTo = aColors[(anIndex + 1) % 7];
		int aBlue = Ftol((double)BlendChannel(aFrom.mBlue, aTo.mBlue, aFraction) * 0.33000001311302185);
		int aGreen = Ftol((double)BlendChannel(aFrom.mGreen, aTo.mGreen, aFraction) * 0.33000001311302185);
		int aRed = Ftol((double)BlendChannel(aFrom.mRed, aTo.mRed, aFraction) * 0.33000001311302185);
		g->SetColor(Color(aRed, aGreen, aBlue));
		int aDrawWidth = mDrawWidth;
		float aDestStrip = (float)(aDrawWidth / 32);
		float aSrcStrip = (float)(mBackground->mWidth / 32);
		double aDestWidth;
		if (aStrip == 0x1f)
			aDestWidth = (double)aDrawWidth - (double)(float)aStrip * (double)aDestStrip;
		else
			aDestWidth = (double)aDestStrip;
		Rect aSrc(Ftol((double)(float)aStrip * (double)aSrcStrip), 0, Ftol((double)aSrcStrip), mBackground->mHeight);
		Rect aDest(Ftol((double)(float)aStrip * (double)aDestStrip), 0, Ftol(aDestWidth), mDrawHeight);
		g->DrawImage(mBackground, aDest, aSrc);  // 0x421780
		aStrip++;
	} while (aStrip < 0x20);
	g->SetDrawMode(1);  // 0x41ff20
	int aPoint = 0;
	do {
		const std::vector<Glyph*>& aGlyphs = mColumns[aPoint];
		int anIndex = 0;
		if (0 < (int)aGlyphs.size()) {
			double aPointShift = (double)aPoint * 0.30000001192092896;
			do {
				Glyph* aGlyph = aGlyphs[anIndex];
				float aT = (float)((double)aGlyph->mLife * 4.0 + aBase + aPointShift);
				float aFraction = (float)((double)aT - (double)(float)std::floor((double)aT));
				int aColorIndex = Ftol((double)aT);
				const Color& aFrom = aColors[aColorIndex % 7];
				const Color& aTo = aColors[(aColorIndex + 1) % 7];
				int anAlpha = aGlyph->mAlpha;
				int aBlue = BlendChannel(aFrom.mBlue, aTo.mBlue, aFraction);
				int aGreen = BlendChannel(aFrom.mGreen, aTo.mGreen, aFraction);
				int aRed = BlendChannel(aFrom.mRed, aTo.mRed, aFraction);
				g->SetColor(Color(aRed, aGreen, aBlue, anAlpha));
				MatrixTransform aTransform;  // 0x44f9d0
				// the glyph is stretched along its way
				float aStepX = aGlyph->mVX;
				float aStepY = aGlyph->mVY;
				float aLength = (float)std::sqrt((double)(float)((double)aStepX * (double)aStepX + (double)aStepY * (double)aStepY));  // 0x618620
				float aDirX;
				float aDirY;
				if (aLength == 0.0f) {
					aDirX = aStepX;
					aDirY = aStepY;
				} else {
					aDirX = (float)((double)aStepX / (double)aLength);
					aDirY = (float)((double)aStepY / (double)aLength);
				}
				float aScale = (float)((double)aGlyph->mLife * 0.8500000238418579);
				float aStretch = (float)((double)aLength * 1.25);
				float aA = (float)((double)aDirX * (double)aScale);
				float aB = (float)((double)aDirY * (double)aScale);
				float aLongX = (float)((double)aDirX * (double)aStretch);
				float aLongY = (float)((double)aDirY * (double)aStretch);
				float aC = (float)((double)aScale * (double)aLongX);
				float aD = (float)((double)aScale * (double)aLongY);
				SexyMatrix3 aMatrix;  // 0x44f860: the identity
				aMatrix.LoadIdentity();
				aMatrix.m00 = aB;
				aMatrix.m01 = aC;
				aMatrix.m10 = -aA;
				aMatrix.m11 = aD;
				aTransform.SetMatrix(aMatrix);  // 0x44fc20
				Rect aCel = mStrip->GetCelRect(aGlyph->mCel);  // 0x423780
				// round the screen's edges: drawn again a screen's width or height away where it reaches over
				if (-20.0f < aGlyph->mX) {
					if (-20.0f < aGlyph->mY)
						g->DrawImageTransform(mStrip, aTransform, aCel, aGlyph->mX, aGlyph->mY);  // 0x4220b0
					if (aGlyph->mY < 20.0f)
						g->DrawImageTransform(mStrip, aTransform, aCel, aGlyph->mX, (float)((double)mApp->mHeight + (double)aGlyph->mY));
				}
				if (aGlyph->mX < 20.0f) {
					if (-20.0f < aGlyph->mY)
						g->DrawImageTransform(mStrip, aTransform, aCel, (float)((double)mApp->mWidth + (double)aGlyph->mX), aGlyph->mY);
					if (aGlyph->mY < 20.0f)
						g->DrawImageTransform(mStrip, aTransform, aCel, (float)((double)mApp->mWidth + (double)aGlyph->mX),
							(float)((double)mApp->mHeight + (double)aGlyph->mY));
				}
				anIndex++;
			} while (anIndex < (int)aGlyphs.size());
		}
		aPoint++;
	} while (aPoint < 0x10);
	g->PopState();  // 0x41fe00
}

// ---------------------------------------------------------------------------------------------------------------------
// SubAtomic

// 0x67c67d and 0x67c75c: the u and v of the swimming mesh at (a, b) in 0 to 1 (UVWarp's, with other constants)
static float AtomU(int theCount, float a, float b)
{
	a = (float)((double)a - 0.5);
	b = (float)((double)b - 0.5);
	float aT = (float)((double)theCount * 0.0010000000474974513);
	float aWave = cosf((float)((double)a * 100.0 * (double)b * (double)aT));
	float aCos = cosf(aT);
	float aSin = sinf(aT);
	float aBreath = sinf((float)((double)aT * 3.0));
	double aSum = (double)aSin * (double)b + ((double)aWave * 0.05000000074505806 + (double)aCos * (double)a);
	return ClampF((float)(aSum * 0.05000000074505806 * ((double)aBreath * 0.5 + 1.0) + ((double)a * 0.8999999761581421 + 0.5)), 0.0f, 1.0f);
}

// part of SubAtomic::DrawMesh (0x67cc9d): a vertex's swimming v coordinate
static float AtomV(int theCount, float a, float b)
{
	a = (float)((double)a - 0.5);
	b = (float)((double)b - 0.5);
	float aT = (float)((double)theCount * 0.0010000000474974513);
	float aWave = sinf((float)((double)a * 100.0 * (double)b * (double)aT));
	float aCos = cosf(aT);
	float aSin = sinf(aT);
	float aBreath = cosf((float)((double)aT * 4.0));
	double aSum = ((double)aWave * 0.05000000074505806 + (double)aCos * (double)b) - (double)aSin * (double)a;
	return ClampF((float)(aSum * 0.05000000074505806 * ((double)aBreath * 0.5 + 1.0) + ((double)b * 0.8999999761581421 + 0.5)), 0.0f, 1.0f);
}

// 0x67c581: five of the game's Rand and two of the C runtime's
SubAtomic::Electron::Electron()
{
	mAge = 0;
	float aSizeX = fabsf(RandSigned());
	mX = (float)((double)(float)gApp->mWidth * (double)aSizeX);
	float aSizeY = fabsf(RandSigned());
	mY = (float)((double)(float)gApp->mHeight * (double)aSizeY);
	mCenterX = (float)((double)RandSigned() + (double)mX);
	mCenterY = (float)((double)RandSigned() + (double)mY);
	mSpeed = 250.0f;
	mSplitScale = 1.0f;
	mTurn = 1.0f;
	mSign = (float)(0.5 + (double)fabsf(RandSigned()) * 0.5);
	mCountdown = CrtRand() % 100 + 0x96;
	mLife = (CrtRand() % 5 + 5) * 100;
}

SubAtomic::SubAtomic(WinTwistApp* theApp) : Visualizer(theApp)
{
	mSprite = LoadVizImage(IMAGE_SUBATOMIC_BLUR_ID);
	mBackground = LoadVizImage(IMAGE_SUBATOMIC_BG_ID);
	mBrightness = 0.2f;  // 0x3e4ccccd
}

SubAtomic::~SubAtomic()
{
	DeleteVizImage(IMAGE_SUBATOMIC_BLUR_ID);
	DeleteVizImage(IMAGE_SUBATOMIC_BG_ID);
	for (Electron* anElectron : mElectrons)
		delete anElectron;
	for (Trail* aTrail : mTrails)
		delete aTrail;
}

void SubAtomic::Init()
{
	int aCount = 3;
	do {
		Electron* anElectron = new Electron();
		mElectrons.push_back(anElectron);  // 0x67e020
		aCount--;
	} while (aCount != 0);
	aCount = 0x3c;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

void SubAtomic::Update()
{
	if ((int)mElectrons.size() < 0xf) {
		Electron* anElectron = new Electron();
		mElectrons.push_back(anElectron);
	}
	mCount++;
	int anIndex = 0;
	if (0 < (int)mElectrons.size()) {
		do {
			Electron* anElectron = mElectrons[anIndex];
			anElectron->mAge++;
			anElectron->mCountdown--;
			float aDX = (float)((double)anElectron->mX - (double)anElectron->mCenterX);
			float aDY = (float)((double)anElectron->mY - (double)anElectron->mCenterY);
			if (anElectron->mCountdown == 0 && (int)mElectrons.size() < 0x19) {
				// it splits: the new one starts where it is, going the other way round
				Electron* aNew = new Electron();
				mElectrons.push_back(aNew);
				aNew->mX = anElectron->mX;
				aNew->mY = anElectron->mY;
				aNew->mCenterX = anElectron->mCenterX;
				aNew->mCenterY = anElectron->mCenterY;
				aNew->mSpeed = anElectron->mSpeed;
				aNew->mSplitScale = anElectron->mSplitScale;
				aNew->mSign = (float)((double)anElectron->mSign * -1.0);
				int aRand = CrtRand();
				aDX = (float)((double)aDX * -1.0);
				aDY = (float)((double)aDY * -1.0);
				anElectron->mCenterX = (float)((double)anElectron->mCenterX - (double)aDX * 2.0);
				anElectron->mCenterY = (float)((double)anElectron->mCenterY - (double)aDY * 2.0);
				anElectron->mTurn = (float)((double)anElectron->mTurn * -1.0);
				anElectron->mSpeed = (float)((double)anElectron->mSpeed - 0.5);
				anElectron->mCountdown = aRand % 100 + 0x96;
			}
			float aLength = (float)std::sqrt((double)(float)((double)aDX * (double)aDX + (double)aDY * (double)aDY));  // 0x4c0650
			bool aRemove = false;
			if (aLength == 0.0f || anElectron->mLife < anElectron->mAge) {
				aRemove = true;
			} else {
				float aFarX = fabsf((float)((double)(mApp->mWidth / 2) - (double)anElectron->mX));  // 0x401ad0
				if ((double)mApp->mWidth * 0.699999988079071 < (double)aFarX) {
					aRemove = true;
				} else {
					float aFarY = fabsf((float)((double)(mApp->mHeight / 2) - (double)anElectron->mY));
					if ((double)mApp->mHeight * 0.699999988079071 < (double)aFarY ||
						(anElectron->mSpeed < 2.0f && anElectron->mSign < 0.0f))
						aRemove = true;
				}
			}
			if (aRemove) {
				// replaced by the last one, which is taken off the end
				Electron* aOld = mElectrons[anIndex];
				mElectrons[anIndex] = mElectrons.back();
				delete aOld;
				if (!mElectrons.empty())
					mElectrons.pop_back();
				anIndex--;
			} else {
				float anOldX = anElectron->mX;
				float anOldY = anElectron->mY;
				double aPull = (double)anElectron->mSign * 0.05999999865889549;
				float aSpeed = (float)((double)anElectron->mSpeed + aPull);
				anElectron->mSpeed = aSpeed;
				if (anElectron->mSign < 0.0f)
					anElectron->mSpeed = (float)(aPull + (double)aSpeed);
				aDX = (float)((double)anElectron->mSpeed * (double)aDX / (double)aLength);
				aDY = (float)((double)anElectron->mSpeed * (double)aDY / (double)aLength);
				float anAngle = (float)((double)anElectron->mTurn * 0.800000011920929 / ((double)anElectron->mSpeed + 1.0));
				float aCos = cosf(anAngle);  // 0x401b20
				float aSin = sinf(anAngle);  // 0x401b70
				anElectron->mX = (float)(((double)aCos * (double)aDX + (double)anElectron->mCenterX) + (double)aSin * (double)aDY);
				aCos = cosf(anAngle);
				aSin = sinf(anAngle);
				anElectron->mY = (float)(((double)aCos * (double)aDY + (double)anElectron->mCenterY) - (double)aSin * (double)aDX);
				if (mCount % 0x14 == 0) {
					Trail* aTrail = new Trail();  // 0x67cc89
					aTrail->mX = anElectron->mX;
					aTrail->mY = anElectron->mY;
					aTrail->mVX = (float)(((double)anElectron->mSpeed * 0.001) * ((double)anOldX - (double)anElectron->mX));
					aTrail->mVY = (float)(((double)anElectron->mSpeed * 0.001) * ((double)anOldY - (double)anElectron->mY));
					mTrails.push_back(aTrail);  // 0x67e075
				}
			}
			anIndex++;
		} while (anIndex < (int)mElectrons.size());
	}
	anIndex = 0;
	if (0 < (int)mTrails.size()) {
		do {
			Trail* aTrail = mTrails[anIndex];
			aTrail->mAge++;
			aTrail->mX = aTrail->mVX + aTrail->mX;
			aTrail->mY = aTrail->mVY + aTrail->mY;
			if (500 < aTrail->mAge) {
				Trail* aOld = mTrails[anIndex];
				mTrails[anIndex] = mTrails.back();
				delete aOld;
				if (!mTrails.empty())
					mTrails.pop_back();
				anIndex--;
			}
			anIndex++;
		} while (anIndex < (int)mTrails.size());
	}
}

// 0x67cc9d: the background on a 16 by 16 mesh whose texture coordinates swim, six vertices a cell, the cells in columns
void SubAtomic::DrawMesh(Graphics* g)
{
	static TriVertex aTriangles[512][3];
	int aCol = 0;
	do {
		int aNextCol = aCol + 1;
		float aRightA = (float)aNextCol * 0.0625f;
		float aLeftA = (float)aCol * 0.0625f;
		TriVertex* aCell = &aTriangles[aCol * 32][0];
		int aRow = 0;
		float aBottomY = 0.0f;
		do {
			float aCellW = (float)mApp->mWidth * 0.0625f;
			float aCellH = (float)mApp->mHeight * 0.0625f;
			float aLeftX = (float)aCol * aCellW;
			float aTopY = (float)aRow * aCellH;
			float aTopB = (float)aRow * 0.0625f;
			float aRightX = (float)aNextCol * aCellW;
			int aNextRow = aRow + 1;
			aBottomY = (float)aNextRow * aCellH;
			float aBottomB = (float)aNextRow * 0.0625f;
			aCell[0] = TriVertex(aLeftX, aTopY, AtomU(mCount, aLeftA, aTopB), AtomV(mCount, aLeftA, aTopB));
			aCell[1] = TriVertex(aRightX, aTopY, AtomU(mCount, aRightA, aTopB), AtomV(mCount, aRightA, aTopB));
			aCell[2] = TriVertex(aLeftX, aBottomY, AtomU(mCount, aLeftA, aBottomB), AtomV(mCount, aLeftA, aBottomB));
			aCell[3] = TriVertex(aRightX, aBottomY, AtomU(mCount, aRightA, aBottomB), AtomV(mCount, aRightA, aBottomB));
			aCell[4] = TriVertex(aRightX, aTopY, AtomU(mCount, aRightA, aTopB), AtomV(mCount, aRightA, aTopB));
			aCell[5] = TriVertex(aLeftX, aBottomY, AtomU(mCount, aLeftA, aBottomB), AtomV(mCount, aLeftA, aBottomB));
			if (aCol == 0xf) {
				// the last column ends at the screen's edge
				aCell[1].x = (float)mApp->mWidth;
				aCell[3].x = (float)mApp->mWidth;
				aCell[4].x = (float)mApp->mWidth;
			}
			aCell += 6;  // two triangles of three vertices
			aRow = aNextRow;
		} while (aRow < 0x10);
		aCol = aNextCol;
	} while (aCol < 0x10);
	g->DrawTrianglesTex(mBackground, aTriangles, 0x200);  // 0x4221c0
}

void SubAtomic::Draw(Graphics* g)
{
	// the eight colours the field and the trails pass through (made once)
	static Color aColors[8];
	static bool aMade = false;
	g->PushState();  // 0x41fd70
	g->SetColorizeImages(true);
	if (!aMade) {
		aMade = true;
		aColors[0] = Color(0x8b, 0x1c, 0x1c);
		aColors[1] = Color(0xb0, 0x5e, 0);
		aColors[2] = Color(0xde, 0x7d, 0x47);
		aColors[3] = Color(0xb0, 0xb0, 0x7c);
		aColors[4] = Color(0x9e, 199, 0x47);
		aColors[5] = Color(0x6e, 0xba, 0x47);
		aColors[6] = Color(0x6e, 0xba, 0xc9);
		aColors[7] = Color(0x80, 0x2a, 0x99);
	}
	// the background's colour: a colour of the eight by the update count, dimmed to 0.6
	float aT = (float)((double)mCount * 0.004999999888241291 + 1.0);
	float aFraction = (float)((double)aT - (double)(float)std::floor((double)aT));
	int anIndex = Ftol((double)aT);
	const Color& aFrom = aColors[anIndex % 8];
	const Color& aTo = aColors[(anIndex + 1) % 8];
	int aBlue = Ftol((double)BlendChannel(aFrom.mBlue, aTo.mBlue, aFraction) * 0.6000000238418579);
	int aGreen = Ftol((double)BlendChannel(aFrom.mGreen, aTo.mGreen, aFraction) * 0.6000000238418579);
	int aRed = Ftol((double)BlendChannel(aFrom.mRed, aTo.mRed, aFraction) * 0.6000000238418579);
	g->SetColor(Color(aRed, aGreen, aBlue, 0xff));
	if (!g->Is3D())
		DrawImage(g, mBackground);  // 0x67fdb6
	else
		DrawMesh(g);
	g->SetDrawMode(1);  // 0x41ff20
	// only the trails are drawn, not the electrons
	for (Trail* aTrail : mTrails) {
		int anAlpha = 0xfa;
		int anAge = aTrail->mAge;
		if (0 < anAge) {
			if (anAge < 0x7f)
				anAlpha = Ftol((double)(anAge + anAge));
			else
				anAlpha = Ftol((double)(500 - anAge) / 500.0 * 255.0);
		}
		float aTrailT = (float)(((double)mCount + (double)anAge) * 0.004999999888241291 + 0.0);
		float aTrailFraction = (float)((double)aTrailT - (double)(float)std::floor((double)aTrailT));
		int aTrailIndex = Ftol((double)aTrailT);
		const Color& aTrailFrom = aColors[aTrailIndex % 8];
		const Color& aTrailTo = aColors[(aTrailIndex + 1) % 8];
		int aTrailBlue = BlendChannel(aTrailFrom.mBlue, aTrailTo.mBlue, aTrailFraction);
		int aTrailGreen = BlendChannel(aTrailFrom.mGreen, aTrailTo.mGreen, aTrailFraction);
		int aTrailRed = BlendChannel(aTrailFrom.mRed, aTrailTo.mRed, aTrailFraction);
		g->SetColor(Color(aTrailRed, aTrailGreen, aTrailBlue, anAlpha));
		Transform aTransform;  // 0x44f9d0
		aTransform.Scale(0.6875f, 0.6875f);  // 0x44fae0
		float aX = (float)((double)(mApp->mWidth / 2) + (double)aTrail->mX + 50.0);
		if (aX < (float)((double)mApp->mWidth + 10.0))
			g->DrawImageTransformF(mSprite, aTransform, aX, (float)((double)aTrail->mY + 0.0));  // 0x422050
		aX = (float)((double)aX - (double)mApp->mWidth);
		if (-10.0f < aX)
			g->DrawImageTransformF(mSprite, aTransform, aX, (float)((double)aTrail->mY + 0.0));
	}
	g->PopState();  // 0x41fe00
}

// ---------------------------------------------------------------------------------------------------------------------
// Cubes

namespace {

struct CubeVec {  // 0x671034
	float x, y, z;
};

// 0x67104e, 0x6710be and 0x67112b: the vector turned about the x, y and z axis (the pair of its other two floats)
void TurnX(CubeVec& theVec, float theAngle)
{
	float aY = theVec.y;
	float aCos = cosf(theAngle);
	float aSin = sinf(theAngle);
	theVec.y = (float)((double)aCos * (double)theVec.y + (double)theVec.z * (double)aSin);
	aCos = cosf(theAngle);
	aSin = sinf(theAngle);
	theVec.z = (float)((double)theVec.z * (double)aCos - (double)aSin * (double)aY);
}

void TurnY(CubeVec& theVec, float theAngle)
{
	float aX = theVec.x;
	float aCos = cosf(theAngle);
	float aSin = sinf(theAngle);
	theVec.x = (float)((double)aSin * (double)theVec.z + (double)theVec.x * (double)aCos);
	aCos = cosf(theAngle);
	aSin = sinf(theAngle);
	theVec.z = (float)((double)aCos * (double)theVec.z - (double)aSin * (double)aX);
}

void TurnZ(CubeVec& theVec, float theAngle)
{
	float aX = theVec.x;
	float aCos = cosf(theAngle);
	float aSin = sinf(theAngle);
	theVec.x = (float)((double)aSin * (double)theVec.y + (double)theVec.x * (double)aCos);
	aCos = cosf(theAngle);
	aSin = sinf(theAngle);
	theVec.y = (float)((double)aCos * (double)theVec.y - (double)aSin * (double)aX);
}

}  // namespace

Cubes::Cubes(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mSecondGrowth = 0.0f;
	mFirstGrowth = 0.0f;
	mBrightness = 0.2f;  // 0x3e4ccccd
	mImage = LoadVizImage(IMAGE_BORDERED_SQUARE_ID);
}

Cubes::~Cubes()
{
	DeleteVizImage(IMAGE_BORDERED_SQUARE_ID);
}

void Cubes::Update()
{
	mCount++;
	mSecondGrowth = (float)((double)mSecondGrowth + 0.02500000037252903);
	mFirstGrowth = (float)((double)mFirstGrowth + 0.03500000014901161);
}

void Cubes::Draw(Graphics* g)
{
	g->PushState();  // 0x41fd70
	g->SetColor(Color(0));  // 0x4016b0
	g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);  // 0x420020
	float aCount = (float)mCount;
	float aSlow = (float)((double)aCount * 0.0010000000474974513);
	float aMedium = (float)((double)aCount * 0.0013000000035390258);
	float aFast = (float)((double)aCount * 0.0006000000284984708);
	int aCube = 0;
	do {
		// the cubes sit on a spiral: a ring of 11 by their number, a turn of the ring for each
		double aBase = (double)mApp->mWidth * 0.0009765625 * 56.0;
		int aRing = aCube % 11;
		float aRadius = (float)(aBase + (double)aRing * aBase);
		int aRingStep = aRing + 11;
		float aAngle = (float)((double)mCount * 4.999999873689376e-06 * (double)aRingStep +
			((double)aCube * (double)6.2831855f) / 360.0);
		float aSin = sinf(aAngle);
		float aCosSlow = (float)((double)mCount * 0.0010000000474974513);
		aCosSlow = cosf(aCosSlow);
		float aCosAngle = cosf(aAngle);
		float aSinAngle = sinf(aAngle);
		float aScale = (float)((double)mApp->mWidth * 0.0009765625);
		float aHue = (float)((double)aSin + 3.0 + (double)aRadius * 0.007000000216066837 * (double)aCosSlow);
		float aRotC = (float)((double)aAngle + (double)aFast);
		float aRotB = (float)((double)(aCube % 10 + 0x1e) * ((double)aAngle * 0.05000000074505806) + (double)aMedium);
		float aRotA = (float)((double)aAngle + (double)aSlow);
		float aSize = (float)(40.0 * ((double)mApp->mWidth * 0.0009765625));
		float aY = (float)((double)(mApp->mHeight / 2) + (double)aCosAngle * (double)aRadius);
		float aX = (float)((double)aScale * 565.0 + (double)aSinAngle * (double)aRadius);
		DrawCube(aX, aY, aSize, aRotA, aRotB, aRotC, aHue);  // 0x671198
		aCube++;
	} while (aCube < 0x168);
	g->PopState();  // 0x41fe00
}

void Cubes::DrawCube(float theX, float theY, float theSize, float theRotA, float theRotB, float theRotC, float theHue)
{
	// off the screen, or no size: nothing
	if (!(theSize != 0.0f && (double)theSize * -2.0 <= (double)theX &&
			(double)theX <= (double)mApp->mWidth + ((double)theSize + (double)theSize) &&
			(double)theSize * -2.0 <= (double)theY &&
			(double)theY <= (double)mApp->mHeight + ((double)theSize + (double)theSize)))
		return;
	// the cube's three edges from its middle, turned
	CubeVec aVa = {0.0f, 0.0f, theSize};
	CubeVec aVb = {0.0f, theSize, 0.0f};
	CubeVec aVc = {theSize, 0.0f, 0.0f};
	TurnX(aVa, theRotA);
	TurnX(aVb, theRotA);
	TurnX(aVc, theRotA);
	TurnY(aVa, theRotB);
	TurnY(aVb, theRotB);
	TurnY(aVc, theRotB);
	TurnZ(aVa, theRotC);
	TurnZ(aVb, theRotC);
	TurnZ(aVc, theRotC);
	// the eight corners, the middle plus or minus each edge (z 0, rhw 1, no colour of their own)
	double aABX = (double)aVb.x + (double)aVa.x;
	double aABY = (double)aVb.y + (double)aVa.y;
	double aACX = (double)aVa.x - (double)aVb.x;
	double aACY = (double)aVa.y - (double)aVb.y;
	double aBAX = (double)aVb.x - (double)aVa.x;
	double aBAY = (double)aVb.y - (double)aVa.y;
	float aAX = (float)((aABX + (double)aVc.x) + (double)theX);
	float aAY = (float)((aABY + (double)aVc.y) + (double)theY);
	float aFX = (float)((double)theX + (aABX - (double)aVc.x));
	float aFY = (float)((double)theY + (aABY - (double)aVc.y));
	float aHX = (float)(((double)theX + (double)aVc.x) + aACX);
	float aHY = (float)(((double)theY + (double)aVc.y) + aACY);
	float aDX = (float)((double)theX + (aACX - (double)aVc.x));
	float aDY = (float)((double)theY + (aACY - (double)aVc.y));
	float aEX = (float)(((double)theX + (double)aVc.x) + aBAX);
	float aEY = (float)(((double)theY + (double)aVc.y) + aBAY);
	float aCX = (float)((double)theX + (aBAX - (double)aVc.x));
	float aCY = (float)((double)theY + (aBAY - (double)aVc.y));
	float aGX = (float)((double)theX + ((double)aVc.x - aABX));
	float aGY = (float)((double)theY + ((double)aVc.y - aABY));
	float aBX = (float)((double)theX - (aABX + (double)aVc.x));
	float aBY = (float)((double)theY - (aABY + (double)aVc.y));
	// the colour: the eight of the rainbow, by the update count and the cube's hue
	static Color aColors[8];
	static bool aMade = false;
	if (!aMade) {
		aMade = true;
		aColors[0] = Color(0, 0xff, 0xff);
		aColors[1] = Color(0, 0, 0xff);
		aColors[2] = Color(0, 0, 0xff);
		aColors[3] = Color(0xff, 0, 0xff);
		aColors[4] = Color(0xff, 0, 0);
		aColors[5] = Color(0xff, 0x80, 0);
		aColors[6] = Color(0xff, 0xff, 0);
		aColors[7] = Color(0, 0xff, 0);
	}
	float aHue = (float)((double)mCount * 0.003000000026077032 + (double)theHue);
	float aFraction = (float)((double)aHue - (double)(float)std::floor((double)aHue));
	int anIndex = Ftol((double)aHue);
	const Color& aFrom = aColors[anIndex % 8];
	const Color& aTo = aColors[(anIndex + 1) % 8];
	int aRed = BlendChannel(aFrom.mRed, aTo.mRed, aFraction);
	int aGreen = BlendChannel(aFrom.mGreen, aTo.mGreen, aFraction);
	int aBlue = BlendChannel(aFrom.mBlue, aTo.mBlue, aFraction);
	GPU2D* aDevice = gD3DInterface->m2D;
	// a face's colour, by how far it turns from the viewer (the z of its axis); 0.2 more or less of the colour, alpha 80
	auto aFaceColor = [&](float theZ) {
		double aQ = (double)theZ * 0.6000000238418579 / (double)theSize;
		float aShade;
		if (theZ <= 0.0f)
			aShade = (float)(0.20000000298023224 - aQ);
		else
			aShade = (float)(aQ + 0.20000000298023224);
		float aRedF = (float)aRed;
		float aGreenF = (float)aGreen;
		float aBlueF = (float)aBlue;
		int aB = Ftol((double)aBlueF * (double)aShade);
		int aG = Ftol((double)aGreenF * (double)aShade);
		int aR = Ftol((double)aRedF * (double)aShade);
		return Color(aR, aG, aB, 0x50);
	};
	auto aTri = [&](const TriVertex& theP1, const TriVertex& theP2, const TriVertex& theP3, const Color& theColor) {
		aDevice->DrawTriangleTex(theP1, theP2, theP3, theColor, 1, mImage, true);  // slot 0x70
	};
	// the faces across Va (the first block), Vb and Vc; the texture coordinates of each corner differ from face to face
	{
		Color aColor = aFaceColor(aVa.z);
		TriVertex A(aAX, aAY, 0.0f, 0.0f), B(aBX, aBY, 1.0f, 1.0f), C(aCX, aCY, 1.0f, 0.0f), D(aDX, aDY, 1.0f, 1.0f);
		TriVertex E(aEX, aEY, 0.0f, 0.0f), F(aFX, aFY, 1.0f, 0.0f), G(aGX, aGY, 0.0f, 1.0f), H(aHX, aHY, 0.0f, 1.0f);
		if (aVa.z <= 0.0f) {
			aTri(E, C, G, aColor);
			aTri(B, C, G, aColor);
		} else {
			aTri(A, F, H, aColor);
			aTri(D, F, H, aColor);
		}
	}
	{
		Color aColor = aFaceColor(aVb.z);
		TriVertex A(aAX, aAY, 0.0f, 0.0f), B(aBX, aBY, 1.0f, 1.0f), C(aCX, aCY, 1.0f, 1.0f), D(aDX, aDY, 1.0f, 0.0f);
		TriVertex E(aEX, aEY, 0.0f, 1.0f), F(aFX, aFY, 1.0f, 0.0f), G(aGX, aGY, 0.0f, 1.0f), H(aHX, aHY, 0.0f, 0.0f);
		if (aVb.z <= 0.0f) {
			aTri(H, D, G, aColor);
			aTri(B, D, G, aColor);
		} else {
			aTri(A, F, E, aColor);
			aTri(C, F, E, aColor);
		}
	}
	{
		Color aColor = aFaceColor(aVc.z);
		TriVertex A(aAX, aAY, 0.0f, 0.0f), B(aBX, aBY, 1.0f, 1.0f), C(aCX, aCY, 0.0f, 1.0f), D(aDX, aDY, 1.0f, 0.0f);
		TriVertex E(aEX, aEY, 0.0f, 1.0f), F(aFX, aFY, 0.0f, 0.0f), G(aGX, aGY, 1.0f, 1.0f), H(aHX, aHY, 1.0f, 0.0f);
		if (aVc.z <= 0.0f) {
			aTri(F, D, C, aColor);
			aTri(B, D, C, aColor);
		} else {
			aTri(A, H, E, aColor);
			aTri(G, H, E, aColor);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// RainDrops

// 0x6784d8: a place 100 or more from the middle (by |x| + |y|), tried again until it is: three of the C runtime's rand a try
void RainDrops::Drop::Place(WinTwistApp* theApp)
{
	(void)theApp;
	int aWidth = gApp->mWidth;
	int aRand = CrtRand();
	mX = (float)(aRand % aWidth - aWidth / 2);
	aWidth = gApp->mWidth;
	aRand = CrtRand();
	mY = (float)((aRand % aWidth - aWidth / 2) + -0x18);
	aWidth = gApp->mWidth;
	aRand = CrtRand();
	mZ = (float)(aRand % aWidth - aWidth / 2);
	mState = 0;
	mTimer = 0;
	float aAbsX = fabsf(mX);
	float aSum = (float)((double)fabsf(mY) + (double)aAbsX);
	while (!(100.0 <= (double)aSum)) {
		aWidth = gApp->mWidth;
		aRand = CrtRand();
		mX = (float)(aRand % aWidth - aWidth / 2);
		aWidth = gApp->mWidth;
		aRand = CrtRand();
		mY = (float)((aRand % aWidth - aWidth / 2) + -0x18);
		aWidth = gApp->mWidth;
		aRand = CrtRand();
		mZ = (float)(aRand % aWidth - aWidth / 2);
		mState = 0;
		mTimer = 0;
		aAbsX = fabsf(mX);
		aSum = (float)((double)fabsf(mY) + (double)aAbsX);
	}
}

RainDrops::RainDrops(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mSway = 0.0f;
	mPointy = LoadVizImage(IMAGE_POINTY_ID);
	mPointy2 = LoadVizImage(IMAGE_POINTY2_ID);
	mPointy3 = LoadVizImage(IMAGE_POINTY3_ID);
	mSplash = LoadVizImage(IMAGE_SPLASH_ID);
	mBackground = LoadVizImage(IMAGE_RAINDROPS_BG_ID);
}

RainDrops::~RainDrops()
{
	DeleteVizImage(IMAGE_POINTY_ID);
	DeleteVizImage(IMAGE_POINTY2_ID);
	DeleteVizImage(IMAGE_POINTY3_ID);
	DeleteVizImage(IMAGE_SPLASH_ID);
	DeleteVizImage(IMAGE_RAINDROPS_BG_ID);
	for (Drop* aDrop : mDrops)
		delete aDrop;
}

void RainDrops::Init()
{
	int aCount = 0x96;
	do {
		Drop* aDrop = new Drop();  // 0x678748
		aDrop->Place(mApp);
		mDrops.push_back(aDrop);  // 0x6795ea
		aCount--;
	} while (aCount != 0);
	aCount = 0x96;
	do {
		Drop* aDrop = new Drop();
		aDrop->Place(mApp);
		aDrop->mZ = (float)(mApp->mWidth / 2);
		aDrop->mState = 1;
		aDrop->mTimer = CrtRand() % 0x78;
		mDrops.push_back(aDrop);
		aCount--;
	} while (aCount != 0);
}

void RainDrops::Update()
{
	mCount++;
	std::vector<Drop*>::iterator anIterator = mDrops.begin();
	while (anIterator != mDrops.end()) {
		Drop* aDrop = *anIterator;
		if (aDrop->mState == 0) {
			aDrop->mZ = (float)((double)aDrop->mZ + 3.5);
			if ((float)(mApp->mWidth / 2) < aDrop->mZ) {
				aDrop->mTimer = 0;
				aDrop->mState = 1;
			}
		} else {
			aDrop->mTimer++;
			if (0x78 < aDrop->mTimer) {
				aDrop->Place(mApp);
				aDrop->mZ = (float)-(mApp->mWidth / 2);
			}
		}
		float anArg = (float)((double)mCount * 0.003000000026077032);
		mSway = (float)((double)cosf(anArg) * 20.0);
		++anIterator;
	}
}

// 0x678754: 13 angles round, each the ring's inner and outer vertices (26 vertices of the strip, 24 triangles), the ring
// the splash image's size turned and scaled
void RainDrops::DrawSplashRing(Graphics* g, float theX, float theY, float theAngle, float theScale)
{
	TriVertex aVertices[26];
	int anIndex = 0;
	TriVertex* aVertex = aVertices;
	do {
		float aRing = (float)((double)anIndex / 12.0 * 3.1415927410125732 * 2.0);
		float aTurned = (float)((double)aRing + (double)theAngle);
		float aCosTurned = cosf(aTurned);
		float aX = (float)((double)aCosTurned * 0.36000001430511475 * (double)mSplash->mWidth * (double)theScale);
		float aSinTurned = sinf(aTurned);
		float aY = (float)((double)aSinTurned * 0.36000001430511475 * (double)mSplash->mHeight * (double)theScale);
		float aU = (float)((double)cosf(aRing) * 0.36000001430511475 + 0.5);
		float aV = (float)((double)sinf(aRing) * 0.36000001430511475 + 0.5);
		aVertex[0] = TriVertex(aX, aY, aU, aV);
		aCosTurned = cosf(aTurned);
		aX = (float)((double)aCosTurned * 0.5 * (double)mSplash->mWidth * (double)theScale);
		aSinTurned = sinf(aTurned);
		aY = (float)((double)aSinTurned * 0.5 * (double)mSplash->mHeight * (double)theScale);
		aU = (float)((double)cosf(aRing) * 0.5 + 0.5);
		aV = (float)((double)sinf(aRing) * 0.5 + 0.5);
		aVertex[1] = TriVertex(aX, aY, aU, aV);
		aVertex += 2;
		anIndex++;
	} while (anIndex < 0xd);
	gD3DInterface->DrawTrianglesTexStrip(mSplash, aVertices, 0x18, g->mColor, 1, theX, theY, true);  // slot 0x78
}

void RainDrops::Draw(Graphics* g)
{
	g->PushState();  // 0x41fd70
	g->SetColorizeImages(true);
	g->SetColor(Color(0x333333));  // 0x4016b0
	DrawImage(g, mBackground);  // 0x67fdb6
	std::vector<Drop*>::iterator anIterator = mDrops.begin();
	while (anIterator != mDrops.end()) {
		Drop* aDrop = *anIterator;
		// how far it has come (0 far, 1 near), how far to the side, its depth
		float aNear = (float)(0.5 - (double)aDrop->mZ * -0.00048828125);
		if (0.0f <= aNear) {
			if (1.0f < aNear)
				aNear = 1.0f;
		} else {
			aNear = 0.0f;
		}
		float aX = (float)((double)aDrop->mX + (double)mSway);
		int aHalfWidth = mApp->mWidth / 2;
		float aHalfWidthF = (float)aHalfWidth;
		float aDepth = (float)(((double)aHalfWidthF + (double)aDrop->mZ) * 0.009999999776482582);
		if (1.0f < aDepth)
			aDepth = 1.0f;
		float aTilt = (float)(((double)aHalfWidthF - (double)aDrop->mZ) * ((double)mSway * 0.00019999999494757503));
		float aAbsY = fabsf(aDrop->mY);  // 0x401ad0
		double aShiftedX = (double)aTilt + (double)aX;
		float aShifted = (float)aShiftedX;
		float aAbsShifted = fabsf(aShifted);
		int aWidth = mApp->mWidth;
		float aInverse = (float)(1.0 / (((((double)aHalfWidth + (double)aDrop->mZ) + (double)aAbsY) + (double)aAbsShifted) /
			(double)(aWidth * 2) + 0.25));
		float aFall = (float)(1.25 - (double)aNear);
		float aRate = (float)((double)mSway * 0.0010000000474974513 * (double)aWidth);
		Transform aTransform;  // 0x44f9d0
		if (aDrop->mState == 0) {
			// falling: a point of the three sizes by how far it is from the middle
			float aTestX = (float)((double)aRate * 3.5 + (double)aX);
			float aSumAbs = (float)((double)fabsf(aDrop->mY) + (double)fabsf(aTestX));
			Image* anImage = mPointy;
			if (aSumAbs < 200.0f)
				anImage = mPointy3;
			else if (aSumAbs < 400.0f)
				anImage = mPointy2;
			g->SetDrawMode(0);  // 0x41ff20
			int aRed = Ftol((double)aInverse * 66.0);
			if (0xff < aRed)
				aRed = 0xff;
			int aGreen = Ftol((double)aInverse * 75.0);
			if (0xff < aGreen)
				aGreen = 0xff;
			int aBlue = Ftol((double)aInverse * 73.0);
			if (0xff < aBlue)
				aBlue = 0xff;
			int anAlpha = Ftol((double)aDepth * 200.0);
			g->SetColor(Color(aRed, aGreen, aBlue, anAlpha));
			float aScale = (float)((double)aInverse * 0.20000000298023224);
			aTransform.Scale(aScale, aScale);  // 0x44fae0
			float anAngle = (float)(atan2((double)aShifted, (double)aDrop->mY) + 3.140000104904175);  // 0x4c1b70
			aTransform.RotateRad(anAngle);  // 0x44fa70
			float aDrawX = (float)(((double)aFall * 1.25 + 0.550000011920929) * aShiftedX + (double)(mApp->mWidth / 2));
			float aDrawY = (float)(((double)aFall * 1.25 + 0.25) * (double)aDrop->mY + (double)(mApp->mHeight / 2));
			g->DrawImageTransformF(anImage, aTransform, aDrawX, aDrawY);  // 0x422050
		} else {
			// splashing: fading out, spreading
			g->SetDrawMode(1);  // 0x41ff20
			float aFade = (float)(1.0 - (double)aDrop->mTimer / 60.0);
			if (aFade < 0.0f)
				aFade = 0.0f;
			float aLengthSquare = (float)((double)aDrop->mX * (double)aDrop->mX + (double)aDrop->mY * (double)aDrop->mY);
			float aFar = (float)(std::sqrt((double)aLengthSquare) * 0.0010000000474974513);  // 0x4c0650
			if (aFar < 1.0f)
				aFar = 1.0f;
			int anAlpha = Ftol((double)aFade * 255.0 / (double)aFar);
			g->SetColor(Color(0x88, 0x88, 0x88, anAlpha));
			float aGrow = (float)((double)aDrop->mTimer * 0.029999999329447746 + 1.0);
			float aScaleX = (float)((double)aGrow * 0.17499999701976776);
			float aScaleY = (float)((double)aGrow * 0.15000000596046448);
			aTransform.Scale(aScaleX, aScaleY);
			float anAngle = (float)(atan2((double)aX, (double)aDrop->mY) + 0.14000000059604645);
			aTransform.RotateRad(anAngle);
			double aFallScaled = (double)aFall * 1.25;
			if (1.0f <= aGrow) {
				// a ring of the splash's triangles
				float anAngle2 = (float)(atan2((double)aX, (double)aDrop->mY) + 0.14000000059604645);
				float aDrawY = (float)((aFallScaled + 0.25) * (double)aDrop->mY + (double)(mApp->mHeight / 2));
				float aDrawX = (float)(((aFallScaled + 0.550000011920929) * (double)aX + (double)(mApp->mWidth / 2)) -
					(double)mSway * 0.75);
				DrawSplashRing(g, aDrawX, aDrawY, anAngle2, aScaleX);  // 0x678754
			} else {
				Rect aSource(0, 0, IMAGE_SPLASH->mWidth, IMAGE_SPLASH->mHeight);
				float aDrawX = (float)(((aFallScaled + 0.550000011920929) * (double)aX + (double)(mApp->mWidth / 2)) -
					(double)mSway * 0.75);
				float aDrawY = (float)((aFallScaled + 0.25) * (double)aDrop->mY + (double)(mApp->mHeight / 2));
				g->DrawImageTransformF(mSplash, aTransform, aSource, aDrawX, aDrawY);  // 0x4220f0
			}
		}
		++anIterator;
	}
	g->SetDrawMode(1);
	// port: the original goes on to set a colour and a transform for each of twelve rays (0x6791fb to 0x679306) and draws
	// nothing with them; only the colour, which the PopState below drops, is kept
	int aRay = 0;
	int aSeed = 0x75;
	do {
		float aFactor = 1.0f;
		if (aRay % 2 == 0)
			aFactor = -0.5f;
		else if (aRay % 2 == 1)
			aFactor = 0.3f;
		float aRayScale = (float)((double)(aSeed % 6 + 9) * 0.11999999731779099);
		(void)aFactor;
		g->SetColor(Color(0xff, 0xff, 0xff, Ftol((double)aRayScale * 35.0)));
		aRay++;
		aSeed += 9;
	} while (aSeed < 0xe1);
	g->PopState();  // 0x41fe00
}
