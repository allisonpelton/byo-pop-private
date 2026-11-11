// CheckpointDialog: the starting point box (see CheckpointDialog.h).

#include "CheckpointDialog.h"
#include "AstroPopApp.h"
#include "AstroDialog.h"
#include "GameText.h"
#include "Graphics.h"
#include "Font.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

// 0x488760. The box has no buttons; its inset picture is made in the colours of the game's set (the button picture the
// maker also gives is not needed and goes at once).
CheckpointDialog::CheckpointDialog(int theColourSet, AstroPopApp* theApp) :
	Text2004<Dialog>(IMAGE_DIALOG, IMAGE_DIALOG_BTN, 0x78, true, GT(0x00564b3c), std::string(), std::string(), 0)
{
	mApp = theApp;
	m114 = 0;
	mLevel = -1;

	Color anInsetColor;
	Color aButtonColor;
	if (theColourSet >= 0 && theColourSet <= 5)
	{
		anInsetColor = theApp->GetDialogInsetColor(theColourSet);
		aButtonColor = theApp->GetDialogColor(theColourSet);
	}

	MemoryImage* anInsetImage = NULL;
	MemoryImage* aButtonImage = NULL;
	theApp->MakeDialogImages(anInsetColor, aButtonColor, &anInsetImage, &aButtonImage, false);
	mComponentImage = anInsetImage;
	delete aButtonImage;

	mAnim.SetNumFrames(1);
	mAnim.SetDelay(0xc);
	mAnim.mHoldLast = false;
	mAnim.mPingPong = false;
	mAnim.SetStopped(false);
}

// 0x4896a0 (body 0x488b70): the box's own inset picture goes with it
CheckpointDialog::~CheckpointDialog()
{
	delete mComponentImage;
	mComponentImage = NULL;
}

// Is a mouse y in the row of the two gates
bool CheckpointDialog::OverGateRow(int y)
{
	return (mHeight - IMAGE_SLOW_WARP_GATE->mHeight) - 0x28 <= y && y <= mHeight - 0x28;
}

// 0x488c00: the two gates side by side (each drawn again additively over itself), the planet each leads to on it, and the
// words under it
void CheckpointDialog::Draw(Graphics* g)
{
	int aGateWidth = IMAGE_SLOW_WARP_GATE->GetCelWidth();
	int aGateY = (mHeight - IMAGE_SLOW_WARP_GATE->mHeight) - 0x28;

	Dialog::Draw(g);
	Rect aBox(mBackgroundInsets.mLeft, mBackgroundInsets.mTop, (mWidth - mBackgroundInsets.mRight) - mBackgroundInsets.mLeft,
			  (mHeight - mBackgroundInsets.mBottom) - mBackgroundInsets.mTop);
	g->DrawImageBox(aBox, IMAGE_DIALOG);

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	int aCel = mAnim.GetCel();
	g->DrawImageCel(IMAGE_SLOW_WARP_GATE, 10, aGateY, aCel);
	g->DrawImageCel(IMAGE_FAST_WARP_GATE, aGateWidth + 0x14, aGateY, aCel);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	g->SetColor(Color(0xff, 0xff, 0xff));
	g->SetFont(GetFontById(0x120));
	WriteString(g, GT(0x56b638), 0x23, mSpaceAfterHeader + 0x3e + mContentInsets.mTop, 0x113, 0, true, 0, -1);

	g->SetFont(mLinesFont);
	int aPlanetWidth = IMAGE_WARP_PLANETS->GetCelWidth();

	// the first gate leads to the first level: the first planet
	Rect aSource(0, 0, aPlanetWidth, IMAGE_WARP_PLANETS->mHeight);
	int aPlanetY = (IMAGE_SLOW_WARP_GATE->mHeight / 2 - IMAGE_WARP_PLANETS->mHeight / 2) + aGateY;
	g->DrawImage(IMAGE_WARP_PLANETS, (aGateWidth / 2 - aPlanetWidth / 2) + 10, aPlanetY, aSource);

	g->SetColor(Color(0xff, 0xff, 0xff));
	Font* aFont = GetFontById(0xb1);
	g->SetFont(aFont);
	std::string aText = GT(0x0056b630);
	int aTextY = IMAGE_SLOW_WARP_GATE->mHeight + aGateY;
	g->DrawString(aText, (aGateWidth / 2 - aFont->StringWidth(aText) / 2) + 10, aTextY);

	// the second gate leads to the checkpoint: the third planet
	int aSecondX = aGateWidth + 0x14;
	Rect aSource2(aPlanetWidth * 2, 0, aPlanetWidth, IMAGE_WARP_PLANETS->mHeight);
	int aPlanetY2 = (IMAGE_FAST_WARP_GATE->mHeight / 2 - IMAGE_WARP_PLANETS->mHeight / 2) + aGateY;
	g->DrawImage(IMAGE_WARP_PLANETS, (IMAGE_FAST_WARP_GATE->GetCelWidth() / 2 - aPlanetWidth / 2) + aSecondX, aPlanetY2, aSource2);

	aText = StrFormat(GTC(0x00564634), mLevel);
	g->SetFont(aFont);
	g->DrawString(aText, (aGateWidth / 2 - aFont->StringWidth(aText) / 2) + aSecondX, aTextY);
}

// 0x483800
void CheckpointDialog::Update()
{
	Dialog::Update();
	mAnim.Update();
}

// 0x483820: the hand over a gate
void CheckpointDialog::MouseMove(int x, int y)
{
	int aGateWidth = IMAGE_SLOW_WARP_GATE->GetCelWidth();
	bool aOver = false;
	if (x >= 0xf && x <= aGateWidth - 10)
		aOver = OverGateRow(y);
	else if (x >= aGateWidth + 0x19 && x <= aGateWidth * 2)
		aOver = OverGateRow(y);
	mApp->SetCursor(aOver ? CURSOR_HAND : CURSOR_POINTER);
}

// 0x4838b0: a click on the left gate answers like a second button (3000 + id), on the right gate like a first (2000 + id)
void CheckpointDialog::MouseDown(int x, int y, int theClickCount)
{
	(void) theClickCount;
	int aGateWidth = IMAGE_SLOW_WARP_GATE->GetCelWidth();
	if (x >= 0xf && x <= aGateWidth - 10 && OverGateRow(y))
	{
		mApp->PlaySample(SOUND_WARP_BUTTON);
		mApp->ButtonDepress(mId + 3000);
		return;
	}
	if (aGateWidth + 0x19 <= x && x <= aGateWidth * 2 && OverGateRow(y))
	{
		mApp->PlaySample(SOUND_WARP_BUTTON);
		mApp->ButtonDepress(mId + 2000);
	}
}
