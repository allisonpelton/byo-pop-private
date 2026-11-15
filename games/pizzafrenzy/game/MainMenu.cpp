#include "MainMenu.h"
#include "App.h"
#include "PizzaFrenzy.h"
#include "Players.h"
#include "ScreenNodes.h"

using namespace pizzafrenzy;

namespace
{
// The string table's texts of the welcome and of what the buttons do
const int STRING_WELCOME_NO_PLAYER = 0x68;
const int STRING_WELCOME_PLAYER = 0xcc;  // with the player's name
const int STRING_NEW_SPEED_GAME = 0xdc;
const int STRING_NEW_MEMORY_GAME = 0xdd;
const int STRING_NEW_CONCENTRATION_GAME = 0xde;
const int STRING_SPROUT_LOGO = 0xe8;

// The welcome's typing: the width a line breaks at, the letters each second and the wait after a sentence
const float WELCOME_WRAP = 320.0f;
const float WELCOME_RATE = 240.0f;
const float WELCOME_SENTENCE_PAUSE = 0.0f;

// The logo's text: three lines at a time, typed slower, with a wait after each sentence
const int LOGO_VISIBLE_LINES = 3;
const float LOGO_LINE_PAUSE = 0.0f;
const float LOGO_RATE = 120.0f;
const float LOGO_SENTENCE_PAUSE = 2.0f;

// The pizza designer is locked while the player has fewer toppings than this
const int DESIGNER_TOPPINGS = 5;
}

// 0x43a9e0
void MainMenu::setup()
{
	mUserProperties = getNode("userProperties");
	mLorenzoBubble = getNode("lorenzoBubble");
	engine::TextItem* aWelcome = static_cast<engine::TextItem*>(getNode("welcomeUser"));
	mWelcomeTyper = new engine::TextTyper();
	mWelcomeTyper->setPosition(*aWelcome->getPosition());
	mWelcomeTyper->setFont(aWelcome->mFont);
	mWelcomeTyper->setWrapAndTiming(WELCOME_WRAP, WELCOME_RATE, WELCOME_SENTENCE_PAUSE);
	mWelcomeTyper->setColor(*aWelcome->getColor());
	mWelcomeTyper->setColorMode(aWelcome->mColorMode);
	mWelcomeTyper->setLineOffsets(0, 0);
	aWelcome->setText("");
	static_cast<engine::Container*>(mUserProperties.get())->addChild(mWelcomeTyper);
	getNode("EditButton")->setVisible(false);
	getNode("EditBtn")->setEnabled(false);
	mHovering = false;

	// The original's slot 0x184 (0x43b0c0) shows the game's link, or the group that has no link, by the settings. The
	// port has no link: the group with the link goes, with its button, and the group without is shown. Nothing moves:
	// the exe's look with no link has the Sprout logo at the bottom left (x=100) and Quit at the bottom right (x=700).
	removeScreenNode(this, "gameLinkGrp");
	getNode("noGameLinkGrp")->setVisible(true);
}

// 0x43b3d0
void MainMenu::refresh()
{
	showWelcome();
	showDesignerLock();
}

// 0x43a800
void MainMenu::showWelcome()
{
	engine::App* anApp = engine::App::instance();
	engine::UserInfo* aUser = engine::Players::instance()->current();
	if (aUser == nullptr)
		mWelcomeTyper->setTypedText(anApp->loadString(STRING_WELCOME_NO_PLAYER));
	else
		mWelcomeTyper->setTypedText(formatMessage(anApp->loadString(STRING_WELCOME_PLAYER), aUser->getName()));
	mWelcomeTyper->reset();
	mWelcomeTyper->start();
	mHovering = false;
}

// 0x43b000
void MainMenu::showDesignerLock()
{
	int aToppingCount = (int)PizzaFrenzy::instance()->mUserGameData->mToppingLevels.size();
	bool aLocked = false;
	engine::Node* aLock = getNode("editPizzaLocked");
	if (aToppingCount < DESIGNER_TOPPINGS && !PizzaFrenzy::sDesignerUnlockOverride)
		aLocked = true;
	aLock->setVisible(aLocked);
}

// 0x43adb0
void MainMenu::buttonEnter(std::string theName)
{
	engine::App* anApp = engine::App::instance();
	std::string aText;
	if (theName.compare("newSpeedGame") == 0)
		aText = anApp->loadString(STRING_NEW_SPEED_GAME);
	else if (theName.compare("newMemoryGame") == 0)
		aText = anApp->loadString(STRING_NEW_MEMORY_GAME);
	else if (theName.compare("newConcentrationGame") == 0)
		aText = anApp->loadString(STRING_NEW_CONCENTRATION_GAME);
	else if (theName.compare("sproutLogo") == 0)
	{
		aText = anApp->loadString(STRING_SPROUT_LOGO);
		mWelcomeTyper->setLines(LOGO_VISIBLE_LINES, LOGO_LINE_PAUSE);
		mWelcomeTyper->setWrapAndTiming(WELCOME_WRAP, LOGO_RATE, LOGO_SENTENCE_PAUSE);
	}
	if (!aText.empty())
	{
		mWelcomeTyper->setTypedText(aText);
		mWelcomeTyper->reset();
		mWelcomeTyper->start();
		mHovering = true;
	}
}

// 0x43a940
void MainMenu::buttonExit(std::string theName)
{
	mWelcomeTyper->setLines(0, 0);
	mWelcomeTyper->setWrapAndTiming(WELCOME_WRAP, WELCOME_RATE, WELCOME_SENTENCE_PAUSE);
	if (mHovering)
		showWelcome();
}
