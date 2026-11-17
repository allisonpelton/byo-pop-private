#ifndef __FEEDINGFRENZY2_USERDATA_H__
#define __FEEDINGFRENZY2_USERDATA_H__

// The current player's data (UserData, 0x88 bytes, vtable 0x558ce4 (feedingfrenzy2); its constructor 0x498300
// (feedingfrenzy2)), the application's +0xb4: the player's name, the food bank, the highest jump and a record of each
// mode's game. The original reads it from the current player of the players' list (0x49cf10 and 0x47b1a0
// (feedingfrenzy2)), each value checked against a signature, and follows the list's changes of player: not yet
// translated, so the data stays as made, with no player.

#include "Object.h"

#include <string>

// The two modes of play, the main menu's newGame and newTimedGame
enum GameMode
{
	GAME_MODE_NORMAL = 0,
	GAME_MODE_TIMED = 1,
	GAME_MODE_COUNT = 2
};

class UserData : public engine::Object
{
public:
	// A mode's record (0x20 bytes), each value the player's "mode<n>..." of the same name
	struct ModeRecord
	{
		int mMode;              // +0x00: GameMode
		int mLastContinues;     // +0x04
		int mLastStage;         // +0x08: -1 for no game to go on with
		int mLastScore;         // +0x0c: -1 likewise
		int mLastLifeCount;     // +0x10
		int mTimeRollover;      // +0x14
		int mMaxStage;          // +0x18: the stages reached
		// +0x1c: set to 0 by the constructor; its readers not yet found
	};

	// 0x498300 (feedingfrenzy2): no player, each mode's record empty
	UserData();

	// 0x402750 (feedingfrenzy2): empty for no player
	const std::string& getName() const { return mName; }
	const ModeRecord& getMode(int theMode) const { return mModes[theMode]; }

private:
	std::string mName;                      // +0x1c
	int mFoodBank;                          // +0x38: the player's foodBank
	float mHighestJump;                     // +0x3c: the player's highestJump
	ModeRecord mModes[GAME_MODE_COUNT];     // +0x40
};

#endif
