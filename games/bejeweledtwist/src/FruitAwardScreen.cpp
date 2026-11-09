// Sexy::FruitAwardScreen (FruitAwardScreen.h).
#include "FruitAwardScreen.h"

#include "Board.h"
#include "EffectsManager.h"
#include "GoodTube.h"
#include "Graphics.h"
#include "Piece.h"
#include "Res.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

FruitAwardScreen::FruitAwardScreen(Board* theBoard)
{
	mBoard = theBoard;
	mFXManager = new EffectsManager();
	mDropImage = nullptr;
	mDim = 0;
	mOverlayAlpha = 0;
	// The fruits leave from the tube's top (a fruit dance moves them to its bottom, AwardFruit)
	GoodTube* aTube = mBoard->mGoodTube;
	mFromX = Ftol((double)aTube->mWidth * 0.5 + (double)aTube->mX);
	mFromY = Ftol((double)aTube->mHeight * 0.15000000596046448 + (double)aTube->mY);
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
}

FruitAwardScreen::~FruitAwardScreen()
{
	delete mFXManager;
	RemoveAllWidgets(true, false);
}

void FruitAwardScreen::Resize(int, int, int, int)
{
	// The whole window, whatever is asked
	Widget::Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mFXManager->Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mCenterX = mWidth / 2;
	mCenterY = mHeight / 2 - 40;
}

void FruitAwardScreen::SetPiece(int thePieceId)
{
	mPieceId = thePieceId;
	Piece* aPiece = gBoard->GetPieceById(thePieceId);
	mRow = aPiece->mRow;
	mCol = aPiece->mCol;
}

void FruitAwardScreen::Init(int theColor)
{
	mDim = 0;
	mSwayStart = 0;
	mArtShift = 0;
	mTubeShift = 0;
	mState = 4;
	mSwaySize = 10.0f;
	mColor = theColor;
	mSwayScale = 1.0f;
	mDimStep = 0.1f;
	mProgressStep = 0.015f;
	mFromScale = 1.25f;
	mToScale = 0.5f;
	mProgress = 0;
	Piece* aPiece = gBoard->GetPieceAt(mRow, mCol);
	mToX = Ftol((double)(gGemWidth / 2) + (double)aPiece->mX);
	mToY = Ftol((double)(gGemHeight / 2) + (double)aPiece->mY);
	mState = 4;
	gApp->PlaySample(SOUND_TROPHY_AWARDED);
}

void FruitAwardScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	AddWidget(mFXManager);
	Init(gBoard->GetPieceAt(mRow, mCol)->mColor);
	mCycles[0].SetPosition(0.1f);
	for (int i = 0; i < 2; i++)
		mCycles[i].mSpeed = 0.5f;
}

void FruitAwardScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	mClicked = false;
	RemoveWidget(mFXManager);
}

void FruitAwardScreen::Update()
{
	TwistWidget::Update();
	// mDropImage's offset at the art's size (mDropImage is never set; the port reads no offset for an image the
	// resources do not know)
	float anArtScale = (float)((double)gApp->mArtRes / 1200.0);
	ResourceId anId = GetIdByImage(mDropImage);
	int anOffset = (unsigned)anId < RESOURCE_ID_MAX ? GetImageOffsetY(anId) : 0;
	mImageOffset = Ftol((double)anOffset * (double)anArtScale);
	for (int i = 0; i < 2; i++)
		mCycles[i].Update();
	if (mState == 4) {
		// The screen clears while the fruits fly from the tube to the gem, which turns into the fruit when they arrive
		float aDim = (float)((double)mDim - 0.029999999329447746);
		mDim = aDim;
		if (!(0.0f < aDim))
			mDim = 0;
		float aProgress = (float)((double)mProgress + 0.014999999664723873);
		mProgress = aProgress;
		if (!(1.0f > aProgress)) {
			mProgress = 1.0f;
			if (!mClicked) {
				mClicked = true;
				gBoard->MakeFruitGem(gBoard->GetPieceAt(mRow, mCol));
			}
		} else if (mUpdateCnt % 3 == 0) {
			EffectsManager* aManager = gBoard->mFXManager;
			Effect* anEffect = aManager->AllocEffect(0);
			anEffect->mX = (float)((double)(mToX - mFromX) * (double)mProgress + (double)mFromX);
			anEffect->mY = (float)((double)(mToY - mFromY) * (double)mProgress + (double)mFromY);
			anEffect->mImage = IMAGE_FRUITS;
			anEffect->mGravity = 0;
			anEffect->mFrame = mColor;
			anEffect->mColor = Color(255, 255, 200);
			anEffect->mIsAdditive = true;
			float aScale = (float)(((double)mToScale - (double)mFromScale) * (double)mProgress + (double)mFromScale);
			anEffect->mScale = (float)((double)aScale + (double)aScale);
			anEffect->mDAlpha = -0.02f;
			anEffect->mDScale = 0;
			aManager->AddEffect(anEffect);
		}
	}
}

void FruitAwardScreen::Draw(Graphics* g)
{
	TwistWidget::Draw(g);
	g->SetColorizeImages(true);
	g->SetColor(Color(0, 0, 0, Ftol((double)mDim * 196.0)));
	g->FillRect(0, 0, mWidth, mHeight);
	g->SetColor(Color(255, 255, 255, 0x60));
	g->SetColor(Color::White);
	// (the original works out a sine of the update count here and drops it)
	if (0.0f < mOverlayAlpha) {
		Color aColor = gGemColors[mColor];
		aColor.mAlpha = Ftol((double)mOverlayAlpha * 200.0 * (double)mDim);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(aColor);
		g->SetColor(Color::White);
	}
}

void FruitAwardScreen::MouseMove(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
}
