// WinWSAppButtons.cpp: WinWSApp::ButtonDepress (0x431630, slot 4), the application's button and dialog handler. Every
// button of the game's screens and every dialog button (ids 2000 + dialog id for yes/ok, 3000 + dialog id for no/cancel,
// Dialog2004) comes here.
//
// Not translated (registration, trial and update code; the port behaves as a registered game):
//  - 0x11b3, 0x159b: buttons of the update dialog 0x9e3 (0x41b230 starts the update check dialogs); 0x38c4, 0x3cac,
//    0x5fd4: buttons of the update dialogs 0x30f4 and 0x9e4 (0x38c4 also asks InternetManager::GetUpdateURL and opens it).
//  - 0x190 (the Register button), 0x11b5 and 0x159d (yes and no of the trial-limit dialog 0x9e5, 0x41c1c0), 0xa28 and
//    0xe10 (yes and no of the register dialog 600, 0x414020): they open or close the register dialogs, which the port
//    does not have. (0xadc is no register button: it is the OK button of the credits dialog 0x30c, translated below.)
//  - 0x41da80 (the trial-limit test, "You have played for more than %d minutes. Please register ...") in the cases of
//    0x64, 0x65, 0x66, 0x67 and 0x6a: it is false for a registered game. 0x41d870 (the play counter of a trial) in 0x9c4.
//    0xc9: the text of the unregistered game ("In the registered version, your progress is saved every %d feet").
// No id of this handler opens a web page except 0x38c4 (More Games has no id here).
//
// The fields of the other classes are named after their offsets (mFieldXXX, mFlagXXX) until their groups name them.

#include "WinWSApp.h"
#include "GameText.h"
#include "Board.h"
#include "MenuScreen.h"
#include "MapScreen.h"
#include "HighScoreScreen.h"
#include "StatsScreen.h"
#include "TypingTutor.h"
#include "UserProfileMgr.h"
#include "UserProfile.h"
#include "LessonManager.h"
#include "DifficultyDialog.h"
#include "NewUserDialog.h"
#include "SelectUserDialog.h"
#include "AbyssModeDialog.h"
#include "OptionsDialog.h"
#include "DeleteUserDialog.h"
#include "HintDialog.h"
#include "WidgetManager.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "EditWidget.h"
#include "Common.h"
#include <string.h>

using namespace Sexy;

// The name of a difficulty setting (0 .. 3 by their own texts, anything else is the last one); 0x431630 repeats this
// chain in both saved-game texts of the 0x64 case
static const std::string& GetDifficultyName(int theDifficulty)
{
	if (theDifficulty == 0)
		return GT(0x0052d280);
	else if (theDifficulty == 1)
		return GT(0x0052d278);       // "Normal"
	else if (theDifficulty == 2)
		return GT(0x0052d270);
	else if (theDifficulty == 3)
		return GT(0x0052d268);       // "Expert"
	else
		return GT(0x0052d260);       // "X-Treme"
}

// 0x431630 slot 4
void WinWSApp::ButtonDepress(int theId)
{
	switch (theId)
	{
	// the difficulty dialog's buttons (600 + the difficulty), then the Adventure button 0x64 and the Abyss button 0x65 that continue
	case 600:
	case 601:
	case 602:
	case 603:
	case 604:
	case 0x65:
		if (theId != 0x65)
		{
			mOpenDialogCount--;
			KillDialog(DIALOG_DIFFICULTY);
			mDialog61c = NULL;
			mDifficulty = theId - 600;
			mUser->mFlag230 = true;

			int i = 0;
			do
			{
				mUser->mField234[i] = 0;
				i++;
			} while (i < 6);
			i = 0;
			do
			{
				mUser->mField24c[i] = 0;
				i++;
			} while (i < 4);

			if (mDifficulty != mUser->mField280)
			{
				mUser->mField280 = mDifficulty;
				i = 0;
				do
				{
					mUser->mField25c[i] = 0;
					i++;
				} while (i < 0x24);
			}
			mProfileMgr->WriteUserProfileToRegistry(mUser, true, false);    // 0x41f1d0
		}
		else
		{
			// 0x41da80(2): the trial limit, not ported
			mDifficulty = -1;
			mAbyssDepth = (mUser->mBestDepth / 400) * 400;
			if (mAbyssDepth >= 400)       // and the game is registered (this[0x470]: always, in the port)
			{
				ShowAbyssModeDialog();      // 0x4129a0
				return;
			}
		}
		mUser->SetField1f4(0);      // 0x412640
		mUser->SetField1f8(0);      // 0x412650
		CreateBoard();             // 0x42f3c0
		return;

	case 0x64:
	{
		mFlag8d8 = true;
		mFlag8d9 = false;
		// 0x41da80(1): the trial limit, not ported
		int aSavedLevel = mUser->mField1c0;
		if ((aSavedLevel >= 1) && (aSavedLevel <= 0x23))
		{
			// a saved adventure game
			mFlag68d = true;
			int aDifficulty = mUser->mField1c4;
			int aLevel = mUser->mField1c8;
			std::string aText = StrFormat(GTC(0x0052d200), aLevel + 1, GetDifficultyName(aDifficulty).c_str());
			ShowConfirmDialog(GT(0x0052d1ec), aText, "", true, GT(0x0052d1f8), GT(0x0052d1fc));     // "CONTINUE?", "YES", "NO"
			return;
		}
		if ((aSavedLevel == -1) || (aSavedLevel == 0))
		{
			// nothing saved: the difficulty dialog
			mOpenDialogCount++;
			mDialog61c = new DifficultyDialog(this);        // 0x43f0f0 (sizeof 0x158)
			mDialog61c->Resize(0xaa, 0x28, 0x12c, mDialog61c->GetPreferredHeight(0));
			AddDialog(DIALOG_DIFFICULTY, mDialog61c);
			mWidgetManager->SetFocus(mDialog61c);
			return;
		}

		// a saved game of the abyss mode (the level numbers above the adventure's)
		mFlag68d = true;
		int aDifficulty = mUser->mField1c4;
		int aLevel = mUser->mField1c8;
		std::string aText = StrFormat(GTC(0x0052d178), aLevel + 1, GetDifficultyName(aDifficulty).c_str());
		ShowConfirmDialog(GT(0x0052d1ec), aText, "", true, GT(0x0052d1f8), GT(0x0052d1fc));
		return;
	}

	case DIALOG_STATS_CLOSE:    // 0x450: the Close button of the stats screen
		if (mStatsScreen == NULL)
			return;
		mWidgetManager->RemoveWidget(mStatsScreen);
		if (mStatsScreen != NULL)
			delete mStatsScreen;
		mStatsScreen = NULL;
		mStatsCloseButton->SetDisabled(true);
		mStatsCloseButton->SetVisible(false);
		if (mTypingTutor == NULL)
		{
			ShowMenuScreen();
			return;
		}
		mTypingTutor->mFlag261 = false;
		mTypingTutor->SetVisible(true);
		mWidgetManager->SetFocus(mTypingTutor);
		return;

	case DIALOG_HIGHSCORE_CLOSE:    // 0xccf: the Close This Screen button of the high score screen
		mFlag8d9 = false;
		mWidgetManager->RemoveWidget(mHighScoreScreen);
		if (mHighScoreScreen != NULL)
			delete mHighScoreScreen;
		mHighScoreScreen = NULL;
		mHighScoreCloseButton->SetVisible(false);
		if ((mBoard == NULL) && (mTypingTutor == NULL))
		{
			ShowMenuScreen();
			mMenuScreen->MarkDirty();
			return;
		}
		if (mTypingTutor == NULL)
			return;
		mTypingTutor->MarkDirty();
		return;

	case 0x12c:
		ReturnToMap();       // 0x429450
		return;

	case 0x1cf3:    // the map screen's start button
		if (!mMapScreen->mFlag224)
			return;
		ChangeMusic(0);
		mMapScreen->ClearNodeWidgets();     // 0x443e20
		StartBoard();                       // 0x431320
		return;

	case 0x1007:    // DeleteUserDialog's cancel
		KillDialog(DIALOG_DELETE_USER);
		return;

	case 0x68:      // quit
		ShowConfirmDialog(GT(0x0052d148), GT(0x0052d154), "", true, GT(0x0052d1f8), GT(0x0052d1fc));   // "LEAVING?", "Are you sure you wish to quit?", "YES", "NO"
		mFlag660 = true;
		return;

	case 0x69:
		ShowSelectUserDialog();     // 0x4128a0
		return;

	case 0xc8:      // pause
		mBoard->SetPaused(!mBoard->mPaused);    // 0x442100 (+0xe60)
		PauseMusic(mBoard->mPaused);
		return;

	case 0xcb:      // the options button of the board
	{
		Board* aBoard = mBoard;
		int aMode = aBoard->mPlayer->mState;    // +0xd68 -> +0xb8
		if ((aMode == 0xc) || (aMode == 7) || ((mLessonManager->IsBonusLevel()) && (aMode == 6)))
			aBoard->mContinueLink->SetVisible(false);
		ShowOptions();      // 0x414150
		return;
	}

	case 0x6b:
	case 0x5b:
		ShowOptions();
		return;

	case 0xca:
		mBoard->KeyChar(0xd);       // slot 20: the board's Enter key
		return;

	case 0x191:     // the title screen's play button
		ShowMapScreen();            // 0x426440
		return;

	case 0x1006:    // SelectUserDialog's cancel
		KillDialog(DIALOG_SELECT_USER);
		return;

	case 0xc1e:     // SelectUserDialog's ok: the chosen name becomes the current user
	{
		SelectUserDialog* aSelectDialog = (SelectUserDialog*) GetDialog(DIALOG_SELECT_USER);
		std::string aName = aSelectDialog->mSelectedName;       // +0x12c
		if (aName != "")
		{
			mProfileMgr->RegisterDefaultUserName(aName);        // 0x419da0
			mUser = mProfileMgr->LoadUser(aName);               // 0x425f20
		}
		if (mUser == NULL)
		{
			// the saved data is missing or damaged: a new, empty profile of that name is made
			ShowMessageDialog(GT(0x0052c6c8), GT(0x0052c6d0), GT(0x0052b4d0), true);

			mUser = new UserProfile(this);                      // 0x421f50 (sizeof 0x2c8)
			mUser->SetName(aName);                              // 0x416380
			mProfileMgr->RegisterDefaultUserName(aName);        // 0x419da0
			mUser->SetUserNumber(mProfileMgr->GetDefaultUserNumber());  // 0x412380, 0x4262b0
			mProfileMgr->WriteUserProfileToRegistry(mUser, false, false);   // 0x41f1d0
			mProfileMgr->SaveUser(mUser);                       // 0x420c60
		}
		KillDialog(DIALOG_SELECT_USER);
		mFlag8f8 = (mUser->mFlag1dc == false);
		return;
	}

	case 0x1009:    // AbyssModeDialog's no
		KillDialog(DIALOG_ABYSS_MODE);
		mAbyssModeDialog = NULL;
		mDifficulty = 0;
		return;

	case 0xc21:     // AbyssModeDialog's yes: the chosen depth
	{
		int aDepth = mAbyssModeDialog->mSelectedDepth;      // +0x130
		mAbyssDepth = aDepth;
		mUser->SetField1e4((aDepth / 1000) % 4 - 1);        // 0x4126e0
		CreateBoard();                                     // 0x42f3c0
		if (aDepth > 0)
			mBoard->SetDepth(aDepth);                       // 0x452cf0
		KillDialog(DIALOG_ABYSS_MODE);
		mAbyssModeDialog = NULL;
		return;
	}

	case 0xad9:     // the difficulty dialog's yes (id 0x309 + 2000)
		mFlag8d8 = false;
		mFlag8d9 = false;
		mOpenDialogCount--;
		KillDialog(DIALOG_DIFFICULTY);
		mDialog61c = NULL;
		return;

	case 0xbc3:     // the hint box (id 0x3f3 + 2000)
		mOpenDialogCount--;
		mUser->mFlag1fc = !mHintDialog->mField124->IsChecked();     // +0x124: the "do not show hints" check box
		mHintsAllowed = mUser->mFlag1fc;
		if ((mBoard != NULL) && (mBoard->mVisible))
		{
			mBoard->SetPaused(false);
			mWidgetManager->SetFocus(mBoard);
		}
		KillDialog(0x3f3);
		mHintDialog = NULL;
		return;

	case 0x85d:     // the map screen's own start button
		if (mMapScreen == NULL)
			return;
		if (!mMapScreen->mVisible)
			return;
		if (mMapScreen->mDisabled)
			return;
		if (!mMapScreen->mFlag234)
			return;
		RestartBoard();       // 0x42ee10
		return;

	case 0xc45:     // WinGameDialog's no (id 0x8d + 3000)
		if (mMapScreen == NULL)
			return;
		if (!mMapScreen->mVisible)
			return;
		if (mMapScreen->mDisabled)
			return;
		if (!mMapScreen->mFlag234)
			return;
		KillDialog(DIALOG_WIN_GAME);
		ButtonDepress(0x1cf3);      // slot 4
		return;

	case 0x9c4:     // the message dialog's ok (id 500 + 2000)
		mOpenDialogCount--;
		KillDialog(DIALOG_MESSAGE);
		mMessageDialog = NULL;
		if (mFlag68c)
		{
			// the question that ends the game was answered "yes"
			if (!mLessonManager->mAbyssMode)    // +0xa5
				SaveGameProgress(true, false);  // 0x426c60
			else
			{
				int aDepth = mBoard->mScore;
				UserProfile* aUser = mUser;
				if (aDepth > aUser->mBestDepth)
					aUser->mBestDepth = aDepth;
				aUser->mLastDepth = aDepth;
			}
			mFlag684 = true;
			mFlag685 = false;
			mFlag68c = false;
			ResetGame(false);       // 0x42a870
			return;
		}
		if (mFlag660)
		{
			// the quit question
			mFlag684 = true;
			mFlag685 = false;
			Shutdown();             // slot 33
			return;
		}
		if (mFlag910)
		{
			mFlag684 = true;
			mFlag685 = false;
			mFlag68c = false;
			mDeleteUserDialog->DeleteSelectedUser();    // 0x43df60
			KillDialog(DIALOG_DELETE_USER);
			mDeleteUserDialog = NULL;
			mFlag910 = false;
			return;
		}
		if (mTypingTutor != NULL)
		{
			mFlag684 = true;
			mFlag685 = false;
			mFlag68c = false;
			if (mTypingTutor->mFlag262)
			{
				mWidgetManager->RemoveWidget(mTypingTutor);
				if (mTypingTutor != NULL)
					delete mTypingTutor;
				mTypingTutor = NULL;
				ResetGame(false);       // 0x42a870
				return;
			}
			if (mTypingTutor->mFlag261)
			{
				ShowStatsScreen(2);     // 0x41b9e0
				return;
			}
		}
		else
		{
			if (mFlag670)
			{
				mFlag670 = false;
				mFlag685 = false;
				mFlag68c = false;
				ButtonDepress(0x66);    // slot 4: the typing test the new user is offered
			}
			else if (mFlag68d)
			{
				// the saved game is continued: the board starts with the state the profile holds
				mFlag684 = true;
				mFlag685 = false;
				mFlag8d8 = false;
				mFlag68d = false;
				mStats->mReserved[8] = 0;                       // +0x24
				mDifficulty = mUser->mField1c4;
				memcpy(&mStats->mReserved[1], &mUser->mDouble1b8, sizeof(double));     // +8
				mStats->mReserved[9] = 0;                       // +0x28
				mStats->mReserved[0] = mUser->mField1c8;        // +4
				mStats->mReserved[3] = mUser->mField1b0;        // +0x10
				mStats->mReserved[5] = 0;                       // +0x18
				mStats->mReserved[4] = 0;                       // +0x14
				mStats->mPointsSoFar = mUser->mField1b4;        // +0
				mField8fc = mUser->mField1cc;
				mField900 = mUser->mField1e0;
				mField908 = mUser->mField1d4;
				mField90c = mUser->mField1d8;
				mField904 = mUser->mField1d0;
				mFlag8f8 = false;
				// 0x41d870 not ported: the play counter of a trial
				WriteToRegistry();      // slot 18
				CreateBoard();         // 0x42f3c0
				mBoard->StartGame(*mStats);     // 0x440130 (the 0x30-byte block by value)
				if (mUser->mField1c0 < 100)
				{
					mLessonManager->SetLevel(mUser->mField1c0);     // 0x4309d0
					if (mUser->mField1c0 >= 0x23)
						mMapScreen->SetFinalLevel(true);            // 0x441080
				}
				else
				{
					mMapScreen->SetFinalLevel(true);
					mMapScreen->ResetScroll();                  // 0x441030
					mLessonManager->SetLevel(mUser->mField1c0 - 0x41);
					mLessonManager->mEndlessCount = mUser->mField1e8;
					mLessonManager->SetLevel(-1);
				}
				mMapScreen->mField1f8 = mUser->mField1c8 + 1;
				mBoard->SetLessonManager(mLessonManager);           // 0x451cb0
				mFlag8f8 = (mUser->mFlag1dc == false);

				// the profile's lists go to the map screen
				unsigned int i = 0;
				if (mUser->mVector2a4.size() != 0)
				{
					do
					{
						ProfilePair* aPair = new ProfilePair;
						ProfilePair* aSource = mUser->mVector2a4[i];
						aPair->mFirst = aSource->mFirst;
						aPair->mSecond = aSource->mSecond;
						mMapScreen->mVector1c4.push_back(aPair);
						i++;
					} while (i < mUser->mVector2a4.size());
				}
				i = 0;
				if (mUser->mVector2b4.size() != 0)
				{
					do
					{
						MapNode* aNode = mMapScreen->mMap1a8[mUser->mVector2b4[i]];     // 0x420280
						mMapScreen->mVector1d4.push_back(aNode);
						mMapScreen->mVector1d4[i]->mVisited = true;
						i++;
					} while (i < mUser->mVector2b4.size());
				}
			}
		}

		// the common end (0x432922)
		mFlag684 = true;
		mFlag685 = false;
		mFlag68c = false;
		if ((mDifficulty == -1) && (mRegisterDialog == NULL) && (mOptionsDialog == NULL))
		{
			CreateBoard();                     // 0x42f3c0
			mBoard->SetDepth(mAbyssDepth);        // 0x452cf0
			return;
		}
		if ((mBoard != NULL) && (mBoard->mVisible) && (mRegisterDialog == NULL))
		{
			mBoard->SetPaused(false);
			mWidgetManager->SetFocus(mBoard);
		}
		if (mMapScreen == NULL)
			return;
		if (!mMapScreen->mVisible)
			return;
		mWidgetManager->SetFocus(mMapScreen);
		return;

	case 0xdac:     // the message dialog's no (id 500 + 3000)
		KillDialog(DIALOG_MESSAGE);
		if (mFlag670)
			mUser->mLastLesson = 1;
		mMessageDialog = NULL;
		mFlag685 = true;
		mFlag684 = false;
		mOpenDialogCount--;
		mFlag670 = false;
		mFlag910 = false;
		mFlag660 = false;
		if (mTypingTutor != NULL)
		{
			mTypingTutor->mFlag261 = false;
			mTypingTutor->MarkDirty();
			mWidgetManager->SetFocus(mTypingTutor);
			return;
		}
		if (mFlag68d)
		{
			// the saved game is not continued: the difficulty dialog
			mFlag68d = false;
			mOpenDialogCount++;
			mDialog61c = new DifficultyDialog(this);        // 0x43f0f0
			mDialog61c->Resize(0xaa, 0x28, 0x12c, mDialog61c->GetPreferredHeight(0));
			AddDialog(DIALOG_DIFFICULTY, mDialog61c);
			return;
		}
		if ((mDifficulty == -1) && (!mFlag68c))
		{
			mFlag68c = false;
			CreateBoard();     // 0x42f3c0
			return;
		}
		mFlag68c = false;
		if (mBoard == NULL)
			return;
		mBoard->SetPaused(false);
		mWidgetManager->SetFocus(mBoard);
		mBoard->MarkDirty();
		return;

	case 0x16c1a:   // GameOverDialog's no (id 0x16062 + 3000)
		KillDialog(DIALOG_GAME_OVER);
		ResetGame(false);       // 0x42a870
		return;

	case 0x16832:   // GameOverDialog's yes (id 0x16062 + 2000)
		KillDialog(DIALOG_GAME_OVER);
		if (mDifficulty != -1)
		{
			RestartBoard();       // 0x42ee10
			return;
		}
		mAbyssDepth = mBoard->mScore;
		ResetGame(true);        // 0x42a870
		CreateBoard();         // 0x42f3c0
		mAbyssDepth = (mAbyssDepth / 400) * 400;
		if (mAbyssDepth < 400)
			return;
		mBoard->SetDepth(mAbyssDepth);    // 0x452cf0
		return;

	case 0xc9:      // end this game
	{
		mFlag68c = true;
		std::string aText;
		if (!mLessonManager->mAbyssMode)
		{
			if (mBoard->mPlayer->mField114 == 0)
				aText = GT(0x0052d010);
			else
				aText = GT(0x0052cf98);
		}
		else
		{
			// the registered game's text (0x52d0f0); the unregistered text (0x52d078) is not ported
			aText = StrFormat(GTC(0x0052d0f0), 400);
		}
		ShowConfirmDialog(GT(0x0052cf90), aText, "", true, GT(0x0052d1f8), GT(0x0052d1fc));    // "CONFIRM", "YES", "NO"
		return;
	}

	case 0xadc:     // the OK button of the credits dialog (id 0x30c + 2000)
		KillDialog(DIALOG_CREDITS);
		return;

	case 0xada:     // the options dialog (id 0x30a + 2000) and its other buttons
	case 0x30a:
	case 0x30b:
	case 0x30c:
	case 0x30d:
	case 0x30e:
	{
		bool aWindowed = !mOptionsDialog->mFullScreenCheckbox->IsChecked();     // +0x140: the full screen check box
		bool a3D = mOptionsDialog->m3DCheckbox->IsChecked();                    // +0x150: the 3D acceleration check box
		mOpenDialogCount = 0;
		mRegisterDialog = NULL;
		if ((aWindowed) && (mForceFullscreen))      // this[0x307]
			ShowNoWindowedModeMessage();            // 0x41d030
		else
			SwitchScreenMode(aWindowed, a3D);       // slot 52
		mFastCPU = mOptionsDialog->mEffectsCheckbox->IsChecked();               // +0x144
		EnableCustomCursors(mOptionsDialog->mCursorsCheckbox->IsChecked());     // +0x14c
		mUser->SetSpacePauses(mOptionsDialog->mSpacePausesCheckbox->IsChecked());   // +0x148; 0x416ed0
		if (mBoard != NULL)
			mBoard->SetFastCPU(mFastCPU);           // 0x440570
		KillDialog(DIALOG_OPTIONS);
		mOptionsDialog = NULL;
		if (mBoard != NULL)
		{
			mWidgetManager->SetFocus(mBoard);
			Board* aBoard = mBoard;
			int aMode = aBoard->mPlayer->mState;
			if ((aMode != 0xc) && (aMode != 7) && ((!mLessonManager->IsBonusLevel()) || (aMode != 6)))
			{
				aBoard->SetPaused(false);
				mBoard->MarkDirty();
				return;
			}
			if ((mLessonManager->IsBonusLevel()) && (aMode == 6))
				aBoard->SetPaused(false);
			mBoard->mContinueLink->SetVisible(true);
			mBoard->MarkDirty();
			return;
		}
		if (mTypingTutor == NULL)
			return;
		mWidgetManager->SetFocus(mTypingTutor);
		mTypingTutor->MarkDirty();
		return;
	}

	case 0xadb:     // the "no windowed mode" message (id 0x30b + 2000)
		mOpenDialogCount--;
		if (mBoard != NULL)
			mBoard->SetPaused(false);
		KillDialog(0x30b);
		return;

	case 0xc1d:     // NewUserDialog's ok (id 0x44d + 2000)
	{
		NewUserDialog* aNewUserDialog = (NewUserDialog*) GetDialog(DIALOG_NEW_USER);
		std::string aName = aNewUserDialog->mNameEdit->mString;     // +0x128 -> +0x5c
		std::string anUpperName = aName;
		for (size_t i = 0; i < anUpperName.size(); i++)             // 0x416620: the name in capitals
		{
			if (('`' < anUpperName[i]) && (anUpperName[i] < '{'))
				anUpperName[i] = (char) (anUpperName[i] - 0x20);
		}

		if (aName.size() == 0)
		{
			ShowMessageDialog(GT(0x0052cf20), GT(0x0052cf30), GT(0x0052b4d0), true);    // "ENTER NAME", "OK"
			return;
		}
		// the two entries of the select dialog's list cannot be names
		if ((anUpperName == GT(0x0052cf0c)) || (anUpperName == GT(0x0052cefc)))     // "(CREATE A NEW USER)", "(DELETE A USER)"
		{
			ShowMessageDialog(GT(0x0052cd94), GT(0x0052cd9c), GT(0x0052b4d0), true);    // "ERROR"
			return;
		}
		if (mProfileMgr->LoadUser(aName) != NULL)       // 0x425f20
		{
			ShowMessageDialog(GT(0x0052cdc8), GT(0x0052cdd8), GT(0x0052b4d0), true);    // "NAME CONFLICT"
			return;
		}

		KillDialog(DIALOG_NEW_USER);
		UserProfile* aProfile = new UserProfile(this);      // 0x421f50 (sizeof 0x2c8)
		mHintsAllowed = aProfile->mFlag1fc;
		aProfile->SetName(aName);                           // 0x416380
		aProfile->SetUserNumber(mProfileMgr->FirstFreeUserNumber());    // 0x412380, 0x413ff0
		mUser = aProfile;
		mProfileMgr->WriteUserProfileToRegistry(aProfile, false, false);    // 0x41f1d0
		mProfileMgr->mUsers.push_back(aProfile);            // 0x420240
		mProfileMgr->RegisterDefaultUserName(aProfile->GetName());      // 0x419da0, 0x415060
		KillDialog(DIALOG_SELECT_USER);
		ShowConfirmDialog(GT(0x0052ce20), GT(0x0052ce30), "", false, GT(0x0052d1f8), GT(0x0052d1fc));  // "ASSESS WPM?", "YES", "NO"
		mFlag670 = true;
		return;
	}

	case 0x1005:    // NewUserDialog's cancel (id 0x44d + 3000)
		KillDialog(DIALOG_NEW_USER);
		if (mProfileMgr->mUsers.size() != 0)       // +0xc of the manager: the count of the list
		{
			if (mProfileMgr->GetCurrentUserName() != "")
				return;
		}
		if (GetDialog(DIALOG_NEW_USER) != NULL)
			return;
		ShowNewUserDialog(GT(0x0052c2d4));      // "Enter your name. (Example: ...)"
		return;

	case 0x66:      // the typing tutor
		// 0x41da80(3): the trial limit, not ported
		mTypingTutor = new TypingTutor(this);       // 0x460b30 (sizeof 0x264)
		mTypingTutor->Resize(0, 0, mWidth, mHeight);
		mTypingTutor->mVisible = true;
		mTypingTutor->mDisabled = false;
		mWidgetManager->AddWidget(mTypingTutor);
		mWidgetManager->SetFocus(mTypingTutor);
		RemoveMenuScreen();     // 0x412af0
		mTypingTutor->MarkDirty();
		return;

	case 0x6a:      // the statistics button
		// 0x41da80(4): the trial limit, not ported
		ShowStatsScreen(0);     // 0x41b9e0
		return;

	case 0x67:      // the high scores button
		// 0x41da80(5): the trial limit, not ported
		mFlag8d8 = false;
		mFlag8d9 = true;
		ShowHighScoreScreen();  // 0x41bbd0
		return;
	}
}
