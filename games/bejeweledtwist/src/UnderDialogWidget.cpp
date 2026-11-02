// The layer behind the dialogs (UnderDialogWidget.h).
#include "UnderDialogWidget.h"

#include "DDImage.h"
#include "Graphics.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <climits>
#include <cmath>
#include <cstring>

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
		DrawBlurred(g);
		return;
	}
	g->SetColor(Color(0, 0, 0, (int)(gApp->mDialogDim * 160.0)));
	g->FillRect(0, 0, mWidth, mHeight);
	DrawPaused(g);
}

void UnderDialogWidget::MakeQuarterImages()
{
	// 0x7567e7: two quarter-size render targets (flag 0x10), prepared for the device
	for (int i = 0; i < 2; i++) {
		DDImage* anImage = new DDImage(gSexyAppBase->mDDInterface);
		(i == 0 ? mQuarterA : mQuarterB) = anImage;
		anImage->mD3DFlags = 0x10;
		anImage->Create(gApp->mWidth / 4, gApp->mHeight / 4);  // slot 0x78
		anImage->SetImageMode(false, false);
		gD3DInterface->PrepareImage(anImage);  // slot 0x44
	}
}

void UnderDialogWidget::DrawBlurred(Graphics* g)
{
	// 0x756ab0: the screen through the grayscale shader into the quarter-size picture +0x88, then through the
	// under-dialog shader's ripples between that and +0x8c (the three shifting constants of each pass from mUpdateCnt),
	// and +0x8c over the screen at mDialogDim
	if (mQuarterA == nullptr)
		MakeQuarterImages();
	float aGray[4];
	aGray[0] = (float)(gApp->mDialogDim * 0.75);
	aGray[1] = cosf((float)(mUpdateCnt * 0.1));  // 0x401b20
	aGray[2] = sinf((float)(mUpdateCnt * 0.12));  // 0x401b70
	aGray[3] = cosf((float)(mUpdateCnt * 0.1));
	Graphics aGraphicsA(mQuarterA);
	gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_GRAYSCALE]);  // slot 0xd8, 0x85e854
	gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_GRAYSCALE], 0, aGray);  // slot 0x108
	// port: the screen is not a texture of the device's; the screen's picture goes to a pooled target for the draw and
	// comes back (the swap again) before the next pass, so the screen stays as it was
	TargetHold aHold;
	DDImage* aScreen = gD3DInterface->HoldScreen(&aHold, "UnderDialog");
	aGraphicsA.DrawImage(aScreen, 0, 0, mQuarterA->mWidth, mQuarterA->mHeight);  // 0x4216f0
	gD3DInterface->SwapScreen(&aHold.mImage, &aHold.mSwapped);
	gD3DInterface->ReleaseTarget(&aHold);
	gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_UNDER_DIALOG]);  // slot 0xd8, 0x85df48
	Graphics aGraphicsB(mQuarterB);
	// The tiny turns (1e-6 radians, the exe's 0x3eb0c6f7a0000000) make the draws go through the rotated-image path
	unsigned long long aTurnBits = 0x3eb0c6f7a0000000ULL;
	double aTurn;
	std::memcpy(&aTurn, &aTurnBits, sizeof(aTurn));
	float aRipple[4];
	aRipple[0] = 1.0f;
	aRipple[1] = (float)(-(double)sinf((float)(mUpdateCnt * 0.0024)) * 0.005900000222027302);
	aRipple[2] = (float)((double)sinf((float)(mUpdateCnt * 0.0022)) * 0.0031999999191612005);
	aRipple[3] = (float)(-(double)sinf((float)(mUpdateCnt * 0.0018)) * 0.005100000184029341);
	gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_UNDER_DIALOG], 0, aRipple);
	aGraphicsB.DrawImageRotated(mQuarterA, 0, 0, aTurn);  // 0x4219d0
	aRipple[1] = (float)((double)sinf((float)(mUpdateCnt * 0.0016)) * 0.0034000000450760126);
	aRipple[2] = (float)(-(double)sinf((float)(mUpdateCnt * 0.0028)) * 0.00559999980032444);
	aRipple[3] = (float)(-(double)sinf((float)(mUpdateCnt * 0.0023)) * 0.003100000089034438);
	gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_UNDER_DIALOG], 0, aRipple);
	aGraphicsA.DrawImageRotated(mQuarterB, 0, 0, -aTurn);
	aRipple[1] = (float)(-(double)sinf((float)(mUpdateCnt * 0.0019)) * 0.0020000000949949026);
	aRipple[2] = (float)((double)sinf((float)(mUpdateCnt * 0.0023)) * 0.003000000026077032);
	aRipple[3] = (float)(-(double)sinf((float)(mUpdateCnt * 0.0022)) * 0.005200000014156103);
	gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_UNDER_DIALOG], 0, aRipple);
	aGraphicsB.DrawImageRotated(mQuarterA, 0, 0, aTurn);
	gD3DInterface->SetPixelShader(nullptr);
	g->SetColorizeImages(true);  // 0x41ff40
	g->SetColor(Color(255, 255, 255, Ftol((double)gApp->mDialogDim * 255.0)));
	g->DrawImage(mQuarterB, 0, 0, mWidth, mHeight);
	DrawPaused(g);
}

void UnderDialogWidget::DrawPaused(Graphics* g)
{
	// The pause text (string BOARD_PAUSED in FONT_INCREDIBLE, gold, faded with mDialogDim, over the board's middle)
	// shows only over a board that is paused without the options up; it comes with the board
	if (gApp->mBoard == nullptr)
		return;
}
