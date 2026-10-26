#ifndef __PIXELUS_LEVELDATA_H__
#define __PIXELUS_LEVELDATA_H__

// LevelData.cpp: the list of temples and their puzzles (Pixelus.exe 0x42abd0 constructor, 0x42a870 Load;
// vftable 0x50f534). levels/levels.txt holds "#level#<name>" followed by "#puzzle#<file>" lines; each puzzle file
// starts with its "#uuid#" line, which is how everything else (best scores, the player's history) names a puzzle.
// The original keeps a std::map of levels, so the temples come in alphabetical order; the editor's functions
// (AddPuzzle, RemovePuzzle, MovePuzzle, Save) are not ported.

#include <map>
#include <string>
#include <vector>

namespace Sexy
{

class PixelusApp;

class LevelData
{
public:
	struct Puzzle
	{
		std::string		mFile;		// "apple.txt"
		std::string		mUuid;		// the 36 characters after "#uuid#"
	};

	struct Level
	{
		std::string				mName;
		std::vector<Puzzle>		mPuzzles;
	};

public:
	PixelusApp*							mApp;
	std::map<std::string, Level>		mLevels;		// +0x10
	std::map<std::string, std::string>	mFileByUuid;	// +0x4
	std::string							mCurrentLevel;	// +0x30
	int									mCurrentPuzzle;	// +0x38

public:
	LevelData(PixelusApp* theApp);					// 0x42abd0

	void				Load();						// 0x42a870 (puzzle by puzzle: 0x429c70)
	const Level*		GetLevel(const std::string& theName) const;
	int					GetPuzzleCount(const std::string& theName) const;		// 0x4283e0 (-1: no such temple)
	const Puzzle*		GetCurrentPuzzle() const;								// 0x42a080 and the index
	bool				IsKnown(const std::string& theUuid) const { return mFileByUuid.find(theUuid) != mFileByUuid.end(); }	// 0x427d00
	const Puzzle*		FindPuzzle(const std::string& theUuid) const;
};

}

#endif
