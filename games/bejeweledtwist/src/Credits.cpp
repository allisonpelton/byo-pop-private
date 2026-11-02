// Credits (Credits.h).
#include "Credits.h"

#include "Board.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

namespace {

// 0x85c668: the lines, wide texts set by the static initialiser 0x7d51ae (the count, 142, at 0x84ecb4). A line that
// starts with a backslash loses it and the character after it; "\B" makes a heading. "GEM" (0x7e0f88) is a fruit.
const unsigned int kLines[] = {
	GTA(0x7e0f88), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e1bc8),
	GTA(0x7e1b8c), GTA(0x80d9a4), GTA(0x7e1b64), GTA(0x7e1b48), GTA(0x80d9a4), GTA(0x7e1b28), GTA(0x7e1ae8), GTA(0x80d9a4),
	GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e0f88), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e1a98), GTA(0x7e1a74),
	GTA(0x80d9a4), GTA(0x7e1a30), GTA(0x7e1a14), GTA(0x80d9a4), GTA(0x7e19e4), GTA(0x7e19a8), GTA(0x80d9a4), GTA(0x80d9a4),
	GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e0f88), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e1988), GTA(0x7e1968), GTA(0x7e1954),
	GTA(0x7e193c), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e0f88), GTA(0x80d9a4), GTA(0x80d9a4),
	GTA(0x7e1928), GTA(0x7e18e8), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e0f88), GTA(0x80d9a4),
	GTA(0x80d9a4), GTA(0x7e18d0), GTA(0x7e18b4), GTA(0x80d9a4), GTA(0x7e1898), GTA(0x7e1880), GTA(0x80d9a4), GTA(0x80d9a4),
	GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e0f88), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e1868), GTA(0x7e1844), GTA(0x80d9a4),
	GTA(0x7e1828), GTA(0x7e180c), GTA(0x7e17f0), GTA(0x7e17d0), GTA(0x7e17bc), GTA(0x7e1798), GTA(0x7e177c), GTA(0x7e175c),
	GTA(0x7e1744), GTA(0x7e172c), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e0f88),
	GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e1704), GTA(0x7e16ec), GTA(0x7e1968), GTA(0x7e16d0), GTA(0x80d9a4), GTA(0x80d9a4),
	GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e0f88), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e16b0), GTA(0x7e1638),
	GTA(0x7e15d0), GTA(0x7e1568), GTA(0x7e1500), GTA(0x7e1488), GTA(0x7e1418), GTA(0x7e13a8), GTA(0x7e1340), GTA(0x7e12d0),
	GTA(0x7e1258), GTA(0x7e11e8), GTA(0x7e1170), GTA(0x7e1120), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4),
	GTA(0x7e0f88), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e10fc), GTA(0x7e10e0), GTA(0x7e10c8), GTA(0x7e10b4), GTA(0x7e109c),
	GTA(0x7e1954), GTA(0x7e1080), GTA(0x7e1060), GTA(0x7e1044), GTA(0x7e1028), GTA(0x7e100c), GTA(0x7e0fe8), GTA(0x7e0fd4),
	GTA(0x7e0fb4), GTA(0x7e0f90), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x80d9a4), GTA(0x7e0f88),
};
const int kLineCount = 142;  // 0x84ecb4
static_assert(sizeof(kLines) / sizeof(kLines[0]) == kLineCount, "the credits have 142 lines");

}  // namespace

Credits::Credits()
{
	mFXManager = new EffectsManager();
	mFade = 0;
	mState = 0;
	mContentAlpha = 0;
	mButton = nullptr;
	mGrow = 0;
	mClip = false;
	mLineDelay = 0;
	mPressed = false;
	mNextLine = 0;
	mFruitCel = 0;
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
}

Credits::~Credits()
{
	delete mFXManager;
	RemoveAllWidgets(true, false);
}

void Credits::Resize(int theX, int theY, int theWidth, int theHeight)
{
	(void)theX; (void)theY; (void)theWidth; (void)theHeight;
	Widget::Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mFXManager->Resize(0, 0, gApp->mWidth, gApp->mHeight);
}

void Credits::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	gApp->PlaySample(SOUND_TROPHY_AWARDED);
	mState = 1;
	mButton = new TwistButton(0, IMAGE_RANKBAR_CONTINUEBUTTON, this);
	mButton->mDoFinger = true;
	int aWidth = mButton->mButtonImage->GetCelWidth();
	int aHeight = mButton->mButtonImage->GetCelHeight();
	mButton->Resize(mWidth / 2 - aWidth / 2, mHeight - Ftol((double)gApp->mScale * 115.0), aWidth, aHeight);
	AddWidget(mButton);
	AddWidget(mFXManager);
	mTilt.SetCurve(GTS(0x7e0e10));
	mTilt.SetMode(1);
	mBob.SetCurve(GTS(0x7e0dc8));
	mBob.SetMode(1);
}

void Credits::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	mClicked = false;
	if (mButton != nullptr) {
		RemoveWidget(mButton);
		gApp->SafeDeleteWidget(mButton);
		mButton = nullptr;
	}
	RemoveWidget(mFXManager);
}

void Credits::ButtonDepress(int theId)
{
	(void)theId;
	mPressed = true;
	Close();
}

void Credits::Close()
{
	if (mState != 5)
		mState = 5;
}

void Credits::Update()
{
	TwistWidget::Update();
	mBob.IncInVal();
	mTilt.IncInVal();
	// The box: three quarters of the screen each way, in the middle
	int aBoxWidth = Ftol((double)mWidth * 0.75);
	int aBoxHeight = Ftol(0.75 * (double)mHeight);
	mBox = Rect(mWidth / 2 - aBoxWidth / 2, mHeight / 2 - aBoxHeight / 2, aBoxWidth, aBoxHeight);

	if (1 < mState && mState < 5 && mGrow < 1.0f) {
		// While the bars grow, sparks in the gems' colours along the box's top or bottom edge
		int aCount = 5;
		do {
			Effect* anEffect = mFXManager->AllocEffect(10);
			anEffect->mScale = (float)((double)anEffect->mScale * 1.5);
			anEffect->mColor = gGemColors[Rand() % 7];
			int aSide = Rand() % 2;
			anEffect->mX = (float)((double)RandSigned() * (double)mBox.mWidth * 0.5 * (double)mGrow +
				(double)(mWidth / 2));
			float aTop = (float)mBox.mY;
			anEffect->mY = aTop;
			if (0 < aSide)
				anEffect->mY = (float)((double)mBox.mHeight + (double)aTop);
			mFXManager->AddEffect(anEffect);
		} while (--aCount != 0);
	}

	int aState = mState;
	if (aState == 1) {
		// the screen darkens
		float aFade = (float)((double)mFade + 0.019999999552965164);
		if (1.0f < aFade)
			aFade = 1.0f;
		mFade = aFade;
		if (aFade == 1.0f)
			mState = 2;
	} else if (aState == 5) {
		// everything fades away; then the app deletes it (WinTwistApp::UpdateFrames)
		float aFade = (float)((double)mFade - 0.019999999552965164);
		if (aFade < 0.0f)
			aFade = 0.0f;
		mFade = aFade;
		mContentAlpha = aFade;
		if (aFade == 0.0f)
			mClicked = true;
	} else if (aState == 4) {
		if (!(0.0f < mLineDelay)) {
			mLineDelay = (float)Ftol((double)gApp->mScale * 50.0) + mLineDelay;
			if (mNextLine < kLineCount) {
				// the next line starts at the box's bottom
				Line aLine;
				aLine.mText.assign(GT(kLines[mNextLine]), 0, SexyString::npos);
				aLine.mImage = nullptr;
				aLine.mY = (float)(mBox.mHeight + mBox.mY);
				aLine.mFont = FONT_DIALOG_CHECKBOX;
				aLine.mColor = Color(255, 220, 52);
				if (aLine.mText[0] == '\\') {
					aLine.mText.erase(aLine.mText.begin());
					if (aLine.mText.size() > 0) {
						if (aLine.mText[0] == 'B') {
							aLine.mFont = FONT_SCORE;
							aLine.mColor = Color(200, 40, 150);
						}
						aLine.mText.erase(aLine.mText.begin());
					}
				} else if (aLine.mText == GT(0x7e0f88)) {
					// "GEM": a fruit, the next of the seven
					aLine.mText = GT(0x80d9a4);
					aLine.mImage = IMAGE_FRUITS;
					aLine.mCel = mFruitCel;
					mFruitCel++;
					if (mFruitCel == 7)
						mFruitCel = 0;
					aLine.mColor = Color::White;  // the exe's 0xe14068
				}
				aLine.mColor.mAlpha = 0;
				mLines.push_back(aLine);
				mNextLine++;
			}
		}
	} else if (aState == 2 || aState == 3) {
		bool aGrow = true;
		if (aState == 3) {
			// the box and the bars appear
			float anAlpha = (float)((double)mContentAlpha + 0.004999999888241291);
			if (1.0f < anAlpha)
				anAlpha = 1.0f;
			mContentAlpha = anAlpha;
			if (anAlpha == 1.0f && 1.0f <= mGrow) {
				mState = 4;
				aGrow = false;
			}
		}
		if (aGrow) {
			// the bars grow (state 2, and state 3 until they are done)
			float aGrowth = (float)((double)mGrow + 0.009999999776482582);
			if (1.0f < aGrowth)
				aGrowth = 1.0f;
			mGrow = aGrowth;
			if (aState != 3 && 1.0f <= aGrowth)
				mState = 3;
		}
	}

	if (mUpdateCnt % 2 == 0) {
		// Every other update the lines rise by mScale: they fade in below a line 150 (art) under the box's top
		// and out above it, and go once they pass the top; the last one gone closes the credits
		float aStep = gApp->mScale;
		mLineDelay = mLineDelay - aStep;
		unsigned int i = 0;
		if (mLines.size() > 0) {
			do {
				Line& aLine = mLines[i];
				float aY = aLine.mY - aStep;
				aLine.mY = aY;
				int aFadeY = mBox.mY - Ftol((double)gApp->mScale * -150.0);
				int anAlpha;
				if ((double)aY <= (double)aFadeY) {
					anAlpha = aLine.mColor.mAlpha - 5;
					if (anAlpha < 0)
						anAlpha = 0;
				} else {
					anAlpha = aLine.mColor.mAlpha + 5;
					if (anAlpha > 0xff)
						anAlpha = 0xff;
				}
				aLine.mColor.mAlpha = anAlpha;
				if (aY < (float)mBox.mY) {
					mLines.erase(mLines.begin() + i);
					i--;
					if (mLines.size() == 0)
						Close();
				}
				i++;
			} while (i < mLines.size());
		}
	}
}

void Credits::Draw(Graphics* g)
{
	TwistWidget::Draw(g);
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// the box, tiled
	g->SetColor(Color(128, 128, 255, Ftol((double)mContentAlpha * 225.0)));
	g->DrawImageBox(mBox, IMAGE_INGAMEUI_GRIDSQUARE);
	// the bars over its top and bottom edges, stretched to the box's width as they grow (both as tall as the top one)
	Image* aTopBar = IMAGE_INGAMEUI_GRIDBAR_TOP;
	int aBarWidth = Ftol((double)mBox.mWidth * (double)mGrow * 1.0299999713897705);
	int aBarHeight = aTopBar->mHeight;
	g->SetColor(Color(200, 200, 255, Ftol((double)mContentAlpha * 255.0)));
	int aHalfBar = aBarWidth / 2;
	g->DrawImage(aTopBar, mWidth / 2 - aHalfBar, mBox.mY - Ftol((double)gApp->mScale * -0.0), aBarWidth, aBarHeight);
	g->DrawImage(IMAGE_INGAMEUI_GRIDBAR_BOT, mWidth / 2 - aHalfBar,
		(mBox.mHeight - Ftol((double)gApp->mScale * 10.0)) + mBox.mY, aBarWidth, aBarHeight);
	// the lines, inside the box
	g->SetClipRect(mBox);
	unsigned int i = 0;
	if (mLines.size() > 0) {
		do {
			Line& aLine = mLines[i];
			Color aColor = aLine.mColor;
			aColor.mAlpha = Ftol((double)aColor.mAlpha * (double)mContentAlpha);
			g->SetColor(aColor);
			if (aLine.mImage != nullptr) {
				// a fruit, tilting and bobbing
				float aRot = (float)(mTilt.GetOutVal() * 3.1415927410125732 * 0.25);
				float aY = (float)((double)aLine.mY - mBob.GetOutVal() * 25.0);
				DrawImageCentered(g, aLine.mImage, aLine.mImage->GetCelRect(aLine.mCel), (float)(mWidth / 2), aY,
					(double)aRot, 1.0f, 1.0f);
			} else {
				g->SetFont(aLine.mFont);
				int aY = Ftol((double)aLine.mY);
				g->DrawString(aLine.mText, mWidth / 2 - g->StringWidth(aLine.mText) / 2, aY);
			}
			i++;
		} while (i < mLines.size());
	}
	g->ClearClipRect();
}

void Credits::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
	// the screen darkens behind it
	g->SetColor(Color(0, 0, 4, Ftol((double)mFade * 200.0)));
	g->FillRect(0, -mY, mWidth, mHeight);
	TwistWidget::DrawAll(theFlags, g);
}
