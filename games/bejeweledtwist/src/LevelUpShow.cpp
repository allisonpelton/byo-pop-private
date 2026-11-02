// Sexy::LevelUpShow (LevelUpShow.h).
#include "LevelUpShow.h"

#include "Board.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

LevelUpShow::LevelUpShow(Board* theBoard)
{
	mBoard = theBoard;
	mUnknown16C = false;
	mClip = false;
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mUnknown158 = 6.2831854820251465f + 6.2831854820251465f;
	mLevelImage = IMAGE_LEVELCOMPLETE_LEVEL;
	mCompleteImage = IMAGE_LEVELCOMPLETE_COMPLETE;
	mFXManager = new EffectsManager();
	mClip = false;
	// (Direct3D has curves of its own, 0x7e6678 and 0x7e6640)
	mScaleCurve.SetCurve(GTS(0x7e660c));
	mHeightCurve.SetCurve(GTS(0x7e65e4), &mScaleCurve);
	mNumberScale = 0.8f;

	// The level's number, its digits a space apart except next to a 1
	Font* aFont = FONT_LEVEL_NUMBER;
	SexyString aDigits = StrFormat(GTC(0x80da2c), gBoard->mUnknown158);
	SexyString aText;
	for (int i = 0; i < (int)aDigits.size(); i++) {
		if (i != 0 && aDigits[i - 1] != '1' && aDigits[i] != '1')
			aText += ' ';
		aText += aDigits[i];
	}
	int aWidth = Ftol((double)gApp->mScale * 90.0);
	aWidth = aFont->StringWidth(aText) + aWidth;
	int aPad = Ftol((double)gApp->mScale * 90.0);
	int aHeight = aFont->mHeight;
	mNumber = new MemoryImage(gApp);
	mNumber->Create(aWidth, aHeight - aPad);
	mNumber->Clear();
	{
		Graphics g(mNumber);
		g.SetFont(aFont);
		g.SetColor(Color(255, 255, 255));
		int aX = Ftol((double)gApp->mScale * 40.0);
		if (aText[0] == '1')
			aX += Ftol((double)gApp->mScale * -30.0);
		int aY = Ftol((double)mNumber->mHeight * 0.75);
		g.DrawString(aText, aX, aY);
		mNumber->BitsChanged();
	}
}

LevelUpShow::~LevelUpShow()
{
	delete mNumber;
	// port: the original never deletes its effects manager; the port does
	delete mFXManager;
}

void LevelUpShow::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	AddWidget(mFXManager);
}

void LevelUpShow::RemovedFromManager(WidgetManager* theWidgetManager)
{
	RemoveWidget(mFXManager);
}

void LevelUpShow::Update()
{
	TwistWidget::Update();
	if (!mScaleCurve.IncInVal())
		mClicked = true;
	mCycle.Update();
	if (mState == 2) {
		double aScale = (double)mNumberScale - 0.02;
		mNumberScale = (float)aScale;
		if (aScale <= 0.01) {
			mNumberScale = 0.01f;
			mClicked = true;
		}
	}
	if (mUpdateCnt == 200 && gApp->mRegistered && mBoard->mGameMode == Board::MODE_CLASSIC &&
		9 < mBoard->mUnknown158) {
		mBoard->UnlockMode(3);  // 0x6b40b5
	}
	MarkDirty();
}

void LevelUpShow::Draw(Graphics* g)
{
	if (mBoard->mUnknown23F4 || 2 <= mState)
		return;
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255));
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// (the original asks the board for its middle, 0x6a48c9, and keeps nothing of it)
	double anAlpha = mScaleCurve.GetOutVal() * 4.0 - 0.5;
	if (1.0 <= anAlpha)
		anAlpha = 1.0;
	if (!(0.0 < anAlpha))
		anAlpha = 0.0;
	float aFade = (float)anAlpha;
	float aCenterX = (float)Ftol((double)mWidth * 0.45);
	float aY = 0;
	// (Direct3D builds a SexyTransform2D for each picture as well and draws by it)
	for (int i = 0; i < 2; i++) {
		Image* anImage;
		int aX;
		if (i == 0) {
			anImage = mLevelImage;
			aX = Ftol((double)-mNumber->mWidth * 0.75 - (double)mLevelImage->mWidth * 0.949999988079071);
		} else {
			anImage = mCompleteImage;
			aX = Ftol((double)mNumber->mWidth * 0.75);
		}
		aY = (float)(mHeightCurve.GetOutVal() * (double)mHeight);
		float aScale = (float)(mScaleCurve.GetOutVal() * (double)mNumberScale);
		if (0.0f < aFade) {
			g->SetColor(Color(255, 255, 255, Ftol((double)aFade * 255.0)));
			Transform aTransform;
			aTransform.Translate((float)(anImage->mWidth / 2 + aX), 0);
			aTransform.Scale(aScale, aScale);
			aTransform.Translate(aCenterX, aY);
			g->DrawImageTransform(anImage, aTransform, 0, 0);
		}
	}
	float aScale = (float)(mScaleCurve.GetOutVal() * 1.25 * (double)mNumberScale);
	float aX = (float)Ftol((double)gApp->mScale * 0.0);
	g->SetColor(Color(255, 255, 255, Ftol((double)aFade * 255.0)));
	Transform aTransform;
	aTransform.Translate(aX, 0);
	aTransform.Scale(aScale, aScale);
	aTransform.Translate(aCenterX, aY);
	g->DrawImageTransform(mNumber, aTransform, 0, 0);
}
