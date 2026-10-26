// GameApp's openers and answers of the three dialogs the app makes itself (not through DoDialog): the game's menu
// (MenuDialog), the options (OptionsDialog) and the new player's name (NewUserDialog).
#include <SDL.h>

#include "ArrowHint.h"
#include "Board.h"
#include "Checkbox.h"
#include "ConversationPanel.h"
#include "CutScene.h"
#include "FlashWidget.h"
#include "GameApp.h"
#include "GameText.h"
#include "MainMenu.h"
#include "MenuDialog.h"
#include "NewUserDialog.h"
#include "OptionsDialog.h"
#include "PopLua.h"
#include "Profile.h"
#include "TileEngine.h"
#include "TooltipWidget.h"
#include "WidgetManager.h"
#include "lua.h"
#include "platform/VFS.h"

using namespace Sexy;

void GameApp::ShowGameMenu(bool theFromTreasure)
{
	// 0x4456a0: the tutorial's arrows hear a click where the mouse is, the battle holds (without its banner), the
	// tooltip under the mouse goes, and the menu comes
	if (mArrowHintManager != nullptr)
		mArrowHintManager->Click(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY, 1);
	if (mBoard != nullptr) {
		mBoard->Pause(true, true);
		Unmute(true);
	}
	if (mHoverTooltip != nullptr)
		mHoverTooltip->Hide();
	AddDialog(new MenuDialog(mBoard != nullptr, theFromTreasure, mMiniGameFromMenu));
	Unmute(true);
	// port: the sound player's pause of the looping samples (0x6afe90) is not ported
}

void GameApp::ShowOptionsDialog()
{
	// 0x4455d0
	if (mBoard != nullptr) {
		mBoard->Pause(true, true);
		Unmute(true);
	}
	OptionsDialog* aDialog = new OptionsDialog(mBoard != nullptr);
	aDialog->Layout(500);
	AddDialog(DIALOG_OPTIONS, aDialog);
	Unmute(true);
	// port: the sound player's pause of the looping samples (0x6afe90) is not ported
	SetCursor(CURSOR_POINTER);
}

void GameApp::CloseOptionsDialog(bool theApply)
{
	// 0x4457b0
	OptionsDialog* aDialog = static_cast<OptionsDialog*>(GetDialog(DIALOG_OPTIONS));
	if (aDialog == nullptr)
		return;
	if (theApply) {
		bool aWindowed = !aDialog->mFullscreenBox->IsChecked();
		bool a3D = aDialog->m3DBox->IsChecked();
		if (mProfile != nullptr)
			mProfile->SetBool("DisableTips", aDialog->mNoTooltipsBox->IsChecked());
		SwitchScreenMode(aWindowed, a3D, false);
		EnableCustomCursors(aDialog->mCursorsBox->IsChecked());
		ClearUpdateBacklog(false);
	}
	KillDialog(DIALOG_OPTIONS);
	if (mBoard != nullptr)
		mBoard->Pause(false, true);
}

void GameApp::SetShowTilePips(bool theShow)
{
	// 0x44f7e0
	mShowTilePips = theShow;
	if (gTileEngine != nullptr) {
		lua_pushboolean(mLua->L, theShow);
		gTileEngine->CallLinkedMethod("SetShowTilePips", 1);
	}
	if (mProfile != nullptr)
		mProfile->SetBool("EnablePips", theShow);
}

void GameApp::StartTreasureToMenuTransition()
{
	// 0x44ded0: unlike StartToMenuTransition (0x44df10), the arrows stay (the treasure screen has none)
	mTransitionState = TRANSITION_TO_MENU - 1;
	DoCurtainTransition(true, "", -1);
}

void GameApp::ShowNewUserDialog(int theButtonMode)
{
	// 0x444b90: "Lex", or "Lex2", "Lex3", ... if the players' files have that name (the file names that begin with
	// "Lex" are the ones to look through)
	std::vector<std::string> aLexNames;
	bool aHasLex = false;
	for (const std::string& aFile : Platform::ListDirectory(GetAppDataFolder() + "users")) {
		size_t aPos = aFile.find(".bwa");
		if (aPos == std::string::npos)
			continue;
		std::string aName = aFile.substr(0, aPos);
		if (SDL_strcasecmp(aName.c_str(), GT(0x7ac5d4).c_str()) == 0)
			aHasLex = true;
		if (SDL_strcasecmp(aName.substr(0, 3).c_str(), GT(0x7ac5d4).c_str()) == 0)
			aLexNames.push_back(aName);
	}
	std::string aDefaultName = GT(0x7ac5d4);
	if (aHasLex) {
		for (int aNumber = 2;; aNumber++) {
			aDefaultName = StrFormat(GT(0x7ac5d8).c_str(), aNumber);
			bool aTaken = false;
			for (const std::string& aLexName : aLexNames) {
				if (SDL_strcasecmp(aLexName.c_str(), aDefaultName.c_str()) == 0) {
					aTaken = true;
					break;
				}
			}
			if (!aTaken)
				break;
		}
	}
	NewUserDialog* aDialog = new NewUserDialog(theButtonMode, aDefaultName);
	aDialog->Layout(500);
	aDialog->Move((mWidth - aDialog->mWidth) / 2, (mHeight - aDialog->mHeight) / 2);
	AddDialog(aDialog->mId, aDialog);
}

void GameApp::NewUserDialogDone()
{
	// 0x4450a0
	NewUserDialog* aDialog = static_cast<NewUserDialog*>(GetDialog(DIALOG_NEW_USER));
	std::string aName = aDialog != nullptr ? aDialog->GetName() : std::string();
	// with the main menu up (a player added from its list) the name must be free
	if (!aName.empty() && (mMainMenu == nullptr || mMainMenu->IsUserNameFree(aName))) {
		if (mProfile == nullptr || !mProfile->mNoSave) {
			// a player of the list: a profile of their own
			delete mProfile;
			mProfile = new Profile(aName, false);
		} else {
			// the intro's, which was never saved, gets its name (0x443040) and is saved from now on
			mProfile->mName = aName;
			mProfile->mNoSave = false;
		}
		mProfile->Save();
	}
	// the conversation that asked goes on to its next panel
	if (mConversationPanel != nullptr && mConversationPanel->Active()) {
		mConversationPanel->NextPanel();
		mConversationPanel->MarkDirty();
	}
	// the intro's movie, which waited for the name, plays
	if (mPlayIntro && mCutScene != nullptr) {
		if (!mCutScene->mEnded)
			mCutScene->mFlash->Unpause();
		mCutScene->mWaitingForCurtain = false;
	}
	mPlayIntro = false;
	// the main menu, when it is up, takes the player, or says the name is taken and the dialog stays
	if (mMainMenu != nullptr) {
		if (mMainMenu->TakeNewUser(aName)) {
			DeleteUserData(aName);
			KillDialog(DIALOG_NEW_USER);
		}
	} else {
		DeleteUserData(aName);
		KillDialog(DIALOG_NEW_USER);
	}
}

void GameApp::DeleteUserData(const std::string& theName)
{
	// 0x4452d0: a new player starts without a saved game under that name
	EraseFile(GetAppDataFolder() + StrFormat("users/data/%s.dat", theName.c_str()));
	// port: the original also erased the files that GetFilesInDirectory finds for "users/backup/<name> *.*", but
	// handed EraseFile their bare names (no folder), so it erased nothing; that is left out
}
