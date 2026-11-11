// port: test hook, the state trace (-statetrace=<file>). The identical-play pass compares the port with the original exe by the
// game's own state: each line of the file is "<update> <name> <value>", written when a value changes. The original's
// values come from origrun's --watch (games/atomica/tools/identical/compare.py has the address of each name), so
// this lists the same fields, read from the port's objects; a pointer is 1 when it is set, a float is its bits, and
// "-" stands for a value that cannot be read (a null pointer on the way), as origrun's watch writes it.

#include "AtomicaApp.h"
#include "Board.h"
#include "Grid.h"
#include "Atom.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

using namespace Sexy;

namespace Sexy
{

struct TraceState
{
	FILE*							mFile;
	std::map<std::string, std::string>	mLast;
};

}

namespace
{

std::string Int(int theValue)
{
	return std::to_string(theValue);
}

std::string Bits(float theValue)
{
	int aBits;
	memcpy(&aBits, &theValue, sizeof(aBits));
	return std::to_string(aBits);
}

std::string Set(const void* thePointer)
{
	return thePointer != NULL ? "1" : "0";
}

}


void AtomicaApp::TraceUpdate()
{

	if (mTraceState == NULL)
	{
		mTraceState = new TraceState();
		mTraceState->mFile = fopen(mTraceFile.c_str(), "w");
		if (mTraceState->mFile == NULL)
			return;
	}
	TraceState* aState = mTraceState;
	if (aState->mFile == NULL)
		return;

	std::map<std::string, std::string> aNow;
	aNow["board_p"] = Set(mBoard);
	aNow["menu_p"] = Set(mMenu);
	aNow["title_p"] = Set(mTitleScreen);
	aNow["mode"] = Int(mMode);
	aNow["pause"] = Int(mPauseCount);
	aNow["rng_j"] = Int(mRand.J());
	aNow["rng_k"] = Int(mRand.K());
	for (int aTable = 0; aTable < 2; aTable++)
	{
		// the high score tables: the five scores, the first four letters of the five names and the best of the day
		for (int i = 0; i < ScoreTable::NUM_SCORES; i++)
		{
			char aName[32];
			int aLetters = 0;
			memcpy(&aLetters, mScoreTables[aTable].mNames[i], sizeof(aLetters));
			snprintf(aName, sizeof(aName), "hs%d_s%d", aTable, i);
			aNow[aName] = Int(mScoreTables[aTable].mScores[i]);
			snprintf(aName, sizeof(aName), "hs%d_n%d", aTable, i);
			aNow[aName] = Int(aLetters);
		}
		char aBest[32];
		snprintf(aBest, sizeof(aBest), "hs%d_best", aTable);
		aNow[aBest] = Int(mScoreTables[aTable].mBestToday);
	}
	if (mBoard != NULL)
	{
		Board* aBoard = mBoard;
		aNow["score"] = Int(aBoard->mScore);
		aNow["level"] = Int(aBoard->mMeter.mLevel);
		aNow["molecules"] = Int(aBoard->mMeter.mMoleculesMade);
		aNow["meter_value"] = Bits(aBoard->mMeter.mValue);
		aNow["meter_max"] = Bits(aBoard->mMeter.mMax);
		aNow["meter_tick"] = Int(aBoard->mMeter.mTick);
		aNow["meter_countdown"] = Int(aBoard->mMeter.mCountdown);
		aNow["meter_over"] = Int(aBoard->mMeter.mOverTick);
		aNow["f8c"] = Bits(aBoard->mStartCountdown);
		aNow["swirls"] = Int(aBoard->mSwirls.Size());
		aNow["i90"] = Int(aBoard->mSwirlTimer);
		aNow["progress"] = Bits(aBoard->mProgress);
		aNow["numcolors_start"] = Int(aBoard->mNumColorsStart);
		aNow["stat_time"] = Int(aBoard->mStatUpdates);
		aNow["stat_atoms"] = Int(aBoard->mStatAtoms);
		aNow["stat_molecules"] = Int(aBoard->mStatMolecules);
		aNow["stat_combo"] = Int(aBoard->mStatBestCombo);
		aNow["grid_p"] = Set(aBoard->mGrid);
		if (aBoard->mGrid != NULL)
		{
			Grid* aGrid = aBoard->mGrid;
			aNow["combo"] = Int(aGrid->mCombo);
			aNow["numcolors"] = Int(aGrid->mNumColors);
			aNow["g_overtick"] = Int(aGrid->mGameOverTick);
			aNow["cursor_x"] = Int(aGrid->mCursorX);
			aNow["cursor_y"] = Int(aGrid->mCursorY);
			aNow["m31c"] = Int(aGrid->mMouseX);
			aNow["m320"] = Int(aGrid->mMouseY);
			aNow["select_glow"] = Int(aGrid->mSelectGlow);
			aNow["rounds"] = Int(aGrid->mRounds);
			aNow["bonus_atoms"] = Int(aGrid->mBiohazards);
			aNow["selected_p"] = Set(aGrid->mSelected);
			for (int c = 0; c < Grid::COLS; c++)
				for (int r = 0; r < Grid::ROWS; r++)
				{
					char aName[32];
					snprintf(aName, sizeof(aName), "cell_%d_%d", c, r);
					Atom* anAtom = aGrid->mCells[c][r];
					aNow[aName] = anAtom != NULL ? Int(anAtom->mType) : "-";
				}
		}
	}
	// what is gone (no board, no grid) reads as unreadable in the original's memory
	for (std::map<std::string, std::string>::iterator i = aState->mLast.begin(); i != aState->mLast.end(); ++i)
		if (aNow.find(i->first) == aNow.end())
			aNow[i->first] = "-";
	for (std::map<std::string, std::string>::iterator i = aNow.begin(); i != aNow.end(); ++i)
	{
		std::string& aLast = aState->mLast[i->first];
		if (aLast != i->second)
		{
			fprintf(aState->mFile, "%d %s %s\n", mUpdateCount, i->first.c_str(), i->second.c_str());
			aLast = i->second;
		}
	}
	fflush(aState->mFile);
}
