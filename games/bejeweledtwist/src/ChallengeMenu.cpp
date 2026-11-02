// Sexy::ChallengeMenu and Sexy::ChallengeButton (ChallengeMenu.h).
#include "ChallengeMenu.h"

#include "AwardTelly.h"
#include "Board.h"
#include "RankBonusWidget.h"
#include "Buffer.h"
#include "Challenge.h"
#include "Common.h"
#include "DDInterface.h"
#include "EffectsManager.h"
#include "Font.h"
#include "Image.h"
#include "MemoryImage.h"
#include "GameText.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Menu.h"
#include "PlayButton.h"
#include "Profile.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

// The globals of the menu's camera and sun (ChallengeMenu.h)
CurvedVal Sexy::gSunCurve;
Transform3D Sexy::gMenuCamera;
float Sexy::gUnknown85C508 = 0;
Color Sexy::gUnknown85C50C;
Color Sexy::gSunGlowColor;
float Sexy::gAspect = 0;
float Sexy::gAspect45 = 0;

namespace {

// 0x7570c1: the colour a blended towards b by theT (a's alpha kept)
Color BlendColors(const Color& a, const Color& b, float theT)
{
	Color aColor = a;
	aColor.mRed = Ftol((double)(b.mRed - a.mRed) * (double)theT + (double)aColor.mRed);
	aColor.mGreen = Ftol((double)(b.mGreen - a.mGreen) * (double)theT + (double)aColor.mGreen);
	aColor.mBlue = Ftol((double)(b.mBlue - a.mBlue) * (double)theT + (double)aColor.mBlue);
	return aColor;
}

// The camera curves are in degrees: the radians as a float, as 0x6e2318 and 0x6e3f41 make them
float CurveRadians(CurvedVal& theCurve)
{
	float aDegrees = (float)(theCurve.GetOutVal() + 0.0);
	return (float)((double)aDegrees * 3.1415927410125732 / 180.0);
}

}  // namespace

// 0x6e5337: the planet is a TwistButton on IMAGE_CHALLENGE_BUTTON whose id is its category's number. The menu's
// constructor gives it its pictures, color and name (0x6e5c3e), AddedToManager its place in the 3D world (0x6dfc49)
ChallengeButton::ChallengeButton(int theCategory, ChallengeMenu* theMenu)
	: TwistButton(theCategory, IMAGE_CHALLENGE_BUTTON, theMenu)
{
	mMenu = theMenu;
	mCategory = theCategory;
	mClip = false;
	// The idle curve (the one curve the constructor sets; Update steps it, nothing here reads it)
	mIdle.SetCurve(GTS(0x7e0b6c));
}

// 0x6e3b0e
void ChallengeButton::Update()
{
	if (mUnknown51D)
		return;
	if (mTip != nullptr)
		mTip->Update();
	if (!mIdle.HasBeenTriggered())
		mIdle.IncInVal();
	// The name's glow rises while the mouse is over a planet that is all the way in, and falls otherwise
	if (!mIsOver || mFade != 1.0f) {
		double aGlow = (double)mGlow - 0.004999999888241291;
		mGlow = (float)(aGlow >= 0.0 ? aGlow : 0.0);
	} else {
		double aGlow = (double)mGlow + 0.019999999552965164;
		mGlow = (float)(aGlow <= 0.5 ? aGlow : 0.5);
	}
	if (mDim.CheckInThreshold(0.8999999761581421))
		gApp->PlaySample(SOUND_VOICE_NEWCHALLENGEUNLOCKED);
	mDim.IncInVal();
	mBarFill.IncInVal();
	mUnknown398.IncInVal();
	if (mUnknown1D8.mIncRate != 0.0 && !mUnknown1D8.HasBeenTriggered() && !mUnknown1D8.IncInVal() && mUnknown5CD)
		gApp->mMainMenu->mChallengeMenu->UpdateProgress(false, false);
	mEclipseFade.IncInVal();
	mTextFade.IncInVal();
	mUnknown248.IncInVal();
	mUnknown2B8.IncInVal();
	int aPulses = mUnknown52C;
	if (0 < aPulses || 0.0f < mUnknown530) {
		double aLeft = (double)mUnknown530 - 0.03999999910593033;
		mUnknown530 = (float)(aLeft >= 0.0 ? aLeft : 0.0);
		if (mUnknown530 <= 0.0f && 0 < aPulses) {
			mUnknown530 = 1.0f;
			mUnknown52C = aPulses - 1;
		}
	}
	if (mUnknown4E8 < 1.0f) {
		double aRise = (double)mUnknown4E8 + 0.004999999888241291;
		mUnknown4E8 = (float)(aRise <= 1.0 ? aRise : 1.0);
		int aWidth = mButtonImage->GetCelWidth();
		int aHeight = mButtonImage->GetCelHeight();
		int aX = mX;
		int aY = mY;
		// Sparks about the planet while it comes in
		int aCount = Ftol((1.0 - (double)mUnknown4E8) * 3.0);
		while (0 < aCount) {
			float anAngle = RandSigned();
			Effect* anEffect = mMenu->mFXManager1B8->AllocEffect(0);
			anEffect->mImage = IMAGE_GEM_FRUIT_SPARK;
			anEffect->mIsAdditive = true;
			float aRadius = std::fabs(RandSigned());
			float aCos = std::cos((float)((double)anAngle * 3.1415927410125732));
			anEffect->mX = (float)((double)aCos * (double)(aWidth / 2) * (double)aRadius + (double)(aX + aWidth / 2));
			float aSin = std::sin((float)((double)anAngle * 3.1415927410125732));
			anEffect->mFlags |= Effect::FLAG_SCALE_BOUNCE;
			anEffect->mY = (float)((double)aSin * (double)(aHeight / 2) * (double)aRadius + (double)(aY + aHeight / 2));
			anEffect->mDY = 0;
			anEffect->mDX = 0;
			anEffect->mScale = 0.009999999776482582f;
			anEffect->mDScale = 0.025f;
			anEffect->mAngle = 0.009999999776482582f;
			anEffect->mGravity = 0;
			anEffect->mAlpha = 1.0f;
			anEffect->mDAlpha = 0;
			anEffect->mMaxScale = 1.0f;
			mMenu->mFXManager1B8->AddEffect(anEffect);
			aCount--;
		}
	}
	// The bar and the name come in while the planet shows its text, and go out when it does not
	if (!mUnknown5C4) {
		double aLeft = (double)mFade - 0.05000000074505806;
		mFade = (float)(aLeft >= 0.0 ? aLeft : 0.0);
	} else {
		double aMore = (double)mFade + 0.007499999832361937;
		mFade = (float)(1.0 < aMore ? 1.0 : aMore);
	}
	if (mEclipse) {
		// The eclipse title's flares (32 of them, each 0 or on its way from 0 to 1) run on, and one more starts every ten
		// updates
		for (int i = 0; i < 32; i++) {
			if (0.0f < mEclipseFlares[i]) {
				float aFlare = (float)((double)mEclipseFlares[i] + 0.009999999776482582);
				mEclipseFlares[i] = aFlare;
				if (1.0f <= aFlare)
					mEclipseFlares[i] = 0.0f;
			}
		}
		if (mUpdateCnt % 10 == 0) {
			int aFlare = Rand() % 32;
			if (mEclipseFlares[aFlare] == 0.0f)
				mEclipseFlares[aFlare] = 0.009999999776482582f;
		}
	}
	TwistButton::Update();
}

// 0x6dd224: the other mouse buttons (a negative click count) do nothing
void ChallengeButton::MouseDown(int x, int y, int theClickCount)
{
	if (-1 < theClickCount)
		TwistButton::MouseDown(x, y, theClickCount);
}

// 0x6dd237: a circle about the middle of the picture, whatever the planet's size on the screen
bool ChallengeButton::IsPointVisible(int x, int y)
{
	float aDX = (float)(x - mWidth / 2);
	float aDY = (float)(y - mHeight / 2);
	float aDistance = std::sqrt(aDX * aDX + aDY * aDY);
	int aRadius = Ftol((double)gApp->mScale * 80.0);
	return (double)aDistance < (double)aRadius;
}

// 0x6dd1e9: panned by where the planet is, and pitched by its number
void ChallengeButton::PlayMouseOverSound()
{
	gApp->PlaySoundAt(SOUND_MOUSEOVER2, mX, (float)((double)mId * 0.5), 0.0f);
}

// 0x6dde98 with Direct3D off. The planet's place in the 3D world goes through the camera to the screen (the button
// takes its place there, so the clicks land on the planet), a picture of the planet is made for the size it has,
// and the ring, name, moon meter and glow are drawn about it. The original also sets up Direct3D's matrices, light and
// fog and the sphere meshes' colors first (0x6e4fad's meshes, the planet's turn, the clouds' additive blend): only the
// 3D drawing uses them.
void ChallengeButton::Draw(Graphics* g)
{
	mClip = false;
	PerspectiveCamera aCamera(gAspect45, gAspect, 1.0f, 10000.0f);  // 0x67417d
	aCamera.mTransform = gMenuCamera;                              // 0x673c10
	if (gApp->Is3DAccelerated())
		NotPorted("ChallengeButton::Draw 0x6dde98: the planet's sphere meshes (0x6e4fad), light and fog with Direct3D");
	g->SetColorizeImages(true);
	g->SetColor(Color::White);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// Where the planet is on the screen, and how big it is there (0.00151515 over its depth)
	Vec3 aWorld{mWorldPos[0], mWorldPos[1], mWorldPos[2]};
	Vec3 aProjected = aCamera.ProjectWorld(aWorld);
	float aScreenX = (float)((double)(float)g->mDestImage->mWidth * (double)aProjected.x);
	float aScreenY = (float)((double)(float)g->mDestImage->mHeight * (double)aProjected.y);
	mScreenPos[0] = aScreenX;
	mScreenPos[1] = aScreenY;
	mScreenPos[2] = aProjected.z;
	if (0.0f > mScreenPos[2])
		return;
	float aScale = (float)(0.0015151499537751079 / (double)mScreenPos[2]);
	mX = Ftol((double)aScreenX - (double)(mWidth / 2));
	mY = Ftol((double)aScreenY - (double)(mHeight / 2));
	// From here on in the screen's own coordinates
	float aTransX = g->mTransX;
	float aTransY = g->mTransY;
	g->Translate(Ftol(-(double)aTransX), Ftol(-(double)aTransY));

	// The planet's picture, in the menu's 512 by 512 image: for each pixel of the size it has, IMAGE_PLANET says how
	// bright it is (red, the less the brighter) and where to look in the planet's layer (blue is the column, green
	// over two the row), and the layer is read by two columns, blended by the part after the point
	static_assert(sizeof(ulong) == 4, "the planet pictures are 32 bits a pixel");
	MemoryImage* aBuffer = gApp->mMainMenu->mChallengeMenu->mPlanetImage;
	double aSize = (double)aScale * 76.0;
	int aBufferWidth = aBuffer->mWidth;
	int aBufferHeight = aBuffer->mHeight;
	int aWidth = Ftol((double)aBufferWidth <= aSize ? (double)aBufferWidth : aSize);
	int aHeight = Ftol((double)aBufferHeight <= aSize ? (double)aBufferHeight : aSize);
	if (aWidth <= aBufferWidth) {
		const uint32_t* aPlanetBits = reinterpret_cast<const uint32_t*>(static_cast<MemoryImage*>(IMAGE_PLANET)->GetBits());
		const uint32_t* aLayerBits = reinterpret_cast<const uint32_t*>(static_cast<MemoryImage*>(mLayers[0])->GetBits());
		uint32_t* aBufferBits = reinterpret_cast<uint32_t*>(aBuffer->GetBits());
		float aStepX = (float)(384.0 / (double)aWidth);
		float aStepY = (float)(384.0 / (double)aHeight);
		int aBrightness = Ftol((1.0 - mDim.GetOutVal()) * 200.0 + 55.0);
		int aDestX = Ftol((double)aScreenX - (double)(aWidth / 2));
		int aDestY = Ftol((double)aScreenY - (double)(aHeight / 2));
		Rect aDest(aDestX, aDestY, aWidth, aHeight);
		if (aDest.Intersects(g->mClipRect)) {
			// the clouds drift with the size: 0.4 of the planet's scale, in columns
			double aShift = (double)aScale * 0.4000000059604645;
			float aRowAt = 0;
			for (int aRow = 0; aRow < aHeight; aRow++) {
				uint32_t* aOut = aBufferBits + (size_t)aBufferWidth * (size_t)aRow;
				const uint32_t* aPlanetRow = aPlanetBits + Ftol((double)aRowAt) * 384;
				float aColumnAt = 0;
				for (int aColumn = 0; aColumn < aWidth; aColumn++) {
					uint32_t aPlanet = aPlanetRow[Ftol((double)aColumnAt)];
					uint32_t aLayerRow = (((aPlanet >> 8 & 0xff) / 2) << 8);
					uint32_t aLight = ((0xff - (aPlanet >> 16 & 0xff)) * (uint32_t)aBrightness) >> 8;
					float aBlue = (float)((double)(aPlanet & 0xff) / 1.5);
					float aLookup = (float)((double)aBlue + aShift);
					int aFloor = Ftol((double)aLookup);
					uint32_t aFraction = (uint32_t)Ftol(((double)aLookup - (double)aFloor) * 255.0);
					uint32_t aFirst = aLayerBits[(aFloor & 0xff) + aLayerRow];
					uint32_t aSecond = aLayerBits[((aFloor + 1) & 0xff) + aLayerRow];
					uint32_t aBlended = ((aFirst & 0xff0000) * (0x100 - aFraction) + (aSecond & 0xff0000) * aFraction) & 0xff0000ff;
					aBlended |= ((aFirst & 0xff00) * (0x100 - aFraction) + (aSecond & 0xff00) * aFraction) & 0xff0000;
					aBlended |= (aFirst & 0xff) * (0x100 - aFraction) + (aSecond & 0xff) * aFraction;
					aBlended >>= 8;
					uint32_t aLit = ((aBlended & 0xff0000) * aLight) & 0xff0000ff;
					aLit |= ((aBlended & 0xff00) * aLight) & 0xff0000;
					aLit |= ((aBlended & 0xff) * aLight) & 0xff00;
					*aOut++ = (aLit >> 8) | (aPlanet & 0xff000000);
					aColumnAt = (float)((double)aColumnAt + (double)aStepX);
				}
				aRowAt = (float)((double)aRowAt + (double)aStepY);
			}
			aBuffer->BitsChanged();
			g->SetColor(Color(255, 255, 255));
			g->DrawImage(aBuffer, aDestX, aDestY, Rect(0, 0, aWidth, aHeight));
		}
	}

	// The ring, and (below) the name and moon meter, about the planet and at half its scale
	float aHalfScale = (float)((double)aScale * 0.5);
	g->PushState();
	g->Translate(Ftol((double)aScreenX), Ftol((double)aScreenY));
	g->SetScale(aHalfScale, aHalfScale, 0, 0);
	int aNameY = Ftol((double)gApp->mScale * -5.0);
	Color aRingColor = Color::White;
	float aRingScale;
	double aOneMinusScale = 1.0 - (double)aScale;
	double aRingAlpha;
	if (!mOpen) {
		aRingColor = Color(0, 0, 0);
		aRingScale = (float)((double)mNameScale * 1.4500000476837158 * 2.0);
		aRingAlpha = 255.0;
	} else {
		aRingScale = (float)((double)mNameScale * 1.5 * 2.0);
		aRingAlpha = 128.0;
	}
	double aRingFade = 1.0 + aOneMinusScale;
	double aRingFadeAbove = aRingFade < 0.0 ? 0.0 : aRingFade;
	if (aRingFadeAbove * aRingAlpha <= aRingAlpha)
		aRingAlpha = aRingAlpha * aRingFadeAbove;
	aRingColor.mAlpha = Ftol(aRingAlpha);
	g->SetColor(aRingColor);
	g->PushState();
	float aRingDrawScale = (float)((double)aRingScale * (double)aScale * 0.25);
	g->SetScale(aRingDrawScale, aRingDrawScale, 0, 0);
	DrawImageScaledCentered(g, IMAGE_CHALLENGEUI_PLANETRING, 0, 0, ALIGN_CENTER, 1.0f, 1.0f);
	g->PopState();
	if (!mUnknown5C4) {
		g->PopState();
		g->Translate(Ftol((double)aTransX), Ftol((double)aTransY));
		return;
	}

	// The name and the moon meter (a bar of cells, one for each challenge of the category) fade in with mFade
	float aTextFade = (float)((double)mFade * (1.0 - mDim.GetOutVal()));
	if (mUnknown5C5) {
		double aShrink = aOneMinusScale * 0.550000011920929 + 1.0;
		if (aShrink < 0.0)
			aShrink = 0.0;
		aTextFade = (float)(aShrink * (double)aTextFade);
	}
	float aBarFade = (float)(mTextFade.GetOutVal() * (double)aTextFade);
	g->SetColor(Color(255, 255, 255, Ftol((double)aTextFade * 255.0)));
	if (mNameImage != nullptr) {
		float aNameScale = mNameScale + mNameScale;
		DrawImageScaledCentered(g, mNameImage, 0, (float)aNameY, aNameScale, aNameScale);
	}
	double aBarAlpha = (double)aBarFade * 255.0;
	int aBarAlphaInt = Ftol((double)aBarFade * 255.0);
	g->SetColor(Color(255, 255, 255, aBarAlphaInt));
	Image* aFillBar = IMAGE_MOONMETER_FILLBAR;
	Image* anEmptyBar = IMAGE_MOONMETER_EMPTYBAR;
	Image* aGlowNumbers = IMAGE_MOONMETER_GLOWNUMBERS;
	Image* aGreyNumbers = IMAGE_MOONMETER_GREYNUMBERS;
	Image* aBarGlow = IMAGE_MOONMETER_BARGLOW;
	int aBarY = Ftol((double)gApp->mScale * 160.0);
	int aBarX = Ftol((double)gApp->mScale * 0.0);
	int aCellCount = aGreyNumbers->mNumCols * aGreyNumbers->mNumRows;  // 0x423770
	Color aFull(mCategoryColor.mRed, mCategoryColor.mGreen, mCategoryColor.mBlue, aBarAlphaInt);
	Color aDark(Ftol((double)mCategoryColor.mRed * 0.5), Ftol((double)mCategoryColor.mGreen * 0.5),
		Ftol((double)mCategoryColor.mBlue * 0.5), aBarAlphaInt);
	float aBarLeft = (float)((double)aBarX - (double)(aFillBar->mWidth / 2));
	int aCellWidth = aFillBar->mWidth / aCellCount;
	g->SetColor(aDark);
	g->DrawImage(anEmptyBar, Ftol((double)aBarX - (double)(anEmptyBar->mWidth / 2)),
		Ftol((double)aBarY - (double)(anEmptyBar->mHeight / 2)));
	g->SetColor(aFull);
	// The bar's fill: the done cells and the part of the next that mBarFill has got to
	double aFillWidth = (double)aFillBar->mWidth;
	if ((double)(float)aCellWidth * ((double)(mStage - 1) + mBarFill.GetOutVal()) <= aFillWidth)
		aFillWidth = ((double)(mStage - 1) + mBarFill.GetOutVal()) * (double)(float)aCellWidth;
	int aFillPixels = Ftol(aFillWidth);
	if (aFillPixels != 0) {
		g->DrawImage(aFillBar, Ftol((double)aBarLeft), Ftol((double)aBarY - (double)(aFillBar->mHeight / 2)),
			Rect(0, 0, aFillPixels, aFillBar->mHeight));
	}
	float aCellLeft = aBarLeft;
	Rect aSavedClip = g->mClipRect;
	bool aTrial = !gApp->mRegistered;
	bool aLockedCategory = aTrial && mId == 3;
	double aHalfCell = (double)((float)aCellWidth * 0.5);
	for (int aCell = 0; aCell < aCellCount; aCell++) {
		Rect aSource(0, 0, 0, 0);
		int aYShift = 0;
		Image* aCellImage;
		if (aCell != mStage - 1 || aLockedCategory || (aTrial && 3 < mStage)) {
			if (aCell < mStage - 1 && !aLockedCategory) {
				// a cell done
				aCellImage = aGreyNumbers;
				aSource = aGreyNumbers->GetCelRect(aCell);
				g->SetColor(aFull);
			} else {
				// a cell still to come: the lock (red in a copy that is not registered, past the third challenge)
				if (aTrial && (2 < aCell || mId == 3))
					g->SetColor(Color(255, 0x40, 0x40, Ftol((double)aBarFade * 220.0)));
				else
					g->SetColor(Color(255, 255, 255, aBarAlphaInt));
				aCellImage = IMAGE_MOONMETER_LOCK;
				aSource = Rect(0, 0, IMAGE_MOONMETER_LOCK->mWidth, IMAGE_MOONMETER_LOCK->mHeight);
				if (aCell == mStage) {
					// the lock the new stage opens wipes away from the top
					aSource.mY = Ftol(mUnknown1D8.GetOutVal() * (double)aSource.mHeight + (double)aSource.mY);
					aSource.mHeight = aSource.mHeight - aSource.mY;
					aYShift = (aSource.mY + (aSource.mY >> 31 & 3)) >> 2;
				}
			}
		} else {
			// the cell of the challenge the planet starts: its glowing number
			aCellImage = aGlowNumbers;
			aSource = aGlowNumbers->GetCelRect(aCell);
			g->SetColor(Color(255, 255, 255, Ftol((1.0 - mUnknown248.GetOutVal()) * aBarAlpha)));
		}
		int aDrawY = Ftol(((double)aBarY - (double)(aSource.mHeight / 2)) + (double)aYShift + 0.0);
		int aDrawX = Ftol(((double)aCellLeft + aHalfCell) - (double)(aSource.mWidth / 2));
		g->DrawImage(aCellImage, aDrawX, aDrawY, aSource);
		aCellLeft = (float)((double)(float)aCellWidth + (double)aCellLeft);
	}
	if (mUnknown1D8.GetOutVal() != 0.0) {
		// the number just done fades to grey, and the next glows
		int anAlpha = Ftol((double)aBarAlphaInt * (double)aBarFade * mUnknown1D8.GetOutVal());
		g->SetColor(Color(aFull.mRed, aFull.mGreen, aFull.mBlue, anAlpha));
		Rect aSource = aGreyNumbers->GetCelRect(mStage - 1);
		int aDrawY = Ftol(((double)aBarY - (double)(aSource.mHeight / 2)) + 0.0);
		int aDrawX = Ftol(((double)(mStage - 1) * (double)(float)aCellWidth + (double)aBarLeft + aHalfCell) -
			(double)(aSource.mWidth / 2));
		g->DrawImage(aGreyNumbers, aDrawX, aDrawY, aSource);
		int aNext = mStage;
		int aCount = gApp->mChallengeInfo->GetChallengeCount(mCategory);
		if (aNext < aCount && (gApp->mRegistered || aNext < 3)) {
			g->SetColor(Color(255, 255, 255, Ftol(mUnknown1D8.GetOutVal() * aBarAlpha)));
			int aCel = aGlowNumbers->mNumCols - 1;
			if (mStage < aGlowNumbers->mNumCols - 1)
				aCel = mStage;
			aSource = aGlowNumbers->GetCelRect(aCel);
			aDrawY = Ftol(((double)aBarY - (double)(aSource.mHeight / 2)) + 0.0);
			aDrawX = Ftol(((double)mStage * (double)(float)aCellWidth + (double)aBarLeft + aHalfCell) -
				(double)(aSource.mWidth / 2));
			g->DrawImage(aGlowNumbers, aDrawX, aDrawY, aSource);
		}
	}
	// The lock the new stage opens glows, added to the picture
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown2B8.GetOutVal() * aBarAlpha)));
	Rect aLockSource = IMAGE_MOONMETER_LOCK->GetCelRect(0);
	aLockSource.mY = Ftol(mUnknown248.GetOutVal() * (double)aLockSource.mHeight + (double)aLockSource.mY);
	aLockSource.mHeight = aLockSource.mHeight - aLockSource.mY;
	int aLockShift = (aLockSource.mY + (aLockSource.mY >> 31 & 3)) >> 2;
	int aLockY = Ftol(((double)aBarY - (double)(aLockSource.mHeight / 2)) + (double)aLockShift + 0.0);
	int aLockX = Ftol(((double)(float)aCellWidth * 0.5 + (double)mStage * (double)(float)aCellWidth + (double)aBarLeft) -
		(double)(aLockSource.mWidth / 2));
	g->DrawImage(IMAGE_MOONMETER_LOCK, aLockX, aLockY, aLockSource);
	if (mEclipse) {
		g->mClipRect = aSavedClip;
		g->SetColor(Color(255, 255, 255, Ftol(mEclipseFade.GetOutVal() * 255.0 * (double)aTextFade)));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->DrawImage(IMAGE_ECLIPSE_TITLE, Ftol((double)aBarX - (double)(IMAGE_ECLIPSE_TITLE->mWidth / 2)),
			Ftol((double)aBarY - (double)(IMAGE_ECLIPSE_TITLE->mHeight / 2)));
		if (gApp->Is3DAccelerated())
			NotPorted("ChallengeButton::Draw 0x6df6b0: the eclipse title's flare strips (mEclipseFlares), drawn with Direct3D only");
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(aFull);
	g->DrawImage(aBarGlow, Ftol((double)aBarX - (double)(aBarGlow->mWidth / 2)),
		Ftol((double)aBarY - (double)(aBarGlow->mHeight / 2)));
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->ClearClipRect();
	if (0.0f < mGlow) {
		// The name glows while the mouse is over the planet: its picture again, a little larger each time, added
		float aGlow = mGlow;
		float aGlowScale = mNameScale + mNameScale;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		for (; 0.0f < aGlow; aGlow = (float)((double)aGlow - 0.009999999776482582)) {
			g->SetColor(Color(255, 255, 255, Ftol((double)aGlow * 255.0 * 0.10000000149011612 * (double)aTextFade)));
			if (mNameImage != nullptr)
				DrawImageScaledCentered(g, mNameImage, 0, (float)aNameY, aGlowScale, aGlowScale);
			aGlowScale = (float)((double)aGlowScale + 0.03999999910593033);
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(Color::White);
	}
	if (mTip != nullptr) {
		mTip->mX = Ftol((double)mWidth * 0.0);
		mTip->mY = Ftol((double)gApp->mScale * 5.0);
		mTip->Draw(g);
	}
	g->Translate(Ftol((double)aTransX), Ftol((double)aTransY));
	g->PopState();
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x6e5c3e
ChallengeMenu::ChallengeMenu()
{
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mVisible = false;
	mHover = -1;
	mCategory = 0;
	mStageSel = 0;
	mStarted = 0;
	mHoverFade = 0;
	mUnknown9BC = false;
	mDragging = false;
	mCameraStackIndex = 0;
	mStartDelay = 0;
	mUnknown15C = 0;
	// The camera starts at (0, -12, 0) (+0x630) and the camera stack comes from its file (0x6e5a8d)
	mCameraPos[0] = 0;
	mCameraPos[1] = -12.0f;
	mCameraPos[2] = 0;
	LoadCameraStack();
	// The sphere meshes of the 3D draw (0x6e4fad, three sizes) and the 512 by 512 picture the software planets are made
	// in (+0x9e0, not freed by the original)
	if (!gApp->Is3DAccelerated()) {
		mPlanetImage = new MemoryImage(gApp);
		mPlanetImage->SetImageMode(true, true);
		mPlanetImage->SetVolatile(true);
		mPlanetImage->Create(0x200, 0x200);
	}
	// The planets, one for each category, with their layer pictures (planet and clouds), title and color
	float aNameScale = 1.0f;
	if ((double)gApp->mWidth / (double)gApp->mHeight < 1.7777777910232544)
		aNameScale = 0.75f;
	ChallengeInfo* aChallengeInfo = gApp->mChallengeInfo;
	for (unsigned int aCategory = 0; aCategory < (unsigned int)aChallengeInfo->mCategories.size(); aCategory++) {
		ChallengeButton* aButton = new ChallengeButton((int)aCategory, this);
		ChallengeCategory* aChallengeCategory = aChallengeInfo->mCategories[aCategory];
		for (unsigned int aLayer = 0; aLayer < (unsigned int)aChallengeCategory->mLayers.size(); aLayer++) {
			SexyString aLayerName = StringToUpper(StringToSexyString("IMAGE_") + aChallengeCategory->mLayers[aLayer]);
			int anId = GetIdByName(aLayerName.c_str());
			aButton->mAdditive = aChallengeCategory->mAdditive;
			if ((int)aCategory < (int)gApp->mProfile->mInts204.size())
				aButton->mProgress = gApp->mProfile->mInts204[aCategory];
			if (anId == RESOURCE_ID_MAX)
				gApp->Popup("Error: Can't find texture '" + aLayerName + "' for challenge menu");
			else
				aButton->mLayers.push_back(GetImageById(anId));
		}
		aButton->Resize(0, 0, aButton->mButtonImage->GetCelWidth(), aButton->mButtonImage->GetCelHeight());
		aButton->mCategory = (int)aCategory;
		SexyString aTitleName = StringToUpper(StringToSexyString("IMAGE_CHALLENGEUI_TITLES_") + aChallengeInfo->GetCategoryId(aCategory));
		int aTitleId = GetIdByName(aTitleName.c_str());
		if (aTitleId == RESOURCE_ID_MAX)
			gApp->Popup("Error: Can't find texture " + aTitleName);
		else
			aButton->mNameImage = GetImageById(aTitleId);
		aButton->mCategoryColor = aChallengeCategory->mColor;
		aButton->mNameScale = aNameScale;
		mButtons.push_back(aButton);
	}
	UpdateProgress(false, false);
	// The two buttons below: back to the play menu and to the main menu (0x754172 makes them)
	mPlayMenuButton = new TwistButton(BUTTON_PLAY_MENU, IMAGE_SIZEABLEBUTTON, this);
	mPlayMenuButton->mLabel = gApp->GetString(GTS(0x7e0cd0), GT(0x7e0cbc));
	mPlayMenuButton->SetFont(FONT_DIALOG_BUTTONS);
	mMainMenuButton = new TwistButton(BUTTON_MAIN_MENU, IMAGE_SIZEABLEBUTTON, this);
	mMainMenuButton->mLabel = gApp->GetString(GTS(0x7e0cb0), GT(0x7e0c9c));
	mMainMenuButton->SetFont(FONT_DIALOG_BUTTONS);
	// Two particle managers (0x6f9f59): +0x1b8 is a widget of the menu, +0x1bc the sun's glow
	mFXManager1B8 = new EffectsManager();
	mFXManager1BC = new EffectsManager();
	TwistButton* aButtons[2] = {mPlayMenuButton, mMainMenuButton};
	for (TwistButton* aButton : aButtons) {
		// as wide as the label needs, with a tenth to spare, but not narrower than the picture
		double aLabelWidth = (double)FONT_DIALOG_BUTTONS->StringWidth(aButton->mLabel) * 1.1000000238418579;
		int aWidth = (double)aButton->mButtonImage->GetCelWidth() >= aLabelWidth ? aButton->mButtonImage->GetCelWidth()
		                                                                           : Ftol(aLabelWidth);
		aButton->Resize(0, 0, aWidth, aButton->mButtonImage->GetCelHeight());
	}
	SetState(0, false);
}

ChallengeMenu::~ChallengeMenu()
{
	for (ChallengeButton* aButton : mButtons) {
		RemoveWidget(aButton);
		delete aButton;
	}
	RemoveWidget(mPlayMenuButton);
	delete mPlayMenuButton;
	RemoveWidget(mMainMenuButton);
	delete mMainMenuButton;
	RemoveWidget(mFXManager1B8);
	delete mFXManager1B8;
	delete mFXManager1BC;
	// port: the original never frees the planet picture
	delete mPlanetImage;
}

// 0x6dfc49: the planets go round a circle in the 3D world about the camera's start, from a quarter turn less half of
// what the circle leaves open (a tenth) and on, an equal step apart
void ChallengeMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	mMouseVisible = false;
	const double aTwoPi = 6.2831854820251465;  // the float at 0x84ec00
	float aSpan = (float)(aTwoPi * 0.8999999761581421);
	float aStart = (float)(aTwoPi * 0.25 - (aTwoPi - (double)aSpan) * 0.5);
	float aNegSpan = -aSpan;
	float aStep = (float)((double)aNegSpan / (double)((int)mButtons.size() - 1));
	mUnknown188 = (float)((double)mHeight * 0.4000000059604645);
	float anAngle = aStart;
	for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++) {
		ChallengeButton* aButton = mButtons[i];
		aButton->mUnknown524 = anAngle;
		aButton->mUnknown520 = aStart;
		aButton->mUnknown528 = mUnknown188;
		float aCos = std::cos(anAngle);
		aButton->mWorldPos[0] = (float)((double)aCos * 4.800000190734863 * 1.5 + (double)mCameraPos[0]);
		float aSin = std::sin(anAngle);
		aButton->mWorldPos[1] = (float)((double)aSin * 4.800000190734863 + (double)mCameraPos[1]);
		aButton->mWorldPos[2] = 0;
		AddWidget(aButton);
		aButton->Update();
		anAngle = (float)((double)aStep + (double)anAngle);
	}
	AddWidget(mPlayMenuButton);
	AddWidget(mMainMenuButton);
	// the two buttons wait below the screen (Update brings them up with mButtonSlide)
	mPlayMenuButton->mX = (mWidth - Ftol((double)gApp->mScale * 40.0)) - mPlayMenuButton->mWidth;
	mMainMenuButton->mX = Ftol(40.0 * (double)gApp->mScale);
	mPlayMenuButton->mY = mHeight * 2;
	mMainMenuButton->mY = mPlayMenuButton->mY;
	AddWidget(mFXManager1B8);
	mFXManager1BC->Resize(0, 0, mWidth, mHeight);
}

// 0x6dd2cd
void ChallengeMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	TwistWidget::RemovedFromManager(theWidgetManager);
	for (ChallengeButton* aButton : mButtons)
		RemoveWidget(aButton);
	RemoveWidget(mPlayMenuButton);
	RemoveWidget(mMainMenuButton);
	RemoveWidget(mFXManager1B8);
}

// 0x6e30d5
void ChallengeMenu::Show(int theMode, bool theNow)
{
	mUnknown9BC = false;
	mFXManager1BC->Clear();
	mVisible = true;
	UpdateProgress(false, false);
	SetState(theMode, theNow);
	if (!theNow) {
		for (ChallengeButton* aButton : mButtons)
			aButton->mMouseVisible = false;
	}
}

// 0x6dd42e
bool ChallengeMenu::IsOpenCurveRunning() const
{
	return 0.0 != mOpen.mIncRate && mOpen.mInMax != mOpen.mInVal;
}

// 0x6e3142: nothing happens while the open curve runs backwards
void ChallengeMenu::Close(int theState)
{
	if (IsOpenCurveRunning() && mOpen.mIncRate < 0.0)
		return;
	mMouseVisible = false;
	if (theState == 2)
		gApp->mMainMenu->PlayMusic(0);
	mMainMenuButton->mMouseVisible = false;
	mPlayMenuButton->mMouseVisible = false;
	for (ChallengeButton* aButton : mButtons) {
		if (theState == 2)
			aButton->mUnknown5C4 = false;
		aButton->mMouseVisible = false;
	}
	SetState(theState, false);
}

// 0x6e2318: the curves of the camera, the sun, the buttons and the moon for each state. All but the first state's are
// linked to the open curve (mOpen), which is what moves them. A state 1 or 2 menu flies the camera in or out; the camera's
// start for the zoom into a picked planet (mZoomPos) comes from the picked planet.
void ChallengeMenu::SetState(int theState, bool theNow)
{
	switch (theState) {
	case 0:
		// away
		mOpen.SetConstant(0.0);
		mButtonSlide.SetConstant(0.0);
		mStartDelay = 0.0f;
		gSunCurve.SetConstant(0.0);
		break;
	case 1: {
		// shown by the play menu: the camera flies in from the side
		gApp->mMainMenu->PlayMusic(0x130);
		mOpen.SetCurve(GTS(0x7e06a8), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e06a8), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e068c), nullptr);
		mUnknown6B0.SetCurve(GTS(0x7e07dc), nullptr);
		mUnknown640.SetCurve(GTS(0x7e0908), nullptr);
		if (theNow) {
			// at once: one step before the ends
			mOpen.mInVal = mOpen.mInMax - 0.0010000000474974513;
			mUnknown6B0.mInVal = mUnknown6B0.mInMax - 0.0010000000474974513;
			mUnknown640.mInVal = mUnknown640.mInMax - 0.0010000000474974513;
		}
		mCameraRotX.SetCurve(GTS(0x7e066c), &mOpen);
		mCameraRotY.SetCurve(GTS(0x7e0648), &mOpen);
		mCameraRotZ.SetCurve(GTS(0x7e0620), &mOpen);
		mCameraDistance.SetCurve(GTS(0x7e05fc), &mOpen);
		mCameraRight.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraUp.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraShiftX.SetCurve(GTS(0x7e05dc), &mOpen);
		mCameraShiftY.SetCurve(GTS(0x7e05bc), &mOpen);
		mUnknown720.SetCurve(GTS(0x7e06f0), &mOpen);
		mCameraTurn.SetCurve(GTS(0x7e06c4), &mOpen);
		// The turn the camera starts at (the three axes' curves at 0), turned 0.625 of the way to the identity, and
		// its transpose (0x4c1670 with the identity for the other, then 0x6dda5c): mCameraTarget
		Mat3 aRotation;
		aRotation.RotateZ(CurveRadians(mCameraRotZ));
		aRotation.RotateX(CurveRadians(mCameraRotX));
		aRotation.RotateY(CurveRadians(mCameraRotY));
		Mat3 anIdentity;
		mCameraTarget = SlerpMat3(aRotation, anIdentity, 0.625f, false).Transposed();
		break;
	}
	case 2:
		// leaving for a menu: the camera flies out the other way
		mOpen.SetCurve(GTS(0x7e0828), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e089c), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e07fc), &mOpen);
		mUnknown6B0.SetCurve(GTS(0x7e07dc), &mOpen);
		mUnknown640.SetCurve(GTS(0x7e0908), &mOpen);
		mCameraRotX.SetCurve(GTS(0x7e07bc), &mOpen);
		mCameraRotY.SetCurve(GTS(0x7e0798), &mOpen);
		mCameraRotZ.SetCurve(GTS(0x7e0798), &mOpen);
		mCameraDistance.SetCurve(GTS(0x7e0778), &mOpen);
		mCameraRight.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraUp.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraShiftX.SetCurve(GTS(0x7e0738), &mOpen);
		mCameraShiftY.SetCurve(GTS(0x7e0718), &mOpen);
		mUnknown720.SetCurve(GTS(0x7e06f0), &mOpen);
		mCameraTurn.SetCurve(GTS(0x7e06c4), &mOpen);
		break;
	case 3: {
		// leaving for a challenge: the camera flies to the picked planet
		float aWas = (float)mOpen.mInVal;
		mOpen.SetCurve(GTS(0x7e0940), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e089c), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e087c), &mOpen);
		mUnknown6B0.SetCurve(GTS(0x7e0858), &mOpen);
		mUnknown640.SetCurve(GTS(0x7e0908), &mOpen);
		mUnknown720.SetConstant(0.0);
		mCameraTurn.SetConstant(1.0);
		ChallengeButton* aPicked = mButtons[mCategory];
		mZoomPos[0] = aPicked->mWorldPos[0];
		mZoomPos[1] = aPicked->mWorldPos[1];
		mZoomPos[2] = aPicked->mWorldPos[2];
		mZoom.SetCurve(GTS(0x7e0940), &mOpen);
		for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++)
			mButtons[i]->mUnknown5C5 = i != (unsigned int)mCategory;
		if (mState == 4) {
			// the menu was still coming in: the way out begins where the way in stood
			float aLeft = (float)(1.0 - (double)aWas);
			if (0.00999999977648258f < aLeft)
				mOpen.mInVal = (double)aLeft;
		}
		break;
	}
	case 4: {
		// shown at once (after a game): the camera flies from the picked planet
		gApp->mMainMenu->PlayMusic(0x130);
		mVisible = true;
		mOpen.SetCurve(GTS(0x7e0940), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e0940), &mOpen);
		mStartDelay = 0.0f;
		if (mState != 5) {
			gSunCurve.SetCurve(GTS(0x7e0924), &mOpen);
			mUnknown640.SetCurve(GTS(0x7e0908), nullptr);
		}
		mUnknown6B0.SetCurve(GTS(0x7e08e4), &mOpen);
		mUnknown720.SetConstant(0.0);
		mCameraTurn.SetConstant(1.0);
		ChallengeButton* aPicked = mButtons[mCategory];
		mZoomPos[0] = aPicked->mWorldPos[0];
		mZoomPos[1] = aPicked->mWorldPos[1];
		mZoomPos[2] = aPicked->mWorldPos[2];
		mZoom.SetCurve(GTS(0x7e08c0), &mOpen);
		UpdateProgress(false, false);
		break;
	}
	case 5:
		// only shown: the sun's glow all the way
		gSunCurve.SetConstant(1.0);
		mVisible = true;
		break;
	}
	mState = theState;
	MarkDirty();
}

// 0x6e5a8d: properties/camerastack.txt, two lines of three numbers each for each camera (a blank line between)
void ChallengeMenu::LoadCameraStack()
{
	mCameraStack.clear();
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile("properties/camerastack.txt", &aBuffer, true))
		return;
	// the lines (the original reads them with fgets in text mode)
	std::vector<std::string> aLines;
	std::string aLine;
	for (unsigned char aChar : aBuffer.mData) {
		if (aChar == '\n') {
			aLines.push_back(aLine);
			aLine.clear();
		} else if (aChar != '\r') {
			aLine += (char)aChar;
		}
	}
	if (!aLine.empty())
		aLines.push_back(aLine);
	for (size_t i = 0; i < aLines.size(); i++) {
		if (Trim(aLines[i]).empty())
			continue;
		CameraStackEntry anEntry = {{0, 0, 0}, {0, 0, 0}};
		std::sscanf(aLines[i].c_str(), "%f,%f,%f\n", &anEntry.mPos[0], &anEntry.mPos[1], &anEntry.mPos[2]);
		i++;
		if (i < aLines.size())
			std::sscanf(aLines[i].c_str(), "%f,%f,%f\n", &anEntry.mRot[0], &anEntry.mRot[1], &anEntry.mRot[2]);
		mCameraStack.push_back(anEntry);
	}
}

// 0x6e0ddb: each planet's stage, whether it is open and its curves, from the player's progress. Animating, a planet
// that moves on or opens starts the curves that show it (a glow, a sound) and the call returns 200 or 250; not
// animating, the curves are put at their end.
int ChallengeMenu::UpdateProgress(bool theAnimate, bool theClearTips)
{
	bool aAdvanced = false;
	bool anOpened = false;
	ChallengeInfo* anInfo = gApp->mChallengeInfo;
	Profile* aProfile = gApp->mProfile;
	for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++) {
		ChallengeButton* aButton = mButtons[i];
		int aStage = aProfile->GetChallengeStage((int)i);
		aButton->mEclipseFade.SetConstant(0.0);
		aButton->mTextFade.SetConstant(1.0);
		if (theClearTips && aButton->mTip != nullptr) {
			// the tip (0x6e665e, piece 3): none is made yet
			gApp->SafeDeleteWidget(aButton->mTip);
			aButton->mTip = nullptr;
		}
		bool aEclipseNow = aButton->mEclipse;
		if (!aEclipseNow) {
			if (aButton->mStage != aStage) {
				int aCount = anInfo->GetChallengeCount(i);
				if (aCount < aStage) {
					// every challenge done: the planet shows its last stage and the eclipse that follows
					aButton->mBarFill.SetCurve(GTS(0x7e0558));
					aButton->mOpen = true;
					aButton->mDim.SetConstant(0.0);
					aButton->mStage = anInfo->GetChallengeCount(i);
					if (theAnimate) {
						aAdvanced = true;
						aButton->mUnknown398.SetCurve(GTS(0x7e0534));
						aButton->mEclipseFade.SetCurve(GTS(0x7e050c));
						aButton->mTextFade.SetCurve(GTS(0x7e04ec));
					} else {
						aEclipseNow = true;
					}
				} else if (!theAnimate || aStage == 1) {
					aButton->mBarFill.SetConstant(0.0);
					aButton->mUnknown1D8.SetConstant(0.0);
					aButton->mUnknown248.SetConstant(0.0);
					aButton->mUnknown2B8.SetConstant(0.0);
					aButton->mStage = aStage;
				} else {
					// the planet moves on a stage: the bar fills, the new number glows and a sound plays
					aAdvanced = true;
					gApp->PlaySample(SOUND_CHALLENGE_UNLOCK);
					aButton->mBarFill.SetCurve(GTS(0x7e0558));
					aButton->mUnknown1D8.SetCurve(GTS(0x7e04c4));
					if (aStage < 4 || gApp->mRegistered) {
						aButton->mUnknown248.SetCurve(GTS(0x7e045c));
						aButton->mUnknown2B8.SetCurve(GTS(0x7e0420));
					} else {
						aButton->mUnknown2B8.SetCurve(GTS(0x7e0488));
					}
				}
			}
		} else {
			aEclipseNow = true;
		}
		if (aEclipseNow) {
			// the eclipse (also reached from the first stage past the last, not animating)
			aButton->mUnknown398.SetConstant(1.0);
			aButton->mEclipseFade.SetConstant(1.0);
			aButton->mTextFade.SetConstant(0.0);
		}
		aButton->mProgress = (int)i < (int)aProfile->mInts204.size() ? aProfile->mInts204[i] : 0;
		aButton->mEclipse = anInfo->GetChallengeCount(i) < aProfile->GetChallengeStage((int)i);
		bool anOpen = aProfile->IsChallengeCategoryOpen((int)i);
		if (!anOpen) {
			aButton->mOpen = false;
			aButton->mUnknown398.SetConstant(0.0);
			aButton->mDim.SetConstant(1.0);
			aButton->mMouseVisible = false;
		} else if (!aButton->mOpen) {
			if (!theAnimate) {
				aButton->mDim.SetConstant(0.0);
				aButton->mOpen = anOpen;
			} else {
				anOpened = true;
				aButton->mDim.SetCurve(GTS(0x7e03f4));
				aButton->mUnknown5C4 = true;
			}
			aButton->mMouseVisible = true;
		}
	}
	if (mHover >= 0)
		ButtonMouseEnter(mHover);
	if (anOpened)
		return 0xFA;
	return aAdvanced ? 200 : 0;
}

// 0x6e3f41
void ChallengeMenu::Update()
{
	TwistWidget::Update();
	gSunCurve.IncInVal();
	mUnknown6B0.IncInVal();
	mUnknown640.IncInVal();
	// 1 - mUnknown6B0, but not below 0.25
	float aFraction = 0.25f;
	if (0.25 <= 1.0 - mUnknown6B0.GetOutVal())
		aFraction = (float)(1.0 - mUnknown6B0.GetOutVal());
	gUnknown85C508 = aFraction;
	if (mHover == -1) {
		mHoverFade = (float)((double)mHoverFade - 0.03999999910593033);
		if (mHoverFade < 0.0f)
			mHoverFade = 0.0f;
		mUnknown9D0 = (float)((double)mUnknown9D0 + 0.009999999776482582);
		if (0.5f < mUnknown9D0) {
			mUnknown9D4 = (float)((double)mUnknown9D4 + 0.019999999552965164);
			if (1.0f < mUnknown9D4)
				mUnknown9D4 = 1.0f;
		}
	} else {
		mHoverFade = (float)((double)mHoverFade + 0.02500000037252903);
		if (1.0f < mHoverFade)
			mHoverFade = 1.0f;
		mUnknown9D0 = 0;
		mUnknown9D4 = 0;
	}
	// The glow colours follow mUnknown928 (0x7570c1 blends two colours, the first's alpha kept)
	mUnknown928.IncInVal();
	float aBlend = (float)mUnknown928.GetOutVal();
	gSunGlowColor = BlendColors(Color(255, 255, 255), Color(255, 128, 255), aBlend);
	aBlend = (float)mUnknown928.GetOutVal();
	gUnknown85C50C = BlendColors(Color(128, 32, 128), Color(0, 0, 128), aBlend);
	mFXManager1BC->Update();
	// The mouse drag changes the camera stack's entry mCameraStackIndex: sideways it turns about y (the right button,
	// about z), up and down about x
	WidgetManager* aWidgetManager = gApp->mWidgetManager;
	int aMouseX = aWidgetManager->mLastMouseX;
	int aMouseY = aWidgetManager->mLastMouseY;
	if ((aWidgetManager->mDownButtons & 1) == 0) {
		mDragging = false;
	} else if (!mDragging) {
		mDragStartX = aMouseX;
		mDragStartY = aMouseY;
		mDragging = true;
	} else {
		int aDeltaX = aMouseX - mLastMouseX;
		int aDeltaY = aMouseY - mLastMouseY;
		// (with no camera stack the original would stop on its bounds check; the file ships with the game)
		if ((size_t)mCameraStackIndex < mCameraStack.size()) {
			CameraStackEntry& anEntry = mCameraStack[mCameraStackIndex];
			if ((aWidgetManager->mDownButtons & 2) == 0) {
				anEntry.mRot[1] = (float)((double)aDeltaX * 0.0020000000949949026 + (double)anEntry.mRot[1]);
				anEntry.mRot[0] = (float)((double)aDeltaY * 0.0020000000949949026 + (double)anEntry.mRot[0]);
			} else {
				anEntry.mRot[2] = (float)((double)aDeltaX * 0.0020000000949949026 + (double)anEntry.mRot[2]);
			}
		}
	}
	mLastMouseX = aMouseX;
	mLastMouseY = aMouseY;
	if (IsOpenCurveRunning()) {
		if (0.0f < mStartDelay) {
			mStartDelay = (float)((double)mStartDelay - 0.009999999776482582);
		} else if (!mOpen.IncInVal()) {
			// the curve has ended
			mUnknown9D4 = 0;
			mHoverFade = 0;
			bool anIn = mOpen.GetOutVal() == 1.0;
			mMouseVisible = anIn;
			mMainMenuButton->mMouseVisible = anIn;
			mPlayMenuButton->mMouseVisible = anIn;
			if (mOpen.GetOutVal() == 1.0 && (mState == 1 || mState == 4))
				mUnknown640.SetCurve(GTS(0x7e0b40), nullptr);
			for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++) {
				if (mState == 1)
					mButtons[i]->mUnknown5C4 = mMouseVisible && mButtons[i]->mOpen;
				mButtons[i]->mMouseVisible = mMouseVisible && mButtons[i]->mOpen;
			}
		}
	}
	// The camera (a Transform3D, kept for the planets' projection): states 1 and 2 follow the camera curves, the others
	// fly from the start to the picked planet (mZoom); state 5 leaves it as it is
	if (mState != 5) {
		Transform3D aCamera;
		if (mState == 1 || mState == 2) {
			aCamera.mRot.RotateZ(CurveRadians(mCameraRotZ));
			aCamera.mRot.RotateX(CurveRadians(mCameraRotX));
			aCamera.mRot.RotateY(CurveRadians(mCameraRotY));
			float aDistance = (float)(mCameraDistance.GetOutVal() + 16.149999618530273);
			Vec3 aRow0 = aCamera.mRot.Row(0);
			Vec3 aRow1 = aCamera.mRot.Row(1);
			Vec3 aRow2 = aCamera.mRot.Row(2);
			// the start, a distance along the third axis, some along the first and the second, and the shifts
			float aX = aRow2.x * aDistance + mCameraPos[0];
			float aY = mCameraPos[1] + aRow2.y * aDistance;
			float aZ = mCameraPos[2] + aRow2.z * aDistance;
			float aRight = (float)mCameraRight.GetOutVal();
			aCamera.mPos.x = aRow0.x * aRight + aX;
			aCamera.mPos.y = aRow0.y * aRight + aY;
			aCamera.mPos.z = aRow0.z * aRight + aZ;
			float anUp = (float)mCameraUp.GetOutVal();
			aCamera.mPos.x = aRow1.x * anUp + aCamera.mPos.x;
			aCamera.mPos.y = aRow1.y * anUp + aCamera.mPos.y;
			aCamera.mPos.z = aRow1.z * anUp + aCamera.mPos.z;
			float aShiftX = (float)mCameraShiftX.GetOutVal();
			aCamera.mPos.x = aShiftX + aCamera.mPos.x;
			float aShiftY = (float)mCameraShiftY.GetOutVal();
			aCamera.mPos.y = aShiftY + aCamera.mPos.y;
		} else {
			float aFly = (float)mZoom.GetOutVal();
			float aFromX = (float)((double)mCameraPos[0] + 0.0);
			float aFromY = (float)((double)mCameraPos[1] + 0.0);
			float aFromZ = (float)((double)mCameraPos[2] + 16.149999618530273);
			float aToX = (float)((double)mZoomPos[0] + 0.0);
			float aToY = (float)((double)mZoomPos[1] + 0.15000000596046448);
			float aToZ = (float)((double)mZoomPos[2] + 7.5);
			aCamera.mPos.x = (float)(((double)aToX - (double)aFromX) * (double)aFly + (double)aFromX);
			aCamera.mPos.y = (float)(((double)aToY - (double)aFromY) * (double)aFly + (double)aFromY);
			aCamera.mPos.z = (float)(((double)aToZ - (double)aFromZ) * (double)aFly + (double)aFromZ);
		}
		gMenuCamera = aCamera;
	}
	// The hyperspace's 3D camera turns from the identity to the menu's by mCameraTurn
	if (gApp->mHyperspace != nullptr) {
		float aTurn = (float)mCameraTurn.GetOutVal();
		Mat3 anIdentity;
		gApp->mHyperspace->mUnknown1548 = SlerpMat3(anIdentity, mCameraTarget, aTurn, false);
	}
	// The buttons rise with mButtonSlide (0x34 is the buttons' y)
	mPlayMenuButton->mY = Ftol(((1.0 - mButtonSlide.GetOutVal()) * 0.5 + 0.9250000119209290) * (double)mHeight);
	mMainMenuButton->mY = Ftol(((1.0 - mButtonSlide.GetOutVal()) * 0.5 + 0.9250000119209290) * (double)mHeight);
	// The glow's particles ride down with the open curve
	double anOpen = mOpen.GetOutVal();
	int aGlowShift = Ftol((double)gApp->mScale * 1500.0);
	mFXManager1BC->mY = Ftol((double)aGlowShift * (1.0 - anOpen));
	mFXManager1BC->mAlpha = (mState == 1 || mState == 4 || mState == 5) ? (float)gSunCurve.GetOutVal() : 0.0f;
	gAspect = (float)((double)gApp->mDDInterface->mWidth / (double)gApp->mDDInterface->mHeight);
	gAspect45 = (float)((double)gAspect * 45.0);
	// The sun's glow throws particles about while the menu is all the way in
	if (mStarted == 0 && mOpen.GetOutVal() == 1.0 && (mState == 1 || mState == 4)) {
		float aRadius = (float)((double)IMAGE_ECLIPSE_SUN->mWidth * 0.30000001192092896);
		int aCount;
		if (gApp->Is3DAccelerated()) {
			NotPorted("ChallengeMenu::Update 0x6e47a2: two glow particles each update with Direct3D");
			aCount = 0;
		} else {
			aCount = (int)(Rand() % 100) / 80;
		}
		while (0 < aCount) {
			Effect* anEffect = mFXManager1BC->AllocEffect(0);
			anEffect->mImage = GetImageById((Rand() & 1) == 0 ? IMAGE_ECLIPSE_FLARE1_ID : IMAGE_ECLIPSE_FLARE2_ID);
			float aSpeed = (float)((double)gApp->mScale * 0.30000001192092896);
			aSpeed = (float)std::fabs(RandSigned() * aSpeed);
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			anEffect->mX = (float)((double)(std::cos(anAngle) * aRadius) + (double)(mWidth / 2));
			anEffect->mY = (float)((double)(std::sin(anAngle) * aRadius) + (double)(mHeight / 2));
			anEffect->mDX = std::cos(anAngle) * aSpeed;
			anEffect->mDY = std::sin(anAngle) * aSpeed;
			anEffect->mAlpha = 0;
			anEffect->mDAlpha = 0.0024999999441206455f;
			anEffect->mMaxAlpha = 0.25f;
			anEffect->mFlags |= Effect::FLAG_ALPHA_BOUNCE;
			anEffect->mScale = (float)((double)RandSigned() * 0.10000000149011612 + 0.5);
			anEffect->mDScale = 0.004999999888241291f;
			anEffect->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
			anEffect->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.0024999999441206455);
			int aGrey = (int)(Rand() % 15) + 240;
			anEffect->mColor = Color(aGrey, aGrey, aGrey);
			anEffect->mIsAdditive = false;
			mFXManager1BC->AddEffect(anEffect);
			aCount--;
		}
	}
	if (0.0f < mUnknown15C) {
		float aLeft = (float)((double)mUnknown15C - 0.029999999329447746);
		mUnknown15C = 0.0f < aLeft ? aLeft : 0.0f;
	}
	// 0x6e4989: the end-of-challenge message at +0x9dc. At its update count 250 the rank bar's timer gains the progress
	// update's result over 100; once clicked it goes, and the menu goes from state 5 to state 4
	if (mAwardTelly != nullptr && mAwardTelly->mUpdateCnt == 250) {
		int aResult = UpdateProgress(true, false);
		mAwardTelly->mRankBonus->mTimer = (float)((double)(float)aResult / 100.0 + (double)mAwardTelly->mRankBonus->mTimer);
	}
	if (mAwardTelly != nullptr && mAwardTelly->mClicked) {
		gApp->mWidgetManager->RemoveWidget(mAwardTelly);
		gApp->SafeDeleteWidget(mAwardTelly);
		mAwardTelly = nullptr;
		if (mState == 5)
			SetState(4, false);
	}
	MarkDirty();
}

// 0x6dd322: the sun's glow particles, and a picture behind each of the two buttons. The sun, the moon and the eclipse
// disc (0x6e132d), the tips and the zoom are piece 3.
void ChallengeMenu::Draw(Graphics* g)
{
	TwistWidget::Draw(g);
	mFXManager1BC->Draw(g);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColor(Color::White);
	Image* aGraphic = IMAGE_PLAYMENU_MMGRAPHIC;
	int aShift = Ftol((double)gApp->mScale * -5.0);
	int aY = ((mPlayMenuButton->mHeight / 2 - aShift) - aGraphic->mHeight / 2) + mPlayMenuButton->mY;
	g->DrawImage(aGraphic, (mPlayMenuButton->mWidth / 2 - aGraphic->mWidth / 2) + mPlayMenuButton->mX, aY);
	g->DrawImage(aGraphic, (mMainMenuButton->mWidth / 2 - aGraphic->mWidth / 2) + mMainMenuButton->mX, aY);
}

// 0x6dfe2a: the camera's zoom
void ChallengeMenu::MouseWheel(int theDelta)
{
	(void)theDelta;
}

// 0x6e54c3
void ChallengeMenu::ButtonDepress(int theId)
{
	int aSound = SOUND_BUTTONPRESS2;
	if (theId == BUTTON_PLAY_MENU) {
		Close(2);
		gApp->ShowModeSelect(0);
		PlayButton* aPlayButton = gApp->mMainMenu->mPlayButton;
		aPlayButton->mAppear.SetCurve(GTS(0x7e0bb0));
		aPlayButton->mAlpha.SetCurve(GTS(0x7e0b88));
	} else if (theId == BUTTON_MAIN_MENU) {
		Close(2);
		gApp->ShowMainMenu(false, false, 0.0f);
	} else if (!mUnknown9BC) {
		ChallengeButton* aButton = mButtons[theId];
		mCategory = aButton->mCategory;
		mStageSel = aButton->mStage;
		if (aButton->mEclipse)
			mStageSel++;
		if (!gApp->mRegistered && (mStageSel > 3 || mCategory > 2)) {
			// The trial: the curve at +0x8a0 (0x6e54c3, whose listener-relative +0x758 it is) starts, and the click sound
			mUnknown8A0.SetCurve(GTS(0x7e0c70));
			aSound = SOUND_TRIALCLICK;
		} else if (!gApp->SaveExists(2, mCategory)) {
			Start(true);
		} else {
			TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, 0, true, GT(0x80e8a4),
				gApp->GetString(GTS(0x7e0c44), GT(0x7e0bf0)), GT(0x80d9a4), 7);
			aDialog->mYesLabel = gApp->GetString(GTS(0x80f054), GT(0x7e0bdc));
			aDialog->mNoLabel = gApp->GetString(GTS(0x7e0bd8), GT(0x7e0bd0));
			aDialog->PopUp();
			aDialog->mTwistListener = this;
			double aMinWidth = (double)gGemWidth * 6.75;
			double aWidth = (double)aDialog->GetPreferredWidth() <= aMinWidth ? aMinWidth : (double)aDialog->GetPreferredWidth();
			aDialog->mWidth = Ftol(aWidth);
			aDialog->mHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
			gApp->AddDialog(aDialog);
			aDialog->mX = (mWidth / 2 - aDialog->mWidth / 2) + mX;
			aDialog->mY = (mHeight / 2 - aDialog->mHeight / 2) + mY;
		}
	}
	gApp->PlaySample(aSound);
}

// 0x6e58f7: Continue goes on with the saved game, the other button gives it up
void ChallengeMenu::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId == 0) {
		bool aNewGame = true;
		if (theButtonId == Dialog::ID_YES) {
			aNewGame = false;
		} else if (theButtonId == Dialog::ID_NO) {
			aNewGame = true;
			gApp->DeleteSave(2, mCategory);
		}
		Start(aNewGame);
	}
	gApp->KillDialog(theDialogId);
}

// 0x6e665e: the planet's text shows (the original builds its two pictures here)
void ChallengeMenu::ButtonMouseEnter(int theId)
{
	if (!mMouseVisible || theId >= 100)
		return;
	mHover = theId;
}

// 0x6dd418
void ChallengeMenu::ButtonMouseLeave(int theId)
{
	if (mHover == theId)
		mHover = -1;
}

// 0x6e4ab6
void ChallengeMenu::Start(bool theNewGame)
{
	if (gApp->mHyperspace == nullptr)
		gApp->ShowHyperspace();
	gApp->NewGame(2, theNewGame, mCategory);
	if (theNewGame)
		gBoard->SetChallenge(gApp->mChallengeInfo->GetChallenge(mCategory, mStageSel));
	mStarted = 1;
	mMainMenuButton->mMouseVisible = false;
	mPlayMenuButton->mMouseVisible = false;
	mMouseVisible = false;
	// (the planet's place in the 3D world, at +0x5b4, aims the 3D camera: unused in 2D)
	gApp->mBoard->mHyperspace->StartChallenge();
	gApp->mBoard->SetUnknown3924(0.0f);
	gApp->mBoard->ApplyUnknown3924();
	Close(3);
}
