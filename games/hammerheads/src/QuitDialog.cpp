#include "QuitDialog.h"

#include <cmath>

#include "Application.h"
#include "Color.h"
#include "DialogButton.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Insets.h"
#include "Res.h"

using namespace Sexy;

namespace
{

// The box is this wide and this tall
const int DIALOG_WIDTH = 700;
const int DIALOG_HEIGHT = 0x244;

// The width the layout of the box starts from, before it is made DIALOG_WIDTH wide
const int LAYOUT_MIN_WIDTH = 0xf0;

// The room around the page: left, top, right and bottom
const Insets CONTENT_INSETS(100, 100, 100, 50);

// Where the question is written
const int QUESTION_Y = 0x50;

// Where the gnome's picture is drawn
const double GNOME_X = 350.0;
const double GNOME_Y = 240.0;

// The rows of the gnome's picture: row 0 is played once (GNOME_ONCE_CELS cels), row 1 goes round (GNOME_LOOP_CELS cels), and
// row 2, which shows while the mouse is over "No", runs up to GNOME_LAST_CEL and stays there
const int GNOME_ROW_ONCE = 0;
const int GNOME_ROW_LOOP = 1;
const int GNOME_ROW_NO = 2;
const double GNOME_ONCE_CELS = 6.0;
const int GNOME_LOOP_CELS = 20;
const double GNOME_LAST_CEL = 13.0;

// How fast the gnome's cels go by in each row, for each tick (theFrac)
const double GNOME_ONCE_SPEED = 0.1;
const double GNOME_LOOP_SPEED = 0.2;

// The hammer swings along the dialog's time taken at this rate: it sways sideways with the sine of SWAY_RATE of that and bobs
// up and down with the cosine of BOB_RATE of it
const double SWING_RATE = 0.4;
const double SWAY_RATE = 0.05;
const double BOB_RATE = 0.11;

// The hammer of the 2D mode: it hangs at this place, and sways and bobs this far
const float HAMMER_X = 160.0f;
const float HAMMER_Y = 55.0f;
const float HAMMER_SWAY = 5.0f;
const float HAMMER_BOB = 10.0f;

// The hammer of the 3D mode is drawn stretched about its middle, by up to HAMMER_STRETCH of its size as it sways. Where it is
// not stretched its corner is at (HAMMER_3D_X, HAMMER_3D_Y) in the box; it bobs HAMMER_3D_BOB around HAMMER_3D_BOB_BASE down
// from there, and shifts HAMMER_3D_SWAY sideways at most
const double HAMMER_STRETCH = 0.1;
const double HAMMER_3D_BOB = 10.0;
const double HAMMER_3D_BOB_BASE = 20.0;
const double HAMMER_3D_SWAY = -5.0;
const int HAMMER_3D_X = 0xa0;
const int HAMMER_3D_Y = 0x2d;

// 0x45c390: the wipe is over, and the game is to end
void WipeQuit()
{
	gApp->mQuitting = true;
}

}

// 0x45c560
QuitDialog::QuitDialog() :
	NcMovingDialog(DIALOG_HEIGHT, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_QUIT, true, "", "", "", Dialog::BUTTONS_YES_NO)
{
	mGnomeCel = 0.0;
	mGnomeRow = GNOME_ROW_ONCE;
	mSwing = 0.0;

	SetButtonFont(FONT_ARIAL10);
	mYesButton->mLabel = "Yes";
	mNoButton->mLabel = "No";
	mContentInsets = CONTENT_INSETS;
}

// 0x45c3c0: the hammer swings, and the gnome waves; when the mouse is over "No" he goes to the row that shows it, and he goes
// back to the first when it is over "Yes"
void QuitDialog::UpdateF(float theFrac)
{
	NcMovingDialog::UpdateF(theFrac);

	mSwing = theFrac + mSwing;

	if (!mYesButton->mIsOver)
	{
		if (mNoButton->mIsOver)
		{
			if (mGnomeRow != GNOME_ROW_NO)
				mGnomeCel = 0.0;
			mGnomeRow = GNOME_ROW_NO;
		}
	}
	else if (mGnomeRow == GNOME_ROW_NO)
	{
		mGnomeRow = GNOME_ROW_ONCE;
		mGnomeCel = 0.0;
	}

	if (mGnomeRow == GNOME_ROW_ONCE)
	{
		mGnomeCel = theFrac * GNOME_ONCE_SPEED + mGnomeCel;
		if (GNOME_ONCE_CELS <= mGnomeCel)
		{
			mGnomeRow = GNOME_ROW_LOOP;
			mGnomeCel = mGnomeCel - GNOME_ONCE_CELS;
		}
	}
	else if (mGnomeRow == GNOME_ROW_LOOP)
	{
		mGnomeCel = theFrac * GNOME_LOOP_SPEED + mGnomeCel;
	}
	else if (mGnomeRow == GNOME_ROW_NO)
	{
		double aCel = theFrac * GNOME_LOOP_SPEED + mGnomeCel;
		if (GNOME_LAST_CEL < aCel)
			aCel = GNOME_LAST_CEL;
		mGnomeCel = aCel;
	}
}

// 0x45c530
int QuitDialog::GetPreferredHeight(int theWidth)
{
	return DIALOG_HEIGHT;
}

// 0x44fe50: either button takes the box away, answering with its own id (the application is not told at the press, as the moving
// dialog's own answer would)
void QuitDialog::ButtonDepress(int theId)
{
	if (theId == Dialog::ID_YES)
		CloseWith(Dialog::ID_YES);
	else if (theId == Dialog::ID_NO)
		CloseWith(Dialog::ID_NO);
}

// 0x45c4e0: "Yes" ends the game after a wipe, "No" only takes the box away
void QuitDialog::Closed()
{
	if (mCloseResult == Dialog::ID_YES)
	{
		gApp->KillDialog(this);
		gApp->DoZoomWipe(WipeQuit, WIPE_HAMMER);
	}
	else if (mCloseResult == Dialog::ID_NO)
	{
		gApp->KillDialog(this);
	}
}

// 0x45c7f0: the box is drawn a second time over the first, then the question, the hammer and the gnome. The hammer swings on its
// own in the 2D mode; in the 3D mode it is stretched (a little more and less) as it swings.
void QuitDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	Rect aBoxRect(mBackgroundInsets.mLeft, mBackgroundInsets.mTop, mWidth - mBackgroundInsets.mRight - mBackgroundInsets.mLeft,
		mHeight - mBackgroundInsets.mBottom - mBackgroundInsets.mTop);
	g->DrawImageBox(aBoxRect, mComponentImage);

	g->SetFont(FONT_PTBANANASPLIT27);
	g->SetColor(Color(255, 255, 255, 255));
	WriteCenteredLine(g, QUESTION_Y, "Do you really want to quit?");

	double aTime = mSwing * SWING_RATE;
	if (!gApp->Is3DAccelerated())
	{
		g->DrawImageF(IMAGE_EXIT_HAMMER, (float) (sin(aTime * SWAY_RATE) * HAMMER_SWAY + HAMMER_X),
			(float) (cos(aTime * BOB_RATE) * HAMMER_BOB + HAMMER_Y));
	}
	else
	{
		double aSwayAngle = aTime * SWAY_RATE;
		double aStretch = (cos(aSwayAngle) + 1.0) * 0.5 * HAMMER_STRETCH + 1.0;
		int aImageWidth = IMAGE_EXIT_HAMMER->GetWidth();
		int aImageHeight = IMAGE_EXIT_HAMMER->GetHeight();
		int aWidth = ToInt(aImageWidth * aStretch);
		int aHeight = ToInt(aImageHeight * aStretch);
		int aBob = ToInt(cos(aTime * BOB_RATE) * HAMMER_3D_BOB + HAMMER_3D_BOB_BASE);
		int aY = aBob + aImageHeight / 2 - aHeight / 2 + HAMMER_3D_Y;
		int aSway = ToInt(sin(aSwayAngle) * HAMMER_3D_SWAY);
		int aX = aImageWidth / 2 - aWidth / 2 - aSway + HAMMER_3D_X;
		g->DrawImage(IMAGE_EXIT_HAMMER, aX, aY, aWidth, aHeight);
	}

	int aGnomeCel = ToInt(mGnomeCel);
	if (mGnomeRow == GNOME_ROW_LOOP)
		aGnomeCel = aGnomeCel % GNOME_LOOP_CELS;
	DrawImageCelF(g, IMAGE_GNOME_BYEBYE, GNOME_X, GNOME_Y, aGnomeCel, mGnomeRow);
}

// 0x45c730
QuitDialog* Sexy::MakeQuitDialog()
{
	QuitDialog* aDialog = new QuitDialog();
	LayoutDialog(aDialog, LAYOUT_MIN_WIDTH);
	int aHeight = aDialog->GetPreferredHeight(DIALOG_WIDTH);
	aDialog->Resize((gApp->mWidth - DIALOG_WIDTH) / 2, -aHeight, DIALOG_WIDTH, aHeight);
	gApp->AddDialog(DIALOG_QUIT, aDialog);
	return aDialog;
}
