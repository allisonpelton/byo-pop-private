// HighScore: the high score table and its window (0x4351b0..0x435bf0 in the original). Eight entries, a score and a
// name each, kept in highscores.dat in the user's folder; the window lists them in two columns, rows alternating in
// colour, and the entry just made, if any, pulses.
#pragma once

#include "BBWindow.h"

namespace Sexy
{

class ObjString;

const int HIGHSCORE_ENTRIES = 8;
const int HIGHSCORE_NAME_SIZE = 20;

// One entry as highscores.dat holds it (0x18 bytes)
struct HighScoreEntry
{
	int mScore;
	char mName[HIGHSCORE_NAME_SIZE];
};

class HighScore : public BBWindow
{
public:
	ObjString* mNames[HIGHSCORE_ENTRIES];  // +0x350
	ObjString* mScores[HIGHSCORE_ENTRIES]; // +0x370

	static HighScoreEntry sEntries[HIGHSCORE_ENTRIES]; // 0x60e558, the best first
	static int sNewEntry;    // 0x600580: the entry just made, which pulses; -1 for none
	static float sPulse;     // 0x60e554: the pulse, 0 to 1
	static bool sPulseRising; // 0x600584

	HighScore();             // 0x4351b0
	~HighScore();            // 0x4352c0
	bool Update() override;  // 0x4352e0

	static void Add(int theScore, const char* theName); // 0x435740
	static bool Save();      // 0x4359e0
	static bool Load();      // 0x435a40

private:
	void Pulse();            // 0x435300
	void BuildEntries();     // 0x4354b0
};

void InitHighScores();       // 0x435aa0: the file's table, or the game's own when there is none
const char* FormatScore(unsigned int theScore); // 0x4732d0: with commas between thousands, in a buffer of its own

} // namespace Sexy
