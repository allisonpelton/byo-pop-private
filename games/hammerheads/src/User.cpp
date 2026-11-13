#include "UserManager.h"

#include "Application.h"
#include "Common.h"
#include "GameScreen.h"
#include "Stats.h"

using namespace Sexy;

// 0x592950, made at start-up: the "version" of the player files the game writes and takes
const char* const Sexy::PROFILE_VERSION = "7";

// 0x46d500: a new player: the how-to-play movie is to be shown, the starting point is the only restart point of each list
User::User(const std::string& theName, int theId) :
	mName(theName),
	mIsCurrent(false),
	mShowHowToPlay(true),
	mToughUnlocked(false),
	mToughAnnounced(false),
	mId(theId)
{
	for (int i = 0; i < NUM_TROPHIES; i++)
		mTrophies[i] = false;
	SetUpRestartPoints();
}

// 0x471380: the player is what the file says (Load, 0x46e500, sets the rest to the start's values first)
User::User(int theId) :
	mIsCurrent(false),
	mShowHowToPlay(true),
	mToughUnlocked(false),
	mToughAnnounced(false),
	mId(theId)
{
	Load();
}

// 0x46a3c0
bool User::IsCurrentVersion() const
{
	return mVersion == PROFILE_VERSION;
}

SavedGame& User::GetSavedGame(int theMode)
{
	if (theMode >= 0 && theMode < NUM_SAVED_GAMES)
		return mSavedGames[theMode];
	return mSavedGames[0];
}

const SavedGame& User::GetSavedGame(int theMode) const
{
	if (theMode >= 0 && theMode < NUM_SAVED_GAMES)
		return mSavedGames[theMode];
	return mSavedGames[0];
}

std::vector<RestartPoint>* User::GetRestartPoints(int theMode)
{
	if (theMode == GAME_MODE_CLASSIC)
		return &mClassicRestartPoints;
	if (theMode == GAME_MODE_TOUGH_COOKIE)
		return &mToughRestartPoints;
	return nullptr;
}

// 0x469a10: each list starts with the point before level 1 (the level number -1 is what the next level's number, 0, is made from)
void User::SetUpRestartPoints()
{
	mClassicRestartPoints.clear();
	RestartPoint aClassicStart;
	aClassicStart.mLevel = -1;
	mClassicRestartPoints.push_back(aClassicStart);

	mToughRestartPoints.clear();
	RestartPoint aToughStart;
	aToughStart.mLevel = -1;
	mToughRestartPoints.push_back(aToughStart);
}

// 0x469e40
bool User::HasTrophy(int theTrophy) const
{
	return mTrophies[theTrophy];
}

// 0x469e50
void User::AwardTrophy(int theTrophy)
{
	mTrophies[theTrophy] = true;
}

// 0x469bf0
bool User::HasSavedGame() const
{
	const SavedGame& aSaved = GetSavedGame(gApp->mGameMode);
	return aSaved.mState != 0 && aSaved.mVersion == SAVED_GAME_VERSION;
}

// 0x469c40
void User::ClearSavedGame()
{
	GetSavedGame(gApp->mGameMode).mState = 0;
}

// 0x469c90
int User::GetSavedGameLevel() const
{
	return GetSavedGame(gApp->mGameMode).mLevel;
}

// 0x469de0: the score is the thirteenth of the stats
int User::GetSavedGameScore() const
{
	const SavedGame& aSaved = GetSavedGame(gApp->mGameMode);
	if ((int)aSaved.mStats.size() <= STAT_SCORE)
		return 0;
	return (int)aSaved.mStats[STAT_SCORE];
}

// 0x46a2b0: "2 hours and 5 minutes", "5 minutes and 1 second", "40 seconds": the two biggest units that are not zero
static std::string FormatTime(int theHours, int theMinutes, int theSeconds)
{
	const char* anHourWord = theHours == 1 ? "hour" : "hours";
	const char* aMinuteWord = theMinutes == 1 ? "minute" : "minutes";
	const char* aSecondWord = theSeconds == 1 ? "second" : "seconds";
	if (theHours > 0)
		return StrFormat("%d %s and %d %s", theHours, anHourWord, theMinutes, aMinuteWord);
	if (theMinutes > 0)
		return StrFormat("%d %s and %d %s", theMinutes, aMinuteWord, theSeconds, aSecondWord);
	return StrFormat("%d %s", theSeconds, aSecondWord);
}

// 0x46a350
std::string User::GetSavedGameTime() const
{
	int aTime = (int)GetSavedGame(gApp->mGameMode).mTime;
	int aHours = aTime / 3600;
	int aMinutes = (aTime % 3600) / 60;
	int aSeconds = (aTime % 3600) % 60;
	return FormatTime(aHours, aMinutes, aSeconds);
}

// 0x469d10
int User::GetRestartPointCount(int theMode) const
{
	if (theMode == GAME_MODE_CLASSIC)
		return (int)mClassicRestartPoints.size();
	if (theMode == GAME_MODE_TOUGH_COOKIE)
		return (int)mToughRestartPoints.size();
	return 0;
}

// 0x469d40
const RestartPoint* User::GetRestartPoint(int theIndex) const
{
	const std::vector<RestartPoint>* aList = nullptr;
	if (gApp->mGameMode == GAME_MODE_CLASSIC)
		aList = &mClassicRestartPoints;
	else if (gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
		aList = &mToughRestartPoints;
	if (aList == nullptr || theIndex < 0 || theIndex >= (int)aList->size())
		return nullptr;
	return &(*aList)[theIndex];
}

// 0x469cd0 (the work is 0x4698d0): the game on screen becomes a restart point; one that is already there for the same level
// is replaced
void User::AddRestartPoint()
{
	std::vector<RestartPoint>* aList = GetRestartPoints(gApp->mGameMode);
	if (aList == nullptr)
		return;

	RestartPoint aPoint;
	aPoint.Capture();
	for (int i = 0; i < (int)aList->size(); i++)
	{
		if ((*aList)[i].mLevel == aPoint.mLevel)
		{
			(*aList)[i] = aPoint;
			return;
		}
	}
	aList->push_back(aPoint);
}

// 0x469d90 (the work is 0x4698b0)
void User::ResumeFromRestartPoint(int theIndex)
{
	std::vector<RestartPoint>* aList = GetRestartPoints(gApp->mGameMode);
	if (aList == nullptr || theIndex < 0 || theIndex >= (int)aList->size())
		return;
	(*aList)[theIndex].Restore();
}
