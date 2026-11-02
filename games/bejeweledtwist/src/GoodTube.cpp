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
	if (theValue == 3)
		mUnknown580.SetConstant(1.0);  // the best's fade (+0x580) full at once
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

// 0x70c94a: the tube's tip text, in theColor, wrapped to theMaxWidth in FONT_TOOLTIP at its spaces; it fades in and
// stays for theSpeed (or 1) of the 0.01 steps of +0x248 (see Update)
void GoodTube::SetTip(const SexyString& theText, const Color& theColor, float theSpeed, int theMaxWidth)
{
	mUnknown24C[0] = theColor.mRed;
	mUnknown24C[1] = theColor.mGreen;
	mUnknown24C[2] = theColor.mBlue;
	mUnknown24C[3] = theColor.mAlpha;
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
	mUnknown244 = 1.0f;
	mUnknown248 = 0.0f < theSpeed ? theSpeed : 1.0f;
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
		// 0x6a6983: the menu comes up over the board
		aBoard->ShowMenu();
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
	// 0x7049e5: with Direct3D, while the board's scale is not 1, the board's matrix is pushed here and popped at the end
	if (mBoard->mUnknown268C != 1.0f && gD3DInterface != nullptr)
		gD3DInterface->PushTransform(mBoard->mTransform, true);  // slot 0x14
	if (mUnknown3B0 != 0 && mUnknown3B0 != 4)
		DrawBonus(g);
	mFXManager->Draw(g);
	if (mUnknown5F0.GetOutVal() != 0.0 && !gBoard->mUnknown37C4) {
		float anAlpha = (float)mUnknown5F0.GetOutVal();
		if (mBoard->mHyperspace != nullptr && gD3DInterface != nullptr) {
			// With Direct3D, during the jump's transitions 8 and 12 the alpha is multiplied by that transition's curve
			if (mBoard->mHyperspace->mTransition == 8)
				anAlpha = (float)(mBoard->mHyperspace->mUnknown3A8.GetOutVal() * (double)anAlpha);
			else if (mBoard->mHyperspace->mTransition == 12)
				anAlpha = (float)(mBoard->mHyperspace->mUnknown12B0.GetOutVal() * (double)anAlpha);
		}
		g->SetColorizeImages(true);
		g->SetDrawMode(0);
		g->SetColor(Color(255, 255, 255, Ftol((double)anAlpha * 255.0)));
		g->DrawImage(IMAGE_MEGAFRUIT_BRACKET, Ftol((double)gApp->mScale * 72.0),
			Ftol((double)gApp->mScale * 842.0));
	}
	if (mBoard->mUnknown268C != 1.0f && gD3DInterface != nullptr)
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
		if (mBoard->mHyperspace != nullptr && gD3DInterface != nullptr &&
			mBoard->mHyperspace->mUnknown3A8.GetOutVal() < 0.5) {
			// With Direct3D the menu button's mouse is off while the jump's curve is under 0.5
			mMenuButton->mMouseVisible = false;
		}
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
						// 0x74b94e: a FruitShockwave at the hint's gem, with the tube's middle for its target, then 0x6bc3cd adds it
						// to the board's waves
						aBoard->mUnknown1660.push_back(new FruitShockwave(aHint, 1, Ftol((double)mWidth * 0.5 + (double)mX),
							Ftol((double)mHeight * 0.8500000238418579 + (double)mY), 200));
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

	if (mChallenge && aBoard->mChallenge != 0 && (aBoard->mChallenge->mFlags & Challenge::FLAG_PATTERN)) {
		// 0x70ae48: the pattern's count shown walks to the matches so far, the curve restarting at each step down
		int aProgress = aBoard->mChallenge->mProgress;
		if (mUnknown538 != aProgress) {
			if (0.0 <= mUnknown4C8.mIncRate || mUnknown4C8.HasBeenTriggered()) {
				if (mUnknown538 > aProgress)
					mUnknown538--;
				mUnknown4C8.SetCurve("b+1,1.5,0.033333,1,####    n~###     3####");
			}
		}
		if (!mUnknown4C8.HasBeenTriggered() && !mUnknown4C8.IncInVal() && mUnknown538 < aProgress)
			mUnknown538++;
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
	// port: the two text popups at +0x1cc and +0x1d0 are updated here (a popup that is done goes); nothing in the exe
	// makes them

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
		// 3 sparks a frame round the bottom's middle, each at one of 512 angles and that angle's radius (the table at
		// 0x7e3390, 512 floats of game data), flying outwards
		float aMidX = (float)Ftol((double)mWidth * 0.5249999761581421);
		float aMidY = (float)Ftol((double)mBottomImage->mHeight * 0.36000001430511475 + (double)GetBottomTop());
		int aSpark = 3;
		do {
			Effect* e = mFXManager->AllocEffect(0);
			int aStep = Rand() % 512;
			float anAngle = (float)((double)aStep * 0.001953125 * (double)6.2831855f);
			int aRadiusBits = GameInt(GTB(0x7e3390, 2048) + aStep * 4);
			float aRadiusF;
			std::memcpy(&aRadiusF, &aRadiusBits, sizeof(aRadiusF));
			float aRadius = aRadiusF * gApp->mScale;
			e->mImage = IMAGE_GEM_FRUIT_SPARK;
			e->mIsAdditive = true;
			e->mX = (float)((double)(float)std::cos(anAngle) * (double)aRadius + (double)aMidX);
			e->mY = (float)((double)(float)std::sin(anAngle) * (double)aRadius + (double)aMidY);
			float aCos = (float)std::cos(anAngle);
			e->mDX = (float)((double)gApp->mScale * 1.100000023841858) * aCos;
			float aSin = (float)std::sin(anAngle);
			e->mDY = (float)((double)gApp->mScale * 1.100000023841858) * aSin;
			e->mAlpha = (float)mUnknown5F0.GetOutVal();
			e->mScale = 0.5f;
			e->mGravity = 0;
			e->mDScale = -0.01f;
			e->mDAlpha = -0.01f;
			e->mDAngle = (float)((double)RandSigned() * 0.019999999552965164 + 0.05000000074505806);
			mFXManager->AddEffect(e);
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
	if (mUnknown16C == 0)
		return;
	// With Direct3D the board's jump curves pick the shader that fades the tube (the whiteout, or its fixed-function
	// class on a card without pixel shaders, local_11 in the original) and set its parameters
	bool aFixedFunction = true;
	if (gD3DInterface != nullptr)
		aFixedFunction = (gD3DInterface->GetCaps(0) & 2) == 0;
	Hyperspace* aSpace = mBoard->mHyperspace;
	if (aSpace != nullptr && gD3DInterface != nullptr) {
		if (0.0 < aSpace->mUnknown2C8.GetOutVal() && !aSpace->mIs3D)
			aSpace->Draw3DSidebar();  // 0x711529
		if (aSpace->mUnknown338.GetOutVal() != 0.0 || aSpace->mUnknown2C8.GetOutVal() != 0.0) {
			if (aSpace->mUnknown3A8.GetOutVal() == 0.0) {
				gD3DInterface->SetPixelShader(nullptr);  // slot 0xd8: the tube is hidden altogether
				return;
			}
			if (aFixedFunction) {
				// 0x501630: WhiteoutFixedFunctionShader's parameters (0x85d6ec, 0x85d6f0) and its ladder
				NotPorted("GoodTube::DrawAll: WhiteoutFixedFunctionShader 0x501630 (a card without ps_2_0)");
			} else {
				gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_WHITEOUT_FINAL]);  // slot 0xd8, 0x85e0d8
				float aParams[4];
				aParams[0] = (float)aSpace->mUnknown338.GetOutVal();
				aParams[1] = (float)aSpace->mUnknown3A8.GetOutVal();
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
	int aTimeY = Ftol((double)mUnknown194) - aTimeOffsetY;
	if (gD3DInterface == nullptr)
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
			bool aShaderDoor = false;
			if (gD3DInterface != nullptr) {
				// With Direct3D (and pixel shaders, and no jump) the bonus door is drawn with the button-cover shader
				// over the bottom half's picture
				unsigned long aCaps = gD3DInterface->GetCaps(0);
				Image* aDoorTop = IMAGE_DOORS_BONUS_TOP;
				if ((aCaps & 2) != 0 && gApp->mBoard->mHyperspace == nullptr && 0.0f < mUnknown330) {
					gD3DInterface->SetTexture(1, IMAGE_DOORS_BONUS_BOTTOM);  // slot 0xd4
					gD3DInterface->SetTextureWrap(0, false);                 // slot 0xcc
					gD3DInterface->SetTextureWrap(1, false);
					gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_BUTTONCOVER]);  // slot 0xd8, 0x85dbd8
					float aParams[4];
					aParams[0] = (float)((double)mUnknown330 * 0.8500000238418579);
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
			int aFrame = Ftol((double)mUnknown330 * 10.0);
			Image* aDoor = IMAGE_INGAMEUI_TWISTDOORANIM;
			if (!aShaderDoor && aFrame < 10) {
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
						float aSpeckScale = (float)((double)aSpeck->mScale * (1.0 - (double)mUnknown294) * (double)aSize);
						int aSpeckHeight = Ftol((double)IMAGE_INGAMEUI_TUBE_FILL_PARTICLE->mHeight * (double)aSpeckScale);
						int aSpeckWidth = Ftol((double)IMAGE_INGAMEUI_TUBE_FILL_PARTICLE->mWidth * (double)aSpeckScale);
						g->DrawImage(aSpeck->mImage, Ftol(aSpeck->mX), Ftol(aSpeck->mY), aSpeckWidth, aSpeckHeight);
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
		// with Direct3D the ring, the level and the timer are drawn at float positions
		float aRingYF = (float)aRingY;
		if (gD3DInterface == nullptr)
			g->DrawImage(aRing, Ftol((double)aRingX), Ftol(aRingY));
		else
			g->DrawImageF(aRing, aRingX, aRingYF);
		Image* aLevel = IMAGE_INGAMEUI_LEVEL;
		if ((mUnknown4AC != 0 || 0 < mLevel) && (gBoard->mGameMode != 2 || mUnknown4AC != 0)) {
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
			if (mUnknown4AC == 0) {
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
		g->SetColor(Color(255, 255, 255, mUnknown1DC));
		g->DrawImageF(IMAGE_INGAMEUI_RING_OUTERGLOW, aRingX, (float)((double)aFillTop - (double)aScale * 6.0));
	}
	// (the text popups: 0x707103 and 0x707215, which make them, are never called, so these stay null)
	if (mUnknown1CC != nullptr)
		NotPorted("GoodTube::DrawFill: the text popup at +0x1cc (0x707103)");
	if (mUnknown1D0 != nullptr)
		NotPorted("GoodTube::DrawFill: the text popup at +0x1d0 (0x707215)");
}

void GoodTube::DrawLight(Graphics* g, int theLight, float theAlpha)
{
	// 0x70675e
	int aLastLight = Board::GetLevelSteps(mUnknown27C) - 1;
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
		int aSteps = Board::GetLevelSteps(mUnknown27C);
		int anImageId = sFirstLight[mUnknown27C] + theLight;
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
	if (gBoard->mChallenge != 0) {
		DrawChallenge(g, aTipFade);
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
	bool aShowBestEver = aNightMode && mUnknown768;
	int aScoreY = GetScoreY();
	if (aShowBestEver)
		aScoreY += Ftol((double)gApp->mScale * -55.0);
	int anAlpha = Ftol((double)theTipFade * 255.0);
	g->SetColor(Color(255, 255, 255, anAlpha));
	if (mUnknown158 == 3) {
		// the moves left, word-wrapped in FONT_SCORE
		int aTop = Ftol((double)gApp->mScale * (aPoints ? 50.0 : -80.0)) + aScoreY;
		SexyString aText;
		int aMovesLeft = aChallenge->mMoveGoal - gApp->mUnknownC74[0x21];
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
		GetGoalLists(aChallenge, mUnknown56C, mUnknown53C, mUnknown55C, mUnknown54C);
		if (aNightMode) {
			// the count's title and the count, and the best under them while +0x580 shows it
			int aCount = gBoard->mUnknown2638;
			SexyString aTitle = aChallenge->mCountTitle;
			SexyString aCountText = StrFormat(GTC(0x80da2c), aCount);  // (made and not drawn)
			int aMidX = Ftol((double)mWidth * 0.5249999761581421);
			int aY = aScoreY - Ftol((double)gApp->mScale * -30.0);
			double aBestShown = mUnknown580.GetOutVal();
			int aRise = Ftol((double)gApp->mScale * 0.0);
			aY = Ftol(aBestShown * (double)aRise + (double)aY);
			g->SetFont(FONT_TOOLTIP);
			int aTitleY = aY - Ftol((double)gApp->mScale * 40.0);
			g->DrawString(aTitle, aMidX - g->StringWidth(aTitle) / 2, aTitleY);
			mTicker660.Draw(g, aMidX, aY, true, theTipFade);
			if (mUnknown580.GetOutVal() != 0.0) {
				g->SetColor(Color(255, 255, 255, Ftol(mUnknown580.GetOutVal() * 255.0 * (double)theTipFade)));
				SexyString aBest = GT(0x7e40a8);
				g->SetFont(FONT_TOOLTIP);
				int aBestY = Ftol((double)gApp->mScale * 65.0) + aScoreY;
				g->DrawString(aBest, aMidX - g->StringWidth(aBest) / 2, aBestY);
				float aBestAlpha = (float)(mUnknown580.GetOutVal() * (double)theTipFade);
				mTicker6E4.Draw(g, aMidX, aScoreY - Ftol((double)gApp->mScale * -95.0), true, aBestAlpha);
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
					if (i < mUnknown538)
						anIndex++;
					Image* aGem = GetImageById(GetPatternGemId(anIndex));
					if (aGem != nullptr) {
						float aSize = aScale;
						if (i == mUnknown538 && mUnknown4C8.mIncRate != 0.0 && !mUnknown4C8.HasBeenTriggered())
							aSize = (float)(mUnknown4C8.GetOutVal() * (double)aScale);
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
		GetGoalLists(aChallenge, mUnknown56C, mUnknown53C, mUnknown55C, mUnknown54C);
		int aMaxHeight = Ftol((double)gApp->mScale * 150.0);
		std::vector<int> aCels;
		std::vector<int> aValues;
		unsigned int i = 0;
		if (mUnknown56C.size() != 0) {
			do {
				int aStat = mUnknown56C[i];
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
					aCel = mUnknown56C[i] - 0x4b;
				}
				if (aCel != -1) {
					aValues.push_back(mUnknown53C[i]);
					aCels.push_back(aCel);
				}
				i++;
			} while (i < mUnknown56C.size());
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
						if (i < mUnknown56C.size())
							aCel = mUnknown56C[i] - 0x4b;
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
			double aBestShown = mUnknown580.GetOutVal();
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
				mTicker660.Draw(g, aMidX, aY, true, theTipFade);
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
		if (aSingle && mUnknown580.GetOutVal() != 0.0) {
			g->SetColor(Color(255, 255, 255, Ftol(mUnknown580.GetOutVal() * 255.0 * (double)theTipFade)));
			SexyString aBest = GT(0x7e40a8);
			g->SetFont(FONT_TOOLTIP);
			int aBestY = Ftol((double)gApp->mScale * 55.0) + aScoreY;
			g->DrawString(aBest, aMidX - g->StringWidth(aBest) / 2, aBestY);
			float aBestAlpha = (float)(mUnknown580.GetOutVal() * (double)theTipFade);
			mTicker6E4.Draw(g, aMidX, aScoreY - Ftol((double)gApp->mScale * -90.0), true, aBestAlpha);
			g->SetColor(Color(255, 255, 255, anAlpha));
		}
	} else {
		// the count's title and the count (unless "nocount"), then the moves (left), or the best while +0x580 shows
		// it
		int aCount = gBoard->mUnknown2638;
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
			double aBestShown = mUnknown580.GetOutVal();
			aY = Ftol((double)aY - aBestShown * (double)Ftol((double)gApp->mScale * 10.0));
			g->SetFont(FONT_TOOLTIP);
			int aTitleY = aY - Ftol((double)gApp->mScale * 42.0);
			g->DrawString(aTitle, aMidX - g->StringWidth(aTitle) / 2, aTitleY);
			mTicker660.Draw(g, aMidX, aY, true, theTipFade);
			aY = aScoreY - Ftol((double)gApp->mScale * -80.0);
		}
		if ((aChallenge->mFlags & Challenge::FLAG_MOVES) != 0) {
			g->SetColor(Color(255, 255, 255, anAlpha));
			aTitle = aChallenge->mMoveGoal != 0 ? GT(0x7e4058) : GT(0x7df518);
			g->SetFont(FONT_TOOLTIP);
			int aMoves = aChallenge->mMoveGoal;
			if (aMoves != 0)
				aMoves -= gApp->mUnknownC74[0x21];
			else
				aMoves = gApp->mUnknownC74[0x21];
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
		} else if (mUnknown580.GetOutVal() != 0.0) {
			g->SetColor(Color(255, 255, 255, Ftol(mUnknown580.GetOutVal() * 255.0 * (double)theTipFade)));
			aTitle = GT(0x7e40a8);
			g->SetFont(FONT_TOOLTIP);
			int aBestY = aY - Ftol((double)gApp->mScale * 20.0);
			g->DrawString(aTitle, aMidX - g->StringWidth(aTitle) / 2, aBestY);
			float aBestAlpha = (float)(mUnknown580.GetOutVal() * (double)theTipFade);
			mTicker6E4.Draw(g, aMidX, aY - Ftol((double)gApp->mScale * -10.0), true, aBestAlpha);
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
		aText = StrFormat(GTC(0x80da2c), mUnknown150);
		int aValueY = aBestY - Ftol((double)gApp->mScale * 10.0);
		g->DrawString(aText, aMidX - g->StringWidth(aText) / 2, aValueY);
	}
}
