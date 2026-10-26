#include "MainMenu.h"
#include "PixelusApp.h"
#include "GameDialog.h"
#include "GameText.h"
#include "UserDirectory.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "Color.h"
#include <ctype.h>
#include <stdio.h>

using namespace Sexy;

static std::string ToUpper(std::string theString)	// __strupr
{
	for (size_t i = 0; i < theString.size(); i++)
		theString[i] = (char) toupper((unsigned char) theString[i]);
	return theString;
}

// 0x40ac30 with the base 0x4e60e0
MainMenu::MainMenu(PixelusApp* theApp)
{
	mApp = theApp;
	mTooltip = NULL;
	mTooltipAlpha = 0xff;
	mFaceCurrent = -1;
	mFaceTarget = -1;
	mFaceAlpha = 0;

	// 0x40a7d0: the four image buttons; the positions are those of 0x40a3b0
	mPlayButton = MakeStripButton(mApp, this, ID_PLAY, IMAGE_GAMESELECTORBUTTON_PLAY, 0x176, 0x40);
	mOptionsButton = MakeStripButton(mApp, this, ID_OPTIONS, IMAGE_GAMESELECTORBUTTON_OPTIONS, 0x1b3, 0xea);
	mMoreGamesButton = MakeStripButton(mApp, this, ID_MORE_GAMES, IMAGE_GAMESELECTORBUTTON_MOREGAMES, 0x194, 0x154);
	mQuitButton = MakeStripButton(mApp, this, ID_QUIT, IMAGE_GAMESELECTORBUTTON_QUIT, 0x194, 399);
	DialogButton* aButtons[4] = {mPlayButton, mOptionsButton, mMoreGamesButton, mQuitButton};
	for (int i = 0; i < 4; i++)
	{
		aButtons[i]->mOverAlphaSpeed = 0.08;
		aButtons[i]->mOverAlphaFadeInSpeed = 0.04;
	}

	mUserLink = new HyperlinkWidget(ID_USER_LINK, this);
	mUserLink->SetFont(mApp->mFonts[FONT_PRAITOR24]);
	mUserLink->mColor = Color(255, 255, 255);
	mUserLink->mOverColor = Color(0xff, 0xf2, 0x5c);
	mUserLink->mUnderlineSize = 0;
	mUserLink->mLabel = GT(0x50c7a8);
	int aWidth = mApp->mFonts[FONT_PRAITOR24]->StringWidth(mUserLink->mLabel) + 10;
	mUserLink->Resize(0xb4 - aWidth / 2, 0x46, aWidth, 0x1e);

	// the animations: the hair (three frames at (0xb2, 0xd3), 0.2 s each), the sea (0x6e, 0x17d), and the sparkles
	mEffects.Add(new FrameEffect(mApp->mImages[IMAGE_HAIR], 3, 0xb2, 0xd3, 0.2, 0.2));
	mEffects.Add(new FrameEffect(mApp->mImages[IMAGE_SEA], 3, 0x6e, 0x17d, 1.0, 1.0));
	Image* aSpark = mApp->mImages[IMAGE_SPARK];
	int aHalf = aSpark->GetHeight() / 2;
	static const int aSparkPlaces[4][2] = {{0x26b, 0x49}, {0x171, 0x133}, {0x1a3, 0xdc}, {0x196, 0xc0}};
	for (int i = 0; i < 4; i++)
		mEffects.Add(new SparkleEffect(aSpark, 6, aSparkPlaces[i][0] - aHalf, aSparkPlaces[i][1] - aHalf, 2.0, 8.0, 0.1, 0.1));
	Image* aSmall = mApp->mImages[IMAGE_SPARKSMALL];
	aHalf = aSmall->GetHeight() / 2;
	static const int aSmallPlaces[2][2] = {{0x176, 0x9c}, {0x171, 0x14d}};
	for (int i = 0; i < 2; i++)
		mEffects.Add(new SparkleEffect(aSmall, 6, aSmallPlaces[i][0] - aHalf, aSmallPlaces[i][1] - aHalf, 2.0, 8.0, 0.1, 0.1));
}

MainMenu::~MainMenu()
{
	delete mPlayButton;
	delete mOptionsButton;
	delete mMoreGamesButton;
	delete mQuitButton;
	delete mUserLink;
}

// 0x40a3b0 (+ 0x4e5920)
void MainMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayButton);
	theWidgetManager->AddWidget(mMoreGamesButton);
	theWidgetManager->AddWidget(mOptionsButton);
	theWidgetManager->AddWidget(mQuitButton);
	theWidgetManager->AddWidget(mUserLink);
	UserChanged();
}

// 0x40a440 (+ 0x4e5990)
void MainMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayButton);
	theWidgetManager->RemoveWidget(mMoreGamesButton);
	theWidgetManager->RemoveWidget(mOptionsButton);
	theWidgetManager->RemoveWidget(mQuitButton);
	theWidgetManager->RemoveWidget(mUserLink);
}

// the link only makes sense when a player is signed in
void MainMenu::UserChanged()
{
	mUserLink->SetVisible(mApp->mUserDirectory->GetCurrent() != NULL);
	MarkDirty();
}

// 0x40a490: the buttons; the first four ids go to the game's own handlers (the base's list of buttons is empty)
void MainMenu::ButtonDepress(int theId)
{
	switch (theId)
	{
	case ID_USER_LINK:
		mApp->ShowUserSelect();
		break;
	case ID_PLAY:
		mApp->PlayGame();
		break;
	case ID_MORE_GAMES:
		mApp->ShowMoreGames();
		break;
	case ID_OPTIONS:
		mApp->ShowOptions();
		break;
	case ID_QUIT:
		mApp->ShowQuitDialog();
		break;
	}
}

// 0x4e5770: the tooltip of the button under the mouse comes in over a few updates and starts again when the mouse moves
// to another button
void MainMenu::UpdateTooltip()
{
	if (mTooltipAlpha < 0xff)
	{
		mTooltipAlpha += 10;
		if (mTooltipAlpha > 0xff)
			mTooltipAlpha = 0xff;
		MarkDirty();
	}

	const std::string* aText = NULL;
	if (mPlayButton->mIsOver)
		aText = &GT(0x50c838);
	else if (mMoreGamesButton->mIsOver)
		aText = &GT(0x50c7f0);
	else if (mOptionsButton->mIsOver)
		aText = &GT(0x50c81c);
	else if (mQuitButton->mIsOver)
		aText = &GT(0x50c7cc);

	if (aText != mTooltip)
	{
		mTooltip = aText;
		mTooltipAlpha = 0;
		MarkDirty();
	}
}

// 0x40a4b0
void MainMenu::Update()
{
	Widget::Update();
	UpdateTooltip();
	mEffects.Update(0.01);
	MarkDirty();

	// the face: Quit frowns, the other three buttons smile; the old face fades out before the new one fades in
	mFaceTarget = -1;
	if (mQuitButton->mIsOver)
		mFaceTarget = IMAGE_FROWN;
	else if (mMoreGamesButton->mIsOver || mOptionsButton->mIsOver || mPlayButton->mIsOver)
		mFaceTarget = IMAGE_SMILE;

	if (mFaceTarget != mFaceCurrent)
	{
		if (mFaceAlpha == 0)
			mFaceCurrent = mFaceTarget;
		if (mFaceTarget != mFaceCurrent)
		{
			mFaceAlpha -= 0.05;
			if (mFaceAlpha < 0)
				mFaceAlpha = 0;
			return;
		}
	}
	mFaceAlpha += 0.03;
	if (mFaceAlpha > 1.0)
		mFaceAlpha = 1.0;
}

// 0x4e5da0: the grey strip with the text, feathered at both ends
void MainMenu::DrawTooltip(Graphics* g)
{
	if (mApp->GetDialog(9) != NULL || mApp->GetDialog(8) != NULL)
		return;
	if (mTooltip == NULL || mTooltip->empty())
		return;

	Font* aFont = mApp->mFonts[FONT_PRAITOR24];
	g->SetFont(aFont);
	g->SetColorizeImages(true);
	int aWidth = g->StringWidth(*mTooltip);
	int aDescent = aFont->GetDescent();
	int aHeight = aFont->GetHeight();
	int aTop = TOOLTIP_BASELINE - (aHeight - aDescent) - 4;
	int aBoxHeight = aHeight + 8;
	int aHalf = (aWidth + 0x20) / 2;

	g->SetColor(Color(0x7f, 0x7f, 0x7f, mTooltipAlpha / 2));
	g->FillRect(0x140 - aHalf, aTop, aWidth + 0x20, aBoxHeight);
	int anAlpha = mTooltipAlpha / 2;
	int aLeft = 0x13e - aHalf;
	int aRight = 0x140 + aHalf;
	while (anAlpha > 0)
	{
		g->SetColor(Color(0x7f, 0x7f, 0x7f, anAlpha));
		g->FillRect(aLeft, aTop, 2, aBoxHeight);
		g->FillRect(aRight, aTop, 2, aBoxHeight);
		anAlpha -= 4;
		aLeft -= 2;
		aRight += 2;
	}

	g->SetColor(Color(0, 0, 0, mTooltipAlpha));
	g->DrawString(*mTooltip, 0x141 - aWidth / 2, TOOLTIP_BASELINE + 1);
	g->SetColor(Color(0xff, 0xf2, 0x5c, mTooltipAlpha));
	g->DrawString(*mTooltip, 0x140 - aWidth / 2, TOOLTIP_BASELINE);
	g->SetColorizeImages(false);
}

// 0x40a8b0
void MainMenu::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_GAMESELECT], 0, 0);
	DrawTooltip(g);
	mEffects.Draw(g);

	if ((mFaceCurrent != -1) && (mFaceAlpha > 0))
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) (mFaceAlpha * 255)));
		g->DrawImage(mApp->mImages[mFaceCurrent], FACE_X, FACE_Y);
		g->SetColorizeImages(false);
	}

	// "Welcome to Pixelus," above the player's name, or "Welcome to Pixelus!" when nobody is signed in
	g->SetFont(mApp->mFonts[FONT_PRAITOR36]);
	g->SetColor(Color(255, 255, 255));
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	char aBuffer[128];
	sprintf(aBuffer, GTC(0x50c764), mApp->mProdName.c_str(), aUser != NULL ? "," : "!");
	std::string aWelcome = ToUpper(aBuffer);
	g->DrawString(aWelcome, 0xb4 - g->StringWidth(aWelcome) / 2, 0x28);
	if (aUser != NULL)
	{
		std::string aName = ToUpper(aUser->mName + "!");
		g->DrawString(aName, 0xb4 - g->StringWidth(aName) / 2, 0x46);
	}
}
