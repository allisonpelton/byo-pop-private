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
#include "LessonManager.h"
#include "WSTimer.h"
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
	for (size_t i = 0; i < aAll.size(); i++)
	{
		const BotCreature& c = aAll[i];
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
		aList += StrFormat(" %s:%s@%.2f,%.2f,%d", c.mKind.c_str(), c.mWord.c_str(), c.mX, c.mY, c.mState);
	}
	int aLevel = app->mLessonManager != NULL ? app->mLessonManager->mLevel : -2;
	return StrFormat("u=%d lvl=%d score=%d time=%d sharks=%d piranhas=%d clams=%d bonus=%d boss=%d typed=%d lives=%d |%s", app->mUpdateCount,
		aLevel, b->mPlayer->mScore, b->mGameTimer->Elapsed(), aCounts[0], aCounts[1], aCounts[2], aCounts[3], aCounts[4] > 0 ? 1 : 0,
		(int) b->mTyped.size(), b->mPlayer->mField110, aList.c_str());
}

// the framework's update, then (with -statelog) the line for this update
bool WinWSApp::DoUpdateFrames()
{
	bool aResult = SexyApp::DoUpdateFrames();
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
	else if (gStateLogDirty)
	{
		fflush(gStateLogFile);		// a game is over: what it logged is on disk
		gStateLogDirty = false;
	}
	return aResult;
}
