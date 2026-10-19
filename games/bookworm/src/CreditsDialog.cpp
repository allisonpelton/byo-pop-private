#include "CreditsDialog.h"
#include "BookWormApp.h"
#include "Res.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"

using namespace Sexy;

// The six headings and their names (hard-coded in the exe, in this order)
static const int NUM_CREDITS = 6;
static const GameTextId gCredits[NUM_CREDITS][2] =
{
	{ GT_CREDITS_DESIGN, GT_CREDITS_DESIGN_NAMES },
	{ GT_CREDITS_PROGRAMMING, GT_CREDITS_PROGRAMMING_NAMES },
	{ GT_CREDITS_ART, GT_CREDITS_ART_NAMES },
	{ GT_CREDITS_MUSIC, GT_CREDITS_MUSIC_NAMES },
	{ GT_CREDITS_BUSINESS, GT_CREDITS_BUSINESS_NAMES },
	{ GT_CREDITS_TESTING, GT_CREDITS_TESTING_NAMES }
};

// 0x41c0b0: one footer button (GT_CREDITS_DONE, id 2016; the app's ButtonDepress kills the dialog)
CreditsDialog::CreditsDialog(BookWormApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_CREDITS, true,
		_S(""), _S(""), GameText(GT_CREDITS_DONE), BUTTONS_FOOTER)
{
	mApp = theApp;
	mContentInsets = Insets(36, 36, 36, 36);
	SetButtonFont(mApp->mFonts[FONT_VERDANA10BOLD]);
	// port: the 2004 Dialog::Draw gives a dialog without header/lines fonts SysFont defaults; the port has
	// no SysFont and Dialog2004 draws with both. The header and lines are empty, so nothing is drawn with
	// them: they get the button font.
	SetHeaderFont(mApp->mFonts[FONT_VERDANA10BOLD]);
	SetLinesFont(mApp->mFonts[FONT_VERDANA10BOLD]);
	mCloudRot = 0.0;
	mCloudX = 0.0;
	mCloudY = 0.0;
	mFont = mApp->mFonts[FONT_LEGACYSERIFMDSCITCTT12];
	// BookWormApp::ShowCredits sets the colours, Resize(0, 47, 388, 396), Move((mWidth - 388 + 233)/2, 47)
	// and adds it as dialog 16.
}

// 0x41bfc0 (the Dialog destructor)
CreditsDialog::~CreditsDialog()
{
}

// 0x41bfe0
int CreditsDialog::GetPreferredHeight(int theWidth)
{
	return PREFERRED_HEIGHT;
}

// 0x41bff0
void CreditsDialog::Update()
{
	Dialog2004::Update();

	mCloudRot += 0.004;

	mCloudX += 0.2;
	if (mCloudX > 0.0)
		mCloudX -= 512.0;

	mCloudY += 0.3;
	if (mCloudY > 0.0)
		mCloudY -= 512.0;

	MarkDirty();
}

// 0x41c2a0
void CreditsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);

	// A gold panel inside the frame with two additive cloud layers scrolling across and down, a rotating
	// normal layer on top, then a 6-pixel dark inner shadow (the right and bottom edges of the outer rings
	// fall outside the clip).
	Image* aClouds = mApp->mImages[IMAGE_CLOUDS];
	Rect anArea(39, 34, mWidth - 78, mHeight - 110);

	Graphics aCloudG(*g);
	aCloudG.ClipRect(anArea);
	aCloudG.SetColor(Color(210, 170, 15));
	aCloudG.FillRect(0, 0, mWidth, mHeight);
	aCloudG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aCloudG.DrawImage(aClouds, (int)mCloudX, 0);
	aCloudG.DrawImage(aClouds, (int)mCloudX + 512, 0);
	aCloudG.DrawImage(aClouds, 0, (int)mCloudY);
	aCloudG.DrawImage(aClouds, 0, (int)mCloudY + 512);
	aCloudG.SetDrawMode(Graphics::DRAWMODE_NORMAL);
	aCloudG.DrawImageRotated(aClouds, -64, -64, mCloudRot);

	int anInset = 0;
	int anInset2 = 0;
	for (int anAlpha = 1530; anAlpha > 0; anAlpha -= 255)
	{
		aCloudG.SetColor(Color(48, 48, 48, anAlpha / 7));
		aCloudG.DrawRect(anInset + anArea.mX, anArea.mY + anInset, anArea.mWidth - anInset2, anArea.mHeight - anInset2);
		anInset++;
		anInset2 += 2;
	}

	// Six headings with their names (hard-coded in the exe), centred, black, unclipped
	g->SetFont(mFont);
	g->SetColor(Color(0, 0, 0));
	int aHeight = g->GetFont()->GetHeight();
	int aGap = (int)(aHeight * 1.5);

	// Each heading, its names one line below, the next heading one and a half lines further
	int aY = 60;
	for (int i = 0; i < NUM_CREDITS; i++)
	{
		if (i > 0)
			aY += aGap;
		WriteCenteredLine(g, aY, GameText(gCredits[i][0]));
		aY += aHeight;
		WriteCenteredLine(g, aY, GameText(gCredits[i][1]));
	}
}
