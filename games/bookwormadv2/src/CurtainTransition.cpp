#include "CurtainTransition.h"

#include "Color.h"
#include "DDImage.h"
#include "Font.h"
#include "GameApp.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Res.h"

using namespace Sexy;

CurtainTransition::CurtainTransition(GameApp* theApp)
{
	mApp = theApp;
	mClosing = false;  // port: the original left it to the first StartClosing or StartOpening
	mDrawn = false;
	mFrame = -1;
	mImage = IMAGE_CURTAIN_ANIM;
	mShowLoadingBox = false;
	mLoadingBoxShown = false;

	// port: the flipped and mirrored copies of the curtain are made once and kept. The original made them for every
	// curtain, which cost 25 to 30 ms before the first frame and a texture upload on it (a visible hitch).
	static DDImage* sFlipped = nullptr;
	static DDImage* sMirrored = nullptr;
	static DDImage* sMirroredFlipped = nullptr;
	if (sFlipped == nullptr) {
		DDImage* anImage = mApp->CopyImage(IMAGE_CURTAIN_ANIM);
		mApp->FlipImage(anImage);
		sFlipped = anImage;
		anImage = mApp->CopyImage(IMAGE_CURTAIN_ANIM);
		mApp->MirrorImage(anImage);
		sMirrored = anImage;
		anImage = mApp->CopyImage(IMAGE_CURTAIN_ANIM);
		mApp->MirrorImage(anImage);
		mApp->FlipImage(anImage);
		sMirroredFlipped = anImage;
	}
	mFlippedImage = sFlipped;
	mMirroredImage = sMirrored;
	mMirroredFlippedImage = sMirroredFlipped;

	mDone = false;
	mTopMoving = false;
	Resize(0, 0, mApp->mWidth, mApp->mHeight);
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	mMouseVisible = false;
	mHasTransparencies = true;
	mTopY = 0;
	mCordX = 42;
	mHasAlpha = true;
}

CurtainTransition::~CurtainTransition()
{
	// (the copies are kept, see the constructor)
}

void CurtainTransition::StartClosing()
{
	mFrame = IMAGE_CURTAIN_ANIM->mNumCols - 1;
	mClosing = true;
	mTopY = -IMAGE_CURTAIN_TOP->mHeight;
	mDone = false;
	mTopMoving = false;
	mDrawn = false;
	mCordX = -IMAGE_CURTAIN_CORD->mWidth;
	mApp->SetCursor(CURSOR_POINTER);
}

void CurtainTransition::StartOpening()
{
	mDrawn = false;
	mClosing = false;
	mFrame = 0;
	mDone = false;
	mTopMoving = false;
	mApp->SetCursor(CURSOR_POINTER);
}

// A cel every 5 updates; halfway the valance starts to move
void CurtainTransition::Update()
{
	Widget::Update();
	if (mDone)
		return;
	int aNumCels = IMAGE_CURTAIN_ANIM->mNumCols;
	if (mClosing) {
		if (mFrame == 0 && mTopY == 0 && mDrawn && (!mShowLoadingBox || mLoadingBoxShown)) {
			mDone = true;
			mApp->CurtainDone();
			mShowLoadingBox = false;
			return;
		}
		if (mUpdateCnt % 5 == 0 && mFrame > 0) {
			mFrame--;
			if (mFrame == aNumCels / 2)
				mTopMoving = true;
		}
		if (mTopMoving && mTopY < 0) {
			mTopY += 4;
			if (mTopY >= 0) {
				mTopMoving = false;
				mTopY = 0;
			}
		}
		mCordX += 9;
		MarkDirty();
		WantFirstDraw(aNumCels);
		return;
	}

	int aTopHeight = IMAGE_CURTAIN_TOP->mHeight;
	if (mFrame == aNumCels - 1 && mDrawn && mTopY == -aTopHeight) {
		mDone = true;
		mApp->CurtainDone();
		return;
	}
	if (mUpdateCnt % 5 == 0 && mFrame < aNumCels) {
		int aFrame = mFrame + 1;
		mFrame = aFrame;
		if (aFrame == aNumCels / 2)
			mTopMoving = true;
		if (aFrame == aNumCels)
			mFrame = aFrame - 1;
	}
	if (mTopMoving && mTopY > -aTopHeight) {
		mTopY -= 4;
		if (mTopY <= -aTopHeight) {
			mTopMoving = false;
			mTopY = -aTopHeight;
		}
	}
	mCordX -= 9;
	MarkDirty();
	WantFirstDraw(aNumCels);
}

// port: the curtain is done only once its last frame has been drawn (mDrawn, set by Draw), and the original draws after
// every update; -fast draws every FAST_DRAW_INTERVAL-th update, which held the board (IsCurtainMoving) for 0 to 9 more
// updates depending on the update count's phase. With -fast the update that reaches the last frame asks for a draw
// after it, as in normal play (2026-11-16, chapter 6.4: a whole battle came out otherwise after a load 8 updates longer).
void CurtainTransition::WantFirstDraw(int theNumCels)
{
	if (mApp->mFastMode && !mDrawn && mFrame == (mClosing ? 0 : theNumCels - 1))
		mApp->mLastFastDrawUpdate = mApp->mUpdateCount - SexyAppBase::FAST_DRAW_INTERVAL;
}

void CurtainTransition::Draw(Graphics* g)
{
	int aNumCels = IMAGE_CURTAIN_ANIM->mNumCols;
	if (!mDrawn && mFrame == (mClosing ? 0 : aNumCels - 1))
		mDrawn = true;
	if (mDone && mClosing) {
		g->SetColor(Color::Black);
		g->FillRect(0, 0, mWidth, mHeight);
	}
	g->DrawImageCel(mImage, -1, -1, mFrame);
	g->DrawImageCel(mMirroredImage, mWidth / 2, -1, aNumCels - mFrame - 1);
	g->DrawImageCel(mFlippedImage, -1, mHeight / 2 + 1, mFrame);
	g->DrawImageCel(mMirroredFlippedImage, mWidth / 2, mHeight / 2, aNumCels - mFrame - 1);

	if (mShowLoadingBox && mClosing && mFrame == 0 && mTopY == 0) {
		mLoadingBoxShown = true;
		g->DrawImageBox(Rect(225, 200, 350, 200), IMAGE_DIALOG);
		g->SetFont(FONT_SMALL);
		g->SetColor(Color::Black);
		g->WriteString(mLoadingText, 225, 310, 340, 0);  // centred
		g->DrawImage(IMAGE_DIALOG_BANNER_LOADING, (350 - IMAGE_DIALOG_BANNER_LOADING->GetWidth()) / 2 + 225, 217);
	}
}
