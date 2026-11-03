#ifndef __BOOKWORM_STATELOG_H__
#define __BOOKWORM_STATELOG_H__

// port: test hook for -statelog=<file> (see BookWormApp.h). Not part of the original game.
//
// After every update, the game values that decide play, each logged when it changes as origrun logs a --watch
// value of the original ("watch k: old -> new at update N", docs/ORIGRUN.md), so the identical-play scripts
// (games/bookworm/tools/identical) compare the two update by update. Here the k is the value's name, which those
// scripts map to the original's address (games/bookworm/tools/identical/values.txt). Values are unsigned, and "-"
// while there is no board, as origrun logs a value it cannot read.

#include <cstdio>
#include <string>
#include <vector>

namespace Sexy
{

class BookWormApp;

class StateLog
{
public:
	StateLog(const std::string& theFileName);
	~StateLog();

	void			Update(BookWormApp* theApp);

private:
	FILE*			mFile;
	std::vector<std::string> mLast;
};

}

#endif
