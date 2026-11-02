// The ranks (Ranks.h).
#include "Ranks.h"

#include "Buffer.h"
#include "GameText.h"
#include "WinTwistApp.h"

#include <cctype>

using namespace Sexy;

// 0x757a41: the first comma-separated field of a line, with its quotes (a comma inside quotes does not end it)
SexyString Sexy::FirstField(const SexyString& theLine)
{
	SexyString aField;
	unsigned int i = 0;
	while (i < theLine.size() && isspace((unsigned char)theLine[i]))
		i++;
	bool inQuotes = false;
	for (; i < theLine.size(); i++) {
		if (inQuotes) {
			if (theLine[i] == '"')
				inQuotes = false;
		} else if (theLine[i] == '"') {
			inQuotes = true;
		} else if (theLine[i] == ',') {
			break;
		}
		aField += theLine[i];
	}
	return aField;
}

// 0x7579e8: theString without theChar
SexyString Sexy::RemoveChar(const SexyString& theString, SexyChar theChar)
{
	SexyString aResult;
	for (SexyChar aChar : theString) {
		if (aChar != theChar)
			aResult += aChar;
	}
	return aResult;
}

Ranks::Ranks()
{
	Load();
}

void Ranks::Load()
{
	// port: the original reads the file a character at a time through the framework's text reader; the file is plain
	// ASCII, read here a byte at a time. As in the original a last line with no line end is dropped (the game's file
	// ends in one).
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile("properties/ranks.txt", &aBuffer, true))
		return;
	SexyString aLine;
	int anIndex = 0;
	int aLastPoints = 0;
	for (unsigned char aChar : aBuffer.mData) {
		if (aChar != '\n' && aChar != '\r') {
			aLine += (SexyChar)aChar;
			continue;
		}
		aLine = Trim(aLine);  // 0x45d880
		if (!aLine.empty()) {
			SexyString aField = FirstField(aLine);
			int aPoints = 0;
			if (anIndex > 0) {
				if (anIndex == 1)
					aPoints = 15;
				else if (anIndex == 2)
					aPoints = aLastPoints + 30;
				else
					aPoints = (anIndex - 1) * 25 + aLastPoints;
			}
			SexyString aName = RemoveChar(aField, '"');
			if (!aName.empty())
				mRanks.push_back(Rank{aPoints, aName});
			anIndex++;
			aLastPoints = aPoints;
		}
		aLine.clear();
	}
}

SexyString Ranks::GetName(int theRank) const
{
	if (theRank < 0)
		return gApp->GetString(GTS(0x7ea294), GT(0x7ea284));
	if (mRanks.empty())
		return _S("");
	if ((unsigned int)theRank < mRanks.size())
		return mRanks[theRank].mName;
	return mRanks.back().mName;
}

int Ranks::GetPoints(unsigned int theRank) const
{
	int aPoints = 0;
	if (mRanks.size() != 0) {
		if (mRanks.size() <= theRank) {
			if (mRanks.size() != 1) {
				int aBefore = mRanks[mRanks.size() - 2].mPoints;
				int aLastPoints = mRanks[mRanks.size() - 1].mPoints;
				int aLast = (int)mRanks.size() - 1;
				if ((int)theRank <= aLast)
					return mRanks[aLast].mPoints;
				return mRanks[aLast].mPoints + (theRank - aLast) * (aLastPoints - aBefore);
			}
			theRank = 0;
		}
		aPoints = mRanks[theRank].mPoints;
	}
	return aPoints;
}
