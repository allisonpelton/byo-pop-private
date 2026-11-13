// The per-frame state log of phase one (docs/COORDINATION.md; games/hammerheads/tools/idplay/idplay.py). Test commands only:
// `trace <file>` starts it, `trace off` ends it. The original runs under origrun, which reads the same values from its memory
// (idplay.py's EXPRESSIONS); the first one that differs is the first difference of the logic.
//
// A line is `TRACE <kind> c=<update count> name=value ...`. The framework's frame (SexyAppBase::Process) is three steps: the
// first runs one tick (Update), the second any more ticks the frame owes and then the frame's UpdateF, the third waits. A tick
// of the first step is kind T (the original's state 2 at its pause), the end of a frame's UpdateF kind F (state 3 with the
// non-draw count 0). Doubles are printed as the two ints that hold them, the low one first, as origrun reads them.

#include "Application.h"

#include "Common.h"
#include "Gameplay.h"
#include "GameScreen.h"
#include "Stats.h"

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

	Stats* aStats = GetStats();
	for (int i = 0; i < Stats::NUM_STATS; i++)
	{
		char aName[16];
		snprintf(aName, sizeof(aName), "s%d", i);
		PutDouble(aLine, aName, aStats->Get(i));
	}

	PutInt(aLine, "gs", mGameScreen != nullptr ? 1 : 0);
	if (mGameScreen != nullptr)
	{
		Gameplay* aGameplay = GetGameplay();
		PutInt(aLine, "level", mGameScreen->mLevel);
		PutInt(aLine, "mode", mGameScreen->mMode);
		PutInt(aLine, "cycle", mGameScreen->mCycle);
		PutInt(aLine, "kindindex", mGameScreen->mKindIndex);
		PutDouble(aLine, "leveltime", mGameScreen->mLevelTime);
		PutInt(aLine, "phase", aGameplay->mPhase);
		PutDouble(aLine, "phasetimer", aGameplay->mPhaseTimer);
		PutDouble(aLine, "nextpop", aGameplay->mNextPopTimer);
		PutDouble(aLine, "clover", aGameplay->mCloverTimer);
		PutDouble(aLine, "strawberry", aGameplay->mStrawberryTimer);
		PutDouble(aLine, "doublehit", aGameplay->mDoubleHitTimer);
		PutDouble(aLine, "fivetimes", aGameplay->mFiveTimesTimer);
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
