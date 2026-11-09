// The challenges' data and goal (properties\challenges.xml; 0x6d8000-0x6dd100): the categories with their
// challenges, loaded once by the app (WinTwistApp+0xf5c), and the check of a challenge's goal against the stats of
// the game going on. A Board holds the challenge being played at +0x2630.
#pragma once

#include "Color.h"
#include "Common.h"

#include <vector>

namespace Sexy {

class Profile;

// One rule of a challenge (12 bytes, the vector at Challenge+0x70): a stat, how it compares and its value.
struct ChallengeRule {
	int mStat = 0;   // +0x0: the stat's index (the names are listed in challenges.xml's rules: Gems, Moves, Time...)
	int mOp = 0;     // +0x4: 1 "=", 2 ">=", 3 "<=", 4 "<"; 0 for none
	int mValue = 0;  // +0x8: Time's is in seconds (the file has minutes)
};

// A rule as the game is going: the stat's value now (0x6db0ec's four vectors, one entry for each rule)
struct ChallengeGoal {
	int mStat = 0;
	int mCurrent = 0;
	int mOp = 0;
	int mTarget = 0;
};

// 0xe4 bytes (ctor 0x6d9fe2, dtor 0x6d9e70): one <challenge> of a category
class Challenge {
public:
	// The bits of mFlags (+0x80); the parser (0x6dbc98) sets them from the rules
	enum {
		FLAG_COUNT = 0x1,            // some rule counts a stat other than the level and the biggest match
		FLAG_TIME = 0x2,             // a rule has Time: mTimeGoal
		FLAG_MOVES = 0x4,            // a rule has Moves other than "= 1": mMoveGoal
		FLAG_POINTS = 0x8,           // a rule has Points other than "<=": the board counts the points
		FLAG_ONBOARD = 0x10,         // "Onboard": the stats are of the gems on the board now
		FLAG_TIME_LIMIT = 0x20,      // Time with "<=" or "<": the time is a limit
		FLAG_ONEMOVE = 0x40,         // "Onemove": the stats are of the last move
		FLAG_PATTERN = 0x80,         // "pattern": matches of the colours in order (mPattern)
		FLAG_CONSTANTCOUNTERS = 0x100,
		FLAG_NIGHTMODE = 0x200,      // the category's last, special challenge (Category::mSpecial)
		FLAG_NOCOUNT = 0x400,        // the text shows no count
		FLAG_NOCASCADES = 0x800,
		FLAG_CONSTANTDOOM = 0x1000,
		FLAG_QUITONFAIL = 0x2000,
		FLAG_BADGEM = 0x4000         // "badgem": a colour's gems must not break (mBadGem)
	};

	SexyString mNarrowName;                // +0x00 (a narrow string; never set, never read)
	SexyString mDescription;               // +0x1c
	SexyString mDoneDescription;           // +0x38: the text once it is met; '#' is where a number goes
	SexyString mCountTitle;                // +0x54
	std::vector<ChallengeRule> mRules;     // +0x70
	unsigned int mFlags = 0;               // +0x80
	int mTimeGoal = 0;                     // +0x84: the Time rule's value (seconds)
	int mMoveGoal = 0;                     // +0x88: the Moves rule's value
	int mCategory = 0;                     // +0x8c: its category's index
	int mNumber = 0;                       // +0x90: the number in its category, from 1
	int mSortKey = 0;                      // +0x94: never set, never read
	std::vector<int> mRuleIndexes;         // +0x98: never filled, never read
	std::vector<int> mPattern;             // +0xa8: the colours a pattern asks for, in order (RWGYVOB)
	int mProgress = 0;                     // +0xb8: the pattern's matches so far
	SexyString mConfig;                    // +0xbc: the properties file the challenge plays under
	int mBestScore = 0;                    // +0xd8
	int mLowScore = 0;                     // +0xdc
	int mBadGem = -1;                      // +0xe0: the colour of the gems that must not break
};

// 0x74 bytes (ctor 0x6db65b, dtor 0x6db6b1): one <Category>
class ChallengeCategory {
public:
	SexyString mId;                          // +0x00
	std::vector<SexyString> mLayers;         // +0x1c: the layer0 and layer1 pictures
	bool mAdditive = false;                  // +0x2c
	std::string mViz;                        // +0x30: the visualizer
	std::vector<Challenge*> mChallenges;     // +0x50: owned
	int mBackground = 0;                     // +0x4c: bkg2d
	Challenge* mSpecial = nullptr;           // +0x60: the nightmode challenge, owned
	Color mColor = Color(0, 0, 0, 255);      // +0x64

	ChallengeCategory() = default;
	~ChallengeCategory();
	ChallengeCategory(const ChallengeCategory&) = delete;
	ChallengeCategory& operator=(const ChallengeCategory&) = delete;
};

// 0x18 bytes (ctor 0x6dd087): the categories, and the state of the goal check
class ChallengeInfo {
public:
	std::vector<ChallengeCategory*> mCategories;  // +0x04: owned
	int mPatternMatches = 0;                      // +0x10: the pattern's matches this move (Check adds, EndMove clears)
	int mResult = 0;                              // +0x14: the last check: 0 not met, 1 met, 2 failed

	ChallengeInfo();
	~ChallengeInfo();
	ChallengeInfo(const ChallengeInfo&) = delete;
	ChallengeInfo& operator=(const ChallengeInfo&) = delete;

	void Load();                                   // 0x6dcff3: properties\challenges.xml (after emptying the list)
	void Clear();                                  // 0x6dbb45
	SexyString GetCategoryId(unsigned int theCategory) const;  // 0x6d9783: "" past the end
	std::string GetViz(unsigned int theCategory) const;        // 0x6d97c3: the category's visualizer, "" past the end
	int GetChallengeCount(unsigned int theCategory) const;     // 0x6d8c6d: not counting the special one
	int GetBackground(unsigned int theCategory) const;         // 0x6d8c9a: the category's bkg2d
	// 0x6d8cc2: the challenge theNumber (from 1) of a category, the special one past the last; its pattern
	// progress starts over. 0 for no such category
	Challenge* GetChallenge(unsigned int theCategory, int theNumber);
	int GetPercentDone(const Profile* theProfile) const;       // 0x6d8d19: the share done, in percent
	void EndMove(Challenge* theChallenge);                     // 0x6d86c9
	// 0x6db0ec: each rule of theChallenge with its stat's value now
	std::vector<ChallengeGoal> GetGoals(const Challenge* theChallenge) const;
	// 0x6db273: whether the goal is met (1), not (0) or failed (2); theValue gets the stat the goal counts.
	// theFinal asks for the verdict as the game ends; a miss then clears the stats that came before the first miss
	int Check(Challenge* theChallenge, int* theValue, bool theFinal);
	// 0x6db73e: the description (theDone: the done description) with the numbers in; theShowCount adds the count
	// "(now/goal)"; theFraction gets how much is done, 0 to 1
	SexyString GetText(const Challenge* theChallenge, bool theShowCount, bool theDone, float* theFraction) const;
};

}  // namespace Sexy
