// The per-frame state log of phase one (docs/COORDINATION.md; games/hammerheads/tools/idplay/idplay.py). Test commands only:
// `trace <file>` starts it, `trace off` ends it. The original runs under origrun, which reads the same values from its memory
// (idplay.py's EXPRESSIONS); the first one that differs is the first difference of the logic.
//
// A line is `TRACE <kind> c=<update count> name=value ...`. The framework's frame (SexyAppBase::Process) is three steps: the
// first runs one tick (Update), the second any more ticks the frame owes and then the frame's UpdateF, the third waits. A tick
// of the first step is kind T (the original's state 2 at its pause), the end of a frame's UpdateF kind F (state 3 with the
// non-draw count 0). Doubles are printed as the two ints that hold them, the low one first, as origrun reads them.

#include "Application.h"
#include "Avatar.h"

#include "Common.h"
#include "Creature.h"
#include "Cubby.h"
#include "Gameplay.h"
#include "GameScreen.h"
#include "Stats.h"
#include "Timer.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

using namespace Sexy;

namespace
{
FILE* sTraceFile = nullptr;

void PutDouble(std::string& theLine, const char* theName, double theValue)
{
	int aWords[2];
	memcpy(aWords, &theValue, sizeof(aWords));
	char aText[64];
	snprintf(aText, sizeof(aText), " %s=%d,%d", theName, aWords[0], aWords[1]);
	theLine += aText;
}

void PutInt(std::string& theLine, const char* theName, int theValue)
{
	char aText[64];
	snprintf(aText, sizeof(aText), " %s=%d", theName, theValue);
	theLine += aText;
}
}

// the cubbies a record lists (a board has fewer)
static const int TRACE_CUBBIES = 10;

// port: starts or ends the state log
bool Application::StartTrace(const std::string& theFile)
{
	if (sTraceFile != nullptr)
	{
		fclose(sTraceFile);
		sTraceFile = nullptr;
	}
	if (theFile.empty() || theFile == "off")
		return true;
	sTraceFile = fopen(theFile.c_str(), "w");
	return sTraceFile != nullptr;
}

// port: one line of the log, for a tick of the frame's first step (T) or the end of a frame (F)
void Application::TraceRecord(char theKind)
{
	if (sTraceFile == nullptr)
		return;

	std::string aLine = "TRACE ";
	aLine += theKind;
	PutInt(aLine, "c", mUpdateCount);

	uint32_t aState[624];
	int anIndex = 0;
	GetRandMTState(aState, &anIndex);
	PutInt(aLine, "mti", anIndex);
	PutInt(aLine, "mt0", (int)aState[0]);
	PutInt(aLine, "mt623", (int)aState[623]);

	// the statistics and the gameplay are made when first asked for: the log looks without making them (an object that does not
	// exist yet is logged as such, and its values as zeros)
	Stats* aStats = PeekStats();
	PutInt(aLine, "hasstats", aStats != nullptr ? 1 : 0);
	for (int i = 0; i < Stats::NUM_STATS; i++)
	{
		char aName[16];
		snprintf(aName, sizeof(aName), "s%d", i);
		PutDouble(aLine, aName, aStats != nullptr ? aStats->Get(i) : 0.0);
	}

	PutInt(aLine, "gs", mGameScreen != nullptr ? 1 : 0);
	if (mGameScreen != nullptr)
	{
		Gameplay* aGameplay = PeekGameplay();
		PutInt(aLine, "hasgp", aGameplay != nullptr ? 1 : 0);
		PutInt(aLine, "level", mGameScreen->mLevel);
		PutInt(aLine, "mode", mGameScreen->mMode);
		PutInt(aLine, "cycle", mGameScreen->mCycle);
		PutInt(aLine, "kindindex", mGameScreen->mKindIndex);
		PutDouble(aLine, "leveltime", mGameScreen->mLevelTime);
		PutInt(aLine, "phase", aGameplay != nullptr ? aGameplay->mPhase : 0);
		PutDouble(aLine, "phasetimer", aGameplay != nullptr ? aGameplay->mPhaseTimer : 0.0);
		PutDouble(aLine, "nextpop", aGameplay != nullptr ? aGameplay->mNextPopTimer : 0.0);
		PutDouble(aLine, "clover", aGameplay != nullptr ? aGameplay->mCloverTimer : 0.0);
		PutDouble(aLine, "strawberry", aGameplay != nullptr ? aGameplay->mStrawberryTimer : 0.0);
		PutDouble(aLine, "doublehit", aGameplay != nullptr ? aGameplay->mDoubleHitTimer : 0.0);
		PutDouble(aLine, "fivetimes", aGameplay != nullptr ? aGameplay->mFiveTimesTimer : 0.0);
		Timer* aTimer = GetTimer();
		PutInt(aLine, "timerphase", aTimer != nullptr ? aTimer->mPhase : 0);
		PutDouble(aLine, "timerleft", aTimer != nullptr ? aTimer->mLeft : 0.0);
		PutDouble(aLine, "timerrate", aTimer != nullptr ? aTimer->mRate : 0.0);

		// the board's cubbies and what stands in them, and the hammer
		PutInt(aLine, "ncubbies", aGameplay != nullptr ? (int)aGameplay->mCubbies.size() : 0);
		for (int i = 0; i < TRACE_CUBBIES; i++)
		{
			Cubby* aCubby = aGameplay != nullptr && i < (int)aGameplay->mCubbies.size() ? aGameplay->GetCubby(i) : nullptr;
			Creature* aCreature = aCubby != nullptr ? aCubby->mCreature : nullptr;
			char aName[24];
			snprintf(aName, sizeof(aName), "cb%d_state", i);
			PutInt(aLine, aName, aCubby != nullptr ? aCubby->mState : 0);
			snprintf(aName, sizeof(aName), "cb%d_rise", i);
			PutDouble(aLine, aName, aCubby != nullptr ? aCubby->mRise : 0.0);
			snprintf(aName, sizeof(aName), "cb%d_kind", i);
			PutInt(aLine, aName, aCreature != nullptr ? aCreature->mKind : 0);
			snprintf(aName, sizeof(aName), "cb%d_cstate", i);
			PutInt(aLine, aName, aCreature != nullptr ? aCreature->mState : 0);
			snprintf(aName, sizeof(aName), "cb%d_age", i);
			PutInt(aLine, aName, aCreature != nullptr ? aCreature->mAge : 0);
			snprintf(aName, sizeof(aName), "cb%d_height", i);
			PutDouble(aLine, aName, aCreature != nullptr ? aCreature->mHeight : 0.0);
			snprintf(aName, sizeof(aName), "cb%d_hvel", i);
			PutDouble(aLine, aName, aCreature != nullptr ? aCreature->mHeightVelocity : 0.0);
			snprintf(aName, sizeof(aName), "cb%d_x", i);
			PutDouble(aLine, aName, aCreature != nullptr ? aCreature->mX : 0.0);
			snprintf(aName, sizeof(aName), "cb%d_y", i);
			PutDouble(aLine, aName, aCreature != nullptr ? aCreature->mY : 0.0);
		}
		Avatar* anAvatar = mGameScreen->mAvatar.IsValid() ? static_cast<Avatar*>(mGameScreen->mAvatar.Get()) : nullptr;
		PutInt(aLine, "swing", anAvatar != nullptr ? anAvatar->mSwing : 0);
		PutDouble(aLine, "swingframe", anAvatar != nullptr ? anAvatar->mFrame : 0.0);
		PutInt(aLine, "clickcount", anAvatar != nullptr ? anAvatar->mClickCount : 0);
	}
	aLine += "\n";
	fputs(aLine.c_str(), sTraceFile);
	fflush(sTraceFile);
}

// port: a tick of the first step is a T line (the frame's UpdateF has not run), a later tick of the frame is not logged
bool Application::DoUpdateFrames()
{
	bool aFirstStep = mUpdateAppState == UPDATESTATE_PROCESS_1;
	bool aResult = SexyApp::DoUpdateFrames();
	if (aFirstStep)
		TraceRecord('T');
	return aResult;
}

// port: the end of a frame's UpdateF is an F line
void Application::DoUpdateFramesF(float theFrac)
{
	SexyApp::DoUpdateFramesF(theFrac);
	TraceRecord('F');
}
