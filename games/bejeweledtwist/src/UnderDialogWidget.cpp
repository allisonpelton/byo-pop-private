// The layer behind the dialogs (UnderDialogWidget.h).
#include "UnderDialogWidget.h"

#include "Graphics.h"
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
	// The Direct3D path (0x756ab0-0x756e4a) is never taken: the game takes its 2D paths
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
