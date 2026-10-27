#include "MinigameMenu.h"

#include <SDL.h>

#include "Checkbox.h"
#include "CurtainTransition.h"
#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "LuaLinkedClass.h"
#include "LuaResourceManager.h"
#include "OptionsDialog.h"
#include "PopLua.h"
#include "Profile.h"
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

// Where the first button is, from the top of the box
const int kFirstButtonY = 90;

bool ProfileBool(GameApp* theApp, const char* theKey)
{
	return theApp->mProfile != nullptr && theApp->mProfile->GetBool(theKey);
}

int ProfileInt(GameApp* theApp, const char* theKey)
{
	return theApp->mProfile != nullptr ? theApp->mProfile->GetInt(theKey) : 0;
}

// Whether a mini-game is in the box (0x470120): all of them once the adventure is complete, else by the book the
// player is in, the class MutaWords from the 6th, GoldenCoins from the 5th and any other from the 4th
bool IsUnlocked(GameApp* theApp, const std::string& theClass)
{
	if (ProfileBool(theApp, "AdventureComplete"))
		return true;
	int aBook = ProfileInt(theApp, "CurrentBook");
	if (aBook > 5 && theClass == "MutaWords")
		return true;
	if (aBook > 4 && theClass == "GoldenCoins")
		return true;
	return aBook > 3 && theClass != "MutaWords" && theClass != "GoldenCoins";
}

}  // namespace

// 0x470120: the resources of the menu and of the mini-games' buttons, a button for each mini-game the player has, the
// main menu button, the tooltips box, Lex and Moxie, and everything put in the layout's places
MinigameMenu::MinigameMenu(GameApp* theApp)
	: mLayout(nullptr), mApp(theApp), mLex(nullptr), mMoxie(nullptr), mMainMenuButton(nullptr), mFromMainMenu(false),
	  mTipsBox(nullptr)
{
	// port: the layout finds its pictures and fonts through the app's resource manager (XLayout.h); the original
	// gave it three function pointers for that (0x490190, 0x49ac60 twice)
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

	// a button for each mini-game the player has, its id its place in the list (not its number: a mini-game he has
	// not unlocked leaves no gap)
	int aNumMiniGames = LuaLib::CallLua("main.GetNumMiniGames", 0).Int();
	for (int i = 0; i < aNumMiniGames; i++) {
		lua_pushint(L, i);
		std::string aClass = LuaLib::CallLua("main.GetMiniGameClass", 1).String();
		lua_pushint(L, i);
		std::string anImageStr = LuaLib::CallLua("main.GetMiniGameImageStr", 1).String();

		// port: the original made the button first and left it unused when the mini-game was not there to be picked
		if (!IsUnlocked(mApp, aClass))
			continue;

		FadeButton* aButton = new FadeButton((int)mButtons.size(), static_cast<ButtonListener*>(this));
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

	Resize(0, 0, mApp->mWidth, mApp->mHeight);

	mMainMenuButton = new FadeButton(BUTTON_MAIN_MENU, static_cast<ButtonListener*>(this));
	mMainMenuButton->mButtonImage = IMAGE_MINIMENU_BTN_MAINMENU;
	mMainMenuButton->SetupImage(nullptr);
	mMainMenuButton->Resize(mMainMenuButton->mButtonImage->GetCelRect(0));
	AddWidget(mMainMenuButton);

	// the box in the corner for the tooltips: ticked, and so in the profile, until the player has said otherwise
	mTipsBox = MakeCheckbox(CHECKBOX_TIPS, static_cast<CheckboxListener*>(this));
	mTipsBox->Move(745, 558);
	mTipsBox->SetVisible(true);
	mTipsBox->SetDisabled(false);
	if (mApp->mProfile != nullptr && mApp->mProfile->KeyExists("ShowMGTips")) {
		mTipsBox->mChecked = mApp->mProfile->GetBool("ShowMGTips");
	} else {
		mTipsBox->mChecked = true;
		if (mApp->mProfile != nullptr)
			mApp->mProfile->SetBool("ShowMGTips", true);
	}
	AddWidget(mTipsBox);

	mLex = new PopAnim(-2, static_cast<PopAnimListener*>(this));
	mLex->LoadAnimation("images/pam/Lex_normal.txt");
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
	// at most 130 from the top, and its title goes with it
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

	// the main menu button at the bottom of the box
	int aMainMenuY = aBox->GetY() - 40;
	int aMainMenuCelWidth = mMainMenuButton->mButtonImage->GetCelWidth();
	mMainMenuButton->Move(aBox->GetX() + (aBox->GetWidth() - aMainMenuCelWidth) / 2, aBox->GetHeight() + aMainMenuY);

	// the mini-games' buttons one under the other, centred in the box
	int aY = 0;
	for (size_t i = 0; i < mButtons.size(); i++) {
		FadeButton* aButton = mButtons[i];
		if (i == 0)
			aY = kFirstButtonY;
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

// 0x4715f0: the resources of the buttons and of the menu go, then the buttons, the layout and the animations
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

// 0x471850: the layout's animations, and Lex and Moxie a frame on (they are not in the widget list)
void MinigameMenu::Update()
{
	Widget::Update();
	mLayout->Update();
	mLex->UpdateF(1.0f);
	mMoxie->UpdateF(1.0f);
}

// 0x4718a0: the layout, then Lex and Moxie where it puts them, and from the main menu the tooltips box's label on its
// panel; the "new" marks come over everything (DrawOverlay)
void MinigameMenu::Draw(Graphics* g)
{
	mLayout->Draw(g);
	g->Translate(mLex->mX, mLex->mY);
	mLex->Draw(g);
	g->Translate(-mLex->mX, -mLex->mY);
	g->Translate(mMoxie->mX, mMoxie->mY);
	mMoxie->Draw(g);
	g->Translate(-mMoxie->mX, -mMoxie->mY);

	if (mFromMainMenu) {
		g->SetFont(FONT_LARGE);
		g->SetColor(Color(120, 50, 75));
		g->DrawImage(IMAGE_MINIMENU_TIP_BG, mWidth - IMAGE_MINIMENU_TIP_BG->mWidth,
		             mHeight - IMAGE_MINIMENU_TIP_BG->mHeight - 9);
		std::string aLabel = GT(0x6fd8bc);
		g->DrawString(aLabel, 737 - FONT_LARGE->StringWidth(aLabel), 578);
	}
	DeferOverlay(0);
}

// 0x471a70: over the curtain's opening, the "new" mark on the mini-game that the player has just unlocked: WordUp in
// the 4th book, GoldenCoins in the 5th and MutaWords in the 6th, none once the adventure is complete
void MinigameMenu::DrawOverlay(Graphics* g)
{
	CurtainTransition* aCurtain = mApp->mCurtain;
	if (aCurtain != nullptr && !(aCurtain->mDone && aCurtain->mDrawn))
		return;
	if (ProfileBool(mApp, "AdventureComplete"))
		return;

	const int aBook = ProfileInt(mApp, "CurrentBook");
	const char* aNewClass = aBook == 4 ? "WordUp" : aBook == 5 ? "GoldenCoins" : aBook == 6 ? "MutaWords" : nullptr;
	if (aNewClass == nullptr)
		return;

	for (size_t i = 0; i < mMiniGames.size(); i++) {
		if (mMiniGames[i].mClass == aNewClass) {
			Image* aMark = IMAGE_MINIMENU_NEW;
			g->DrawImage(aMark, mButtons[i]->mX - aMark->mWidth / 2 + 5, mButtons[i]->mY - aMark->mHeight / 2 + 5);
		}
	}
}

// 0x4721d0
void MinigameMenu::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_CLICK2);
}

// 0x471f70: once the curtain is open, a button turns every button off and closes the whole curtain: the main menu
// button's over the main menu (0x4724d0), a mini-game's over the mini-game, which the curtain's callback then
// starts (0x472330)
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
		mApp->mMiniGameFromMenu = mFromMainMenu;
		mApp->mCurtainCallback = StartPickedMiniGame;
		mApp->mCurtainCallbackObject = this;
		if (theId >= 0 && (size_t)theId < mMiniGames.size())
			mClass = mMiniGames[theId].mClass;
		mApp->DoFullCurtainTransition("");
	}
}

// 0x4724d0: the menu goes and the main menu (which has stayed under it) is back
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

// 0x472330: the main menu (if there is one) and the menu go, and the scripts start the mini-game picked, the
// chapter screen's way or the main menu's (0x472530: the class as the script's argument)
void MinigameMenu::StartPickedMiniGame(void* theMenu)
{
	MinigameMenu* aMenu = static_cast<MinigameMenu*>(theMenu);
	GameApp* anApp = aMenu->mApp;
	const bool aFromMainMenu = aMenu->mFromMainMenu;
	if (anApp->mMainMenu != nullptr)
		anApp->RemoveMainMenu();
	anApp->mWidgetManager->RemoveWidget(anApp->mMiniGameMenu);
	anApp->SafeDeleteWidget(anApp->mMiniGameMenu);
	anApp->mMiniGameMenu = nullptr;
	std::string aClass = aMenu->mClass;
	lua_pushstring(anApp->mLua->L, aClass.c_str());
	LuaLib::CallLua(aFromMainMenu ? "main.StartMinigameFromMainMenu" : "main.StartMinigameFromCS", 1);
}

// 0x4721f0
void MinigameMenu::CheckboxChecked(int theId, bool checked)
{
	if (theId == CHECKBOX_TIPS && mApp->mProfile != nullptr)
		mApp->mProfile->SetBool("ShowMGTips", checked);
	mApp->PlaySample(SOUND_CLICK2);
}

// 0x472250: Lex or Moxie has played their label and idles again
void MinigameMenu::PopAnimStopped(int theId)
{
	if (theId == mLex->mId)
		mLex->PlayLabel("idle");
	else if (theId == mMoxie->mId)
		mMoxie->PlayLabel("idle");
}
