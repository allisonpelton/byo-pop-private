#ifndef __FEEDINGFRENZY2_PLAYERRANKS_H__
#define __FEEDINGFRENZY2_PLAYERRANKS_H__

// The titles a player earns by points (PlayerRanks, vtable 0x55a714 (feedingfrenzy2); its constructor 0x49ff20
// (feedingfrenzy2)), the application's +0xc4, read from resource/playerRanks.xml: each <rank>'s points and title, kept
// in order of points. Its handler is PlayerRanks::RankDocHandler (0x6c bytes, its constructor 0x49d650
// (feedingfrenzy2)).

#include "MasterDocHandler.h"
#include "Object.h"

#include <string>
#include <vector>

class PlayerRanks : public engine::Object
{
public:
	// A <rank> (0x20 bytes)
	struct Rank
	{
		int mPoints;          // +0x00
		std::string mTitle;   // +0x04
	};

	// 0x494b20 (feedingfrenzy2): the ranks emptied and the file's read
	void load(const std::string& thePath);
	// 0x4788c0 (feedingfrenzy2): before the first rank of more points, else last
	void addRank(int thePoints, const std::string& theTitle);

	const std::vector<Rank>& getRanks() const { return mRanks; }
	// 0x405f40 (feedingfrenzy2): the title of the last rank of no more points than these, empty for none
	std::string getTitle(int thePoints) const;

private:
	std::vector<Rank> mRanks;  // +0x0c
};

// PlayerRanks::RankDocHandler
class RankDocHandler : public engine::SubDocHandler
{
public:
	explicit RankDocHandler(PlayerRanks* theRanks) : mRanks(theRanks) {}

	void begin(engine::Params& theAttributes) override {}
	// 0x00 (0x4222d0 (feedingfrenzy2)): <rank points title>
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	void endElement(const std::string& theName) override {}

private:
	PlayerRanks* mRanks;  // +0x60
};

#endif
