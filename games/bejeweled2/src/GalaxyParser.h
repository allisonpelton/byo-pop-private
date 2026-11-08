#ifndef __GALAXYPARSER_H__
#define __GALAXYPARSER_H__

// GalaxyParser.h: reads puzzles\<pack>\galaxy.xml, the map of the puzzle galaxy (Sexy::GalaxyParser, 0x5be56d-0x5c05b5).
// TODO: the planets, the levels and the puzzles of a pack are not parsed yet.

#include <string>

namespace Sexy
{

class Buffer;

class GalaxyParser
{
public:
	std::string		mPuzzlePack;
	bool			mHasFailed;
	std::string		mErrorText;

public:
	GalaxyParser() : mHasFailed(false) {}

	void			Fail(const std::string& theError) { if (!mHasFailed) { mHasFailed = true; mErrorText = theError; } }   // 0x5be56d
	bool			HasFailed() const { return mHasFailed; }
	const std::string& GetErrorText() const { return mErrorText; }
	bool			ParseBuffer(const Buffer& theBuffer);
};

}

#endif
