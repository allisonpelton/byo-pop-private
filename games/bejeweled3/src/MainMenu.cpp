// Sexy::MainMenu: see the header.
#include "MainMenu.h"

#include "Bej3App.h"
#include "EncodingParser.h"
#include "Font.h"
#include "Graphics.h"
#include "Profile.h"
#include "Res.h"

using namespace Sexy;

namespace {

// 0x475e30: the text of a numbered string in default.xml (its ids are the numbers 0x11d and up, in decimal there)
std::string GetTextById(int theId, const std::string& theDefault)
{
	return gApp->GetString(std::to_string(theId), theDefault);
}

}  // namespace

// port: the button stands in for the crystal ball (see the header)
MainMenuButton::MainMenuButton(const std::string& theText) : mText(theText)
{
}

// port: the button stands in for the crystal ball (see the header)
void MainMenuButton::Draw(Graphics* g)
{
	// port: the crystal ball's picture is not translated: a plate and the text
	g->SetColor(mHighlighted ? Color(255, 255, 255, 120) : Color(0, 0, 0, 120));
	g->FillRect(0, 0, mWidth, mHeight);
	if (FONT_PLAYBUTTONS != nullptr) {
		g->SetFont(FONT_PLAYBUTTONS);
		g->SetColor(mHighlighted ? Color(255, 255, 160) : Color(255, 255, 255));
		g->DrawString(mText, (mWidth - FONT_PLAYBUTTONS->StringWidth(mText)) / 2,
			(mHeight + FONT_PLAYBUTTONS->GetAscent()) / 2);
	}
}

// port: the button stands in for the crystal ball (see the header)
void MainMenuButton::MouseEnter()
{
	mHighlighted = true;
	MarkDirty();
}

// port: the button stands in for the crystal ball (see the header)
void MainMenuButton::MouseLeave()
{
	mHighlighted = false;
	MarkDirty();
}

MainMenu::MainMenu()
{
	// 0x6a54b0: the buttons, made in the order OPTIONS, RECORDS, PLAY; the texts are string ids 0x11d, 0x11e, 0x11f
	// with the English words as their default
	mOptionsButton = new MainMenuButton(GetTextById(0x11d, "OPTIONS"));
	mRecordsButton = new MainMenuButton(GetTextById(0x11e, "RECORDS"));
	mPlayButton = new MainMenuButton(GetTextById(0x11f, "PLAY"));

	// port: the original puts the buttons where its curves take them (0x6a3530 and 0x6a09e0, with the screen's size);
	// here they stand at the lower middle of the 800 by 600 screen's 2D picture, in the art size's proportion
	int anArtRes = gApp->mArtRes;
	int aMiddleX = anArtRes * 960 / 1200;   // the menu is 1920 wide at 1200 and starts 160 left of the screen: its middle is 960
	int aButtonWidth = anArtRes * 300 / 600;
	int aButtonHeight = anArtRes * 60 / 600;
	mPlayButton->Resize(aMiddleX - aButtonWidth / 2, anArtRes * 290 / 600, aButtonWidth, aButtonHeight);
	mOptionsButton->Resize(aMiddleX - aButtonWidth * 2 / 3 - 10, anArtRes * 370 / 600, aButtonWidth * 2 / 3, aButtonHeight * 2 / 3);
	mRecordsButton->Resize(aMiddleX + 10, anArtRes * 370 / 600, aButtonWidth * 2 / 3, aButtonHeight * 2 / 3);
	AddWidget(mOptionsButton);
	AddWidget(mRecordsButton);
	AddWidget(mPlayButton);
}

// port: the button stands in for the crystal ball (see the header)
MainMenu::~MainMenu()
{
	RemoveAllWidgets(true, true);
}

void MainMenu::Draw(Graphics* g)
{
	// 0x6a2a20 makes the 2D mode's background (images\NonResize\2donly\mainmenu\background) 1920 wide at 1200,
	// stretched over the menu; 0x6a0ca0 draws it first
	if (IMAGE_2DONLY_MAINMENU_BACKGROUND != nullptr)
		g->DrawImage(IMAGE_2DONLY_MAINMENU_BACKGROUND, 0, 0, mWidth, mHeight);
	// port: the foreground picture (images\NonResize\2donly\mainmenu\foreground), the logos, the particles and the tips

	// 0x6a0ca0: the loading line at y = art size * 0x352 / 1200 (string id 0x122, "Loading..."), while the loading thread runs
	if (!gApp->mLoadingThreadCompleted && FONT_PLAYBUTTONS != nullptr) {
		std::string aText = GetTextById(0x122, "Loading...");
		g->SetFont(FONT_PLAYBUTTONS);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(aText, (mWidth - FONT_PLAYBUTTONS->StringWidth(aText)) / 2, gApp->mArtRes * 0x352 / 1200);
	}
	// the player's name and "Click here to change user" (string id 0x120)
	if (gApp->mProfile != nullptr && !gApp->mProfile->mName.empty() && FONT_PLAYBUTTONS != nullptr) {
		g->SetFont(FONT_PLAYBUTTONS);
		g->SetColor(Color(255, 255, 255));
		std::string aName;
		for (wchar_t aChar : gApp->mProfile->mName)
			aName = AppendUTF8(aName, aChar);
		g->DrawString(aName, 10, mHeight - 10);
	}
}

void MainMenu::Update()
{
	Widget::Update();
	// port: 0x6a3530 runs the curves, shows the buttons once the loading thread ends, and opens the user dialog
	// when there is no player; the buttons are shown only after loading
	bool aShown = gApp->mLoadingThreadCompleted;
	mPlayButton->mVisible = aShown;
	mOptionsButton->mVisible = aShown;
	mRecordsButton->mVisible = aShown;
	MarkDirty();
}
