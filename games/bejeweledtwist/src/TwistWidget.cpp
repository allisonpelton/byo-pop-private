// Sexy::TwistWidget (TwistWidget.h).
#include "TwistWidget.h"

#include "Font.h"
#include "Graphics.h"
#include "Res.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

TwistWidget::TwistWidget()
{
	mImageColor = Color::White;
	mHasAlpha = true;
	ResetToolTipDelay();
}

TwistWidget::~TwistWidget()
{
}

void TwistWidget::Update()
{
	if (mFrozen)
		return;
	Widget::Update();
	// In 2D the zoom jumps to its end unless the widget asks for it
	if (mZooming && (!mZoom.IncInVal() || (!gApp->Is3DAccelerated() && !mZoomIn2D))) {
		if (!gApp->Is3DAccelerated() && !mZoomIn2D)
			mZoom.SetInVal(1.0);
		if (std::fabs(mZoom.GetOutVal() - 1.0) < 0.009999999776482582 || std::fabs(mZoom.GetOutVal() - 0.0) < 0.009999999776482582)
			mZooming = false;
	}
	float aTarget;
	float aNext;
	if (!mMouseOver) {
		if (mToolTipAlpha <= 0.0f) {
			ResetToolTipDelay();
			return;
		}
		aNext = (float)(mToolTipAlpha - 0.1);
		aTarget = 0.0f;
		if (aNext < 0.0f) {
			mToolTipAlpha = aTarget;
			return;
		}
	} else {
		if (mToolTipDelay > 0.0f) {
			mToolTipDelay = (float)(mToolTipDelay - 0.01);
			return;
		}
		if (mToolTipAlpha >= 1.0f)
			return;
		aNext = (float)(mToolTipAlpha + 0.05);
		aTarget = 1.0f;
		if (aNext > 1.0f) {
			mToolTipAlpha = aTarget;
			return;
		}
	}
	mToolTipAlpha = aNext;
}

void TwistWidget::Draw(Graphics* g)
{
	Widget::Draw(g);
	if (mAlpha == 0.0f)
		return;
	g->SetColorizeImages(true);
	if (mImage != nullptr) {
		g->SetColor(mImageColor);
		g->DrawImage(mImage, 0, 0);
	}
	if (mToolTipAlpha > 0.0f && !mToolTip.empty()) {
		g->PushState();
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetFont(FONT_MENU_TINY);
		int anAlpha = Ftol((double)mToolTipAlpha * 255.0 * mAlpha);
		int aY = mHeight - 5;
		int aX = mWidth - 5;
		int aWidth = g->StringWidth(mToolTip) + 6;
		int anAscent = g->GetFont()->GetAscent();
		int aHeight = g->GetFont()->GetHeight() + (6 - anAscent);
		if (aWidth + aX >= gApp->mWidth)
			aX = gApp->mWidth - aWidth - 3;
		if (aY + aHeight >= gApp->mHeight)
			aY = gApp->mHeight - aHeight - 3;
		Rect aRect(aX, aY, aWidth, aHeight);
		g->SetColor(Color(255, 255, 255, anAlpha < 201 ? anAlpha : 200));
		g->FillRect(aRect);
		g->SetColor(Color(0, 0, 0, anAlpha));
		aRect.mX += 3;
		aRect.mWidth -= 6;
		aRect.mHeight -= 6;
		g->WriteWordWrapped(aRect, mToolTip, -1, -1, nullptr, -1, nullptr);
		g->PopState();
	}
}

void TwistWidget::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	if (mFlushOverlays)
		mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
	// The original zooms 3D through the renderer's transform (gD3DInterface); the port is 2D only for now
	bool aScaled = mZooming && !gApp->Is3DAccelerated() && mZoomIn2D;
	if (aScaled) {
		g->PushState();
		double aScaleY = mZoom.GetOutVal();
		float aScaleX = (float)mZoom.GetOutVal();
		g->SetScale(aScaleX, (float)aScaleY, (float)(mWidth / 2), (float)(mHeight / 2));
	}
	Widget::DrawAll(theFlags, g);
	if (aScaled)
		g->PopState();
}

void TwistWidget::MouseEnter()
{
	Widget::MouseEnter();
	// The original says "TwistWidget ... entered" with OutputDebugString (mName or the address)
	mMouseOver = true;
}

void TwistWidget::MouseLeave()
{
	Widget::MouseLeave();
	// The original says "TwistWidget ... left" with OutputDebugString (mName or the address)
	mMouseOver = false;
	ResetToolTipDelay();
}

void TwistWidget::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	if (mToolTipAlpha == 0.0f)
		ResetToolTipDelay();
}

void TwistWidget::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	if (mNoticeClicks)
		mClicked = true;
}
