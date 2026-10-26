#ifndef __WORDHARMONY_HIGHSCORES_H__
#define __WORDHARMONY_HIGHSCORES_H__

// A high score table (0x30 bytes; the controller holds three at +0x24c, +0x27c and +0x2ac): up to ten names and
// scores, best first, kept in the registry under Software\GameHouse\WordHarmony\<the table's name> as scoreName<i>
// and scoreValue<i>. The original opens that key with its own registry helper (0x421c20-0x422030); the port goes
// through engine::Host's registry functions, the table's name and a backslash before the value's.

#include <string>
#include <vector>

class HighScores
{
public:
	HighScores() {}  // 0x4215e0
	~HighScores();   // 0x421100

	// 0x421750: the table theName read from the registry
	void load(const std::string& theName);
	// 0x421200: the table written back
	void save();
	// 0x421600: theName and theScore put in before the first score lower or equal, the table cut to ten
	void add(std::string theName, unsigned int theScore);

	// 0x4210a0: score theIndex, -1 past the end
	unsigned int getScore(unsigned int theIndex);
	// 0x4210d0: the place theScore would take, the count of higher scores
	unsigned int findRank(unsigned int theScore);
	// 0x4213b0: name theIndex
	std::string getName(int theIndex);
	// (inline in the original) the entries
	unsigned int getCount() { return (unsigned int)mEntries.size(); }

private:
	// 0x14 bytes
	struct Entry
	{
		char mName[16];        // +0x00: by strncpy, so a name of 16 letters has no '\0'
		unsigned int mScore;   // +0x10
	};

	std::vector<Entry*> mEntries;  // +0x04
	std::string mKey;              // +0x10: the table's name
};

#endif
