// port: test hook -typist=<file>[;every=K][;start=U][;stop=U][;typo=N][;enter=U] (phase 1 of the identical-play pass,
// games/typershark/tools/phase1): the game's own bot (Bot.cpp) plays by script lines and writes down every line it plays, so the
// same inputs can be sent to the original at the same updates. Each line is `<update> char <c>` or `<update> click <x> <y>`,
// as -script takes them (the framework's script path: the line of update N runs before update N).
//   every=K   a letter every K updates (default 1)       start=U   no input before update U
//   stop=U    no input from update U on (an idle player)  typo=N    every Nth letter typed is a wrong one
//   enter=U   press Enter (the Shark Zapper) once at update U
// The bot reads the board after an update and its input goes in at the next one. Nothing here runs without the option.

#include "TestHooks.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Player.h"
#include "TypingTutor.h"
#include "Common.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace Sexy;

namespace
{

struct TypistState
{
	int			mMode;			// -1 not looked at yet, 0 off, 1 on
	FILE*		mFile;
	int			mEvery;
	int			mStart;
	int			mStop;
	int			mTypoEvery;
	int			mEnterAt;
	int			mTypedCount;
	bool		mEnterDone;
	int			mPromptUpdate;	// the update of the last prompt click: the same dialog is not clicked twice in a row
	int			mTutorCorrect;	// the typing tutor's count of right keys at the last key, and the keys since that counted for nothing
	int			mTutorStuck;

	TypistState() : mMode(-1), mFile(NULL), mEvery(1), mStart(0), mStop(2000000000), mTypoEvery(0), mEnterAt(-1), mTypedCount(0),
		mEnterDone(false), mPromptUpdate(-100), mTutorCorrect(-1), mTutorStuck(0) {}
	~TypistState()
	{
		if (mFile != NULL)
			fclose(mFile);
	}
} gTypist;

void ParseOptions(WinWSApp* app)
{
	gTypist.mMode = 0;
	for (size_t i = 0; i < app->mCmdLineArgs.size(); i++)
	{
		const std::string& anArg = app->mCmdLineArgs[i];
		if (anArg.compare(0, 8, "-typist=") != 0)
			continue;
		std::string aSpec = anArg.substr(8);
		if (aSpec.size() >= 2 && aSpec[0] == '"' && aSpec[aSpec.size() - 1] == '"')
			aSpec = aSpec.substr(1, aSpec.size() - 2);
		size_t aSemi = aSpec.find(';');
		std::string aFileName = aSpec.substr(0, aSemi);
		while (aSemi != std::string::npos)
		{
			size_t aNext = aSpec.find(';', aSemi + 1);
			std::string anOption = aSpec.substr(aSemi + 1, aNext == std::string::npos ? std::string::npos : aNext - aSemi - 1);
			size_t anEq = anOption.find('=');
			if (anEq != std::string::npos)
			{
				std::string aName = anOption.substr(0, anEq);
				int aValue = atoi(anOption.c_str() + anEq + 1);
				if (aName == "every")
					gTypist.mEvery = aValue > 0 ? aValue : 1;
				else if (aName == "start")
					gTypist.mStart = aValue;
				else if (aName == "stop")
					gTypist.mStop = aValue;
				else if (aName == "typo")
					gTypist.mTypoEvery = aValue;
				else if (aName == "enter")
					gTypist.mEnterAt = aValue;
			}
			aSemi = aNext;
		}
		gTypist.mFile = fopen(aFileName.c_str(), "w");
		if (gTypist.mFile == NULL)
			printf("ERROR typist: cannot open %s\n", aFileName.c_str());
		else
			gTypist.mMode = 1;
		fflush(stdout);
	}
}

void Play(WinWSApp* app, int theUpdate, const std::string& theCommand, const std::string& theArgs)
{
	app->AddScriptCommand(theUpdate, theCommand, theArgs);
	fprintf(gTypist.mFile, "%d %s %s\n", theUpdate, theCommand.c_str(), theArgs.c_str());
	fflush(gTypist.mFile);
}

}  // namespace

void Sexy::TestTypistStep(WinWSApp* app)
{
	if (gTypist.mMode == 0)
		return;
	if (gTypist.mMode < 0)
	{
		ParseOptions(app);
		if (gTypist.mMode == 0)
			return;
	}
	int aNext = app->mUpdateCount + 1;			// the update the input is read at
	if (aNext < gTypist.mStart || aNext >= gTypist.mStop || aNext % gTypist.mEvery != 0)
		return;
	if (app->mBoard == NULL && app->mTypingTutor != NULL && app->mTypingTutor->mVisible)
	{
		// the typing tutor: the key that is next, until it stops being accepted (a space at the first or last place of a line
		// is neither right nor wrong: the lesson cannot go on, and the bot gives up after 20 keys that counted for nothing)
		TypingTutor* t = app->mTypingTutor;
		if (t->mState == 2 || !app->mDialogList.empty() || t->mPauseCount > 0)
			return;
		if (t->mCorrectKeys != gTypist.mTutorCorrect)
		{
			gTypist.mTutorCorrect = t->mCorrectKeys;
			gTypist.mTutorStuck = 0;
		}
		else if (++gTypist.mTutorStuck > 20)
			return;
		char c = t->mExpectedChar;
		if (c == ' ')
			Play(app, aNext, "char", "SPACE");
		else if (c == '\r' || c == '\n')
			Play(app, aNext, "key", "RETURN");
		else
			Play(app, aNext, "char", std::string(1, c));
		return;
	}
	if (app->mBoard == NULL && app->GetDialog(DIALOG_MESSAGE) == NULL)
		return;

	if (!gTypist.mEnterDone && gTypist.mEnterAt >= 0 && aNext >= gTypist.mEnterAt && app->mBoard != NULL)
	{
		gTypist.mEnterDone = true;
		Play(app, aNext, "key", "RETURN");
		return;
	}

	BotMove aMove = ChooseMove(TestTakeSnapshot(app));
	if (!aMove.mValid)
		return;
	for (size_t i = 0; i < aMove.mInput.size(); i++)
	{
		const BotStep& aStep = aMove.mInput[i];
		if (aStep.mKind == BotStep::CLICK)
		{
			if (aNext - gTypist.mPromptUpdate < 3)
				return;
			gTypist.mPromptUpdate = aNext;
			Play(app, aNext, "click", StrFormat("%d %d", aStep.mX, aStep.mY));
			return;
		}
		if (aStep.mKind == BotStep::KEY)
		{
			std::string aKey = aStep.mKey;
			if (aKey == " ")
			{
				if (aNext - gTypist.mPromptUpdate < 3)
					return;
				gTypist.mPromptUpdate = aNext;
				Play(app, aNext, "char", "SPACE");
				return;
			}
			if (aKey.size() == 1)
			{
				gTypist.mTypedCount++;
				char c = aKey[0];
				if (gTypist.mTypoEvery > 0 && gTypist.mTypedCount % gTypist.mTypoEvery == 0 && c >= 'A' && c <= 'Z')
					c = (char) ('A' + (c - 'A' + 7) % 26);
				Play(app, aNext, "char", std::string(1, c));
				return;
			}
			return;
		}
	}
}
