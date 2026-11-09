// port: test hook -statelog=<file> (docs/TEST_COMMANDS.md): one line per game update while a Board exists, for comparing the
// port with the original's memory watches:
//   u=<update count> lvl=<n> score=<n> time=<n> sharks=<n> piranhas=<n> clams=<n> bonus=<n> boss=<0/1> typed=<n> lives=<n> |
//   <kind>:<word>@<x>,<y>,<state> ...
// the creatures in the order sharks, piranhas, clams, bonus creatures, boss, boss shots, each list in the order the creatures
// were made (the board's lists are never reordered; the game keeps no creation order across lists). Without the option the
// only cost is one test of an integer per update.

#include "TestHooks.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Player.h"
#include "Clam.h"
#include "LessonManager.h"
#include "WSTimer.h"
#include "TypingTutor.h"
#include "Common.h"

#include <cstdio>
#include <cstring>

using namespace Sexy;

namespace
{

int gStateLogMode = -1;		// -1 not looked at yet, 0 no -statelog, 1 on
FILE* gStateLogFile = NULL;
int gStateLogLines = 0;
bool gStateLogDirty = false;

struct StateLogCloser
{
	~StateLogCloser()
	{
		if (gStateLogFile != NULL)
			fclose(gStateLogFile);
		gStateLogFile = NULL;
	}
} gStateLogCloser;

}  // namespace

std::string Sexy::TestStateLogLine(WinWSApp* app)
{
	Board* b = app->mBoard;
	if (b == NULL)
		return "";
	int aCounts[6] = { 0, 0, 0, 0, 0, 0 };
	std::vector<BotCreature> aAll = TestCollectCreatures(b);
	std::string aList;
	size_t aClamNumber = 0;
	for (size_t i = 0; i < aAll.size(); i++)
	{
		const BotCreature& c = aAll[i];
		int aTyped = c.mTyped;
		if (c.mList == "clams")
		{
			// a clam's last field packs what is its own: the kind, the frame, the opened and pulse flags and the tick modulo
			// the frame delay (P1b: the original's Clam fields are read at +0xe8 and +0xf0, its GameObject's frame and tick)
			Clam* aClam = dynamic_cast<Clam*>(b->mClams[aClamNumber++]);
			if (aClam != NULL)
				aTyped = aClam->mType * 100000 + aClam->mFrame * 1000 + (aClam->mOpened ? 100 : 0) + (aClam->mPulseUp ? 10 : 0) +
					aClam->mTick % 4;
		}
		if (c.mList == "sharks")
			aCounts[0]++;
		else if (c.mList == "piranhas")
			aCounts[1]++;
		else if (c.mList == "clams")
			aCounts[2]++;
		else if (c.mList == "bonus")
			aCounts[3]++;
		else if (c.mList == "boss")
			aCounts[4]++;
		aList += StrFormat(" %s:%s@%.6f,%.6f,%d,%d", c.mKind.c_str(), c.mWord.c_str(), c.mX, c.mY, c.mState, aTyped);
	}
	int anOpenClam = -1;		// the index of the clam the board has open (the pearl waiting for its pair)
	for (size_t i = 0; i < b->mClams.size(); i++)
		if (b->mClams[i] == b->mOpenClam)
			anOpenClam = (int) i;
	int aLevel = app->mLessonManager != NULL ? app->mLessonManager->mLevel : -2;
	uint32_t aRandState[MTRAND_N];
	int aRandIndex = 0;
	gGameRand->GetState(aRandState, &aRandIndex);
	// the fields after `lives` are what phase 1 of the identical-play pass reads out of the original's memory to compare
	// (games/typershark/tools/phase1): the board's own update count, its score (the depth in an abyss game), the game timer's
	// three raw values, the pause and hint flags, the typed letters, the dive, the diver's state and EMP charge
	return StrFormat("u=%d lvl=%d score=%d time=%d sharks=%d piranhas=%d clams=%d bonus=%d boss=%d typed=%d lives=%d bupd=%d bscore=%d "
		"tcnt=%d tstart=%d tpaused=%d paused=%d hint=%d ty=%s dive=%.6f dstate=%d emp=%.6f rng=%d gameover=%d sw=%s swp=%d pairs=%d open=%d f38=%.6f diff=%d e94=%d cwt=%d |%s", app->mUpdateCount,
		aLevel, b->mPlayer->mScore, b->mGameTimer->Elapsed(), aCounts[0], aCounts[1], aCounts[2], aCounts[3], aCounts[4] > 0 ? 1 : 0,
		(int) b->mTyped.size(), b->mPlayer->mField110, b->mUpdateCount, b->mScore, b->mGameTimer->mUpdateCnt, b->mGameTimer->mStart,
		b->mGameTimer->mPausedTotal, b->mPaused ? 1 : 0, app->mHintDialog != NULL ? 1 : 0, b->mTyped.c_str(), b->mDivePos,
		b->mPlayer->mState, b->mPlayer->mEmpCharge, aRandIndex, b->mGameOverDialog != NULL ? 1 : 0, b->mString0f10.c_str(), b->mField0f2c, b->mPairsFound, anOpenClam, b->mField0f38, app->mDifficulty, b->mField0e94, app->mLessonManager->GetCurrentWaveType(), aList.c_str());
}

// the framework's update, then (with -statelog) the line for this update
bool WinWSApp::DoUpdateFrames()
{
	bool aResult = SexyApp::DoUpdateFrames();
	TestTypistStep(this);
	if (gStateLogMode == 0)
		return aResult;

	if (gStateLogMode < 0)
	{
		gStateLogMode = 0;
		for (size_t i = 0; i < mCmdLineArgs.size(); i++)
		{
			if (mCmdLineArgs[i].compare(0, 10, "-statelog=") != 0)
				continue;
			std::string aFileName = mCmdLineArgs[i].substr(10);
			if (aFileName.size() >= 2 && aFileName[0] == '"' && aFileName[aFileName.size() - 1] == '"')
				aFileName = aFileName.substr(1, aFileName.size() - 2);
			gStateLogFile = fopen(aFileName.c_str(), "w");
			if (gStateLogFile == NULL)
				printf("ERROR statelog: cannot open %s\n", aFileName.c_str());
			else
				gStateLogMode = 1;
			fflush(stdout);
		}
		if (gStateLogMode == 0)
			return aResult;
	}

	if (gStateLogFile == NULL)
		return aResult;
	if (mBoard != NULL)
	{
		std::string aLine = TestStateLogLine(this);
		fprintf(gStateLogFile, "%s\n", aLine.c_str());
		gStateLogDirty = true;
		if (++gStateLogLines % 100 == 0)
			fflush(gStateLogFile);
	}
	else
	{
		// no board: only the game generator's index, to see what the menus and dialogs draw
		uint32_t aRandState[MTRAND_N];
		int aRandIndex = -1;
		if (gGameRand != NULL)
			gGameRand->GetState(aRandState, &aRandIndex);
		// and the typing tutor, when there is one: its lesson, state, task, the keys typed so far and the key that is next
		std::string aTutor;
		if (mTypingTutor != NULL)
		{
			TypingTutor* t = mTypingTutor;
			aTutor = StrFormat(" tut=1 tles=%d tst=%d ttask=%d terr=%d tclean=%d tcorr=%d trwpm=%d tawpm=%d tsec=%d tacc=%d texp=%d tleft=%d "
				"tfirst=%d tcre=%d tpause=%d", t->mLessons != NULL ? t->mLessons->mCurrent : -9, t->mState, t->mTask, t->mErrors,
				t->mCleanKeys, t->mCorrectKeys, t->mRawWPM, t->mAdjustedWPM, t->mSeconds, t->mAccuracy,
				(int) (unsigned char) t->mExpectedChar, t->mTimeLeft, t->mFirstLine, (int) t->mCreatures.size(), t->mPauseCount);
		}
		fprintf(gStateLogFile, "u=%d noboard=1 rng=%d%s |\n", mUpdateCount, aRandIndex, aTutor.c_str());
	}
	if (mBoard == NULL && gStateLogDirty)
	{
		fflush(gStateLogFile);		// a game is over: what it logged is on disk
		gStateLogDirty = false;
	}
	return aResult;
}
