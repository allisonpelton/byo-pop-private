// The layer behind the dialogs (UnderDialogWidget.h).
#include "UnderDialogWidget.h"

#include "Graphics.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"

#include <climits>

using namespace Sexy;

UnderDialogWidget::UnderDialogWidget()
{
	mMouseVisible = false;
	mHasAlpha = true;
}

void UnderDialogWidget::Draw(Graphics* g)
{
	mWidgetManager->FlushDeferredOverlayWidgets(INT_MAX);
	if (gD3DInterface != nullptr && gApp->mDDInterface != nullptr && (gD3DInterface->GetCaps(0) & 2) != 0) {
		// 0x756ab0-0x756e4a: the screen goes through the grayscale shader into the quarter-size picture +0x88 (a render
		// target, 0x10), and from it through the under-dialog shader's ripples (constants from mUpdateCnt) between that and
		// +0x8c, then +0x8c is drawn over the screen at mDialogDim. Without the screen image as a source of a draw (the
		// device's GPUDisplay draws to it only) the picture it blurs does not exist.
		NotPorted("UnderDialogWidget::Draw 0x756a92: the blurred, rippling screen behind a dialog (needs the screen image as a "
			"texture source for the grayscale shader's draw into a quarter-size render target)");
	}
	g->SetColor(Color(0, 0, 0, (int)(gApp->mDialogDim * 160.0)));
	g->FillRect(0, 0, mWidth, mHeight);
	DrawPaused(g);
}

void UnderDialogWidget::DrawPaused(Graphics* g)
{
	// The pause text (string BOARD_PAUSED in FONT_INCREDIBLE, gold, faded with mDialogDim, over the board's middle)
	// shows only over a board that is paused without the options up; it comes with the board
	if (gApp->mBoard == nullptr)
		return;
}
