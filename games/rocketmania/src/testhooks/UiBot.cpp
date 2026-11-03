// Test hooks for the identical-play runs; see UiBot.h.

#include "UiBot.h"
#include "RocketApp.h"
#include "Board.h"
#include "Dialogs.h"
#include "Effects.h"
#include "Screens.h"
#include "WidgetManager.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "HyperlinkWidget.h"
#include "EditWidget.h"
#include "Common.h"
#include "MTRand.h"

#include <SDL.h>
#include <cstdlib>
#include <algorithm>
#include <queue>

using namespace Sexy;

static std::vector<int> ParseList(const std::string& theValue)
{
	std::vector<int> aList;
	size_t aPos = 0;
	while (aPos < theValue.size())
	{
		aList.push_back(atoi(theValue.c_str() + aPos));
		size_t aComma = theValue.find(',', aPos);
		if (aComma == std::string::npos)
			break;
		aPos = aComma + 1;
	}
	return aList;
}

bool UiBotParams::Parse(const std::string& theName, const std::string& theValue)
{
	if (theName == "-uibot")
	{
		mOn = true;
		mSeed = (unsigned int) strtoul(theValue.c_str(), NULL, 10);
	}
	else if (theName == "-uimodes")
		mModes = ParseList(theValue);
	else if (theName == "-uidiffs")
		mDiffs = ParseList(theValue);
	else if (theName == "-uigames")
		mGames = atoi(theValue.c_str());
	else if (theName == "-uicap")
		mCap = atoi(theValue.c_str());
	else if (theName == "-uistart")
		mStart = atoi(theValue.c_str());
	else if (theName == "-uistop")
		mStop = atoi(theValue.c_str());
	else if (theName == "-uicontinue")
		mContinue = atoi(theValue.c_str());
	else if (theName == "-uiname")
		mName = theValue;
	else if (theName == "-uiscript")
		mScriptFile = theValue;
	else if (theName == "-statelog")
		mStateFile = theValue;
	else
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// StateLog

namespace
{
// the original's watches: name, expression ("B" is the board pointer 0x4f69f8, the app is at 0x4f6460)
const char* const kNames[] =
{
	"mode", "diff", "bupd", "state", "score", "level", "m74", "m78", "m7c", "m80", "coins", "fall", "burn", "burnt", "path",
	"flames", "dropped", "pause", "m36c", "m37c", "rngb", "rngb0", "rngg", "rngg0", "fx", "m370", "m374", "m378", NULL
};
}

StateLog::StateLog(RocketApp* theApp) :
	mApp(theApp),
	mFile(fopen(theApp->mUiParams.mStateFile.c_str(), "w")),
	mStarted(false)
{
}

StateLog::~StateLog()
{
	if (mFile != NULL)
		fclose(mFile);
}

void StateLog::Update()
{
	if (mFile == NULL)
		return;
	Board* aBoard = mApp->mBoard;
	char aBuffer[32];
	for (int i = 0; kNames[i] != NULL; i++)
	{
		const char* aName = kNames[i];
		bool aHasValue = true;
		long long aValue = 0;
		if (i == 0)
			aValue = mApp->mMode;
		else if (i == 1)
			aValue = mApp->mDifficulty;
		else if (i == 22 || i == 23)
		{
			uint32_t aState[MTRAND_N];
			int anIndex = 0;
			GetGlobalRandState(aState, &anIndex);
			aValue = (i == 22) ? anIndex : (long long) aState[0];
		}
		else if (aBoard == NULL)
			aHasValue = false;
		else
		{
			switch (i)
			{
			case 2: aValue = aBoard->mUpdateCnt; break;
			case 3: aValue = aBoard->mState; break;
			case 4: aValue = aBoard->mScore; break;
			case 5: aValue = aBoard->mLevel; break;
			case 6: aValue = aBoard->m74; break;
			case 7: aValue = aBoard->m78; break;
			case 8: aValue = aBoard->m7c; break;
			case 9: aValue = aBoard->m80; break;
			case 10: aValue = aBoard->m8c[7]; break;
			case 11: aValue = aBoard->m2dc; break;
			case 12: aValue = aBoard->m2e8 ? 1 : 0; break;
			case 13: aValue = aBoard->m2ec; break;
			case 14: aValue = aBoard->m2f0; break;
			case 15: aValue = aBoard->m2f4; break;
			case 16: aValue = aBoard->m2f8; break;
			case 17: aValue = aBoard->m350; break;
			case 18: aValue = aBoard->m36c; break;
			case 19: aValue = aBoard->m37c; break;
			case 24: aValue = (long long) aBoard->mEffects->mEffects.size(); break;
			case 25: aValue = aBoard->m370; break;
			case 26: aValue = aBoard->m374; break;
			case 27: aValue = aBoard->m378; break;

			case 20:
			case 21:
			{
				uint32_t aState[MTRAND_N];
				int anIndex = 0;
				aBoard->mRand->GetState(aState, &anIndex);
				aValue = (i == 20) ? anIndex : (long long) aState[0];
				break;
			}
			}
		}
		if (aHasValue)
			snprintf(aBuffer, sizeof(aBuffer), "%d", (int) aValue);
		else
			snprintf(aBuffer, sizeof(aBuffer), "-");
		if (!mStarted || mLast[i] != aBuffer)
		{
			mLast[i] = aBuffer;
			fprintf(mFile, "%d %s %s\n", mApp->mUpdateCount, aName, aBuffer);
		}
	}
	// the grid, for the runs that compare it (tools/identical/values_grid.txt): the rotation and the kind of each cell
	for (int i = 0; i < 108; i++)
	{
		int aRow = (i % 54) / 6;
		int aColumn = i % 6;
		Piece* aPiece = aBoard == NULL ? NULL : aBoard->mGrid[aRow][aColumn];
		if (aBoard == NULL)
			snprintf(aBuffer, sizeof(aBuffer), "-");
		else
			snprintf(aBuffer, sizeof(aBuffer), "%d", aPiece == NULL ? 0 : (i < 54 ? aPiece->mRotation : aPiece->mKind));
		char aName[8];
		snprintf(aName, sizeof(aName), "%c%d%d", i < 54 ? 'r' : 'k', aRow, aColumn);
		if (!mStarted || mLast[28 + i] != aBuffer)
		{
			mLast[28 + i] = aBuffer;
			fprintf(mFile, "%d %s %s\n", mApp->mUpdateCount, aName, aBuffer);
		}
	}
	mStarted = true;
	fflush(mFile);
}
