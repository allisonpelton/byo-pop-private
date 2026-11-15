// WinWSAppButtons.cpp: WinWSApp::ButtonDepress (0x431630, slot 4), the application's button and dialog handler. Every
// button of the game's screens and every dialog button (ids 2000 + dialog id for yes/ok, 3000 + dialog id for no/cancel,
// Dialog2004) comes here.
//
// Left out (registration, trial and update code, removed): the original's ids 0x190 (Register), 0x11b3, 0x11b5, 0x159b,
// 0x159d, 0x38c4 (the update dialogs' buttons, which also opened the update URL), 0x3cac, 0x5fd4, 0xa28 and 0xe10, and its
// trial-limit tests (0x41da80, 0x41d870) in the cases of 0x64, 0x65, 0x66, 0x67, 0x6a and 0x9c4. No id of this handler leads out.

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
			mDifficultyDialog = NULL;
			mDifficulty = theId - 600;
			mUser->mCompletionPending = true;

			int i = 0;
			do
			{
				mUser->mGameGemsFound[i] = 0;
				i++;
			} while (i < 6);
			i = 0;
			do
			{
				mUser->mGameSecretsFound[i] = 0;
				i++;
			} while (i < 4);

			if (mDifficulty != mUser->mVisitedDifficulty)
			{
				mUser->mVisitedDifficulty = mDifficulty;
				i = 0;
				do
				{
					mUser->mVisitedLevels[i] = 0;
					i++;
				} while (i < 0x24);
			}
			mProfileMgr->WriteUserProfileToRegistry(mUser, true, false);    // 0x41f1d0
		}
		else
		{
			mDifficulty = -1;
			mAbyssDepth = (mUser->mBestDepth / 400) * 400;
			if (mAbyssDepth >= 400)
			{
				ShowAbyssModeDialog();      // 0x4129a0
				return;
			}
		}
		mUser->SetGameSharksKilled(0);      // 0x412640
		mUser->SetGamePiranhasKilled(0);      // 0x412650
		CreateBoard();             // 0x42f3c0
		return;

	case 0x64:
	{
		mAdventureStarting = true;
		mHighScoreScreenUp = false;
		int aSavedLevel = mUser->mSavedLevel;
		if ((aSavedLevel >= 1) && (aSavedLevel <= 0x23))
		{
			// a saved adventure game
			mContinueGameAsked = true;
			int aDifficulty = mUser->mSavedDifficulty;
			int aLevel = mUser->mSavedLevelNumber;
			std::string aText = StrFormat(GTC(0x0052d200), aLevel + 1, GetDifficultyName(aDifficulty).c_str());
			ShowConfirmDialog(GT(0x0052d1ec), aText, "", true, GT(0x0052d1f8), GT(0x0052d1fc));     // "CONTINUE?", "YES", "NO"
			return;
		}
		if ((aSavedLevel == -1) || (aSavedLevel == 0))
		{
			// nothing saved: the difficulty dialog
			mOpenDialogCount++;
			mDifficultyDialog = new DifficultyDialog(this);        // 0x43f0f0 (sizeof 0x158)
			mDifficultyDialog->Resize(0xaa, 0x28, 0x12c, mDifficultyDialog->GetPreferredHeight(0));
			AddDialog(DIALOG_DIFFICULTY, mDifficultyDialog);
			mWidgetManager->SetFocus(mDifficultyDialog);
			return;
		}

		// a saved game of the abyss mode (the level numbers above the adventure's)
		mContinueGameAsked = true;
		int aDifficulty = mUser->mSavedDifficulty;
		int aLevel = mUser->mSavedLevelNumber;
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
		mTypingTutor->mViewStatsAsked = false;
		mTypingTutor->SetVisible(true);
		mWidgetManager->SetFocus(mTypingTutor);
		return;

	case DIALOG_HIGHSCORE_CLOSE:    // 0xccf: the Close This Screen button of the high score screen
		mHighScoreScreenUp = false;
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
		if (!mMapScreen->mLevelChosen)
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
		mQuitAsked = true;
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

	case 0x191:     // the end of the loading screen (the original's continue button of the title screen)
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
		mTutorialUnseen = (mUser->mSeenTutorial == false);
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
		mUser->SetAbyssBossIndex((aDepth / 1000) % 4 - 1);        // 0x4126e0
		CreateBoard();                                     // 0x42f3c0
		if (aDepth > 0)
			mBoard->SetDepth(aDepth);                       // 0x452cf0
		KillDialog(DIALOG_ABYSS_MODE);
		mAbyssModeDialog = NULL;
		return;
	}

	case 0xad9:     // the difficulty dialog's yes (id 0x309 + 2000)
		mAdventureStarting = false;
		mHighScoreScreenUp = false;
		mOpenDialogCount--;
		KillDialog(DIALOG_DIFFICULTY);
		mDifficultyDialog = NULL;
		return;

	case 0xbc3:     // the hint box (id 0x3f3 + 2000)
		mOpenDialogCount--;
		mUser->mHintBoxesWanted = !mHintDialog->mDisableHintsCheckbox->IsChecked();     // +0x124: the "do not show hints" check box
		mHintsAllowed = mUser->mHintBoxesWanted;
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
		if (!mMapScreen->mFinalScrollStarted)
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
		if (!mMapScreen->mFinalScrollStarted)
			return;
		KillDialog(DIALOG_WIN_GAME);
		ButtonDepress(0x1cf3);      // slot 4
		return;

	case 0x9c4:     // the message dialog's ok (id 500 + 2000)
		mOpenDialogCount--;
		KillDialog(DIALOG_MESSAGE);
		mMessageDialog = NULL;
		if (mEndGameAsked)
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
			mLastAnswerYes = true;
			mLastAnswerNo = false;
			mEndGameAsked = false;
			ResetGame(false);       // 0x42a870
			return;
		}
		if (mQuitAsked)
		{
			// the quit question
			mLastAnswerYes = true;
			mLastAnswerNo = false;
			Shutdown();             // slot 33
			return;
		}
		if (mDeleteUserAsked)
		{
			mLastAnswerYes = true;
			mLastAnswerNo = false;
			mEndGameAsked = false;
			mDeleteUserDialog->DeleteSelectedUser();    // 0x43df60
			KillDialog(DIALOG_DELETE_USER);
			mDeleteUserDialog = NULL;
			mDeleteUserAsked = false;
			return;
		}
		if (mTypingTutor != NULL)
		{
			mLastAnswerYes = true;
			mLastAnswerNo = false;
			mEndGameAsked = false;
			if (mTypingTutor->mLeaveAsked)
			{
				mWidgetManager->RemoveWidget(mTypingTutor);
				if (mTypingTutor != NULL)
					delete mTypingTutor;
				mTypingTutor = NULL;
				ResetGame(false);       // 0x42a870
				return;
			}
			if (mTypingTutor->mViewStatsAsked)
			{
				ShowStatsScreen(2);     // 0x41b9e0
				return;
			}
		}
		else
		{
			if (mTypingTestAsked)
			{
				mTypingTestAsked = false;
				mLastAnswerNo = false;
				mEndGameAsked = false;
				ButtonDepress(0x66);    // slot 4: the typing test the new user is offered
			}
			else if (mContinueGameAsked)
			{
				// the saved game is continued: the board starts with the state the profile holds
				mLastAnswerYes = true;
				mLastAnswerNo = false;
				mAdventureStarting = false;
				mContinueGameAsked = false;
				mStats->mReserved[8] = 0;                       // +0x24
				mDifficulty = mUser->mSavedDifficulty;
				memcpy(&mStats->mReserved[1], &mUser->mSavedEmpCharge, sizeof(double));     // +8
				mStats->mReserved[9] = 0;                       // +0x28
				mStats->mReserved[0] = mUser->mSavedLevelNumber;        // +4
				mStats->mReserved[3] = mUser->mSavedLives;        // +0x10
				mStats->mReserved[5] = 0;                       // +0x18
				mStats->mReserved[4] = 0;                       // +0x14
				mStats->mPointsSoFar = mUser->mSavedScore;        // +0
				mGameTimeSoFar = mUser->mSavedGameTime;
				mTypingTimeSoFar = mUser->mSavedTypingTime;
				mLettersWrongSoFar = mUser->mSavedLettersWrong;
				mLettersRightSoFar = mUser->mSavedLettersRight;
				mPausedTimeSoFar = mUser->mSavedPausedTime;
				mTutorialUnseen = false;
				WriteToRegistry();      // slot 18
				CreateBoard();         // 0x42f3c0
				mAutosplitContinued = true;     // port: the autosplit observer's flag for a saved game continued
				mBoard->StartGame(*mStats);     // 0x440130 (the 0x30-byte block by value)
				if (mUser->mSavedLevel < 100)
				{
					mLessonManager->SetLevel(mUser->mSavedLevel);     // 0x4309d0
					if (mUser->mSavedLevel >= 0x23)
						mMapScreen->SetFinalLevel(true);            // 0x441080
				}
				else
				{
					mMapScreen->SetFinalLevel(true);
					mMapScreen->ResetScroll();                  // 0x441030
					mLessonManager->SetLevel(mUser->mSavedLevel - 0x41);
					mLessonManager->mEndlessCount = mUser->mEndlessLevelsPlayed;
					mLessonManager->SetLevel(-1);
				}
				mMapScreen->mLevelNumberShown = mUser->mSavedLevelNumber + 1;
				mBoard->SetLessonManager(mLessonManager);           // 0x451cb0
				mTutorialUnseen = (mUser->mSeenTutorial == false);

				// the profile's lists go to the map screen
				unsigned int i = 0;
				if (mUser->mSavedRouteDots.size() != 0)
				{
					do
					{
						ProfilePair* aPair = new ProfilePair;
						ProfilePair* aSource = mUser->mSavedRouteDots[i];
						aPair->mFirst = aSource->mFirst;
						aPair->mSecond = aSource->mSecond;
						mMapScreen->mRouteDots.push_back(aPair);
						i++;
					} while (i < mUser->mSavedRouteDots.size());
				}
				i = 0;
				if (mUser->mSavedRouteNodeIds.size() != 0)
				{
					do
					{
						MapNode* aNode = mMapScreen->mNodesById[mUser->mSavedRouteNodeIds[i]];     // 0x420280
						mMapScreen->mRouteNodes.push_back(aNode);
						mMapScreen->mRouteNodes[i]->mVisited = true;
						i++;
					} while (i < mUser->mSavedRouteNodeIds.size());
				}
			}
		}

		// the common end (0x432922)
		mLastAnswerYes = true;
		mLastAnswerNo = false;
		mEndGameAsked = false;
		if ((mDifficulty == -1) && (mOptionsDialog == NULL))
		{
			CreateBoard();                     // 0x42f3c0
			mBoard->SetDepth(mAbyssDepth);        // 0x452cf0
			return;
		}
		if ((mBoard != NULL) && (mBoard->mVisible))
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
		if (mTypingTestAsked)
			mUser->mLastLesson = 1;
		mMessageDialog = NULL;
		mLastAnswerNo = true;
		mLastAnswerYes = false;
		mOpenDialogCount--;
		mTypingTestAsked = false;
		mDeleteUserAsked = false;
		mQuitAsked = false;
		if (mTypingTutor != NULL)
		{
			mTypingTutor->mViewStatsAsked = false;
			mTypingTutor->MarkDirty();
			mWidgetManager->SetFocus(mTypingTutor);
			return;
		}
		if (mContinueGameAsked)
		{
			// the saved game is not continued: the difficulty dialog
			mContinueGameAsked = false;
			mOpenDialogCount++;
			mDifficultyDialog = new DifficultyDialog(this);        // 0x43f0f0
			mDifficultyDialog->Resize(0xaa, 0x28, 0x12c, mDifficultyDialog->GetPreferredHeight(0));
			AddDialog(DIALOG_DIFFICULTY, mDifficultyDialog);
			return;
		}
		if ((mDifficulty == -1) && (!mEndGameAsked))
		{
			mEndGameAsked = false;
			CreateBoard();     // 0x42f3c0
			return;
		}
		mEndGameAsked = false;
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
		mEndGameAsked = true;
		std::string aText;
		if (!mLessonManager->mAbyssMode)
		{
			if (mBoard->mPlayer->mLevelNumber == 0)
				aText = GT(0x0052d010);
			else
				aText = GT(0x0052cf98);
		}
		else
		{
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
		mHintsAllowed = aProfile->mHintBoxesWanted;
		aProfile->SetName(aName);                           // 0x416380
		aProfile->SetUserNumber(mProfileMgr->FirstFreeUserNumber());    // 0x412380, 0x413ff0
		mUser = aProfile;
		mProfileMgr->WriteUserProfileToRegistry(aProfile, false, false);    // 0x41f1d0
		mProfileMgr->mUsers.push_back(aProfile);            // 0x420240
		mProfileMgr->RegisterDefaultUserName(aProfile->GetName());      // 0x419da0, 0x415060
		KillDialog(DIALOG_SELECT_USER);
		ShowConfirmDialog(GT(0x0052ce20), GT(0x0052ce30), "", false, GT(0x0052d1f8), GT(0x0052d1fc));  // "ASSESS WPM?", "YES", "NO"
		mTypingTestAsked = true;
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
		ShowStatsScreen(0);     // 0x41b9e0
		return;

	case 0x67:      // the high scores button
		mAdventureStarting = false;
		mHighScoreScreenUp = true;
		ShowHighScoreScreen();  // 0x41bbd0
		return;
	}
}
