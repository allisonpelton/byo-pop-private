#include "HighScore.h"

#include "GameApp.h"
#include "MainMenu.h"
#include "ObjString.h"

#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace Sexy;

static const char* HIGHSCORE_FILE = "highscores.dat";

// The longest name an entry takes
const int HIGHSCORE_NAME_LENGTH = 0x10;

// The panel's size, in 100-pixel tiles
const int HIGHSCORE_COLUMNS = 13;
const int HIGHSCORE_ROWS = 11;

// The rows' places: the first 230 pixels below the panel's top, each 86 below the last; the names start 90 pixels in
// from the panel's left and the scores 160 right of its middle
const double HIGHSCORE_FIRST_ROW = 230.0;
const double HIGHSCORE_ROW_HEIGHT = 86.0;
const double HIGHSCORE_NAME_X = 90.0;
const int HIGHSCORE_SCORE_X = 160;
const int HIGHSCORE_NAME_WIDTH = 600;

// The pulse's step a frame
const double HIGHSCORE_PULSE_STEP = 0.02;

HighScoreEntry HighScore::sEntries[HIGHSCORE_ENTRIES];
int HighScore::sNewEntry = -1;
float HighScore::sPulse = 0.0f;
bool HighScore::sPulseRising = true;

// 0x4351b0: the window with its title in red and the table
HighScore::HighScore()
{
	sPulse = 0.0f;
	SetStateInt(STATE_Z, 300);
	Open(HIGHSCORE_COLUMNS, HIGHSCORE_ROWS, 1, false);
	BuildLines(gApp->mFont, false);
	MoveLines(0, 48.0f, true);
	SetLineColor(0, 1.0f, 0.2f, 0.2f, 1.0f);
	SetLineScale(0, 1.1f, 1.1f);
	SetLineText(0, "HIGH SCORES");
	BuildEntries();
}

// 0x4352c0: the entry just made pulses only the first time the table is shown
HighScore::~HighScore()
{
	sNewEntry = -1;
}

// 0x4352e0
bool HighScore::Update()
{
	if (!BBWindow::Update())
		return false;
	if (sNewEntry >= 0)
		Pulse();
	return true;
}

// 0x435300: the new entry's colour runs between pale green and white
void HighScore::Pulse()
{
	if (sNewEntry < 0)
		return;
	float aRest = (float)(1.0 - sPulse);
	float aRed = (float)(0.2 * aRest + sPulse);
	float aGreen = (float)((double)sPulse + aRest);
	float aBlue = (float)(aRest * 0.8 + sPulse * 0.1);
	if (aRed > 1.0f)
		aRed = 1.0f;
	if (aGreen > 1.0f)
		aGreen = 1.0f;
	if (aBlue > 1.0f)
		aBlue = 1.0f;
	if (aRed < 0.0f)
		aRed = 0.0f;
	if (aGreen < 0.0f)
		aGreen = 0.0f;
	if (aBlue < 0.0f)
		aBlue = 0.0f;
	if (sPulseRising) {
		sPulse = (float)(sPulse + HIGHSCORE_PULSE_STEP);
		if (sPulse >= 1.0f) {
			sPulse = 1.0f;
			sPulseRising = false;
		}
	} else {
		sPulse = (float)(sPulse - HIGHSCORE_PULSE_STEP);
		if (sPulse <= 0.0f) {
			sPulse = 0.0f;
			sPulseRising = true;
		}
	}
	mNames[sNewEntry]->SetState(STATE_RED, aRed);
	mNames[sNewEntry]->SetState(STATE_GREEN, aGreen);
	mNames[sNewEntry]->SetState(STATE_BLUE, aBlue);
	mScores[sNewEntry]->SetState(STATE_RED, aRed);
	mScores[sNewEntry]->SetState(STATE_GREEN, aGreen);
	mScores[sNewEntry]->SetState(STATE_BLUE, aBlue);
}

// 0x4354b0: the names in gold and the scores in green, every second row a shade darker
void HighScore::BuildEntries()
{
	float aY = (float)(HIGHSCORE_FIRST_ROW - GetPanelHalfHeight());
	for (int i = 0; i < HIGHSCORE_ENTRIES; i++) {
		bool isOdd = (i & 1) != 0;

		ObjString* aName = new ObjString();
		mNames[i] = aName;
		aName->SetParent(this, nullptr);
		aName->SetFont(gApp->mFont);
		aName->Create(sEntries[i].mName, true, HIGHSCORE_NAME_WIDTH);
		aName->SetState(STATE_X, (float)(HIGHSCORE_NAME_X - GetPanelHalfWidth()));
		aName->SetState(STATE_Y, aY);
		aName->SetStateInt(STATE_Z, 1);
		if (!isOdd) {
			aName->SetStateInt(STATE_RED, 1);
			aName->SetState(STATE_GREEN, 0.8f);
			aName->SetState(STATE_BLUE, 0.1f);
		} else {
			aName->SetState(STATE_RED, 0.8f);
			aName->SetState(STATE_GREEN, 0.7f);
			aName->SetState(STATE_BLUE, 0.3f);
		}
		aName->SetStateInt(STATE_ALPHA, 1);
		Activate(aName, false);

		ObjString* aScore = new ObjString();
		mScores[i] = aScore;
		aScore->SetParent(this, nullptr);
		aScore->SetFont(gApp->mFont);
		aScore->Create(FormatScore((unsigned int)sEntries[i].mScore), true, 0);
		aScore->SetStateInt(STATE_X, HIGHSCORE_SCORE_X);
		aScore->SetState(STATE_Y, aY);
		aScore->SetStateInt(STATE_Z, 1);
		if (!isOdd) {
			aScore->SetState(STATE_RED, 0.2f);
			aScore->SetStateInt(STATE_GREEN, 1);
			aScore->SetState(STATE_BLUE, 0.3f);
		} else {
			aScore->SetState(STATE_RED, 0.3f);
			aScore->SetState(STATE_GREEN, 0.9f);
			aScore->SetState(STATE_BLUE, 0.5f);
		}
		aScore->SetStateInt(STATE_ALPHA, 1);
		Activate(aScore, false);
		aY = (float)(aY + HIGHSCORE_ROW_HEIGHT);
	}
}

// 0x435740: a score above an entry's goes in above it, the rest moving down and the last dropping off, and the menu
// shows the table next with the new entry pulsing. A score that beats none is not kept. The table is saved either way.
void HighScore::Add(int theScore, const char* theName)
{
	HighScoreEntry aBefore[HIGHSCORE_ENTRIES];
	memcpy(aBefore, sEntries, sizeof(sEntries));
	int anEntry = 0;
	while (theScore <= sEntries[anEntry].mScore) {
		anEntry++;
		if (anEntry >= HIGHSCORE_ENTRIES) {
			Save();
			return;
		}
	}
	if (anEntry < HIGHSCORE_ENTRIES - 1)
		memcpy(&sEntries[anEntry + 1], &aBefore[anEntry], (HIGHSCORE_ENTRIES - 1 - anEntry) * sizeof(HighScoreEntry));
	sEntries[anEntry].mScore = theScore;
	strncpy(sEntries[anEntry].mName, theName, HIGHSCORE_NAME_LENGTH);
	sNewEntry = anEntry;
	MainMenu::ShowHighScoresNext();
	Save();
}

// 0x4359e0
bool HighScore::Save()
{
	std::string aData((const char*)sEntries, sizeof(sEntries));
	return Platform::WriteUserFileAtomic(HIGHSCORE_FILE, aData);
}

// 0x435a40: the entries the file holds whole; a short file leaves the rest as they were
bool HighScore::Load()
{
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + HIGHSCORE_FILE, &aData))
		return false;
	size_t aSize = aData.size() / sizeof(HighScoreEntry) * sizeof(HighScoreEntry);
	if (aSize > sizeof(sEntries))
		aSize = sizeof(sEntries);
	memcpy(sEntries, aData.data(), aSize);
	return true;
}

// 0x435aa0
void Sexy::InitHighScores()
{
	if (HighScore::Load())
		return;
	static const HighScoreEntry DEFAULT_ENTRIES[HIGHSCORE_ENTRIES] = {
		{ 2500000, "MonkeyJoe" },
		{ 1000000, "EchoLogin" },
		{ 500000, "Wrender" },
		{ 250000, "DeadSilense" },
		{ 100000, "RockyRoad" },
		{ 20000, "BlitzScorch" },
		{ 10000, "Swerdnk" },
		{ 5000, "RelBeep" },
	};
	memcpy(HighScore::sEntries, DEFAULT_ENTRIES, sizeof(DEFAULT_ENTRIES));
}

// 0x4732d0: a comma before each group of three digits that has more before it
const char* Sexy::FormatScore(unsigned int theScore)
{
	static char sFormatted[32]; // 0x6a5630
	char aDigits[24];
	int aLeft = snprintf(aDigits, sizeof(aDigits), "%u", theScore);
	const char* aFrom = aDigits;
	char* aTo = sFormatted;
	*aTo = *aFrom;
	while (*aFrom != '\0') {
		char* aNext = aTo + 1;
		aFrom++;
		aLeft--;
		if (aLeft != 0 && aLeft % 3 == 0) {
			*aNext = ',';
			aNext = aTo + 2;
		}
		*aNext = *aFrom;
		aTo = aNext;
	}
	return sFormatted;
}
