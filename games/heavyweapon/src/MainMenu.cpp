#include "MainMenu.h"
#include "ContinueDialog.h"
#include "FlickerLight.h"
#include "HeavyWeaponApp.h"
#include "HighScoresScreen.h"
#include "HyperlinkWidget.h"
#include "MenuButton.h"
#include "MessageDialog.h"
#include "UserListDialog.h"
#include "YesNoDialog.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"

using namespace Sexy;

namespace
{
	const int SONG = SONG_ATOMIC_TANK_59;		// the song of the menu and the armory, from order 0x3b
	const int SONG_ORDER = 0x3b;
	const int NUM_QUOTES = 10;
	const int SCREEN_WIDTH = 0x280;
	const int SCREEN_HEIGHT = 0x1e0;
	const int USER_BUTTON_CENTER_X = 0x1ea;
	const int USER_BUTTON_Y = 0x4b;
	const int USER_BUTTON_HEIGHT = 0x1e;
	const int TITLE_SHIFT = 0xaa;				// the greeting and the name are drawn this far right of the picture's edge
	const int GREETING_Y = 0x37;
	const int NAME_Y = 0x4b;
	const int GLOW_ALPHA_SPEED = 12;
	const int GLOW_X = 0xed;
	const int GLOW_Y = 0x98;
	const int DESCRIPTION_X = 0xd2;
	const int DESCRIPTION_BLACK_Y = 0xaf;		// the black copy of a description; the yellow one is 5 above it
	const int DESCRIPTION_YELLOW_Y = 0xaa;
	const int DESCRIPTION_WIDTH = 0xd2;
	const int DESCRIPTION_HEIGHT = 100;
	const double YELLOW_ALPHA_SCALE = 1.96;		// the button's glow (up to 0x82) in alpha
	const int QUIT_BOX_WIDTH = 0x140;
	const int QUIT_BOX_HEIGHT = 0xe6;
	const int LOCKED_BOX_WIDTH = 0x118;
	const int LOCKED_BOX_HEIGHT = 0xd2;
	const int HIGH_SCORES_LIST_SIZE = 10;

	// The id of each button, in the order of mButtons and DESCRIPTIONS
	const int BUTTON_IDS[MainMenu::NUM_BUTTONS] = {
		MainMenu::BUTTON_CAMPAIGN, MainMenu::BUTTON_SURVIVAL, MainMenu::BUTTON_HIGH_SCORES, HeavyWeaponApp::BUTTON_OPTIONS,
		MainMenu::BUTTON_HELP, MainMenu::BUTTON_QUIT};

	// The addresses of each button's description and of the quit box's quips (GT's keys)
	const unsigned int DESCRIPTIONS[MainMenu::NUM_BUTTONS] = {
		GTA(0x52aaa0), GTA(0x52aa50), GTA(0x52aa14), GTA(0x52a9e4), GTA(0x52a9c0), GTA(0x52a99c)};
	const unsigned int QUOTES[NUM_QUOTES] = {
		GTA(0x52adbc), GTA(0x52ad68), GTA(0x52ad20), GTA(0x52ac98), GTA(0x52ac54), GTA(0x52ac10), GTA(0x52abdc),
		GTA(0x52ab80), GTA(0x52ab28), GTA(0x52aae8)};
}

// 0x425e20: the buttons, the lamp, and the song if it is not playing
MainMenu::MainMenu(HeavyWeaponApp* theApp)
{
	mApp = theApp;
	mUserButton = new HyperlinkWidget(BUTTON_USER, this);
	mUserButton->mLabel = GT(0x52e070);
	mUserButton->SetFont(mApp->mFonts[FONT_COMPUTER]->Duplicate());
	mUserButton->mColor = Color(0xc0, 0x40, 0x20);
	mUserButton->mUnderlineSize = 0;
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		// OPTIONS reports to the app
		ButtonListener* aListener = this;
		if (BUTTON_IDS[i] == HeavyWeaponApp::BUTTON_OPTIONS)
			aListener = mApp;
		mButtons[i] = new MenuButton(mApp, aListener, BUTTON_IDS[i]);
		mButtons[i]->mSprite = mApp->mSprites[(i < 2) ? IMAGE_MAINBIGBTN : IMAGE_MAINSMALLBTN];
	}
	if (!mApp->mMusicInterface->IsPlaying(SONG))
	{
		mApp->mMusicInterface->FadeOutAll(true, 0.939);
		mApp->mMusicInterface->FadeIn(SONG, SONG_ORDER, 0.8765, false);
	}
	mFlickerLight = new FlickerLight(mApp, mApp->mSprites[IMAGE_MAINGLOW]);
	mState = 0;
	mGlowAlpha = 0;
	Resize(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// 0x426a4a (body 0x425d40)
MainMenu::~MainMenu()
{
	mApp->mMainMenu = NULL;
	delete mUserButton;
	for (int i = 0; i < NUM_BUTTONS; i++)
		delete mButtons[i];
	delete mFlickerLight;
	mApp->mIsNewUser = false;
}

// 0x425b00 slot 1
void MainMenu::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mUserButton);
	mUserButton->mColor = Color(0xc0, 0x40, 0x20);
	for (int i = 0; i < NUM_BUTTONS; i++)
		theManager->AddWidget(mButtons[i]);
}

// 0x425b60 slot 2
void MainMenu::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mUserButton);
	for (int i = 0; i < NUM_BUTTONS; i++)
		theManager->RemoveWidget(mButtons[i]);
}

// 0x425ba0 slot 17: the glow picture rises while no button is under the mouse and falls while one is
void MainMenu::Update()
{
	Widget::Update();
	mFlickerLight->Update();
	bool aNoneOver = true;
	for (int i = NUM_BUTTONS - 1; i >= 0; i--)
	{
		if (mButtons[i]->mIsOver)
			aNoneOver = false;
	}
	if (aNoneOver)
	{
		mGlowAlpha += GLOW_ALPHA_SPEED;
		if (mGlowAlpha > 0xff)
			mGlowAlpha = 0xff;
	}
	else
	{
		mGlowAlpha -= GLOW_ALPHA_SPEED;
		if (mGlowAlpha < 0)
			mGlowAlpha = 0;
	}
	MarkDirty();
}

// 0x425c40 slot 13: the buttons' places on the picture; the user link is centred on its text
void MainMenu::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aLinkWidth = mApp->mFonts[FONT_COMPUTER]->StringWidth(mUserButton->mLabel);
	mUserButton->Resize(USER_BUTTON_CENTER_X - aLinkWidth / 2, USER_BUTTON_Y, aLinkWidth, USER_BUTTON_HEIGHT);
	mButtons[0]->Resize(0x2d, 0xc3, 0x6e, 0x6e);
	mButtons[1]->Resize(0xaf, 0x145, 0x6e, 0x6e);
	mButtons[2]->Resize(0x1db, 0xc3, 0x78, 0x28);
	mButtons[3]->Resize(0x1bd, 0x109, 0x78, 0x28);
	mButtons[4]->Resize(0x1bd, 0x14f, 0x78, 0x28);
	mButtons[5]->Resize(0x1db, 0x195, 0x78, 0x28);
}

// 0x426140 slot 16: the picture, the greeting and the name, the lamp, the glow, and the description of each glowing
// button in black and then (above it) in yellow
void MainMenu::Draw(Graphics* g)
{
	if (mState != 0)
		return;
	mApp->mSprites[IMAGE_MAINMENU]->Draw(g, 0, 0, false, 0, 0, false);
	g->Translate(TITLE_SHIFT, 0);
	g->SetFont(mApp->mFonts[FONT_NORMAL]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (mApp->mUsers.empty())
		WriteCenteredLine(g, GREETING_Y, GT(0x52e104));
	else
	{
		if (!mApp->mIsNewUser)
			WriteCenteredLine(g, GREETING_Y, GT(0x52e0f4));
		else
			WriteCenteredLine(g, GREETING_Y, GT(0x52e0e8));
		WriteCenteredLine(g, NAME_Y, std::string(mApp->CurrentUser().mName));
	}
	g->Translate(-TITLE_SHIFT, 0);
	mFlickerLight->Draw(g);
	g->SetColorizeImages(true);
	if (mGlowAlpha != 0)
	{
		g->SetColor(Color(0xff, 0xff, 0xff, mGlowAlpha));
		mApp->mSprites[IMAGE_ATMENU]->Draw(g, GLOW_X, GLOW_Y, false, 0, 0, false);
	}
	g->SetColorizeImages(false);
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		if (mButtons[i]->mGlow == 0)
			continue;
		std::string aText;
		if ((i == BUTTON_SURVIVAL) && (mApp->CurrentUser().mMaxLevel < SURVIVAL_STAGES_NEEDED))
			aText = GT(0x52e098);
		else
			aText = GT(DESCRIPTIONS[i]);
		g->SetFont(mApp->mFonts[FONT_OUTLINE]);
		g->SetColor(Color(0, 0, 0, mButtons[i]->mGlow));
		WriteWordWrapped2004(g, Rect(DESCRIPTION_X, DESCRIPTION_BLACK_Y, DESCRIPTION_WIDTH, DESCRIPTION_HEIGHT), aText,
			mApp->mFonts[FONT_NORMAL]->GetHeight(), -1);
		g->SetFont(mApp->mFonts[FONT_OUTLINED]);
		g->SetColor(Color(0xff, 0xe0, 0x40, (int) (mButtons[i]->mGlow * YELLOW_ALPHA_SCALE)));
		WriteWordWrapped2004(g, Rect(DESCRIPTION_X, DESCRIPTION_YELLOW_Y, DESCRIPTION_WIDTH, DESCRIPTION_HEIGHT), aText,
			mApp->mFonts[FONT_NORMAL]->GetHeight(), -1);
	}
	g->SetColorizeImages(false);
}

// 0x425d20 listener slot 4
void MainMenu::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x4265c0 listener slot 5: the campaign and survival buttons start a game and take the menu away
void MainMenu::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	switch (theId)
	{
	case BUTTON_CAMPAIGN:
		if (mApp->CurrentUser().mLevel == 0)
		{
			mApp->NewGame(false);
			mApp->mWidgetManager->RemoveWidget(this);
			mApp->SafeDeleteWidget(this);
			return;
		}
		mApp->AddDialog(ContinueDialog::DIALOG_ID, new ContinueDialog(mApp));
		break;
	case BUTTON_SURVIVAL:
		if (mApp->CurrentUser().mMaxLevel < SURVIVAL_STAGES_NEEDED)
		{
			mApp->AddDialog(MessageDialog::DIALOG_ID, new MessageDialog(mApp, LOCKED_BOX_WIDTH, LOCKED_BOX_HEIGHT,
				GT(0x52e110), GT(0x52e118)));
			return;
		}
		if (((mApp->CurrentUser().mFlags & UserRecord::FLAG_NO_HELP) == 0) && mApp->mHelpPending)
		{
			mApp->mHelpScreen = mApp->ShowHelp(mApp);
			mApp->mHelpPending = false;
		}
		else
			mApp->NewGame(true);
		mApp->mMainMenu = NULL;
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
		return;
	case BUTTON_HIGH_SCORES:
		mApp->mWidgetManager->AddWidget(new HighScoresScreen(mApp, false, HIGH_SCORES_LIST_SIZE));
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
		return;
	case BUTTON_HELP:
		mApp->ShowHelp(static_cast<ButtonListener*>(NULL));
		return;
	case BUTTON_QUIT:
		mApp->AddDialog(YesNoDialog::DIALOG_ID, new YesNoDialog(mApp, this, GT(0x52d1dc), GT(QUOTES[Rand() % NUM_QUOTES]),
			QUIT_BOX_WIDTH, QUIT_BOX_HEIGHT, BUTTON_QUIT_YES, BUTTON_QUIT_NO));
		break;
	case BUTTON_QUIT_YES:
		mApp->KillDialog(YesNoDialog::DIALOG_ID);
		mApp->Shutdown();
		return;
	case BUTTON_QUIT_NO:
		mApp->KillDialog(YesNoDialog::DIALOG_ID);
		return;
	case ContinueDialog::BUTTON_CONTINUE:
		mApp->KillDialog(ContinueDialog::DIALOG_ID);
		mApp->ContinueGame(mApp->CurrentUser().mLevel);
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
		return;
	case ContinueDialog::BUTTON_NEW_GAME:
		mApp->KillDialog(ContinueDialog::DIALOG_ID);
		mApp->NewGame(false);
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
		return;
	case ContinueDialog::BUTTON_CANCEL:
		mApp->KillDialog(ContinueDialog::DIALOG_ID);
		return;
	case BUTTON_USER:
		mApp->AddDialog(HeavyWeaponApp::DIALOG_USER_LIST, new UserListDialog(mApp));
		break;
	}
}
