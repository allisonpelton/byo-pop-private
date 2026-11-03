// port: the test scripts' own commands (-script, see SexyAppBase.h); the original had none.
//
//   continue   moves to the tutorial's continue button and clicks it (a move, and a click 10 updates later).
//              Logs "script: continue step <step> at <x> <y>", or "script: continue step <step> none" while the
//              step hides the button (or there is no tutorial).
//   step       logs the tutorial's step: "script: step <step> shown <shown step>" (-1 -1 without a tutorial)
//   board      logs the board: "script: board mode <game type> score <score> eggs <egg count> cells <hash>", the
//              hash an FNV-1a of the grid's cells ("script: board none" without a board), so a test can check
//              that a saved game loads as it was saved
//   combo <n>  has Inchy call out a chain of level n (1 combo ... 7 dynomite combo; 0 clears his bubble), as the
//              board's Chain does a match later; logs "script: combo <n>" ("script: combo none" without the side bar)
#include <SDL.h>
#include <cstdlib>
#include <cstring>

#include "CellButton.h"
#include "Dynomite.h"
#include "DynomiteFossil.h"
#include "DynomitePanic.h"
#include "DynomiteStomped.h"
#include "DynomiteTimeTrial.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "InchySmall.h"
#include "Launcher.h"
#include "Egg.h"
#include "SideBar.h"
#include "Steggy.h"
#include "Tutorial.h"

using namespace Sexy;

bool FossilizedApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "continue")
	{
		if ((mTutor == NULL) || (mTutor->mContinueButton->mX < 0) || (mTutor->mStep != mTutor->mShownStep))
		{
			SDL_Log("script: continue step %d none", (mTutor == NULL) ? -1 : mTutor->mStep);
			return true;
		}
		CellButton* aButton = mTutor->mContinueButton;
		std::string aCentre = StrFormat("%d %d", aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2);
		SDL_Log("script: continue step %d at %s", mTutor->mStep, aCentre.c_str());
		AddScriptCommand(mUpdateCount + 1, "move", aCentre);
		AddScriptCommand(mUpdateCount + 10, "click", aCentre);
		return true;
	}
	if (theCommand.mCommand == "step")
	{
		if (mTutor == NULL)
			SDL_Log("script: step -1 shown -1");
		else
			SDL_Log("script: step %d shown %d", mTutor->mStep, mTutor->mShownStep);
		return true;
	}
	if (theCommand.mCommand == "board")
	{
		if ((mBoard == NULL) || (mBoard->mGrid == NULL))
		{
			SDL_Log("script: board none");
			return true;
		}
		unsigned int aHash = 2166136261u;
		const unsigned char* aBytes = (const unsigned char*) &mBoard->mGrid->mCells[0][0];
		for (size_t i = 0; i < sizeof(mBoard->mGrid->mCells); i++)
			aHash = (aHash ^ aBytes[i]) * 16777619u;
		SDL_Log("script: board mode %d score %d eggs %d cells %08x", mBoard->mGameType,
			(mSideBar != NULL) ? mSideBar->mScore : -1, mBoard->mGrid->mEggCount, aHash);
		return true;
	}
	if (theCommand.mCommand == "combo")
	{
		if ((mSideBar == NULL) || (mSideBar->mInchy == NULL))
		{
			SDL_Log("script: combo none");
			return true;
		}
		mSideBar->mInchy->Combo(1, atoi(theCommand.mArgs.c_str()));
		SDL_Log("script: combo %s", theCommand.mArgs.c_str());
		return true;
	}
	return SexyAppBase::HandleScriptCommand(theCommand);
}

void FossilizedApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-statelog")
		mStateLog = fopen(theParamValue.c_str(), "w");
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

// The state after the last update and its draw (the modes draw random numbers while drawing), under that update's number,
// as origrun reads the original's at its wait after the draw
void FossilizedApp::UpdateFrames()
{
	if (mStateLog != NULL)
		LogState();
	SexyApp::UpdateFrames();
}

namespace
{

std::string StateInt(int theValue)
{
	return StrFormat("%d", theValue);
}

std::string StateFloat(float theValue)
{
	unsigned int aBits;
	memcpy(&aBits, &theValue, 4);
	return StrFormat("%d", (int) aBits);
}

}

// The values in case.sh's order; "-" where the original's pointer is null
void FossilizedApp::LogState()
{
	std::vector<std::string> aValues;
	uint32_t aState[MTRAND_N];
	int anIndex;
	mRand.GetState(aState, &anIndex);
	aValues.push_back(StateInt(anIndex));								// mti
	aValues.push_back(StateInt((int) aState[0]));						// mt0
	aValues.push_back(StateInt(mPicker.mI));							// fibi
	aValues.push_back(StateInt(mPicker.mJ));							// fibj
	aValues.push_back((mSideBar != NULL) ? StateInt(mSideBar->mScore) : "-");	// score
	Dynomite* b = mBoard;
	const char* aNone = "-";
	aValues.push_back(b ? StateInt(b->mGameType) : aNone);				// type
	aValues.push_back(b ? StateInt(b->mChain) : aNone);					// chain
	aValues.push_back(b ? StateInt(b->mLongestChain) : aNone);			// longest
	aValues.push_back(b ? StateInt(b->mBabies) : aNone);				// babies
	aValues.push_back(b ? StateInt(b->mBonusBits) : aNone);				// bonus
	aValues.push_back(b ? StateInt(b->mRank) : aNone);					// rank
	aValues.push_back(b ? StateInt(b->mPauseCount) : aNone);			// pause
	aValues.push_back(b ? StateInt(b->mField15c) : aNone);				// fired
	EggGrid* g = b ? b->mGrid : NULL;
	aValues.push_back(g ? StateInt(g->mEggCount) : aNone);				// eggs
	aValues.push_back(g ? StateInt(g->mMaxColor) : aNone);				// maxcolor
	aValues.push_back(g ? StateInt(g->mBottom) : aNone);				// bottom
	aValues.push_back(g ? StateFloat(g->mCeiling) : aNone);				// ceiling
	aValues.push_back(g ? StateFloat(g->mRows) : aNone);				// rows
	aValues.push_back(g ? StateInt(g->mPastLineFrames) : aNone);		// pastline
	Launcher* l = b ? b->mLauncher : NULL;
	aValues.push_back(l ? StateInt(l->mAngle) : aNone);					// angle
	aValues.push_back((l && l->mLoaded) ? StateInt(l->mLoaded->mColor) : aNone);	// loaded
	aValues.push_back((l && l->mHeld) ? StateInt(l->mHeld->mColor) : aNone);		// held
	aValues.push_back(l ? StateInt(l->mFrozen) : aNone);				// frozen
	Steggy* st = b ? b->mSteggy : NULL;
	aValues.push_back((st && st->mNext) ? StateInt(st->mNext->mColor) : aNone);	// next: the egg Steggy holds
	// the mode's own: Stomped, Panic, Fossil, Time Trial (four slots, by the board's type)
	int aType = b ? b->mGameType : -1;
	for (int i = 0; i < 4; i++)
	{
		std::string aValue = aNone;
		if (aType == 0)
		{
			DynomiteStomped* s = (DynomiteStomped*) b;
			int aFields[4] = { s->mRound, s->mShotsLeft, s->mEggIndex, s->mShots };
			aValue = StateInt(aFields[i]);
		}
		else if (aType == 1)
		{
			DynomitePanic* p = (DynomitePanic*) b;
			if (i == 2)
				aValue = StateFloat(p->mKick);
			else
			{
				int aFields[4] = { p->mWhirleyLevel, p->mWhirleysHit, 0, p->mWhirleys.mCount };
				aValue = StateInt(aFields[i]);
			}
		}
		else if (aType == 2)
		{
			DynomiteFossil* f = (DynomiteFossil*) b;
			int aFields[4] = { f->mLevel, f->mRound, f->mPushes, f->mFossilFell };
			aValue = StateInt(aFields[i]);
		}
		else if (aType == 3)
		{
			DynomiteTimeTrial* t = (DynomiteTimeTrial*) b;
			int aFields[4] = { t->mEggsBroken, t->mEggsDropped, t->mRowsCleared, 0 };
			aValue = StateInt(aFields[i]);
		}
		aValues.push_back(aValue);
	}
	static const char* gNames[] = { "mti", "mt0", "fibi", "fibj", "score", "type", "chain", "longest", "babies", "bonus",
		"rank", "pause", "fired", "eggs", "maxcolor", "bottom", "ceiling", "rows", "pastline", "angle", "loaded", "held",
		"frozen", "next", "m0", "m1", "m2", "m3" };
	mStateValues.resize(aValues.size());
	for (size_t i = 0; i < aValues.size(); i++)
	{
		if (aValues[i] != mStateValues[i])
		{
			fprintf(mStateLog, "%d %s %s\n", mUpdateCount, gNames[i], aValues[i].c_str());
			mStateValues[i] = aValues[i];
		}
	}
	fflush(mStateLog);
}
