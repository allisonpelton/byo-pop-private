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
		if (!WriteBufferToFile("users\datahs.usr", &aBuffer))
			Popup(GT(0x562c88));
		return true;
	}
	return false;
}
