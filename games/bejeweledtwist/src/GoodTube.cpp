// Sexy::GoodTube (0x702736-0x70a9bb): being ported, see GoodTube.h.
#include "GoodTube.h"
#include "Shockwave.h"

#include "Board.h"
#include "EffectsManager.h"
#include "GameOverShow.h"
#include "GameText.h"
#include "Hyperspace.h"
#include "Graphics.h"
#include "Font.h"
#include "Image.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "Profile.h"
#include "RankBonusWidget.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "Traveller.h"
#include "TwistButton.h"
#include "TriVertex.h"
#include "CurvedVal.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstring>

using namespace Sexy;

// The first light of each level (0x84f028 in the exe): IMAGE_INGAMEUI_ARC_4_FILL_1 for the first, then 8, 12 and 16
static const int sFirstLight[11] = {IMAGE_INGAMEUI_BONUS_BLUE_LIT_ID, IMAGE_INGAMEUI_ARC_4_FILL_1_ID,
	IMAGE_INGAMEUI_ARC_8_FILL_1_ID, IMAGE_INGAMEUI_ARC_12_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID,
	IMAGE_INGAMEUI_ARC_16_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID,
	IMAGE_INGAMEUI_ARC_16_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID};

// 0x4a7220 (stdcall): the colour of a hue (0 to 1535, six sectors of 256), a saturation and a lightness (0 to 255),
// as 0xffRRGGBB
static unsigned int HSLToColor(int theHue, int theSaturation, int theLightness)
{
	int aMax;  // the brighter end of the sector (the exe's v1)
	if (theLightness < 0x80)
		aMax = ((theSaturation + 0xff) * theLightness) / 0xff;
	else
		aMax = (theSaturation - (theSaturation * theLightness) / 0xff) + theLightness;
	int aMin = theLightness * 2 - aMax;
	int aSector = (theHue * 6 + ((theHue * 6) >> 31 & 0xff)) >> 8;
	int aFrac = (theHue - (aSector << 8) / 6) * 6;
	double aStep = ((double)(aMax - aMin) * (double)aFrac) / 255.0;
	int aUp = Ftol((double)aMin + aStep);
	if (aUp > 0xff)
		aUp = 0xff;
	int aDown = Ftol((double)aMax - aStep);
	if (aDown < 0)
		aDown = 0;
	int aRed, aGreen, aBlue;
	switch (aSector) {
	case 1:
		aRed = aDown;
		aGreen = aMax;
		aBlue = aMin;
		break;
	case 2:
		aRed = aMin;
		aGreen = aMax;
		aBlue = aUp;
		break;
	case 3:
		aRed = aMin;
		aGreen = aDown;
		aBlue = aMax;
		break;
	case 4:
		aRed = aUp;
		aGreen = aMin;
		aBlue = aMax;
		break;
	case 5:
		aRed = aMax;
		aGreen = aMin;
		aBlue = aDown;
		break;
	default:
		aRed = aMax;
		aGreen = aUp;
		aBlue = aMin;
		break;
	}
	return (((unsigned int)aRed | 0xffffff00u) << 8 | (unsigned int)aGreen) << 8 | (unsigned int)aBlue;
}

// 0x6a3403: the pixel (0 outside the image)
unsigned int Sexy::GetPixelAt(Image* theImage, int theX, int theY)
{
	if (theImage == nullptr)
		return 0;
	if (theX < 0 || theY < 0 || theImage->mWidth <= theX || theImage->mHeight <= theY)
		return 0;
	const ulong* aBits = static_cast<MemoryImage*>(theImage)->GetBits();
	return (unsigned int)aBits[theImage->mWidth * theY + theX];
}

// 0x84f128: the light the bonus challenge's flash is on
static int sBonusLight = -1;

Ticker::Ticker()
{
	// 0x707960
	mAlpha = 1.0f;
	mValue = 0;
	mTarget = 0;
	mFade = 0;
	mTimer = 0;
	mSmallLabelFont = FONT_TOOLTIP;
	mLabelFont = FONT_TOOLTIP;
	mFont = FONT_SCORE;
	mGlowFont = FONT_SCOREGLOW;
	mSmallFont = FONT_SCORE_SMALL;
	mSmallGlowFont = FONT_SCOREGLOW_SMALL;
	mUseSmallFont = false;
	UpdateText();
}

void Ticker::UpdateText()
{
	// 0x7073be
	mText = CommaSeperate(mValue);
}

void Ticker::Update()
{
	// 0x7073fc
	mTimer++;
	int aTimer = mTimer;
	int aTarget = mTarget;
	int aValue = mValue;
	if (aValue == aTarget) {
		float aFade = (float)((double)mFade - 0.019999999552965164);
		if (aFade < 0.0f)
			aFade = 0.0f;
		mFade = aFade;
	} else {
		mFade = 1.0f;
		if (aTimer % 10 == 0) {
			int aStep = aTarget - aValue;
			if (aStep < 0) {
				mValue = aTarget;
			} else {
				if (100 < aStep) {
					int aBig = 10;
					if (9 < aStep / 10)
						aBig = aStep / 10;
					aStep = aBig - aBig % 10;
				}
				mValue = aValue + aStep;
			}
			UpdateText();
		}
	}
}

void Ticker::Draw(Graphics* g, int theX, int theY, bool theCentre, float theAlpha)
{
	// 0x707476
	Font* aFont = mFont;
	Font* aGlowFont = mGlowFont;
	int aWidth = aFont->StringWidth(mText);
	if (1999999 < mValue || mUseSmallFont) {
		aFont = mSmallFont;
		aGlowFont = mSmallGlowFont;
		aWidth = aFont->StringWidth(mText);
	}
	g->SetFont(aFont);
	if (theCentre)
		theX += -(aWidth / 2);
	g->SetColor(Color(255, 255, 255, Ftol((double)mAlpha * 255.0 * (double)theAlpha)));
	g->DrawString(mText, theX, theY);
	if (0.0f < mFade) {
		g->SetColorizeImages(true);
		double aGlow = (double)mAlpha * 255.0 * (double)mFade * (double)theAlpha;
		if (255.0 < aGlow)
			aGlow = 255.0;
		g->SetColor(Color(255, 255, 255, Ftol(aGlow)));
		g->SetFont(aGlowFont);
		SetFontLayerColor(g->GetFont(), GTS(0x7dd6b0), Color::White);
		g->DrawString(mText, theX, theY);
	}
}

GoodTube::GoodTube(Board* theBoard)
{
	mBoard = theBoard;
	mBaseStep = 1.0f;
	mAddedSum = 0;
	mName = "GOODTUBE";
	mTopFall = (float)Ftol((double)gApp->mScale * 0.0);
	mChallengeStyle = 1;
	mShowTenths = true;
	mColorCycle.mColors.clear();
	mColorCycle.mPos = 0;
	mColorCycle.mColors.push_back(Color::White);
	UpdateImages();
	SetLevelSoundOn(false);
	SetShown(false);
	for (int i = 16; i != 0; i--) {
		Bubble* aBubble = new Bubble();
		aBubble->mPhaseStep = 0;
		aBubble->mPhase = (float)((double)(Rand() % 100) * 0.001);
		mBubbles.push_back(aBubble);  // 0x707ad5
	}
	// The tube does not clip its drawing to itself (+0x41). With Direct3D the clip rectangle stands in the screen's
	// pixels after the board's transform, so a clipped tube lost its top, glass and pod under a menu replay's 0.4 scale.
	mClip = false;
	mLevelText = new TextImage(FONT_INGAME, GTS(0x7e4018), 0, 0, -1, 0, nullptr);  // 0x74e701 ("LEVEL"; nothing draws it)
	mRankBadge = new RankBadge(this);
	mTipWidth = Ftol((double)gApp->mScale * 200.0);
	mMenuButton = new TwistButton(0, IMAGE_INGAMEUI_MENUBUTTON, this);
	mMenuButton->mName = "MENU_BUTTON";
	if (IMAGE_INGAMEUI_MENUBUTTON != nullptr) {
		mMenuButton->mNormalRect = IMAGE_INGAMEUI_MENUBUTTON->GetCelRect(0);
		mMenuButton->mOverRect = IMAGE_INGAMEUI_MENUBUTTON->GetCelRect(1);
		mMenuButton->mDownRect = IMAGE_INGAMEUI_MENUBUTTON->GetCelRect(2);
	}
	mMenuButton->mColors[0] = Color(255, 255, 255);
	mFXManager = new EffectsManager();
	mRandomPhase = std::fabs((float)RandSigned());
	mBonusSwap.SetCurve(GTS(0x7e3e80));  // 0x704d9c
}

RankBadge::RankBadge(GoodTube* theTube)
{
	// 0x701efb
	mClickToggle = 0;
	mOver = false;
	mClickTimer = 0;
	mClip = false;
	mTube = theTube;
}

int RankBadge::GetSize()
{
	// 0x701fa5
	return Ftol((double)gApp->mScale * 90.0);
}

void RankBadge::Draw(Graphics* g)
{
	// 0x701f3a
	if (mTube->mShown) {
		int anX = Ftol((double)mWidth * 0.49000000953674316);
		int aY = Ftol((double)mHeight * 0.550000011920929);
		RankBonusWidget::DrawBadge(g, gApp->mProfile->mRank, anX, aY, 0.4f, 1.0f);
	}
}

// 0x701fbb and 0x70200f: not while the board is busy, after its tutorial, between games (+0x160) or paused (+0x1570)
static bool BadgeIgnoresMouse()
{
	Board* aBoard = gBoard;
	return aBoard != nullptr && (aBoard->IsBusy() || aBoard->mIsReplay || aBoard->mPlayState != 0 ||
		aBoard->mPauseCount != 0);
}

void RankBadge::MouseEnter()
{
	// 0x701fbb
	if (BadgeIgnoresMouse())
		return;
	mClickToggle = 0;
	mClickTimer = 0;
	mOver = true;
}

void RankBadge::MouseLeave()
{
	// 0x702007
	mOver = false;
}

void RankBadge::MouseDown(int x, int y, int theClickCount)
{
	// 0x70200f
	if (BadgeIgnoresMouse())
		return;
	mClickToggle++;
	mClickTimer = 0;
	if (1 < mClickToggle)
		mClickToggle = 0;
}

GoodTube::~GoodTube()
{
	// 0x707941, 0x704bfd
	RemoveAllWidgets(false, false);
	delete mLevelText;
	delete mFXManager;
	delete mMenuButton;
	delete mRankBadge;
	for (unsigned int i = 0; i < mSpecks.size(); i++)
		delete mSpecks[i];
	for (Bubble* aBubble : mBubbles)
		delete aBubble;
}

void GoodTube::SetShown(bool theShown)
{
	mShown = theShown;
	if (mRankBadge != nullptr) {
		mRankBadge->mVisible = theShown;
		mRankBadge->mMouseVisible = theShown;
	}
}

void GoodTube::SetTubeKind(int theValue)
{
	mTubeKind = theValue;
	if (theValue == 3)
		mBestFade.SetConstant(1.0);  // the best's fade (+0x580) full at once
}

void GoodTube::SetChallenge(bool on)
{
	mChallenge = on;
	UpdateImages();
}

void GoodTube::SetTime(int theTime, bool theAtOnce)
{
	mTimeLimit = theTime;
	if (mTimeFull == 0)
		mTimeFull = theTime;
	if (theAtOnce) {
		mTimeShown = theTime;
		UpdateFill(theAtOnce);
	}
}

void GoodTube::StartTimer(bool theFull, bool theDrawTime)
{
	if (mTimeKind < 1) {
		mTimePulseStep = 0;
		mTimePulse = 0;
		mTimeKind = 1;
		mBlitzTimer.SetCurve(GTS(0x7e3fe0));
		if (theFull)
			mBlitzTimer.mInVal = mBlitzTimer.mInMax;
		mBlitzTimer.mIncRate = 0;
	}
	if (theFull)
		mTimeKind = 4;
	mDrawTime = theDrawTime;
}

void GoodTube::SetColor(const Color& theColor, bool theAtOnce)
{
	Color aLast = mColorCycle.mColors[mColorCycle.mColors.size() - 1];
	mColorCycle.mColors.clear();
	mColorCycle.mPos = 0;
	mColorCycle.mColors.push_back(aLast);
	mColorCycle.mColors.push_back(theColor);
	mColorCycle.mLoop = false;
	if (theAtOnce)
		mColorCycle.SetPosition(1.0f);
}

// 0x70c94a: the tube's tip text, in theColor, wrapped to theMaxWidth in FONT_TOOLTIP at its spaces; it fades in and
// stays for theSpeed (or 1) of the 0.01 steps of +0x248 (see Update)
void GoodTube::SetTip(const SexyString& theText, const Color& theColor, float theSpeed, int theMaxWidth)
{
	mTipColor[0] = theColor.mRed;
	mTipColor[1] = theColor.mGreen;
	mTipColor[2] = theColor.mBlue;
	mTipColor[3] = theColor.mAlpha;
	Font* aFont = FONT_TOOLTIP;
	SexyString aText = theText;
	if (theMaxWidth < aFont->StringWidth(aText)) {
		std::vector<SexyString> aLines;
		int aLastSpace = -1;
		unsigned int i = 0;
		if (aText.size() != 0) {
			do {
				if (aText[i] == '\n') {
					aLines.push_back(aText.substr(0, i));
					aText = aText.substr(i + 1);
					i = 0;
					aLastSpace = -1;
				} else if (aText[i] == ' ') {
					bool aTooWide = theMaxWidth < aFont->StringWidth(aText.substr(0, i));
					int aSpace = (int)i;
					if (aTooWide) {
						if (aLastSpace == -1)
							aLastSpace = (int)i;
						aLines.push_back(aText.substr(0, aLastSpace));
						aText = aText.substr(aLastSpace + 1);
						i = 0;
						aSpace = aLastSpace;
					}
					aLastSpace = aSpace;
				}
				i++;
			} while (i < aText.size());
		}
		if (theMaxWidth < aFont->StringWidth(aText) && aLastSpace != -1) {
			aLines.push_back(aText.substr(0, aLastSpace));
			aText = aText.substr(aLastSpace + 1);
		}
		aLines.push_back(aText);
		aText.clear();
		for (unsigned int j = 0; j < aLines.size(); j++) {
			if (j != 0)
				aText += _S("\n");
			aText += aLines[j];
		}
	}
	mTip = aText;
	mTipFade = 1.0f;
	mTipTime = 0.0f < theSpeed ? theSpeed : 1.0f;
}

void GoodTube::UpdateImages()
{
	mTopImage = mChallenge ? IMAGE_INGAMEUI_TUBE_TOP_CHALLENGE : IMAGE_INGAMEUI_TUBE_TOP;
	mBottomImage = IMAGE_INGAMEUI_TUBE_BOTTOM;
	mGlassImage = IMAGE_INGAMEUI_TUBE_GLASS;
	mFillImage = IMAGE_INGAMEUI_TUBE_FILL_GOLD;
	mFillImage2 = IMAGE_INGAMEUI_TUBE_FILL_GOLD;
	if (mGlassImage == nullptr)
		mGlassHeight = 0;
	else
		mGlassHeight = GetBottomTop() - GetGlassTop();
	float aTop = (float)(GetGlassTop() + mGlassHeight);
	mFillTopGoal = aTop;
	mFillTopShown = aTop;
}

int GoodTube::GetTubeHeight()
{
	if (mGlassHeight == 0)
		return 0;
	return Ftol((double)(mGlassImage->mHeight + mBottomImage->mHeight + mTopImage->mHeight) * 0.9300000071525574);
}

int GoodTube::GetTubeWidth()
{
	if (mGlassHeight == 0)
		return 0;
	int aWidth = IMAGE_INGAMEUI_TOP_LIGHTGLOW->mWidth;
	if (IMAGE_INGAMEUI_TOP_LIGHTGLOW->mWidth <= IMAGE_INGAMEUI_BOT_LIGHTGLOW->mWidth)
		aWidth = IMAGE_INGAMEUI_BOT_LIGHTGLOW->mWidth;
	return aWidth;
}

int GoodTube::GetGlassTop()
{
	float aScale = (float)((double)gApp->mArtRes / 1200.0);
	return Ftol((double)GetImageOffsetY(GetIdByImage(mGlassImage)) * (double)aScale);
}

int GoodTube::GetBottomTop()
{
	float aScale = (float)((double)gApp->mArtRes / 1200.0);
	int aTop = Ftol((double)GetImageOffsetY(GetIdByImage(mBottomImage)) * (double)aScale);
	return Ftol((double)aTop + (double)mTopBounce);
}

void GoodTube::SetProgress(float theProgress, bool theAtOnce)
{
	if (1.0f < theProgress)
		theProgress = 1.0f;
	mProgress = theProgress;
	UpdateFill(theAtOnce);
}

void GoodTube::UpdateFill(bool theAtOnce)
{
	if (mIsBlitzTube == 1) {
		// Blitz: the time left
		float aProgress;
		if (mTimeFull == 0)
			aProgress = 0.0f;
		else
			aProgress = (float)mTimeShown / (float)mTimeFull;
		mProgress = aProgress;
	}
	int aMargin = Ftol((double)gApp->mScale * 35.0);
	mFillTop = GetGlassTop() + aMargin;
	int aBottom = Ftol((double)gApp->mScale * 5.0) + GetBottomTop();
	mFillBottom = aBottom;
	int aHeight = Ftol((double)(aBottom - mFillTop) * (double)mProgress);
	if (0 < mBreakState)
		aHeight = 0;
	float aTop = (float)(aBottom - aHeight);
	mFillTopGoal = aTop;
	if (mBoard->mHyperspace == nullptr && !theAtOnce)
		mFillTopShown = (float)(((double)aTop - (double)mFillTopShown) / 30.0 + (double)mFillTopShown);
	else
		mFillTopShown = aTop;
}

void GoodTube::SetLevelSteps(int theSteps)
{
	if (mChainSteps < theSteps) {
		int aFlash = mFlashBrightness + 30;
		int aMax = 200;
		if (aFlash < 200)
			aMax = aFlash;
		mFlashBrightness = aMax;
		mFlashLevel = 0.5f;
	}
	if (theSteps < mChainSteps) {
		MarkLevelChange();
		mLevelFlash = mLevelFlash * 0.5f;
	}
	mChainSteps = theSteps;
}

void GoodTube::SetLevelShown(int theLevel)
{
	if (!mLevelSoundOn)
		return;
	if (10 < theLevel)
		theLevel = 10;
	if (theLevel != mShownLevel) {
		float aSteps;
		if (mShownLevel < theLevel) {
			if (!mBoard->IsBusy()) {
				int aSound = 3;
				if (theLevel < 4)
					aSound = theLevel;
				// SOUND_MULTIPLIER_UP2_1 is resource 0x397 (0x397 + the level, at most 3)
				gApp->PlaySample(GetSoundById(SOUND_MULTIPLIER_UP2_1_ID + aSound));
			}
			mLevelFlash = 1.0f;
			gBoard->mRingBurst = 1.0f;
			int aFlash = mFlashBrightness + 64;
			int aMax = 255;
			if (aFlash < 255)
				aMax = aFlash;
			mFlashBrightness = aMax;
			mFlashLevel = 0.5f;
			aSteps = 0.0f;
		} else {
			if (!mBoard->IsBusy())
				gApp->PlaySample(SOUND_MULTIPLIER_DOWN);
			aSteps = (float)Board::GetLevelSteps(theLevel);
		}
		mShownSteps = aSteps;
	}
	mShownLevel = theLevel;
	mLevelCel = theLevel;
}

void GoodTube::GetStepPoint(int* thePoint, int theStep, bool theCentre)
{
	int aLevel = mShownLevel;
	int aSteps = Board::GetLevelSteps(aLevel);
	int aStep = theStep % aSteps;
	int aLast = Board::GetLevelSteps(aLevel) - 1;
	if (aLast < aStep)
		aStep = aLast;
	int anId = sFirstLight[aLevel] + aStep;
	Image* anImage = GetImageById(anId);
	int aX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(anId));
	int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(anId));
	if (theCentre) {
		aX += anImage->mWidth / 2;
		aY += anImage->mHeight / 2;
	}
	thePoint[0] = aX;
	thePoint[1] = aY;
}

void GoodTube::ResetBonus()
{
	mBonusState = 0;
	for (unsigned int i = 0; i < mBonusGlowTop.size(); i++) {
		mBonusGlowTop[i] = 0;
		mBonusGlowBottom[i] = 0;
	}
	mBonusTimer = 0;
	mBonusGlowFade = 1.0f;
	mBonusIntro = 0;
	mBonusFade = 0;
}

void GoodTube::AddedToManager(WidgetManager* theWidgetManager)
{
	// 0x703582 (the base class' is not called)
	if (mGlassHeight != 0) {
		mWidth = GetTubeWidth();
		mHeight = GetTubeHeight();
	}
	AddWidget(mMenuButton);
	AddWidget(mRankBadge);
	SetShown(mShown);
	SetLevelSoundOn(mLevelSoundOn);
}

void GoodTube::RemovedFromManager(WidgetManager* theWidgetManager)
{
	// 0x702a4a (the base class' is not called)
	RemoveWidget(mMenuButton);
	RemoveWidget(mRankBadge);
}

void GoodTube::GotFocus()
{
	// 0x7029ae
	mHasFocus = true;
	gApp->SetCursor(0);
}

void GoodTube::ButtonPress(int theId, int theClickCount)
{
	// 0x7029c1 (the ButtonListener's slot 1)
	gApp->PlaySample(SOUND_CLICK);
}

void GoodTube::ButtonDepress(int theId)
{
	// 0x7029d8 (the ButtonListener's slot 2)
	Board* aBoard = gBoard;
	if (aBoard != nullptr) {
		if (aBoard->IsBusy())
			return;
		if (aBoard->mIsReplay)
			return;
		if (aBoard->mPlayState != 0)
			return;
		if (aBoard->mPauseCount != 0)
			return;
	}
	gApp->PlaySoundEx(SOUND_BUTTONPRESS2, 0, 5.0f, 0.0f, true, true);
	if (theId == 0) {
		// 0x6a6983: the menu comes up over the board
		aBoard->ShowMenu();
	}
}

void GoodTube::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// 0x70318f
	Widget::Resize(theX, theY, theWidth, theHeight);
	if (mGlassHeight != 0) {
		int anArtRes = gApp->mArtRes;
		mLastX = theX;
		mGlassX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_INGAMEUI_TUBE_GLASS_ID));
		mGlassHeight = mGlassImage->mHeight;
		mGlassBottom = GetGlassTop() + mGlassHeight;
		if (mMenuButton != nullptr) {
			int anId = GetIdByImage(mMenuButton->mButtonImage);
			int aCelWidth = mMenuButton->mButtonImage->GetCelWidth();
			int aCelHeight = mMenuButton->mButtonImage->GetCelHeight();
			int anX = Ftol((double)gApp->mScale * 9.0);
			anX = Ftol(((double)mWidth * 0.5 + (double)anX) - (double)(aCelWidth / 2));
			int aY = Ftol((double)GetImageOffsetY(anId) * (double)((float)((double)anArtRes / 1200.0)));
			mMenuButton->Resize(anX, aY, aCelWidth, aCelHeight);
		}
		if (mRankBadge != nullptr) {
			int anX = Ftol((double)mWidth * 0.5299999713897705);
			int aY = Ftol((double)GetTopY() + (double)mTopImage->mHeight * 0.8299999833106995);
			int aWidth = Ftol((double)mWidth * 0.75);
			int aHeight = RankBadge::GetSize();
			mRankBadge->Resize(anX - aWidth / 2, aY - aHeight / 2, aWidth, aHeight);
		}
	}
}

void GoodTube::DrawOverlay(Graphics* g)
{
	// 0x7049e5: with Direct3D, while the board's scale is not 1, the board's matrix is pushed here and popped at the end
	if (mBoard->mBoardZoom != 1.0f && gD3DInterface != nullptr)
		gD3DInterface->PushTransform(mBoard->mTransform, true);  // slot 0x14
	if (mBonusState != 0 && mBonusState != 4)
		DrawBonus(g);
	mFXManager->Draw(g);
	if (mBonusGlowCurve.GetOutVal() != 0.0 && !gBoard->mIsReplay) {
		float anAlpha = (float)mBonusGlowCurve.GetOutVal();
		if (mBoard->mHyperspace != nullptr && gD3DInterface != nullptr) {
			// With Direct3D, during the jump's transitions 8 and 12 the alpha is multiplied by that transition's curve
			if (mBoard->mHyperspace->mTransition == 8)
				anAlpha = (float)(mBoard->mHyperspace->mTubeAlpha.GetOutVal() * (double)anAlpha);
			else if (mBoard->mHyperspace->mTransition == 12)
				anAlpha = (float)(mBoard->mHyperspace->mBoardBrightnessFinal.GetOutVal() * (double)anAlpha);
		}
		g->SetColorizeImages(true);
		g->SetDrawMode(0);
		g->SetColor(Color(255, 255, 255, Ftol((double)anAlpha * 255.0)));
		g->DrawImage(IMAGE_MEGAFRUIT_BRACKET, Ftol((double)gApp->mScale * 72.0),
			Ftol((double)gApp->mScale * 842.0));
	}
	if (mBoard->mBoardZoom != 1.0f && gD3DInterface != nullptr)
		gD3DInterface->PopTransform();  // slot 0x18
}

int GoodTube::GetTopX()
{
	// 0x7027fb
	float aScale = (float)((double)gApp->mArtRes / 1200.0);
	return Ftol((double)GetImageOffsetX(GetIdByImage(mTopImage)) * (double)aScale);
}

int GoodTube::GetBottomX()
{
	// 0x702837
	float aScale = (float)((double)gApp->mArtRes / 1200.0);
	return Ftol((double)GetImageOffsetX(GetIdByImage(mBottomImage)) * (double)aScale);
}

int GoodTube::GetTopY()
{
	// 0x702873
	int anY = Ftol((double)GetImageOffsetY(GetIdByImage(mTopImage)) * (double)gApp->mScale);
	return Ftol((double)anY + (double)mTopFall);
}

int GoodTube::GetScoreY()
{
	// 0x7028b7
	float aScale = (float)((double)gApp->mArtRes / 1200.0);
	int anY = Ftol((double)mTopImage->mHeight * 0.42500001192092896 + (double)GetTopY());
	double aRow = gBoard->mGameMode == Board::MODE_CHALLENGE ? 0.0 : 23.0;
	return Ftol((double)aScale * aRow + (double)anY);
}

void GoodTube::StartBonus()
{
	// 0x703642
	if (mBonusState == 0) {
		ResetBonus();
		mBonusIntro = 1.0f;
		mBonusTimer = 1.0f;
		mBonusListSet = false;
		mBonusShownCount = 0;
		mBonusStep = 0;
		mBonusSwap.SetInVal(0.0);
		mBonusState = 1;
		mBonusNumber = gBoard->mBonusPoints;
		int aSteps = 0;
		for (int aLevel = 1; aLevel <= mBoard->mStepLevel; aLevel++)
			aSteps += Board::GetLevelSteps(aLevel);
		mBonusStepsLeft = (aSteps - mBoard->mChainLinks) - 1;
	}
}

void GoodTube::Update()
{
	// 0x702736: the update ticks as often as mBaseStep says
	mMenuButton->mMouseVisible = true;
	if (mBoard != nullptr) {
		if (mBoard->mHyperspace != nullptr && gD3DInterface != nullptr &&
			mBoard->mHyperspace->mTubeAlpha.GetOutVal() < 0.5) {
			// With Direct3D the menu button's mouse is off while the jump's curve is under 0.5
			mMenuButton->mMouseVisible = false;
		}
		Board* aBoard = mBoard;
		if (aBoard->IsBusy())
			return;
		if (0 < aBoard->mPauseCount)
			return;
	}
	float aSum = (float)((double)mBaseStep + (double)mAddedSum);
	mAddedSum = aSum;
	while (1.0f <= mAddedSum) {
		mAddedSum = (float)((double)mAddedSum - 1.0);
		UpdateTick();
	}
}

void GoodTube::UpdateTick()
{
	// 0x70a9bb
	float anArtScale = (float)((double)gApp->mArtRes / 1200.0);
	mUpdateCnt++;
	mColorCycle.Update();
	mCountTicker.mTarget = gBoard->mChallengeCount;
	mBestTicker.mTarget = gBoard->mChallengeBest;
	if (mBestSoFar <= gBoard->mChallengeBest)
		mBestSoFar = gBoard->mChallengeBest;
	mCountTicker.Update();
	mBestTicker.Update();

	// The bonus challenge's bar: 1 the colours light up one by one, 2 they wait for the player, 4 they fade away
	Board* aBoard = gBoard;
	int aState = mBonusState;
	if (aState != 0) {
		if (aState == 1) {
			int aCount = (int)aBoard->mBonusColors.size();
			if (aCount != 0) {
				mBonusTimer = (float)((double)mBonusTimer - 0.0035000001080334187);
				if (mBonusTimer < 0.0f) {
					mBonusState = 2;
					mBonusTimer = 1.0f;
				}
				int aLight;
				if (mBonusTimer <= 0.25f) {
					aLight = -1;
					sBonusLight = -1;
				} else {
					aLight = Ftol(((1.0 - (double)mBonusTimer) * (double)aCount) * 4.0);
					aLight = aLight % aCount;
				}
				if (aLight != sBonusLight)
					sBonusLight = aLight;
				int i = 0;
				if (0 < aCount) {
					do {
						float aGlow;
						if (i == sBonusLight) {
							aGlow = (float)((double)mBonusGlowBottom[i] + 0.05000000074505806);
						} else {
							aGlow = 0.0f;
							if (0.0 < (double)mBonusGlowBottom[i] - 0.019999999552965164)
								aGlow = (float)((double)mBonusGlowBottom[i] - 0.019999999552965164);
						}
						i++;
						mBonusGlowBottom[i - 1] = aGlow;
					} while (i < aCount);
				}
			}
		} else if (aState == 2) {
			int aCount = (int)aBoard->mBonusColors.size();
			if (aCount != 0) {
				float aLastProgress = mBonusTimer;
				if (0.5f < mBonusTimer ||
					(aBoard->mTutorial == nullptr && aBoard->mHintsReady && aBoard->mShownDialog == nullptr))
					mBonusTimer = (float)((double)mBonusTimer - 0.004999999888241291);
				float aFade = (float)((double)mBonusFade + 0.019999999552965164);
				if (1.0f < aFade)
					aFade = 1.0f;
				mBonusFade = aFade;
				if (mBonusTimer < 0.800000011920929) {
					float aLeft = (float)((double)mBonusGlowFade - 0.009999999776482582);
					if (aLeft < 0.0f)
						aLeft = 0.0f;
					mBonusGlowFade = aLeft;
					for (int i = 0; i < aCount; i++) {
						float aValue = 0.0f;
						if (0.0 < (double)mBonusGlowTop[i] - 0.05000000074505806)
							aValue = (float)((double)mBonusGlowTop[i] - 0.05000000074505806);
						mBonusGlowTop[i] = aValue;
					}
				}
				if (mBonusTimer < 0.800000011920929 && 0.800000011920929 <= aLastProgress)
					mBonusSparkTimer = 8.0f;
				if (mBonusTimer < 0.30000001192092896f && 0.30000001192092896f <= aLastProgress) {
					Piece* aHint = aBoard->FindHintPiece();
					if (aHint == nullptr) {
						mBonusSparkTimer = (float)((double)mBonusSparkTimer + 4.0);
					} else {
						// 0x74b94e: a FruitShockwave at the hint's gem, with the tube's middle for its target, then 0x6bc3cd adds it
						// to the board's waves
						aBoard->mShockwaves.push_back(new FruitShockwave(aHint, 1, Ftol((double)mWidth * 0.5 + (double)mX),
							Ftol((double)mHeight * 0.8500000238418579 + (double)mY), 200));
					}
				}
				if (mBonusTimer <= 0.0f) {
					mBonusState = 4;
					mBonusTimer = 1.0f;
				}
			}
		} else if (aState == 4) {
			if (mBonusShown <= 0.0f) {
				mBonusTimer = (float)((double)mBonusTimer - 0.009999999776482582);
				if (mBonusTimer <= 0.0f) {
					mBonusSwap.SetInVal(0.0);
					mBonusFade = 0;
					mBonusState = 0;
					mBoard->StartBonusChallenge(-1);
				}
			} else {
				float aShown = (float)((double)mBonusShown - 0.029999999329447746);
				mBonusShown = aShown;
				if (!(0.0f < aShown)) {
					mBonusShown = 0;
					mBonusFlash = 1.0f;
					mBonusGlowFade = 1.0f;
				}
				mBonusTimer = 1.5f;
			}
		}
	}

	if (mChallenge && aBoard->mChallenge != 0 && (aBoard->mChallenge->mFlags & Challenge::FLAG_PATTERN)) {
		// 0x70ae48: the pattern's count shown walks to the matches so far, the curve restarting at each step down
		int aProgress = aBoard->mChallenge->mProgress;
		if (mGoalShown != aProgress) {
			if (0.0 <= mGoalMove.mIncRate || mGoalMove.HasBeenTriggered()) {
				if (mGoalShown > aProgress)
					mGoalShown--;
				mGoalMove.SetCurve("b+1,1.5,0.033333,1,####    n~###     3####");
			}
		}
		if (!mGoalMove.HasBeenTriggered() && !mGoalMove.IncInVal() && mGoalShown < aProgress)
			mGoalShown++;
	}

	if (mLastX != mX)
		mLastX = mX;
	if (0.0f < mLevelFlash) {
		float aValue = (float)((double)mLevelFlash - 0.009999999776482582);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mLevelFlash = aValue;
	}
	if (mEaseValue <= mEaseGoal) {
		if (mEaseValue < mEaseGoal) {
			float aValue = (float)((double)mEaseValue + 0.009999999776482582);
			if (1.0f < aValue)
				aValue = 1.0f;
			mEaseValue = aValue;
		}
	} else {
		float aValue = (float)((double)mEaseValue - 0.009999999776482582);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mEaseValue = aValue;
	}
	mFXManager->Update();
	if (mFlashLevel <= 0.0f) {
		mFlashBrightness -= 2;
		if (mFlashBrightness < 0)
			mFlashBrightness = 0;
	} else {
		float aValue = (float)((double)mFlashLevel - 0.009999999776482582);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mFlashLevel = aValue;
	}
	if (mFlashBrightness < 1) {
		mGlowAlpha = 0;
	} else {
		float aWave = (float)((double)mFlashLevel * 3.141590118408203);
		float aSin = (float)std::sin((double)aWave);
		float aScale = (float)((double)aSin + (double)aSin + 1.0);
		aScale = std::fabs(aScale);
		int aFlash = mFlashBrightness;
		int aGlow = aFlash - Ftol((double)aFlash * (double)mFlashLevel * -0.5 * (double)aScale);
		mGlowAlpha = aGlow;
		if (aGlow < 0x100) {
			if (aGlow < 0)
				aGlow = 0;
		} else {
			aGlow = 0xff;
		}
		mGlowAlpha = aGlow;
	}
	mFillTop = GetGlassTop() + 0x14;
	mFillBottom = GetBottomTop();
	// port: the two text popups at +0x1cc and +0x1d0 are updated here (a popup that is done goes); nothing in the exe
	// makes them

	if (0.0f < mLightFade) {
		float aValue = (float)((double)mLightFade - 0.00800000037997961);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mLightFade = aValue;
	}
	if (0.0f < mLevelChangeFlash) {
		float aValue = (float)((double)mLevelChangeFlash - 0.009999999776482582);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mLevelChangeFlash = aValue;
	}

	// The specks rise in the tube
	unsigned int aSpeck = 0;
	if (mSpecks.size() != 0) {
		int aMargin = Ftol((double)anArtScale * 23.0);
		do {
			Speck* aSpeckPtr = mSpecks[aSpeck];
			float aWaveT = (float)((double)mUpdateCnt * 0.009999999776482582 + (double)(int)aSpeck);
			float aWaveSin = (float)std::sin((double)aWaveT);
			float aDX = (float)((double)aWaveSin * 0.009999999776482582 + (double)aSpeckPtr->mDX);
			aSpeckPtr->mDX = aDX;
			aSpeckPtr->mX = (float)((double)aSpeckPtr->mX + (double)aDX);
			aSpeckPtr->mY = (float)((double)aSpeckPtr->mDY + (double)aSpeckPtr->mY);
			aSpeckPtr->mDY = (float)((double)aSpeckPtr->mDDY + (double)aSpeckPtr->mDY);
			int aMiddle = mGlassImage->mWidth / 2 - 3 + mGlassX;
			float aLow = (float)(aMiddle - aMargin);
			if (aLow <= aSpeckPtr->mX) {
				float aHigh = (float)(aMiddle + aMargin);
				if (aHigh < aSpeckPtr->mX) {
					aSpeckPtr->mX = aHigh;
					aSpeckPtr->mDX = (float)((double)std::fabs(aDX) * -0.5);
				}
			} else {
				aSpeckPtr->mX = aLow;
				aSpeckPtr->mDX = (float)((double)std::fabs(aDX) * 0.5);
			}
			if (aSpeckPtr->mY < mFillTopShown) {
				aSpeckPtr->mAlpha = (float)((double)aSpeckPtr->mDAlpha + (double)aSpeckPtr->mAlpha);
				aSpeckPtr->mDDY = 0.019999999552965164f;
			}
			if (aSpeckPtr->mAlpha <= 0.0f || (mFillTopShown <= aSpeckPtr->mY && 0.0f < aSpeckPtr->mDDY)) {
				mSpecks[aSpeck] = mSpecks[mSpecks.size() - 1];
				if (mSpecks.size() != 0)
					mSpecks.pop_back();
				aSpeck--;
				delete aSpeckPtr;
			}
			aSpeck++;
		} while (aSpeck < mSpecks.size());
	}
	mTurnSlow = (float)((double)6.2831854820251465 / 200.0 + (double)mTurnSlow);
	if (6.2831854820251465 < (double)mTurnSlow)
		mTurnSlow = (float)((double)mTurnSlow - 6.2831854820251465);
	{
		float aTurn = (float)((double)6.2831854820251465 / 400.0 + (double)mTurnSlower);
		mTurnSlower = aTurn;
		if (6.2831854820251465 < (double)aTurn)
			mTurnSlower = (float)((double)aTurn - 6.2831854820251465);
	}

	int aMode = mTimeKind;
	if (aMode < 1 || 2 < aMode) {
		float aValue = (float)((double)mBlitzPulse - 0.029999999329447746);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mBlitzPulse = aValue;
	} else {
		float aValue = (float)((double)mBlitzPulse + 0.05000000074505806);
		if (1.0f < aValue)
			aValue = 1.0f;
		mBlitzPulse = aValue;
	}
	if (0 < aMode && mBoard->mHyperspace == nullptr) {
		float aValue = (float)((double)mTimePulse + 0.019999999552965164);
		if (1.0f < aValue)
			aValue = 1.0f;
		mTimePulse = aValue;
	}
	if (aBoard->mGameMode == Board::MODE_BLITZ) {
		if (aMode == 1) {
			aBoard->mSettleWait = 0.5f;
			if (1.0f <= mTimePulse)
				mTimeKind++;
		} else if (aMode == 2) {
			aBoard->mSettleWait = 0.5f;
			if (mUpdateCnt % 5 == 0) {
				int aMax = mTimeLimit;
				mTimeShown += 1000;
				int aTime = mTimeShown;
				if (aMax <= aTime) {
					mBlitzTimer.mIncRate = 0.019999999552965164;
					mTimeShown = aMax;
					mBlitzTimer.SetInVal(mBlitzTimer.mInMin);
					gApp->PlaySample(SOUND_TALLY_ZOOMIN);
					mTimeKind++;
				} else {
					float aRatio = (float)((double)aTime / (double)aMax);
					float aPitch = (float)((double)aRatio * 5.0);
					gApp->PlaySoundEx(SOUND_BLITZ_COUNTUP, 0, aPitch, 0.0f, true, true);
				}
			}
		} else if (aMode == 3) {
			aBoard->mSettleWait = 0.5f;
			if (!mBlitzTimer.IncInVal())
				mTimeKind++;
		}
	}
	UpdateFill(false);
	int aSteps = Board::GetLevelSteps(mShownLevel);
	int aLit = mChainSteps + 1;
	if (aSteps < aLit)
		aLit = aSteps;
	mLightsLit = aLit;
	if (0 < aSteps) {
		int aLight = aSteps;
		float* aFlash = mLightFlash;
		do {
			float aValue = (float)((double)*aFlash - 0.009999999776482582);
			if ((double)*aFlash - 0.009999999776482582 < 0.0)
				aValue = 0.0f;
			*aFlash = aValue;
			aFlash++;
			aLight--;
		} while (aLight != 0);
	}
	int aLastLit = mLightsLitBefore;
	int aNowLit = mLightsLit;
	if (aLastLit != aNowLit) {
		int aRow = (aNowLit - 1 + ((aNowLit - 1) >> 31 & 3)) >> 2;
		if (aRow != mLastLightRow)
			mLastLightRow = aRow;
		if (aLastLit < aNowLit) {
			float* aFlash = &mLightFlash[aLastLit];
			do {
				aLastLit++;
				*aFlash = 1.0f;
				aFlash++;
			} while (aLastLit < mLightsLit);
		}
	}

	if (mProgress != 0.0f && 0 < mWidth + mX && (mSpeckWait = mSpeckWait - 1, mSpeckWait < 1)) {
		mSpeckWait = Rand() % 20 + 5;
		if ((double)GetGlassTop() < (double)mFillTopShown) {
			Speck* aSpeck = new Speck();
			int aWidth = mGlassImage->mWidth;
			int aRand = Rand();
			aSpeck->mX = (float)(aRand % (aWidth / 2 - 15) + ((aWidth + (aWidth >> 31 & 3)) >> 2) + mGlassX);
			aSpeck->mY = (float)GetBottomTop();
			aSpeck->mDY = (float)-((double)(Rand() % 10) / 10.0 + 0.5);
			aSpeck->mAlpha = 1.0f;
			double aTopY = (double)aSpeck->mY;
			aSpeck->mImage = IMAGE_SPARKLET;
			if ((double)(GetGlassTop() + 0x50) <= aTopY)
				aSpeck->mDAlpha = -0.01f;
			else
				aSpeck->mDAlpha = (float)-(((aTopY - (double)GetGlassTop()) * 0.019999999552965164) / 80.0 + 0.019999999552965164);
			aSpeck->mDDY = -0.001f;
			aSpeck->mScale = (float)((double)std::fabs(RandSigned()) * 0.25 + 0.75);
			if ((Rand() & 1) == 0) {
				aSpeck->mScale = (float)((double)aSpeck->mScale * 0.5);
				aSpeck->mImage = IMAGE_SPARKLET;
			} else {
				aSpeck->mImage = IMAGE_INGAMEUI_TUBE_FILL_PARTICLE;
			}
			mSpecks.push_back(aSpeck);  // 0x707b2a
		}
	}
	for (unsigned int i = 0; i < mBubbles.size(); i++) {
		Bubble* aBubble = mBubbles[i];
		float aPhase = (float)((double)aBubble->mPhase + (double)aBubble->mPhaseStep);
		aBubble->mPhaseStep = aPhase;
		if (6.2831854820251465 < (double)aPhase)
			aBubble->mPhaseStep = (float)((double)aPhase - 6.2831854820251465);
	}
	if (0.0f < mTipTime) {
		mTipTime = (float)((double)mTipTime - 0.009999999776482582);
		float aValue = (float)((double)mTipFade + 0.03999999910593033);
		if (1.0f < aValue)
			aValue = 1.0f;
		mTipFade = aValue;
	} else {
		float aValue = (float)((double)mTipFade - 0.03999999910593033);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mTipFade = aValue;
	}

	// The light the chain is on eases to the chain's steps
	float aGap = (float)mChainSteps - mShownSteps;
	float aMore = (float)((double)Board::GetLevelSteps(mShownLevel) * 0.09375 + 0.25);
	if (!(std::fabs(aGap) <= 0.004999999888241291f)) {
		float aNext;
		if ((float)mChainSteps < mShownSteps) {
			double aDrop = ((double)aGap * 0.009999999776482582 - 0.029999999329447746) * (double)aMore;
			if (aDrop <= (double)aGap * 0.9900000095367432)
				aDrop = (double)aGap * 0.9900000095367432;
			aNext = (float)(aDrop + (double)mShownSteps);
		} else if (mShownSteps < (float)mChainSteps) {
			double aRise = (double)aGap * 0.05000000074505806 + 0.05000000074505806;
			if ((double)aGap * 0.9900000095367432 <= aRise)
				aRise = (double)aGap * 0.9900000095367432;
			aNext = (float)(aRise + (double)mShownSteps);
		} else {
			aNext = mShownSteps;
		}
		mShownSteps = aNext;
	} else {
		mShownSteps = (float)mChainSteps;
	}
	float aShownSteps = (float)Board::GetLevelSteps(mShownLevel);
	if (aShownSteps < mShownSteps && mLevelSoundOn && 0.0 < (double)mShownSteps - (double)aShownSteps) {
		int aLightId = 0;
		int aHue0 = Board::GetLevelSteps(mShownLevel) * 0x28;
		do {
			int aPointA[2];
			int aPointB[2];
			GetStepPoint(aPointA, aLightId, false);
			int aLightX = aPointA[0];
			GetStepPoint(aPointB, aLightId, false);
			int aLightY = aPointB[1];
			Image* aLightImage = GetImageById(sFirstLight[mShownLevel] + aLightId);
			float aRX = (float)(Rand() % aLightImage->mWidth);
			float aRY = (float)(Rand() % aLightImage->mHeight);
			unsigned int aPixel = GetPixelAt(aLightImage, Ftol((double)aRX), Ftol((double)aRY));
			if (aPixel > 0x80000000u) {
				Effect* anEffect = mFXManager->AllocEffect(0);
				anEffect->mIsAdditive = true;
				anEffect->mX = (float)((double)aLightX + (double)aRX);
				anEffect->mImage = IMAGE_GEM_FRUIT_SPARK;
				anEffect->mY = (float)((double)aLightY + (double)aRY);
				anEffect->mDX = 0;
				anEffect->mDY = 0;
				anEffect->mGravity = 0;
				anEffect->mAngle = 0;
				anEffect->mDAngle = 0;
				anEffect->mAlpha = 1.0f;
				anEffect->mDAlpha = -0.05f;
				int aHue = (Rand() % 120 - Ftol((double)mUpdateCnt * -1.5)) + aHue0;
				aHue = aHue % 256;
				anEffect->mColor = Color((int)HSLToColor(aHue, 0x80, 0x40));
				anEffect->mScale = 0.2f;
				anEffect->mDScale = -0.0003f;
				mFXManager->AddEffect(anEffect);
			}
			aLightId++;
			aHue0 -= 0x28;
		} while ((double)aLightId < (double)mShownSteps - (double)aShownSteps);
	}
	if (0.0f < mSparkBurst) {
		// Sparks burst from the rank badge's middle (0x702078, 0x70208a)
		int aBadgeX = mRankBadge->mWidth / 2 + mRankBadge->mX;
		int aBadgeY = mRankBadge->mHeight / 2 + mRankBadge->mY;
		mSparkBurst = (float)((double)mSparkBurst - 0.009999999776482582);
		float aCentreX = (float)aBadgeX;
		float aCentreY = (float)aBadgeY;
		int aRounds = 3;
		do {
			float anAngle = (float)((double)RandSigned() * 1.100000023841858);
			float aSpread = (float)RandSigned();
			float aSpeed = (float)((double)std::fabs(aSpread) + 2.0);
			int i = 0;
			do {
				Effect* anEffect = mFXManager->AllocEffect(0);
				anEffect->mImage = IMAGE_SPARKLET;
				anEffect->mX = (float)((double)(float)std::sin(anAngle) * 80.0 * (double)gApp->mScale + (double)aCentreX);
				anEffect->mY = (float)((40.0 - (double)(float)std::cos(anAngle) * 30.0) * (double)gApp->mScale + (double)aCentreY);
				anEffect->mDX = (float)std::sin(anAngle) * aSpeed;
				anEffect->mDY = (float)((double)(float)std::cos(anAngle) * -1.0 * (double)aSpeed);
				anEffect->mAlpha = 1.0f;
				anEffect->mDAlpha = -0.02f;
				anEffect->mScale = (float)((double)RandSigned() + 4.0);
				anEffect->mColor = Color(255, 200, 16);
				anEffect->mIsAdditive = true;
				if (i == 1) {
					anEffect->mScale = (float)((double)anEffect->mScale * 0.25);
					anEffect->mColor = Color::White;
					anEffect->mIsAdditive = true;
				}
				mFXManager->AddEffect(anEffect);
				i++;
			} while (i < 2);
			aRounds--;
		} while (aRounds != 0);
	}
	mLightsLitBefore = mLightsLit;

	// The glass breaks (mBreakState is 1 then)
	if (mBreakState == 1) {
		if ((int)mShardRows.size() < 2 && mGlassGone < 1.0f) {
			double aFall = ((double)mGlassGone * 0.0 + (double)mGlassGone) + 0.009999999776482582;
			if (1.0 < aFall)
				aFall = 1.0;
			mGlassGone = (float)aFall;
		}
		if (1.0f <= mGlassGone) {
			float aFloor = (float)((double)anArtScale * 420.0);
			bool aBefore = mTopFall <= aFloor;
			mTopFall = (float)((double)anArtScale * (double)mTopFallSpeed + (double)mTopFall);
			float aSpeed = (float)((double)mTopFallSpeed + 0.07500000298023224);
			mTopFallSpeed = aSpeed;
			if (aFloor < mTopFall) {
				if (aBefore) {
					mTopFallSpeed = (float)((double)aSpeed * 0.4000000059604645);
					gApp->PlaySample(SOUND_BOMB_EXPLODE);
				}
				mTopBounce = mTopFall - aFloor;
				mTopSkew = mTopSkewStep + mTopSkew;
			}
			mTopTurn = mTopTurnSpeed + mTopTurn;
			int aTopY = GetTopY();
			int aShardY = Ftol((double)mTopImage->mHeight * 0.949999988079071 + (double)aTopY);
			if (gBoard->mHeight < aTopY) {
				mBreakState = 2;
			} else if (mUpdateCnt % 10 == 0 && mShardShare < 1.0f) {
				double aShare = (double)(aShardY - GetGlassTop()) / (double)mGlassImage->mHeight - 0.10000000149011612;
				if (1.0 < aShare)
					aShare = 1.0;
				float aShareF = (float)aShare;
				mShardShare = aShareF;
				if (!(0.0f <= aShareF))
					mShardShare = 0;
				float aShardBase = (float)aShardY;
				for (int aShard = 10; aShard != 0; aShard--) {
					Effect* anEffect = gBoard->mFXManager->AllocEffect(0);
					anEffect->mDX = (float)((double)RandSigned() * 6.0);
					anEffect->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
					anEffect->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.019999999552965164);
					anEffect->mX = (float)((double)(mWidth / 2 + mX) + (double)anEffect->mDX * 10.0);
					anEffect->mY = (float)((double)std::fabs(RandSigned()) * 10.0 + (double)aShardBase);
					anEffect->mIsAdditive = false;
					anEffect->mDY = (float)((double)RandSigned() * 0.4000000059604645 + (double)mTopFallSpeed * 0.5);
					anEffect->mAlpha = 1.0f;
					anEffect->mDAlpha = -0.01f;
					anEffect->mGravity = 0.03f;
					anEffect->mImage = IMAGE_GLASS_SHARD1;
					float aGrow = (float)((double)RandSigned() * 0.004000000189989805);
					anEffect->mDScale = aGrow;
					float aScale = (float)((double)aGrow * 10.0 + 1.0);
					anEffect->mScale = aScale;
					double aShade = (double)aScale * 128.0;
					if (255.0 < aShade)
						aShade = 255.0;
					int aGrey = Ftol(aShade);
					anEffect->mColor = Color(aGrey, aGrey, aGrey);
					gBoard->mFXManager->AddEffect(anEffect);
				}
				for (int aDust = 0; aDust < 10; aDust++) {
					Effect* anEffect = gBoard->mFXManager->AllocEffect(0x1a);
					float aBoost = (float)((double)RandSigned() + 2.0);
					float aSpread = std::fabs((float)RandSigned());
					anEffect->mType = 0;
					float aPush = (float)(((double)aSpread * 0.05000000074505806 + 0.25) * (double)aBoost);
					double aFlight = (double)aBoost * 0.10000000149011612;
					if (1.0 < aFlight)
						aFlight = 1.0;
					anEffect->mAlpha = (float)aFlight;
					anEffect->mDAlpha = -0.01f;
					anEffect->mDScale = 0.05f;
					anEffect->mScale = 1.5f;
					anEffect->mMaxScale = 1.5f;
					anEffect->mBlend = 0.25f;
					anEffect->mRateFrom = -0.01f;
					anEffect->mRateTo = 0.95f;
					aPush = (float)((double)aPush * 15.0);
					float anAngle = (aDust & 1) == 0 ? -0.31415927f : 3.4557521f;
					float aCos = (float)std::cos((double)anAngle);
					anEffect->mDX = (float)((double)aCos * (double)aPush);
					float aSin = (float)std::sin((double)anAngle);
					anEffect->mDY = (float)((double)aSin * (double)aPush);
					anEffect->mX = (float)((double)(mWidth / 2 + mX) + (double)anEffect->mDX * 10.0);
					anEffect->mY = (float)((double)std::fabs((float)RandSigned()) * 10.0 + (double)aShardBase);
					gBoard->mFXManager->AddEffect(anEffect);
				}
			}
		}
	}

	// The fill's glow and the bonus challenge's bar
	int aBonusCount = (int)aBoard->mBonusColors.size();
	int aPhase = mBoard->mBonusStage;
	float aShown;
	bool aShownSet = true;
	if (aPhase == 2 || aPhase == 5) {
		aShown = (float)((double)mBonusShown - 0.02500000037252903);
	} else if (aPhase == 4 && mBoard->mFruitDance != nullptr && 0x64 < mBoard->mFruitDance->mUpdateCnt) {
		aShown = (float)((double)mBonusShown - 0.009999999776482582);
	} else {
		aShownSet = false;
		aShown = 0;
		if (aBoard->mGameMode == Board::MODE_CHALLENGE) {
			if (mBonusShown < 1.0f) {
				float aValue = (float)((double)mBonusShown + 0.029999999329447746);
				mBonusShown = aValue;
				if (1.0f < aValue)
					mBonusShown = 1.0f;
			}
			if (mBestFade.GetOutVal() == 0.0 && mBestFade.mIncRate == 0.0) {
				if (aBoard->mChallengeCount < aBoard->mChallengeBest)
					mBestFade.SetCurve(GTS(0x7d8630));
			} else {
				mBestFade.IncInVal();
			}
			mBestFadeOut = (float)mBestFade.GetOutVal();
		} else if (aBonusCount != 0 && mBonusState == 0) {
			if (!mBonusListSet) {
				mBonusListSet = true;
				mBonusGlowTop.resize(mBonusGlowTop.size() <= (unsigned int)aBonusCount ? (unsigned int)aBonusCount
																					: mBonusGlowTop.size());
				for (unsigned int i = 0; i < (unsigned int)aBonusCount; i++)
					mBonusGlowTop[i] = 1.0f;
			}
			float aValue = (float)((double)mBonusShown + 0.029999999329447746);
			mBonusShown = aValue;
			if (1.0f < aValue)
				mBonusShown = 1.0f;
			if (mBonusTarget == aBoard->mBonusMatched) {
				if (mBonusShownCount == mBonusTarget) {
					float aSpin = (float)(6.2831854820251465 * 0.003000000026077032 + (double)mBonusSpin);
					mBonusSpin = aSpin;
					if (6.2831854820251465 <= (double)aSpin)
						mBonusSpin = (float)((double)aSpin - 6.2831854820251465);
				} else if (!mBonusSwap.IncInVal()) {
					mBonusShownCount = mBonusTarget;
					mBonusSwap.SetInVal(0.0);
					mBonusSwap.SetCurve(GTS(0x7e3e80));  // 0x704d9c
				}
			} else {
				mBonusSpin = 0;
				mBonusTarget = aBoard->mBonusMatched;
				mBonusSwap.SetInVal(0.0);
			}
			for (unsigned int i = 0; i < (unsigned int)aBonusCount; i++) {
				float aGlow;
				if (i < (unsigned int)mBonusShownCount) {
					aGlow = 0.5f;
					if (0.5 <= (double)mBonusGlowTop[i] - 0.009999999776482582)
						aGlow = (float)((double)mBonusGlowTop[i] - 0.009999999776482582);
				} else {
					aGlow = 1.0f;
					if ((double)mBonusGlowTop[i] + 0.019999999552965164 <= 1.0)
						aGlow = (float)((double)mBonusGlowTop[i] + 0.019999999552965164);
				}
				mBonusGlowTop[i] = aGlow;
			}
		}
	}
	if (aShownSet) {
		if (aShown < 0.0f)
			aShown = 0.0f;
		mBonusShown = aShown;
	}
	if (mBonusFlash != 0.0f) {
		float aValue = (float)((double)mBonusFlash - 0.019999999552965164);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mBonusFlash = aValue;
	}
	if (0.0f < mBonusSparkTimer) {
		int aWas = Ftol((double)mBonusSparkTimer);
		float aLeft = (float)((double)mBonusSparkTimer - 0.10000000149011612);
		mBonusSparkTimer = aLeft;
		int aNow = Ftol((double)aLeft);
		if (-1 < aNow && aNow != aWas) {
			int aSteps = mChainSteps;
			int aTravellers = (int)mBoard->mSmokeTravellers.size();
			int aPoint[2];
			GetStepPoint(aPoint, aSteps + aTravellers, true);
			Color aColor(255, 255, 0);
			float aFromX = (float)(((double)RandSigned() * 0.20000000298023224 + 0.5) * (double)mWidth + (double)mX);
			float aFromY = (float)((double)mHeight * 0.8199999928474426 + (double)mY);
			SmokeTraveller* aSmoke = new SmokeTraveller(aColor, Ftol((double)aFromX), Ftol((double)aFromY),
				mX + aPoint[0], mY + aPoint[1], 0, true);
			aSmoke->mHurry = 0.4f;
			aSmoke->mBurst = true;
			mBoard->mSmokeTravellers.push_back(aSmoke);  // 0x6b9ccc
		}
	}
	if (mBonusGlowCurve.GetOutVal() > 0.0 && mBoard->mBoardZoom == 1.0f && mBoard->mFruitDance == nullptr) {
		// 3 sparks a frame round the bottom's middle, each at one of 512 angles and that angle's radius (the table at
		// 0x7e3390, 512 floats of game data), flying outwards
		float aMidX = (float)Ftol((double)mWidth * 0.5249999761581421);
		float aMidY = (float)Ftol((double)mBottomImage->mHeight * 0.36000001430511475 + (double)GetBottomTop());
		int aSpark = 3;
		do {
			Effect* anEffect = mFXManager->AllocEffect(0);
			int aStep = Rand() % 512;
			float anAngle = (float)((double)aStep * 0.001953125 * (double)6.2831855f);
			int aRadiusBits = GameInt(GTB(0x7e3390, 2048) + aStep * 4);
			float aRadiusF;
			std::memcpy(&aRadiusF, &aRadiusBits, sizeof(aRadiusF));
			float aRadius = aRadiusF * gApp->mScale;
			anEffect->mImage = IMAGE_GEM_FRUIT_SPARK;
			anEffect->mIsAdditive = true;
			anEffect->mX = (float)((double)(float)std::cos(anAngle) * (double)aRadius + (double)aMidX);
			anEffect->mY = (float)((double)(float)std::sin(anAngle) * (double)aRadius + (double)aMidY);
			float aCos = (float)std::cos(anAngle);
			anEffect->mDX = (float)((double)gApp->mScale * 1.100000023841858) * aCos;
			float aSin = (float)std::sin(anAngle);
			anEffect->mDY = (float)((double)gApp->mScale * 1.100000023841858) * aSin;
			anEffect->mAlpha = (float)mBonusGlowCurve.GetOutVal();
			anEffect->mScale = 0.5f;
			anEffect->mGravity = 0;
			anEffect->mDScale = -0.01f;
			anEffect->mDAlpha = -0.01f;
			anEffect->mDAngle = (float)((double)RandSigned() * 0.019999999552965164 + 0.05000000074505806);
			mFXManager->AddEffect(anEffect);
			aSpark--;
		} while (aSpark != 0);
	}
}

void GoodTube::Draw(Graphics* g)
{
	// 0x70a974
	DeferOverlay(40);
	g->SetColor(Color::White);
	DrawGlow(g);
	DrawFill(g, false);
	DrawBody(g);
	DrawFill(g, true);
}

void GoodTube::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	// 0x70623f
	if (mGlassHeight == 0)
		return;
	// With Direct3D the board's jump curves pick the shader that fades the tube (the whiteout, or its fixed-function
	// class on a card without pixel shaders, local_11 in the original) and set its parameters
	bool aFixedFunction = true;
	if (gD3DInterface != nullptr)
		aFixedFunction = (gD3DInterface->GetCaps(0) & 2) == 0;
	Hyperspace* aSpace = mBoard->mHyperspace;
	if (aSpace != nullptr && gD3DInterface != nullptr) {
		if (0.0 < aSpace->mSidebarGlance.GetOutVal() && !aSpace->mIs3D)
			aSpace->Draw3DSidebar();  // 0x711529
		if (aSpace->mTubeBlendBase.GetOutVal() != 0.0 || aSpace->mSidebarGlance.GetOutVal() != 0.0) {
			if (aSpace->mTubeAlpha.GetOutVal() == 0.0) {
				gD3DInterface->SetPixelShader(nullptr);  // slot 0xd8: the tube is hidden altogether
				return;
			}
			if (aFixedFunction) {
				// 0x501630: WhiteoutFixedFunctionShader's parameters (0x85d6ec, 0x85d6f0) and its ladder
				NotPorted("GoodTube::DrawAll: WhiteoutFixedFunctionShader 0x501630 (a card without ps_2_0)");
			} else {
				gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_WHITEOUT_FINAL]);  // slot 0xd8, 0x85e0d8
				float aParams[4];
				aParams[0] = (float)aSpace->mTubeBlendBase.GetOutVal();
				aParams[1] = (float)aSpace->mTubeAlpha.GetOutVal();
				aParams[2] = 0;
				aParams[3] = 0;
				gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_WHITEOUT_FINAL], 0, aParams);  // slot 0x108
			}
		}
	}
	TwistWidget::DrawAll(theFlags, g);
	if (gD3DInterface != nullptr) {
		if (!aFixedFunction)
			gD3DInterface->SetPixelShader(nullptr);  // slot 0xd8
		// (a card without pixel shaders: 0x501890 takes the fixed-function class off again)
	}

	int anArtScale = Ftol((double)gApp->mScale * 10.0);
	int aTimeX = Ftol((double)mWidth * 0.5 + (double)anArtScale);
	int aTimeOffsetY = Ftol((double)gApp->mScale * -20.0);
	int aTimeY = Ftol((double)mFillTopShown) - aTimeOffsetY;
	if (gD3DInterface == nullptr)
		aTimeY++;  // (the original adds 1 unless Direct3D draws)
	int aCenterX, aCenterY;
	mBoard->GetCenter(&aCenterX, &aCenterY);
	int aToX = aCenterX - mX;
	int aToY = aCenterY - mY;
	int aX = Ftol((double)(aToX - aTimeX) * mBlitzTimer.GetOutVal() + (double)aTimeX);
	int anY = Ftol((double)(aToY - aTimeY) * mBlitzTimer.GetOutVal() + (double)aTimeY);
	double aGrowth = mBlitzTimer.GetOutVal();
	float aScale = (float)(aGrowth + aGrowth + 0.15000000596046448);
	int aTenths = mTimeShown / 100;
	if (0.0f < mBlitzPulse) {
		g->SetColor(Color(0, 0, 0, Ftol((double)mBlitzPulse * 100.0)));
		g->FillRect(-mX, -mY, gApp->mWidth, gApp->mHeight);
		g->SetColor(Color::White);
	}
	if (mTimeKind == 0)
		return;
	if (!(0.0f < mTimePulse))
		return;
	Color aColorA;
	Color aColorB;
	if (aTenths < 10 && mShowTenths) {
		float aWave = (float)std::sin((double)(float)((double)(mUpdateCnt % 75) * 3.1415927410125732 / 75.0));
		aColorA = Color(Ftol((double)aWave * 255.0), 0x20, 0x20);
		aColorB = Color(Ftol((1.0 - (double)aWave) * 255.0), 0x20, 0x20);
	} else if (mTimeKind < 4) {
		ColorCycle aCycle;
		aCycle.SetPosition((float)((double)(mUpdateCnt % 100) / 100.0));
		aColorA = aCycle.mColor;
		aColorB = Color::Black;
	} else {
		aColorA = Color::Black;
		aColorB = Color::White;
	}
	aColorA.mAlpha = Ftol((double)mTimePulse * 255.0);
	DrawTime(g, mTimeShown, aX, anY, aColorA, aColorB, aScale, mDrawTime);
}

void GoodTube::DrawTime(Graphics* g, int theTime, int theX, int theY, const Color& theColor, const Color& theColorTop,
	float theScale, bool theHundredths)
{
	// 0x704dde: blitz's clock in FONT_COUNTER, a character at a time, mTimePulse opaque
	Font* aFont = FONT_COUNTER;
	g->SetFont(aFont);
	// the font's top layer is drawn in theColorTop, the others in theColor
	int aLayer = (int)static_cast<ImageFont*>(aFont)->mFontData->mFontLayerList.size() - 1;  // (FONT_COUNTER is an ImageFont)
	const Color* aLayerColor = &theColorTop;
	while (true) {
		SetFontLayerColor(aFont, aLayer, *aLayerColor);
		aLayer--;
		if (aLayer < 0)
			break;
		aLayerColor = &theColor;
	}
	int aSeconds = theTime / 100;
	if (0 < theTime % 100 && !theHundredths)
		aSeconds++;
	SexyString aText = StrFormat(GTC(0x7dfdd4), aSeconds / 60, aSeconds % 60);
	if (theHundredths)
		aText += StrFormat(GTC(0x7e3ea4), theTime % 100);
	int aCharWidth = Ftol((double)gApp->mScale * 75.0);
	float aGrow = (float)((double)gApp->mScale * 9.0);
	double aExtra = (double)aGrow * (double)theScale;
	if (0.0 <= aExtra) {
		aGrow = (float)((double)gApp->mScale * 9.0);
		aExtra = (double)aGrow * (double)theScale;
	} else {
		aExtra = 0.0;
	}
	aCharWidth = Ftol(aExtra + (double)aCharWidth);
	unsigned int aLength = (unsigned int)aText.length();
	int aStartX = theX - (int)(aLength * aCharWidth) / 2;
	int aDrawX = aStartX;
	g->SetColor(Color(255, 255, 255, Ftol((double)mTimePulse * 255.0)));
	// The scale about the clock, as a matrix for the device with Direct3D (0x44f860 on)
	SexyTransform2D aClockMatrix;
	aClockMatrix.Translate((float)-(mX + theX), (float)-(mY + theY));
	aClockMatrix.Scale(theScale, theScale);
	aClockMatrix.Translate((float)(mX + theX), (float)(mY + theY));
	g->PushState();
	if (!gApp->Is3DAccelerated()) {
		float aClamped = theScale;
		if (aClamped <= 0.2f)
			aClamped = 0.2f;
		if (1.5f <= aClamped)
			aClamped = 1.5f;
		g->SetScale(aClamped, aClamped, (float)theX, (float)theY);
		if (aClamped < 0.5f)
			g->SetFastStretch(false);
	} else {
		gD3DInterface->PushTransform(aClockMatrix, true);  // slot 0x14
	}
	if (gD3DInterface != nullptr) {
		// The clip is opened wide: the matrix may take the clock out of it
		g->mClipRect.mX = -1000;
		g->mClipRect.mY = -1000;
		g->mClipRect.mWidth += 2000;
		g->mClipRect.mHeight += 2000;
	}
	g->TranslateF(0.0f, (float)theY);
	unsigned int i = 0;
	if (aLength != 0) {
		int aHalfWidth = aCharWidth / 2;
		do {
			char aChar = (char)aText[i];
			Font* aCurFont = g->GetFont();
			SexyString aLabel = StrFormat(GTC(0x7e3e9c), (int)aChar);
			int aWidth = aCurFont->CharWidth((SexyChar)aChar);
			g->DrawString(aLabel, (aHalfWidth - aWidth / 2) + aDrawX, 0);
			aDrawX += aCharWidth;
			if (theScale < 0.4 && i == aLength - 2)
				aDrawX += 12;
			i++;
		} while (i < aLength);
	}
	g->PopState();
	if (gApp->Is3DAccelerated())
		gD3DInterface->PopTransform();  // slot 0x18
}

void GoodTube::DrawGlow(Graphics* g)
{
	// 0x704637
	float anArtScale = (float)((double)gApp->mArtRes / 1200.0);
	int aX = GetBottomX();
	int anY = GetBottomTop();
	g->SetColorizeImages(true);
	Color aGlowColor(255, 255, 255, mGlowAlpha);
	g->SetDrawMode(0);
	if (mBreakState < 1) {
		g->SetColor(Color(255, 255, 255));
		g->DrawImage(mBottomImage, aX, anY);
		g->SetFont(FONT_SCORE);
		if ((mBonusState == 0 || mBonusState == 4) && 0.0f < mBonusShown)
			DrawBonus(g);
		g->SetColor(Color::White);
		if (mBonusShown < 1.0f) {
			int aDoorX = Ftol((double)mWidth * 0.5199999809265137);
			int aDoorY = GetBottomTop();
			aDoorY = Ftol((double)mBottomImage->mHeight * 0.36500000953674316 + (double)aDoorY);
			bool aShaderDoor = false;
			if (gD3DInterface != nullptr) {
				// With Direct3D (and pixel shaders, and no jump) the bonus door is drawn with the button-cover shader
				// over the bottom half's picture
				unsigned long aCaps = gD3DInterface->GetCaps(0);
				Image* aDoorTop = IMAGE_DOORS_BONUS_TOP;
				if ((aCaps & 2) != 0 && gApp->mBoard->mHyperspace == nullptr && 0.0f < mBonusShown) {
					gD3DInterface->SetTexture(1, IMAGE_DOORS_BONUS_BOTTOM);  // slot 0xd4
					gD3DInterface->SetTextureWrap(0, false);                 // slot 0xcc
					gD3DInterface->SetTextureWrap(1, false);
					gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_BUTTONCOVER]);  // slot 0xd8, 0x85dbd8
					float aParams[4];
					aParams[0] = (float)((double)mBonusShown * 0.8500000238418579);
					aParams[1] = 0.0f;
					aParams[2] = 0.0f;
					aParams[3] = 0.0f;
					gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_BUTTONCOVER], 0, aParams);  // slot 0x108
					((MemoryImage*)aDoorTop)->mD3DFlags |= 0x80;
					int aTopY = aDoorY - aDoorTop->GetCelHeight() / 2;
					g->DrawImage(aDoorTop, aDoorX - aDoorTop->GetCelWidth() / 2, aTopY);
					gD3DInterface->SetPixelShader(nullptr);
					aShaderDoor = true;
				}
			}
			int aFrame = Ftol((double)mBonusShown * 10.0);
			Image* aDoor = IMAGE_INGAMEUI_TWISTDOORANIM;
			if (!aShaderDoor && aFrame < 10) {
				int aDoorTop = aDoorY - aDoor->GetCelHeight() / 2;
				g->DrawImageCel(aDoor, aDoorX - aDoor->GetCelWidth() / 2, aDoorTop, aFrame);
				Image* aDoorGlow = IMAGE_INGAMEUI_TWISTDOORGLOW;
				if (mBonusFlash != 0.0f) {
					g->SetDrawMode(1);
					g->SetColor(Color(255, 255, 255, Ftol((double)mBonusFlash * 255.0)));
					int aGlowTop = aDoorY - aDoorGlow->GetCelHeight() / 2;
					g->DrawImage(aDoorGlow, aDoorX - aDoorGlow->GetCelWidth() / 2, aGlowTop);
					g->SetDrawMode(0);
				}
			}
		}
	} else {
		g->SetColor(Color(255, 255, 255, 255));
	}
	g->SetDrawMode(1);
	g->SetColor(aGlowColor);
	Image* aBottomGlow = IMAGE_INGAMEUI_BOT_LIGHTGLOW;
	g->SetColor(Color(255, 255, 255, Ftol((1.0 - (double)mGlowCut) * (double)mGlowAlpha)));
	int aGlowY = Ftol((double)GetImageOffsetY(GetIdByImage(aBottomGlow)) * (double)anArtScale);
	int aGlowX = Ftol((double)GetImageOffsetX(GetIdByImage(aBottomGlow)) * (double)anArtScale);
	g->DrawImage(aBottomGlow, aGlowX, aGlowY);
}

// 0x84eff4: the bonus lights' images, a colour's unlit and lit
static const int sBonusLightIds[14] = {
	IMAGE_INGAMEUI_BONUS_RED_UNLIT_ID, IMAGE_INGAMEUI_BONUS_RED_LIT_ID,
	IMAGE_INGAMEUI_BONUS_WHITE_UNLIT_ID, IMAGE_INGAMEUI_BONUS_WHITE_LIT_ID,
	IMAGE_INGAMEUI_BONUS_GREEN_UNLIT_ID, IMAGE_INGAMEUI_BONUS_GREEN_LIT_ID,
	IMAGE_INGAMEUI_BONUS_YELLOW_UNLIT_ID, IMAGE_INGAMEUI_BONUS_YELLOW_LIT_ID,
	IMAGE_INGAMEUI_BONUS_PURPLE_UNLIT_ID, IMAGE_INGAMEUI_BONUS_PURPLE_LIT_ID,
	IMAGE_INGAMEUI_BONUS_ORANGE_UNLIT_ID, IMAGE_INGAMEUI_BONUS_ORANGE_LIT_ID,
	IMAGE_INGAMEUI_BONUS_BLUE_UNLIT_ID, IMAGE_INGAMEUI_BONUS_BLUE_LIT_ID,
};

void GoodTube::DrawBonus(Graphics* g)
{
	// 0x703bdf: the bonus challenge's text, or its lights over the bottom
	g->SetColorizeImages(true);
	int aCenterX = Ftol((double)mWidth * 0.5224999785423279);
	int aBaseY = Ftol((double)mBottomImage->mHeight * 0.38999998569488525 + (double)GetBottomTop());
	if (gBoard->mGameMode == 2) {
		g->SetFont(FONT_TOOLTIP);
		int aWidth = Ftol((double)mBottomImage->mWidth * 0.800000011920929);
		int aHeight = g->GetWordWrappedHeight(aWidth, mText, -1, nullptr);
		int aTop = Ftol(((double)mBottomImage->mHeight * 0.36500000953674316 + (double)GetBottomTop())
			- (double)(aHeight / 2));
		Rect aRect(aCenterX - aWidth / 2, aTop, aWidth, aHeight);
		g->WriteWordWrapped(aRect, mText, -1, 0, nullptr, -1, nullptr);
	} else if (0 < mLevel) {
		unsigned int aCount = (unsigned int)gBoard->mBonusColors.size();
		int aHeaderBottom;
		if (mBoard->mBonusStage < 3) {
			Image* aHeader = IMAGE_INGAMEUI_BONUS_HEADER;
			g->SetColor(Color(255, 255, 255, Ftol((double)mBonusGlowFade * 255.0)));
			int aHeaderY = Ftol((double)mBottomImage->mHeight * 0.20499999821186066 + (double)GetBottomTop());
			g->DrawImage(aHeader, aCenterX - aHeader->mWidth / 2, aHeaderY);
			aHeaderBottom = Ftol(0.0);
		} else {
			g->PushState();
			g->SetColor(Color(255, 255, 255, Ftol(mBonusGlowCurve.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
			g->ClipRect(Ftol((double)gApp->mScale * 78.0), Ftol((double)gApp->mScale * 902.0),
				Ftol((double)gApp->mScale * 500.0), Ftol((double)gApp->mScale * 104.0));
			Transform aTransform;
			aTransform.RotateRad((float)((double)mUpdateCnt * -0.009999999776482582));
			if (mBoard->mBoardZoom == 1.0f)
				g->DrawImageTransform(IMAGE_MEGAFRUIT_RAINBOW, aTransform, (float)aCenterX, (float)aBaseY);
			g->PopState();
			aHeaderBottom = Ftol((float)((double)gApp->mScale + (double)gApp->mScale));
		}
		if (0.0f < mBonusGlowFade && aCount != 0) {
			Image* aBar = IMAGE_INGAMEUI_BONUS_HILIGHTBAR;
			int aBarY = Ftol((double)mBottomImage->mHeight * 0.38999998569488525 + (double)GetBottomTop()
				+ (double)aHeaderBottom);
			int aProduct = aBar->mWidth * (int)aCount;
			double aProductD = (double)aProduct;
			if (aProduct < 0)
				aProductD += 4294967296.0;
			int aTotalWidth = Ftol(aProductD * 0.8999999761581421);
			unsigned int aCellWidth = (unsigned int)aTotalWidth / aCount;
			int aStartX = aCenterX - aTotalWidth / 2;
			bool aAnimating = mBonusState == 1;
			if (aAnimating) {
				// (the exe works out a sine of the curve's output here and drops it)
				mBonusSwap.GetOutVal();
				mBonusSwap.GetOutVal();
			}
			unsigned int i = 0;
			if (aCount != 0) {
				int aRowY = aHeaderBottom + aBaseY;
				float aRowYF = (float)aRowY;
				int aX = (int)aCellWidth / 2 + aStartX;
				do {
					int aColor = gBoard->mBonusColors[i];
					int aLight = aColor * 2;
					if (i < (unsigned int)gBoard->mBonusMatched)
						aLight++;
					Image* aLightImage = GetImageById(sBonusLightIds[aLight]);
					if (aAnimating)
						(void)mBonusGlowTop[i];
					Color aLightColor(255, 255, 255, Ftol((double)mBonusGlowTop[i] * 255.0));
					if (aAnimating) {
						int aAlpha = 0x80;
						if (aAlpha <= aLightColor.mAlpha)
							aAlpha = aLightColor.mAlpha;
						aLightColor.mAlpha = Ftol((double)aAlpha * (double)mBonusGlowFade);
					}
					g->SetColor(aLightColor);
					DrawImageScaledCentered(g, aLightImage, (float)aX, aRowYF, 3, 1.0f, 1.0f);
					if (i == (unsigned int)gBoard->mBonusMatched) {
						// the light the chain has reached glows
						g->SetDrawMode(1);
						Color aSavedColor = g->GetColor();
						float aWave = (float)std::sin((double)mBonusSpin);
						double aGlow;
						if (0.0 <= (double)aWave) {
							aGlow = (double)(float)std::sin((double)mBonusSpin);
						} else {
							aGlow = 0.0;
						}
						float aGlowF = (float)aGlow;
						g->SetColor(Color(255, 255, 255, Ftol((double)mBonusGlowFade * 127.5 * (double)aGlowF)));
						DrawImageScaledCentered(g, aLightImage, (float)aX, (float)aBaseY, 3, 1.0f, 1.0f);
						g->SetColor(aSavedColor);
						g->SetDrawMode(0);
					}
					aX += aCellWidth;
					i++;
				} while (i < aCount);
			}
			// three rings of the lights, spreading out while the challenge is won (+0x3c4)
			int aRing = 0;
			do {
				i = 0;
				if (aCount != 0) {
					float aRingScale = (float)((double)(aRing + 1) / 3.0);
					int aX = (int)aCellWidth / 2 + aStartX;
					int anOffset = aStartX - aCenterX;
					float aRowYF = (float)aBaseY;
					do {
						int aColor = gBoard->mBonusColors[i];
						Image* aLightImage = GetImageById(sBonusLightIds[aColor * 2]);
						double aSpread = ((double)mBonusGlowBottom[i] * (1.0 - (double)mBonusFade)) * 4.0;
						if (!(aSpread < 1.0))
							aSpread = 1.0;
						g->SetColor(Color(255, 255, 255, Ftol(aSpread * 255.0)));
						float aGrow = (float)(((double)mBonusGlowBottom[i] * (double)aRingScale) * 0.800000011920929 + 1.0);
						double aDelta = (double)aGrow - 1.0;
						float anImageX = (float)(aDelta * (double)anOffset + (double)aX);
						float anImageY = (float)(aDelta * 20.0 + (double)aRowYF);
						DrawImageScaledCentered(g, aLightImage, anImageX, anImageY, aGrow, aGrow);
						anOffset += aCellWidth;
						aX += aCellWidth;
						i++;
					} while (i < aCount);
				}
				aRing++;
			} while (aRing < 3);
			if (mBonusState == 0 && mBonusTarget < 4) {
				bool aMoving = mBonusShownCount != mBonusTarget;
				int aMarkX = (int)aCellWidth / 2 + mBonusShownCount * (int)aCellWidth + aStartX;
				if (aMoving) {
					aMarkX = Ftol(mBonusSwap.GetOutVal() * (double)(mBonusTarget - mBonusShownCount) * (double)(int)aCellWidth
						+ (double)aMarkX);
				}
				g->SetColor(Color(255, 255, 255, Ftol((double)mBonusGlowFade * 255.0)));
				DrawImageScaledCentered(g, aBar, (float)aMarkX, (float)aBarY, 3, 1.0f, 1.0f);
			}
		}
		if (mBonusState != 2) {
			if (mBonusState == 3) {
				// the bonus won: "+N" in green rising out of the score font, then its glow
				double aAlpha = (double)mBonusTimer * 255.0;
				if (255.0 < aAlpha)
					aAlpha = 255.0;
				Color aTextColor(0x20, 0xff, 0x20, Ftol(aAlpha));
				g->SetColor(aTextColor);
				SexyString aText = StrFormat(GTC(0x7e3ce8), mBonusNumber);
				g->SetFont(FONT_SCORE);
				int aTextY = Ftol((double)gApp->mScale * 0.0) + aBaseY;
				double aHalf = (double)mWidth * 0.5;
				int aTextX = Ftol(aHalf - (double)(g->StringWidth(aText) / 2));
				g->DrawString(aText, aTextX, aTextY);
				float aFade = (float)((double)mBonusTimer - 1.0);
				if (0.0 < (double)aFade) {
					double aGlowAlpha = (double)aFade * 255.0;
					if (aGlowAlpha <= 255.0 && aGlowAlpha < 0.0)
						aGlowAlpha = 0.0;
					else if (255.0 < aGlowAlpha)
						aGlowAlpha = 255.0;
					aTextColor.mAlpha = Ftol(aGlowAlpha);
					g->SetColor(aTextColor);
					g->SetFont(FONT_SCOREGLOW);
					aTextY = Ftol((double)gApp->mScale * 0.0) + aBaseY;
					aHalf = (double)mWidth * 0.5;
					aTextX = Ftol(aHalf - (double)(g->StringWidth(aText) / 2));
					g->DrawString(aText, aTextX, aTextY);
				}
				return;
			}
			if (mBonusState != 4)
				return;
		}
		// the challenge done (2) or its time out (4)
		Image* aDone = IMAGE_INGAMEUI_CHALLENGECOMPLETE;
		g->SetColor(Color(255, 255, 255, Ftol((double)mBonusFade * 255.0)));
		int aDoneY = aBaseY - Ftol((double)gApp->mScale * 10.0);
		DrawImageScaledCentered(g, aDone, (float)aCenterX, (float)aDoneY, 3, 1.0f, 1.0f);
	}
}

// 0x7020cc: the u of a point (theU, theV) of the fill's waving mesh at this update (kept to 0 to 1 by 0x70209c)
static float FillWaveU(int theUpdateCnt, float theU, float theV)
{
	theU = (float)((double)theU - 0.5);
	float aTime = (float)((double)theUpdateCnt * 0.015);
	float aCos1 = cosf((float)((double)theU * 100.0 * ((double)theV - 0.5) * (double)aTime));
	float aCos2 = cosf(aTime);
	float aSin1 = sinf(aTime);
	double aSin3 = (double)sinf((float)((double)aTime * 3.0));
	double aValue = (((double)aSin1 * ((double)theV - 0.5) + (double)aCos1 * 0.125 + (double)aCos2 * (double)theU) * 0.25 *
		(aSin3 * 0.5 + 1.0)) + (double)theU * 0.800000011920929 + 0.5;
	float aResult = (float)aValue;
	if (aResult < 0.0f)
		return 0.0f;
	if (1.0f < aResult)
		return 1.0f;
	return aResult;
}

// 0x7021a8: the v of the same point
static float FillWaveV(int theUpdateCnt, float theU, float theV)
{
	theV = (float)((double)theV - 0.5);
	float aTime = (float)((double)theUpdateCnt * 0.01);
	float aCos1 = cosf((float)((double)theUpdateCnt * 0.003));
	float aSin1 = sinf((float)(((double)theU - 0.5) * 100.0 * (double)theV * (double)aTime));
	float aCos2 = cosf(aTime);
	float aSin2 = sinf(aTime);
	double aCos4 = (double)cosf((float)((double)aTime * 4.0));
	return (float)(0.800000011920929 * ((double)aCos1 * 0.20000000298023224 + 0.699999988079071 + (double)theV * 0.800000011920929 +
		(((double)aSin1 * 0.10000000149011612 + (double)aCos2 * (double)theV) - (double)aSin2 * ((double)theU - 0.5)) *
		0.019999999552965164 * (aCos4 * 0.5 + 1.0)));
}

void GoodTube::DrawFill(Graphics* g, bool theFront)
{
	// 0x7051a4: the fill, the glass over it and the specks (theFront false), then the bottom's ring and the level
	// over the tube (theFront true). (Direct3D: the fill is drawn again as a waving mesh of the two fill textures,
	// and the specks and the ring's number are placed with the device's matrices.)
	float aScale = gApp->mScale;
	int aTubeX = Ftol((double)GetImageOffsetX(GetIdByImage(mGlassImage)) * (double)aScale);
	aTubeX += Ftol((double)aScale);
	int aInnerGlowX = mGlassImage->mWidth / 2 + aTubeX;
	int aNegScale = Ftol((double)aScale * -1.0);
	float aRingX = (float)((double)GetImageOffsetX(IMAGE_INGAMEUI_RING_BACK_ID) * (double)aScale + (double)aNegScale);
	float aFillTop = mFillTopShown;
	if (gBoard->mGameMode == 3)
		aFillTop = (float)Ftol((double)aFillTop);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255));
	bool aLit;
	if (mBreakState < 1 || (double)mTopFall + (double)aScale * 370.0 < (double)aFillTop)
		aLit = false;
	else
		aLit = true;
	if (!theFront) {
		if (!aLit) {
			g->DrawImageF(IMAGE_INGAMEUI_RING_BACK, (float)((double)aScale * 0.0 + (double)aRingX),
				(float)((double)aFillTop - (double)aScale * 18.0));
			float anInnerGlowY = (float)((double)(GetImageOffsetY(IMAGE_INGAMEUI_RING_INNERGLOW_ID)
				- GetImageOffsetY(IMAGE_INGAMEUI_RING_FRONT_ID)) * (double)aScale + (double)aFillTop);
			float anInnerGlowF = (float)((double)(GetImageOffsetX(IMAGE_INGAMEUI_RING_INNERGLOW_ID)
				- GetImageOffsetX(IMAGE_INGAMEUI_RING_FRONT_ID)) * (double)aScale + (double)aRingX);
			g->DrawImageF(IMAGE_INGAMEUI_RING_INNERGLOW, anInnerGlowF, anInnerGlowY);
			int aFillX = aTubeX - Ftol((double)gApp->mScale * 45.0);
			int aFillBottom = Ftol((double)aScale * 3.0 + (double)GetBottomTop());
			int aFillY = Ftol((double)aScale * 25.0 + (double)mFillTopShown);
			// 0x85d6bc and 0x85d6cc: the fill's colour at the start and at the end of +0x294
			static const Color sFillStart(255, 255, 255, 255);
			static const Color sFillEnd(0x80, 0x10, 0x10, 255);
			Color aFill;
			if (gBoard->mGameMode == 3) {
				aFill = mColorCycle.mColor;
			} else {
				double aValue = (double)(sFillEnd.mRed - sFillStart.mRed) * (double)mEaseValue + (double)sFillStart.mRed;
				if (255.0 < aValue)
					aValue = 255.0;
				aFill.mRed = Ftol(aValue);
				aValue = (double)(sFillEnd.mGreen - sFillStart.mGreen) * (double)mEaseValue + (double)sFillStart.mGreen;
				if (255.0 < aValue)
					aValue = 255.0;
				aFill.mGreen = Ftol(aValue);
				aValue = (double)(sFillEnd.mBlue - sFillStart.mBlue) * (double)mEaseValue + (double)sFillStart.mBlue;
				if (255.0 < aValue)
					aValue = 255.0;
				aFill.mBlue = Ftol(aValue);
				aValue = (double)(sFillEnd.mAlpha - sFillStart.mAlpha) * (double)mEaseValue + (double)sFillStart.mAlpha;
				if (255.0 < aValue)
					aValue = 255.0;
				aFill.mAlpha = Ftol(aValue);
			}
			g->SetDrawMode(0);
			g->SetColor(aFill);
			int aMaxCut = (aFillBottom - Ftol((double)gApp->mScale * -30.0)) - aFillY;
			int aCut = mFillImage->mHeight;
			if (!(aCut < aMaxCut))
				aCut = aMaxCut;
			Rect aFillSrc(0, mFillImage->mHeight - aCut, mFillImage->mWidth, aCut);
			g->DrawImage(mFillImage, aFillX, aFillY, aFillSrc);
			g->SetDrawMode(1);
			g->SetColor(Color(255, 255, 255, 0x78));
			// With Direct3D the fill is drawn again, added, as two textures of the fill (TEXTURE1 as a 4 by 4 mesh that waves,
			// TEXTURE2 as a quad that scrolls up), over its picture
			int aMeshWidth = Ftol((double)IMAGE_INGAMEUI_TUBE_FILL_TEXTURE1->mWidth * 0.8999999761581421);
			int aMeshHeight = aCut - Ftol((double)aScale * 3.0);
			int aMeshX = (((mFillImage->mWidth / 2 - Ftol((double)aScale * -21.0)) + mX) - aMeshWidth / 2) + aFillX;
			int aMeshY = aFillY - Ftol((double)aScale * -0.0);
			if (gD3DInterface != nullptr) {
				gD3DInterface->SetTextureWrap(0, true);  // slot 0xcc
				float aStepX = (float)((double)aMeshWidth * 0.25);
				float aStepY = (float)((double)aMeshHeight * 0.25);
				float aStepV = (float)((double)((float)aCut / (float)mFillImage->mHeight) * 0.25);
				float anOriginX = (float)aMeshX;
				float anOriginY = (float)aMeshY;
				TriVertex aMesh[32][3];
				int aTriangle = 0;
				for (int aColumn = 0; aColumn < 4; aColumn++) {
					float aRight = (float)((double)(aColumn + 1) * (double)aStepX + (double)anOriginX);
					float aRightU = (float)((double)(aColumn + 1) * 0.25);
					float aLeft = (float)((double)aColumn * (double)aStepX + (double)anOriginX);
					float aLeftU = (float)((double)aColumn * 0.25);
					for (int aRow = 0; aRow < 4; aRow++) {
						float aTopY = (float)((double)aRow * (double)aStepY + (double)anOriginY);
						float aTopV = (float)((double)aRow * (double)aStepV);
						float aBottomY = (float)((double)(aRow + 1) * (double)aStepY + (double)anOriginY);
						float aBottomV = (float)((double)(aRow + 1) * (double)aStepV);
						TriVertex aLeftTop(aLeft, aTopY, FillWaveU(mUpdateCnt, aLeftU, aTopV), FillWaveV(mUpdateCnt, aLeftU, aTopV));
						TriVertex aRightTop(aRight, aTopY, FillWaveU(mUpdateCnt, aRightU, aTopV),
							FillWaveV(mUpdateCnt, aRightU, aTopV));
						TriVertex aLeftBottom(aLeft, aBottomY, FillWaveU(mUpdateCnt, aLeftU, aBottomV),
							FillWaveV(mUpdateCnt, aLeftU, aBottomV));
						TriVertex aRightBottom(aRight, aBottomY, FillWaveU(mUpdateCnt, aRightU, aBottomV),
							FillWaveV(mUpdateCnt, aRightU, aBottomV));
						aMesh[aTriangle][0] = aLeftTop;
						aMesh[aTriangle][1] = aRightTop;
						aMesh[aTriangle][2] = aLeftBottom;
						aTriangle++;
						aMesh[aTriangle][0] = aRightBottom;
						aMesh[aTriangle][1] = aRightTop;
						aMesh[aTriangle][2] = aLeftBottom;
						aTriangle++;
					}
				}
				gD3DInterface->DrawTrianglesTex(IMAGE_INGAMEUI_TUBE_FILL_TEXTURE1, aMesh, 0x20, Color::White, 1, 0.0f, 0.0f, true);
			}
			g->SetColor(Color(255, 255, 255, 0x80));
			float aScroll = (float)((double)(float)(mUpdateCnt % 500) / 500.0);
			float aScrollEnd = (float)((double)(float)aMeshHeight / (double)(float)mFillImage->mHeight + (double)aScroll);
			aMeshX += Ftol((double)aScale * 0.0);
			if (gD3DInterface != nullptr) {
				float aLeft = (float)aMeshX;
				float aRight = (float)(aMeshX + aMeshWidth);
				float aTopY = (float)aMeshY;
				float aBottomY = (float)(aMeshY + aMeshHeight);
				TriVertex aQuad[2][3];
				TriVertex aLeftTop(aLeft, aTopY, 0.0f, aScroll);
				TriVertex aRightTop(aRight, aTopY, 1.0f, aScroll);
				TriVertex aLeftBottom(aLeft, aBottomY, 0.0f, aScrollEnd);
				TriVertex aRightBottom(aRight, aBottomY, 1.0f, aScrollEnd);
				aQuad[0][0] = aLeftTop;
				aQuad[0][1] = aRightTop;
				aQuad[0][2] = aLeftBottom;
				aQuad[1][0] = aLeftBottom;
				aQuad[1][1] = aRightTop;
				aQuad[1][2] = aRightBottom;
				gD3DInterface->SetSamplerState(0, GPUSAMP_ADDRESSV, 1);  // slot 0xc4: the v wraps
				gD3DInterface->DrawTrianglesTex(IMAGE_INGAMEUI_TUBE_FILL_TEXTURE2, aQuad, 2, Color::White, 1, 0.0f, 0.0f, true);
				gD3DInterface->SetSamplerState(0, GPUSAMP_ADDRESSV, 3);  // clamped again
			}
			g->SetDrawMode(0);
			g->SetDrawMode(1);
			g->SetColor(Color(255, 255, 255));
			g->SetColorizeImages(true);
			g->SetDrawMode(0);
			int aGlassTop = GetGlassTop();
			int aGlassHeight = mGlassHeight;
			g->SetColor(Color(255, 255, 255, Ftol((1.0 - (double)mGlassGone) * 255.0)));
			Rect aGlassSrc(0, 0, mGlassImage->mWidth, aGlassHeight);
			Rect aGlassDest(aTubeX, aGlassTop, mGlassImage->mWidth, aGlassHeight);
			g->DrawImage(mGlassImage, aGlassDest, aGlassSrc);
			g->SetDrawMode(1);
			g->SetColor(aFill);
			unsigned int i = 0;
			if (mSpecks.size() != 0) {
				do {
					Speck* aSpeck = mSpecks[i];
					aFill.mAlpha = Ftol((double)aSpeck->mAlpha * 255.0 * (1.0 - (double)mEaseValue));
					g->SetColor(aFill);
					// port: the original's Graphics::Is3D (a Direct3D destination); the port's 2D screen also answers true, so
					// 2D mode is told by gD3DInterface
					if (gD3DInterface == nullptr || !g->Is3D()) {
						g->DrawImage(aSpeck->mImage, Ftol(aSpeck->mX), Ftol(aSpeck->mY));
					} else {
						// with Direct3D stretched by 0.75 to 1.25 of the particle picture's size (0.75 of it again), more
						// the further it is from the tube's middle (x 87) and by its speed
						float aFromMiddle = (float)std::fabs((double)(float)((double)aSpeck->mX - 87.0));
						double aNear = (double)(float)((double)aFromMiddle / 23.0);
						float aStretch = (float)(aNear + (1.0 - aNear) * 0.333 * ((double)aSpeck->mDX * 2.5 + 3.0));
						if (aStretch < 0.75f)
							aStretch = 0.75f;
						else if (1.25f < aStretch)
							aStretch = 1.25f;
						float aSize = (float)(0.75 * (double)aStretch);
						float aSpeckScale = (float)((double)aSpeck->mScale * (1.0 - (double)mEaseValue) * (double)aSize);
						int aSpeckHeight = Ftol((double)IMAGE_INGAMEUI_TUBE_FILL_PARTICLE->mHeight * (double)aSpeckScale);
						int aSpeckWidth = Ftol((double)IMAGE_INGAMEUI_TUBE_FILL_PARTICLE->mWidth * (double)aSpeckScale);
						g->DrawImage(aSpeck->mImage, Ftol(aSpeck->mX), Ftol(aSpeck->mY), aSpeckWidth, aSpeckHeight);
					}
					i++;
				} while (i < mSpecks.size());
			}
		}
	} else if (mBreakState < 1 || (double)mTopFall + (double)aScale * 370.0 < (double)aFillTop) {
		g->SetDrawMode(0);
		g->SetColor(Color::White);
		Image* aBottomGlow = IMAGE_INGAMEUI_TUBE_BOTTOM_INNERGLOW;
		int aBottomGlowY = Ftol((double)gApp->mScale * 24.0) + GetBottomTop();
		DrawImageScaledCentered(g, aBottomGlow, (float)aInnerGlowX, (float)aBottomGlowY, 3, 1.0f, 1.0f);
		Image* aRing = IMAGE_INGAMEUI_RING_FRONT;
		float aRingStep = (float)(-(double)aScale * 12.0);
		double aRingY = (double)aRingStep + (double)aFillTop;
		// with Direct3D the ring, the level and the timer are drawn at float positions
		float aRingYF = (float)aRingY;
		if (gD3DInterface == nullptr)
			g->DrawImage(aRing, Ftol((double)aRingX), Ftol(aRingY));
		else
			g->DrawImageF(aRing, aRingX, aRingYF);
		Image* aLevel = IMAGE_INGAMEUI_LEVEL;
		if ((mTimeKind != 0 || 0 < mLevel) && (gBoard->mGameMode != 2 || mTimeKind != 0)) {
			float aRingMidX = (float)((double)aRing->mWidth * 0.5 + (double)aRingX);
			double aRingDrop = (double)aRing->mHeight * 0.2800000011920929;
			float aLevelY = (float)(aRingDrop + (double)Ftol((double)aFillTop));
			if (gD3DInterface == nullptr) {
				g->DrawImage(aLevel, Ftol((double)aRingMidX) - aLevel->mWidth / 2, Ftol((double)aLevelY) - aLevel->mHeight / 2);
			} else {
				float aLevelYF = (float)((double)aFillTop + aRingDrop);
				g->DrawImageF(aLevel, aRingMidX - (float)(aLevel->mWidth / 2), aLevelYF - (float)(aLevel->mHeight / 2));
			}
			g->SetFont(FONT_TUBE_TEXT);
			float aTimerY = (float)((double)aRing->mHeight * 0.800000011920929 + (double)Ftol(aRingY));
			if (gD3DInterface != nullptr)
				aTimerY = (float)((double)aRing->mHeight * 0.800000011920929 + (double)aRingYF);
			Image* aTimer = IMAGE_INGAMEUI_RING_FRONT_TIMER;
			if (mTimeKind == 0) {
				// with Direct3D the number's fraction of a pixel in y goes through a matrix
				SexyTransform2D aFractionMatrix;  // 0x44f860
				aFractionMatrix.Translate(0.0f, (float)((double)aTimerY - (double)Ftol((double)aTimerY)));  // 0x44f8a0
				if (gD3DInterface != nullptr)
					gD3DInterface->PushTransform(aFractionMatrix, true);  // slot 0x14
				if (gApp->mArtRes == 600)
					aTimerY = (float)((double)aTimerY + 1.0);
				SexyString aLevelText = StrFormat(GTC(0x80da2c), mLevel);
				int aTextWidth = g->StringWidth(aLevelText);
				if (mLevel < 10)
					aTextWidth -= 2;
				g->DrawString(aLevelText, Ftol((double)aRingMidX - (double)(aTextWidth / 2)), Ftol((double)aTimerY));
				if (gD3DInterface != nullptr)
					gD3DInterface->PopTransform();  // slot 0x18
			} else if (gD3DInterface == nullptr) {
				int aTimerStep = Ftol((double)gApp->mScale * -25.0);
				int aTimerTop = (-(aTimer->mHeight / 2) - aTimerStep) + Ftol(aRingY);
				g->DrawImage(aTimer, Ftol((double)aRingMidX) - aTimer->mWidth / 2, aTimerTop);
			} else {
				int aTimerStep = Ftol((double)gApp->mScale * 25.0);
				g->DrawImageF(aTimer, aRingMidX - (float)(aTimer->mWidth / 2),
					(float)(((double)aRingYF + (double)aTimerStep) - (double)(aTimer->mHeight / 2)));
			}
		}
	}
	g->SetDrawMode(1);
	if (mBoard->mGameMode != 3 && 0 < mLevel) {
		g->SetColor(Color(255, 255, 255, mGlowAlpha));
		g->DrawImageF(IMAGE_INGAMEUI_RING_OUTERGLOW, aRingX, (float)((double)aFillTop - (double)aScale * 6.0));
	}
	// (the text popups: 0x707103 and 0x707215, which make them, are never called, so these stay null)
	if (mLevelPopup != nullptr)
		NotPorted("GoodTube::DrawFill: the text popup at +0x1cc (0x707103)");
	if (mScorePopup != nullptr)
		NotPorted("GoodTube::DrawFill: the text popup at +0x1d0 (0x707215)");
}

void GoodTube::DrawLight(Graphics* g, int theLight, float theAlpha)
{
	// 0x70675e
	int aLastLight = Board::GetLevelSteps(mShownLevel) - 1;
	if (aLastLight < theLight)
		theLight = aLastLight;
	if (gD3DInterface != nullptr) {
		// With Direct3D the light is a strip of triangles along its arc (the curves of the arc's two edges, from the
		// fractions of the arc this light covers: its start, and the end of what shows of it by theAlpha), the light's
		// picture laid over it
		gD3DInterface->SetTextureWrap(0, false);  // slot 0xcc
		CurvedVal anEdgeA;
		anEdgeA.SetCurve(GTS(0x7e3fc4));
		CurvedVal anEdgeB;
		anEdgeB.SetCurve(GTS(0x7e3fa8));
		int aSteps = Board::GetLevelSteps(mShownLevel);
		int anImageId = sFirstLight[mShownLevel] + theLight;
		Image* aLightImage = GetImageById(anImageId);
		float aStartA = 0.0f;  // where on the arc this light's edge A, and edge B, start
		float aStartB = 0.0f;
		if (aSteps == 4) {
			static const float sA[4] = {0.01f, 0.19f, 0.51f, 0.85f};
			static const float sB[4] = {0.01f, 0.24f, 0.51f, 0.74f};
			aStartA = sA[theLight];
			aStartB = sB[theLight];
		} else if (aSteps == 8) {
			static const float sA[8] = {0.01f, 0.05f, 0.18f, 0.33f, 0.51f, 0.7f, 0.84f, 0.96f};
			static const float sB[8] = {0.01f, 0.13f, 0.235f, 0.37f, 0.51f, 0.63f, 0.78f, 0.89f};
			aStartA = sA[theLight];
			aStartB = sB[theLight];
		} else if (aSteps == 12) {
			static const float sA[12] = {0.02f, 0.03f, 0.1f, 0.185f, 0.28f, 0.4f, 0.51f, 0.64f, 0.75f, 0.84f, 0.93f, 0.98f};
			static const float sB[12] = {0.02f, 0.1f, 0.17f, 0.25f, 0.33f, 0.42f, 0.51f, 0.6f, 0.68f, 0.77f, 0.84f, 0.91f};
			aStartA = sA[theLight];
			aStartB = sB[theLight];
		} else if (aSteps == 16) {
			static const float sA[16] = {0.015f, 0.025f, 0.07f, 0.1f, 0.16f, 0.23f, 0.32f, 0.41f, 0.515f, 0.626f, 0.72f, 0.81f,
				0.87f, 0.92f, 0.96f, 0.98f};
			static const float sB[16] = {0.02f, 0.075f, 0.12f, 0.17f, 0.225f, 0.31f, 0.38f, 0.44f, 0.515f, 0.612f, 0.67f, 0.73f,
				0.8f, 0.85f, 0.9f, 0.91f};
			aStartA = sA[theLight];
			aStartB = sB[theLight];
		}
		float aSpread = (float)(1.0 / ((double)theAlpha + 0.0001));
		g->SetDrawMode(0);
		g->SetColor(Color(255, 255, 255));
		float aScale = gApp->mScale;
		TriVertex aPreviousA;
		TriVertex aPreviousB;
		aPreviousA.x = aPreviousA.y = aPreviousA.u = aPreviousA.v = 0.0f;
		aPreviousB = aPreviousA;
		float aLength = 0.0f;
		do {
			float aTimeA = (float)(((double)aLength - (double)aStartA) * (double)aSpread + (double)aStartA);
			float aTimeB = (float)(((double)aLength - (double)aStartB) * (double)aSpread + (double)aStartB);
			// the arc's point for edge A at aLength: across by 278 and down the curve's 155 from the 286, for the edge's
			// vertex, and the same for the point of the picture it shows (aTimeA)
			anEdgeA.SetInVal((double)aLength);
			float aXA = (float)((double)Ftol((double)aScale * 278.0) * (double)aLength + (double)Ftol((double)aScale * 70.0));
			int aBase = Ftol((double)aScale * 286.0);
			float aYA = (float)((double)aBase - anEdgeA.GetOutVal() * (double)Ftol((double)aScale * 155.0));
			anEdgeA.SetInVal((double)aTimeA);
			float aTexXA = (float)((double)Ftol((double)aScale * 278.0) * (double)aTimeA + (double)Ftol((double)aScale * 70.0));
			aBase = Ftol((double)aScale * 286.0);
			float aTexYA = (float)((double)aBase - anEdgeA.GetOutVal() * (double)Ftol((double)aScale * 155.0));
			float anImageX = (float)(((double)aTexXA - (double)Ftol((double)aScale * (double)GetImageOffsetX(anImageId))) /
				(double)aLightImage->mWidth);
			float anImageY = (float)(((double)aTexYA - (double)Ftol((double)aScale * (double)GetImageOffsetY(anImageId))) /
				(double)aLightImage->mHeight);
			TriVertex aCurrentA((float)((double)aXA - 1.0), (float)((double)aYA - 1.0), anImageX, anImageY);
			// and edge B
			anEdgeB.SetInVal((double)aLength);
			float aXB = (float)((double)Ftol((double)aScale * 278.0) * (double)aLength + (double)Ftol((double)aScale * 70.0));
			aBase = Ftol((double)aScale * 286.0);
			float aYB = (float)((double)aBase - anEdgeB.GetOutVal() * (double)Ftol((double)aScale * 155.0));
			anEdgeB.SetInVal((double)aTimeB);
			float aTexXB = (float)((double)Ftol((double)aScale * 278.0) * (double)aTimeB + (double)Ftol((double)aScale * 70.0));
			aBase = Ftol((double)aScale * 286.0);
			float aTexYB = (float)((double)aBase - anEdgeB.GetOutVal() * (double)Ftol((double)aScale * 155.0));
			float anImageXB = (float)(((double)aTexXB - (double)Ftol((double)aScale * (double)GetImageOffsetX(anImageId))) /
				(double)aLightImage->mWidth);
			float anImageYB = (float)(((double)aTexYB - (double)Ftol((double)aScale * (double)GetImageOffsetY(anImageId))) /
				(double)aLightImage->mHeight);
			TriVertex aCurrentB((float)((double)aXB - 1.0), (float)((double)aYB - 1.0), anImageXB, anImageYB);
			if (aLength != 0.0f) {
				g->DrawTriangleTex(aLightImage, aPreviousA, aCurrentA, aPreviousB);  // 0x422130
				g->DrawTriangleTex(aLightImage, aCurrentA, aCurrentB, aPreviousB);
			}
			aLength = (float)((double)aLength + 0.019999999552965164);
			aPreviousA = aCurrentA;
			aPreviousB = aCurrentB;
		} while (aLength <= 1.0f);
		return;
	}
	int anId = sFirstLight[mShownLevel] + theLight;
	Image* aLightImage = GetImageById(anId);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol((double)theAlpha * 255.0)));
	int aLightY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(anId));
	int aLightX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(anId));
	g->DrawImage(aLightImage, aLightX, aLightY);
}

void GoodTube::DrawBody(Graphics* g)
{
	// 0x7081c9: the top, the lit lights, the score under it (or the challenge's), the tip and the multiplier's bulbs
	bool aIsChallengeTube = mTubeKind == 4;
	float anArtScale = (float)((double)gApp->mArtRes / 1200.0);
	int aTopX = GetTopX();
	int aTopY = GetTopY();
	g->SetDrawMode(0);
	g->SetColor(Color::White);
	g->DrawImageRotated(mTopImage, aTopX, aTopY, (double)mTopTurn, nullptr);
	g->SetColorizeImages(true);
	bool aIsChallengeMode = gBoard->mGameMode == 2;
	if (0 < mBreakState)
		return;
	int aLevelIndex = mShownLevel - 1;
	if (mLevelSoundOn && !aIsChallengeMode) {
		// the slots of the lights, then the lights lit so far
		int aLitLights = Ftol((double)(float)std::floor((double)mShownSteps));
		Image* aSlots[11] = {IMAGE_INGAMEUI_ARC_4_SLOT, IMAGE_INGAMEUI_ARC_8_SLOT, IMAGE_INGAMEUI_ARC_12_SLOT,
			IMAGE_INGAMEUI_ARC_16_SLOT, IMAGE_INGAMEUI_ARC_16_SLOT, IMAGE_INGAMEUI_ARC_16_SLOT,
			IMAGE_INGAMEUI_ARC_16_SLOT, IMAGE_INGAMEUI_ARC_16_SLOT, IMAGE_INGAMEUI_ARC_16_SLOT,
			IMAGE_INGAMEUI_ARC_16_SLOT, IMAGE_INGAMEUI_ARC_16_SLOT};
		Image* aSlot = aSlots[aLevelIndex];
		int aSlotId = GetIdByImage(aSlot);
		int aSlotDrop = GetImageOffsetY(aSlotId) - GetImageOffsetY(GetIdByImage(mTopImage));
		double aSlotDropScaled = (double)aSlotDrop * (double)anArtScale;
		int aSlotY = Ftol((double)GetTopY() + aSlotDropScaled);
		int aSlotX = Ftol((double)GetImageOffsetX(aSlotId) * (double)anArtScale);
		g->DrawImage(aSlot, aSlotX, aSlotY);
		int aFirstLight = sFirstLight[mShownLevel];
		int aLightCount = Board::GetLevelSteps(mShownLevel);
		if (aLightCount < aLitLights)
			aLitLights = aLightCount;
		int i = 0;
		if (0 < aLitLights) {
			float aCountF = (float)aLightCount;
			int aHueBase = aLightCount * 15;
			float* aFlash = mLightFlash;
			do {
				int aPointA[2];
				int aPointB[2];
				GetStepPoint(aPointA, i, false);
				int aLightX = aPointA[0];
				GetStepPoint(aPointB, i, false);
				int aLightY = aPointB[1];
				Image* aLightImage = GetImageById(aFirstLight + i);
				g->SetDrawMode(0);
				g->SetColor(Color::White);
				// the brightness the light has reached, from 0.78 of white
				double aBright = 1.0 - ((double)mShownSteps - (double)aCountF) * 0.25;
				if ((0.7799999713897705 <= aBright && 1.0 < aBright) || aBright < 0.7799999713897705) {
					if (aBright < 0.7799999713897705)
						aBright = 0.7799999713897705;
					else
						aBright = 1.0;
				}
				float aBrightF = (float)aBright;
				int aGrey = Ftol((double)aBrightF * 255.0);
				g->SetColor(Color(aGrey, aGrey, aGrey, 255));
				float aReached = (float)((double)mShownSteps - (double)(float)i);
				if (aReached < 0.99) {
					DrawLight(g, i, aReached);
				} else {
					float aBeyond = (float)(((double)mShownSteps - (double)aCountF) - (double)(float)i);
					if (0.0 < aBeyond) {
						// a light beyond the level's last: the hue goes round with the time
						aLightImage = GetImageById(i + 0xdb);
						float aSpeed = gBoard->mFruitDue ? 10.0f : 1.5f;
						int aHueTime = Ftol((double)mUpdateCnt * (double)aSpeed);
						unsigned int aHue = (unsigned int)(aHueTime + aHueBase) & 0x800000ffu;
						if ((int)aHue < 0)
							aHue = (aHue - 1 | 0xffffff00u) + 1;
						g->SetColor(Color((int)HSLToColor((int)aHue, 255, 128)));
					}
					g->DrawImage(aLightImage, aLightX, aLightY);
				}
				if (0.0f < *aFlash) {
					g->SetDrawMode(1);
					g->SetColor(Color(255, 255, 255, Ftol((double)*aFlash * 255.0)));
					g->DrawImage(aLightImage, aLightX, aLightY);
				}
				i++;
				aFlash++;
				aHueBase -= 15;
			} while (i < aLitLights);
		}
		g->SetColorizeImages(true);
		if (0.0f < mLevelFlash) {
			GetTopY();
			Image* anArcGlow = IMAGE_INGAMEUI_ARC_GLOW;
			int anArcGlowId = GetIdByImage(anArcGlow);
			g->SetDrawMode(1);
			g->SetColor(Color(255, 255, 255, Ftol((double)mLevelFlash * 255.0)));
			int anArcGlowY = Ftol((double)GetImageOffsetY(anArcGlowId) * (double)anArtScale);
			int anArcGlowX = Ftol((double)GetImageOffsetX(anArcGlowId) * (double)anArtScale);
			g->DrawImage(anArcGlow, anArcGlowX, anArcGlowY);
			g->SetDrawMode(0);
		}
	}
	float aTipFade = (float)(1.0 - (double)mTipFade);
	if (gBoard->mChallenge != 0) {
		DrawChallenge(g, aTipFade);
	} else if (mTubeKind == 0) {
		// the score under the top
		SexyString aScoreText = CommaSeperate((int)mBoard->mShownScore);
		Font* aGlowFont = FONT_SCOREGLOW;
		Font* aScoreFont = FONT_SCORE;
		int aScoreWidth = aScoreFont->StringWidth(aScoreText);
		if (1999999 < (unsigned int)mBoard->mShownScore) {
			aGlowFont = FONT_SCOREGLOW_SMALL;
			aScoreFont = FONT_SCORE_SMALL;
			aScoreWidth = aScoreFont->StringWidth(aScoreText);
		}
		g->SetFont(aScoreFont);
		int aScoreX = (mTopImage->mWidth / 2 - aScoreWidth / 2) + 2 + aTopX;
		Font* aCurFont = g->GetFont();
		int aScoreDrop = Ftol((double)gApp->mScale * 5.0);
		Font* aCurFont2 = g->GetFont();
		int aScoreY = GetScoreY() + (aCurFont->GetHeight() - aCurFont2->mAscentPadding) / 2 + aScoreDrop;
		if (!mLevelSoundOn)
			aScoreY = Ftol((double)aScoreY - (double)anArtScale * 0.0);
		g->SetColor(Color(255, 255, 255, Ftol((double)aTipFade * 255.0)));
		g->DrawString(aScoreText, aScoreX, aScoreY);
		if (0.0f < mBoard->mScoreRollGlow) {
			g->SetColorizeImages(true);
			double aGlowAlpha = (double)aTipFade * (double)mBoard->mScoreRollGlow * 255.0;
			if (255.0 < aGlowAlpha)
				aGlowAlpha = 255.0;
			g->SetColor(Color(255, 255, 255, Ftol(aGlowAlpha)));
			g->SetFont(aGlowFont);
			SetFontLayerColor(g->GetFont(), 0, Color::White);
			g->DrawString(aScoreText, aScoreX, aScoreY);
		}
	}
	if (mTubeKind != 4 && !mTip.empty() && 0.0f < mTipFade) {
		Color aTipColor(mTipColor[0], mTipColor[1], mTipColor[2], mTipColor[3]);
		aTipColor.mAlpha = Ftol((double)mTipFade * 255.0);
		g->SetFont(FONT_TOOLTIP);
		int aTipHeight = g->GetWordWrappedHeight(mTipWidth, mTip, -1, nullptr);
		int aTipMid = mTopImage->mHeight / 2 + aTopY;
		int aTipY = Ftol(((double)anArtScale * 5.0 + (double)aTipMid) - (double)aTipHeight * 0.375);
		if (gBoard->mGameMode == 2)
			aTipY += Ftol((double)gApp->mScale * -40.0);
		g->SetColor(aTipColor);
		int aTipX = mWidth / 2 - mTipWidth / 2;
		Rect aTipRect(Ftol((double)aTipX + (double)anArtScale * 8.0), aTipY, mTipWidth, aTipHeight);
		g->WriteWordWrapped(aTipRect, mTip, -1, 0, nullptr, -1, nullptr);
		g->SetColor(Color::White);
	}
	if (!aIsChallengeMode && mLevelSoundOn) {
		// the multiplier's bulbs, and the glow of the one that has just lit
		g->SetDrawMode(1);
		Image* aBulbs = GetImageById(IMAGE_INGAMEUI_MULTIBULBS_ID);
		int aBulbX = Ftol((double)GetImageOffsetX(GetIdByImage(aBulbs)) * (double)anArtScale);
		int aBulbOffset = -1 - GetImageOffsetY(GetIdByImage(mTopImage));
		aBulbOffset += GetImageOffsetY(GetIdByImage(aBulbs));
		double aBulbOffsetScaled = (double)aBulbOffset * (double)anArtScale;
		int aBulbY = Ftol((double)GetTopY() + aBulbOffsetScaled);
		g->SetDrawMode(0);
		g->SetColor(Color(255, 255, 255));
		int aCel = mLevelCel - 1;
		if (9 < aCel)
			aCel = 9;
		g->DrawImageCel(aBulbs, aBulbX, aBulbY, aCel);
		g->SetDrawMode(1);
		if (0.0f < gBoard->mRingBurst) {
			float aGrow = (float)((1.0 - (double)gBoard->mRingBurst) * 3.0 + 1.0);
			g->SetColor(Color(255, 255, 255, Ftol((double)gBoard->mRingBurst * 255.0)));
			Image* aBulbGlow = IMAGE_INGAMEUI_MULTIBULB_GLOW;
			for (; 1.0f <= aGrow; aGrow = (float)((double)aGrow - 1.0)) {
				int aGlowWidth = Ftol((double)aBulbGlow->mWidth * (double)aGrow);
				int aGlowHeight = Ftol((double)aBulbGlow->mHeight * (double)aGrow);
				int aGlowMidX = aBulbs->GetCelWidth() / 2 + aBulbX;
				int aGlowMidY = aBulbs->GetCelHeight() / 2 + aBulbY;
				g->DrawImage(aBulbGlow, aGlowMidX - aGlowWidth / 2, aGlowMidY - aGlowHeight / 2, aGlowWidth,
					aGlowHeight);
			}
		}
	}
	if (aIsChallengeTube && mTopTurn == 0.0f) {
		// the doors of the challenge's tube
		g->DrawImage(IMAGE_DOORS_CHALLENGE_TOP, aTopX - Ftol((double)gApp->mScale * -8.0),
			aTopY - Ftol((double)gApp->mScale * -45.0));
		g->DrawImage(IMAGE_DOORS_CHALLENGE_BOTTOM, aTopX - Ftol((double)gApp->mScale * -8.0),
			aTopY - Ftol((double)gApp->mScale * -45.0));
	}
	g->SetDrawMode(1);
	Image* aTopGlow = IMAGE_INGAMEUI_TOP_LIGHTGLOW;
	g->SetColor(Color(255, 255, 255, mGlowAlpha));
	int aTopGlowY = Ftol((double)GetImageOffsetY(GetIdByImage(aTopGlow)) * (double)anArtScale);
	int aTopGlowX = Ftol((double)GetImageOffsetX(GetIdByImage(aTopGlow)) * (double)anArtScale);
	g->DrawImage(aTopGlow, aTopGlowX, aTopGlowY);
	g->SetColor(Color(255, 255, 255));
	g->SetDrawMode(0);
}

namespace {

// 0x84eff4: the pattern's gem pictures, two for each colour (RWGYVOB): waiting, then matched
int GetPatternGemId(int theIndex)
{
	return GameInt(GTB(0x84eff4, 56) + theIndex * 4);
}

// The four lists 0x6db0ec fills (each emptied first): each rule's stat, its value now, its comparison and its goal
void GetGoalLists(const Challenge* theChallenge, std::vector<int>& theStats, std::vector<int>& theCurrents,
	std::vector<int>& theOps, std::vector<int>& theTargets)
{
	theStats.clear();
	theCurrents.clear();
	theOps.clear();
	theTargets.clear();
	std::vector<ChallengeGoal> aGoals = gApp->mChallengeInfo->GetGoals(theChallenge);
	for (const ChallengeGoal& aGoal : aGoals) {
		theStats.push_back(aGoal.mStat);
		theOps.push_back(aGoal.mOp);
		theTargets.push_back(aGoal.mTarget);
		theCurrents.push_back(aGoal.mCurrent);
	}
}

}  // namespace

void GoodTube::DrawChallenge(Graphics* g, float theTipFade)
{
	// 0x7087b2-0x70a209 (part of DrawBody): the challenge's display in the glass under the top, by the tube's kind
	// and the challenge's flags
	Challenge* aChallenge = gBoard->mChallenge;
	unsigned int aFlags = aChallenge->mFlags;
	bool aPoints = (aFlags & Challenge::FLAG_POINTS) != 0;
	bool aNightMode = (aFlags & Challenge::FLAG_NIGHTMODE) != 0;
	// the night mode's best ever shows under it once the player has one (Board::SetChallenge sets +0x768)
	bool aShowBestEver = aNightMode && mShowBestEver;
	int aScoreY = GetScoreY();
	if (aShowBestEver)
		aScoreY += Ftol((double)gApp->mScale * -55.0);
	int anAlpha = Ftol((double)theTipFade * 255.0);
	g->SetColor(Color(255, 255, 255, anAlpha));
	if (mTubeKind == 3) {
		// the moves left, word-wrapped in FONT_SCORE
		int aTop = Ftol((double)gApp->mScale * (aPoints ? 50.0 : -80.0)) + aScoreY;
		SexyString aText;
		int aMovesLeft = aChallenge->mMoveGoal - gApp->mStats[0x21];
		if (aMovesLeft == 1)
			aText = GT(0x7e40ec);
		else
			aText = StrFormat(gApp->GetString(GTS(0x7e40dc), GT(0x7e40c0)).c_str(), aMovesLeft);
		g->SetFont(FONT_SCORE);
		float aWidthF = (float)mWidth;
		int aRectWidth = Ftol((double)aWidthF * 0.75);
		int aRectHeight = Ftol((double)mHeight * 0.20000000298023224);
		Rect aRect(Ftol((double)aWidthF * 0.5249999761581421 - (double)(aRectWidth / 2)), aTop, aRectWidth,
			aRectHeight);
		g->WriteWordWrapped(aRect, aText, -1, 0, nullptr, -1, nullptr);
	} else if ((aFlags & Challenge::FLAG_PATTERN) != 0) {
		GetGoalLists(aChallenge, mRuleGoals, mRuleValues, mRuleComparisons, mRuleStats);
		if (aNightMode) {
			// the count's title and the count, and the best under them while +0x580 shows it
			int aCount = gBoard->mChallengeCount;
			SexyString aTitle = aChallenge->mCountTitle;
			SexyString aCountText = StrFormat(GTC(0x80da2c), aCount);  // (made and not drawn)
			int aMidX = Ftol((double)mWidth * 0.5249999761581421);
			int aY = aScoreY - Ftol((double)gApp->mScale * -30.0);
			double aBestShown = mBestFade.GetOutVal();
			int aRise = Ftol((double)gApp->mScale * 0.0);
			aY = Ftol(aBestShown * (double)aRise + (double)aY);
			g->SetFont(FONT_TOOLTIP);
			int aTitleY = aY - Ftol((double)gApp->mScale * 40.0);
			g->DrawString(aTitle, aMidX - g->StringWidth(aTitle) / 2, aTitleY);
			mCountTicker.Draw(g, aMidX, aY, true, theTipFade);
			if (mBestFade.GetOutVal() != 0.0) {
				g->SetColor(Color(255, 255, 255, Ftol(mBestFade.GetOutVal() * 255.0 * (double)theTipFade)));
				SexyString aBest = GT(0x7e40a8);
				g->SetFont(FONT_TOOLTIP);
				int aBestY = Ftol((double)gApp->mScale * 65.0) + aScoreY;
				g->DrawString(aBest, aMidX - g->StringWidth(aBest) / 2, aBestY);
				float aBestAlpha = (float)(mBestFade.GetOutVal() * (double)theTipFade);
				mBestTicker.Draw(g, aMidX, aScoreY - Ftol((double)gApp->mScale * -95.0), true, aBestAlpha);
				g->SetColor(Color(255, 255, 255, anAlpha));
			}
		} else {
			// the pattern's gems in a row, those matched so far (+0x538) lit; the next one grows with +0x4c8
			int aMaxWidth = Ftol((double)mWidth * 0.550000011920929);
			int aCount = (int)aChallenge->mPattern.size();
			float aGap = (float)((double)gApp->mScale * 5.0);
			float aGemWidth = (float)GetImageById(GetPatternGemId(0))->mWidth;
			float aCountLess = (float)(aCount - 1);
			float aCountF = (float)aCount;
			float aRowWidth = (float)((double)aCountF * (double)aGemWidth + (double)aCountLess * (double)aGap);
			float aScale = 1.0f;
			float aMaxWidthF = (float)aMaxWidth;
			if (aMaxWidthF < aRowWidth)
				aScale = (float)((double)aMaxWidthF / (double)aRowWidth);
			aGemWidth = (float)((double)aScale * (double)aGemWidth);
			aGap = (float)((double)aScale * (double)aGap);
			float aMidX = (float)((double)mWidth * 0.5249999761581421);
			aRowWidth = (float)((double)aCountF * (double)aGemWidth + (double)aCountLess * (double)aGap);
			float aX = (float)((double)aMidX - (double)aRowWidth * 0.5);
			int aY = aScoreY - Ftol((double)gApp->mScale * -0.0);
			int i = 0;
			if (0 < aCount) {
				double aStep = (double)aGemWidth + (double)aGap;
				do {
					int anIndex = aChallenge->mPattern[i] * 2;
					if (i < mGoalShown)
						anIndex++;
					Image* aGem = GetImageById(GetPatternGemId(anIndex));
					if (aGem != nullptr) {
						float aSize = aScale;
						if (i == mGoalShown && mGoalMove.mIncRate != 0.0 && !mGoalMove.HasBeenTriggered())
							aSize = (float)(mGoalMove.GetOutVal() * (double)aScale);
						DrawImageScaledCentered(g, aGem, (float)((double)aGemWidth * 0.5 + (double)aX), (float)aY,
							aSize, aSize);
					}
					i++;
					aX = (float)((double)aX + aStep);
				} while (i < aCount);
			}
		}
	} else if ((aFlags & Challenge::FLAG_ONBOARD) != 0) {
		// a column of the colours' (and specials') icons, each with "x" and its count on the board, shrunk by
		// hundredths until it fits 150
		int anIconSize = Ftol((double)gApp->mScale * 54.0);
		float anIconSizeF = (float)anIconSize;
		int aGap = Ftol((double)anIconSizeF * 0.4000000059604645);
		GetGoalLists(aChallenge, mRuleGoals, mRuleValues, mRuleComparisons, mRuleStats);
		int aMaxHeight = Ftol((double)gApp->mScale * 150.0);
		std::vector<int> aCels;
		std::vector<int> aValues;
		unsigned int i = 0;
		if (mRuleGoals.size() != 0) {
			do {
				int aStat = mRuleGoals[i];
				int aCel = -1;
				if (aStat < 0x4b || 0x51 < aStat) {
					if (aStat == 0x14)
						aCel = 8;
					else if (aStat == 0x15)
						aCel = 7;
					else if (aStat == 0x18)
						aCel = 9;
					else if (aStat == 0x4a)
						aCel = 10;
				} else {
					aCel = mRuleGoals[i] - 0x4b;
				}
				if (aCel != -1) {
					aValues.push_back(mRuleValues[i]);
					aCels.push_back(aCel);
				}
				i++;
			} while (i < mRuleGoals.size());
		}
		int aCount = (int)aCels.size();
		float aScale = 1.0f;
		float aHeightF = (float)((aCount - 1) * aGap + aCount * anIconSize);
		int aHeight = Ftol((double)aHeightF);
		while (aMaxHeight < aHeight) {
			aScale = (float)((double)aScale - 0.009999999776482582);
			aHeight = Ftol((double)aHeightF * (double)aScale);
		}
		int aY = (-Ftol((double)gApp->mScale * -15.0) - aHeight / 2) + aScoreY;
		for (unsigned int j = 0; aCels.size() != 0 && j < aCels.size(); j++) {
			Image* anIcons = IMAGE_CHALLENGEICONS;
			int aCelWidth = Ftol((double)anIcons->GetCelWidth() * (double)aScale);
			int aCelHeight = Ftol((double)anIcons->GetCelHeight() * (double)aScale);
			Rect anIconRect(Ftol((double)mWidth * 0.5249999761581421 - (double)aCelWidth),
				Ftol((double)aY - (double)anIconSizeF * (double)aScale * 0.6000000238418579), aCelWidth, aCelHeight);
			g->DrawImageCel(anIcons, anIconRect, aCels[j]);
			g->SetFont(FONT_SCORE);
			SexyString aTimes = GT(0x811a00);
			Font* aFont = g->GetFont();
			int aDrop = Ftol((double)gApp->mScale * 14.0);
			int aTop = aY - g->GetFont()->mAscent;
			g->DrawString(aTimes, Ftol((double)mWidth * 0.5249999761581421), aFont->GetHeight() + aTop + aDrop);
			SexyString aValue = StrFormat(GTC(0x80da2c), aValues[j]);
			aFont = g->GetFont();
			aDrop = Ftol((double)gApp->mScale * 16.0);
			aTop = aY - g->GetFont()->mAscent;
			g->DrawString(aValue, Ftol((double)mWidth * 0.625), aFont->GetHeight() + aTop + aDrop);
			aY = Ftol((double)(aGap + anIconSize) * (double)aScale + (double)aY);
		}
	} else if ((aFlags & Challenge::FLAG_ONEMOVE) != 0) {
		// each ">=" goal of the move: an icon with "x" and the count so far, or (a stat without an icon) the
		// count's title and the count; a single goal moves up for the best under it
		std::vector<ChallengeGoal> aGoals = gApp->mChallengeInfo->GetGoals(aChallenge);
		std::vector<int> aCels;
		std::vector<int> aValues;
		int aLines = 0;
		for (unsigned int i = 0; aGoals.size() != 0 && i < aGoals.size(); i++) {
			if (aGoals[i].mOp == 2) {
				int aCel = -1;
				int aStat = aGoals[i].mStat;
				if (aStat > 0x40) {
					if (aStat == 0x41) {
						aCel = 8;
					} else if (aStat == 0x4a) {
						aCel = 10;
					} else if ((unsigned int)(aStat - 0x4b) <= 6) {
						// port: the original takes the colour from the tube's list (+0x56c, what the last pattern or
						// on-board display left there), not from this goal; no shipped challenge gets here (no
						// "Onemove" rule counts a colour). An index past that list leaves the goal without an icon
						if (i < mRuleGoals.size())
							aCel = mRuleGoals[i] - 0x4b;
					}
				} else if (aStat == 0x40) {
					aCel = 7;
				} else if (aStat == 0x14) {
					aCel = 8;
				} else if (aStat == 0x15) {
					aCel = 7;
				} else if (aStat == 0x18) {
					aCel = 9;
				}
				aCels.push_back(aCel);
				aValues.push_back(aGoals[i].mCurrent);
				if (aCel == -1)
					aLines += 2;
				else
					aLines++;
			}
		}
		g->SetFont(FONT_SCORE);
		int aLineHeight = g->GetFont()->mHeight - g->GetFont()->mAscentPadding;
		int aSpacing = Ftol((double)gApp->mScale * 20.0);
		int aY = aScoreY - ((aLines - 1) * aSpacing + aLineHeight * aLines) / 2;
		int aMidX = Ftol((double)mWidth * 0.5249999761581421);
		int aStep = aSpacing + aLineHeight;
		bool aSingle = aCels.size() == 1;
		if (aSingle) {
			double aBestShown = mBestFade.GetOutVal();
			aY = Ftol((double)aY - aBestShown * (double)Ftol((double)gApp->mScale * 10.0));
		}
		for (unsigned int i = 0; aCels.size() != 0 && i < aCels.size(); i++) {
			if (aCels[i] == -1) {
				SexyString aTitle;
				if (aChallenge->mCountTitle.empty())
					aTitle = GT(0x7df720);
				else
					aTitle = aChallenge->mCountTitle;
				g->SetFont(FONT_TOOLTIP);
				int aTitleY = aY + aLineHeight + Ftol((double)gApp->mScale * 0.0);
				g->DrawString(aTitle, aMidX - g->StringWidth(aTitle) / 2, aTitleY);
				aY += aStep - Ftol((double)gApp->mScale * -20.0);
				mCountTicker.Draw(g, aMidX, aY, true, theTipFade);
			} else {
				int aDrop = Ftol((double)gApp->mScale * -8.0);
				g->DrawImageCel(IMAGE_CHALLENGEICONS, Ftol((double)mWidth * 0.30000001192092896), aY + aDrop,
					aCels[i]);
				int aValueX = Ftol((double)mWidth * 0.6499999761581421);
				g->SetFont(FONT_SCORE);
				SexyString aTimes = GT(0x811a00);
				Font* aFont = g->GetFont();
				int aTimesDrop = Ftol((double)gApp->mScale * 39.0);
				int aTop = aY - g->GetFont()->mAscent;
				g->DrawString(aTimes, Ftol((double)mWidth * 0.5299999713897705),
					aFont->GetHeight() + aTimesDrop + aTop);
				SexyString aValue = StrFormat(GTC(0x80da2c), aValues[i]);
				int aValueY = aY + aLineHeight + Ftol((double)gApp->mScale * 0.0);
				g->DrawString(aValue, aValueX - g->StringWidth(aValue) / 2, aValueY);
				aY += aStep;
			}
		}
		if (aSingle && mBestFade.GetOutVal() != 0.0) {
			g->SetColor(Color(255, 255, 255, Ftol(mBestFade.GetOutVal() * 255.0 * (double)theTipFade)));
			SexyString aBest = GT(0x7e40a8);
			g->SetFont(FONT_TOOLTIP);
			int aBestY = Ftol((double)gApp->mScale * 55.0) + aScoreY;
			g->DrawString(aBest, aMidX - g->StringWidth(aBest) / 2, aBestY);
			float aBestAlpha = (float)(mBestFade.GetOutVal() * (double)theTipFade);
			mBestTicker.Draw(g, aMidX, aScoreY - Ftol((double)gApp->mScale * -90.0), true, aBestAlpha);
			g->SetColor(Color(255, 255, 255, anAlpha));
		}
	} else {
		// the count's title and the count (unless "nocount"), then the moves (left), or the best while +0x580 shows
		// it
		int aCount = gBoard->mChallengeCount;
		if ((aChallenge->mFlags & Challenge::FLAG_PATTERN) != 0)
			aCount = aChallenge->mProgress;
		bool aNoCount = (aChallenge->mFlags & Challenge::FLAG_NOCOUNT) != 0;
		SexyString aTitle;
		if (aChallenge->mCountTitle.empty())
			aTitle = GT(0x7e4078);
		else
			aTitle = aChallenge->mCountTitle;
		SexyString aCountText = StrFormat(GTC(0x80da2c), aCount);
		int aMidX = Ftol((double)mWidth * 0.5249999761581421);
		int aY = aScoreY - Ftol((double)gApp->mScale * -35.0);
		if (!aNoCount) {
			double aBestShown = mBestFade.GetOutVal();
			aY = Ftol((double)aY - aBestShown * (double)Ftol((double)gApp->mScale * 10.0));
			g->SetFont(FONT_TOOLTIP);
			int aTitleY = aY - Ftol((double)gApp->mScale * 42.0);
			g->DrawString(aTitle, aMidX - g->StringWidth(aTitle) / 2, aTitleY);
			mCountTicker.Draw(g, aMidX, aY, true, theTipFade);
			aY = aScoreY - Ftol((double)gApp->mScale * -80.0);
		}
		if ((aChallenge->mFlags & Challenge::FLAG_MOVES) != 0) {
			g->SetColor(Color(255, 255, 255, anAlpha));
			aTitle = aChallenge->mMoveGoal != 0 ? GT(0x7e4058) : GT(0x7df518);
			g->SetFont(FONT_TOOLTIP);
			int aMoves = aChallenge->mMoveGoal;
			if (aMoves != 0)
				aMoves -= gApp->mStats[0x21];
			else
				aMoves = gApp->mStats[0x21];
			if (aMoves < 0)
				aMoves = 0;
			aCountText = StrFormat(GTC(0x80da2c), aMoves);
			int aTitleY = aY - Ftol((double)gApp->mScale * 10.0);
			g->DrawString(aTitle, aMidX - g->StringWidth(aTitle) / 2, aTitleY);
			if (aChallenge->mRules.size() == 1) {
				g->SetFont(FONT_SCORE);
				aY += Ftol((double)gApp->mScale * 20.0);
			}
			int aMovesY = aY + Ftol((double)gApp->mScale * 15.0);
			g->DrawString(aCountText, aMidX - g->StringWidth(aCountText) / 2, aMovesY);
		} else if (mBestFade.GetOutVal() != 0.0) {
			g->SetColor(Color(255, 255, 255, Ftol(mBestFade.GetOutVal() * 255.0 * (double)theTipFade)));
			aTitle = GT(0x7e40a8);
			g->SetFont(FONT_TOOLTIP);
			int aBestY = aY - Ftol((double)gApp->mScale * 20.0);
			g->DrawString(aTitle, aMidX - g->StringWidth(aTitle) / 2, aBestY);
			float aBestAlpha = (float)(mBestFade.GetOutVal() * (double)theTipFade);
			mBestTicker.Draw(g, aMidX, aY - Ftol((double)gApp->mScale * -10.0), true, aBestAlpha);
			g->SetColor(Color(255, 255, 255, anAlpha));
		}
	}
	if (aShowBestEver) {
		// the night mode's best ever under the display (+0x150)
		g->SetColor(Color(255, 255, 255, anAlpha));
		int aMidX = Ftol((double)mWidth * 0.5249999761581421);
		int aDrop = Ftol((double)gApp->mScale * 100.0);
		int aBestY = GetScoreY() + aDrop;
		SexyString aText = GT(0x7e4038);
		g->SetFont(FONT_TOOLTIP);
		int aTitleY = aBestY - Ftol((double)gApp->mScale * 35.0);
		g->DrawString(aText, aMidX - g->StringWidth(aText) / 2, aTitleY);
		aText = StrFormat(GTC(0x80da2c), mBestSoFar);
		int aValueY = aBestY - Ftol((double)gApp->mScale * 10.0);
		g->DrawString(aText, aMidX - g->StringWidth(aText) / 2, aValueY);
	}
}
