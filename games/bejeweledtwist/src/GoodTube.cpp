// Sexy::GoodTube (0x702736-0x70a9bb): being ported, see GoodTube.h.
#include "GoodTube.h"

#include "Board.h"
#include "EffectsManager.h"
#include "GameOverShow.h"
#include "GameText.h"
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
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

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
static unsigned int GetPixelAt(Image* theImage, int theX, int theY)
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
	mUnknown6C = FONT_TOOLTIP;
	mUnknown68 = FONT_TOOLTIP;
	mFont = FONT_SCORE;
	mGlowFont = FONT_SCOREGLOW;
	mSmallFont = FONT_SCORE_SMALL;
	mSmallGlowFont = FONT_SCOREGLOW_SMALL;
	mUnknown80 = false;
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
	if (1999999 < mValue || mUnknown80) {
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
	mUnknown308 = 1.0f;
	mUnknown30C = 0;
	mName = "GOODTUBE";
	mUnknown2C8 = (float)Ftol((double)gApp->mScale * 0.0);
	mUnknown160 = 1;
	mUnknown4A8 = true;
	mColorCycle.mColors.clear();
	mColorCycle.mPos = 0;
	mColorCycle.mColors.push_back(Color::White);
	UpdateImages();
	SetUnknown14D(false);
	SetShown(false);
	for (int i = 16; i != 0; i--) {
		Bubble* aBubble = new Bubble();
		aBubble->mUnknown0 = 0;
		aBubble->mPhase = (float)((double)(Rand() % 100) * 0.001);
		mBubbles.push_back(aBubble);  // 0x707ad5
	}
	mLevelText = new TextImage(FONT_INGAME, GTS(0x7e4018), 0, 0, -1, 0, nullptr);  // 0x74e701 ("LEVEL"; nothing draws it)
	mRankBadge = new RankBadge(this);
	mUnknown240 = Ftol((double)gApp->mScale * 200.0);
	mMenuButton = new TwistButton(0, IMAGE_INGAMEUI_MENUBUTTON, this);
	mMenuButton->mName = "MENU_BUTTON";
	if (IMAGE_INGAMEUI_MENUBUTTON != nullptr) {
		mMenuButton->mNormalRect = IMAGE_INGAMEUI_MENUBUTTON->GetCelRect(0);
		mMenuButton->mOverRect = IMAGE_INGAMEUI_MENUBUTTON->GetCelRect(1);
		mMenuButton->mDownRect = IMAGE_INGAMEUI_MENUBUTTON->GetCelRect(2);
	}
	mMenuButton->mColors[0] = Color(255, 255, 255);
	mFXManager = new EffectsManager();
	mUnknown2E0 = std::fabs((float)RandSigned());
	mUnknown338.SetCurve(GTS(0x7e3e80));  // 0x704d9c
}

RankBadge::RankBadge(GoodTube* theTube)
{
	// 0x701efb
	mUnknown154 = 0;
	mOver = false;
	mUnknown150 = 0;
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
	return aBoard != nullptr && (aBoard->IsBusy() || aBoard->mUnknown37C4 || aBoard->mUnknown160 != 0 ||
		aBoard->mUnknown1570 != 0);
}

void RankBadge::MouseEnter()
{
	// 0x701fbb
	if (BadgeIgnoresMouse())
		return;
	mUnknown154 = 0;
	mUnknown150 = 0;
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
	mUnknown154++;
	mUnknown150 = 0;
	if (1 < mUnknown154)
		mUnknown154 = 0;
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

void GoodTube::SetUnknown158(int theValue)
{
	mUnknown158 = theValue;
	if (theValue == 3) {
		// port: sets a CurvedVal to 1 (CurvedVal::SetConstant), once it is known which
		NotPorted("GoodTube::SetUnknown158(3)");
	}
}

void GoodTube::SetChallenge(bool on)
{
	mChallenge = on;
	UpdateImages();
}

void GoodTube::SetTime(int theTime, bool theAtOnce)
{
	mUnknown4B4 = theTime;
	if (mUnknown4BC == 0)
		mUnknown4BC = theTime;
	if (theAtOnce) {
		mUnknown4B8 = theTime;
		UpdateFill(theAtOnce);
	}
}

void GoodTube::StartUnknown4AC(bool theFour, bool theUnknown4A9)
{
	if (mUnknown4AC < 1) {
		mUnknown4B0 = 0;
		mUnknown4C0 = 0;
		mUnknown4AC = 1;
		mUnknown400.SetCurve(GTS(0x7e3fe0));
		if (theFour)
			mUnknown400.mInVal = mUnknown400.mInMax;
		mUnknown400.mIncRate = 0;
	}
	if (theFour)
		mUnknown4AC = 4;
	mUnknown4A9 = theUnknown4A9;
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

void GoodTube::UpdateImages()
{
	mTopImage = mChallenge ? IMAGE_INGAMEUI_TUBE_TOP_CHALLENGE : IMAGE_INGAMEUI_TUBE_TOP;
	mBottomImage = IMAGE_INGAMEUI_TUBE_BOTTOM;
	mGlassImage = IMAGE_INGAMEUI_TUBE_GLASS;
	mFillImage = IMAGE_INGAMEUI_TUBE_FILL_GOLD;
	mFillImage2 = IMAGE_INGAMEUI_TUBE_FILL_GOLD;
	if (mGlassImage == nullptr)
		mUnknown16C = 0;
	else
		mUnknown16C = GetBottomTop() - GetGlassTop();
	float aTop = (float)(GetGlassTop() + mUnknown16C);
	mUnknown198 = aTop;
	mUnknown194 = aTop;
}

int GoodTube::GetTubeHeight()
{
	if (mUnknown16C == 0)
		return 0;
	return Ftol((double)(mGlassImage->mHeight + mBottomImage->mHeight + mTopImage->mHeight) * 0.9300000071525574);
}

int GoodTube::GetTubeWidth()
{
	if (mUnknown16C == 0)
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
	return Ftol((double)aTop + (double)mUnknown2D0);
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
	if (mUnknown15C == 1) {
		// Blitz: the time left
		float aProgress;
		if (mUnknown4BC == 0)
			aProgress = 0.0f;
		else
			aProgress = (float)mUnknown4B8 / (float)mUnknown4BC;
		mProgress = aProgress;
	}
	int aMargin = Ftol((double)gApp->mScale * 35.0);
	mFillTop = GetGlassTop() + aMargin;
	int aBottom = Ftol((double)gApp->mScale * 5.0) + GetBottomTop();
	mFillBottom = aBottom;
	int aHeight = Ftol((double)(aBottom - mFillTop) * (double)mProgress);
	if (0 < mUnknown300)
		aHeight = 0;
	float aTop = (float)(aBottom - aHeight);
	mUnknown198 = aTop;
	if (mBoard->mHyperspace == nullptr && !theAtOnce)
		mUnknown194 = (float)(((double)aTop - (double)mUnknown194) / 30.0 + (double)mUnknown194);
	else
		mUnknown194 = aTop;
}

void GoodTube::SetLevelSteps(int theSteps)
{
	if (mUnknown274 < theSteps) {
		int aFlash = mUnknown1D8 + 30;
		int aMax = 200;
		if (aFlash < 200)
			aMax = aFlash;
		mUnknown1D8 = aMax;
		mUnknown1E0 = 0.5f;
	}
	if (theSteps < mUnknown274) {
		MarkLevelChange();
		mUnknown29C = mUnknown29C * 0.5f;
	}
	mUnknown274 = theSteps;
}

void GoodTube::SetLevelShown(int theLevel)
{
	if (!mUnknown14D)
		return;
	if (10 < theLevel)
		theLevel = 10;
	if (theLevel != mUnknown27C) {
		float aSteps;
		if (mUnknown27C < theLevel) {
			if (!mBoard->IsBusy()) {
				int aSound = 3;
				if (theLevel < 4)
					aSound = theLevel;
				// SOUND_MULTIPLIER_UP2_1 is resource 0x397 (0x397 + the level, at most 3)
				gApp->PlaySample(GetSoundById(SOUND_MULTIPLIER_UP2_1_ID + aSound));
			}
			mUnknown29C = 1.0f;
			gBoard->mUnknown20B8 = 1.0f;
			int aFlash = mUnknown1D8 + 64;
			int aMax = 255;
			if (aFlash < 255)
				aMax = aFlash;
			mUnknown1D8 = aMax;
			mUnknown1E0 = 0.5f;
			aSteps = 0.0f;
		} else {
			if (!mBoard->IsBusy())
				gApp->PlaySample(SOUND_MULTIPLIER_DOWN);
			aSteps = (float)Board::GetLevelSteps(theLevel);
		}
		mUnknown278 = aSteps;
	}
	mUnknown27C = theLevel;
	mUnknown280 = theLevel;
}

void GoodTube::GetStepPoint(int* thePoint, int theStep, bool theCentre)
{
	int aLevel = mUnknown27C;
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
	mUnknown3B0 = 0;
	for (unsigned int i = 0; i < mUnknown3D0.size(); i++) {
		mUnknown3D0[i] = 0;
		mUnknown3E0[i] = 0;
	}
	mUnknown3B4 = 0;
	mUnknown3F0 = 1.0f;
	mUnknown3B8 = 0;
	mUnknown3C4 = 0;
}

void GoodTube::AddedToManager(WidgetManager* theWidgetManager)
{
	// 0x703582 (the base class' is not called)
	if (mUnknown16C != 0) {
		mWidth = GetTubeWidth();
		mHeight = GetTubeHeight();
	}
	AddWidget(mMenuButton);
	AddWidget(mRankBadge);
	SetShown(mShown);
	SetUnknown14D(mUnknown14D);
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
		if (aBoard->mUnknown37C4)
			return;
		if (aBoard->mUnknown160 != 0)
			return;
		if (aBoard->mUnknown1570 != 0)
			return;
	}
	gApp->PlaySoundEx(SOUND_BUTTONPRESS2, 0, 5.0f, 0.0f, true, true);
	if (theId == 0) {
		// 0x6a6983: the board's dialog goes (+0x138c) and the menu comes up (0x75f6fc)
		NotPorted("GoodTube::ButtonDepress: the board's menu (0x6a6983)");
	}
}

void GoodTube::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// 0x70318f
	Widget::Resize(theX, theY, theWidth, theHeight);
	if (mUnknown16C != 0) {
		int anArtRes = gApp->mArtRes;
		mUnknown2A0 = theX;
		mUnknown168 = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_INGAMEUI_TUBE_GLASS_ID));
		mUnknown16C = mGlassImage->mHeight;
		mUnknown164 = GetGlassTop() + mUnknown16C;
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
	// 0x7049e5 (Direct3D: while the board's scale is not 1, the board's matrix is pushed here and popped at the end)
	if (mUnknown3B0 != 0 && mUnknown3B0 != 4)
		DrawBonus(g);
	mFXManager->Draw(g);
	if (mUnknown5F0.GetOutVal() != 0.0 && !gBoard->mUnknown37C4) {
		float anAlpha = (float)mUnknown5F0.GetOutVal();
		// (Direct3D: during the jump's transitions 8 and 12 the alpha is multiplied by the curve again)
		g->SetColorizeImages(true);
		g->SetDrawMode(0);
		g->SetColor(Color(255, 255, 255, Ftol((double)anAlpha * 255.0)));
		g->DrawImage(IMAGE_MEGAFRUIT_BRACKET, Ftol((double)gApp->mScale * 72.0),
			Ftol((double)gApp->mScale * 842.0));
	}
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
	return Ftol((double)anY + (double)mUnknown2C8);
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
	if (mUnknown3B0 == 0) {
		ResetBonus();
		mUnknown3B8 = 1.0f;
		mUnknown3B4 = 1.0f;
		mUnknown3F4 = false;
		mUnknown3A8 = 0;
		mUnknown3C0 = 0;
		mUnknown338.SetInVal(0.0);
		mUnknown3B0 = 1;
		mUnknown3BC = gBoard->mUnknown2658;
		int aSteps = 0;
		for (int aLevel = 1; aLevel <= mBoard->mUnknown21D0; aLevel++)
			aSteps += Board::GetLevelSteps(aLevel);
		mUnknown3FC = (aSteps - mBoard->mUnknown818) - 1;
	}
}

void GoodTube::Update()
{
	// 0x702736: the update ticks as often as mUnknown308 says
	mMenuButton->mMouseVisible = true;
	if (mBoard != nullptr) {
		// (Direct3D: the menu button's mouse is off while the jump's curve is under 0.5)
		Board* aBoard = mBoard;
		if (aBoard->IsBusy())
			return;
		if (0 < aBoard->mUnknown1570)
			return;
	}
	float aSum = (float)((double)mUnknown308 + (double)mUnknown30C);
	mUnknown30C = aSum;
	while (1.0f <= mUnknown30C) {
		mUnknown30C = (float)((double)mUnknown30C - 1.0);
		UpdateTick();
	}
}

void GoodTube::UpdateTick()
{
	// 0x70a9bb
	float anArtScale = (float)((double)gApp->mArtRes / 1200.0);
	mUpdateCnt++;
	mColorCycle.Update();
	mTicker660.mTarget = gBoard->mUnknown2638;
	mTicker6E4.mTarget = gBoard->mUnknown2634;
	if (mUnknown150 <= gBoard->mUnknown2634)
		mUnknown150 = gBoard->mUnknown2634;
	mTicker660.Update();
	mTicker6E4.Update();

	// The bonus challenge's bar: 1 the colours light up one by one, 2 they wait for the player, 4 they fade away
	Board* aBoard = gBoard;
	int aState = mUnknown3B0;
	if (aState != 0) {
		if (aState == 1) {
			int aCount = (int)aBoard->mBonusColors.size();
			if (aCount != 0) {
				mUnknown3B4 = (float)((double)mUnknown3B4 - 0.0035000001080334187);
				if (mUnknown3B4 < 0.0f) {
					mUnknown3B0 = 2;
					mUnknown3B4 = 1.0f;
				}
				int aLight;
				if (mUnknown3B4 <= 0.25f) {
					aLight = -1;
					sBonusLight = -1;
				} else {
					aLight = Ftol(((1.0 - (double)mUnknown3B4) * (double)aCount) * 4.0);
					aLight = aLight % aCount;
				}
				if (aLight != sBonusLight)
					sBonusLight = aLight;
				int i = 0;
				if (0 < aCount) {
					do {
						float aGlow;
						if (i == sBonusLight) {
							aGlow = (float)((double)mUnknown3E0[i] + 0.05000000074505806);
						} else {
							aGlow = 0.0f;
							if (0.0 < (double)mUnknown3E0[i] - 0.019999999552965164)
								aGlow = (float)((double)mUnknown3E0[i] - 0.019999999552965164);
						}
						i++;
						mUnknown3E0[i - 1] = aGlow;
					} while (i < aCount);
				}
			}
		} else if (aState == 2) {
			int aCount = (int)aBoard->mBonusColors.size();
			if (aCount != 0) {
				float aLastProgress = mUnknown3B4;
				if (0.5f < mUnknown3B4 ||
					(aBoard->mUnknown2584 == nullptr && aBoard->mUnknown25A4 && aBoard->mUnknown2208 == nullptr))
					mUnknown3B4 = (float)((double)mUnknown3B4 - 0.004999999888241291);
				float aFade = (float)((double)mUnknown3C4 + 0.019999999552965164);
				if (1.0f < aFade)
					aFade = 1.0f;
				mUnknown3C4 = aFade;
				if (mUnknown3B4 < 0.800000011920929) {
					float aLeft = (float)((double)mUnknown3F0 - 0.009999999776482582);
					if (aLeft < 0.0f)
						aLeft = 0.0f;
					mUnknown3F0 = aLeft;
					for (int i = 0; i < aCount; i++) {
						float aValue = 0.0f;
						if (0.0 < (double)mUnknown3D0[i] - 0.05000000074505806)
							aValue = (float)((double)mUnknown3D0[i] - 0.05000000074505806);
						mUnknown3D0[i] = aValue;
					}
				}
				if (mUnknown3B4 < 0.800000011920929 && 0.800000011920929 <= aLastProgress)
					mUnknown3F8 = 8.0f;
				if (mUnknown3B4 < 0.30000001192092896f && 0.30000001192092896f <= aLastProgress) {
					Piece* aHint = aBoard->FindHintPiece();
					if (aHint == nullptr) {
						mUnknown3F8 = (float)((double)mUnknown3F8 + 4.0);
					} else {
						// 0x74b94e: a FruitShockwave (0x110 bytes, a Shockwave) at the hint's gem, then 0x6bc3cd adds it to the
						// board's waves; it takes the hint, 1, and the tube's middle (Ftol(mWidth * 0.5 + mX), Ftol(mHeight *
						// 0.85 + mY)) and 200
						NotPorted("GoodTube::UpdateTick: the bonus challenge's hint wave (0x74b94e)");
					}
				}
				if (mUnknown3B4 <= 0.0f) {
					mUnknown3B0 = 4;
					mUnknown3B4 = 1.0f;
				}
			}
		} else if (aState == 4) {
			if (mUnknown330 <= 0.0f) {
				mUnknown3B4 = (float)((double)mUnknown3B4 - 0.009999999776482582);
				if (mUnknown3B4 <= 0.0f) {
					mUnknown338.SetInVal(0.0);
					mUnknown3C4 = 0;
					mUnknown3B0 = 0;
					mBoard->StartBonusChallenge(-1);
				}
			} else {
				float aShown = (float)((double)mUnknown330 - 0.029999999329447746);
				mUnknown330 = aShown;
				if (!(0.0f < aShown)) {
					mUnknown330 = 0;
					mUnknown334 = 1.0f;
					mUnknown3F0 = 1.0f;
				}
				mUnknown3B4 = 1.5f;
			}
		}
	}

	if (mChallenge && aBoard->mUnknown2630 != 0) {
		// port: the challenge's goal (+0x538 and the curve at +0x4c8) waits for the challenges
		NotPorted("GoodTube::UpdateTick: the challenge's goal");
	}

	if (mUnknown2A0 != mX)
		mUnknown2A0 = mX;
	if (0.0f < mUnknown29C) {
		float aValue = (float)((double)mUnknown29C - 0.009999999776482582);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknown29C = aValue;
	}
	if (mUnknown294 <= mUnknown290) {
		if (mUnknown294 < mUnknown290) {
			float aValue = (float)((double)mUnknown294 + 0.009999999776482582);
			if (1.0f < aValue)
				aValue = 1.0f;
			mUnknown294 = aValue;
		}
	} else {
		float aValue = (float)((double)mUnknown294 - 0.009999999776482582);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknown294 = aValue;
	}
	mFXManager->Update();
	if (mUnknown1E0 <= 0.0f) {
		mUnknown1D8 -= 2;
		if (mUnknown1D8 < 0)
			mUnknown1D8 = 0;
	} else {
		float aValue = (float)((double)mUnknown1E0 - 0.009999999776482582);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknown1E0 = aValue;
	}
	if (mUnknown1D8 < 1) {
		mUnknown1DC = 0;
	} else {
		float aWave = (float)((double)mUnknown1E0 * 3.141590118408203);
		float aSin = (float)std::sin((double)aWave);
		float aScale = (float)((double)aSin + (double)aSin + 1.0);
		aScale = std::fabs(aScale);
		int aFlash = mUnknown1D8;
		int aGlow = aFlash - Ftol((double)aFlash * (double)mUnknown1E0 * -0.5 * (double)aScale);
		mUnknown1DC = aGlow;
		if (aGlow < 0x100) {
			if (aGlow < 0)
				aGlow = 0;
		} else {
			aGlow = 0xff;
		}
		mUnknown1DC = aGlow;
	}
	mFillTop = GetGlassTop() + 0x14;
	mFillBottom = GetBottomTop();
	// port: the two text popups at +0x1cc and +0x1d0 are updated here (a popup that is done goes); none is made yet

	if (0.0f < mUnknown1C4) {
		float aValue = (float)((double)mUnknown1C4 - 0.00800000037997961);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknown1C4 = aValue;
	}
	if (0.0f < mUnknown1C8) {
		float aValue = (float)((double)mUnknown1C8 - 0.009999999776482582);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknown1C8 = aValue;
	}

	// The specks rise in the tube
	unsigned int aSpeck = 0;
	if (mSpecks.size() != 0) {
		int aMargin = Ftol((double)anArtScale * 23.0);
		do {
			Speck* s = mSpecks[aSpeck];
			float aWaveT = (float)((double)mUpdateCnt * 0.009999999776482582 + (double)(int)aSpeck);
			float aWaveSin = (float)std::sin((double)aWaveT);
			float aDX = (float)((double)aWaveSin * 0.009999999776482582 + (double)s->mDX);
			s->mDX = aDX;
			s->mX = (float)((double)s->mX + (double)aDX);
			s->mY = (float)((double)s->mDY + (double)s->mY);
			s->mDY = (float)((double)s->mDDY + (double)s->mDY);
			int aMiddle = mGlassImage->mWidth / 2 - 3 + mUnknown168;
			float aLow = (float)(aMiddle - aMargin);
			if (aLow <= s->mX) {
				float aHigh = (float)(aMiddle + aMargin);
				if (aHigh < s->mX) {
					s->mX = aHigh;
					s->mDX = (float)((double)std::fabs(aDX) * -0.5);
				}
			} else {
				s->mX = aLow;
				s->mDX = (float)((double)std::fabs(aDX) * 0.5);
			}
			if (s->mY < mUnknown194) {
				s->mAlpha = (float)((double)s->mDAlpha + (double)s->mAlpha);
				s->mDDY = 0.019999999552965164f;
			}
			if (s->mAlpha <= 0.0f || (mUnknown194 <= s->mY && 0.0f < s->mDDY)) {
				mSpecks[aSpeck] = mSpecks[mSpecks.size() - 1];
				if (mSpecks.size() != 0)
					mSpecks.pop_back();
				aSpeck--;
				delete s;
			}
			aSpeck++;
		} while (aSpeck < mSpecks.size());
	}
	mUnknown18C = (float)((double)6.2831854820251465 / 200.0 + (double)mUnknown18C);
	if (6.2831854820251465 < (double)mUnknown18C)
		mUnknown18C = (float)((double)mUnknown18C - 6.2831854820251465);
	{
		float aTurn = (float)((double)6.2831854820251465 / 400.0 + (double)mUnknown190);
		mUnknown190 = aTurn;
		if (6.2831854820251465 < (double)aTurn)
			mUnknown190 = (float)((double)aTurn - 6.2831854820251465);
	}

	int aMode = mUnknown4AC;
	if (aMode < 1 || 2 < aMode) {
		float aValue = (float)((double)mUnknown4A4 - 0.029999999329447746);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknown4A4 = aValue;
	} else {
		float aValue = (float)((double)mUnknown4A4 + 0.05000000074505806);
		if (1.0f < aValue)
			aValue = 1.0f;
		mUnknown4A4 = aValue;
	}
	if (0 < aMode && mBoard->mHyperspace == nullptr) {
		float aValue = (float)((double)mUnknown4C0 + 0.019999999552965164);
		if (1.0f < aValue)
			aValue = 1.0f;
		mUnknown4C0 = aValue;
	}
	if (aBoard->mGameMode == Board::MODE_BLITZ) {
		if (aMode == 1) {
			aBoard->mUnknown37F4 = 0.5f;
			if (1.0f <= mUnknown4C0)
				mUnknown4AC++;
		} else if (aMode == 2) {
			aBoard->mUnknown37F4 = 0.5f;
			if (mUpdateCnt % 5 == 0) {
				int aMax = mUnknown4B4;
				mUnknown4B8 += 1000;
				int aTime = mUnknown4B8;
				if (aMax <= aTime) {
					mUnknown400.mIncRate = 0.019999999552965164;
					mUnknown4B8 = aMax;
					mUnknown400.SetInVal(mUnknown400.mInMin);
					gApp->PlaySample(SOUND_TALLY_ZOOMIN);
					mUnknown4AC++;
				} else {
					float aRatio = (float)((double)aTime / (double)aMax);
					float aPitch = (float)((double)aRatio * 5.0);
					gApp->PlaySoundEx(SOUND_BLITZ_COUNTUP, 0, aPitch, 0.0f, true, true);
				}
			}
		} else if (aMode == 3) {
			aBoard->mUnknown37F4 = 0.5f;
			if (!mUnknown400.IncInVal())
				mUnknown4AC++;
		}
	}
	UpdateFill(false);
	int aSteps = Board::GetLevelSteps(mUnknown27C);
	int aLit = mUnknown274 + 1;
	if (aSteps < aLit)
		aLit = aSteps;
	mUnknown25C = aLit;
	if (0 < aSteps) {
		int aLight = aSteps;
		float* aFlash = mUnknown1E4;
		do {
			float aValue = (float)((double)*aFlash - 0.009999999776482582);
			if ((double)*aFlash - 0.009999999776482582 < 0.0)
				aValue = 0.0f;
			*aFlash = aValue;
			aFlash++;
			aLight--;
		} while (aLight != 0);
	}
	int aLastLit = mUnknown260;
	int aNowLit = mUnknown25C;
	if (aLastLit != aNowLit) {
		int aRow = (aNowLit - 1 + ((aNowLit - 1) >> 31 & 3)) >> 2;
		if (aRow != mUnknown264)
			mUnknown264 = aRow;
		if (aLastLit < aNowLit) {
			float* aFlash = &mUnknown1E4[aLastLit];
			do {
				aLastLit++;
				*aFlash = 1.0f;
				aFlash++;
			} while (aLastLit < mUnknown25C);
		}
	}

	if (mProgress != 0.0f && 0 < mWidth + mX && (mUnknown1B0 = mUnknown1B0 - 1, mUnknown1B0 < 1)) {
		mUnknown1B0 = Rand() % 20 + 5;
		if ((double)GetGlassTop() < (double)mUnknown194) {
			Speck* s = new Speck();
			int aWidth = mGlassImage->mWidth;
			int aRand = Rand();
			s->mX = (float)(aRand % (aWidth / 2 - 15) + ((aWidth + (aWidth >> 31 & 3)) >> 2) + mUnknown168);
			s->mY = (float)GetBottomTop();
			s->mDY = (float)-((double)(Rand() % 10) / 10.0 + 0.5);
			s->mAlpha = 1.0f;
			double aTopY = (double)s->mY;
			s->mImage = IMAGE_SPARKLET;
			if ((double)(GetGlassTop() + 0x50) <= aTopY)
				s->mDAlpha = -0.01f;
			else
				s->mDAlpha = (float)-(((aTopY - (double)GetGlassTop()) * 0.019999999552965164) / 80.0 + 0.019999999552965164);
			s->mDDY = -0.001f;
			s->mScale = (float)((double)std::fabs(RandSigned()) * 0.25 + 0.75);
			if ((Rand() & 1) == 0) {
				s->mScale = (float)((double)s->mScale * 0.5);
				s->mImage = IMAGE_SPARKLET;
			} else {
				s->mImage = IMAGE_INGAMEUI_TUBE_FILL_PARTICLE;
			}
			mSpecks.push_back(s);  // 0x707b2a
		}
	}
	for (unsigned int i = 0; i < mBubbles.size(); i++) {
		Bubble* aBubble = mBubbles[i];
		float aPhase = (float)((double)aBubble->mPhase + (double)aBubble->mUnknown0);
		aBubble->mUnknown0 = aPhase;
		if (6.2831854820251465 < (double)aPhase)
			aBubble->mUnknown0 = (float)((double)aPhase - 6.2831854820251465);
	}
	if (0.0f < mUnknown248) {
		mUnknown248 = (float)((double)mUnknown248 - 0.009999999776482582);
		float aValue = (float)((double)mUnknown244 + 0.03999999910593033);
		if (1.0f < aValue)
			aValue = 1.0f;
		mUnknown244 = aValue;
	} else {
		float aValue = (float)((double)mUnknown244 - 0.03999999910593033);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknown244 = aValue;
	}

	// The light the chain is on eases to the chain's steps
	float aGap = (float)mUnknown274 - mUnknown278;
	float aMore = (float)((double)Board::GetLevelSteps(mUnknown27C) * 0.09375 + 0.25);
	if (!(std::fabs(aGap) <= 0.004999999888241291f)) {
		float aNext;
		if ((float)mUnknown274 < mUnknown278) {
			double aDrop = ((double)aGap * 0.009999999776482582 - 0.029999999329447746) * (double)aMore;
			if (aDrop <= (double)aGap * 0.9900000095367432)
				aDrop = (double)aGap * 0.9900000095367432;
			aNext = (float)(aDrop + (double)mUnknown278);
		} else if (mUnknown278 < (float)mUnknown274) {
			double aRise = (double)aGap * 0.05000000074505806 + 0.05000000074505806;
			if ((double)aGap * 0.9900000095367432 <= aRise)
				aRise = (double)aGap * 0.9900000095367432;
			aNext = (float)(aRise + (double)mUnknown278);
		} else {
			aNext = mUnknown278;
		}
		mUnknown278 = aNext;
	} else {
		mUnknown278 = (float)mUnknown274;
	}
	float aShownSteps = (float)Board::GetLevelSteps(mUnknown27C);
	if (aShownSteps < mUnknown278 && mUnknown14D && 0.0 < (double)mUnknown278 - (double)aShownSteps) {
		int aLightId = 0;
		int aHue0 = Board::GetLevelSteps(mUnknown27C) * 0x28;
		do {
			int aPointA[2];
			int aPointB[2];
			GetStepPoint(aPointA, aLightId, false);
			int aLightX = aPointA[0];
			GetStepPoint(aPointB, aLightId, false);
			int aLightY = aPointB[1];
			Image* aLightImage = GetImageById(sFirstLight[mUnknown27C] + aLightId);
			float aRX = (float)(Rand() % aLightImage->mWidth);
			float aRY = (float)(Rand() % aLightImage->mHeight);
			unsigned int aPixel = GetPixelAt(aLightImage, Ftol((double)aRX), Ftol((double)aRY));
			if (aPixel > 0x80000000u) {
				Effect* e = mFXManager->AllocEffect(0);
				e->mIsAdditive = true;
				e->mX = (float)((double)aLightX + (double)aRX);
				e->mImage = IMAGE_GEM_FRUIT_SPARK;
				e->mY = (float)((double)aLightY + (double)aRY);
				e->mDX = 0;
				e->mDY = 0;
				e->mGravity = 0;
				e->mAngle = 0;
				e->mDAngle = 0;
				e->mAlpha = 1.0f;
				e->mDAlpha = -0.05f;
				int aHue = (Rand() % 120 - Ftol((double)mUpdateCnt * -1.5)) + aHue0;
				aHue = aHue % 256;
				e->mColor = Color((int)HSLToColor(aHue, 0x80, 0x40));
				e->mScale = 0.2f;
				e->mDScale = -0.0003f;
				mFXManager->AddEffect(e);
			}
			aLightId++;
			aHue0 -= 0x28;
		} while ((double)aLightId < (double)mUnknown278 - (double)aShownSteps);
	}
	if (0.0f < mUnknown2A4) {
		// Sparks burst from the rank badge's middle (0x702078, 0x70208a)
		int aBadgeX = mRankBadge->mWidth / 2 + mRankBadge->mX;
		int aBadgeY = mRankBadge->mHeight / 2 + mRankBadge->mY;
		mUnknown2A4 = (float)((double)mUnknown2A4 - 0.009999999776482582);
		float aCentreX = (float)aBadgeX;
		float aCentreY = (float)aBadgeY;
		int aRounds = 3;
		do {
			float anAngle = (float)((double)RandSigned() * 1.100000023841858);
			float aSpread = (float)RandSigned();
			float aSpeed = (float)((double)std::fabs(aSpread) + 2.0);
			int i = 0;
			do {
				Effect* e = mFXManager->AllocEffect(0);
				e->mImage = IMAGE_SPARKLET;
				e->mX = (float)((double)(float)std::sin(anAngle) * 80.0 * (double)gApp->mScale + (double)aCentreX);
				e->mY = (float)((40.0 - (double)(float)std::cos(anAngle) * 30.0) * (double)gApp->mScale + (double)aCentreY);
				e->mDX = (float)std::sin(anAngle) * aSpeed;
				e->mDY = (float)((double)(float)std::cos(anAngle) * -1.0 * (double)aSpeed);
				e->mAlpha = 1.0f;
				e->mDAlpha = -0.02f;
				e->mScale = (float)((double)RandSigned() + 4.0);
				e->mColor = Color(255, 200, 16);
				e->mIsAdditive = true;
				if (i == 1) {
					e->mScale = (float)((double)e->mScale * 0.25);
					e->mColor = Color::White;
					e->mIsAdditive = true;
				}
				mFXManager->AddEffect(e);
				i++;
			} while (i < 2);
			aRounds--;
		} while (aRounds != 0);
	}
	mUnknown260 = mUnknown25C;

	// The glass breaks (mUnknown300 is 1 then)
	if (mUnknown300 == 1) {
		if ((int)mUnknown2F0.size() < 2 && mUnknown2B8 < 1.0f) {
			double aFall = ((double)mUnknown2B8 * 0.0 + (double)mUnknown2B8) + 0.009999999776482582;
			if (1.0 < aFall)
				aFall = 1.0;
			mUnknown2B8 = (float)aFall;
		}
		if (1.0f <= mUnknown2B8) {
			float aFloor = (float)((double)anArtScale * 420.0);
			bool aBefore = mUnknown2C8 <= aFloor;
			mUnknown2C8 = (float)((double)anArtScale * (double)mUnknown2CC + (double)mUnknown2C8);
			float aSpeed = (float)((double)mUnknown2CC + 0.07500000298023224);
			mUnknown2CC = aSpeed;
			if (aFloor < mUnknown2C8) {
				if (aBefore) {
					mUnknown2CC = (float)((double)aSpeed * 0.4000000059604645);
					gApp->PlaySample(SOUND_BOMB_EXPLODE);
				}
				mUnknown2D0 = mUnknown2C8 - aFloor;
				mUnknown2B0 = mUnknown2B4 + mUnknown2B0;
			}
			mUnknown2C0 = mUnknown2C4 + mUnknown2C0;
			int aTopY = GetTopY();
			int aShardY = Ftol((double)mTopImage->mHeight * 0.949999988079071 + (double)aTopY);
			if (gBoard->mHeight < aTopY) {
				mUnknown300 = 2;
			} else if (mUpdateCnt % 10 == 0 && mUnknown2D8 < 1.0f) {
				double aShare = (double)(aShardY - GetGlassTop()) / (double)mGlassImage->mHeight - 0.10000000149011612;
				if (1.0 < aShare)
					aShare = 1.0;
				float aShareF = (float)aShare;
				mUnknown2D8 = aShareF;
				if (!(0.0f <= aShareF))
					mUnknown2D8 = 0;
				float aShardBase = (float)aShardY;
				for (int aShard = 10; aShard != 0; aShard--) {
					Effect* e = gBoard->mFXManager->AllocEffect(0);
					e->mDX = (float)((double)RandSigned() * 6.0);
					e->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
					e->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.019999999552965164);
					e->mX = (float)((double)(mWidth / 2 + mX) + (double)e->mDX * 10.0);
					e->mY = (float)((double)std::fabs(RandSigned()) * 10.0 + (double)aShardBase);
					e->mIsAdditive = false;
					e->mDY = (float)((double)RandSigned() * 0.4000000059604645 + (double)mUnknown2CC * 0.5);
					e->mAlpha = 1.0f;
					e->mDAlpha = -0.01f;
					e->mGravity = 0.03f;
					e->mImage = IMAGE_GLASS_SHARD1;
					float aGrow = (float)((double)RandSigned() * 0.004000000189989805);
					e->mDScale = aGrow;
					float aScale = (float)((double)aGrow * 10.0 + 1.0);
					e->mScale = aScale;
					double aShade = (double)aScale * 128.0;
					if (255.0 < aShade)
						aShade = 255.0;
					int aGrey = Ftol(aShade);
					e->mColor = Color(aGrey, aGrey, aGrey);
					gBoard->mFXManager->AddEffect(e);
				}
				for (int aDust = 0; aDust < 10; aDust++) {
					Effect* e = gBoard->mFXManager->AllocEffect(0x1a);
					float aBoost = (float)((double)RandSigned() + 2.0);
					float aSpread = std::fabs((float)RandSigned());
					e->mType = 0;
					float aPush = (float)(((double)aSpread * 0.05000000074505806 + 0.25) * (double)aBoost);
					double aFlight = (double)aBoost * 0.10000000149011612;
					if (1.0 < aFlight)
						aFlight = 1.0;
					e->mAlpha = (float)aFlight;
					e->mDAlpha = -0.01f;
					e->mDScale = 0.05f;
					e->mScale = 1.5f;
					e->mMaxScale = 1.5f;
					e->mUnknown9C = 0.25f;
					e->mUnknownA0 = -0.01f;
					e->mUnknownA4 = 0.95f;
					aPush = (float)((double)aPush * 15.0);
					float anAngle = (aDust & 1) == 0 ? -0.31415927f : 3.4557521f;
					float aCos = (float)std::cos((double)anAngle);
					e->mDX = (float)((double)aCos * (double)aPush);
					float aSin = (float)std::sin((double)anAngle);
					e->mDY = (float)((double)aSin * (double)aPush);
					e->mX = (float)((double)(mWidth / 2 + mX) + (double)e->mDX * 10.0);
					e->mY = (float)((double)std::fabs((float)RandSigned()) * 10.0 + (double)aShardBase);
					gBoard->mFXManager->AddEffect(e);
				}
			}
		}
	}

	// The fill's glow and the bonus challenge's bar
	int aBonusCount = (int)aBoard->mBonusColors.size();
	int aPhase = mBoard->mUnknown265C;
	float aShown;
	bool aShownSet = true;
	if (aPhase == 2 || aPhase == 5) {
		aShown = (float)((double)mUnknown330 - 0.02500000037252903);
	} else if (aPhase == 4 && mBoard->mUnknown267C != nullptr && 0x64 < mBoard->mUnknown267C->mUpdateCnt) {
		aShown = (float)((double)mUnknown330 - 0.009999999776482582);
	} else {
		aShownSet = false;
		aShown = 0;
		if (aBoard->mGameMode == Board::MODE_CHALLENGE) {
			if (mUnknown330 < 1.0f) {
				float aValue = (float)((double)mUnknown330 + 0.029999999329447746);
				mUnknown330 = aValue;
				if (1.0f < aValue)
					mUnknown330 = 1.0f;
			}
			if (mUnknown580.GetOutVal() == 0.0 && mUnknown580.mIncRate == 0.0) {
				if (aBoard->mUnknown2638 < aBoard->mUnknown2634)
					mUnknown580.SetCurve(GTS(0x7d8630));
			} else {
				mUnknown580.IncInVal();
			}
			mUnknown700 = (float)mUnknown580.GetOutVal();
		} else if (aBonusCount != 0 && mUnknown3B0 == 0) {
			if (!mUnknown3F4) {
				mUnknown3F4 = true;
				mUnknown3D0.resize(mUnknown3D0.size() <= (unsigned int)aBonusCount ? (unsigned int)aBonusCount
																					: mUnknown3D0.size());
				for (unsigned int i = 0; i < (unsigned int)aBonusCount; i++)
					mUnknown3D0[i] = 1.0f;
			}
			float aValue = (float)((double)mUnknown330 + 0.029999999329447746);
			mUnknown330 = aValue;
			if (1.0f < aValue)
				mUnknown330 = 1.0f;
			if (mUnknown3AC == aBoard->mUnknown2660) {
				if (mUnknown3A8 == mUnknown3AC) {
					float aSpin = (float)(6.2831854820251465 * 0.003000000026077032 + (double)mUnknown3CC);
					mUnknown3CC = aSpin;
					if (6.2831854820251465 <= (double)aSpin)
						mUnknown3CC = (float)((double)aSpin - 6.2831854820251465);
				} else if (!mUnknown338.IncInVal()) {
					mUnknown3A8 = mUnknown3AC;
					mUnknown338.SetInVal(0.0);
					mUnknown338.SetCurve(GTS(0x7e3e80));  // 0x704d9c
				}
			} else {
				mUnknown3CC = 0;
				mUnknown3AC = aBoard->mUnknown2660;
				mUnknown338.SetInVal(0.0);
			}
			for (unsigned int i = 0; i < (unsigned int)aBonusCount; i++) {
				float aValue2;
				if (i < (unsigned int)mUnknown3A8) {
					aValue2 = 0.5f;
					if (0.5 <= (double)mUnknown3D0[i] - 0.009999999776482582)
						aValue2 = (float)((double)mUnknown3D0[i] - 0.009999999776482582);
				} else {
					aValue2 = 1.0f;
					if ((double)mUnknown3D0[i] + 0.019999999552965164 <= 1.0)
						aValue2 = (float)((double)mUnknown3D0[i] + 0.019999999552965164);
				}
				mUnknown3D0[i] = aValue2;
			}
		}
	}
	if (aShownSet) {
		if (aShown < 0.0f)
			aShown = 0.0f;
		mUnknown330 = aShown;
	}
	if (mUnknown334 != 0.0f) {
		float aValue = (float)((double)mUnknown334 - 0.019999999552965164);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknown334 = aValue;
	}
	if (0.0f < mUnknown3F8) {
		int aWas = Ftol((double)mUnknown3F8);
		float aLeft = (float)((double)mUnknown3F8 - 0.10000000149011612);
		mUnknown3F8 = aLeft;
		int aNow = Ftol((double)aLeft);
		if (-1 < aNow && aNow != aWas) {
			int aSteps = mUnknown274;
			int aTravellers = (int)mBoard->mUnknown2170.size();
			int aPoint[2];
			GetStepPoint(aPoint, aSteps + aTravellers, true);
			Color aColor(255, 255, 0);
			float aFromX = (float)(((double)RandSigned() * 0.20000000298023224 + 0.5) * (double)mWidth + (double)mX);
			float aFromY = (float)((double)mHeight * 0.8199999928474426 + (double)mY);
			SmokeTraveller* aSmoke = new SmokeTraveller(aColor, Ftol((double)aFromX), Ftol((double)aFromY),
				mX + aPoint[0], mY + aPoint[1], 0, true);
			aSmoke->mHurry = 0.4f;
			aSmoke->mBurst = true;
			mBoard->mUnknown2170.push_back(aSmoke);  // 0x6b9ccc
		}
	}
	if (mUnknown5F0.GetOutVal() > 0.0 && mBoard->mUnknown268C == 1.0f && mBoard->mUnknown267C == nullptr) {
		// port: 3 sparks fly from the bottom's middle (0x7e3390: a table of 512 radii, game data); Rand is drawn the
		// same
		for (int aSpark = 3; aSpark != 0; aSpark--) {
			Rand();
			RandSigned();
		}
		NotPorted("GoodTube::UpdateTick: the sparks of the bonus challenge's bracket (0x7e3390)");
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
	if (mUnknown16C == 0)
		return;
	// (Direct3D: the board's jump curves pick the shader that fades the tube, and set its parameters)
	TwistWidget::DrawAll(theFlags, g);

	int anArtScale = Ftol((double)gApp->mScale * 10.0);
	int aTimeX = Ftol((double)mWidth * 0.5 + (double)anArtScale);
	int aTimeOffsetY = Ftol((double)gApp->mScale * -20.0);
	int aTimeY = Ftol((double)mUnknown194) - aTimeOffsetY;
	aTimeY++;  // (the original adds 1 unless Direct3D draws)
	int aCenterX, aCenterY;
	mBoard->GetCenter(&aCenterX, &aCenterY);
	int aToX = aCenterX - mX;
	int aToY = aCenterY - mY;
	int aX = Ftol((double)(aToX - aTimeX) * mUnknown400.GetOutVal() + (double)aTimeX);
	int anY = Ftol((double)(aToY - aTimeY) * mUnknown400.GetOutVal() + (double)aTimeY);
	double aGrowth = mUnknown400.GetOutVal();
	float aScale = (float)(aGrowth + aGrowth + 0.15000000596046448);
	int aTenths = mUnknown4B8 / 100;
	if (0.0f < mUnknown4A4) {
		g->SetColor(Color(0, 0, 0, Ftol((double)mUnknown4A4 * 100.0)));
		g->FillRect(-mX, -mY, gApp->mWidth, gApp->mHeight);
		g->SetColor(Color::White);
	}
	if (mUnknown4AC == 0)
		return;
	if (!(0.0f < mUnknown4C0))
		return;
	Color aColorA;
	Color aColorB;
	if (aTenths < 10 && mUnknown4A8) {
		float aWave = (float)std::sin((double)(float)((double)(mUpdateCnt % 75) * 3.1415927410125732 / 75.0));
		aColorA = Color(Ftol((double)aWave * 255.0), 0x20, 0x20);
		aColorB = Color(Ftol((1.0 - (double)aWave) * 255.0), 0x20, 0x20);
	} else if (mUnknown4AC < 4) {
		ColorCycle aCycle;
		aCycle.SetPosition((float)((double)(mUpdateCnt % 100) / 100.0));
		aColorA = aCycle.mColor;
		aColorB = Color::Black;
	} else {
		aColorA = Color::Black;
		aColorB = Color::White;
	}
	aColorA.mAlpha = Ftol((double)mUnknown4C0 * 255.0);
	DrawTime(g, mUnknown4B8, aX, anY, aColorA, aColorB, aScale, mUnknown4A9);
}

void GoodTube::DrawTime(Graphics* g, int theTime, int theX, int theY, const Color& theColor, const Color& theColorTop,
	float theScale, bool theHundredths)
{
	// 0x704dde: blitz's clock in FONT_COUNTER, a character at a time, mUnknown4C0 opaque
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
	g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown4C0 * 255.0)));
	// (Direct3D: the matrix of the scale about the clock goes to the device)
	g->PushState();
	float aClamped = theScale;
	if (aClamped <= 0.2f)
		aClamped = 0.2f;
	if (1.5f <= aClamped)
		aClamped = 1.5f;
	g->SetScale(aClamped, aClamped, (float)theX, (float)theY);
	if (aClamped < 0.5f)
		g->SetFastStretch(false);
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
}

void GoodTube::DrawGlow(Graphics* g)
{
	// 0x704637
	float anArtScale = (float)((double)gApp->mArtRes / 1200.0);
	int aX = GetBottomX();
	int anY = GetBottomTop();
	g->SetColorizeImages(true);
	Color aGlowColor(255, 255, 255, mUnknown1DC);
	g->SetDrawMode(0);
	if (mUnknown300 < 1) {
		g->SetColor(Color(255, 255, 255));
		g->DrawImage(mBottomImage, aX, anY);
		g->SetFont(FONT_SCORE);
		if ((mUnknown3B0 == 0 || mUnknown3B0 == 4) && 0.0f < mUnknown330)
			DrawBonus(g);
		g->SetColor(Color::White);
		if (mUnknown330 < 1.0f) {
			int aDoorX = Ftol((double)mWidth * 0.5199999809265137);
			int aDoorY = GetBottomTop();
			aDoorY = Ftol((double)mBottomImage->mHeight * 0.36500000953674316 + (double)aDoorY);
			// (Direct3D: the bonus door is drawn with a shader)
			int aFrame = Ftol((double)mUnknown330 * 10.0);
			Image* aDoor = IMAGE_INGAMEUI_TWISTDOORANIM;
			if (aFrame < 10) {
				int aDoorTop = aDoorY - aDoor->GetCelHeight() / 2;
				g->DrawImageCel(aDoor, aDoorX - aDoor->GetCelWidth() / 2, aDoorTop, aFrame);
				Image* aDoorGlow = IMAGE_INGAMEUI_TWISTDOORGLOW;
				if (mUnknown334 != 0.0f) {
					g->SetDrawMode(1);
					g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown334 * 255.0)));
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
	g->SetColor(Color(255, 255, 255, Ftol((1.0 - (double)mUnknown2AC) * (double)mUnknown1DC)));
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
		if (mBoard->mUnknown265C < 3) {
			Image* aHeader = IMAGE_INGAMEUI_BONUS_HEADER;
			g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown3F0 * 255.0)));
			int aHeaderY = Ftol((double)mBottomImage->mHeight * 0.20499999821186066 + (double)GetBottomTop());
			g->DrawImage(aHeader, aCenterX - aHeader->mWidth / 2, aHeaderY);
			aHeaderBottom = Ftol(0.0);
		} else {
			g->PushState();
			g->SetColor(Color(255, 255, 255, Ftol(mUnknown5F0.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
			g->ClipRect(Ftol((double)gApp->mScale * 78.0), Ftol((double)gApp->mScale * 902.0),
				Ftol((double)gApp->mScale * 500.0), Ftol((double)gApp->mScale * 104.0));
			Transform aTransform;
			aTransform.RotateRad((float)((double)mUpdateCnt * -0.009999999776482582));
			if (mBoard->mUnknown268C == 1.0f)
				g->DrawImageTransform(IMAGE_MEGAFRUIT_RAINBOW, aTransform, (float)aCenterX, (float)aBaseY);
			g->PopState();
			aHeaderBottom = Ftol((float)((double)gApp->mScale + (double)gApp->mScale));
		}
		if (0.0f < mUnknown3F0 && aCount != 0) {
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
			bool aAnimating = mUnknown3B0 == 1;
			if (aAnimating) {
				// (the exe works out a sine of the curve's output here and drops it)
				mUnknown338.GetOutVal();
				mUnknown338.GetOutVal();
			}
			unsigned int i = 0;
			if (aCount != 0) {
				int aRowY = aHeaderBottom + aBaseY;
				float aRowYF = (float)aRowY;
				int aX = (int)aCellWidth / 2 + aStartX;
				do {
					int aColor = gBoard->mBonusColors[i];
					int aLight = aColor * 2;
					if (i < (unsigned int)gBoard->mUnknown2660)
						aLight++;
					Image* aLightImage = GetImageById(sBonusLightIds[aLight]);
					if (aAnimating)
						(void)mUnknown3D0[i];
					Color aLightColor(255, 255, 255, Ftol((double)mUnknown3D0[i] * 255.0));
					if (aAnimating) {
						int aAlpha = 0x80;
						if (aAlpha <= aLightColor.mAlpha)
							aAlpha = aLightColor.mAlpha;
						aLightColor.mAlpha = Ftol((double)aAlpha * (double)mUnknown3F0);
					}
					g->SetColor(aLightColor);
					DrawImageScaledCentered(g, aLightImage, (float)aX, aRowYF, 3, 1.0f, 1.0f);
					if (i == (unsigned int)gBoard->mUnknown2660) {
						// the light the chain has reached glows
						g->SetDrawMode(1);
						Color aSavedColor = g->GetColor();
						float aWave = (float)std::sin((double)mUnknown3CC);
						double aGlow;
						if (0.0 <= (double)aWave) {
							aGlow = (double)(float)std::sin((double)mUnknown3CC);
						} else {
							aGlow = 0.0;
						}
						float aGlowF = (float)aGlow;
						g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown3F0 * 127.5 * (double)aGlowF)));
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
						double aSpread = ((double)mUnknown3E0[i] * (1.0 - (double)mUnknown3C4)) * 4.0;
						if (!(aSpread < 1.0))
							aSpread = 1.0;
						g->SetColor(Color(255, 255, 255, Ftol(aSpread * 255.0)));
						float aGrow = (float)(((double)mUnknown3E0[i] * (double)aRingScale) * 0.800000011920929 + 1.0);
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
			if (mUnknown3B0 == 0 && mUnknown3AC < 4) {
				bool aMoving = mUnknown3A8 != mUnknown3AC;
				int aMarkX = (int)aCellWidth / 2 + mUnknown3A8 * (int)aCellWidth + aStartX;
				if (aMoving) {
					aMarkX = Ftol(mUnknown338.GetOutVal() * (double)(mUnknown3AC - mUnknown3A8) * (double)(int)aCellWidth
						+ (double)aMarkX);
				}
				g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown3F0 * 255.0)));
				DrawImageScaledCentered(g, aBar, (float)aMarkX, (float)aBarY, 3, 1.0f, 1.0f);
			}
		}
		if (mUnknown3B0 != 2) {
			if (mUnknown3B0 == 3) {
				// the bonus won: "+N" in green rising out of the score font, then its glow
				double aAlpha = (double)mUnknown3B4 * 255.0;
				if (255.0 < aAlpha)
					aAlpha = 255.0;
				Color aTextColor(0x20, 0xff, 0x20, Ftol(aAlpha));
				g->SetColor(aTextColor);
				SexyString aText = StrFormat(GTC(0x7e3ce8), mUnknown3BC);
				g->SetFont(FONT_SCORE);
				int aTextY = Ftol((double)gApp->mScale * 0.0) + aBaseY;
				double aHalf = (double)mWidth * 0.5;
				int aTextX = Ftol(aHalf - (double)(g->StringWidth(aText) / 2));
				g->DrawString(aText, aTextX, aTextY);
				float aFade = (float)((double)mUnknown3B4 - 1.0);
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
			if (mUnknown3B0 != 4)
				return;
		}
		// the challenge done (2) or its time out (4)
		Image* aDone = IMAGE_INGAMEUI_CHALLENGECOMPLETE;
		g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown3C4 * 255.0)));
		int aDoneY = aBaseY - Ftol((double)gApp->mScale * 10.0);
		DrawImageScaledCentered(g, aDone, (float)aCenterX, (float)aDoneY, 3, 1.0f, 1.0f);
	}
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
	float aFillTop = mUnknown194;
	if (gBoard->mGameMode == 3)
		aFillTop = (float)Ftol((double)aFillTop);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255));
	bool aLit;
	if (mUnknown300 < 1 || (double)mUnknown2C8 + (double)aScale * 370.0 < (double)aFillTop)
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
			int aFillY = Ftol((double)aScale * 25.0 + (double)mUnknown194);
			// 0x85d6bc and 0x85d6cc: the fill's colour at the start and at the end of +0x294
			static const Color sFillStart(255, 255, 255, 255);
			static const Color sFillEnd(0x80, 0x10, 0x10, 255);
			Color aFill;
			if (gBoard->mGameMode == 3) {
				aFill = mColorCycle.mColor;
			} else {
				double aValue = (double)(sFillEnd.mRed - sFillStart.mRed) * (double)mUnknown294 + (double)sFillStart.mRed;
				if (255.0 < aValue)
					aValue = 255.0;
				aFill.mRed = Ftol(aValue);
				aValue = (double)(sFillEnd.mGreen - sFillStart.mGreen) * (double)mUnknown294 + (double)sFillStart.mGreen;
				if (255.0 < aValue)
					aValue = 255.0;
				aFill.mGreen = Ftol(aValue);
				aValue = (double)(sFillEnd.mBlue - sFillStart.mBlue) * (double)mUnknown294 + (double)sFillStart.mBlue;
				if (255.0 < aValue)
					aValue = 255.0;
				aFill.mBlue = Ftol(aValue);
				aValue = (double)(sFillEnd.mAlpha - sFillStart.mAlpha) * (double)mUnknown294 + (double)sFillStart.mAlpha;
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
			// (Direct3D: the first texture's waving mesh)
			g->SetColor(Color(255, 255, 255, 0x80));
			// (Direct3D: the second texture's quad, moving by mUpdateCnt % 500)
			g->SetDrawMode(0);
			g->SetDrawMode(1);
			g->SetColor(Color(255, 255, 255));
			g->SetColorizeImages(true);
			g->SetDrawMode(0);
			int aGlassTop = GetGlassTop();
			int aGlassHeight = mUnknown16C;
			g->SetColor(Color(255, 255, 255, Ftol((1.0 - (double)mUnknown2B8) * 255.0)));
			Rect aGlassSrc(0, 0, mGlassImage->mWidth, aGlassHeight);
			Rect aGlassDest(aTubeX, aGlassTop, mGlassImage->mWidth, aGlassHeight);
			g->DrawImage(mGlassImage, aGlassDest, aGlassSrc);
			g->SetDrawMode(1);
			g->SetColor(aFill);
			unsigned int i = 0;
			if (mSpecks.size() != 0) {
				do {
					Speck* aSpeck = mSpecks[i];
					aFill.mAlpha = Ftol((double)aSpeck->mAlpha * 255.0 * (1.0 - (double)mUnknown294));
					g->SetColor(aFill);
					if (!g->Is3D()) {
						g->DrawImage(aSpeck->mImage, Ftol(aSpeck->mX), Ftol(aSpeck->mY));
					} else {
						// (Direct3D: the speck is drawn stretched by its distance from the tube's middle)
					}
					i++;
				} while (i < mSpecks.size());
			}
		}
	} else if (mUnknown300 < 1 || (double)mUnknown2C8 + (double)aScale * 370.0 < (double)aFillTop) {
		g->SetDrawMode(0);
		g->SetColor(Color::White);
		Image* aBottomGlow = IMAGE_INGAMEUI_TUBE_BOTTOM_INNERGLOW;
		int aBottomGlowY = Ftol((double)gApp->mScale * 24.0) + GetBottomTop();
		DrawImageScaledCentered(g, aBottomGlow, (float)aInnerGlowX, (float)aBottomGlowY, 3, 1.0f, 1.0f);
		Image* aRing = IMAGE_INGAMEUI_RING_FRONT;
		float aRingStep = (float)(-(double)aScale * 12.0);
		double aRingY = (double)aRingStep + (double)aFillTop;
		// (Direct3D: the ring is placed by DrawImageF at the float position)
		g->DrawImage(aRing, Ftol((double)aRingX), Ftol(aRingY));
		Image* aLevel = IMAGE_INGAMEUI_LEVEL;
		if ((mUnknown4AC != 0 || 0 < mLevel) && (gBoard->mGameMode != 2 || mUnknown4AC != 0)) {
			float aRingMidX = (float)((double)aRing->mWidth * 0.5 + (double)aRingX);
			double aRingDrop = (double)aRing->mHeight * 0.2800000011920929;
			float aLevelY = (float)(aRingDrop + (double)Ftol((double)aFillTop));
			g->DrawImage(aLevel, Ftol((double)aRingMidX) - aLevel->mWidth / 2, Ftol((double)aLevelY) - aLevel->mHeight / 2);
			g->SetFont(FONT_TUBE_TEXT);
			float aTimerY = (float)((double)aRing->mHeight * 0.800000011920929 + (double)Ftol(aRingY));
			Image* aTimer = IMAGE_INGAMEUI_RING_FRONT_TIMER;
			if (mUnknown4AC == 0) {
				// (Direct3D: a transform of the fraction of the y goes to the device round the number)
				if (gApp->mArtRes == 600)
					aTimerY = (float)((double)aTimerY + 1.0);
				SexyString aLevelText = StrFormat(GTC(0x80da2c), mLevel);
				int aTextWidth = g->StringWidth(aLevelText);
				if (mLevel < 10)
					aTextWidth -= 2;
				g->DrawString(aLevelText, Ftol((double)aRingMidX - (double)(aTextWidth / 2)), Ftol((double)aTimerY));
			} else {
				int aTimerStep = Ftol((double)gApp->mScale * -25.0);
				int aTimerTop = (-(aTimer->mHeight / 2) - aTimerStep) + Ftol(aRingY);
				g->DrawImage(aTimer, Ftol((double)aRingMidX) - aTimer->mWidth / 2, aTimerTop);
			}
		}
	}
	g->SetDrawMode(1);
	if (mBoard->mGameMode != 3 && 0 < mLevel) {
		g->SetColor(Color(255, 255, 255, mUnknown1DC));
		g->DrawImageF(IMAGE_INGAMEUI_RING_OUTERGLOW, aRingX, (float)((double)aFillTop - (double)aScale * 6.0));
	}
	if (mUnknown1CC != nullptr)
		NotPorted("GoodTube::DrawFill: the text popup at +0x1cc (0x707103)");
	if (mUnknown1D0 != nullptr)
		NotPorted("GoodTube::DrawFill: the text popup at +0x1d0 (0x707215)");
}

void GoodTube::DrawLight(Graphics* g, int theLight, float theAlpha)
{
	// 0x70675e (Direct3D: the light is drawn as strips of its arc, up to theAlpha of the way along it)
	int aLastLight = Board::GetLevelSteps(mUnknown27C) - 1;
	if (aLastLight < theLight)
		theLight = aLastLight;
	int anId = sFirstLight[mUnknown27C] + theLight;
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
	bool aIsChallengeTube = mUnknown158 == 4;
	float anArtScale = (float)((double)gApp->mArtRes / 1200.0);
	int aTopX = GetTopX();
	int aTopY = GetTopY();
	g->SetDrawMode(0);
	g->SetColor(Color::White);
	g->DrawImageRotated(mTopImage, aTopX, aTopY, (double)mUnknown2C0, nullptr);
	g->SetColorizeImages(true);
	bool aIsChallengeMode = gBoard->mGameMode == 2;
	if (0 < mUnknown300)
		return;
	int aLevelIndex = mUnknown27C - 1;
	if (mUnknown14D && !aIsChallengeMode) {
		// the slots of the lights, then the lights lit so far
		int aLitLights = Ftol((double)(float)std::floor((double)mUnknown278));
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
		int aFirstLight = sFirstLight[mUnknown27C];
		int aLightCount = Board::GetLevelSteps(mUnknown27C);
		if (aLightCount < aLitLights)
			aLitLights = aLightCount;
		int i = 0;
		if (0 < aLitLights) {
			float aCountF = (float)aLightCount;
			int aHueBase = aLightCount * 15;
			float* aFlash = mUnknown1E4;
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
				double aBright = 1.0 - ((double)mUnknown278 - (double)aCountF) * 0.25;
				if ((0.7799999713897705 <= aBright && 1.0 < aBright) || aBright < 0.7799999713897705) {
					if (aBright < 0.7799999713897705)
						aBright = 0.7799999713897705;
					else
						aBright = 1.0;
				}
				float aBrightF = (float)aBright;
				int aGrey = Ftol((double)aBrightF * 255.0);
				g->SetColor(Color(aGrey, aGrey, aGrey, 255));
				float aReached = (float)((double)mUnknown278 - (double)(float)i);
				if (aReached < 0.99) {
					DrawLight(g, i, aReached);
				} else {
					float aBeyond = (float)(((double)mUnknown278 - (double)aCountF) - (double)(float)i);
					if (0.0 < aBeyond) {
						// a light beyond the level's last: the hue goes round with the time
						aLightImage = GetImageById(i + 0xdb);
						float aSpeed = gBoard->mUnknown214 ? 10.0f : 1.5f;
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
		if (0.0f < mUnknown29C) {
			GetTopY();
			Image* anArcGlow = IMAGE_INGAMEUI_ARC_GLOW;
			int anArcGlowId = GetIdByImage(anArcGlow);
			g->SetDrawMode(1);
			g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown29C * 255.0)));
			int anArcGlowY = Ftol((double)GetImageOffsetY(anArcGlowId) * (double)anArtScale);
			int anArcGlowX = Ftol((double)GetImageOffsetX(anArcGlowId) * (double)anArtScale);
			g->DrawImage(anArcGlow, anArcGlowX, anArcGlowY);
			g->SetDrawMode(0);
		}
	}
	float aTipFade = (float)(1.0 - (double)mUnknown244);
	if (gBoard->mUnknown2630 != 0) {
		// the challenge's display: its count and its best, its goals and the moves left (+0x2630's flags at +0x80)
		NotPorted("GoodTube::DrawBody: the challenge's display (the board's +0x2630 object, 0x6db0ec)");
	} else if (mUnknown158 == 0) {
		// the score under the top
		SexyString aScoreText = CommaSeperate((int)mBoard->mUnknown14F8);
		Font* aGlowFont = FONT_SCOREGLOW;
		Font* aScoreFont = FONT_SCORE;
		int aScoreWidth = aScoreFont->StringWidth(aScoreText);
		if (1999999 < (unsigned int)mBoard->mUnknown14F8) {
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
		if (!mUnknown14D)
			aScoreY = Ftol((double)aScoreY - (double)anArtScale * 0.0);
		g->SetColor(Color(255, 255, 255, Ftol((double)aTipFade * 255.0)));
		g->DrawString(aScoreText, aScoreX, aScoreY);
		if (0.0f < mBoard->mUnknown20F0) {
			g->SetColorizeImages(true);
			double aGlowAlpha = (double)aTipFade * (double)mBoard->mUnknown20F0 * 255.0;
			if (255.0 < aGlowAlpha)
				aGlowAlpha = 255.0;
			g->SetColor(Color(255, 255, 255, Ftol(aGlowAlpha)));
			g->SetFont(aGlowFont);
			SetFontLayerColor(g->GetFont(), 0, Color::White);
			g->DrawString(aScoreText, aScoreX, aScoreY);
		}
	}
	if (mUnknown158 != 4 && !mTip.empty() && 0.0f < mUnknown244) {
		Color aTipColor(mUnknown24C[0], mUnknown24C[1], mUnknown24C[2], mUnknown24C[3]);
		aTipColor.mAlpha = Ftol((double)mUnknown244 * 255.0);
		g->SetFont(FONT_TOOLTIP);
		int aTipHeight = g->GetWordWrappedHeight(mUnknown240, mTip, -1, nullptr);
		int aTipMid = mTopImage->mHeight / 2 + aTopY;
		int aTipY = Ftol(((double)anArtScale * 5.0 + (double)aTipMid) - (double)aTipHeight * 0.375);
		if (gBoard->mGameMode == 2)
			aTipY += Ftol((double)gApp->mScale * -40.0);
		g->SetColor(aTipColor);
		int aTipX = mWidth / 2 - mUnknown240 / 2;
		Rect aTipRect(Ftol((double)aTipX + (double)anArtScale * 8.0), aTipY, mUnknown240, aTipHeight);
		g->WriteWordWrapped(aTipRect, mTip, -1, 0, nullptr, -1, nullptr);
		g->SetColor(Color::White);
	}
	if (!aIsChallengeMode && mUnknown14D) {
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
		int aCel = mUnknown280 - 1;
		if (9 < aCel)
			aCel = 9;
		g->DrawImageCel(aBulbs, aBulbX, aBulbY, aCel);
		g->SetDrawMode(1);
		if (0.0f < gBoard->mUnknown20B8) {
			float aGrow = (float)((1.0 - (double)gBoard->mUnknown20B8) * 3.0 + 1.0);
			g->SetColor(Color(255, 255, 255, Ftol((double)gBoard->mUnknown20B8 * 255.0)));
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
	if (aIsChallengeTube && mUnknown2C0 == 0.0f) {
		// the doors of the challenge's tube
		g->DrawImage(IMAGE_DOORS_CHALLENGE_TOP, aTopX - Ftol((double)gApp->mScale * -8.0),
			aTopY - Ftol((double)gApp->mScale * -45.0));
		g->DrawImage(IMAGE_DOORS_CHALLENGE_BOTTOM, aTopX - Ftol((double)gApp->mScale * -8.0),
			aTopY - Ftol((double)gApp->mScale * -45.0));
	}
	g->SetDrawMode(1);
	Image* aTopGlow = IMAGE_INGAMEUI_TOP_LIGHTGLOW;
	g->SetColor(Color(255, 255, 255, mUnknown1DC));
	int aTopGlowY = Ftol((double)GetImageOffsetY(GetIdByImage(aTopGlow)) * (double)anArtScale);
	int aTopGlowX = Ftol((double)GetImageOffsetX(GetIdByImage(aTopGlow)) * (double)anArtScale);
	g->DrawImage(aTopGlow, aTopGlowX, aTopGlowY);
	g->SetColor(Color(255, 255, 255));
	g->SetDrawMode(0);
}
