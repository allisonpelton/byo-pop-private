#ifndef __FEEDINGFRENZY2_USERDATA_H__
#define __FEEDINGFRENZY2_USERDATA_H__

// The current player's data (UserData, 0x88 bytes, vtable 0x558ce4 (feedingfrenzy2); its constructor 0x498300
// (feedingfrenzy2)), the application's +0xb4: the player's name, the food bank, the highest jump and a record of each
// mode's game, read from the current player's parameters and read again whenever the players' list changes its
// current player. The food bank and the jump are signed together ("globalSign"), and each mode's record on its own
// ("mode<n>Sign"); a signature that does not match its values resets them and saves the player again, unless the
// stored signature is the cheat's word.

#include "Object.h"
#include "Params.h"
#include "sigslot.h"

#include <string>

class GameData;
class SignatureDigest;

// The two modes of play, the main menu's newGame and newTimedGame
enum GameMode
{
	GAME_MODE_NORMAL = 0,
	GAME_MODE_TIMED = 1,
	GAME_MODE_COUNT = 2
};

class UserData : public engine::Object, public sigslot::HasSlots
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
		// +0x1c: set to 0 by the constructor and init; its readers not yet found
	};

	// 0x498300 (feedingfrenzy2): no player, each mode's record empty
	UserData();

	// 0x49cf10 (feedingfrenzy2): the records emptied, the food bank and the jump 0, the players' list's signal
	// connected, then the current player read; false with no list or no player
	bool init();
	// 0x47b1a0 (feedingfrenzy2): the current player's name and values read and checked against their signatures;
	// false with no player
	bool load();
	// 0x4819c0 (feedingfrenzy2): the values and their signatures set in the current player's parameters, then the
	// player saved; nothing with no player
	void save();

	// 0x402750 (feedingfrenzy2): empty for no player
	const std::string& getName() const { return mName; }
	const ModeRecord& getMode(int theMode) const { return mModes[theMode]; }
	int getFoodBank() const { return mFoodBank; }
	void setFoodBank(int theFoodBank) { mFoodBank = theFoodBank; }
	// The stage's end, as the last stage of the mode is finished (0x49aef0 (feedingfrenzy2))
	void setMaxStage(int theMode, int theMaxStage) { mModes[theMode].mMaxStage = theMaxStage; }
	// port (the test command unlock all): every mode's furthest stage the count of stages, as a player who has
	// finished both has, and the player saved
	void unlockAll(int theStageCount);
	// 0x49fa40 (feedingfrenzy2): the mode's record holds a game to go on with
	bool hasSavedGame(int theMode) const { return mModes[theMode].mLastStage >= 0 && mModes[theMode].mLastScore >= 0; }
	// 0x49ae90 (feedingfrenzy2): the game kept in its mode's record, the stages reached raised to its stage
	// (0x49aed0 (feedingfrenzy2))
	void recordGame(const GameData& theGameData);
	// 0x4980c0 (feedingfrenzy2): the mode's record holds no game to go on with, and the player is saved
	void clearSavedGame(int theMode);

private:
	// 0x49a9d0 (feedingfrenzy2): the players' list's slot, which only jumps to load
	void currentUserSlot() { load(); }
	// 0x47e1e0 (feedingfrenzy2): a mode's record read from the player's parameters, each value added to the digest
	void loadMode(ModeRecord& theRecord, engine::Params& theParams, SignatureDigest& theDigest);
	// 0x473570 (feedingfrenzy2): a mode's record and its signature set in the player's parameters
	void saveMode(const ModeRecord& theRecord, engine::Params& theParams);
	// The records emptied (0x49cf10 and 0x47b1a0 (feedingfrenzy2), inlined)
	void resetModes();

	std::string mName;                      // +0x1c
	int mFoodBank;                          // +0x38: the player's foodBank
	float mHighestJump;                     // +0x3c: the player's highestJump
	ModeRecord mModes[GAME_MODE_COUNT];     // +0x40
};

#endif
