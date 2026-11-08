#include "HighScoreScreen.h"
#include "UserProfileMgr.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include "Common.h"
#include "compat2004/Compat2004.h"
#include "Res.h"

using namespace Sexy;

// The constructor is the inline part of WinWSApp::ShowHighScoreScreen (0x41bbd0, WinWSAppScreens.cpp): Widget, then the
// user manager (+0x58) and the app (+0x5c). The original also clears the flag at +0x60 (the registration prompt, not ported)
HighScoreScreen::HighScoreScreen(WinWSApp* theApp)
{
	mProfileMgr = theApp->mProfileMgr;
	mApp = theApp;
}

// 0x4470c0 slot 17 not ported: Widget::Update, then for a game that is not registered (+0x470 of the app clear) and with
// the flag at +0x60 clear: the flag is set and the register dialog 0x41c1c0 is shown with the text 0x52f948. The port is
// registered, so Widget::Update is all that is left

// 0x447140: see the header. The sample text is built again for each test in the original, which is why it is read again
// here; the font that measures the name is the one passed in, the one that measures the sample is always the app's Rockwell 13
std::string HighScoreScreen::FitName(const std::string& theName, Font* theFont)
{
	if (theFont->StringWidth(theName) <= mApp->mFontRockwell13->StringWidth(GT(0x0052f97c)))
		return theName;

	std::string aResult = "";
	unsigned int i = 0;
	while (true)
	{
		if (!(theFont->StringWidth(aResult) < mApp->mFontRockwell13->StringWidth(GT(0x0052f97c))))
			break;
		if (i >= theName.size())
			break;
		aResult.append(1, theName[i]);
		i++;
	}

	aResult.erase(aResult.size() - 1, 1);
	aResult.append(GT(0x0052c310));
	return aResult;
}

// 0x450110 slot 16: the picture, then ten lines in Rockwell 13. Each line is drawn from the adventure table (name in black
// at x = 0x31, the score in blue-grey right-aligned in 0..0x108, the mode label in gold centred in 0x119..0x13f) and then
// from the abyss table (name in black at x = 0x185, the depth in brown right-aligned in 0..0x245). The mode label is a
// string that lives from line to line: a mode other than 0 to 4 repeats the label of the line before (the original does
// that too). The abyss lines have no mode label
void HighScoreScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_HIGH_SCORE_BG], 0, 0);
	g->SetFont(mApp->mFontRockwell13);
	g->SetColor(Color(0xff, 0, 0));

	std::string aText = "";
	std::string aModeText = "";
	int i = 0;
	do
	{
		HighScoreEntry aEntry = mProfileMgr->GetHighScore(false, i);	// 0x41ac10
		if (aEntry.mMode == 0)
			aModeText = GT(0x0052d280);
		else
		{
			if (aEntry.mMode == 1)
				aModeText = GT(0x005306f0);
			if (aEntry.mMode == 2)
				aModeText = GT(0x0052d270);
			if (aEntry.mMode == 3)
				aModeText = GT(0x005306ec);
			if (aEntry.mMode == 4)
				aModeText = GT(0x005306e8);
		}

		g->SetColor(Color(0, 0, 0));
		aText = FitName(aEntry.mName, mApp->mFontRockwell13);
		g->DrawString(aText, 0x31, g->GetFont()->GetHeight() * i + 0xa5);

		aText = StrFormat(GTC(0x0052b19c), aEntry.mScore);
		g->SetColor(Color(0x66, 0x8b, 0x9f));
		WriteString2004(g, aText, 0, g->GetFont()->GetHeight() * i + 0xa5, 0x108, 1, true, 0, -1);

		g->SetColor(Color(0xb2, 0x9e, 0x58));
		WriteString2004(g, aModeText, 0x119, g->GetFont()->GetHeight() * i + 0xa5, 0x26, 0, true, 0, -1);

		aEntry = mProfileMgr->GetHighScore(true, i);
		g->SetColor(Color(0, 0, 0));
		aText = FitName(aEntry.mName, mApp->mFontRockwell13);
		g->DrawString(aText, 0x185, g->GetFont()->GetHeight() * i + 0xa5);

		aText = StrFormat(GTC(0x0052b19c), aEntry.mScore);
		g->SetColor(Color(0xb5, 0x6b, 0x40));
		WriteString2004(g, aText, 0, g->GetFont()->GetHeight() * i + 0xa5, 0x245, 1, true, 0, -1);

		i++;
	} while (i < 10);
}
