#include "LevelData.h"
#include "PixelusApp.h"
#include "PakInterface.h"
#include <stdio.h>

using namespace Sexy;

LevelData::LevelData(PixelusApp* theApp)
{
	mApp = theApp;
	mCurrentPuzzle = -1;
	Load();
}

// 0x42a870: the tags are "#level#<name>" and "#puzzle#<file>" (the value runs to the next '#' and ignores line breaks);
// each puzzle's uuid comes from the first 42 bytes of its file (0x429c70)
void LevelData::Load()
{
	mLevels.clear();
	mFileByUuid.clear();

	PFILE* aFile = p_fopen("levels/levels.txt", "r");
	if (aFile == NULL)
	{
		fprintf(stderr, "LevelData::Load() : couldn't open level data file\n");
		return;
	}

	std::string aTag, aLevel, aPuzzle;
	bool inTag = false;		// reading a tag name
	bool inValue = false;	// reading a value, after the closing '#' of the tag
	bool aFirst = true;
	for (;;)
	{
		int aChar = p_fgetc(aFile);
		if (aChar == -1)
			break;
		if (aChar == '\r' || aChar == '\n')
			continue;
		if (aChar == '#')
		{
			if (inValue)
			{
				inValue = false;
				inTag = true;
				if (!aPuzzle.empty())
				{
					// a finished "#puzzle#" value
					PFILE* aPuzzleFile = p_fopen(("levels/" + aPuzzle).c_str(), "r");
					char aHead[42];
					if (aPuzzleFile != NULL && p_fread(aHead, 1, 42, aPuzzleFile) == 42u && std::string(aHead, 6) == "#uuid#")
					{
						Puzzle aNew;
						aNew.mFile = aPuzzle;
						aNew.mUuid = UuidKey(std::string(aHead + 6, 36));
						mLevels[aLevel].mName = aLevel;
						mLevels[aLevel].mPuzzles.push_back(aNew);
						mFileByUuid[aNew.mUuid] = aPuzzle;
					}
					else
						fprintf(stderr, "LevelData::Load() : puzzle file %s not found or without uuid\n", aPuzzle.c_str());
					if (aPuzzleFile != NULL)
						p_fclose(aPuzzleFile);
					aPuzzle.clear();
				}
				aTag.clear();
			}
			else if (inTag)
			{
				inTag = false;
				inValue = true;
				if (aTag == "level")
				{
					aLevel.clear();
					mLevels[aLevel];
				}
				else if (aTag == "puzzle")
					aPuzzle.clear();
			}
			else
			{
				inTag = true;
				aTag.clear();
			}
			aFirst = false;
			continue;
		}
		if (inTag)
			aTag += (char)aChar;
		else if (inValue)
		{
			if (aTag == "level")
				aLevel += (char)aChar;
			else if (aTag == "puzzle")
				aPuzzle += (char)aChar;
		}
		else if (aFirst)
			break;
	}
	// the last "#puzzle#" value ends with the file
	if (!aPuzzle.empty())
	{
		PFILE* aPuzzleFile = p_fopen(("levels/" + aPuzzle).c_str(), "r");
		char aHead[42];
		if (aPuzzleFile != NULL && p_fread(aHead, 1, 42, aPuzzleFile) == 42u && std::string(aHead, 6) == "#uuid#")
		{
			Puzzle aNew;
			aNew.mFile = aPuzzle;
			aNew.mUuid = UuidKey(std::string(aHead + 6, 36));
			mLevels[aLevel].mName = aLevel;
			mLevels[aLevel].mPuzzles.push_back(aNew);
			mFileByUuid[aNew.mUuid] = aPuzzle;
		}
		if (aPuzzleFile != NULL)
			p_fclose(aPuzzleFile);
	}
	p_fclose(aFile);
}

// port: a helper of the port (Sexy::UuidKey)
std::string Sexy::UuidKey(const std::string& theUuid)
{
	std::string aKey = theUuid;
	for (size_t i = 0; i < aKey.size(); i++)
		if (aKey[i] >= 'A' && aKey[i] <= 'Z')
			aKey[i] = (char) (aKey[i] - 'A' + 'a');
	return aKey;
}

// port: a helper of the port (LevelData::GetLevel)
const LevelData::Level* LevelData::GetLevel(const std::string& theName) const
{
	std::map<std::string, Level>::const_iterator anItr = mLevels.find(theName);
	return anItr == mLevels.end() ? NULL : &anItr->second;
}

// 0x4283e0
int LevelData::GetPuzzleCount(const std::string& theName) const
{
	const Level* aLevel = GetLevel(theName);
	return aLevel == NULL ? -1 : (int)aLevel->mPuzzles.size();
}
