// The ranks (0x10 bytes, no virtuals; 0x7393d2-0x739af7, app +0xf58): their names, from properties\ranks.txt, and
// the points each one needs. A player's rank is Profile::mRank.
#pragma once

#include "Common.h"

#include <vector>

namespace Sexy {

class Ranks {
public:
	// One rank (0x20 bytes)
	struct Rank {
		int mPoints;       // +0x00: what the rank needs (15 for the second, then 30 more, then 25 more each time)
		SexyString mName;  // +0x04
	};

	std::vector<Rank> mRanks;  // +0x04

	Ranks();                   // 0x739af7
	void Load();               // 0x73991a
	// 0x739332: the rank's name; below 0 the game's "Trainee" text, past the last the last one's
	SexyString GetName(int theRank) const;
};

}  // namespace Sexy
