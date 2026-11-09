// The hall of fame: the best results of all players, kept in users\datahs.usr.

#include "AstroPopApp.h"
#include "Buffer.h"
#include "GameText.h"

using namespace Sexy;

// 0x429a40: puts a result into the hall of fame if it ranks. Kind 1 ranks by the score (theSecond is the level), kind 2
// by the time played (theSecond, in seconds). The result goes in front of the first entry of its kind that it
// matches or beats; of the entries of its kind only the best are kept (the lowest goes when there are five), and the file
// is written again. Returns whether the result ranked.
bool AstroPopApp::AddHallOfFame(const std::string& theName, int theScore, int theSecond, int theMode, int theKind)
{
	for (unsigned int i = 0; i < mHall.size(); i++)
	{
		bool aBefore = false;
		if (theKind == 1)
			aBefore = mHall[i].mKind == 1 && mHall[i].mScore <= theScore;
		else if (theKind == 2)
			aBefore = mHall[i].mKind == 2 && mHall[i].mSecond <= theSecond;
		if (!aBefore)
			continue;

		HallEntry anEntry;
		anEntry.mName = theName;
		anEntry.mScore = theScore;
		anEntry.mSecond = theSecond;
		anEntry.mMode = theMode;
		anEntry.mKind = theKind;
		mHall.insert(mHall.begin() + i, anEntry);

		// the lowest of the kind goes when there are more than four
		int aLowest = 0x7ffffc17;
		int aLowestIndex = -1;
		int aCount = 0;
		for (unsigned int j = 0; j < mHall.size(); j++)
		{
			if (mHall[j].mKind != theKind)
				continue;
			int aValue = theKind == 1 ? mHall[j].mScore : mHall[j].mSecond;
			aCount++;
			if (aValue <= aLowest)
			{
				aLowestIndex = j;
				aLowest = aValue;
			}
		}
		if (aLowestIndex != -1 && aCount > 4)
			mHall.erase(mHall.begin() + aLowestIndex);

		Buffer aBuffer;
		aBuffer.WriteString(mProdName);
		aBuffer.WriteString(mProductVersion);
		for (unsigned int j = 0; j < mHall.size(); j++)
		{
			aBuffer.WriteString(mHall[j].mName);
			aBuffer.WriteLong(mHall[j].mScore);
			aBuffer.WriteLong(mHall[j].mSecond);
			aBuffer.WriteLong(mHall[j].mMode);
			aBuffer.WriteLong(mHall[j].mKind);
		}
		if (!WriteBufferToFile("users\\datahs.usr", &aBuffer))
			Popup(GT(0x562c88));
		return true;
	}
	return false;
}

// 0x42a0b0: the hall of fame starts with ten results (0x558040 makes them: five of the classic game, with the level
// reached, then five of the survival game, with the seconds played; the last number is the pilot). The file of the
// hall, users\datahs.usr, replaces them one by one as far as it goes. A file of an older version is deleted first.
void AstroPopApp::LoadHighScores()
{
	static const struct
	{
		unsigned int	mName;      // the text of the name
		int				mScore;
		int				mSecond;
		int				mMode;
		int				mKind;
	} kDefaults[10] = {
		{0x5639dc, 100000, 10, 2, 1},
		{0x5639d4, 90000, 9, 1, 1},
		{0x5639cc, 80000, 8, 2, 1},
		{0x5639c4, 70000, 7, 3, 1},
		{0x5639c0, 60000, 6, 0, 1},
		{0x5639dc, 50000, 300, 1, 2},
		{0x5639d4, 40000, 240, 2, 2},
		{0x5639c0, 30000, 180, 3, 2},
		{0x5639b8, 20000, 120, 0, 2},
		{0x5639b4, 10000, 60, 1, 2}
	};
	for (int i = 0; i < 10; i++)
	{
		HallEntry anEntry;
		anEntry.mName = GT(kDefaults[i].mName);
		anEntry.mScore = kDefaults[i].mScore;
		anEntry.mSecond = kDefaults[i].mSecond;
		anEntry.mMode = kDefaults[i].mMode;
		anEntry.mKind = kDefaults[i].mKind;
		mHall.push_back(anEntry);
	}

	if (mOldScoresDeleted)
		EraseFile("users\\datahs.usr");

	Buffer aBuffer;
	if (!ReadBufferFromFile("users\\datahs.usr", &aBuffer, false))
		return;

	// the name of the product and its version come first
	aBuffer.ReadString();
	aBuffer.ReadString();
	size_t anIndex = 0;
	while (!aBuffer.AtEnd())
	{
		std::string aName = aBuffer.ReadString();
		bool aShort = aBuffer.AtEnd();
		int aScore = 0, aSecond = 0, aMode = 0, aKind = 0;
		if (!aShort)
		{
			aScore = aBuffer.ReadLong();
			aShort = aBuffer.AtEnd();
		}
		if (!aShort)
		{
			aSecond = aBuffer.ReadLong();
			aShort = aBuffer.AtEnd();
		}
		if (!aShort)
		{
			aMode = aBuffer.ReadLong();
			aShort = aBuffer.AtEnd();
		}
		if (aShort)
		{
			Popup(GT(0x562cd8));
			return;
		}
		aKind = aBuffer.ReadLong();

		mHall[anIndex].mName = aName;
		mHall[anIndex].mScore = aScore;
		mHall[anIndex].mSecond = aSecond;
		mHall[anIndex].mMode = aMode;
		mHall[anIndex].mKind = aKind;
		anIndex++;
		if (anIndex >= mHall.size())
			return;
	}
}
