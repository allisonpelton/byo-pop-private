// port: the test scripts' own commands (-script, see SexyAppBase.h); the original had none.
//
//   continue   moves to the tutorial's continue button and clicks it (a move, and a click 10 updates later).
//              Logs "script: continue step <step> at <x> <y>", or "script: continue step <step> none" while the
//              step hides the button (or there is no tutorial).
//   step       logs the tutorial's step: "script: step <step> shown <shown step>" (-1 -1 without a tutorial)
#include <SDL.h>

#include "CellButton.h"
#include "FossilizedApp.h"
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
	return SexyAppBase::HandleScriptCommand(theCommand);
}
