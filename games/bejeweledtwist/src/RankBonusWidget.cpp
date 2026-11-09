#include "RankBonusWidget.h"
#include "TwistD3D.h"
#include "SexyMatrix.h"

#include "AwardTelly.h"
#include "Board.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Image.h"
#include "LevelUpShow.h"
#include "Profile.h"
#include "Ranks.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstring>

using namespace Sexy;

namespace {

// 0x736550: theValue to the power theExp by squaring (std::pow(double, int))
double PowInt(double theValue, int theExp)
{
	unsigned int anExp = theExp < 0 ? (unsigned int)-theExp : (unsigned int)theExp;
	double aResult = 1.0;
	for (;;) {
		if (anExp & 1)
			aResult *= theValue;
		anExp >>= 1;
		if (anExp == 0)
			break;
		theValue *= theValue;
	}
	return theExp < 0 ? 1.0 / aResult : aResult;
}

// The cel a star of theWidth shows: the narrower, the later the cel (0 to 9)
int StarCel(Image* theImage, double theWidth)
{
	double aCel = (1.0 - theWidth / (double)theImage->GetCelWidth()) * 10.0;
	if (aCel < 0.0)
		aCel = 0.0;
	else if (9.0 < aCel)
		aCel = 9.0;
	return Ftol(aCel);
}

}  // namespace

// 0x7ea230: the badge for every five ranks; 0x7ea208: the mark for the ranks between
const int Sexy::kRankBadges[21] = { IMAGE_BADGES_00_ID, IMAGE_BADGES_05_ID, IMAGE_BADGES_10_ID, IMAGE_BADGES_15_ID,
	IMAGE_BADGES_20_ID, IMAGE_BADGES_25_ID, IMAGE_BADGES_30_ID, IMAGE_BADGES_35_ID, IMAGE_BADGES_40_ID,
	IMAGE_BADGES_45_ID, IMAGE_BADGES_50_ID, IMAGE_BADGES_55_ID, IMAGE_BADGES_60_ID, IMAGE_BADGES_65_ID,
	IMAGE_BADGES_70_ID, IMAGE_BADGES_75_ID, IMAGE_BADGES_80_ID, IMAGE_BADGES_85_ID, IMAGE_BADGES_90_ID,
	IMAGE_BADGES_95_ID, IMAGE_BADGES_100_ID };
const int Sexy::kRankBadgeMarks[10] = { IMAGE_BADGES_00_ID, IMAGE_BADGES_01_ID, IMAGE_BADGES_02_ID, IMAGE_BADGES_03_ID,
	IMAGE_BADGES_04_ID, IMAGE_BADGES_05_ID, IMAGE_BADGES_06_ID, IMAGE_BADGES_07_ID, IMAGE_BADGES_08_ID,
	IMAGE_BADGES_09_ID };


RankBonusWidget::RankBonusWidget(int thePoints, int theBonus, int theX, int theY)
{
	mBonus = theBonus;
	mStarsX = theX;
	mStarsY = theY;
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mPoints = thePoints;
	mTelly = nullptr;
	mTotal = mBonus + thePoints;
	mStarsAdded = 0;
	mBar = IMAGE_RANKBAR_LEVELUP;
	mClip = false;
	mRank = gApp->mProfile->mRank;
	mSlots = gApp->mRanks->GetPoints(mRank + 1) - gApp->mRanks->GetPoints(mRank);
	int aFilled = mSlots;
	if (thePoints - gApp->mRanks->GetPoints(mRank) <= aFilled)
		aFilled = thePoints - gApp->mRanks->GetPoints(mRank);
	mPulseLeft = 0;
	mFilled = aFilled;
	mFocus = aFilled;
	mTarget = mBonus + aFilled;
	mFXManager = new EffectsManager();
	mStarImage = IMAGE_RANKBAR_STAR;
	mStarWidth = Ftol(mStarImage->mWidth * 1.100000023841858);
	mScale = 0;
	mStarHeight = Ftol(mStarImage->mHeight * 1.100000023841858);
	mBarWidth = Ftol(mBar->mWidth * 0.4749999940395355);
	mFilledFlash = 0;
	mLastFilled = -1;
	mOldWidths = new float[mSlots];
	mWidths = new float[mSlots];
	mWantWidths = new float[mSlots];
	memset(mWantWidths, 0, mSlots * sizeof(float));
	memset(mWidths, 0, mSlots * sizeof(float));
	memset(mOldWidths, 0, mSlots * sizeof(float));
	Layout();
	mBlend = 1.0f;
	BlendWidths();

	// The stars wait in a row about theX, theY, at most 66 apart
	unsigned int aCount = (unsigned int)mBonus;
	unsigned int aWide = aCount <= 50 ? aCount : 50;
	float aCountF = (float)aCount;
	float aGap = (float)Ftol(((double)aWide * 0.006500000134110451 + 0.23000000417232513) * (double)gApp->mWidth) / aCountF;
	float aMax = (float)Ftol((double)gApp->mScale * 66.0);
	if (!(aMax > aGap))
		aGap = aMax;
	float aStarX = (float)((double)mStarsX - (double)aCountF * (double)aGap * 0.5);
	float aStarY = (float)mStarsY;
	if (aCount != 0) {
		int aY = Ftol(aStarY);
		double aWidth = (double)aGap * 1.5;
		unsigned int i = 0;
		do {
			Star aStar;
			aStar.mX = Ftol(aStarX);
			aStar.mY = aY;
			aStar.mToX = aStar.mX;
			aStar.mToY = aY;
			aStar.mCel = StarCel(IMAGE_STARBAR_STARS_GOLDSTARS, aWidth);
			aStar.mStartCel = aStar.mCel;
			aStar.mProgress = 0;
			aStar.mAlpha = 1.0f;
			mStars.push_back(aStar);
			i++;
			aStarX = aStarX + aGap;
		} while (i < (unsigned int)mBonus);
	}
	mFocusX = mWidth / 2;
	gApp->mProfile->mRankPoints += theBonus;
	mMouseVisible = false;
}

RankBonusWidget::~RankBonusWidget()
{
	RemoveWidget(mFXManager);
	delete mFXManager;
	delete[] mOldWidths;
	delete[] mWidths;
	delete[] mWantWidths;
	RemoveAllWidgets(true, false);
}

void RankBonusWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	(void)theX; (void)theY; (void)theWidth; (void)theHeight;
	Widget::Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mCenterX = mWidth / 2;
	mCenterY = Ftol((double)(mHeight / 2) + (double)mBarY);
}

void RankBonusWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	AddWidget(mFXManager);
	Reset();
}

void RankBonusWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	RemoveWidget(mFXManager);
	mClicked = false;
}

void RankBonusWidget::Reset()
{
	mBarY = (float)gApp->mHeight;
	int aStep = mBonus / 20;
	mVel = 3.0f;
	mFadeIn = 0;
	mFadeIn2 = 1.0f;
	mBarScale = 1.0f;
	mIsFinished = 0;
	mState = 0;
	if (aStep < 1)
		aStep = 1;
	mStep = aStep;
}

void RankBonusWidget::MouseDown(int x, int y, int theClickCount)
{
	(void)x; (void)y; (void)theClickCount;
	if (1 < mState) {
		gApp->PlaySample(SOUND_RANK_COMPLETION);
		mState = 5;
	}
}

void RankBonusWidget::RankUp()
{
	gApp->mProfile->mRank++;
	gApp->mProfile->Save();
	AwardTelly* aTelly = new AwardTelly();
	aTelly->mZOrder = mZOrder + 1;
	aTelly->ShowRank(gApp->mProfile->mRank);
	mTelly = aTelly;
	if (gBoard != nullptr && gBoard->mHyperspace != nullptr)
		gBoard->mHyperspace->mHoldsJump = true;
	AddWidget(mTelly);
}

void RankBonusWidget::Layout()
{
	mBlend = 0;
	memcpy(mOldWidths, mWidths, mSlots * sizeof(float));
	memset(mWantWidths, 0, mSlots * sizeof(float));
	memset(mWidths, 0, mSlots * sizeof(float));
	int aWidest = Ftol((double)Ftol((double)gApp->mScale * 66.0) * 0.8500000238418579);
	float* aWidths = new float[mSlots];
	float aSum = 0;
	int i = 0;
	if (0 < mSlots) {
		do {
			double aShare = PowInt(0.75, std::abs(i - mFocus + 1));
			if (aShare < 0.0)
				aShare = 0.0;
			else if (1.0 < aShare)
				aShare = 1.0;
			aWidths[i] = (float)(aShare * aWidest);
			i++;
			aSum = (float)(aShare * aWidest) + aSum;
		} while (i < mSlots);
	}
	int aCount = mSlots;
	float aSpare = (float)mBarWidth - aSum;
	i = 0;
	float aX = (float)(mWidth / 2 - mBarWidth / 2);
	float aPrev = 0;
	if (0 < aCount) {
		do {
			mWantWidths[i] = aWidths[i] + aSpare / (float)aCount;
			if (i == mFocus)
				mFocusX = Ftol(aX);
			float aWidth = mWantWidths[i];
			i++;
			aX = (float)((double)aPrev * 0.5 + (double)aWidth * 0.5 + (double)aX);
			aPrev = aWidth;
		} while (i < mSlots);
	}
	BlendWidths();
	delete[] aWidths;
}

void RankBonusWidget::BlendWidths()
{
	int i = 0;
	if (0 < mSlots) {
		do {
			if (mOldWidths == nullptr)
				mWidths[i] = mWantWidths[i];
			else
				mWidths[i] = mWantWidths[i] * mBlend + (1.0f - mBlend) * mOldWidths[i];
			i++;
		} while (i < mSlots);
	}
}

void RankBonusWidget::UpdateFades()
{
	if (mBlend < 1.0f) {
		float aBlend = (float)((double)mBlend + 0.05000000074505806);
		mBlend = aBlend;
		if (1.0f < aBlend)
			mBlend = 1.0f;
		BlendWidths();
		MarkDirty();
	}
	if (0.0f < mFilledFlash) {
		float aFlash = (float)((double)mFilledFlash - 0.05999999865889549);
		if (aFlash < 0.0f)
			aFlash = 0.0f;
		mFilledFlash = aFlash;
	}
}

void RankBonusWidget::Update()
{
	TwistWidget::Update();
	if (gBoard == nullptr || gBoard->mLevelUpShow == nullptr) {
		double aScale = (double)mScale + 0.009999999776482582;
		if (1.0 < aScale)
			aScale = 1.0;
		mScale = (float)aScale;
	} else {
		// The level's end: the LevelUpShow's zoom
		double aScale = static_cast<LevelUpShow*>(gBoard->mLevelUpShow)->mScaleCurve.GetOutVal() * 1.0499999523162842;
		if (1.0 < aScale)
			aScale = 1.0;
		mScale = (float)aScale;
	}
	if (mState < 5) {
		float aFade = (float)((double)mFadeIn + 0.019999999552965164);
		if (1.0f < aFade)
			aFade = 1.0f;
		mFadeIn = aFade;
		mFadeIn2 = aFade;
	}
	if (!mFlash.IsInitialized()) {
		unsigned int i = 0;
		if (mStars.size() != 0) {
			do {
				Star* aStar = &mStars[i];
				double anAlpha = (double)aStar->mAlpha + 0.10000000149011612;
				if (1.0 < anAlpha)
					anAlpha = 1.0;
				aStar->mAlpha = (float)anAlpha;
				if (0.0f < aStar->mProgress) {
					float aProgress = (float)((double)aStar->mProgress + 0.05000000074505806);
					aStar->mProgress = aProgress;
					int aCel = aStar->mStartCel - Ftol((double)aProgress * 9.0);
					if (aCel < 0)
						aCel = 0;
					aStar->mCel = aCel;
					if (1.0f <= aProgress) {
						// In its slot: the slot fills
						mStars.erase(mStars.begin() + i);
						mFilledFlash = 1.0f;
						i--;
						mLastFilled = mFilled;
						mFilled++;
						if (mSlots <= mFilled && !mFlash.IsInitialized()) {
							mFlash.SetCurve(GTS(0x7ea180));
							mPillars.SetCurve(GTS(0x7ea158));
							mSlide.SetCurve(GTS(0x7ea12c));
							mGlow.SetCurve(GTS(0x7ea0f4));
							mStarFade.SetCurve(GTS(0x7e0bb0));
							mFade.SetCurve(GTS(0x7e0bb0));
							gApp->PlaySample(SOUND_LIGHTBAR_BLINK);
						}
						float aPitch = (float)((double)mFilled / (double)mSlots * 6.0);
						gApp->PlaySoundEx(SOUND_GEODE, 0, aPitch, 0.0f, true, true);
					}
				}
				i++;
			} while (i < mStars.size());
		}
	}
	switch (mState) {
	case 0:
		// It rises until its top is a bar's height from the bottom
		if (1.0f <= mScale) {
			mBarY = mBarY - mVel;
			mVel = (float)((double)mVel + 0.20000000298023224);
			float aTop = (float)(mHeight - mBar->mHeight);
			if (mBarY < aTop) {
				mState = 1;
				mBarY = aTop;
				mVel = 0;
				mTimer = 1.5f;
			}
		}
		break;
	case 1:
		mScale = 1.0f;
		if (0.0f < mTimer) {
			mTimer = (float)((double)mTimer - 0.009999999776482582);
		} else if (mSlide.CheckInThreshold(0.8500000238418579)) {
			RankUp();
			mState = 3;
		} else if (!mClicked && mUpdateCnt % 15 == 0) {
			if (mStars.size() != 0) {
				// The next waiting star starts for the focused slot
				int i = 0;
				if (0 < (int)mStars.size()) {
					do {
						if (mStars[i].mProgress == 0.0f) {
							mFocus++;
							Layout();
							mStars[i].mProgress = 0.01f;
							int aStarWidth = Ftol((double)gApp->mScale * 66.0);
							mStars[i].mToX = mFocusX - aStarWidth / 2;
							mStars[i].mToY = Ftol((double)mHeight * 0.8799999952316284);
							break;
						}
						i++;
					} while (i < (int)mStars.size());
				}
				mPulseLength = 3.0f;
				mPulseLeft = 1.0f;
			} else if (!mSlide.IsInitialized()) {
				mState++;
				mTimer = gBoard != nullptr ? 1.75f : 1.0f;
			}
		}
		break;
	case 2:
		mScale = 1.0f;
		mIsWaiting = true;
		mTimer = (float)((double)mTimer - 0.009999999776482582);
		if (!(0.0f < mTimer))
			mState = 5;
		break;
	case 3:
		if (mTelly->mState == 3) {
			mPillars.SetCurve(GTS(0x7ea0cc));
			mState = 4;
		}
		break;
	case 4:
		if (mPillars.HasBeenTriggered())
			mState = 5;
		break;
	case 5: {
		// It falls off the bottom, the award with it
		int aWas = Ftol(mBarY);
		mBarY = mVel + mBarY;
		mVel = (float)((double)mVel + 0.20000000298023224);
		if (mTelly != nullptr && mTelly->mButton != nullptr)
			mTelly->mButton->mY += Ftol(mBarY) - aWas;
		float aLeft = (float)((double)mPulseLeft - 0.019999999552965164);
		mPulseLeft = aLeft;
		if (!(0.0f < aLeft))
			mPulseLeft = 0;
		if ((float)mHeight <= mBarY && !(0.0f < mPulseLeft)) {
			mPulseLeft = 0;
			if (gBoard != nullptr && gBoard->mHyperspace != nullptr)
				gBoard->mHyperspace->mHoldsJump = false;
			if (mTelly == nullptr || mTelly->mClicked)
				mClicked = true;
		}
		break;
	}
	default:
		break;
	}
	if (0 < mState)
		UpdateFades();
}

void RankBonusWidget::Draw(Graphics* g)
{
	if (mTelly != nullptr) {
		int aPriority = 0x7fffffff;
		if (gBoard != nullptr && gBoard->mBannerShow.GetOutVal() != 0.0)
			aPriority = 50;
		mWidgetManager->FlushDeferredOverlayWidgets(aPriority);
	}
	DeferOverlay(0);
	TwistWidget::Draw(g);
	// A zoom by mScale about the screen's middle, which only the Direct3D draw of the stars uses
	SexyTransform2D aZoomMatrix;  // 0x44f860
	aZoomMatrix.Translate((float)-(gApp->mWidth / 2), (float)-(gApp->mHeight / 2));  // 0x44f8a0
	aZoomMatrix.Scale(mScale, mScale);                                               // 0x44f990
	aZoomMatrix.Translate((float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2));
	if (gD3DInterface != nullptr)
		gD3DInterface->PushTransform(aZoomMatrix, true);  // slot 0x14

	// The waiting and flying stars' shadows
	int i = (int)mStars.size() - 1;
	while (0 <= i) {
		Star* aStar = &mStars[i];
		if (0.0f < aStar->mAlpha) {
			g->SetColor(Color(255, 255, 255, Ftol((double)aStar->mAlpha * 255.0 * (1.0 - mFade.GetOutVal()))));
			int anOffset = Ftol((double)gApp->mScale * 6.0);
			int aY = Ftol((double)(aStar->mToY - aStar->mY) * (double)aStar->mProgress + (double)(aStar->mY - anOffset));
			int aX = Ftol((double)(aStar->mToX - aStar->mX) * (double)aStar->mProgress + (double)(aStar->mX - anOffset));
			g->DrawImageCel(IMAGE_STARBAR_STARS_STARSHADOW, aX, aY, aStar->mCel);
		}
		i--;
	}
	if (gD3DInterface != nullptr)
		gD3DInterface->PopTransform();  // slot 0x18
	if (mTelly != nullptr) {
		double aDark = (1.0 - mTelly->mShow.GetOutVal()) * 0.7 * 255.0;
		if (aDark < 0.0)
			aDark = 0.0;
		g->SetColor(Color(0, 0, 0, Ftol(aDark)));
		g->FillRect(0, -mY, mWidth, mHeight);
	}
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColor(Color(255, 255, 255));
	g->PushState();
	int aScreenOffX = Ftol((double)gApp->mScale * GetImageOffsetX(IMAGE_RANKBAR_SCREEN_ID));
	int aLeft = -(IMAGE_RANKBAR_SCREEN->mWidth / 2) - aScreenOffX + mWidth / 2;
	if (0.0 < mPillars.GetOutVal()) {
		g->ClipRect(0, 0, mWidth, Ftol((double)Ftol((double)gApp->mScale * 60.0) + (double)mBarY));
		// The pillars rise and close in from the sides
		int anOffY = Ftol((double)gApp->mScale * GetImageOffsetY(IMAGE_RANKBAR_LEFTPILLAR_ID));
		double aRise = mPillars.GetOutVal();
		int aDrop = Ftol((double)gApp->mScale * 800.0);
		int anOffX = Ftol((double)gApp->mScale * GetImageOffsetX(IMAGE_RANKBAR_LEFTPILLAR_ID));
		double aSlide = mSlide.GetOutVal();
		double aLeftD = (double)aLeft;
		int aY = Ftol((1.0 - aRise) * aDrop + anOffY);
		int anApart = Ftol((double)gApp->mScale * 400.0);
		g->DrawImage(IMAGE_RANKBAR_LEFTPILLAR, Ftol((double)anApart * (aSlide - 1.0) + aLeftD + anOffX), aY);
		anOffY = Ftol((double)gApp->mScale * GetImageOffsetY(IMAGE_RANKBAR_RIGHTPILLAR_ID));
		aRise = mPillars.GetOutVal();
		aDrop = Ftol((double)gApp->mScale * 800.0);
		anOffX = Ftol((double)gApp->mScale * GetImageOffsetX(IMAGE_RANKBAR_RIGHTPILLAR_ID));
		aSlide = mSlide.GetOutVal();
		aY = Ftol((1.0 - aRise) * aDrop + anOffY);
		anApart = Ftol((double)gApp->mScale * 400.0);
		g->DrawImage(IMAGE_RANKBAR_RIGHTPILLAR, Ftol(aLeftD - (double)anApart * (aSlide - 1.0) + anOffX), aY);

		// The screen between them opens with the slide
		double anOpen = mSlide.GetOutVal() + 0.019999999552965164;
		if (1.0 < anOpen)
			anOpen = 1.0;
		int aWidth = Ftol(anOpen * IMAGE_RANKBAR_SCREEN->mWidth);
		anOffY = Ftol((double)gApp->mScale * GetImageOffsetY(IMAGE_RANKBAR_SCREEN_ID));
		aRise = mPillars.GetOutVal();
		aDrop = Ftol((double)gApp->mScale * 800.0);
		anOffX = GetImageOffsetX(IMAGE_RANKBAR_SCREEN_ID);
		aY = Ftol((1.0 - aRise) * aDrop + anOffY);
		g->DrawImage(IMAGE_RANKBAR_SCREEN, (IMAGE_RANKBAR_SCREEN->mWidth - aWidth) / 2 +
			(Ftol((double)gApp->mScale * anOffX) + aLeft), aY, aWidth, IMAGE_RANKBAR_SCREEN->mHeight);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, Ftol(mGlow.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		anOpen = mSlide.GetOutVal() + 0.019999999552965164;
		if (1.0 < anOpen)
			anOpen = 1.0;
		aWidth = Ftol(anOpen * IMAGE_RANKBAR_HOTNSEXY->mWidth);
		anOffY = Ftol((double)gApp->mScale * GetImageOffsetY(IMAGE_RANKBAR_HOTNSEXY_ID));
		aRise = mPillars.GetOutVal();
		aDrop = Ftol((double)gApp->mScale * 800.0);
		anOffX = GetImageOffsetX(IMAGE_RANKBAR_HOTNSEXY_ID);
		aY = Ftol((1.0 - aRise) * aDrop + anOffY);
		g->DrawImage(IMAGE_RANKBAR_HOTNSEXY, (IMAGE_RANKBAR_HOTNSEXY->mWidth - aWidth) / 2 +
			(Ftol((double)gApp->mScale * anOffX) + aLeft), aY, aWidth, IMAGE_RANKBAR_HOTNSEXY->mHeight);
		g->PopState();
	}
	// (the original pushes the state before the test and pops it only inside, as here: one state too many stays
	// pushed while the pillars are down, which changes nothing drawn)

	// The bar, its slots and the ranks at its ends
	g->SetColor(Color(255, 255, 255));
	g->Translate(0, Ftol(mBarY));
	int aHalfHeight = mBar->mHeight / 2;
	int aBarX = gApp->mWidth / 2 - mBar->mWidth / 2;
	g->DrawImage(mBar, aBarX, 0);
	g->SetColor(Color::White);
	DrawBar(g);
	int aTextY = Ftol((double)mBar->mHeight * -0.0 + (double)aHalfHeight);
	g->SetFont(FONT_TOOLTIP);
	float aBarWidthF = (float)mBar->mWidth;
	float aBarXF = (float)aBarX;
	int aLeftX = Ftol((double)aBarWidthF * 0.15000000596046448 + (double)aBarXF);
	int aRightX = Ftol((double)aBarWidthF * 0.8500000238418579 + (double)aBarXF);
	SexyString aText = StrFormat(GTC(0x7ea1d0), mRank + 1);
	g->SetColor(Color(0x20, 0x20, 0x20));
	g->DrawString(aText, aLeftX + 2, aTextY + 2);
	g->SetColor(Color(0xff, 0xf8, 0xa6));
	g->DrawString(aText, aLeftX, aTextY);
	aText = StrFormat(GTC(0x7ea1d0), mRank + 2);
	g->SetColor(Color(0x20, 0x20, 0x20));
	int aShadowY = Ftol((double)gApp->mScale * 3.0) + aTextY;
	g->DrawString(aText, aRightX - g->GetFont()->StringWidth(aText) + 2, aShadowY);
	g->SetColor(Color(0xff, 0xf8, 0xa6));
	g->DrawString(aText, aRightX - g->GetFont()->StringWidth(aText), aTextY);
	DrawBadge(g, mRank, Ftol((double)mBar->mWidth * 0.07999999821186066 + (double)aBarXF),
		Ftol((double)mBar->mHeight * 0.46000000834465027), 0.35f, 1.0f);
	DrawBadge(g, mRank + 1, Ftol((double)mBar->mWidth * 0.9200000166893005 + (double)aBarXF),
		Ftol((double)mBar->mHeight * 0.46000000834465027), 0.35f, 1.0f);
	int aShown = mFilled;
	if (mSlots < mFilled)
		aShown = mSlots;
	aText = StrFormat(GTC(0x7ea1b0), aShown, mSlots);
	int aCountY = Ftol((double)gApp->mScale * 65.0) + aTextY;
	g->DrawString(aText, mWidth / 2 - g->GetFont()->StringWidth(aText) / 2, aCountY);
	g->Translate(0, Ftol(-(double)mBarY));

	// The stars
	g->SetColorizeImages(true);
	i = (int)mStars.size() - 1;
	while (0 <= i) {
		Star* aStar = &mStars[i];
		if (0.0f < aStar->mAlpha) {
			g->SetColor(Color(255, 255, 255, Ftol((double)aStar->mAlpha * 255.0 * (1.0 - mFade.GetOutVal()))));
			int aY = Ftol((double)(aStar->mToY - aStar->mY) * (double)aStar->mProgress + (double)aStar->mY);
			int aX = Ftol((double)(aStar->mToX - aStar->mX) * (double)aStar->mProgress + (double)aStar->mX);
			g->DrawImageCel(IMAGE_STARBAR_STARS_GOLDSTARS, aX, aY, aStar->mCel);
		}
		i--;
	}
	g->SetColor(Color::White);
}

void RankBonusWidget::DrawOverlay(Graphics* g)
{
	if (0.0 < mFlash.GetOutVal()) {
		int aBarWidth = mBar->mWidth;
		int anAppWidth = gApp->mWidth;
		int aY = Ftol((double)mBarY);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mFlash.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aDY = GetImageOffsetY(IMAGE_RANKBAR_BUTTONFLASH_ID) - GetImageOffsetY(IMAGE_RANKBAR_LEVELUP_ID);
		aY = Ftol((double)gApp->mScale * aDY) + aY;
		int aDX = GetImageOffsetX(IMAGE_RANKBAR_BUTTONFLASH_ID) - GetImageOffsetX(IMAGE_RANKBAR_LEVELUP_ID);
		g->DrawImage(IMAGE_RANKBAR_BUTTONFLASH, Ftol((double)gApp->mScale * aDX) + (anAppWidth / 2 - aBarWidth / 2), aY);
	}
}

void RankBonusWidget::DrawBar(Graphics* g)
{
	g->SetColor(Color(255, 255, 255));
	int aLeft = mWidth / 2 - mBarWidth / 2;
	int aTop = Ftol((double)gApp->mScale * 56.0);
	int aCount = mSlots;
	g->SetColorizeImages(false);
	float aPrev = 0;
	bool inRun = false;
	int i = 0;
	float aX = (float)aLeft;
	float aRunStart = aX;
	float aRunEnd = aX;
	if (0 < aCount) {
		do {
			float aWidth = mWidths[i];
			aX = (float)((double)aWidth * 0.5 + (double)aPrev * 0.5 + (double)aX);
			aPrev = aWidth;
			Image* aStars = i < mFilled ? IMAGE_STARBAR_STARS_GOLDSTARS : IMAGE_STARBAR_STARS_BLUESTARS;
			int aSize = Ftol(0.5 + (double)aWidth);
			if (aStars->mWidth <= aSize)
				aSize = aStars->mWidth;
			bool drawRun;
			bool drawStar;
			if ((float)aSize < 4.5f) {
				// Too narrow for a star: part of a bar
				if (!inRun) {
					inRun = true;
					aRunStart = aX;
				}
				aRunEnd = aX;
				drawRun = i == mSlots - 1;
				drawStar = false;
			} else if (inRun) {
				drawRun = true;
				inRun = false;
				drawStar = true;
			} else {
				drawRun = false;
				drawStar = true;
			}
			if (drawRun) {
				int aRunX = Ftol(aRunStart) - 1;
				int aRunWidth = Ftol((double)aRunEnd - (double)aRunStart + 3.0);
				g->SetColor(Color(255, 255, 255));
				Image* aBarImage = IMAGE_STARBAR_BARS_GOLD_BAR;
				Rect aSrc(0, 0, aRunWidth, IMAGE_STARBAR_BARS_GOLD_BAR->mHeight);
				if (mFilled <= i) {
					aSrc.mX = IMAGE_STARBAR_BARS_BLUEBAR->mWidth - aRunWidth;
					aBarImage = IMAGE_STARBAR_BARS_BLUEBAR;
				}
				int aY = aTop - Ftol((double)gApp->mScale * -17.0);
				if (gApp->mArtRes == 0x300)
					aY = Ftol((double)(float)aY + 1.0);
				int aHeight = Ftol((double)aStars->GetCelHeight() * 0.6000000238418579);
				g->DrawImage(aBarImage, Rect(aRunX, aY, aRunWidth, aHeight), aSrc);
				if (inRun)
					drawStar = false;
			}
			if (drawStar) {
				int aCel = StarCel(aStars, (double)aWidth * 1.149999976158142);
				float aShare = (float)((double)aWidth / (double)aStars->GetCelWidth());
				int aHeight = Ftol((double)aStars->GetCelHeight() * ((double)aShare * 0.5 + 0.6000000238418579));
				// The slot just filled is drawn again, additively, in its flash
				int aPasses = (i == mLastFilled) + 1;
				int aPass = 0;
				if (0 < aPasses) {
					do {
						int aCelWidth = aStars->GetCelWidth();
						int aY = Ftol((double)(aStars->GetCelHeight() - aHeight) * 0.65 + (double)aTop);
						int aStarX = Ftol((double)aX - (double)(aStars->GetCelWidth() / 2));
						g->DrawImageCel(aStars, Rect(aStarX, aY, aCelWidth, aHeight), aCel);
						g->SetColorizeImages(true);
						g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
						float anAngle = (float)((double)mFilledFlash * 3.1415927410125732 + 3.1415927410125732);
						float aCos = cosf(anAngle);
						g->SetColor(Color(255, 255, 255, Ftol(((double)aCos + 1.0) * 255.0 * 0.5)));
						aPass++;
					} while (aPass < (i == mLastFilled) + 1);
				}
				g->SetColorizeImages(false);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
			i++;
		} while (i < aCount);
	}
}

void RankBonusWidget::DrawBadge(Graphics* g, int theRank, int theX, int theY, float theScale, float theAlpha)
{
	if (theRank < 0)
		theRank = 0;
	if (98 < theRank)
		theRank = 100;
	int aMark = theRank % 10;
	bool wasColorized = g->GetColorizeImages();
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColor(Color(255, 255, 255, Ftol((double)theAlpha * 255.0)));
	Image* aBadge = GetImageById(kRankBadges[theRank / 5]);
	int aWidth = Ftol((double)aBadge->GetWidth() * (double)theScale);
	int aHeight = Ftol((double)aBadge->GetHeight() * (double)theScale);
	int aY = Ftol((double)(theY - aHeight / 2) - (double)theScale * 3.0);
	g->DrawImage(aBadge, theX - aWidth / 2, aY, aWidth, aHeight);
	if (aMark != 0 && aMark != 5) {
		Image* aMarkImage = GetImageById(kRankBadgeMarks[aMark]);
		int aMarkWidth = Ftol((double)aMarkImage->mWidth * (double)theScale);
		int aMarkHeight = Ftol((double)aMarkImage->mHeight * (double)theScale);
		g->DrawImage(aMarkImage, theX - aMarkWidth / 2, theY - aMarkHeight / 2, aMarkWidth, aMarkHeight);
	}
	g->SetColorizeImages(wasColorized);
}
