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
//
// The standard test commands (docs/TEST_COMMANDS.md), each answered with one line on stdout: STATE {...}, MOVE {...},
// OK <command> or ERROR <command>: <reason>. A command that is never given changes nothing.
//
//   state                 STATE {json}: screen (title, menu, board, tutorial), page (the main menu's), dialog (the
//                         topmost dialog's id, 0 for none), score, and on a board: mode (stomped, endless, fossil,
//                         timetrial), paused, over (lost or won, its end running), level (Fossil's fossil with its
//                         round, Stomped's round, Endless's whirley level) and the board: eggs, loaded and next
//                         colours, frozen, and rows (the grid's cells, a row a string of hex colours, '.' empty)
//   goto <screen>         menu, options, highscores, credits, or a mode (endless, stomped, fossil, timetrial, tutorial),
//                         started through the main menu's own pages with their present choices of skill and pack
//   level <n>             Fossil: fossil n (1-15) from its first fragment, as the museum starts one
//   set score <n>         the side bar's score
//   win                   Stomped and Time Trial: every egg goes, so the board's own end follows
//   lose                  the eggs have passed the line: Inchy sinks and the eggs run off, as in play
//   seed <n>              the app's twister and its picker, as the original seeds them from the tick count
//   move                  MOVE {json}: the bot's shot (Bot.h): angle, cell, colour, group and the click that fires it
//   unlock all            ERROR: nothing is locked in the port
#include <SDL.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include "Bot.h"
#include "CellButton.h"
#include "compat2004/Compat2004.h"
#include "IntroScreen.h"
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

namespace
{

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void Ok(const std::string& theCommand)
{
	Reply("OK " + theCommand);
}

void Error(const std::string& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand + ": " + theReason);
}

const char* ModeName(int theGameType)
{
	switch (theGameType)
	{
	case 0:
		return "stomped";
	case 1:
		return "endless";
	case 2:
		return "fossil";
	case 3:
		return "timetrial";
	}
	return "none";
}

int TopDialogId(FossilizedApp* theApp)
{
	int anId = 0;
	for (DialogMap::iterator it = theApp->mDialogMap.begin(); it != theApp->mDialogMap.end(); ++it)
		anId = it->first;
	return anId;
}

std::string StateJson(FossilizedApp* theApp)
{
	std::string s = "{\"screen\":\"";
	Dynomite* b = theApp->mBoard;
	if (theApp->mTitleScreen != NULL)
		s += "title";
	else if (theApp->mTutor != NULL)
		s += "tutorial";
	else if (b != NULL)
		s += "board";
	else if (theApp->mIntroScreen != NULL)
		s += "menu";
	else
		s += "none";
	s += "\"";
	if (theApp->mIntroScreen != NULL)
		s += StrFormat(",\"page\":%d", theApp->mIntroScreen->mPage);
	s += StrFormat(",\"dialog\":%d", TopDialogId(theApp));
	if (theApp->mSideBar != NULL)
		s += StrFormat(",\"score\":%d", theApp->mSideBar->mScore);
	if (b != NULL)
	{
		s += StrFormat(",\"mode\":\"%s\",\"paused\":%d,\"over\":%s", ModeName(b->mGameType), b->mPauseCount,
			(b->mField131 || b->mField132) ? "true" : "false");
		if (b->mGameType == 2)
		{
			DynomiteFossil* f = (DynomiteFossil*) b;
			s += StrFormat(",\"level\":%d,\"round\":%d", f->mLevel, f->mRound);
		}
		else if (b->mGameType == 0)
			s += StrFormat(",\"level\":%d", ((DynomiteStomped*) b)->mRound);
		else if (b->mGameType == 1)
			s += StrFormat(",\"level\":%d", ((DynomitePanic*) b)->mWhirleyLevel);
		EggGrid* g = b->mGrid;
		Launcher* l = b->mLauncher;
		s += StrFormat(",\"board\":{\"eggs\":%d,\"loaded\":%d,\"next\":%d,\"frozen\":%s,\"rows\":[", g->mEggCount,
			(l && l->mLoaded) ? l->mLoaded->mColor : -1,
			(b->mSteggy && b->mSteggy->mNext) ? b->mSteggy->mNext->mColor : -1, (l && l->mFrozen) ? "true" : "false");
		int aRows = (int) g->mRows;
		for (int r = 0; (r <= aRows) && (r < 100); r++)
		{
			std::string aRow;
			for (int c = 0; c < 15; c++)
			{
				int aColor = (int) (g->mCells[c][r].mBits & 0xff);
				aRow += (aColor == 0) ? std::string(".") : StrFormat("%x", aColor & 0xf);
			}
			s += StrFormat("%s\"%s\"", (r == 0) ? "" : ",", aRow.c_str());
		}
		s += "]}";
	}
	return s + "}";
}

// the main menu's own pages, from the start page with the given choice and mode, as its OK button goes through them
bool StartFromMenu(FossilizedApp* theApp, int theStartChoice, int theMode)
{
	if (theApp->mTitleScreen != NULL)
		theApp->TitleScreenDone();
	else if (theApp->mIntroScreen == NULL)
		theApp->ShowMainMenu();
	if (theApp->mIntroScreen == NULL)
		return false;
	theApp->mIntroScreen->mStartChoice = theStartChoice;
	if (theMode != 0)
		theApp->mGameMode = theMode;
	theApp->mIntroScreen->mPage = 0;
	for (int i = 0; (i < 4) && (theApp->mIntroScreen != NULL) && (theApp->mBoard == NULL) && (theApp->mTutor == NULL); i++)
		theApp->mIntroScreen->ButtonDepress(7);
	return (theApp->mBoard != NULL) || (theApp->mTutor != NULL);
}

bool HandleStandardCommand(FossilizedApp* theApp, const std::string& theName, const std::string& theArgs)
{
	std::string aLine = theArgs.empty() ? theName : (theName + " " + theArgs);
	Dynomite* b = theApp->mBoard;
	if (theName == "state")
	{
		Reply("STATE " + StateJson(theApp));
		return true;
	}
	if (theName == "move")
	{
		BotShot aShot;
		if (!BotChooseShot(b, &aShot))
			Reply("MOVE null");
		else
			Reply(StrFormat("MOVE {\"kind\":\"shot\",\"angle\":%d,\"cell\":[%d,%d],\"color\":%d,\"group\":%d,"
				"\"clicks\":[[%d,%d]]}", aShot.mAngle, aShot.mCol, aShot.mRow, aShot.mColor, aShot.mGroup,
				aShot.mClickX, aShot.mClickY));
		return true;
	}
	if (theName == "goto")
	{
		static const char* kModes[] = { "stomped", "endless", "fossil", "timetrial" };
		for (int i = 0; i < 4; i++)
		{
			if (theArgs == kModes[i])
			{
				if (StartFromMenu(theApp, 0, i + 1))
					Ok(aLine);
				else
					Error(aLine, "the main menu did not start it");
				return true;
			}
		}
		if (theArgs == "tutorial")
		{
			if (StartFromMenu(theApp, 2, 0))
				Ok(aLine);
			else
				Error(aLine, "the main menu did not start it");
		}
		else if (theArgs == "menu")
		{
			if (theApp->mTitleScreen != NULL)
				theApp->TitleScreenDone();
			else if (theApp->mIntroScreen == NULL)
				theApp->ShowMainMenu();
			Ok(aLine);
		}
		else if (theArgs == "options")
		{
			theApp->ShowOptions();
			Ok(aLine);
		}
		else if (theArgs == "highscores")
		{
			theApp->ShowHighScores();
			Ok(aLine);
		}
		else if (theArgs == "credits")
		{
			theApp->ShowCredits();
			Ok(aLine);
		}
		else if (theArgs == "title")
			Error(aLine, "the title shows only at the start");
		else
			Error(aLine, "no such screen");
		return true;
	}
	if (theName == "level")
	{
		int aLevel = atoi(theArgs.c_str());
		if ((b == NULL) || (b->mGameType != 2))
			Error(aLine, "only the Fossil Challenge has levels to start");
		else if ((aLevel < 1) || (aLevel > 15))
			Error(aLine, "a fossil is 1 to 15");
		else
		{
			DynomiteFossil* f = (DynomiteFossil*) b;
			f->mLevel = aLevel;
			f->mRound = 1;
			f->StartRound(aLevel, 1);
			Ok(aLine);
		}
		return true;
	}
	if (theName == "unlock")
	{
		Error(aLine, "nothing is locked in the port");
		return true;
	}
	if (theName == "set")
	{
		char aKey[32] = {};
		int aValue = 0;
		if ((sscanf(theArgs.c_str(), "%31s %d", aKey, &aValue) != 2) || (strcmp(aKey, "score") != 0))
			Error(aLine, "only set score <n>");
		else if (theApp->mSideBar == NULL)
			Error(aLine, "no side bar yet");
		else
		{
			theApp->mSideBar->mScore = aValue;
			theApp->mSideBar->MarkDirty();
			Ok(aLine);
		}
		return true;
	}
	if (theName == "lose")
	{
		if ((b == NULL) || b->mField131 || b->mField132)
			Error(aLine, "no game in play");
		else
		{
			// as Dynomite::Update when the eggs have been past the line too long
			if (b->mField130 == false)
				theApp->mSideBar->mInchy->Sink(0);
			b->mField130 = true;
			b->mGrid->mBreakPastLine = true;
			Ok(aLine);
		}
		return true;
	}
	if (theName == "win")
	{
		if ((b == NULL) || b->mField131 || b->mField132)
			Error(aLine, "no game in play");
		else if ((b->mGameType != 0) && (b->mGameType != 3))
			Error(aLine, "Endless and the Fossil Challenge have no win to give");
		else
		{
			b->mGrid->ClearDynamite();
			for (int c = 0; c < 15; c++)
				for (int r = 0; r < 100; r++)
					b->mGrid->mCells[c][r].mBits = 0;
			b->mGrid->CountRows();
			Ok(aLine);
		}
		return true;
	}
	if (theName == "seed")
	{
		unsigned int aSeed = (unsigned int) strtoul(theArgs.c_str(), NULL, 10);
		SeedMTRand2004(theApp->mRand, aSeed);
		theApp->mPicker.Seed(aSeed >> 1);
		Ok(aLine);
		return true;
	}
	return false;
}

}

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
	if (HandleStandardCommand(this, theCommand.mCommand, theCommand.mArgs))
		return true;
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
