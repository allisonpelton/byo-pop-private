// Sexy::TextImage and Sexy::GameOverShow (GameOverShow.h).
#include "GameOverShow.h"

#include "Board.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

TextImage::TextImage(Font* theFont, const SexyString& theText, int thePadX, int thePadY, int theJustification,
	int theLineSpacing, const Rect* theRect) : MemoryImage(gApp)
{
	Rect aRect;
	// -1 (the font's own) unless the size comes from the text (0x74e72f): theLineSpacing counts only without theRect
	int aLineSpacing = -1;
	if (theRect == nullptr) {
		aLineSpacing = theLineSpacing;
		if (aLineSpacing < 1)
			aLineSpacing = theFont->mLineSpacingOffset + theFont->mHeight + theFont->mAscentPadding;
		// the widest line
		int aLines = 0;
		int aWidest = 0;
		SexyString aLine;
		for (size_t i = 0; i < theText.size(); i++) {
			if (theText[i] == '\n') {
				aLines++;
				int aWidth = theFont->StringWidth(aLine);
				if (aWidest <= aWidth)
					aWidest = aWidth;
				aLine.clear();
			} else {
				aLine += theText[i];
			}
		}
		aLines++;
		int aWidth = theFont->StringWidth(aLine);
		if (aWidest <= aWidth)
			aWidest = aWidth;
		mTextWidth = aWidest;
		aRect = Rect(0, 0, aWidest + thePadX * 2, aLines * aLineSpacing + thePadY * 2);
	} else {
		aRect = *theRect;
	}
	Create(aRect.mWidth, aRect.mHeight);
	Clear();
	Graphics g(this);
	g.SetFont(theFont);
	g.SetColor(Color(255, 255, 255));
	int aHeight = theFont->GetHeight();
	aRect.mY += thePadY - Ftol((double)aHeight * 0.10000000149011612);
	g.WriteWordWrapped(aRect, theText, aLineSpacing, theJustification);
	BitsChanged();  // 0x42c050
	mD3DFlags = 1;  // +0x44: the Direct3D image flag 1 (the texture is made in the fewest pieces); the original sets it in both modes
}

GameOverShow::GameOverShow(Board* theBoard)
{
	mBoard = theBoard;
	mUnknown168 = false;
	mClip = false;
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mUnknown154 = 6.2831854820251465f + 6.2831854820251465f;
	mText = new TextImage(FONT_INCREDIBLE, gApp->GetString(GTS(0x7e2764), GT(0x7e2750)), 0, 0, 0, 0, nullptr);
	mFXManager = new EffectsManager();
	mScaleCurve.SetCurve(GTS(0x7e2724));
	mHeightCurve.SetCurve(GTS(0x7e2704), &mScaleCurve);
}

GameOverShow::~GameOverShow()
{
	delete mText;
	// port: the original never deletes its effects manager; the port does
	delete mFXManager;
}

void GameOverShow::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	AddWidget(mFXManager);
}

void GameOverShow::RemovedFromManager(WidgetManager* theWidgetManager)
{
	RemoveWidget(mFXManager);
}

void GameOverShow::Update()
{
	TwistWidget::Update();
	mScaleCurve.IncInVal();
	mCycle.Update();
	MarkDirty();
}

void GameOverShow::Draw(Graphics* g)
{
	if (mBoard->mUnknown23F4 || 2 <= mUnknown164)
		return;
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// (the original asks the board for its middle, 0x6a48c9, and keeps nothing of it)
	double anAlpha = mScaleCurve.GetOutVal() * 4.0 - 0.5;
	if (1.0 <= anAlpha)
		anAlpha = 1.0;
	if (!(0.0 < anAlpha))
		anAlpha = 0.0;
	float aFade = (float)anAlpha;
	Transform aTransform;
	float aScaleX = (float)mScaleCurve.GetOutVal();
	float aScaleY = (float)mScaleCurve.GetOutVal();
	aTransform.Scale(aScaleX, aScaleY);
	int aX = Ftol((double)mWidth * 0.5);
	int aY = Ftol(mHeightCurve.GetOutVal() * (double)mHeight);
	if (0.0f < aFade) {
		g->SetColor(Color(255, 213, 63, Ftol((double)aFade * 255.0)));
		g->DrawImageTransformF(mText, aTransform, (float)aX, (float)aY);
	}
}
