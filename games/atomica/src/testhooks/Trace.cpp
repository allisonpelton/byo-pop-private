// Test hook: the state trace (-statetrace=<file>). The identical-play pass compares the port with the original exe by the
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
	TraceState* s = mTraceState;
	if (s->mFile == NULL)
		return;

	std::map<std::string, std::string> aNow;
	aNow["board_p"] = Set(mBoard);
	aNow["menu_p"] = Set(mMenu);
	aNow["title_p"] = Set(mTitleScreen);
	aNow["mode"] = Int(mMode);
	aNow["pause"] = Int(mPauseCount);
	aNow["rng_j"] = Int(mRand.J());
	aNow["rng_k"] = Int(mRand.K());
	if (mBoard != NULL)
	{
		Board* b = mBoard;
		aNow["score"] = Int(b->mScore);
		aNow["level"] = Int(b->mMeter.mLevel);
		aNow["molecules"] = Int(b->mMeter.mMoleculesMade);
		aNow["meter_value"] = Bits(b->mMeter.mValue);
		aNow["meter_max"] = Bits(b->mMeter.mMax);
		aNow["meter_tick"] = Int(b->mMeter.mTick);
		aNow["meter_countdown"] = Int(b->mMeter.mCountdown);
		aNow["meter_over"] = Int(b->mMeter.mOverTick);
		aNow["f8c"] = Bits(b->mF8c);
		aNow["swirls"] = Int(b->mSwirls.Size());
		aNow["i90"] = Int(b->mI90);
		aNow["progress"] = Bits(b->mProgress);
		aNow["numcolors_start"] = Int(b->mNumColorsStart);
		aNow["stat_time"] = Int(b->mStat1c4);
		aNow["stat_atoms"] = Int(b->mStat1cc);
		aNow["stat_molecules"] = Int(b->mStat1d0);
		aNow["stat_combo"] = Int(b->mStat1d4);
		aNow["grid_p"] = Set(b->mGrid);
		if (b->mGrid != NULL)
		{
			Grid* g = b->mGrid;
			aNow["combo"] = Int(g->mCombo);
			aNow["numcolors"] = Int(g->mNumColors);
			aNow["g_overtick"] = Int(g->mOverTick);
			aNow["rounds"] = Int(g->mRounds);
			aNow["bonus_atoms"] = Int(g->mI364);
			aNow["selected_p"] = Set(g->mSelected);
			for (int c = 0; c < Grid::COLS; c++)
				for (int r = 0; r < Grid::ROWS; r++)
				{
					char aName[32];
					snprintf(aName, sizeof(aName), "cell_%d_%d", c, r);
					Atom* anAtom = g->mCells[c][r];
					aNow[aName] = anAtom != NULL ? Int(anAtom->mType) : "-";
				}
		}
	}
	for (std::map<std::string, std::string>::iterator i = aNow.begin(); i != aNow.end(); ++i)
	{
		std::string& aLast = s->mLast[i->first];
		if (aLast != i->second)
		{
			fprintf(s->mFile, "%d %s %s\n", mUpdateCount, i->first.c_str(), i->second.c_str());
			aLast = i->second;
		}
	}
	fflush(s->mFile);
}
