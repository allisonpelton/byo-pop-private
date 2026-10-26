#include "ProgressManager.h"
#include "PixelusApp.h"
#include "LevelData.h"
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>

using namespace Sexy;

static std::string BufferToString(const Buffer& theBuffer)
{
	return std::string((const char*) theBuffer.GetDataPtr(), theBuffer.GetDataLen());
}

// 0x421380 (the original loads maxedscores.txt in the constructor: 0x421240)
ProgressManager::ProgressManager(PixelusApp* theApp, LevelData* theLevelData)
{
	mApp = theApp;
	mLevelData = theLevelData;
	mHistoryUser = -1;
	LoadBest();
}

// 0x420460: fields separated by '#': uuid, moves, solution; line breaks are ignored; an empty field is an error
// in the original ("empty field found"), which the port skips. A table entry is only taken if it beats the
// one already there.
void ProgressManager::ParseBest(const std::string& theText)
{
	std::string aField;
	std::string aUuid, aSolution;
	int aMoves = 0;
	int aIndex = 0;
	for (size_t i = 0; i < theText.size(); i++)
	{
		char c = theText[i];
		if (c == '\r' || c == '\n')
			continue;
		if (c != '#')
		{
			aField += c;
			continue;
		}
		if (aField.empty())
		{
			aIndex = 0;
			continue;
		}
		if (aIndex == 0)
			aUuid = aField;
		else if (aIndex == 1)
			aMoves = atoi(aField.c_str());
		else if (aIndex == 2)
			aSolution = aField;
		aField.clear();
		if (++aIndex == 3)
		{
			aIndex = 0;
			std::map<std::string, Best>::iterator anItr = mBest.find(aUuid);
			if (anItr == mBest.end() || aMoves < anItr->second.mMoves)
			{
				Best& aBest = mBest[aUuid];
				aBest.mMoves = aMoves;
				aBest.mSolution = aSolution;
				aBest.mImproved = false;
			}
		}
	}
}

// 0x421240
void ProgressManager::LoadBest()
{
	mBest.clear();
	Buffer aBuffer;
	if (mApp->ReadBufferFromFile("levels/maxedscores.txt", &aBuffer))
		ParseBest(BufferToString(aBuffer));
}

// 0x41ecf0: the whole table (known puzzles only) to maxedscores.txt, and the entries a player improved to
// improvedmax.txt
void ProgressManager::SaveBest()
{
	Buffer aAll, anImproved;
	int anImprovedCount = 0;
	for (std::map<std::string, Best>::const_iterator anItr = mBest.begin(); anItr != mBest.end(); ++anItr)
	{
		if (mLevelData != NULL && !mLevelData->IsKnown(anItr->first))
			continue;
		char aNumber[16];
		sprintf(aNumber, "%d", anItr->second.mMoves);
		std::string aLine = anItr->first + "#" + aNumber + "#" + anItr->second.mSolution + "#";
		aAll.WriteLine(aLine);
		if (anItr->second.mImproved)
		{
			anImproved.WriteLine(aLine);
			anImprovedCount++;
		}
	}
	mApp->WriteBufferToFile("levels/maxedscores.txt", &aAll);
	if (anImprovedCount > 0)
		mApp->WriteBufferToFile("levels/improvedmax.txt", &anImproved);
}

// 0x41e6c0
int ProgressManager::GetBestMoves(const std::string& theUuid) const
{
	const Best* aBest = FindBest(theUuid);
	return aBest == NULL ? 0 : aBest->mMoves;
}

const ProgressManager::Best* ProgressManager::FindBest(const std::string& theUuid) const
{
	std::map<std::string, Best>::const_iterator anItr = mBest.find(theUuid);
	return anItr == mBest.end() ? NULL : &anItr->second;
}

// 0x420760
void ProgressManager::UpdateBest(const std::string& theUuid, int theMoves, const std::string& theSolution, bool* theMatched)
{
	*theMatched = false;
	std::map<std::string, Best>::iterator anItr = mBest.find(theUuid);
	if (anItr == mBest.end())
	{
		Best& aBest = mBest[theUuid];
		aBest.mMoves = theMoves;
		aBest.mSolution = theSolution;
		aBest.mImproved = true;
		return;
	}
	if (theMoves <= anItr->second.mMoves)
	{
		*theMatched = true;
		if (theMoves < anItr->second.mMoves)
		{
			anItr->second.mMoves = theMoves;
			anItr->second.mSolution = theSolution;
			anItr->second.mImproved = true;
		}
	}
}

// 0x4208f0: users/his<id>, "<uuid>#<moves>#<tiles>#<undo>#<count>#" with line breaks ignored
void ProgressManager::LoadHistory(int theUserId)
{
	mHistory.clear();
	mHistoryUser = theUserId;
	char aName[64];
	sprintf(aName, "users/his%d", theUserId);
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(aName, &aBuffer))
	{
		Buffer anEmpty;
		mApp->WriteBufferToFile(aName, &anEmpty);
		return;
	}
	std::string aText = BufferToString(aBuffer);
	std::string aFields[5];
	std::string aField;
	int aIndex = 0;
	for (size_t i = 0; i < aText.size(); i++)
	{
		char c = aText[i];
		if (c == '\r' || c == '\n')
			continue;
		if (c != '#')
		{
			aField += c;
			continue;
		}
		aFields[aIndex++] = aField;
		aField.clear();
		if (aIndex == 5)
		{
			aIndex = 0;
			History& aHistory = mHistory[aFields[0]];
			aHistory.mMoves = aFields[1];
			aHistory.mTiles = aFields[2];
			aHistory.mUndo = aFields[3];
			aHistory.mCount = aFields[4];
		}
	}
}

void ProgressManager::SaveHistory(int theUserId)
{
	char aName[64];
	sprintf(aName, "users/his%d", theUserId);
	Buffer aBuffer;
	for (std::map<std::string, History>::const_iterator anItr = mHistory.begin(); anItr != mHistory.end(); ++anItr)
		aBuffer.WriteLine(anItr->first + "#" + anItr->second.mMoves + "#" + anItr->second.mTiles + "#" +
			anItr->second.mUndo + "#" + anItr->second.mCount + "#");
	mApp->WriteBufferToFile(aName, &aBuffer);
}

// 0x4211f0
const ProgressManager::History* ProgressManager::GetHistory(int theUserId, const std::string& theUuid)
{
	LoadHistory(theUserId);
	std::map<std::string, History>::const_iterator anItr = mHistory.find(theUuid);
	return anItr == mHistory.end() ? NULL : &anItr->second;
}

// 0x420cb0
void ProgressManager::SetHistory(int theUserId, const std::string& theUuid, const History& theHistory)
{
	LoadHistory(theUserId);
	mHistory[theUuid] = theHistory;
	SaveHistory(theUserId);
}

// 0x421080 (0x420cb0 with the remove flag)
void ProgressManager::RemoveHistory(int theUserId, const std::string& theUuid)
{
	LoadHistory(theUserId);
	mHistory.erase(theUuid);
	SaveHistory(theUserId);
}
