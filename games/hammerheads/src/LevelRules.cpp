#include "LevelRules.h"

#include <cstdlib>
#include <memory>

#include "Application.h"
#include "Buffer.h"
#include "DrawUtil.h"
#include "GameScreen.h"
#include "Timer.h"

using namespace Sexy;

// 0x424e40: the names of the columns, in the order the game asks for them (RuleColumn)
static const char* const sRuleNames[NUM_RULE_COLUMNS] =
{
	"Level", "Time", "Intro File", "Max Gnomes Start", "Max Gnomes End", "Countdown Start", "Countdown End",
	"Cubby Top s Start", "Cubby Top s End", "Cubby Speed Start", "Cubby Speed End", "Waves",
	"Time Factor Gnome 1", "Time Factor Gnome Woman", "Time Factor Gnome Hide", "Time Factor Gnome Bomb",
	"Time Factor Gnome BubbleHead", "Time Factor Gnome Strong", "Time Factor Heart 1", "Time Factor Gnome Coin 1",
	"Time Factor Gnome Shop", "Time Factor Gnome Challenge", "Time Factor Gnome Forky Chap", "Time Factor Gnome MultiplierX5",
	"Time Factor Gnome Cupido", "Time Factor Gnome Silver", "Time Factor Gnome Chest", "Time Factor Gnome Lucky Clover",
	"Time Factor Gnome Bonus Duck", "Time Factor Gnome Bonus Icecream", "Time Factor Gnome Bonus Strawberry",
	"Time Factor Gnome Rabbit", "Time Factor Gnome King", "Time Factor Gnome PowerUp", "Time Factor Gnome Switcher",
	"Time Factor Gnome Napoleon",
	"Gnome 1 Max", "Gnome Woman Max", "Gnome Hide Max", "Gnome Bomb Max", "Gnome BubbleHead Max", "Gnome Strong Max",
	"Gnome Shop Max", "Gnome Challenge Max", "Gnome Bouncing Max", "Gnome Forky Chap Max", "Gnome MultiplierX5 Max",
	"Gnome Cupido Max", "Gnome Silver Max", "Gnome Chest Max", "Gnome Lucky Clover Max", "Gnome Bonus Duck Max",
	"Gnome Bonus Icecream Max", "Gnome Bonus Strawberry Max", "Gnome Rabbit Max", "Gnome King Max", "Gnome Flying Max",
	"Gnome PowerUp Max", "Gnome Switcher Max", "Gnome Napoleon Max"
};

// 0x424e40
RulesSheet::RulesSheet()
{
	for (int i = 0; i < NUM_RULE_COLUMNS; i++)
		mColumnInFile[i] = -1;
}

// 0x425d40: the fields of a line: cut at each semicolon outside quotes, the quotes themselves dropped
static std::vector<std::string> SplitLine(const std::string& theLine)
{
	std::vector<std::string> aFields;
	std::string aField;
	bool aInQuotes = false;
	for (size_t i = 0; i < theLine.length(); i++)
	{
		char aChar = theLine[i];
		if (aChar == '"')
			aInQuotes = !aInQuotes;
		else if (aChar != ';' || aInQuotes)
			aField += aChar;
		else
		{
			aFields.push_back(aField);
			aField.clear();
		}
	}
	aFields.push_back(aField);
	return aFields;
}

// 0x4261b0: the first line names the columns, and every later line with more than one field is a level
void RulesSheet::Load(const std::string& theFileName)
{
	mRows.clear();
	for (int i = 0; i < NUM_RULE_COLUMNS; i++)
		mColumnInFile[i] = -1;

	Buffer aFile;
	if (!gApp->ReadBufferFromFile(theFileName, &aFile))
		return;

	std::string aText((const char*)aFile.GetDataPtr(), aFile.GetDataLen());
	size_t aPos = 0;
	int aLineNumber = 0;
	while (aPos < aText.length())
	{
		size_t anEnd = aText.find('\n', aPos);
		if (anEnd == std::string::npos)
			anEnd = aText.length();
		std::string aLine = aText.substr(aPos, anEnd - aPos);
		aPos = anEnd + 1;

		// the exe reads the file in text mode, so a return before the line feed is not part of the line
		if (!aLine.empty() && aLine[aLine.length() - 1] == '\r')
			aLine.erase(aLine.length() - 1);

		std::vector<std::string> aFields = SplitLine(aLine);
		if (aLineNumber == 0)
		{
			for (size_t aFileColumn = 0; aFileColumn < aFields.size(); aFileColumn++)
			{
				for (int aColumn = 0; aColumn < NUM_RULE_COLUMNS; aColumn++)
				{
					if (aFields[aFileColumn] == sRuleNames[aColumn])
					{
						mColumnInFile[aColumn] = (int)aFileColumn;
						break;
					}
				}
			}
		}
		else if (aFields.size() > 1)
			mRows.push_back(aFields);

		aLineNumber++;
	}
}

int RulesSheet::GetRowCount() const
{
	return (int)mRows.size();
}

// 0x424500
std::string RulesSheet::GetText(int theRow, int theColumn) const
{
	if (mRows.empty())
		return std::string();
	if (theRow >= (int)mRows.size())
		theRow = (int)mRows.size() - 1;

	int aFileColumn = mColumnInFile[theColumn];
	const std::vector<std::string>& aRow = mRows[theRow];
	if (aFileColumn < 0 || aFileColumn >= (int)aRow.size())
		return std::string();
	return aRow[aFileColumn];
}

// 0x4245a0
int RulesSheet::GetInt(int theRow, int theColumn) const
{
	return atol(GetText(theRow, theColumn).c_str());
}

// 0x424ad0: the files write 1,5 for one and a half
double RulesSheet::GetDouble(int theRow, int theColumn) const
{
	std::string aText = GetText(theRow, theColumn);
	for (size_t i = 0; i < aText.length(); i++)
	{
		if (aText[i] == ',')
			aText[i] = '.';
	}
	return atof(aText.c_str());
}

// 0x423110
bool RulesSheet::IsLastRow(int theRow) const
{
	return theRow == (int)mRows.size() - 1;
}

static std::unique_ptr<RulesSheet> gRulesSheet;		// 0x597944

// 0x426660
RulesSheet* Sexy::GetRulesSheet(bool theReload)
{
	if (theReload)
		gRulesSheet.reset();

	if (gRulesSheet == nullptr)
	{
		gRulesSheet.reset(new RulesSheet());
		const char* aFileName = "boards/marathon.csv";
		if (gApp->mGameMode == GAME_MODE_CLASSIC)
			aFileName = "boards/classic.csv";
		else if (gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
			aFileName = "boards/classictough.csv";
		gRulesSheet->Load(aFileName);
	}
	return gRulesSheet.get();
}

int Sexy::RuleRow()
{
	return gApp->mGameScreen->mLevel - 1;
}

int Sexy::RuleInt(int theColumn)
{
	return GetRulesSheet()->GetInt(RuleRow(), theColumn);
}

double Sexy::RuleDouble(int theColumn)
{
	return GetRulesSheet()->GetDouble(RuleRow(), theColumn);
}

// 0x4514d0
double Sexy::GetWaveLength()
{
	int aTime = RuleInt(RULE_TIME);
	int aWaves = RuleInt(RULE_WAVES);
	return (double)aTime / (double)aWaves;
}

// 0x451570
int Sexy::GetWaveIndex()
{
	double aTimeUsed = GetTimer()->GetTimeUsed();
	int aLastWave = RuleInt(RULE_WAVES) - 1;
	int aWave = ToInt(aTimeUsed / GetWaveLength());
	if (aWave < aLastWave)
		return aWave;
	return aLastWave;
}

// 0x451900: the seconds kept free at the end of the first wave and of the second (0x559688)
static const double sWaveTailSeconds[2] = { 6.0, 12.0 };

bool Sexy::GetWaveActivePart(double* theFraction)
{
	int aWave = GetWaveIndex();
	double aIntoWave = GetTimer()->GetTimeUsed() - (double)aWave * GetWaveLength();
	double aActiveLength = GetWaveLength() - sWaveTailSeconds[aWave];
	if (aIntoWave < aActiveLength)
	{
		*theFraction = aIntoWave / aActiveLength;
		return true;
	}
	*theFraction = 1.0;
	return false;
}

// 0x451970
bool Sexy::IsWaveTail()
{
	if (GetTimer()->GetProgress() == 1.0)
		return true;
	double aFraction;
	return !GetWaveActivePart(&aFraction);
}
