#ifndef __SPROUT_SCOREBOARD_H__
#define __SPROUT_SCOREBOARD_H__

// A list of the best scores of one kind (the exe's Scoreboard, constructor 0x483630 (pizzafrenzy), 0x88 bytes;
// vtable of the game's board 0x4fd768 (pizzafrenzy)) and its entries (0x4796f0 (pizzafrenzy)). The game says what an
// entry is (createEntry) and which place a new one takes (findPosition).
//
// Left out: the online scoreboard (the original's thread, its state machine, its critical section and its requests):
// the port has no online scores.

#include "Object.h"
#include "UserInfo.h"

#include <string>
#include <vector>

namespace engine
{

// An entry of a scoreboard, with its place (0x4796f0 (pizzafrenzy), vtable 0x4fa838 (pizzafrenzy) for the game's)
class ScoreEntry : public Object
{
public:
	ScoreEntry() : mRank(0) {}  // 0x4796f0 (pizzafrenzy)

	// Slot 0x04 (0x4be760 (pizzafrenzy) for the game's): what the entries are ordered by
	virtual int getScore() = 0;
	// Slot 0x08: the entry as the parameters it is saved as
	virtual void toParams(Params& theParams) = 0;
	// Slot 0x0c: the entry from the parameters
	virtual void fromParams(const Params& theParams) = 0;
	// Slot 0x10: the parameters a table shows
	virtual void displayParams(Params& theParams) = 0;
	// Slot 0x14: the parameters the entry saves, with empty values (what a saved entry is read by)
	virtual void defaultParams(Params& theParams) = 0;

	int getRank() const { return mRank; }       // 0x411960 (pizzafrenzy)
	void setRank(int theRank) { mRank = theRank; }  // 0x479670 (pizzafrenzy)

protected:
	// What every entry saves besides its own, the rank as the parameter "rank" (0x521320 (pizzafrenzy)), which the
	// game's slots call first: 0x479690 (pizzafrenzy), 0x4796b0 (pizzafrenzy) and 0x4796d0 (pizzafrenzy)
	void rankToParams(Params& theParams);
	void rankFromParams(const Params& theParams);
	void rankDefaultParams(Params& theParams);

private:
	int mRank;  // +0x0c
};

class Scoreboard : public Object
{
public:
	// 0x483630 (pizzafrenzy)
	Scoreboard(const std::string& theName, int theMaxCount) :
		mName(theName), mState(STATE_CREATED), mMaxCount(theMaxCount), mLastAddedIndex(-1)
	{
	}

	// States of the board (0x482cb0 (pizzafrenzy) sets one)
	enum { STATE_CREATED = 0, STATE_LOADING = 1, STATE_READY = 2, STATE_ERROR = 3 };

	// Slot 0x04: a new entry of the game's kind
	virtual ScoreEntry* createEntry() = 0;
	// Slot 0x08: the index of the first entry that the new one goes before, the count when it goes last
	virtual int findPosition(ScoreEntry* theEntry) = 0;

	const std::string& getName() const { return mName; }    // 0x479850 (pizzafrenzy)
	const std::string& getTitle() const { return mTitle; }
	void setTitle(const std::string& theTitle) { mTitle = theTitle; }
	int getCount() const { return (int)mEntries.size(); }   // 0x482ce0 (pizzafrenzy)
	bool isEmpty() const { return mEntries.empty(); }       // 0x482d00 (pizzafrenzy)
	ScoreEntry* getEntry(int theIndex);                     // 0x4830c0 (pizzafrenzy): null when there is none
	int getState() const { return mState; }
	void setState(int theState) { mState = theState; }      // 0x482cb0 (pizzafrenzy)
	int getLastAddedIndex() const { return mLastAddedIndex; }
	void forgetLastAdded() { mLastAddedIndex = -1; }        // 0x482ca0 (pizzafrenzy)

	// 0x4830f0 (pizzafrenzy): the place the entry would take (1 is the first), 0 when it would not be on the board
	int getRank(ScoreEntry* theEntry);
	// 0x483840 (pizzafrenzy): the entry is put on the board when it has a place
	bool add(ScoreEntry* theEntry);
	// 0x483350 (pizzafrenzy): the entries are let go
	void clear();

private:
	bool insert(ScoreEntry* theEntry);  // 0x483720 (pizzafrenzy)

	std::string mName;                    // +0x14
	std::string mTitle;                   // +0x30
	int mState;                           // +0x4c
	std::vector<Ref<ScoreEntry>> mEntries;  // +0x54
	int mMaxCount;                        // +0x60
	int mLastAddedIndex;                  // +0x64
};

} // namespace engine

#endif
