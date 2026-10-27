// port: the test scripts' own commands (-script, see SexyAppBase.h); the original had none.
//
//   continue   moves to the tutorial's continue button and clicks it (a move, and a click 10 updates later).
//              Logs "script: continue step <step> at <x> <y>", or "script: continue step <step> none" while the
//              step hides the button (or there is no tutorial).
//   step       logs the tutorial's step: "script: step <step> shown <shown step>" (-1 -1 without a tutorial)
//   board      logs the board: "script: board mode <game type> score <score> eggs <egg count> cells <hash>", the
//              hash an FNV-1a of the grid's cells ("script: board none" without a board), so a test can check
//              that a saved game loads as it was saved
#include <SDL.h>

#include "CellButton.h"
#include "Dynomite.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "SideBar.h"
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
	return SexyAppBase::HandleScriptCommand(theCommand);
}
