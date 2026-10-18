#include "CreditsDialog.h"
#include "WinBMApp.h"
#include "Res.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"

using namespace Sexy;

// 0x40ef30
CreditsDialog::CreditsDialog(WinBMApp* theApp) :
	Dialog(theApp->mImages[IMAGE_OLDDIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_CREDITS, true,
		_S(""), _S(""), theApp->GetString("DONE"), BUTTONS_FOOTER)
{
	mApp = theApp;
	mContentInsets = Insets(36, 36, 36, 36);
	SetButtonFont(mApp->mFont3);
	mCloudRot = 0.0;
	mCloudX = 0.0;
	mCloudY = 0.0;
	mUnused14C = NULL;
	// WinBMApp::ShowCreditsDialog sets the colours, Resize(196, 16, 388, GetPreferredHeight(388)) and adds it.
}

// 0x40ee10
CreditsDialog::~CreditsDialog()
{
	delete mUnused14C;
}

// 0x40ee80
int CreditsDialog::GetPreferredHeight(int theWidth)
{
	return PREFERRED_HEIGHT;
}

// 0x40ee90
void CreditsDialog::Update()
{
	Dialog::Update();

	mCloudRot += 0.004;

	mCloudX += 0.2;
	if (mCloudX > 0.0)
		mCloudX -= 512.0;

	mCloudY += 0.3;
	if (mCloudY > 0.0)
		mCloudY -= 512.0;

	MarkDirty();
}

// 0x40f190
void CreditsDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	// Clouds inside the frame: two additive layers scrolling across and down, a rotating normal layer on
	// top, then a 6-pixel dark inner shadow.
	Image* aClouds = mApp->mImages[IMAGE_CLOUDS];
	int aClipX = 39;
	int aClipY = 34;
	int aClipWidth = mWidth - 78;
	int aClipHeight = mHeight - 110;

	Graphics aCloudG(*g);
	aCloudG.ClipRect(aClipX, aClipY, aClipWidth, aClipHeight);
	aCloudG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aCloudG.DrawImage(aClouds, (int)mCloudX, 0);
	aCloudG.DrawImage(aClouds, (int)mCloudX + 512, 0);
	aCloudG.DrawImage(aClouds, 0, (int)mCloudY);
	aCloudG.DrawImage(aClouds, 0, (int)mCloudY + 512);
	aCloudG.SetDrawMode(Graphics::DRAWMODE_NORMAL);
	aCloudG.DrawImageRotated(aClouds, -64, -64, mCloudRot);

	int anInset = 0;
	for (int anAlpha = 1530; anAlpha > 0; anAlpha -= 255)
	{
		aCloudG.SetColor(Color(48, 48, 48, anAlpha / 7));
		aCloudG.DrawRect(anInset + aClipX, aClipY + anInset, aClipWidth - 2*anInset, aClipHeight - 2*anInset);
		anInset++;
	}

	// Headings green, names (hard-coded in the exe) yellow, centred.
	g->SetFont(mApp->mFont1OutlineNoShadow);
	g->SetColor(Color(0, 255, 0));
	g->SetColor(Color(0, 255, 0)); // set twice in the original
	WriteCenteredLine(g, 62, mApp->GetString("GAME_DESIGN"));
	g->SetColor(Color(229, 255, 5));
	WriteCenteredLine(g, 81, _S("Jason Kapalka"));

	g->SetColor(Color(0, 255, 0));
	WriteCenteredLine(g, 107, mApp->GetString("PROGRAMMING"));
	g->SetColor(Color(229, 255, 5));
	WriteCenteredLine(g, 126, _S("Brian Fiete"));

	g->SetColor(Color(0, 255, 0));
	WriteCenteredLine(g, 152, mApp->GetString("ART"));
	g->SetColor(Color(229, 255, 5));
	WriteCenteredLine(g, 171, _S("Josh Langley, Stephen Notley"));

	g->SetColor(Color(0, 255, 0));
	WriteCenteredLine(g, 197, mApp->GetString("MUSIC"));
	g->SetColor(Color(229, 255, 5));
	WriteCenteredLine(g, 216, _S("Skaven"));

	g->SetColor(Color(0, 255, 0));
	WriteCenteredLine(g, 242, mApp->GetString("BUSINESS"));
	g->SetColor(Color(229, 255, 5));
	WriteCenteredLine(g, 261, _S("John Vechey"));

	g->SetColor(Color(0, 255, 0));
	WriteCenteredLine(g, 287, mApp->GetString("VOICE_OF_MR_MONEYBAGS"));
	g->SetColor(Color(229, 255, 5));
	WriteCenteredLine(g, 306, _S("Roch Shannon Fraser"));
}
