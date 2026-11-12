// The profiles and the high score lists (0x435e10-0x4364d0, 0x408910-0x408a20).

#include "Profiles.h"

#include "Buffer.h"
#include "VeniceApp.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

Profile gProfiles[ProfileCount]; // 0xa1c940
int gCurrentProfile = 0; // 0xa29578
HighScoreEntry gHighScores[HighScoreListCount][HighScoresPerList]; // 0xa281f0

static const char* ProfilesFileName = "userdata\\profiles.dat";

// the lists' file: the current profile's number, the profiles and the lists
static const int ProfilesFileSize = 4 + (int)sizeof(gProfiles) + (int)sizeof(gHighScores); // 0xcc3c

// 0x43a530
const char* FormatTime(unsigned int theTicks)
{
	static char aText[32];
	sprintf(aText, "%d:%02d", (int)(theTicks / 15000), (int)((theTicks / 250) % 60));
	return aText;
}

// 0x435e70
void SaveProfiles()
{
	std::vector<uint8_t> aFile(ProfilesFileSize);
	memcpy(&aFile[0], &gCurrentProfile, 4);
	memcpy(&aFile[4], gProfiles, sizeof(gProfiles));
	memcpy(&aFile[4 + sizeof(gProfiles)], gHighScores, sizeof(gHighScores));
	gApp->WriteBytesToFile(ProfilesFileName, &aFile[0], (unsigned long)aFile.size());
}

struct StartingScore
{
	const char* mName;
	int mScore;
	int mTimeTicks;
};

// the ten names of every list, with the times (in ticks) their entries show
static const char* const HighScoreNames[HighScoresPerList] = {"Marco Polo", "Casanova", "Tintoretto", "Dandolo",
	"Bellini", "Longhena", "Goldoni", "Albinoni", "Canaletto", "Titian"};
static const int HighScoreTimes[HighScoresPerList] = {375000, 350000, 325000, 300000, 250000, 275000, 250000, 225000,
	200000, 175000};

// the scores the lists of the modes start with; the fifth list keeps the first, general scores
static const int StartingScoresGeneral[HighScoresPerList] = {200000, 180000, 160000, 140000, 120000, 100000, 90000, 80000,
	70000, 60000};
static const int StartingScoresFlood[HighScoresPerList] = {2100000, 1980000, 1960000, 1940000, 1860000, 1800000, 1760000,
	1700000, 1800000, 1800000};
static const int StartingScoresSurvival[HighScoresPerList] = {150000, 140000, 130000, 120000, 110000, 100000, 80000, 60000,
	60000, 60000};
static const int StartingTimesSurvival[HighScoresPerList] = {225000, 210000, 195000, 180000, 165000, 150000, 120000, 90000,
	90000, 90000};
static const int StartingScoresTrickShot[HighScoresPerList] = {550000, 520000, 460000, 400000, 350000, 320000, 300000,
	250000, 250000, 250000};

static void FillHighScoreList(HighScoreEntry* theList, const int* theScores, const int* theTimes)
{
	for (int i = 0; i < HighScoresPerList; i++) {
		strcpy(theList[i].mName, HighScoreNames[i]);
		theList[i].mScore = theScores[i];
		strcpy(theList[i].mTimeText, FormatTime((unsigned int)theTimes[i]));
	}
}

// 0x408a20: the starting lists of the modes, then every profile empty with its tries at three
static void SetStartingHighScores()
{
	FillHighScoreList(gHighScores[GameModeJourney], StartingScoresFlood, HighScoreTimes);
	FillHighScoreList(gHighScores[GameModeFlood], StartingScoresFlood, HighScoreTimes);
	FillHighScoreList(gHighScores[GameModeTrickShot], StartingScoresTrickShot, HighScoreTimes);
	FillHighScoreList(gHighScores[GameModeSurvival], StartingScoresSurvival, StartingTimesSurvival);
	memset(gProfiles, 0, sizeof(gProfiles));
	for (int aProfile = 0; aProfile < ProfileCount - 1; aProfile++)
		for (int aMode = 0; aMode < GameModeCount; aMode++)
			gProfiles[aProfile].mTriesLeft[aMode] = TriesPerLevel;
}

// 0x435f10
void SetDefaultProfiles()
{
	gCurrentProfile = 0;
	for (int aList = 0; aList < HighScoreListCount; aList++)
		FillHighScoreList(gHighScores[aList], StartingScoresGeneral, HighScoreTimes);
	SetStartingHighScores(); // which also empties the profiles

	SaveProfiles();
}

// 0x436300: a file of any other size is replaced by the defaults
void LoadProfiles()
{
	Sexy::Buffer aFile;
	bool aRead = gApp->ReadBufferFromFile(ProfilesFileName, &aFile) && aFile.GetDataLen() == ProfilesFileSize;
	if (!aRead) {
		SetDefaultProfiles();
		return;
	}
	const uint8_t* aData = (const uint8_t*)aFile.GetDataPtr();
	memcpy(&gCurrentProfile, aData, 4);
	memcpy(gProfiles, aData + 4, sizeof(gProfiles));
	memcpy(gHighScores, aData + 4 + sizeof(gProfiles), sizeof(gHighScores));
}

// 0x408a00: the list is sorted from the highest score (a score that ties goes after the other)
static int CompareHighScores(const void* theFirst, const void* theSecond)
{
	return ((const HighScoreEntry*)theFirst)->mScore < ((const HighScoreEntry*)theSecond)->mScore ? 1 : -1;
}

// 0x4363d0: the ninth entry is replaced and the nine sorted, so the tenth (never replaced) stays last
bool EnterHighScore(const char* theTimeText)
{
	Profile& aProfile = CurrentProfile();
	HighScoreEntry* aList = gHighScores[aProfile.mMode];
	strcpy(aList[8].mName, aProfile.mName);
	aList[8].mScore = aProfile.mScore;
	if (theTimeText == nullptr || theTimeText[0] == 0)
		aList[8].mTimeText[0] = 0;
	else
		strcpy(aList[8].mTimeText, theTimeText);
	qsort(aList, 9, sizeof(HighScoreEntry), CompareHighScores);
	SaveProfiles();
	return true;
}

// 0x408910: only the Trick Shot mode forgets its tricks
void ClearTrickShotProgress()
{
	Profile& aProfile = CurrentProfile();
	if (aProfile.mMode == GameModeTrickShot) {
		memset(aProfile.mTrickShotDone, 0, sizeof(aProfile.mTrickShotDone));
		aProfile.mTricksInARow = 0;
	}
}

// 0x436ae0
void ResetProgress()
{
	Profile& aProfile = CurrentProfile();
	int aMode = aProfile.mMode;
	aProfile.mLevelIndex[aMode] = 0;
	aProfile.mLevelStartScore[aMode] = 0;
	aProfile.mScore = 0;
	aProfile.mTriesLeft[aMode] = TriesPerLevel;
	aProfile.mStartTries[aMode] = TriesPerLevel;
	ClearTrickShotProgress();
}

// 0x436bb0
int CountProfiles()
{
	int aCount = 0;
	for (int i = 0; i < ProfileCount; i++)
		if (gProfiles[i].mName[0] != 0)
			aCount++;
	return aCount;
}
