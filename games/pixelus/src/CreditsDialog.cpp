#include "CreditsDialog.h"
#include "PixelusApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "Compat2004.h"

using namespace Sexy;

// 0x402290 and 0x4e5580: the dialog has no header, only the line "DONE" at the foot
CreditsDialog::CreditsDialog(PixelusApp* theApp) :
	GameDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], DIALOG_ID, true,
		"", "", GT(0x5284c8), BUTTONS_FOOTER, NULL)
{
}

// 0x4e4ce0
int CreditsDialog::GetPreferredHeight(int theWidth)
{
	return PREFERRED_HEIGHT;
}

// 0x4e4f70: centred lines, every one at a place of its own; offsets from the dialog's top
void CreditsDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	const int aTop = 0x24;
	g->SetColor(Color(255, 255, 255, 255));

	struct Line { unsigned int mText; int mY; int mFont; };
	static const Line aLines[] =
	{
		{0x5284b8, aTop + 0x14, FONT_GARAMOND12},	// "Developed by"
		{0x50b73c, aTop + 0x2a, FONT_GARAMOND12},
		{0x50b714, aTop + 0x40, FONT_GARAMOND12},
		{0x50b700, aTop + 0x56, FONT_GARAMOND12},
		{0x5284a4, aTop + 0x6c, FONT_GARAMOND12},	// "for Nuclide.com."
		{0x528494, aTop + 0x94, FONT_GARAMOND12},	// "Many thanks to"
		{0x50b6d4, aTop + 0xaa, FONT_GARAMOND12},
		{0x50b6b4, aTop + 0xc0, FONT_GARAMOND12},
		{0x50b698, aTop + 0xd6, FONT_GARAMOND12},	// "and all others at PopCap."
		{0x50b664, aTop + 0xfe, FONT_GARAMOND12}	// the beta testers
	};
	for (size_t i = 0; i < sizeof(aLines) / sizeof(aLines[0]); i++)
	{
		g->SetFont(mApp->mFonts[aLines[i].mFont]);
		WriteString2004(g, GT(aLines[i].mText), 0, aLines[i].mY, mWidth, 0);
	}
}
