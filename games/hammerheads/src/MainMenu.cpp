#include "MainMenu.h"

#include <cmath>
#include <string>

#include "Application.h"
#include "Color.h"
#include "Creature.h"
#include "Dialog.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "Image.h"
#include "NcButton.h"
#include "NcMovingDialog.h"
#include "Rect.h"
#include "Res.h"
#include "Screens.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The box that says a game mode is locked
const int DIALOG_MODE_LOCKED = 0x14;

// Every button's picture is three cels
const int NUM_CELS = 3;

// Where the buttons go (the left and the top of each), and how long each waits before it slides in (as the scale of the time)
const int PLAY_CLASSIC_X = 0x2a;
const int PLAY_CLASSIC_Y = 0xdd;
const int PLAY_TOUGH_X = 0x84;
const int PLAY_TOUGH_Y = 0xb4;
const double PLAY_TOUGH_DELAY = 0.5;
const int PLAY_MARATHON_X = 0x215;
const int PLAY_MARATHON_Y = 0xdd;
const int OPTIONS_Y = 0x1ba;
const double OPTIONS_DELAY = 1.0;
const int QUIT_Y = 0x1ba;
const double QUIT_DELAY = 1.0;
const int CORNER_Y = 0x1cf;
const int TROPHY_ROOM_X = 0x297;
const double CORNER_DELAY = 1.2;

// The options and the quit button sit next to each other under the middle (the original's third button was between them): this far
// from the left of each
const int OPTIONS_X = 0xeb;
const int QUIT_X = 0x196;

// The peekaboo gnome comes up from this far below the screen to this far above its bottom; it waits this long out of the screen
// (it starts so) and this long in it
const int PEEKABOO_START_BELOW = 300;
const int PEEKABOO_SHOWN_ABOVE = 0x5a;
const double PEEKABOO_HIDDEN_DELAY = 60.0;
const double PEEKABOO_SHOWN_DELAY = 1.0;

// How fast the buttons' waits run down and how fast they slide, for each frame of time
const double DELAY_RATE = 0.01;
const double SLIDE_RATE = 0.006;

// How much the text of the player's name fades in or out in an update, and the widest it is allowed to be
const int FADE_STEP = 10;
const int GREETING_MAX_WIDTH = 0xda;

// The text of the menu: where the lines of the greeting (and of the description of a button, which takes its place) are, and where
// the letters typed for the cheat words are
const int LINE_Y = 0x109;
const int LINE2_Y = 0x12c;
const int LINE3_Y = 0x14b;
const int LINE4_Y = 0x15f;
const int LINE5_Y = 0x173;
const int TYPED_X = 10;
const int TYPED_Y = 0x14;

// The link under the greeting
const int USER_LINK_Y = 0x136;
const int USER_LINK_PADDING = 10;

// 0x459270: the wipe to the high score table is over
void WipeToHighScores()
{
	gApp->ShowHighScoreTableFromMenu();
}

// 0x459260: the wipe to the trophy room is over
void WipeToTrophyRoom()
{
	gApp->ShowTrophyRoomFromMenu();
}

// Draws a line of text with its middle in the middle of the window
void DrawCenteredLine(Graphics* g, const std::string& theLine, int theY)
{
	int aHalfWidth = g->GetFont()->StringWidth(theLine) / 2;
	g->DrawString(theLine, gApp->mWidth / 2 - aHalfWidth, theY);
}

}

// 0x459a60
MainMenu::MainMenu()
{
	mTime = 0.0;
	mPeekabooButton = nullptr;

	gApp->ClearCheats();

	mPlayClassicButton = MakeNcButton(BUTTON_PLAY_CLASSIC, this, "", IMAGE_MAINMENU_PLAY1, NUM_CELS);
	AddSlide(mPlayClassicButton, -mPlayClassicButton->mWidth, PLAY_CLASSIC_Y, PLAY_CLASSIC_X, PLAY_CLASSIC_Y, 0.0);

	mPlayToughButton = MakeNcButton(BUTTON_PLAY_TOUGH, this, "", IMAGE_MAINMENU_PLAY1TOUGH, NUM_CELS);
	AddSlide(mPlayToughButton, -mPlayToughButton->mWidth, PLAY_TOUGH_Y, PLAY_TOUGH_X, PLAY_TOUGH_Y, PLAY_TOUGH_DELAY);
	mPlayToughButton->mVisible = gApp->IsToughUnlocked();

	Image* aMarathonImage = gApp->HasFewRestartPoints() ? IMAGE_MAINMENU_PLAY2LOCKED : IMAGE_MAINMENU_PLAY2;
	mPlayMarathonButton = MakeNcButton(BUTTON_PLAY_MARATHON, this, "", aMarathonImage, NUM_CELS);
	AddSlide(mPlayMarathonButton, gApp->mWidth, PLAY_MARATHON_Y, PLAY_MARATHON_X, PLAY_MARATHON_Y, 0.0);

	mOptionsButton = MakeNcButton(BUTTON_OPTIONS, this, "", IMAGE_MAINMENU_OPTIONS, NUM_CELS);
	AddSlide(mOptionsButton, -mOptionsButton->mWidth, OPTIONS_Y, OPTIONS_X, OPTIONS_Y, OPTIONS_DELAY);

	mQuitButton = MakeNcButton(BUTTON_QUIT, this, "", IMAGE_MAINMENU_QUIT, NUM_CELS);
	AddSlide(mQuitButton, gApp->mWidth, QUIT_Y, QUIT_X, QUIT_Y, QUIT_DELAY);

	mHighScoresButton = MakeNcButton(BUTTON_HIGH_SCORES, this, "", IMAGE_MAINMENU_HIGHSCORES, NUM_CELS);
	AddSlide(mHighScoresButton, -mHighScoresButton->mWidth, mHighScoresButton->mHeight + CORNER_Y, 0, CORNER_Y, CORNER_DELAY);

	mTrophyRoomButton = MakeNcButton(BUTTON_TROPHY_ROOM, this, "", IMAGE_MAINMENU_TROPHYROOM, NUM_CELS);
	AddSlide(mTrophyRoomButton, gApp->mWidth, mTrophyRoomButton->mHeight + CORNER_Y, TROPHY_ROOM_X, CORNER_Y, CORNER_DELAY);

	if (!gApp->GetCheatFlag(Application::CHEAT_PEEKABOO))
	{
		mPeekabooButton = MakeNcButton(BUTTON_PEEKABOO, this, "", IMAGE_MAINMENU_PEEKABOO, NUM_CELS);
		int aX = (gApp->mWidth - IMAGE_MAINMENU_PEEKABOO->mWidth / NUM_CELS) / 2;
		AddSlide(mPeekabooButton, aX, gApp->mHeight + PEEKABOO_START_BELOW, aX, gApp->mHeight - PEEKABOO_SHOWN_ABOVE,
			PEEKABOO_HIDDEN_DELAY);
	}

	mUserLink = new HyperlinkWidget(BUTTON_USER, this);
	mUserLink->SetFont(FONT_EUROFURENCE14);
	mUserLink->mColor = Color(255, 255, 255, 255);
	mUserLink->mOverColor = Color(0xf0, 0xf0, 0xff);
	mUserLink->mLabel = "( If this is not you, click here )";
	int aLinkWidth = mUserLink->mFont->StringWidth(mUserLink->mLabel);
	int aLinkHeight = mUserLink->mFont->GetHeight();
	mUserLink->Resize(gApp->mWidth / 2 - (aLinkWidth + USER_LINK_PADDING) / 2, USER_LINK_Y, aLinkWidth + USER_LINK_PADDING,
		aLinkHeight + USER_LINK_PADDING);
	mUserLink->mUnderlineSize = 0;
}

// 0x459890
MainMenu::~MainMenu()
{
	delete mPlayClassicButton;
	delete mPlayToughButton;
	delete mPlayMarathonButton;
	delete mOptionsButton;
	delete mHighScoresButton;
	delete mTrophyRoomButton;
	delete mQuitButton;
	delete mUserLink;
	delete mPeekabooButton;
}

// port: one of the original's slides is made (the original makes the structure in the constructor, where each is added)
void MainMenu::AddSlide(Widget* theButton, int theFromX, int theFromY, int theToX, int theToY, double theDelay)
{
	ButtonSlide aSlide;
	aSlide.mButton = theButton;
	aSlide.mDelay = theDelay;
	aSlide.mProgress = 0.0;
	aSlide.mFromX = theFromX;
	aSlide.mFromY = theFromY;
	aSlide.mToX = theToX;
	aSlide.mToY = theToY;
	mSlides.push_back(aSlide);
}

// port: the first of the buttons that the description is made for that the mouse is over, in the order of the original's tests
NcButton* MainMenu::GetOverButton() const
{
	if (mPlayClassicButton->mIsOver)
		return mPlayClassicButton;
	if (mPlayToughButton->mIsOver)
		return mPlayToughButton;
	if (mPlayMarathonButton->mIsOver)
		return mPlayMarathonButton;
	if (mOptionsButton->mIsOver)
		return mOptionsButton;
	if (mHighScoresButton->mIsOver)
		return mHighScoresButton;
	if (mTrophyRoomButton->mIsOver)
		return mTrophyRoomButton;
	if (mQuitButton->mIsOver)
		return mQuitButton;
	return nullptr;
}

// 0x4594e0
void MainMenu::SetPlayButtonImage(Image* theImage)
{
	mPlayToughButton->mComponentImage = theImage;
}

// 0x459540, 0x459520
void MainMenu::UserChanged()
{
	mPlayMarathonButton->mComponentImage = gApp->HasFewRestartPoints() ? IMAGE_MAINMENU_PLAY2LOCKED : IMAGE_MAINMENU_PLAY2;
	mPlayToughButton->mVisible = gApp->IsToughUnlocked();
}

// 0x459360
void MainMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayClassicButton);
	theWidgetManager->AddWidget(mPlayToughButton);
	theWidgetManager->AddWidget(mPlayMarathonButton);
	theWidgetManager->AddWidget(mOptionsButton);
	theWidgetManager->AddWidget(mHighScoresButton);
	theWidgetManager->AddWidget(mTrophyRoomButton);
	theWidgetManager->AddWidget(mQuitButton);
	theWidgetManager->AddWidget(mUserLink);
	if (mPeekabooButton != nullptr)
		theWidgetManager->AddWidget(mPeekabooButton);
}

// 0x459410
void MainMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayClassicButton);
	theWidgetManager->RemoveWidget(mPlayToughButton);
	theWidgetManager->RemoveWidget(mPlayMarathonButton);
	theWidgetManager->RemoveWidget(mOptionsButton);
	theWidgetManager->RemoveWidget(mHighScoresButton);
	theWidgetManager->RemoveWidget(mTrophyRoomButton);
	theWidgetManager->RemoveWidget(mQuitButton);
	theWidgetManager->RemoveWidget(mUserLink);
	if (mPeekabooButton != nullptr)
		theWidgetManager->RemoveWidget(mPeekabooButton);
}

// 0x459830: the buttons go with the menu, to the start of their slides
void MainMenu::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	for (size_t i = 0; i < mSlides.size(); i++)
		mSlides[i].mButton->Move(mSlides[i].mFromX + theX, mSlides[i].mFromY + theY);
}

// 0x459280: the player's link and the greeting fade out while the mouse is over a button (the description takes their place) and in
// when it is not
void MainMenu::Update()
{
	if (!gApp->mHasFocus)
		return;

	Widget::Update();
	if (GetOverButton() == nullptr)
	{
		int anAlpha = mUserLink->mColor.mAlpha + FADE_STEP;
		if (anAlpha > 0xff)
			anAlpha = 0xff;
		mUserLink->mColor.mAlpha = anAlpha;
	}
	else
	{
		int anAlpha = mUserLink->mColor.mAlpha - FADE_STEP;
		if (anAlpha < 0)
			anAlpha = 0;
		mUserLink->mColor.mAlpha = anAlpha;
	}
	MarkDirty();
}

// 0x459580: each button waits, then slides to its place; the peekaboo gnome waits and goes in and out of the screen
void MainMenu::UpdateF(float theFrac)
{
	if (!gApp->mHasFocus)
		return;

	mTime = theFrac + mTime;
	double aDelayStep = theFrac * DELAY_RATE;
	for (size_t i = 0; i < mSlides.size(); i++)
	{
		ButtonSlide* aSlide = &mSlides[i];
		double aDelay = aSlide->mDelay - aDelayStep;
		if (aDelay < 0.0)
			aDelay = 0.0;
		aSlide->mDelay = aDelay;

		if (aSlide->mDelay == 0.0 && aSlide->mProgress < 1.0)
		{
			double aProgress = theFrac * SLIDE_RATE + aSlide->mProgress;
			if (aProgress > 1.0)
				aProgress = 1.0;
			aSlide->mProgress = aProgress;

			int aY = ToInt(Ease(aSlide->mProgress, 0.0, 0.5) * (aSlide->mToY - aSlide->mFromY)) + aSlide->mFromY;
			int aX = ToInt(Ease(aSlide->mProgress, 0.0, 0.5) * (aSlide->mToX - aSlide->mFromX)) + aSlide->mFromX;
			aSlide->mButton->Move(aX, aY);

			// the gnome has got where it was going: in the screen it calls out and stays a moment, and out of it it waits (and, until
			// the head start is taken, comes back); then it goes the other way
			if (aSlide->mProgress == 1.0 && aSlide->mButton == mPeekabooButton)
			{
				bool aStays = false;
				Rect aScreen(0, 0, mWidth, mHeight);
				if (aScreen.Contains(aSlide->mToX, aSlide->mToY))
				{
					gApp->PlaySample(SOUND_VOICE_BOUNCING_YOOHOO);
					aSlide->mDelay = PEEKABOO_SHOWN_DELAY;
				}
				else
				{
					aSlide->mDelay = PEEKABOO_HIDDEN_DELAY;
					aStays = gApp->GetCheatFlag(Application::CHEAT_PEEKABOO);
				}

				if (!aStays)
				{
					aSlide->mProgress = 0.0;
					int aFromX = aSlide->mFromX;
					int aFromY = aSlide->mFromY;
					aSlide->mFromX = aSlide->mToX;
					aSlide->mFromY = aSlide->mToY;
					aSlide->mToX = aFromX;
					aSlide->mToY = aFromY;
				}
			}
		}
	}
}

// 0x45ad20
void MainMenu::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_MAINMENU_SCREEN, 0, 0);
	g->DrawImageF(IMAGE_MAINMENU_HAMMER, (float)(sin(mTime * 0.025) * 2.5 + 345.0), (float)(cos(mTime * 0.06) * 5.0 + 3.0));

	// the greeting, in the player's link's alpha
	g->SetFont(FONT_PTBANANASPLIT24);
	g->SetColor(Color(255, 255, 255, mUserLink->mColor.mAlpha));
	User* aUser = gApp->mUserManager->GetCurrentUser();
	if (aUser == nullptr)
		DrawCenteredLine(g, "Welcome!", LINE_Y);
	else
	{
		DrawCenteredLine(g, "Welcome,", LINE_Y);

		// a name that is too long for it is cut short and ends in dots
		std::string aName = aUser->mName;
		std::string aLine = aName + "!";
		int aLength = (int)aName.size();
		while (g->GetFont()->StringWidth(aLine) > GREETING_MAX_WIDTH)
		{
			aLength--;
			aLine = aName.substr(0, aLength) + "...";
		}
		DrawCenteredLine(g, aLine, LINE2_Y);
	}

	// the description of the button that the mouse is over, in the alpha that the greeting is not
	NcButton* aOverButton = GetOverButton();
	if (aOverButton != nullptr && aOverButton->mId != -1)
		DrawButtonCaption(aOverButton->mId, g, 0xff - mUserLink->mColor.mAlpha);

	// the letters typed so far for the cheat words
	g->SetFont(FONT_EUROFURENCE14);
	g->SetColor(Color(255, 255, 255, 255));
	g->DrawString(gApp->mTyped, TYPED_X, TYPED_Y);
}

// 0x45a5e0: what the button is: its name over two lines in the big font (the second is empty for some), then two lines (and, for the
// marathon, a third) of what it does
void MainMenu::DrawButtonCaption(int theButtonId, Graphics* g, int theAlpha)
{
	std::string aLine1;
	std::string aLine2;
	std::string aLine3;
	std::string aLine4;
	switch (theButtonId)
	{
	case BUTTON_PLAY_CLASSIC:
		aLine1 = "Play";
		aLine2 = "Classic";
		aLine3 = "Play through 25 levels of";
		aLine4 = "Hammer Heads mayhem";
		break;
	case BUTTON_PLAY_TOUGH:
		aLine1 = "Tough";
		aLine2 = "Cookie!";
		aLine3 = "Play an extra challenging";
		aLine4 = "game of Classic Bash";
		break;
	case BUTTON_PLAY_MARATHON:
		aLine1 = "Play";
		aLine2 = "Marathon";
		aLine3 = "For advanced players.";
		aLine4 = "An everlasting avalanche";
		break;
	case BUTTON_OPTIONS:
		aLine1 = "Options";
		aLine3 = "Change the volume";
		aLine4 = "and other options";
		break;
	case BUTTON_HIGH_SCORES:
		aLine1 = "View";
		aLine2 = "High Scores";
		aLine3 = "Look out for number one";
		break;
	case BUTTON_TROPHY_ROOM:
		aLine1 = "View";
		aLine2 = "Trophies";
		aLine3 = "Bask in your own glory";
		break;
	case BUTTON_QUIT:
		aLine1 = "Quit";
		aLine3 = "Save the lawn another time";
		break;
	}

	g->SetColor(Color(255, 255, 255, theAlpha));
	DrawCenteredLine(g, aLine1, LINE_Y);
	DrawCenteredLine(g, aLine2, LINE2_Y);
	g->SetFont(FONT_EUROFURENCE14);
	DrawCenteredLine(g, aLine3, LINE3_Y);
	DrawCenteredLine(g, aLine4, LINE4_Y);
	if (theButtonId == BUTTON_PLAY_MARATHON)
		DrawCenteredLine(g, "of gnome badness", LINE5_Y);
}

// 0x459500
void MainMenu::KeyChar(SexyChar theChar)
{
	Widget::KeyChar(theChar);
	gApp->KeyChar(theChar);
}

// 0x4594c0
void MainMenu::ButtonPress(int theId)
{
	if (theId == BUTTON_USER)
		gApp->PlaySample(SOUND_UI_BUTTON_CLICK);
}

// 0x45aa50
void MainMenu::ButtonDepress(int theId)
{
	switch (theId)
	{
	case BUTTON_PLAY_CLASSIC:
		gApp->NewGame(GAME_MODE_CLASSIC);
		break;

	case BUTTON_PLAY_TOUGH:
		mPlayToughButton->mComponentImage = IMAGE_MAINMENU_PLAY1TOUGHBITTEN;
		gApp->NewGame(GAME_MODE_TOUGH_COOKIE);
		break;

	case BUTTON_PLAY_MARATHON:
		if (!gApp->HasFewRestartPoints())
			gApp->NewGame(GAME_MODE_MARATHON);
		else
		{
			Dialog* aDialog = gApp->NewDialog(DIALOG_MODE_LOCKED, true, "Not yet!",
				"This game mode is locked.\n\nComplete level 10 of Classic Bash to unlock it!", "OK", Dialog::BUTTONS_FOOTER);
			LayoutDialog(aDialog, 300);
			int aWidth = aDialog->mWidth;
			int aHeight = aDialog->GetPreferredHeight(aWidth);
			aDialog->Resize((gApp->mWidth - aWidth) / 2, -aHeight, aWidth, aHeight);
			gApp->AddDialog(DIALOG_MODE_LOCKED, aDialog);
		}
		break;

	case BUTTON_OPTIONS:
		MakeOptionsDialog();
		break;

	case BUTTON_HIGH_SCORES:
		gApp->DoZoomWipe(WipeToHighScores, WIPE_TROPHY);
		break;

	case BUTTON_TROPHY_ROOM:
		gApp->DoZoomWipe(WipeToTrophyRoom, WIPE_TROPHY);
		break;

	case BUTTON_QUIT:
		MakeQuitDialog();
		break;

	case BUTTON_USER:
		gApp->ShowUserDialog();
		break;

	case BUTTON_PEEKABOO:
		if (!gApp->GetCheatFlag(Application::CHEAT_PEEKABOO))
			gApp->ToggleCheat(Application::CHEAT_PEEKABOO);
		break;
	}
}
