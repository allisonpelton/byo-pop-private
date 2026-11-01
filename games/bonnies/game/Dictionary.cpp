// The word list and the letters' odds (see Dictionary.h).

#include "Dictionary.h"
#include "App.h"
#include "BonniesApp.h"
#include "Book.h"
#include "File.h"
#include "Game.h"
#include "Globals.h"
#include "Host.h"
#include "Language.h"
#include "Options.h"
#include "Platform.h"

#include <cstring>

namespace bonnies
{

LetterOdds gLetterOdds[37];
float gVowelShare;
int gLetterWeights;
int gAllWeights;
int gLetterTotal;
int gAllTotal;
static int gLastVowel;          // 0x475e8c: whether the last letter given was a vowel
static int gRun;                // 0x475e90: how many of that kind in a row
static uint32_t gLastPickTime;  // 0x475e94

void loadLetterOdds()
{
	for (int i = 0; i < 37; i++)
	{
		gLetterOdds[i].mCount = exeI32(0x466470 + i * 0xc);
		gLetterOdds[i].mCumulative = exeI32(0x466474 + i * 0xc);
		gLetterOdds[i].mChar = (char)*exePtr(0x466478 + i * 0xc);
	}
	gVowelShare = exeFloat(0x46662c);
}

// 0x404450
void resetLetterOdds()
{
	int aVowels = 0;
	int aSum = 0;
	int aLetters = 0;
	LetterOdds* anOdds = gLetterOdds;
	do
	{
		int aCount = anOdds->mCount;
		char aChar = anOdds->mChar;
		aSum += aCount;
		anOdds->mCumulative = aSum;
		if ('@' < aChar && aChar < '[')
		{
			aLetters += aCount;
			if (aChar == 'A' || aChar == 'E' || aChar == 'I' || aChar == 'O' || aChar == 'U')
				aVowels += aCount;
		}
		anOdds++;
	} while (anOdds < gLetterOdds + 37);
	int aDivisor = 1;
	if (!(aLetters < 1))
		aDivisor = aLetters;
	gVowelShare = (float)((double)aVowels / (double)aDivisor);
	gLetterWeights = 0;
	anOdds = gLetterOdds;
	do
	{
		gLetterWeights = gLetterWeights + anOdds[1].mCount + anOdds[0].mCount;
		anOdds += 2;
	} while (anOdds < gLetterOdds + 26);
	gAllWeights = 0;
	anOdds = gLetterOdds;
	do
	{
		gAllWeights = gAllWeights + anOdds->mCount;
		anOdds++;
	} while (anOdds < gLetterOdds + 37);
	gLetterTotal = gLetterWeights;
	gAllTotal = gAllWeights;
}

// 0x404510
Dictionary::~Dictionary()
{
	delete[] mRarities;
	delete[] mWords;
	delete[] mSpellings;
}

Dictionary::Dictionary()
{
}

// 0x404570
Str expandWord(Str theWord, int theWild)
{
	int i = 0;
	int aLength = (int)theWord.size();
	if (0 < aLength)
	{
		do
		{
			if (theWild != 0 && (at(theWord, i) == '?' || at(theWord, i) == '!'))
			{
				Str aWord = "";
				if (0 < i)
					aWord += substr(theWord, 0, i);
				aWord += lit(0x457ae4);
				if (i + 1 < aLength)
					aWord += substr(theWord, i + 1, aLength);
				theWord = aWord;
			}
			Str aPair = "";
			switch (at(theWord, i))
			{
			case 'Q':
				aPair = lit(0x457ae0);
				break;
			case 'a':
				aPair = lit(0x457adc);
				break;
			case 'c':
				aPair = lit(0x457ad8);
				break;
			case 'e':
				aPair = lit(0x457ad4);
				break;
			case 'i':
				aPair = lit(0x457ad0);
				break;
			case 'l':
				aPair = lit(0x457acc);
				break;
			case 'r':
				aPair = lit(0x457ac8);
				break;
			case 's':
				aPair = lit(0x457ac4);
				break;
			case 't':
				aPair = lit(0x457ac0);
				break;
			}
			if (!aPair.empty())
			{
				Str aWord = "";
				if (0 < i)
					aWord += substr(theWord, 0, i);
				aWord += aPair;
				if (i + 1 < aLength)
					aWord += substr(theWord, i + 1, aLength);
				theWord = aWord;
				aLength++;
			}
			i++;
		} while (i < aLength);
	}
	return theWord;
}

// 0x404880
int Dictionary::isWord(Str theWord, int theExpand)
{
	if (theWord.empty())
		return 0;
	if (theExpand != 0)
		theWord = expandWord(theWord, 0);
	int aLength = (int)theWord.size();
	if (aLength <= 2)
		return 0;
	if (gBonnies->mDebug1EC != 0)
		return 1;
	mMatch = "";
	mMatchRarity = 0;
	int aWild = 0;
	int i = 0;
	if (0 < aLength)
	{
		do
		{
			if (at(theWord, i) == '*')
				return 0;
			if (at(theWord, i) == '?' || at(theWord, i) == '!')
				aWild++;
			i++;
		} while (i < aLength);
		if (0 < aWild)
		{
			int aFirst = 0;
			int j = 0;
			int aLast = mCount - 1;
			if (at(theWord, 0) != '?' && at(theWord, 0) != '!')
			{
				int aLetter = (char)at(theWord, 0) - 0x41;
				if (-1 < aLetter && aLetter < 0x1a)
				{
					aFirst = mStarts[aLetter];
					if ((char)at(theWord, 0) < 'Z')
						aLast = mStarts[aLetter + 1] - 1;
				}
			}
			Str aBest = "";
			int aRarity = 0;
			if (2 < aWild)
				aRarity = 1;
			// The original has room for 20 letters; a longer word overran it
			int aWilds[128];
			do
			{
				aWilds[j] = (at(theWord, j) == '?' || at(theWord, j) == '!') ? 1 : 0;
				j++;
			} while (j < aLength);
			for (; aFirst <= aLast; aFirst++)
			{
				if (aLength == (int)mWords[aFirst].size() && aRarity < (int)mRarities[aFirst])
				{
					int k = 1;
					if (1 < aLength)
					{
						do
						{
							if (at(mWords[aFirst], k) != at(theWord, k) && aWilds[k] == 0)
								goto next;
							k++;
						} while (k < aLength);
					}
					aRarity = mRarities[aFirst];
					aBest = mWords[aFirst];
				}
			next:;
			}
			if (aBest.empty())
				return 0;
			mMatch = aBest;
			mMatchRarity = aRarity;
			return 1;
		}
	}
	return lookup(theWord);
}

// 0x404c50
int Dictionary::lookup(Str theWord)
{
	int aLength = (int)theWord.size();
	int i = 0;
	if (0 < aLength)
	{
		do
		{
			if (at(theWord, i) == '*')
				return 0;
			if (at(theWord, i) == '?' || at(theWord, i) == '!')
			{
				Str aBest = "";
				int aRarity = -99;
				const LetterOdds* anOdds = gLetterOdds;
				do
				{
					Str aWord = "";
					if (0 < i)
						aWord += substr(theWord, 0, i);
					aWord += anOdds->mChar;
					if (i < aLength - 1)
						aWord += substr(theWord, i + 1, aLength);
					if (isWord(aWord, 0) != 0)
					{
						int aWordRarity = mMatchRarity;
						if (0 < aWordRarity && aRarity < aWordRarity)
						{
							aBest = mMatch;
							aRarity = aWordRarity;
						}
					}
					anOdds++;
				} while (anOdds < gLetterOdds + 26);
				if (aBest.empty())
					return 0;
				mMatch = aBest;
				mMatchRarity = aRarity;
				return 1;
			}
			i++;
		} while (i < aLength);
	}
	int aLow = 0;
	int aRange = mCount - 1;
	int aHigh = aRange;
	while (1 < aRange)
	{
		int aMiddle = aHigh + aLow >> 1;
		int aCompare = strcmp(theWord.c_str(), mWords[aMiddle].c_str());
		int aNewHigh = aMiddle;
		if (-1 < aCompare)
		{
			aNewHigh = aHigh;
			aLow = aMiddle;
			if (aCompare < 1)
			{
				mMatch = mSpellings[aMiddle];
				mMatchRarity = mRarities[aMiddle];
				return 1;
			}
		}
		aHigh = aNewHigh;
		aRange = aNewHigh - aLow;
	}
	if (strcmp(theWord.c_str(), mWords[aLow].c_str()) == 0)
	{
		mMatch = mSpellings[aLow];
		mMatchRarity = mRarities[aLow];
		return 1;
	}
	if (aHigh != aLow && strcmp(theWord.c_str(), mWords[aHigh].c_str()) == 0)
	{
		mMatch = mSpellings[aHigh];
		mMatchRarity = mRarities[aHigh];
		return 1;
	}
	// The texts 1001 to 1009 are words too
	int anId = 0x3e9;
	while (equalsNoCase(theWord, getText(anId)) == 0)
	{
		anId++;
		if (0x3f1 < anId)
			return 0;
	}
	return 1;
}

// 0x405050
char* normalizeWord(char* theWord)
{
	int aLength = (int)strlen(theWord);
	int i = 0;
	if (0 < aLength)
	{
		do
		{
			unsigned char aChar = toUpper((unsigned char)theWord[i]);
			theWord[i] = (char)aChar;
			unsigned char aPlain = aChar;
			if (0xbf < aChar)
			{
				if (aChar == 0xdf)
					aPlain = 'S';
				if (aChar == 0xd1)
					aPlain = 'N';
				if (aChar == 199)
					aPlain = 'C';
				if (aChar < 0xc6)
					aPlain = 'A';
				if (199 < aChar && aChar < 0xcc)
					aPlain = 'E';
				if (0xcb < aChar && aChar < 0xd0)
					aPlain = 'I';
				if (0xd1 < aChar && aChar < 0xd7)
					aPlain = 'O';
				if (0xd8 < aChar && aChar < 0xdd)
					aPlain = 'U';
				if (0xdc < aChar && aChar < 0xde)
					aPlain = 'Y';
			}
			theWord[i] = (char)aPlain;
			i++;
		} while (i < aLength);
	}
	return theWord;
}

// 0x4050e0
void updateLetterOdds()
{
	int aCounts[26] = {};
	int aVowels = 0;
	int aTotal = 0;
	int aCell = 0;
	int aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0 && (gGame->mTileFlags[aCell + aRow] & 2) == 0)
			{
				char aChar = gGame->mLetters[aCell + aRow];
				if ('@' < aChar && aChar < '[')
				{
					aTotal++;
					aCounts[aChar - 'A']++;
					if (aChar == 'A' || aChar == 'E' || aChar == 'I' || aChar == 'O' || aChar == 'U')
						aVowels++;
				}
			}
			aRow++;
		} while (aRow < 9);
		aCell += 9;
		aColumn++;
	} while (aCell < 0x51);
	if (aTotal < 0xc)
	{
		resetLetterOdds();
		return;
	}
	float aTotalF = (float)aTotal;
	double aShare = (double)aVowels / aTotalF;
	if (!(aShare > 0.0099999998f))
		aShare = 0.0099999998f;
	double aVowelFactor = ((gVowelShare / aShare) * 0.60000002f + 0.40000001f) * 1.03f;
	if (aVowelFactor < 0.30000001f)
		aVowelFactor = 0.30000001f;
	else if (!(aVowelFactor <= 3.0f))
		aVowelFactor = 3.0f;
	float aPerTile = (float)(1.0f / (double)aTotalF);
	double aPerWeight = 1.0f / (double)gLetterWeights;
	int aCumulative = 0;
	LetterOdds* anOdds = gLetterOdds;
	int i = 0;
	do
	{
		double aShown = aCounts[i] * (double)aPerTile;
		float aCount = (float)anOdds->mCount;
		double anExpected = aCount * aPerWeight;
		if (anExpected == 0.0f)
			anExpected = 0.001f;
		double aFactor = aShown / anExpected;
		char aChar = (char)(i + 'A');
		if (aFactor > 1.0f)
		{
			if (!(aFactor < 3.0f))
				aFactor = 3.0f;
			aFactor = 1.0f / aFactor;
		}
		else
		{
			aFactor = (1.0f - aFactor) + 1.0f;
			if ((aChar == 'Q' || aChar == 'J' || aChar == 'X' || aChar == 'Z') && !(aFactor < 1.2f))
				aFactor = 1.2f;
		}
		if (aChar == 'A' || aChar == 'E' || aChar == 'I' || aChar == 'O' || aChar == 'U')
			aFactor = aFactor * aVowelFactor;
		else
			aFactor = aFactor / aVowelFactor;
		aCumulative = (int)(aCount * aFactor + aCumulative);
		anOdds->mCumulative = aCumulative;
		anOdds++;
		i++;
	} while (anOdds < gLetterOdds + 26);
	gLetterTotal = aCumulative;
	gAllTotal = (gAllWeights - gLetterWeights) + aCumulative;
}

// 0x405390
char randomLetter(int theColumn, int theAll)
{
	updateLetterOdds();
	if (500 < (int)(host()->getTicks() - gLastPickTime))
		gRun = 0;
	gLastPickTime = host()->getTicks();
	int aTotal = gAllTotal;
	if (theAll == 0)
		aTotal = gLetterTotal;
	int aTries = 0;
	do
	{
		aTries++;
		int aPick = randomRange(0, aTotal - 1);
		int i = 0;
		do
		{
			if (aPick < gLetterOdds[i].mCumulative)
			{
				char aChar = gLetterOdds[i].mChar;
				if (gGame->currentBook()->allows(aChar, theColumn) != 0)
				{
					int aVowel = (aChar < 'A' || 'Z' < aChar ||
						(aChar != 'A' && aChar != 'E' && aChar != 'I' && aChar != 'O' && aChar != 'U')) ? 0 : 1;
					if (aVowel == gLastVowel)
					{
						if (gRun < (aVowel == 0 ? 4 : 2) || 0x45 < randomRange(0, 100))
						{
							gLastVowel = aVowel;
							gRun++;
							return aChar;
						}
					}
					else
					{
						gRun = 0;
						gLastVowel = aVowel;
						gRun++;
						return aChar;
					}
				}
				i = 999;
			}
			i++;
		} while (i < 0x25);
	} while (aTries < 0x9c4);
	return 'E';
}

// 0x4054f0
void Dictionary::init()
{
	mMatch = "";
	mMatchRarity = 0;
	resetLetterOdds();
	Str aText;
	int aFile = fileOpen(dataDir(true) + lit(0x457b08), false, false);
	if (aFile != -1)
	{
		aText = fileReadText(aFile, 5000000);
		fileClose(aFile);
		int aLength = (int)aText.size();
		int aSize = aLength;
		int anEnd = 0;
		int aCounted = 0;
		int anIndex = 0;
		int aPrefixes = 0;
		int aRarities = 0;
		if (0 < aLength)
		{
			while (anEnd < aLength)
			{
				int aStart = anEnd;
				while (aStart < aSize && (unsigned char)aText[aStart] < 0x21)
				{
					aStart++;
					if (aLength <= aStart)
						goto done;
				}
				if (aLength <= aStart)
					break;
				int aNext = aStart + 1;
				for (anEnd = aNext; anEnd < aLength && (aSize <= anEnd || 0x20 < (unsigned char)aText[anEnd]); anEnd++)
				{
				}
				if (aCounted == 0)
				{
					long aCount = strToInt(substr(aText, aStart, anEnd));
					mCount = aCount;
					mWords = new Str[aCount];
					mSpellings = new Str[mCount];
					mRarities = new uint8_t[mCount];
					int i = 0;
					if (0 < mCount)
					{
						do
						{
							mRarities[i] = 1;
							i++;
						} while (i < mCount);
					}
					aCounted = 1;
				}
				else if (aStart < aSize && aText[aStart] == '!')
				{
					// A line "!X weight" sets a letter's weight ("!XY" a pair's, which the table has no place for)
					char aFirst = (char)at(aText, aNext);
					char aSecond = (char)at(aText, aStart + 2);
					int aLetter = aFirst - 0x41;
					if (aSecond != '_')
					{
						Str aPair = "";
						aPair += aFirst;
						aPair += aSecond;
						if (equalsNoCase(aPair, lit(0x457adc)))
							aLetter = 0x5a;
						else if (equalsNoCase(aPair, lit(0x457ad8)))
							aLetter = 0x5b;
						else if (equalsNoCase(aPair, lit(0x457ad4)))
							aLetter = 0x5c;
						else if (equalsNoCase(aPair, lit(0x457ad0)))
							aLetter = 0x5d;
						else if (equalsNoCase(aPair, lit(0x457acc)))
							aLetter = 0x5e;
						else if (equalsNoCase(aPair, lit(0x457ac8)))
							aLetter = 0x5f;
						else if (equalsNoCase(aPair, lit(0x457ac4)))
							aLetter = 0x60;
						else
							aLetter = equalsNoCase(aPair, lit(0x457ac0)) ? 0x61 : -1;
					}
					Str aValue = substr(aText, aStart + 3, anEnd);
					while (!aValue.empty() && 0 < (int)aValue.size() && aValue[0] == '_')
						aValue = substr(aValue, 1, (int)aValue.size());
					float aWeight = strToFloat(aValue);
					if (-1 < aLetter && aLetter < 0x23)
						gLetterOdds[aLetter].mCount = (int)aWeight;
				}
				else
				{
					if (anIndex == 0)
					{
						char aFlag = (char)at(aText, aStart);
						char aFlag2 = (char)at(aText, aNext);
						if (aFlag == '2')
						{
							aRarities = 1;
							if (aFlag2 == '0')
								aPrefixes = 1;
							goto rarity;
						}
						if (aFlag == '0')
							aPrefixes = 1;
					}
					if (aRarities != 0)
					{
					rarity:
						if (aStart < aSize && '/' < aText[aStart] && aText[aStart] < ':')
						{
							mRarities[anIndex] = (uint8_t)(aText[aStart] - 0x30);
							aStart = aNext;
						}
					}
					char aDigit;
					if (aPrefixes == 0 || aSize <= aStart || (aDigit = aText[aStart], aDigit < '0' || '9' < aDigit))
						mSpellings[anIndex] = "";
					else
					{
						aStart++;
						if (aDigit - 0x30 < 1)
							mSpellings[anIndex] = "";
						else
							mSpellings[anIndex] = substr(mSpellings[anIndex - 1], 0, aDigit - 0x30);
					}
					mSpellings[anIndex] += substr(aText, aStart, anEnd);
					// The original copies it into a 108-byte buffer first
					std::string aCopy = mSpellings[anIndex];
					mWords[anIndex] = normalizeWord(&aCopy[0]);
					anIndex++;
				}
				if (aLength <= anEnd)
					break;
			}
		}
	done:
		resetLetterOdds();
		mStarts[0] = 0;
		char aChar = 'A';
		int i = 0;
		if (0 < mCount)
		{
			do
			{
				if ((char)at(mWords[i], 0) != aChar)
				{
					aChar = (char)at(mWords[i], 0);
					int aLetter = aChar - 0x41;
					if (-1 < aLetter && aLetter < 0x1a)
						mStarts[aLetter] = i;
				}
				i++;
			} while (i < mCount);
		}
	}
}

} // namespace bonnies
