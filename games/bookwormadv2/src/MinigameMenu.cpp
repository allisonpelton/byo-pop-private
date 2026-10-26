#include "MinigameMenu.h"

#include <SDL.h>

#include "CurtainTransition.h"
#include "FadeButton.h"
#include "GameApp.h"
#include "Graphics.h"
#include "Image.h"
#include "LuaLinkedClass.h"
#include "LuaResourceManager.h"
#include "PopLua.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SharedImage.h"
#include "WidgetManager.h"
#include "XLayout.h"
#include "lua.h"

using namespace Sexy;

namespace {

// How much of the box each mini-game's button takes
const int kButtonSpacing = 66;

}  // namespace

// 0x4805e0: the resources of the menu and of the mini-games' buttons, a button for each mini-game, the main menu
// button, Lex and Moxie, and everything put in the layout's places
MinigameMenu::MinigameMenu(GameApp* theApp)
	: mLayout(nullptr), mApp(theApp), mLex(nullptr), mMoxie(nullptr), mMainMenuButton(nullptr)
{
	// port: the layout finds its pictures and fonts through the app's resource manager (XLayout.h); the original
	// gave it three function pointers for that (0x4aebe0, 0x4b7a10, 0x4b7a20)
	mLayout = new XLayout();
	mPriority = 3;

	if (!mApp->mResourceManager->LoadResources("MiniGameMenu"))
		mApp->ShowResourceError(false);
	if (!ExtractMiniGameMenuResources(mApp->mResourceManager))
		mApp->ShowResourceError(false);

	// the resource groups of the mini-games' buttons are the strings the script returns, all of them
	lua_State* L = mApp->mLua->L;
	if (mApp->mLua->Call("main.GetInitUnlockedMiniGameResGroups", 0, LUA_MULTRET)) {
		int aNumGroups = mApp->mLua->Results();
		for (int i = 0; i < aNumGroups; i++)
			mResGroups.push_back(lua_tostring(L, i - aNumGroups));
		lua_pop(L, aNumGroups);
	}

	for (size_t i = 0; i < mResGroups.size(); i++) {
		mApp->mResourceManager->StartLoadResources(mResGroups[i]);
		while (mApp->mResourceManager->LoadNextResource())
			;
		if (mApp->mResourceManager->HadError())
			mApp->ShowResourceError(false);
	}
	static_cast<LuaResourceManager*>(mApp->mResourceManager)->ExportToLua();

	// a button for each mini-game, its id its number
	int aNumMiniGames = LuaLib::CallLua("main.GetNumMiniGames", 0).Int();
	for (int i = 0; i < aNumMiniGames; i++) {
		lua_pushint(L, i);
		std::string aClass = LuaLib::CallLua("main.GetMiniGameClass", 1).String();
		lua_pushint(L, i);
		std::string anImageStr = LuaLib::CallLua("main.GetMiniGameImageStr", 1).String();

		FadeButton* aButton = new FadeButton(i, static_cast<ButtonListener*>(this));
		SharedImageRef anImage = mApp->mResourceManager->GetImage(anImageStr);
		aButton->mButtonImage = anImage;
		aButton->mHasAlpha = true;
		aButton->mHasTransparencies = true;
		aButton->SetupImage(nullptr);
		aButton->Resize(aButton->mButtonImage->GetCelRect(0));
		AddWidget(aButton);
		mButtons.push_back(aButton);

		MiniGame aMiniGame;
		aMiniGame.mClass = aClass;
		aMiniGame.mImageStr = anImageStr;
		mMiniGames.push_back(aMiniGame);
	}

	mMainMenuButton = new FadeButton(BUTTON_MAIN_MENU, static_cast<ButtonListener*>(this));
	mMainMenuButton->mButtonImage = IMAGE_MINIMENU_BTN_MAINMENU;
	mMainMenuButton->SetupImage(nullptr);
	mMainMenuButton->Resize(mMainMenuButton->mButtonImage->GetCelRect(0));
	AddWidget(mMainMenuButton);

	Resize(0, 0, mApp->mWidth, mApp->mHeight);

	mLex = new PopAnim(-2, static_cast<PopAnimListener*>(this));
	mLex->LoadAnimation("images/pam/lex.pam");
	mMoxie = new PopAnim(-3, static_cast<PopAnimListener*>(this));
	mMoxie->LoadAnimation("images/pam/moxie.pam");

	mLayout->Clear();
	mLayout->Load("data/layout_minihut.xml");

	// port: the original used the layout's places unchecked; a place the file lacks is a spare one here
	static XLayout sMissing;
	auto aFind = [this](const char* theName) -> XLayout* {
		XLayout* aNode = mLayout->FindNode(theName);
		if (aNode == nullptr) {
			SDL_Log("MinigameMenu: the layout has no \"%s\"", theName);
			aNode = &sMissing;
		}
		return aNode;
	};

	// the box grows by a row of buttons for each mini-game and is put at (the app's height - its height + 18) / 2,
	// at most 130 from the top, and its title, welcome and instructions go with it
	XLayout* aBox = aFind("Box");
	aBox->SetBoundsHeight(aBox->GetHeight() + (int)mButtons.size() * kButtonSpacing);
	Rect aSrcRect = aBox->GetSrcRect();
	aSrcRect.mHeight = aBox->GetHeight();
	aBox->SetSrcRect(aSrcRect);
	int anOldY = aBox->GetY();
	int aNewY = (mApp->mHeight - aBox->GetHeight() + 18) / 2;
	if (aNewY > 130)
		aNewY = 130;
	aBox->SetBoundsY(aNewY);

	XLayout* aTitle = aFind("Title");
	aTitle->SetBoundsY(aTitle->GetY() + (aNewY - anOldY) + 5);
	XLayout* aWelcome = aFind("Welcome");
	aWelcome->SetBoundsY(aWelcome->GetY() + (aNewY - anOldY) + 13);
	XLayout* anInstructions = aFind("Instructions");
	anInstructions->SetBoundsY(anInstructions->GetY() + (aNewY - anOldY) + 13);

	// the main menu button at the bottom of the box
	int aMainMenuY = aBox->GetY() - 40;
	int aMainMenuCelWidth = mMainMenuButton->mButtonImage->GetCelWidth();
	mMainMenuButton->Move(aBox->GetX() + (aBox->GetWidth() - aMainMenuCelWidth) / 2, aBox->GetHeight() + aMainMenuY);

	// the mini-games' buttons one under the other, centred in the box
	int aY = 0;
	for (size_t i = 0; i < mButtons.size(); i++) {
		FadeButton* aButton = mButtons[i];
		if (i == 0)
			aY = aBox->GetY() + aNumMiniGames * 30 - 50;
		else
			aY += kButtonSpacing;
		int aCelWidth = aButton->mButtonImage->GetCelWidth();
		aButton->Move(aBox->GetX() + (aBox->GetWidth() - aCelWidth) / 2, aBox->GetY() + aY);
	}

	XLayout* aLexNode = aFind("Lex");
	mLex->mX = aLexNode->GetX();
	mLex->mY = aLexNode->GetY();
	XLayout* aMoxieNode = aFind("Moxie");
	mMoxie->mX = aMoxieNode->GetX();
	mMoxie->mY = aMoxieNode->GetY();
	mLex->PlayLabel("idle");
	mMoxie->PlayLabel("idle");
}

// 0x481670: the resources of the buttons and of the menu go, then the buttons, the layout and the animations
MinigameMenu::~MinigameMenu()
{
	for (size_t i = 0; i < mResGroups.size(); i++)
		mApp->mResourceManager->DeleteResources(mResGroups[i]);
	mApp->mResourceManager->DeleteResources("MiniGameMenu");
	RemoveAllWidgets(true, true);
	delete mLayout;
	delete mLex;
	delete mMoxie;
}

// 0x4818c0: the layout's animations, and Lex and Moxie a frame on (they are not in the widget list)
void MinigameMenu::Update()
{
	Widget::Update();
	mLayout->Update();
	mLex->UpdateF(1.0f);
	mMoxie->UpdateF(1.0f);
}

// 0x481900: the layout, then Lex and Moxie where it puts them
void MinigameMenu::Draw(Graphics* g)
{
	mLayout->Draw(g);
	g->Translate(mLex->mX, mLex->mY);
	mLex->Draw(g);
	g->Translate(-mLex->mX, -mLex->mY);
	g->Translate(mMoxie->mX, mMoxie->mY);
	mMoxie->Draw(g);
	g->Translate(-mMoxie->mX, -mMoxie->mY);
}

// 0x481be0
void MinigameMenu::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_CLICK2);
}

// 0x4819a0: once the curtain is open, a button turns every button off and closes the whole curtain: the main menu
// button's over the main menu (0x481e00), a mini-game's over the mini-game, which the curtain's callback then
// starts (0x481cd0)
void MinigameMenu::ButtonDepress(int theId)
{
	CurtainTransition* aCurtain = mApp->mCurtain;
	if (aCurtain != nullptr && !(aCurtain->mDone && aCurtain->mDrawn))
		return;

	if (theId == mMainMenuButton->mId) {
		mMainMenuButton->SetDisabled(true);
		for (size_t i = 0; i < mButtons.size(); i++)
			mButtons[i]->SetDisabled(true);
		mApp->mCurtainCallbackObject = this;
		mApp->mCurtainCallback = ReturnToMainMenu;
		mApp->DoFullCurtainTransition("");
	} else {
		mMainMenuButton->SetDisabled(true);
		for (size_t i = 0; i < mButtons.size(); i++)
			mButtons[i]->SetDisabled(true);
		mApp->mMiniGameFromMenu = true;
		mApp->mCurtainCallback = StartPickedMiniGame;
		mApp->mCurtainCallbackObject = this;
		mClass = mMiniGames[theId].mClass;
		mApp->DoFullCurtainTransition("");
	}
}

// 0x481e00: the menu goes and the main menu (which has stayed under it) is back
void MinigameMenu::ReturnToMainMenu(void* theMenu)
{
	MinigameMenu* aMenu = static_cast<MinigameMenu*>(theMenu);
	GameApp* anApp = aMenu->mApp;
	anApp->mMiniGameFromMenu = false;
	anApp->mWidgetManager->RemoveWidget(anApp->mMiniGameMenu);
	anApp->SafeDeleteWidget(anApp->mMiniGameMenu);
	anApp->mMiniGameMenu = nullptr;
	anApp->ShowMainMenu(false, false);
}

// 0x481cd0: the main menu and the menu go, and the scripts start the mini-game picked
void MinigameMenu::StartPickedMiniGame(void* theMenu)
{
	MinigameMenu* aMenu = static_cast<MinigameMenu*>(theMenu);
	GameApp* anApp = aMenu->mApp;
	anApp->RemoveMainMenu();
	anApp->mWidgetManager->RemoveWidget(anApp->mMiniGameMenu);
	anApp->SafeDeleteWidget(anApp->mMiniGameMenu);
	anApp->mMiniGameMenu = nullptr;
	// 0x481e70: the class as the script's argument
	std::string aClass = aMenu->mClass;
	lua_pushstring(anApp->mLua->L, aClass.c_str());
	LuaLib::CallLua("main.StartMinigameFromMainMenu", 1);
}

// 0x481c00: Lex or Moxie has played their label and idles again
void MinigameMenu::PopAnimStopped(int theId)
{
	if (theId == mLex->mId)
		mLex->PlayLabel("idle");
	else if (theId == mMoxie->mId)
		mMoxie->PlayLabel("idle");
}

// 0x4805d0
void MinigameMenu::PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam)
{
}
