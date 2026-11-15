#ifndef __HAMMERHEADS_LEVELRULES_H__
#define __HAMMERHEADS_LEVELRULES_H__

// The rules of each level: a sheet read from boards/classic.csv, classictough.csv or marathon.csv (0x424e40-0x426660, the
// exe's own reader), one row a level, and what the game works out of a row and of the time that has passed (0x4514d0-0x451970).
// The files are semicolon separated with quoted names in the first row and decimal commas; the exe finds each column by its
// name, so the order of the columns in a file does not matter.

#include <string>
#include <vector>

namespace Sexy
{

// The columns the game asks for, in the order of the exe's list of names (0x424e40). The time factor columns are the
// seconds a gnome of the kind stays up, as a share of what the level says; the max columns are how many gnomes of the kind
// the level lets come.
enum RuleColumn
{
	RULE_LEVEL = 0,
	RULE_TIME = 1,						// seconds of the level
	RULE_INTRO_FILE = 2,				// (no file has it)
	RULE_MAX_GNOMES_START = 3,			// the most gnomes up at once: at the start of the level, and at the end (the pace is
	RULE_MAX_GNOMES_END = 4,			// worked out between them over the time of the level)
	RULE_COUNTDOWN_START = 5,			// the wait between one gnome coming up and the next
	RULE_COUNTDOWN_END = 6,
	RULE_TOP_START = 7,					// how long a gnome stays on top
	RULE_TOP_END = 8,
	RULE_SPEED_START = 9,				// how fast a cubby sends its gnome up
	RULE_SPEED_END = 10,
	RULE_WAVES = 11,					// how many waves the time of the level is cut into

	RULE_TIME_FACTOR_GNOME_1 = 12,		// the time factor of each kind, by the order of the names in the exe's list
	RULE_TIME_FACTOR_WOMAN,
	RULE_TIME_FACTOR_HIDE,
	RULE_TIME_FACTOR_BOMB,
	RULE_TIME_FACTOR_BUBBLEHEAD,
	RULE_TIME_FACTOR_STRONG,
	RULE_TIME_FACTOR_HEART_1,
	RULE_TIME_FACTOR_COIN_1,
	RULE_TIME_FACTOR_SHOP,
	RULE_TIME_FACTOR_CHALLENGE,
	RULE_TIME_FACTOR_FORKY_CHAP,
	RULE_TIME_FACTOR_MULTIPLIER_X5,
	RULE_TIME_FACTOR_CUPIDO,
	RULE_TIME_FACTOR_SILVER,
	RULE_TIME_FACTOR_CHEST,
	RULE_TIME_FACTOR_LUCKY_CLOVER,
	RULE_TIME_FACTOR_BONUS_DUCK,
	RULE_TIME_FACTOR_BONUS_ICECREAM,
	RULE_TIME_FACTOR_BONUS_STRAWBERRY,
	RULE_TIME_FACTOR_RABBIT,
	RULE_TIME_FACTOR_KING,
	RULE_TIME_FACTOR_POWERUP,
	RULE_TIME_FACTOR_SWITCHER,
	RULE_TIME_FACTOR_NAPOLEON,

	RULE_MAX_GNOME_1 = 36,
	RULE_MAX_WOMAN,
	RULE_MAX_HIDE,
	RULE_MAX_BOMB,
	RULE_MAX_BUBBLEHEAD,
	RULE_MAX_STRONG,
	RULE_MAX_SHOP,
	RULE_MAX_CHALLENGE,
	RULE_MAX_BOUNCING,
	RULE_MAX_FORKY_CHAP,
	RULE_MAX_MULTIPLIER_X5,
	RULE_MAX_CUPIDO,
	RULE_MAX_SILVER,
	RULE_MAX_CHEST,
	RULE_MAX_LUCKY_CLOVER,
	RULE_MAX_BONUS_DUCK,
	RULE_MAX_BONUS_ICECREAM,
	RULE_MAX_BONUS_STRAWBERRY,
	RULE_MAX_RABBIT,
	RULE_MAX_KING,
	RULE_MAX_FLYING,
	RULE_MAX_POWERUP,
	RULE_MAX_SWITCHER,
	RULE_MAX_NAPOLEON,

	NUM_RULE_COLUMNS = 60
};

class RulesSheet
{
public:
	std::vector<std::vector<std::string> >	mRows;					// the rows after the first one, as the file has them
	int										mColumnInFile[NUM_RULE_COLUMNS];	// where the file keeps each named column; -1, it has none

public:
	RulesSheet();													// 0x424e40
	void			Load(const std::string& theFileName);			// 0x4261b0

	int				GetRowCount() const;
	// 0x424500: the text of a cell; a row past the last is the last row
	std::string		GetText(int theRow, int theColumn) const;
	int				GetInt(int theRow, int theColumn) const;		// 0x4245a0: atol of the cell
	double			GetDouble(int theRow, int theColumn) const;		// 0x424ad0: atof of the cell with its commas read as points
	bool			IsLastRow(int theRow) const;					// 0x423110
};

// 0x426660: the sheet of the game mode, read on first use (or again when theReload)
RulesSheet* GetRulesSheet(bool theReload = false);

// The row of the level being played, and the cells of it
int RuleRow();														// the level number less one
int RuleInt(int theColumn);											// the sheet's number in the level's row
double RuleDouble(int theColumn);

// 0x4514d0: the seconds of one wave of the level: its time over its number of waves
double GetWaveLength();
// 0x451570: which wave the time that has passed is in (the last one stays)
int GetWaveIndex();
// 0x451900: whether the time is in the part of its wave in which gnomes come, and how far through that part it is (1.0
// when it is not in it); the last seconds of a wave (6 of the first, 12 of the second) are kept free of new gnomes
bool GetWaveActivePart(double* theFraction);
// 0x451970: the level is out of time or in the free seconds at the end of a wave
bool IsWaveTail();

}

#endif
