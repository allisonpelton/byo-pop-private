// WinWSAppGame.cpp: the part of WinWSApp that starts, restarts and ends games (the original's unnamed WinWSApp methods
// 0x426a40-0x430b20): the reset to the menu, the creation of the lesson manager, board, status bar and map screen, the
// return to the map after a level, the profile's saved game, the random words of a game (0x428d80, 0x42aad0, 0x430b20,
// 0x4292b0, 0x429110) and two helpers of the user manager (0x4262b0, 0x413ff0). The button handler that calls them is in
// WinWSAppButtons.cpp.
//
// Not translated: the demo recording trace 0x4ad5f0 (the first line of 0x4262b0) and the texts of the two error traces
// of 0x428d80 and 0x42aad0 (0x52c860, 0x52c9a8); the erase that follows each stays, but it can not run.
// The fields of the other classes are named after their offsets (mFieldXXX) until their groups name them.

#include "WinWSApp.h"
#include "GameText.h"
#include "Board.h"
#include "MapScreen.h"
#include "StatusBar.h"
#include "UserProfileMgr.h"
#include "UserProfile.h"
#include "LessonManager.h"
#include "WidgetManager.h"
#include "Common.h"
#include <algorithm>

using namespace Sexy;

// port: mVector754 and mVector764 (void*[4]: pad, begin, end, capacity) stand for the original's two vectors of word lists
// (16 bytes per entry in the original; the config job gives them their type). Until then begin and end are read as
// arrays of WordList.
typedef std::vector<std::string> WordList;

static WordList* WordListsBegin(void* const theVector[4])
{
	return (WordList*) theVector[1];
}

static unsigned int WordListCount(void* const theVector[4])
{
	if (theVector[1] == NULL)
		return 0;
	return (unsigned int) ((WordList*) theVector[2] - (WordList*) theVector[1]);
}

// 0x42a870: ends the game in progress and goes back to the menu. theKeepGameMode keeps the chosen mode (+0x690);
// otherwise it is set to 0.
void WinWSApp::ResetGame(bool theKeepGameMode)
{
	mOpenDialogCount = 0;
	ChangeMusic(0x11);
	if (mBoard != NULL)
		mBoard->SaveProgress(false, false);     // 0x440580
	if (mLessonManager != NULL)
		delete mLessonManager;                  // 0x421c50, then free()
	mLessonManager = NULL;
	mField8fc = 0;
	mField900 = 0;
	mField90c = 0;
	if (!theKeepGameMode)
		mField690 = 0;
	mField908 = 0;
	mMap780.clear();
	mUsedLetters.erase(mUsedLetters.begin(), mUsedLetters.end());
	mMap78c.clear();
	mString6d0 = "";
	mString6b4 = "";
	mTreasure = 0;
	GetTreasureIndex();
	GetWordA();         // 0x4292b0 (the result is not used)
	GetWordB();         // 0x429110
	std::vector<int>().swap(mVector730);        // free()
	mField740 = -1;
	mFlag68c = false;
	mFlag684 = false;
	mFlag685 = false;
	mField904 = 0;
	if (mStatusBar != NULL)
	{
		mWidgetManager->RemoveWidget(mStatusBar);
		if (mStatusBar != NULL)
			delete mStatusBar;
		mStatusBar = NULL;
	}
	if (mBoard != NULL)
		mWidgetManager->RemoveWidget(mBoard);   // only taken off here; ShowMenuScreen (0x41d5c0) deletes it and clears mBoard
	if (mMapScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mMapScreen);
		if (mMapScreen != NULL)
			delete mMapScreen;
		mMapScreen = NULL;
	}
	ShowMenuScreen();       // 0x41d5c0
}

// 0x42ee10: restarts the game with a new lesson manager, board, status bar and (if it is gone) map screen
void WinWSApp::RestartBoard()
{
	if (mLessonManager != NULL)
		delete mLessonManager;                  // 0x421c50, then free()
	mField8fc = 0;
	mField900 = 0;
	mField90c = 0;
	mField908 = 0;
	mMap780.clear();
	mUsedLetters.erase(mUsedLetters.begin(), mUsedLetters.end());
	mMap78c.clear();
	mString6d0 = "";
	mString6b4 = "";
	mTreasure = 0;
	GetTreasureIndex();
	GetWordA();         // 0x4292b0
	GetWordB();         // 0x429110
	std::vector<int>().swap(mVector730);        // free()
	mField740 = -1;
	mFlag68c = false;
	mFlag684 = false;
	mFlag685 = false;
	mField904 = 0;
	if (mUser != NULL)
	{
		mUser->SetField1f4(0);      // 0x412640
		mUser->SetField1f8(0);      // 0x412650
	}
	if (GetDialog(DIALOG_GAME_OVER) != NULL)
		KillDialog(DIALOG_GAME_OVER);
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	if (mStatusBar != NULL)
	{
		mWidgetManager->RemoveWidget(mStatusBar);
		SafeDeleteWidget(mStatusBar);
		mStatusBar = NULL;
	}
	if (mMapScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mMapScreen);
		SafeDeleteWidget(mMapScreen);
		mMapScreen = NULL;
	}

	mLessonManager = new LessonManager(this);       // 0x4270e0 (sizeof 0xc0)
	if (mLessonManager->LoadConfig(mField690) == 0) // 0x42bfb0
	{
		if (!mLessonManager->mAbyssMode)    // +0xa5
		{
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
			if (mField690 != mUser->mField280)
			{
				mUser->mField280 = mField690;
				i = 0;
				do
				{
					mUser->mField25c[i] = 0;
					i++;
				} while (i < 0x24);
			}
			mProfileMgr->WriteUserProfileToRegistry(mUser, true, false);    // 0x41f1d0
		}

		mBoard = new Board(this);                   // 0x4544b0 (0xf98 bytes)
		mBoard->SetFastCPU(mFastCPU);               // 0x440570
		mBoard->SetLessonManager(mLessonManager);   // 0x451cb0
		mBoard->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mBoard);
		mWidgetManager->SetFocus(mBoard);
		mBoard->SetVisible(true);

		mStatusBar = new StatusBar(this);           // 0x448a40 (sizeof 0x88)
		int aStatusBarHeight = mImages[IMAGE_STATUSBARX]->GetHeight();
		mStatusBar->Resize(0, mHeight - aStatusBarHeight, mWidth, aStatusBarHeight);
		mWidgetManager->AddWidget(mStatusBar);
		mStatusBar->SetVisible(true);
		mStatusBar->SetDisabled(false);
		mWidgetManager->BringToFront(mStatusBar);

		if (mMapScreen == NULL)
		{
			mMapScreen = new MapScreen(this);       // 0x458d10 (sizeof 0x238)
			mMapScreen->SetVisible(false);
			mMapScreen->SetDisabled(true);
			mMapScreen->Resize(0, 0, mWidth, mHeight);
			mWidgetManager->AddWidget(mMapScreen);
		}
		return;
	}

	Popup(GT(0x0052cd40));      // "There was an error loading the configuration file."
	mLoadingFailed = true;
	if (mLessonManager != NULL)
		delete mLessonManager;
	mLessonManager = NULL;      // port: the original leaves the pointer; nothing reads it after Shutdown
	Shutdown();
}

// 0x42f3c0: a new lesson manager with the configuration of the chosen mode (+0x690), then the board, the map screen
// (hidden, if it is gone) and the status bar
void WinWSApp::CreateBoard()
{
	if (mLessonManager != NULL)
		delete mLessonManager;                  // 0x421c50, then free()
	mLessonManager = new LessonManager(this);   // 0x4270e0 (sizeof 0xc0)
	mFlag8d8 = false;
	GetTreasureIndex();
	GetWordA();         // 0x4292b0
	GetWordB();         // 0x429110
	if (mLessonManager->LoadConfig(mField690) != 0)     // 0x42bfb0
	{
		Popup(GT(0x0052cd40));      // "There was an error loading the configuration file."
		mLoadingFailed = true;
		Shutdown();
		return;
	}

	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		if (mBoard != NULL)
			delete mBoard;
	}
	mBoard = new Board(this);                   // 0x4544b0 (0xf98 bytes)
	mBoard->SetFastCPU(mFastCPU);               // 0x440570
	mBoard->SetLessonManager(mLessonManager);   // 0x451cb0
	mBoard->Resize(0, 0, mWidth, mHeight);

	if (mMapScreen == NULL)
	{
		mMapScreen = new MapScreen(this);       // 0x458d10 (sizeof 0x238)
		mMapScreen->SetVisible(false);
		mMapScreen->SetDisabled(true);
		mMapScreen->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mMapScreen);
	}
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mBoard->SetVisible(true);

	if (mStatusBar == NULL)
	{
		mStatusBar = new StatusBar(this);       // 0x448a40 (sizeof 0x88)
		int aStatusBarHeight = mImages[IMAGE_STATUSBARX]->GetHeight();
		mStatusBar->Resize(0, mHeight - aStatusBarHeight, mWidth, aStatusBarHeight);
		mWidgetManager->AddWidget(mStatusBar);
	}
	mStatusBar->SetVisible(true);
	mStatusBar->SetDisabled(false);
	mWidgetManager->BringToFront(mStatusBar);

	RemoveMenuScreen();     // 0x412af0
	KillDialog(DIALOG_DIFFICULTY);
}

// 0x429450: a level is over: the board's results are added to the game's totals, the board is hidden and the map screen
// comes back
void WinWSApp::ReturnToMap()
{
	mString6d0 = "";
	mString6b4 = "";
	mTreasure = 0;
	GetTreasureIndex();
	GetWordA();         // 0x4292b0
	GetWordB();         // 0x429110

	*mStats = mBoard->GetStats();       // 0x440080 (12 dwords)
	mField8fc += mStats->mReserved[7];              // +0x20
	mField900 += mBoard->mField0e1c;                // +0xe1c
	mField908 += mStats->mReserved[9];              // +0x28
	mField90c += mStats->mReserved[8];              // +0x24
	mField904 = mStats->mReserved[6];               // +0x1c
	mBoard->SetVisible(false);
	mBoard->SetDisabled(true);
	mStatusBar->SetDisabled(true);
	mMapScreen->OnEnter();                          // 0x4533b0
	mWidgetManager->SetFocus(mMapScreen);
	ChangeMusic(0x14);
	mWidgetManager->BringToFront(mMapScreen);
	mMapScreen->SetVisible(true);
	mMapScreen->SetDisabled(false);
	mStatusBar->MarkDirty();
}

// 0x426c60: the game's totals go into the current user's profile (the level reached, the totals of the run, the lists of the
// map screen and the lesson) and the profile is written. theFlag is the flag the profile manager's write takes (and,
// inverted, the first argument of the two result calls); theWithDouble picks the result call that also takes the double of
// the level configuration.
void WinWSApp::SaveGameProgress(bool theFlag, bool theWithDouble)
{
	int aLevel = mLessonManager->mLevel;        // +0x8c
	if (aLevel == -1)
		aLevel = mLessonManager->mField0a8 + 0x64;      // 0x412370
	if (mLessonManager->mAbyssMode)             // +0xa5
		aLevel = 0;

	mUser->mField1e0 = mField900;
	LevelConfig* aConfig = mBoard->mLevelConfig;    // 0x412890: the board's +0xd68 (read again for each argument)
	if (theWithDouble)
	{
		mUser->RecordResultWithDouble(theFlag == false, aConfig->mField110, aConfig->mField0f4, aConfig->mDouble0f8,
			aLevel, mField690, aConfig->mField114);     // 0x412520
	}
	else
	{
		mUser->RecordResult(theFlag == false, aConfig->mField110, aConfig->mField0f4, aLevel, mField690,
			aConfig->mField114);                        // 0x412580
	}

	mUser->mField1cc = mField8fc;
	mUser->mField1d0 = mField904;
	mUser->mField1d4 = mField908;
	mUser->mField1d8 = mField90c;
	if (!mLessonManager->mAbyssMode)
	{
		CopyMapListsToUser();       // 0x426a40
		int aLesson = 0;
		if (mLessonManager != NULL)
			aLesson = mLessonManager->mField0a8 - 1;
		mUser->SetField1e8(aLesson);    // 0x412390
	}
	mProfileMgr->WriteUserProfileToRegistry(mUser, theFlag, false);    // 0x41f1d0
}

// 0x4292b0: the first of the game's two random words: while there is none, one is drawn from the list at +0x6f8 that is not
// in the used set (+0x720); when the set holds every word it starts again. The word is also returned. The draws use the
// game's generator. (The original divides by the list's size; an empty list would fault.)
std::string WinWSApp::GetWordA()
{
	unsigned int aCount = 0;
	if (mString6b4 == "")
	{
		if (!mVector6f4.empty())
			aCount = (unsigned int) mVector6f4.size();
		unsigned int aRand = gGameRand->NextNoAssert();
		mString6b4 = mVector6f4[aRand % aCount];
		while (mMap720[mString6b4])
		{
			if (mVector6f4.empty())
				aCount = 0;
			else
				aCount = (unsigned int) mVector6f4.size();
			aRand = gGameRand->NextNoAssert();
			mString6b4 = mVector6f4[aRand % aCount];
		}
		mMap720[mString6b4] = true;
		unsigned int aListSize = 0;
		if (!mVector6f4.empty())
			aListSize = (unsigned int) mVector6f4.size();
		if (mMap720.size() == aListSize)
			mMap720.clear();
	}
	return mString6b4;
}

// 0x429110: the second random word, the same way from the list at +0x708, the used set +0x714 and the word +0x6d0
std::string WinWSApp::GetWordB()
{
	unsigned int aCount = 0;
	if (mString6d0 == "")
	{
		if (!mVector704.empty())
			aCount = (unsigned int) mVector704.size();
		unsigned int aRand = gGameRand->NextNoAssert();
		mString6d0 = mVector704[aRand % aCount];
		while (mMap714[mString6d0])
		{
			if (mVector704.empty())
				aCount = 0;
			else
				aCount = (unsigned int) mVector704.size();
			aRand = gGameRand->NextNoAssert();
			mString6d0 = mVector704[aRand % aCount];
		}
		mMap714[mString6d0] = true;
		unsigned int aListSize = 0;
		if (!mVector704.empty())
			aListSize = (unsigned int) mVector704.size();
		if (mMap714.size() == aListSize)
			mMap714.clear();
	}
	return mString6d0;
}

// 0x428d80: a random word for a falling letter game: its length is picked from theMinLength .. theMaxLength (-1: 3 and 7; the
// longest is 7) by choosing one of the word lists at +0x758 (entry length - 3) and one of its words that is not in the used
// set at +0x780 and whose first letter is not in mUsedLetters. The word and its letter are then taken. When the letters are
// all taken (26) the game reports an error and returns "".
std::string WinWSApp::GetWord(int theMinLength, int theMaxLength)
{
	if (theMinLength == -1)
		theMinLength = 3;
	if ((theMaxLength == -1) || (theMaxLength > 7))
		theMaxLength = 7;

	if (mUsedLetters.size() >= 0x1a)
	{
		Popup(GT(0x0052c8a8));      // "The game has encountered a problem (GWW 7283) ..."
		return "";                  // 0x52c8a0
	}

	theMinLength = theMinLength - 3;
	int aRange = (theMaxLength - 3) - theMinLength;
	unsigned int aCount = 1;
	if (aRange != 0)
		aCount = aRange;
	unsigned int aListIndex = (gGameRand->NextNoAssert() % aCount) + theMinLength;

	std::string aWord;
	int aTries = -1;
	while (true)
	{
		do
		{
			aTries++;
			WordList& aList = WordListsBegin(mVector754)[aListIndex];
			unsigned int aListSize = 0;
			if (!aList.empty())
				aListSize = (unsigned int) aList.size();
			aWord = aList[gGameRand->NextNoAssert() % aListSize];

			WordList& aListAgain = WordListsBegin(mVector754)[aListIndex];
			if (aListAgain.empty() || ((unsigned int) aTries >= aListAgain.size()) || ((unsigned int) aTries >= mMap780.size()))
				mMap780.clear();
		} while (mMap780[aWord]);

		if (mUsedLetters.size() == 0x1a)
		{
			// the trace "############ERROR: mUsedLetters.size() is 26#################" (0x52c860, 0x4ad5f0) is not ported;
			// the entry test returns for 26 or more letters, so this can not run
			mUsedLetters.erase(mUsedLetters.begin(), mUsedLetters.end());
		}

		unsigned int i = 0;
		if (mUsedLetters.size() == 0)
			break;
		bool aLetterTaken = false;
		while (true)
		{
			if (mUsedLetters[i] == aWord[0])
			{
				aLetterTaken = true;
				break;
			}
			i++;
			if (i >= mUsedLetters.size())
				break;
		}
		if (!aLetterTaken)
			break;
	}

	mMap780[aWord] = true;
	mUsedLetters.append(1, aWord[0]);
	return aWord;
}

// 0x42aad0 (the original's own trace calls it "getthemeword"): a random word of the current theme. The theme (+0x740) is
// chosen once: from all the theme lists at +0x768, or from theMinTheme .. theMaxTheme, and not one of the themes used before
// (+0x730; that list starts again when it holds every theme or the whole range). A word of the theme that is not in
// the used set (+0x780) and whose first letter is not in mUsedLetters is taken; when the theme has no such word left the
// word comes from GetWord (0x428d80) with the two lengths. Returns "" when all 26 letters are taken.
std::string WinWSApp::GetThemeWord(int theMinLength, int theMaxLength, int theMinTheme, int theMaxTheme)
{
	if (theMinLength == -1)
		theMinLength = 3;
	if ((theMaxLength == -1) || (theMaxLength > 7))
		theMaxLength = 7;

	if (mUsedLetters.size() == 0x1a)
		return "";

	if (mField740 == -1)
	{
		unsigned int aUsedCount = 0;
		if (!mVector730.empty())
			aUsedCount = (unsigned int) mVector730.size();
		bool aReset = false;
		if ((mVector764[1] == NULL) || (aUsedCount >= WordListCount(mVector764)))
			aReset = true;
		else if ((theMinTheme != -1) && (theMaxTheme != -1)
			&& !((unsigned int) mVector730.size() < (unsigned int) (theMaxTheme - theMinTheme)))
			aReset = true;
		if (aReset)
			mVector730.clear();     // 0x414d20 (frees)

		unsigned int aTheme;
		if ((theMinTheme == -1) && (theMaxTheme == -1))
			aTheme = gGameRand->NextNoAssert() % WordListCount(mVector764);     // (the original divides by 0 without lists)
		else
		{
			unsigned int aRange = 1;
			if (theMaxTheme - theMinTheme != 0)
				aRange = theMaxTheme - theMinTheme;
			aTheme = (gGameRand->NextNoAssert() % aRange) + theMinTheme;
		}

		if (std::find(mVector730.begin(), mVector730.end(), (int) aTheme) != mVector730.end())
		{
			do
			{
				if ((theMinTheme == -1) && (theMaxTheme == -1))
					aTheme = gGameRand->NextNoAssert() % WordListCount(mVector764);
				else
				{
					unsigned int aRange = 1;
					if (theMaxTheme - theMinTheme != 0)
						aRange = theMaxTheme - theMinTheme;
					aTheme = (gGameRand->NextNoAssert() % aRange) + theMinTheme;
				}
			} while (std::find(mVector730.begin(), mVector730.end(), (int) aTheme) != mVector730.end());
		}

		mField740 = aTheme;
		if (std::find(mVector730.begin(), mVector730.end(), (int) aTheme) == mVector730.end())
			mVector730.push_back(mField740);        // 0x421310
	}

	// the words of the theme, to be taken out one by one as they fail
	WordList aWords;
	unsigned int aIndex = 0;
	while (true)
	{
		WordList& aThemeList = WordListsBegin(mVector764)[mField740];
		if (aIndex >= aThemeList.size())
			break;
		aWords.push_back(aThemeList[aIndex]);
		aIndex++;
	}

	std::string aWord;
	while (true)
	{
		unsigned int aWordCount = 0;
		if (!aWords.empty())
			aWordCount = (unsigned int) aWords.size();
		unsigned int aPick = gGameRand->NextNoAssert() % aWordCount;
		aWord = aWords[aPick];

		if (!mMap780[aWord])
		{
			if (mUsedLetters.size() == 0x1a)
			{
				// the trace "############ERROR: mUsedLetters.size() is 26 in getthemeword#################" (0x52c9a8, 0x4ad5f0) is
				// not ported; the test at the top returns for 26 letters, so this can not run
				mUsedLetters.erase(mUsedLetters.begin(), mUsedLetters.end());
			}

			unsigned int i = 0;
			bool aLetterTaken = false;
			if (mUsedLetters.size() != 0)
			{
				do
				{
					if (mUsedLetters[i] == aWord[0])
					{
						aLetterTaken = true;
						break;
					}
					i++;
				} while (i < mUsedLetters.size());
			}
			if (!aLetterTaken)
			{
				mMap780[aWord] = true;
				mUsedLetters.append(1, aWord[0]);
				return aWord;
			}
		}

		// this word is no good: the other words of the theme are tried
		aWords.erase(aWords.begin() + aPick);
		if (aWords.empty())
			break;
	}

	return GetWord(theMinLength, theMaxLength);     // 0x428d80
}

// 0x430b20: a random word of the lists at +0x774 (by word length; the lengths 3 .. 0x15), that is not in the used set at
// +0x78c. The length is picked from theMinLength .. theMaxLength (-1: 3 and 0x15; the longest is 0x15) and is picked again
// whenever the lists of the length are used up in turn (as many tries as the list has words); the length drawn last
// goes to +0x6b0.
std::string WinWSApp::GetLongWord(int theMinLength, int theMaxLength)
{
	if (theMinLength == -1)
		theMinLength = 3;
	if ((theMaxLength == -1) || (theMaxLength > 0x15))
		theMaxLength = 0x15;

	int aRange = theMaxLength - theMinLength;
	unsigned int aCount = 1;
	if (aRange != 0)
		aCount = aRange;
	int aLength = (gGameRand->NextNoAssert() % aCount) + theMinLength;

	std::string aWord;
	int aTries = 0;
	unsigned int aListSize = 0;
	while (true)
	{
		aListSize = 0;
		std::vector<std::string>& aList = mMap774[aLength];     // 0x42f930
		if (!aList.empty())
			aListSize = (unsigned int) aList.size();
		aTries++;
		if (aTries >= (int) aListSize)
		{
			// every try of this length was made: the length is drawn again
			unsigned int aNewCount = aRange;
			if (aRange == 0)
				aNewCount = 1;
			aLength = (gGameRand->NextNoAssert() % aNewCount) + theMinLength;
			std::vector<std::string>& aNewList = mMap774[aLength];
			if (aNewList.empty())
				aListSize = 0;
			else
				aListSize = (unsigned int) aNewList.size();
			aTries = 0;
		}
		aWord = mMap774[aLength][gGameRand->NextNoAssert() % aListSize];
		if (!mMap78c[aWord])
			break;
	}

	mMap78c[aWord] = true;
	mField6b0 = aLength;
	return aWord;
}

// 0x426a40: the map screen's lists go into the user's profile: the pairs of the screen's list at +0x1c4 (each copied) and
// the ids (+0x24) of the nodes in its list at +0x1d4
void WinWSApp::CopyMapListsToUser()
{
	unsigned int aPairIndex = 0;
	unsigned int aNodeIndex = 0;

	while (true)
	{
		if (aPairIndex >= mUser->mVector2a4.size())
			break;
		delete mUser->mVector2a4[aPairIndex];       // free()
		aPairIndex++;
	}
	mUser->mVector2a4.clear();

	while (true)
	{
		if (aNodeIndex >= mMapScreen->mVector1c4.size())
			break;
		ProfilePair* aSource = mMapScreen->mVector1c4[aNodeIndex];
		ProfilePair* aPair = new ProfilePair;
		aPair->mFirst = aSource->mFirst;
		aPair->mSecond = aSource->mSecond;
		mUser->mVector2a4.push_back(aPair);
		aNodeIndex++;
	}

	mUser->mVector2b4.clear();
	aNodeIndex = 0;
	while (true)
	{
		if (aNodeIndex >= mMapScreen->mVector1d4.size())
			break;
		mUser->mVector2b4.push_back(mMapScreen->mVector1d4[aNodeIndex]->mField24);
		aNodeIndex++;
	}
}

// 0x4262b0 (a UserProfileMgr method, listed here by the job): the number of the current user: the number the registry holds
// for the current user name, or the first free number when there is no such user
int UserProfileMgr::GetDefaultUserNumber()
{
	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::GetDefaultUserNumber"
	int aNumber = FirstFreeUserNumber();        // 0x413ff0
	std::string aName = GetCurrentUserName();   // 0x4150a0
	if (aName != "")
	{
		UserProfile* aProfile = LoadUserProfileFromRegistry(aName);     // 0x425390
		if (aProfile != NULL)
		{
			aNumber = aProfile->mUserNumber;    // +0x30
			delete aProfile;                    // 0x417170, free()
		}
	}
	return aNumber;
}

// 0x413ff0 (a UserProfileMgr method, listed here by the job): counts up through the users in list order, one for each
// user whose number is not above the count so far (the first number no user has, when the list is in order)
int UserProfileMgr::FirstFreeUserNumber()
{
	int aNumber = 0;
	std::list<UserProfile*>::iterator anItr = mUsers.begin();
	while (anItr != mUsers.end())
	{
		if ((*anItr)->mUserNumber <= aNumber)
			aNumber = aNumber + 1;
		++anItr;
	}
	return aNumber;
}
