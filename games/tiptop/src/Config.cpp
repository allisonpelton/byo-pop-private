// The configuration (0x412ca6-0x418091)

#include "Config.h"
#include "Game.h"
#include "GameText.h"
#include "Gui.h"
#include "Res.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include <SDL.h>

namespace tiptop
{

Config gConfig;
HighScore gHighScores[4][5];
Level* gLevels[4];
uint32_t gLevelCount[4];

namespace
{

// 0x412ca6: reads list theList (after theList + 1 '@' marks) of levels into a new array; returns the number of
// levels
int ReadLevels(Level** theLevels, const char* theText, int theList)
{
	int aCount;
	const char* aPos = theText;
	for (aCount = 0; aCount <= theList; aCount++)
	{
		if (SkipPast(&aPos, '@') == 2)
			lowos::MessageBox_(GTC(0x0046c478), GTC(0x0046c498));
	}
	aCount = 0;
	while (SkipToToken(&aPos, 0) == 0)
	{
		ReadToken(&aPos);
		if (TokenChar(0) == 0 || TokenChar(0) == '@')
			break;
		aCount++;
	}
	int aFields = theList == 2 ? 0x1c : 0x19;
	if (aCount % aFields != 0)
		lowos::MessageBox_(GTC(0x0046c450), GTC(0x0046c498));
	aCount = aCount / aFields;
	*theLevels = (Level*)malloc(aCount * sizeof(Level));
	int aLevels = aCount;
	aPos = theText;
	for (aCount = 0; aCount <= theList; aCount++)
		SkipPast(&aPos, '@');
	aCount = 0;
	Level* l = *theLevels;
	// a field: '#' repeats the level before's
	auto anInt = [&](int32_t Level::*theField)
	{
		SkipToToken(&aPos, 0);
		ReadToken(&aPos);
		if (TokenChar(0) == '#')
			l[aCount].*theField = l[aCount - 1].*theField;
		else
			l[aCount].*theField = TokenInt();
	};
	auto aFloat = [&](float Level::*theField)
	{
		SkipToToken(&aPos, 0);
		ReadToken(&aPos);
		if (TokenChar(0) == '#')
			l[aCount].*theField = l[aCount - 1].*theField;
		else
			l[aCount].*theField = (float)TokenFloat();
	};
	while (true)
	{
		if (SkipToToken(&aPos, 0) != 0)
			return aLevels;
		ReadToken(&aPos);
		if (TokenChar(0) == 0)
			break;
		if (TokenChar(0) == '@')
			return aLevels;
		// the first field's token is already read
		if (TokenChar(0) == '#')
			l[aCount].m18[0] = l[aCount - 1].m18[0];
		else
			l[aCount].m18[0] = TokenInt();
		for (int i = 1; i < 16; i++)
		{
			SkipToToken(&aPos, 0);
			ReadToken(&aPos);
			if (TokenChar(0) == '#')
				l[aCount].m18[i] = l[aCount - 1].m18[i];
			else
				l[aCount].m18[i] = TokenInt();
		}
		anInt(&Level::m00);
		anInt(&Level::m04);
		anInt(&Level::m08);
		anInt(&Level::m0c);
		aFloat(&Level::m10);
		aFloat(&Level::m14);
		anInt(&Level::m60);
		anInt(&Level::m5c);
		anInt(&Level::m58);
		if (theList == 2)
		{
			anInt(&Level::m64);
			aFloat(&Level::m68);
			anInt(&Level::m6c);
		}
		aCount++;
	}
	return aLevels;
}

// 0x4177d5: the offset just past theKey's token in theText; a missing key ends the game with the key as message
int FindKey(const char* theText, const char* theKey)
{
	const char* aPos = theText;
	do
	{
		if (SkipToToken(&aPos, 0) != 0)
		{
			lowos::ScreenScope(2);
			lowos::MessageBox_(theKey, GTC(0x0046c318));
			exit(0);
		}
		ReadToken(&aPos);
	} while (SDL_strcasecmp(Token(), theKey) != 0);
	return (int)(aPos - theText);
}

// The text between the '=' after theKey and the next ';'
std::string ValueOf(const std::string& theText, const char* theKey)
{
	size_t aPos = (size_t)FindKey(theText.c_str(), theKey);
	aPos = theText.find("=", aPos) + 1;
	size_t anEnd = theText.find(";", aPos);
	return theText.substr(aPos, anEnd - aPos);
}

// 0x417835: YES or TRUE, NO or FALSE (anywhere in the value); negative if neither
int ReadBool(const std::string& theText, const char* theKey, char* theValue)
{
	std::string aValue = ValueOf(theText, theKey);
	if (aValue.find("YES", 0) != std::string::npos || aValue.find("TRUE", 0) != std::string::npos)
	{
		*theValue = 1;
		return 0;
	}
	if (aValue.find("NO", 0) == std::string::npos && aValue.find("FALSE", 0) == std::string::npos)
		return (int)0x80004005;
	*theValue = 0;
	return 0;
}

// 0x417947: theCount numbers after the key
int ReadInts(const std::string& theText, const char* theKey, int32_t* theValues, int theCount)
{
	int anOffset = FindKey(theText.c_str(), theKey);
	const char* aPos = theText.c_str() + anOffset;
	ReadToken(&aPos);
	for (; theCount != 0; theCount--)
	{
		SkipToToken(&aPos, 0);
		ReadToken(&aPos);
		*theValues = TokenInt();
		theValues++;
	}
	return 0;
}

// 0x4179b7: the numbers after the key, up to the first token that does not start with a digit; theValues 0 only
// counts them
int ReadList(const std::string& theText, const char* theKey, int32_t* theValues, uint32_t* theCount)
{
	int anOffset = FindKey(theText.c_str(), theKey);
	const char* aPos = theText.c_str() + anOffset;
	ReadToken(&aPos);
	*theCount = 0;
	while (true)
	{
		if (SkipToToken(&aPos, 0) != 0)
			return 0;
		ReadToken(&aPos);
		if ((char)TokenChar(0) < '0')
			break;
		if ('9' < (char)TokenChar(0))
			return 0;
		if (theValues != nullptr)
		{
			*theValues = TokenInt();
			theValues++;
		}
		*theCount = *theCount + 1;
	}
	return 0;
}

// 0x417a59: three numbers and an integer, on the key's line
int ReadMotion(const std::string& theText, const char* theKey, Motion* theMotion)
{
	int anOffset = FindKey(theText.c_str(), theKey);
	const char* aPos = theText.c_str() + anOffset;
	ReadToken(&aPos);
	SkipToToken(&aPos, 1);
	ReadToken(&aPos);
	theMotion->mParam[0] = (float)TokenFloat();
	SkipToToken(&aPos, 1);
	ReadToken(&aPos);
	theMotion->mParam[1] = (float)TokenFloat();
	SkipToToken(&aPos, 1);
	ReadToken(&aPos);
	theMotion->mParam[2] = (float)TokenFloat();
	SkipToToken(&aPos, 1);
	ReadToken(&aPos);
	theMotion->mTime = (int32_t)TokenInt();
	return 0;
}

// 0x417b17
int ReadLong(const std::string& theText, const char* theKey, int32_t* theValue)
{
	std::string aValue = ValueOf(theText, theKey);
	*theValue = (int32_t)atol(aValue.c_str());
	return 0;
}

// 0x417bae: negative (and theValue unchanged) for 1000 or more
int ReadFloat(const std::string& theText, const char* theKey, float* theValue)
{
	std::string aValue = ValueOf(theText, theKey);
	float aFloat = (float)atof(aValue.c_str());
	if (1000.0f <= aFloat)
		return (int)0x80004005;
	*theValue = aFloat;
	return 0;
}

// 0x417c6d
int ReadName(const std::string& theText, const char* theKey, char* theName)
{
	std::string aValue = ValueOf(theText, theKey);
	strcpy(theName, aValue.c_str());
	return 0;
}

// 0x417cfd: the size of the quoted string on the key's line, with its NUL
int StringLength(const std::string& theText, const char* theKey)
{
	int anOffset = FindKey(theText.c_str(), theKey);
	int aLength = 0;
	const char* aPos = theText.c_str() + anOffset;
	ReadToken(&aPos);
	SkipToToken(&aPos, 1);
	for (; *aPos != '"'; aPos++)
		aLength++;
	return aLength + 1;
}

// 0x417d63
int ReadString(const std::string& theText, const char* theKey, char* theString)
{
	int anOffset = FindKey(theText.c_str(), theKey);
	const char* aPos = theText.c_str() + anOffset;
	ReadToken(&aPos);
	SkipToToken(&aPos, 1);
	for (; *aPos != '"'; aPos++)
	{
		*theString = *aPos;
		theString++;
	}
	*theString = 0;
	return 0;
}

// 0x417dd2: theCount records of 13 numbers
void ReadParticles(const std::string& theText, const char* theKey, Particle* theParticles, uint32_t theCount)
{
	int anOffset = FindKey(theText.c_str(), theKey);
	const char* aPos = theText.c_str() + anOffset;
	ReadToken(&aPos);
	SkipToToken(&aPos, 0);
	auto aNext = [&]()
	{
		ReadToken(&aPos);
		SkipToToken(&aPos, 0);
	};
	for (uint32_t i = 0; i < theCount; i++)
	{
		Particle* p = &theParticles[i];
		aNext();
		p->mValue0 = TokenInt();
		aNext();
		p->mValue1 = (float)TokenFloat();
		aNext();
		p->mValue2 = (float)TokenFloat();
		aNext();
		p->mColor1 = (uint32_t)TokenInt() << 16;
		aNext();
		p->mColor1 = p->mColor1 | (uint32_t)TokenInt() << 8;
		aNext();
		p->mColor1 = p->mColor1 | (uint32_t)TokenInt();
		aNext();
		p->mColor2 = (uint32_t)TokenInt() << 16;
		aNext();
		p->mColor2 = p->mColor2 | (uint32_t)TokenInt() << 8;
		aNext();
		p->mColor2 = p->mColor2 | (uint32_t)TokenInt();
		aNext();
		p->mValue5 = TokenInt();
		aNext();
		p->mValue6 = TokenInt();
		aNext();
		p->mValue7 = (char)TokenInt();
		aNext();
		p->mValue8 = (char)TokenInt();
	}
}

// 0x418091: two numbers, then theCount records of 10
void ReadEffects(const std::string& theText, const char* theKey, EffectList* theList, Effect* theEffects,
	uint32_t theCount)
{
	int anOffset = FindKey(theText.c_str(), theKey);
	const char* aPos = theText.c_str() + anOffset;
	ReadToken(&aPos);
	SkipToToken(&aPos, 0);
	auto aNext = [&]()
	{
		ReadToken(&aPos);
		SkipToToken(&aPos, 0);
	};
	aNext();
	theList->mFlag = (char)TokenInt();
	aNext();
	theList->mValue = TokenInt();
	theList->mCount = theCount;
	for (uint32_t i = 0; i < theCount; i++)
	{
		Effect* e = &theEffects[i];
		aNext();
		e->mValue0 = TokenInt();
		aNext();
		e->mValue1 = TokenInt();
		aNext();
		e->mValue2 = TokenInt();
		aNext();
		e->mValue3 = TokenInt();
		aNext();
		e->mValue4 = (char)TokenInt();
		aNext();
		e->mValue5 = (char)TokenInt();
		aNext();
		e->mValue6 = TokenInt();
		aNext();
		e->mValue7 = TokenInt();
		aNext();
		e->mValue8 = TokenInt();
		aNext();
		e->mValue9 = TokenInt();
	}
}

// The original repeats the allocation and its failure message for every list
void* Allocate(size_t theSize)
{
	void* aBlock = malloc(theSize);
	if (aBlock == nullptr)
		NoMemory();
	return aBlock;
}

// A list of numbers whose length the file gives (0x4179b7 twice)
int32_t* AllocateList(const std::string& theText, const char* theKey, int32_t** theList, uint32_t* theCount)
{
	ReadList(theText, theKey, nullptr, theCount);
	if (*theCount != 0)
	{
		*theList = (int32_t*)Allocate(*theCount << 2);
		ReadList(theText, theKey, *theList, theCount);
	}
	return *theList;
}

} // namespace

// 0x413830
int LoadConfig(Config* theConfig)
{
	if (res::Load(res::GROUP_TEXTS, 1, 0x11) != 0)
	{
		lowos::ScreenScope(2);
		lowos::MessageBox_(GTC(0x0046ebdc), GTC(0x0046c318));
		exit(0);
	}
	res::Get(res::GROUP_TEXTS, 0x11);
	std::string aText;
	int aSize = res::SizeOf(res::GROUP_TEXTS, 0x11);
	aText.resize(aSize + 1);
	memcpy(&aText[0], res::Get(res::GROUP_TEXTS, 0x11), aSize + 1);
	res::Free(res::GROUP_TEXTS, 1, 0x11);
	if (ReadLong(aText, GTC(0x0046ebd0), &theConfig->mStartLevel) < 0)
		theConfig->mStartLevel = 1;
	if (ReadLong(aText, GTC(0x0046ebc4), &theConfig->mNumColumns) < 0)
		theConfig->mNumColumns = 10;
	if (ReadLong(aText, GTC(0x0046ebb8), &theConfig->mNumRows) < 0)
		theConfig->mNumRows = 0x14;
	if (ReadBool(aText, GTC(0x0046eba8), &theConfig->mLockFloaters) < 0)
		theConfig->mLockFloaters = 0;
	if (ReadBool(aText, GTC(0x0046eb98), &theConfig->mLockFallers) < 0)
		theConfig->mLockFallers = 0;
	if (ReadLong(aText, GTC(0x0046eb84), &theConfig->mDequeueFloatDelay) < 0)
		theConfig->mDequeueFloatDelay = 100;
	ReadMotion(aText, GTC(0x0046eb6c), &theConfig->mPrimaryfallerMotion);
	ReadMotion(aText, GTC(0x0046eb54), &theConfig->mSecondaryfallerMotion);
	ReadMotion(aText, GTC(0x0046eb44), &theConfig->mFloaterMotion);
	ReadMotion(aText, GTC(0x0046eb34), &theConfig->mRocketMotion);
	ReadMotion(aText, GTC(0x0046eb28), &theConfig->mExfaMotion);
	ReadMotion(aText, GTC(0x0046eb1c), &theConfig->mExflMotion);
	ReadMotion(aText, GTC(0x0046eb0c), &theConfig->mHammerMotion);
	ReadMotion(aText, GTC(0x0046eafc), &theConfig->mMagnetMotion);
	ReadMotion(aText, GTC(0x0046eaec), &theConfig->mRainbowMotion);
	ReadMotion(aText, GTC(0x0046ead8), &theConfig->mColorbombMotion);
	ReadMotion(aText, GTC(0x0046eac4), &theConfig->mColumnnukeMotion);
	if (ReadLong(aText, GTC(0x0046eab8), &theConfig->mTapSpeed) < 0)
		theConfig->mTapSpeed = 5;
	if (ReadLong(aText, GTC(0x0046eaac), &theConfig->mTapDelay) < 0)
		theConfig->mTapDelay = 0xf;
	if (ReadLong(aText, GTC(0x0046ea94), &theConfig->mSecondaryfallInitdelay) < 0)
		theConfig->mSecondaryfallInitdelay = 0x32;
	if (ReadLong(aText, GTC(0x0046ea7c), &theConfig->mSecondaryfallAdddelay) < 0)
		theConfig->mSecondaryfallAdddelay = 0x14;
	if (ReadLong(aText, GTC(0x0046ea64), &theConfig->mSecondaryfallRowdelay) < 0)
		theConfig->mSecondaryfallRowdelay = 0x14;
	if (ReadLong(aText, GTC(0x0046eaac), &theConfig->mTapDelay) < 0)
		theConfig->mTapDelay = 0xf;
	if (ReadLong(aText, GTC(0x0046ea54), &theConfig->mRemoveDelay) < 0)
		theConfig->mRemoveDelay = 100;
	ReadLong(aText, GTC(0x0046ea3c), &theConfig->mScoreEmptyrowsInitial);
	ReadLong(aText, GTC(0x0046ea20), &theConfig->mScoreEmptyrowsCoefficient);
	if (ReadBool(aText, GTC(0x0046ea0c), &theConfig->mWinningPrecedence) < 0)
		theConfig->mWinningPrecedence = 0;
	if (ReadLong(aText, GTC(0x0046e9fc), &theConfig->mExploderRadius) < 0)
		theConfig->mExploderRadius = 0x1e;
	if (ReadBool(aText, GTC(0x0046e9ec), &theConfig->mRandomBlocks) < 0)
		theConfig->mRandomBlocks = 1;
	if (ReadBool(aText, GTC(0x0046e9dc), &theConfig->mFloatersMatch) < 0)
		theConfig->mFloatersMatch = 0;
	if (ReadLong(aText, GTC(0x0046e9cc), &theConfig->mFallerTapRows) < 0)
		theConfig->mFallerTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e9b8), &theConfig->mFloaterTapRows) < 0)
		theConfig->mFloaterTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e9a8), &theConfig->mRocketTapRows) < 0)
		theConfig->mRocketTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e998), &theConfig->mHammerTapRows) < 0)
		theConfig->mHammerTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e984), &theConfig->mExploderTapRows) < 0)
		theConfig->mExploderTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e974), &theConfig->mMagnetTapRows) < 0)
		theConfig->mMagnetTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e960), &theConfig->mRainbowTapRows) < 0)
		theConfig->mRainbowTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e94c), &theConfig->mColorbombTapRows) < 0)
		theConfig->mColorbombTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e938), &theConfig->mColumnnukeTapRows) < 0)
		theConfig->mColumnnukeTapRows = 1;
	if (ReadLong(aText, GTC(0x0046e92c), &theConfig->mGroupSize) < 0)
		theConfig->mGroupSize = 1;
	if (ReadBool(aText, GTC(0x0046e918), &theConfig->mPrimaryStopCenter) < 0)
		theConfig->mPrimaryStopCenter = 1;
	if (ReadBool(aText, GTC(0x0046e900), &theConfig->mSecondaryStopCenter) < 0)
		theConfig->mSecondaryStopCenter = 1;
	if (ReadLong(aText, GTC(0x0046e8ec), &theConfig->mSecondaryDistance) < 0)
		theConfig->mSecondaryDistance = 1;
	if (ReadLong(aText, GTC(0x0046e8dc), &theConfig->mSecondaryCarry) < 0)
		theConfig->mSecondaryCarry = 1;
	if (ReadLong(aText, GTC(0x0046e8c8), &theConfig->mMaxMatchOrdinal) < 0)
		theConfig->mMaxMatchOrdinal = 1;
	if (ReadBool(aText, GTC(0x0046e8a8), &theConfig->mFloaterMatchesCreateFloats) < 0)
		theConfig->mFloaterMatchesCreateFloats = 1;
	if (ReadBool(aText, GTC(0x0046e888), &theConfig->mFloaterMatchesAwardPoints) < 0)
		theConfig->mFloaterMatchesAwardPoints = 1;
	if (ReadBool(aText, GTC(0x0046e870), &theConfig->mFallingExploderFall) < 0)
		theConfig->mFallingExploderFall = 1;
	if (ReadBool(aText, GTC(0x0046e858), &theConfig->mFloatingExploderFall) < 0)
		theConfig->mFloatingExploderFall = 1;
	if (ReadBool(aText, GTC(0x0046e848), &theConfig->mColorbombFall) < 0)
		theConfig->mColorbombFall = 1;
	if (ReadBool(aText, GTC(0x0046e828), &theConfig->mFallingExploderCreateFloats) < 0)
		theConfig->mFallingExploderCreateFloats = 1;
	if (ReadBool(aText, GTC(0x0046e808), &theConfig->mFloatingExploderCreateFloats) < 0)
		theConfig->mFloatingExploderCreateFloats = 1;
	if (ReadBool(aText, GTC(0x0046e7f0), &theConfig->mColorbombCreateFloats) < 0)
		theConfig->mColorbombCreateFloats = 1;
	if (ReadBool(aText, GTC(0x0046e7d4), &theConfig->mColumnnukeCreateFloats) < 0)
		theConfig->mColumnnukeCreateFloats = 1;
	if (ReadBool(aText, GTC(0x0046e7c4), &theConfig->mExploderPoints) < 0)
		theConfig->mExploderPoints = 1;
	if (ReadBool(aText, GTC(0x0046e7b0), &theConfig->mColumnnukePoints) < 0)
		theConfig->mColumnnukePoints = 1;
	if (ReadBool(aText, GTC(0x0046e79c), &theConfig->mColorbombPoints) < 0)
		theConfig->mColorbombPoints = 1;
	if (ReadLong(aText, GTC(0x0046e78c), &theConfig->mGamewindowX) < 0)
		theConfig->mGamewindowX = 1;
	if (ReadLong(aText, GTC(0x0046e77c), &theConfig->mGamewindowY) < 0)
		theConfig->mGamewindowY = 1;
	if (ReadLong(aText, GTC(0x0046e770), &theConfig->mBlockWidth) < 0)
		theConfig->mBlockWidth = 1;
	if (ReadLong(aText, GTC(0x0046e760), &theConfig->mBlockHeight) < 0)
		theConfig->mBlockHeight = 1;
	if (ReadLong(aText, GTC(0x0046e750), &theConfig->mIngameOptionsx) < 0)
		theConfig->mIngameOptionsx = 1;
	if (ReadLong(aText, GTC(0x0046e740), &theConfig->mIngameOptionsy) < 0)
		theConfig->mIngameOptionsy = 1;
	if (ReadLong(aText, GTC(0x0046e730), &theConfig->mIngameEndgamex) < 0)
		theConfig->mIngameEndgamex = 1;
	if (ReadLong(aText, GTC(0x0046e720), &theConfig->mIngameEndgamey) < 0)
		theConfig->mIngameEndgamey = 1;
	if (ReadLong(aText, GTC(0x0046e70c), &theConfig->mIngameNextblocksx) < 0)
		theConfig->mIngameNextblocksx = 1;
	if (ReadLong(aText, GTC(0x0046e6f8), &theConfig->mIngameNextblocksy) < 0)
		theConfig->mIngameNextblocksy = 1;
	if (ReadLong(aText, GTC(0x0046e6e4), &theConfig->mOptionsContinuex) < 0)
		theConfig->mOptionsContinuex = 1;
	if (ReadLong(aText, GTC(0x0046e6d0), &theConfig->mOptionsContinuey) < 0)
		theConfig->mOptionsContinuey = 1;
	if (ReadLong(aText, GTC(0x0046e6c0), &theConfig->mOptionsTipsx) < 0)
		theConfig->mOptionsTipsx = 1;
	if (ReadLong(aText, GTC(0x0046e6b0), &theConfig->mOptionsTipsy) < 0)
		theConfig->mOptionsTipsy = 1;
	if (ReadLong(aText, GTC(0x0046e69c), &theConfig->mOptionsFullscreenx) < 0)
		theConfig->mOptionsFullscreenx = 1;
	if (ReadLong(aText, GTC(0x0046e688), &theConfig->mOptionsFullscreeny) < 0)
		theConfig->mOptionsFullscreeny = 1;
	if (ReadLong(aText, GTC(0x0046e678), &theConfig->mOptionsVolumex) < 0)
		theConfig->mOptionsVolumex = 1;
	if (ReadLong(aText, GTC(0x0046e664), &theConfig->mOptionsVolumewidth) < 0)
		theConfig->mOptionsVolumewidth = 1;
	if (ReadLong(aText, GTC(0x0046e654), &theConfig->mOptionsSoundy) < 0)
		theConfig->mOptionsSoundy = 1;
	if (ReadLong(aText, GTC(0x0046e644), &theConfig->mOptionsMusicy) < 0)
		theConfig->mOptionsMusicy = 1;
	ReadLong(aText, GTC(0x0046e63c), &theConfig->mBkghix);
	ReadLong(aText, GTC(0x0046e634), &theConfig->mBkghiy);
	ReadLong(aText, GTC(0x0046e624), &theConfig->mBkgcolyoffset);
	ReadLong(aText, GTC(0x0046e610), &theConfig->mBkgcolGlowradius);
	ReadLong(aText, GTC(0x0046e5f8), &theConfig->mBkgcolLengthNormal);
	ReadLong(aText, GTC(0x0046e5e4), &theConfig->mBkgcolLengthTimed);
	ReadLong(aText, GTC(0x0046e5d0), &theConfig->mBkgcolLengthSpeed);
	ReadLong(aText, GTC(0x0046e5b8), &theConfig->mBkgcolLengthStrategy);
	ReadLong(aText, GTC(0x0046e5a8), &theConfig->mBkgcolShrink);
	if (ReadLong(aText, GTC(0x0046e598), &theConfig->mIngameScorex) < 0)
		theConfig->mIngameScorex = 1;
	if (ReadLong(aText, GTC(0x0046e588), &theConfig->mIngameScorey) < 0)
		theConfig->mIngameScorey = 1;
	if (ReadLong(aText, GTC(0x0046e578), &theConfig->mIngameLevelx) < 0)
		theConfig->mIngameLevelx = 1;
	if (ReadLong(aText, GTC(0x0046e568), &theConfig->mIngameLevely) < 0)
		theConfig->mIngameLevely = 1;
	if (ReadLong(aText, GTC(0x0046e55c), &theConfig->mBlockgridX) < 0)
		theConfig->mBlockgridX = 1;
	if (ReadLong(aText, GTC(0x0046e550), &theConfig->mBlockgridY) < 0)
		theConfig->mBlockgridY = 1;
	if (ReadLong(aText, GTC(0x0046e53c), &theConfig->mTipsBackgroundx) < 0)
		theConfig->mTipsBackgroundx = 1;
	if (ReadLong(aText, GTC(0x0046e528), &theConfig->mTipsBackgroundy) < 0)
		theConfig->mTipsBackgroundy = 1;
	if (ReadLong(aText, GTC(0x0046e518), &theConfig->mTipsTogglex) < 0)
		theConfig->mTipsTogglex = 1;
	ReadLong(aText, GTC(0x0046e508), &theConfig->mTipsToggley);
	ReadLong(aText, GTC(0x0046e4fc), &theConfig->mTipsPrevx);
	ReadLong(aText, GTC(0x0046e4f0), &theConfig->mTipsPrevy);
	ReadLong(aText, GTC(0x0046e4e4), &theConfig->mTipsNextx);
	ReadLong(aText, GTC(0x0046e4d8), &theConfig->mTipsNexty);
	ReadLong(aText, GTC(0x0046e4cc), &theConfig->mTipsOkx);
	ReadLong(aText, GTC(0x0046e4c0), &theConfig->mTipsOky);
	ReadLong(aText, GTC(0x0046e4b4), &theConfig->mTipsTextx);
	ReadLong(aText, GTC(0x0046e4a8), &theConfig->mTipsTexty);
	ReadLong(aText, GTC(0x0046e498), &theConfig->mTipsTextwidth);
	ReadLong(aText, GTC(0x0046e488), &theConfig->mTipsTextheight);
	ReadLong(aText, GTC(0x0046e474), &theConfig->mHighscoresTextx);
	ReadLong(aText, GTC(0x0046e460), &theConfig->mHighscoresTexty);
	ReadLong(aText, GTC(0x0046e450), &theConfig->mHighscoresOkx);
	ReadLong(aText, GTC(0x0046e440), &theConfig->mHighscoresOky);
	ReadLong(aText, GTC(0x0046e42c), &theConfig->mInstructionsNextx);
	ReadLong(aText, GTC(0x0046e418), &theConfig->mInstructionsNexty);
	ReadLong(aText, GTC(0x0046e404), &theConfig->mInstructionsPrevx);
	ReadLong(aText, GTC(0x0046e3f0), &theConfig->mInstructionsPrevy);
	ReadLong(aText, GTC(0x0046e3dc), &theConfig->mInstructionsOkx);
	ReadLong(aText, GTC(0x0046e3c8), &theConfig->mInstructionsOky);
	ReadLong(aText, GTC(0x0046e3bc), &theConfig->mCreditsOkx);
	ReadLong(aText, GTC(0x0046e3b0), &theConfig->mCreditsOky);
	ReadLong(aText, GTC(0x0046e39c), &theConfig->mMainmenuHighscorex);
	ReadLong(aText, GTC(0x0046e388), &theConfig->mMainmenuHighscorey);
	ReadLong(aText, GTC(0x0046e374), &theConfig->mMainmenuNewgamex);
	ReadLong(aText, GTC(0x0046e360), &theConfig->mMainmenuNewgamey);
	ReadLong(aText, GTC(0x0046e34c), &theConfig->mMainmenuOptionsx);
	ReadLong(aText, GTC(0x0046e338), &theConfig->mMainmenuOptionsy);
	ReadLong(aText, GTC(0x0046e320), &theConfig->mMainmenuInstructionsx);
	ReadLong(aText, GTC(0x0046e308), &theConfig->mMainmenuInstructionsy);
	ReadLong(aText, GTC(0x0046e2f4), &theConfig->mMainmenuCreditsx);
	ReadLong(aText, GTC(0x0046e2e0), &theConfig->mMainmenuCreditsy);
	ReadLong(aText, GTC(0x0046e2d0), &theConfig->mMainmenuQuitx);
	ReadLong(aText, GTC(0x0046e2c0), &theConfig->mMainmenuQuity);
	ReadLong(aText, GTC(0x0046e2b0), &theConfig->mAreyousureX);
	ReadLong(aText, GTC(0x0046e2a0), &theConfig->mAreyousureY);
	ReadLong(aText, GTC(0x0046e290), &theConfig->mAreyousureOkx);
	ReadLong(aText, GTC(0x0046e280), &theConfig->mAreyousureOky);
	ReadLong(aText, GTC(0x0046e26c), &theConfig->mAreyousureCancelx);
	ReadLong(aText, GTC(0x0046e258), &theConfig->mAreyousureCancely);
	ReadLong(aText, GTC(0x0046e248), &theConfig->mAreyousureregX);
	ReadLong(aText, GTC(0x0046e238), &theConfig->mAreyousureregY);
	ReadLong(aText, GTC(0x0046e224), &theConfig->mAreyousureregOkx);
	ReadLong(aText, GTC(0x0046e210), &theConfig->mAreyousureregOky);
	ReadLong(aText, GTC(0x0046e1f8), &theConfig->mAreyousureregCancelx);
	ReadLong(aText, GTC(0x0046e1e0), &theConfig->mAreyousureregCancely);
	ReadLong(aText, GTC(0x0046e1c8), &theConfig->mAreyousureregRegisterx);
	ReadLong(aText, GTC(0x0046e1b0), &theConfig->mAreyousureregRegistery);
	ReadLong(aText, GTC(0x0046e19c), &theConfig->mAreyousureregsaveX);
	ReadLong(aText, GTC(0x0046e188), &theConfig->mAreyousureregsaveY);
	ReadLong(aText, GTC(0x0046e170), &theConfig->mAreyousureregsaveOkx);
	ReadLong(aText, GTC(0x0046e158), &theConfig->mAreyousureregsaveOky);
	ReadLong(aText, GTC(0x0046e13c), &theConfig->mAreyousureregsaveCancelx);
	ReadLong(aText, GTC(0x0046e120), &theConfig->mAreyousureregsaveCancely);
	ReadLong(aText, GTC(0x0046e104), &theConfig->mAreyousureregsaveRegisterx);
	ReadLong(aText, GTC(0x0046e0e8), &theConfig->mAreyousureregsaveRegistery);
	ReadLong(aText, GTC(0x0046e0dc), &theConfig->mSavegameX);
	ReadLong(aText, GTC(0x0046e0d0), &theConfig->mSavegameY);
	ReadLong(aText, GTC(0x0046e0c0), &theConfig->mSavegameOkx);
	ReadLong(aText, GTC(0x0046e0b0), &theConfig->mSavegameOky);
	ReadLong(aText, GTC(0x0046e09c), &theConfig->mSavegameCancelx);
	ReadLong(aText, GTC(0x0046e088), &theConfig->mSavegameCancely);
	ReadLong(aText, GTC(0x0046e078), &theConfig->mDeletegameX);
	ReadLong(aText, GTC(0x0046e068), &theConfig->mDeletegameY);
	ReadLong(aText, GTC(0x0046e058), &theConfig->mDeletegameOkx);
	ReadLong(aText, GTC(0x0046e048), &theConfig->mDeletegameOky);
	ReadLong(aText, GTC(0x0046e034), &theConfig->mDeletegameCancelx);
	ReadLong(aText, GTC(0x0046e020), &theConfig->mDeletegameCancely);
	ReadBool(aText, GTC(0x0046e00c), &theConfig->mSecondaryUnlimited);
	ReadLong(aText, GTC(0x0046dffc), &theConfig->mProgressbarX);
	ReadLong(aText, GTC(0x0046dfec), &theConfig->mProgressbarY);
	ReadLong(aText, GTC(0x0046dfd8), &theConfig->mProgressbarTopclip);
	ReadLong(aText, GTC(0x0046dfc0), &theConfig->mMainmenuHsRegulary);
	ReadLong(aText, GTC(0x0046dfac), &theConfig->mMainmenuHsTimedy);
	ReadLong(aText, GTC(0x0046df98), &theConfig->mMainmenuHsSpeedy);
	ReadLong(aText, GTC(0x0046df80), &theConfig->mMainmenuHsStrategyy);
	ReadLong(aText, GTC(0x0046df6c), &theConfig->mMainmenuHsNamex);
	ReadLong(aText, GTC(0x0046df58), &theConfig->mMainmenuHsScorex);
	ReadLong(aText, GTC(0x0046df48), &theConfig->mNewgameNormalx);
	ReadLong(aText, GTC(0x0046df38), &theConfig->mNewgameNormaly);
	ReadLong(aText, GTC(0x0046df28), &theConfig->mNewgameTimedx);
	ReadLong(aText, GTC(0x0046df18), &theConfig->mNewgameTimedy);
	ReadLong(aText, GTC(0x0046df08), &theConfig->mNewgameSpeedx);
	ReadLong(aText, GTC(0x0046def8), &theConfig->mNewgameSpeedy);
	ReadLong(aText, GTC(0x0046dee4), &theConfig->mNewgameStrategyx);
	ReadLong(aText, GTC(0x0046ded0), &theConfig->mNewgameStrategyy);
	ReadLong(aText, GTC(0x0046dec0), &theConfig->mNewgameMainx);
	ReadLong(aText, GTC(0x0046deb0), &theConfig->mNewgameMainy);
	ReadLong(aText, GTC(0x0046de9c), &theConfig->mNewgameContinuex);
	ReadLong(aText, GTC(0x0046de88), &theConfig->mNewgameContinuey);
	ReadLong(aText, GTC(0x0046de74), &theConfig->mNewgameSummaryboxx);
	ReadLong(aText, GTC(0x0046de60), &theConfig->mNewgameSummaryboxy);
	ReadLong(aText, GTC(0x0046de48), &theConfig->mNewgameSummarydatex);
	ReadLong(aText, GTC(0x0046de30), &theConfig->mNewgameSummarydatey);
	ReadLong(aText, GTC(0x0046de18), &theConfig->mNewgameSummarytypex);
	ReadLong(aText, GTC(0x0046de00), &theConfig->mNewgameSummarytypey);
	ReadLong(aText, GTC(0x0046dde8), &theConfig->mNewgameSummarylevelx);
	ReadLong(aText, GTC(0x0046ddd0), &theConfig->mNewgameSummarylevely);
	ReadLong(aText, GTC(0x0046ddb8), &theConfig->mNewgameSummarycombox);
	ReadLong(aText, GTC(0x0046dda0), &theConfig->mNewgameSummarycomboy);
	ReadLong(aText, GTC(0x0046dd88), &theConfig->mNewgameSummaryscorex);
	ReadLong(aText, GTC(0x0046dd70), &theConfig->mNewgameSummaryscorey);
	ReadLong(aText, GTC(0x0046dd50), &theConfig->mHighscoresmenuClearhighscoresx);
	ReadLong(aText, GTC(0x0046dd30), &theConfig->mHighscoresmenuClearhighscoresy);
	ReadLong(aText, GTC(0x0046dd18), &theConfig->mHighscoresmenuMainx);
	ReadLong(aText, GTC(0x0046dd00), &theConfig->mHighscoresmenuMainy);
	ReadLong(aText, GTC(0x0046dce8), &theConfig->mHighscoresmenuNormaly);
	ReadLong(aText, GTC(0x0046dcd0), &theConfig->mHighscoresmenuTimedy);
	ReadLong(aText, GTC(0x0046dcb8), &theConfig->mHighscoresmenuSpeedy);
	ReadLong(aText, GTC(0x0046dc9c), &theConfig->mHighscoresmenuStrategyy);
	ReadLong(aText, GTC(0x0046dc84), &theConfig->mHighscoresmenuSkipy);
	ReadLong(aText, GTC(0x0046dc6c), &theConfig->mHighscoresmenuNamex);
	ReadLong(aText, GTC(0x0046dc54), &theConfig->mHighscoresmenuScorex);
	ReadLong(aText, GTC(0x0046dc48), &theConfig->mPlayagainX);
	ReadLong(aText, GTC(0x0046dc3c), &theConfig->mPlayagainY);
	ReadLong(aText, GTC(0x0046dc2c), &theConfig->mPlayagainNox);
	ReadLong(aText, GTC(0x0046dc1c), &theConfig->mPlayagainNoy);
	ReadLong(aText, GTC(0x0046dc0c), &theConfig->mPlayagainYesx);
	ReadLong(aText, GTC(0x0046dbfc), &theConfig->mPlayagainYesy);
	ReadFloat(aText, GTC(0x0046dbe8), &theConfig->mNormalRocketRamp);
	ReadFloat(aText, GTC(0x0046dbd4), &theConfig->mNormalExfaRamp);
	ReadFloat(aText, GTC(0x0046dbc0), &theConfig->mNormalExflRamp);
	ReadFloat(aText, GTC(0x0046dbac), &theConfig->mNormalHammerRamp);
	ReadFloat(aText, GTC(0x0046db98), &theConfig->mNormalMagnetRamp);
	ReadFloat(aText, GTC(0x0046db84), &theConfig->mNormalRainbowRamp);
	ReadFloat(aText, GTC(0x0046db6c), &theConfig->mNormalColorbombRamp);
	ReadFloat(aText, GTC(0x0046db54), &theConfig->mNormalColumnnukeRamp);
	ReadFloat(aText, GTC(0x0046db40), &theConfig->mNormalRndfltRamp);
	ReadLong(aText, GTC(0x0046db2c), &theConfig->mNormalRndfltMin);
	ReadFloat(aText, GTC(0x0046db18), &theConfig->mTimedRocketRamp);
	ReadFloat(aText, GTC(0x0046db08), &theConfig->mTimedExfaRamp);
	ReadFloat(aText, GTC(0x0046daf8), &theConfig->mTimedExflRamp);
	ReadFloat(aText, GTC(0x0046dae4), &theConfig->mTimedHammerRamp);
	ReadFloat(aText, GTC(0x0046dad0), &theConfig->mTimedMagnetRamp);
	ReadFloat(aText, GTC(0x0046dabc), &theConfig->mTimedRainbowRamp);
	ReadFloat(aText, GTC(0x0046daa4), &theConfig->mTimedColorbombRamp);
	ReadFloat(aText, GTC(0x0046da8c), &theConfig->mTimedColumnnukeRamp);
	ReadFloat(aText, GTC(0x0046da78), &theConfig->mTimedRndfltRamp);
	ReadLong(aText, GTC(0x0046da64), &theConfig->mTimedRndfltMin);
	ReadFloat(aText, GTC(0x0046da50), &theConfig->mTimedPlpsecRamp);
	ReadFloat(aText, GTC(0x0046da3c), &theConfig->mSpeedRocketRamp);
	ReadFloat(aText, GTC(0x0046da2c), &theConfig->mSpeedExfaRamp);
	ReadFloat(aText, GTC(0x0046da1c), &theConfig->mSpeedExflRamp);
	ReadFloat(aText, GTC(0x0046da08), &theConfig->mSpeedHammerRamp);
	ReadFloat(aText, GTC(0x0046d9f4), &theConfig->mSpeedMagnetRamp);
	ReadFloat(aText, GTC(0x0046d9e0), &theConfig->mSpeedRainbowRamp);
	ReadFloat(aText, GTC(0x0046d9c8), &theConfig->mSpeedColorbombRamp);
	ReadFloat(aText, GTC(0x0046d9b0), &theConfig->mSpeedColumnnukeRamp);
	ReadFloat(aText, GTC(0x0046d99c), &theConfig->mSpeedRndfltRamp);
	ReadLong(aText, GTC(0x0046d988), &theConfig->mSpeedRndfltMin);
	ReadFloat(aText, GTC(0x0046d970), &theConfig->mStrategyRocketRamp);
	ReadFloat(aText, GTC(0x0046d95c), &theConfig->mStrategyExfaRamp);
	ReadFloat(aText, GTC(0x0046d948), &theConfig->mStrategyExflRamp);
	ReadFloat(aText, GTC(0x0046d930), &theConfig->mStrategyHammerRamp);
	ReadFloat(aText, GTC(0x0046d918), &theConfig->mStrategyMagnetRamp);
	ReadFloat(aText, GTC(0x0046d900), &theConfig->mStrategyRainbowRamp);
	ReadFloat(aText, GTC(0x0046d8e8), &theConfig->mStrategyColorbombRamp);
	ReadFloat(aText, GTC(0x0046d8cc), &theConfig->mStrategyColumnnukeRamp);
	ReadFloat(aText, GTC(0x0046d8b4), &theConfig->mStrategyRndfltRamp);
	ReadLong(aText, GTC(0x0046d8a0), &theConfig->mStrategyRndfltMin);
	ReadLong(aText, GTC(0x0046d894), &theConfig->mTopcoverX);
	ReadLong(aText, GTC(0x0046d888), &theConfig->mTopcoverY);
	ReadLong(aText, GTC(0x0046d878), &theConfig->mBottomcoverX);
	ReadLong(aText, GTC(0x0046d868), &theConfig->mBottomcoverY);
	ReadLong(aText, GTC(0x0046d858), &theConfig->mWarningshake0);
	ReadLong(aText, GTC(0x0046d848), &theConfig->mWarningshake1);
	ReadLong(aText, GTC(0x0046d838), &theConfig->mWarningshake2);
	ReadLong(aText, GTC(0x0046d828), &theConfig->mWarningshake3);
	ReadLong(aText, GTC(0x0046d818), &theConfig->mWarningshake4);
	ReadLong(aText, GTC(0x0046d808), &theConfig->mRemovingshake);
	ReadLong(aText, GTC(0x0046d7f0), &theConfig->mIngameInstructionsx);
	ReadLong(aText, GTC(0x0046d7d8), &theConfig->mIngameInstructionsy);
	ReadLong(aText, GTC(0x0046d7c4), &theConfig->mMainmenuHsLevelx);
	ReadLong(aText, GTC(0x0046d7ac), &theConfig->mHighscoresmenuLevelx);
	ReadName(aText, GTC(0x0046d794), gHighScores[0][0].mName);
	ReadName(aText, GTC(0x0046d77c), gHighScores[0][1].mName);
	ReadName(aText, GTC(0x0046d764), gHighScores[0][2].mName);
	ReadName(aText, GTC(0x0046d74c), gHighScores[0][3].mName);
	ReadName(aText, GTC(0x0046d734), gHighScores[0][4].mName);
	ReadName(aText, GTC(0x0046d720), gHighScores[1][0].mName);
	ReadName(aText, GTC(0x0046d70c), gHighScores[1][1].mName);
	ReadName(aText, GTC(0x0046d6f8), gHighScores[1][2].mName);
	ReadName(aText, GTC(0x0046d6e4), gHighScores[1][3].mName);
	ReadName(aText, GTC(0x0046d6d0), gHighScores[1][4].mName);
	ReadName(aText, GTC(0x0046d6bc), gHighScores[2][0].mName);
	ReadName(aText, GTC(0x0046d6a8), gHighScores[2][1].mName);
	ReadName(aText, GTC(0x0046d694), gHighScores[2][2].mName);
	ReadName(aText, GTC(0x0046d680), gHighScores[2][3].mName);
	ReadName(aText, GTC(0x0046d66c), gHighScores[2][4].mName);
	ReadName(aText, GTC(0x0046d654), gHighScores[3][0].mName);
	ReadName(aText, GTC(0x0046d63c), gHighScores[3][1].mName);
	ReadName(aText, GTC(0x0046d624), gHighScores[3][2].mName);
	ReadName(aText, GTC(0x0046d60c), gHighScores[3][3].mName);
	ReadName(aText, GTC(0x0046d5f4), gHighScores[3][4].mName);
	ReadLong(aText, GTC(0x0046d5dc), &gHighScores[0][0].mLevel);
	ReadLong(aText, GTC(0x0046d5c4), &gHighScores[0][1].mLevel);
	ReadLong(aText, GTC(0x0046d5ac), &gHighScores[0][2].mLevel);
	ReadLong(aText, GTC(0x0046d594), &gHighScores[0][3].mLevel);
	ReadLong(aText, GTC(0x0046d57c), &gHighScores[0][4].mLevel);
	ReadLong(aText, GTC(0x0046d564), &gHighScores[1][0].mLevel);
	ReadLong(aText, GTC(0x0046d54c), &gHighScores[1][1].mLevel);
	ReadLong(aText, GTC(0x0046d534), &gHighScores[1][2].mLevel);
	ReadLong(aText, GTC(0x0046d51c), &gHighScores[1][3].mLevel);
	ReadLong(aText, GTC(0x0046d504), &gHighScores[1][4].mLevel);
	ReadLong(aText, GTC(0x0046d4ec), &gHighScores[2][0].mLevel);
	ReadLong(aText, GTC(0x0046d4d4), &gHighScores[2][1].mLevel);
	ReadLong(aText, GTC(0x0046d4bc), &gHighScores[2][2].mLevel);
	ReadLong(aText, GTC(0x0046d4a4), &gHighScores[2][3].mLevel);
	ReadLong(aText, GTC(0x0046d48c), &gHighScores[2][4].mLevel);
	ReadLong(aText, GTC(0x0046d474), &gHighScores[3][0].mLevel);
	ReadLong(aText, GTC(0x0046d45c), &gHighScores[3][1].mLevel);
	ReadLong(aText, GTC(0x0046d444), &gHighScores[3][2].mLevel);
	ReadLong(aText, GTC(0x0046d42c), &gHighScores[3][3].mLevel);
	ReadLong(aText, GTC(0x0046d414), &gHighScores[3][4].mLevel);
	ReadLong(aText, GTC(0x0046d3fc), &gHighScores[0][0].mScore);
	ReadLong(aText, GTC(0x0046d3e4), &gHighScores[0][1].mScore);
	ReadLong(aText, GTC(0x0046d3cc), &gHighScores[0][2].mScore);
	ReadLong(aText, GTC(0x0046d3b4), &gHighScores[0][3].mScore);
	ReadLong(aText, GTC(0x0046d39c), &gHighScores[0][4].mScore);
	ReadLong(aText, GTC(0x0046d384), &gHighScores[1][0].mScore);
	ReadLong(aText, GTC(0x0046d36c), &gHighScores[1][1].mScore);
	ReadLong(aText, GTC(0x0046d354), &gHighScores[1][2].mScore);
	ReadLong(aText, GTC(0x0046d33c), &gHighScores[1][3].mScore);
	ReadLong(aText, GTC(0x0046d324), &gHighScores[1][4].mScore);
	ReadLong(aText, GTC(0x0046d30c), &gHighScores[2][0].mScore);
	ReadLong(aText, GTC(0x0046d2f4), &gHighScores[2][1].mScore);
	ReadLong(aText, GTC(0x0046d2dc), &gHighScores[2][2].mScore);
	ReadLong(aText, GTC(0x0046d2c4), &gHighScores[2][3].mScore);
	ReadLong(aText, GTC(0x0046d2ac), &gHighScores[2][4].mScore);
	ReadLong(aText, GTC(0x0046d294), &gHighScores[3][0].mScore);
	ReadLong(aText, GTC(0x0046d27c), &gHighScores[3][1].mScore);
	ReadLong(aText, GTC(0x0046d264), &gHighScores[3][2].mScore);
	ReadLong(aText, GTC(0x0046d24c), &gHighScores[3][3].mScore);
	ReadLong(aText, GTC(0x0046d234), &gHighScores[3][4].mScore);
	ReadLong(aText, GTC(0x0046d224), &theConfig->mHighscoresBgx);
	ReadLong(aText, GTC(0x0046d214), &theConfig->mHighscoresBgy);
	ReadInts(aText, GTC(0x0046d20c), theConfig->mBkgcolx, 8);
	ReadInts(aText, GTC(0x0046d200), theConfig->mBkgcoldx, 8);
	ReadInts(aText, GTC(0x0046d1ec), theConfig->mFloaterprobability, 8);
	ReadLong(aText, GTC(0x0046d1e0), &theConfig->mMusicDelay);
	ReadLong(aText, GTC(0x0046d1d0), &theConfig->mSplashDelay);
	ReadLong(aText, GTC(0x0046d1bc), &theConfig->mSplashDisplacement);
	ReadFloat(aText, GTC(0x0046d1a8), &theConfig->mSplashSpringtime);
	ReadFloat(aText, GTC(0x0046d194), &theConfig->mSplashCoefficient);
	ReadLong(aText, GTC(0x0046d184), &theConfig->mSplashMovetime);
	ReadLong(aText, GTC(0x0046d174), &theConfig->mFloatershake);
	ReadLong(aText, GTC(0x0046d160), &theConfig->mScoremessageDelay);
	ReadFloat(aText, GTC(0x0046d144), &theConfig->mScoremessageMoveexponent);
	ReadFloat(aText, GTC(0x0046d128), &theConfig->mScoremessageFadeexponent);
	ReadLong(aText, GTC(0x0046d110), &theConfig->mScoremessageDistance);
	ReadLong(aText, GTC(0x0046d0f8), &theConfig->mScoreRemovedbyspecial);
	ReadLong(aText, GTC(0x0046d0e8), &theConfig->mSplashLoadbarx);
	ReadLong(aText, GTC(0x0046d0d8), &theConfig->mSplashLoadbary);
	ReadLong(aText, GTC(0x0046d0c4), &theConfig->mSplashRegisterx);
	ReadLong(aText, GTC(0x0046d0b0), &theConfig->mSplashRegistery);
	ReadLong(aText, GTC(0x0046d0a0), &theConfig->mSplashPlayx);
	ReadLong(aText, GTC(0x0046d090), &theConfig->mSplashPlayy);
	ReadLong(aText, GTC(0x0046d078), &theConfig->mSplashUnregisteredx);
	ReadLong(aText, GTC(0x0046d060), &theConfig->mSplashUnregisteredy);
	ReadLong(aText, GTC(0x0046d048), &theConfig->mSplashUnregistered2x);
	ReadLong(aText, GTC(0x0046d030), &theConfig->mSplashUnregistered2y);
	ReadLong(aText, GTC(0x0046d01c), &theConfig->mSplashRegisteredx);
	ReadLong(aText, GTC(0x0046d008), &theConfig->mSplashRegisteredy);
	ReadLong(aText, GTC(0x0046cff4), &theConfig->mSplashTimesplayedx);
	ReadLong(aText, GTC(0x0046cfe0), &theConfig->mSplashTimesplayedy);
	ReadLong(aText, GTC(0x0046cfc8), &theConfig->mSplashExecutionsleftx);
	ReadLong(aText, GTC(0x0046cfb0), &theConfig->mSplashExecutionslefty);
	ReadLong(aText, GTC(0x0046cf98), &theConfig->mRegisterBackgroundx);
	ReadLong(aText, GTC(0x0046cf80), &theConfig->mRegisterBackgroundy);
	ReadLong(aText, GTC(0x0046cf70), &theConfig->mRegisterNamex);
	ReadLong(aText, GTC(0x0046cf60), &theConfig->mRegisterNamey);
	ReadLong(aText, GTC(0x0046cf4c), &theConfig->mRegisterNamewidth);
	ReadLong(aText, GTC(0x0046cf3c), &theConfig->mRegisterCodex);
	ReadLong(aText, GTC(0x0046cf2c), &theConfig->mRegisterCodey);
	ReadLong(aText, GTC(0x0046cf18), &theConfig->mRegisterCodewidth);
	ReadLong(aText, GTC(0x0046cf08), &theConfig->mRegisterOkx);
	ReadLong(aText, GTC(0x0046cef8), &theConfig->mRegisterOky);
	ReadLong(aText, GTC(0x0046cee4), &theConfig->mRegisterCancelx);
	ReadLong(aText, GTC(0x0046ced0), &theConfig->mRegisterCancely);
	ReadLong(aText, GTC(0x0046cebc), &theConfig->mRegisterGetcodex);
	ReadLong(aText, GTC(0x0046cea8), &theConfig->mRegisterGetcodey);
	ReadLong(aText, GTC(0x0046ce98), &theConfig->mRegisterPastex);
	ReadLong(aText, GTC(0x0046ce88), &theConfig->mRegisterPastey);
	ReadLong(aText, GTC(0x0046ce78), &theConfig->mValidcodeBgx);
	ReadLong(aText, GTC(0x0046ce68), &theConfig->mValidcodeBgy);
	ReadLong(aText, GTC(0x0046ce58), &theConfig->mValidcodeOkx);
	ReadLong(aText, GTC(0x0046ce48), &theConfig->mValidcodeOky);
	ReadLong(aText, GTC(0x0046ce38), &theConfig->mInvalidcodeBgx);
	ReadLong(aText, GTC(0x0046ce28), &theConfig->mInvalidcodeBgy);
	ReadLong(aText, GTC(0x0046ce18), &theConfig->mInvalidcodeOkx);
	ReadLong(aText, GTC(0x0046ce08), &theConfig->mInvalidcodeOky);
	ReadLong(aText, GTC(0x0046cdf8), &theConfig->mInitialWait);
	ReadLong(aText, GTC(0x0046cde8), &theConfig->mAdditionalWait);
	ReadLong(aText, GTC(0x0046cdd4), &theConfig->mOptionsRegisterx);
	ReadLong(aText, GTC(0x0046cdc0), &theConfig->mOptionsRegistery);
	ReadLong(aText, GTC(0x0046cda8), &theConfig->mOptionsVisitpopcapx);
	ReadLong(aText, GTC(0x0046cd90), &theConfig->mOptionsVisitpopcapy);
	ReadLong(aText, GTC(0x0046cd84), &theConfig->mSummaryX);
	ReadLong(aText, GTC(0x0046cd78), &theConfig->mSummaryY);
	ReadLong(aText, GTC(0x0046cd64), &theConfig->mSummaryPlayagainx);
	ReadLong(aText, GTC(0x0046cd50), &theConfig->mSummaryPlayagainy);
	ReadLong(aText, GTC(0x0046cd40), &theConfig->mSummaryTimex);
	ReadLong(aText, GTC(0x0046cd30), &theConfig->mSummaryTimey);
	ReadLong(aText, GTC(0x0046cd18), &theConfig->mSummaryLevelreachedx);
	ReadLong(aText, GTC(0x0046cd00), &theConfig->mSummaryLevelreachedy);
	ReadLong(aText, GTC(0x0046ccec), &theConfig->mSummaryBestcombox);
	ReadLong(aText, GTC(0x0046ccd8), &theConfig->mSummaryBestcomboy);
	ReadLong(aText, GTC(0x0046ccc8), &theConfig->mSummaryScorex);
	ReadLong(aText, GTC(0x0046ccb8), &theConfig->mSummaryScorey);
	ReadLong(aText, GTC(0x0046cca4), &theConfig->mSummaryBestmatchx);
	ReadLong(aText, GTC(0x0046cc90), &theConfig->mSummaryBestmatchy);
	ReadLong(aText, GTC(0x0046cc84), &theConfig->mSummaryHsy);
	ReadLong(aText, GTC(0x0046cc74), &theConfig->mSummaryHsskipy);
	ReadLong(aText, GTC(0x0046cc64), &theConfig->mSummaryHsnamex);
	ReadLong(aText, GTC(0x0046cc50), &theConfig->mSummaryHsscorex);
	ReadLong(aText, GTC(0x0046cc3c), &theConfig->mSummaryHslevelx);
	ReadInts(aText, GTC(0x0046cc30), theConfig->mScoretable, 100);
	ReadInts(aText, GTC(0x0046cc20), theConfig->mBlreducetable, 100);
	ReadLong(aText, GTC(0x0046cc10), &theConfig->mFlashDuration);
	ReadLong(aText, GTC(0x0046cc00), &theConfig->mFlashDivisor);
	ReadLong(aText, GTC(0x0046cbf0), &theConfig->mSummaryDelay);
	ReadLong(aText, GTC(0x0046cbd8), &theConfig->mSummaryFademovetime);
	ReadLong(aText, GTC(0x0046cbc0), &theConfig->mSummaryMovedistance);
	ReadLong(aText, GTC(0x0046cbac), &theConfig->mSummaryDuration);
	ReadFloat(aText, GTC(0x0046cb94), &theConfig->mSummaryMotionexponent);
	ReadLong(aText, GTC(0x0046cb80), &theConfig->mNormalBonusmodulus);
	ReadLong(aText, GTC(0x0046cb6c), &theConfig->mTimedBonusmodulus);
	ReadLong(aText, GTC(0x0046cb58), &theConfig->mSpeedBonusmodulus);
	ReadLong(aText, GTC(0x0046cb40), &theConfig->mStrategyBonusmodulus);
	ReadLong(aText, GTC(0x0046cb2c), &theConfig->mBonusTimecountx);
	ReadLong(aText, GTC(0x0046cb18), &theConfig->mBonusTimecounty);
	ReadLong(aText, GTC(0x0046cb00), &theConfig->mBonusLargetimecountx);
	ReadLong(aText, GTC(0x0046cae8), &theConfig->mBonusLargetimecounty);
	ReadLong(aText, GTC(0x0046cacc), &theConfig->mBonustimerCriticaltime1);
	ReadLong(aText, GTC(0x0046cab0), &theConfig->mBonustimerCriticaltime2);
	ReadLong(aText, GTC(0x0046ca90), &theConfig->mBonustimerInitialmagnification);
	ReadFloat(aText, GTC(0x0046ca74), &theConfig->mBonustimerMotionexponent);
	ReadLong(aText, GTC(0x0046ca5c), &theConfig->mNextfloaterBlinkperiod);
	ReadLong(aText, GTC(0x0046ca48), &theConfig->mLevelcompleteDelay);
	ReadLong(aText, GTC(0x0046ca34), &theConfig->mInitialSfxvolume);
	ReadLong(aText, GTC(0x0046ca20), &theConfig->mInitialMusicvolume);
	ReadBool(aText, GTC(0x0046ca14), &theConfig->mTurnbased);
	ReadLong(aText, GTC(0x0046ca00), &theConfig->mLevelsummaryScorey);
	ReadLong(aText, GTC(0x0046c9ec), &theConfig->mLevelsummaryTimey);
	ReadLong(aText, GTC(0x0046c9d8), &theConfig->mLevelsummaryComboy);
	ReadLong(aText, GTC(0x0046c9c4), &theConfig->mLevelsummaryMatchy);
	ReadLong(aText, GTC(0x0046c9b0), &theConfig->mLevelsummaryItemx);
	ReadLong(aText, GTC(0x0046c99c), &theConfig->mLevelsummaryDelay);
	ReadLong(aText, GTC(0x0046c984), &theConfig->mLevelsummaryButtonx);
	ReadLong(aText, GTC(0x0046c96c), &theConfig->mLevelsummaryButtony);
	ReadBool(aText, GTC(0x0046c954), &theConfig->mLevelcompleteDisable);
	ReadLong(aText, GTC(0x0046c938), &theConfig->mFloaterdeterminationNormal);
	ReadLong(aText, GTC(0x0046c91c), &theConfig->mFloaterdeterminationTimed);
	ReadLong(aText, GTC(0x0046c900), &theConfig->mFloaterdeterminationSpeed);
	ReadLong(aText, GTC(0x0046c8e0), &theConfig->mFloaterdeterminationStrategy);
	ReadLong(aText, GTC(0x0046c8d0), &theConfig->mExecutionlimit);
	ReadBool(aText, GTC(0x0046c8bc), &theConfig->mExecutionlimitmode);
	ReadBool(aText, GTC(0x0046c89c), &theConfig->mExecutionlimitAllowcontinue);
	ReadLong(aText, GTC(0x0046c880), &theConfig->mSplashNomoreexecutionsx);
	ReadLong(aText, GTC(0x0046c864), &theConfig->mSplashNomoreexecutionsy);
	ReadLong(aText, GTC(0x0046c854), &theConfig->mSplashNmequitx);
	ReadLong(aText, GTC(0x0046c844), &theConfig->mSplashNmequity);
	ReadLong(aText, GTC(0x0046c830), &theConfig->mSplashNmeregisterx);
	ReadLong(aText, GTC(0x0046c81c), &theConfig->mSplashNmeregistery);
	ReadLong(aText, GTC(0x0046c804), &theConfig->mLevelsummaryStartevent);
	ReadLong(aText, GTC(0x0046c7ec), &theConfig->mLevelsummaryEndevent);
	ReadLong(aText, GTC(0x0046c7d4), &theConfig->mLevelsummaryEntertime);
	ReadLong(aText, GTC(0x0046c7bc), &theConfig->mLevelsummaryBouncetime);
	ReadLong(aText, GTC(0x0046c7a4), &theConfig->mLevelsummaryFadetime);
	ReadFloat(aText, GTC(0x0046c788), &theConfig->mLevelsummaryMaxcompression);
	ReadBool(aText, GTC(0x0046c76c), &theConfig->mDeletegamewarningDisable);
	ReadInts(aText, GTC(0x0046c758), theConfig->mNormalShakesounds, 5);
	ReadInts(aText, GTC(0x0046c744), theConfig->mTimedShakesounds, 5);
	ReadInts(aText, GTC(0x0046c730), theConfig->mSpeedShakesounds, 5);
	ReadInts(aText, GTC(0x0046c718), theConfig->mStrategyShakesounds, 5);
	ReadLong(aText, GTC(0x0046c708), &theConfig->mIngamePausex);
	ReadLong(aText, GTC(0x0046c6f8), &theConfig->mIngamePausey);
	ReadLong(aText, GTC(0x0046c6e0), &theConfig->mBkgcolMinbrightness);
	ReadLong(aText, GTC(0x0046c6c8), &theConfig->mBkgcolMaxbrightness);
	ReadLong(aText, GTC(0x0046c6b0), &theConfig->mBkgcolAddbrightness);
	ReadLong(aText, GTC(0x0046c69c), &theConfig->mBkgcolCycletime);
	ReadLong(aText, GTC(0x0046c684), &theConfig->mBkgcolResolutiontime);
	ReadBool(aText, GTC(0x0046c66c), &theConfig->mDisableUnregistered);
	ReadInts(aText, GTC(0x0046c65c), theConfig->mComboanimTable, 100);
	ReadInts(aText, GTC(0x0046c64c), theConfig->mStarburstTable, 100);
	ReadInts(aText, GTC(0x0046c63c), theConfig->mScorefontTable, 100);
	ReadInts(aText, GTC(0x0046c62c), theConfig->mScorewordTable, 100);
	if (theConfig->mRegisterUrl == nullptr)
	{
		theConfig->mRegisterUrl = (char*)Allocate(StringLength(aText, GTC(0x0046c61c)));
		ReadString(aText, GTC(0x0046c61c), theConfig->mRegisterUrl);
	}
	if (theConfig->mPublisherUrl == nullptr)
	{
		theConfig->mPublisherUrl = (char*)Allocate(StringLength(aText, GTC(0x0046c60c)));
		ReadString(aText, GTC(0x0046c60c), theConfig->mPublisherUrl);
	}
	theConfig->mRisCompanyname = (char*)Allocate(StringLength(aText, GTC(0x0046c5fc)));
	ReadString(aText, GTC(0x0046c5fc), theConfig->mRisCompanyname);
	theConfig->mRisProductname = (char*)Allocate(StringLength(aText, GTC(0x0046c5ec)));
	ReadString(aText, GTC(0x0046c5ec), theConfig->mRisProductname);
	theConfig->mRisHiddenfilename = (char*)Allocate(StringLength(aText, GTC(0x0046c5d8)));
	ReadString(aText, GTC(0x0046c5d8), theConfig->mRisHiddenfilename);
	uint32_t aCount;
	AllocateList(aText, GTC(0x0046c5c4), &theConfig->mMainmenuanimations, &aCount);
	theConfig->mMainmenuanimationsCount = aCount / 6;
	AllocateList(aText, GTC(0x0046c5ac), &theConfig->mNewgamemenuanimations, &aCount);
	theConfig->mNewgamemenuanimationsCount = aCount / 6;
	AllocateList(aText, GTC(0x0046c590), &theConfig->mHighscoresmenuanimations, &aCount);
	theConfig->mHighscoresmenuanimationsCount = aCount / 6;
	AllocateList(aText, GTC(0x0046c578), &theConfig->mOptionsmenuanimations, &aCount);
	theConfig->mOptionsmenuanimationsCount = aCount / 6;
	AllocateList(aText, GTC(0x0046c55c), &theConfig->mInstructionsmenuanimations, &aCount);
	theConfig->mInstructionsmenuanimationsCount = aCount / 6;
	AllocateList(aText, GTC(0x0046c544), &theConfig->mCreditsmenuanimations, &aCount);
	theConfig->mCreditsmenuanimationsCount = aCount / 6;
	if (ReadBool(aText, GTC(0x0046c530), &theConfig->mFallfloatAdjust) < 0)
		theConfig->mFallfloatAdjust = 1;
	if (ReadBool(aText, GTC(0x0046c520), &theConfig->mDisableStops) < 0)
		theConfig->mDisableStops = 1;
	if (ReadLong(aText, GTC(0x0046c510), &theConfig->mMagnetEffect) < 0)
		theConfig->mMagnetEffect = 1;
	AllocateList(aText, GTC(0x0046c4fc), &theConfig->mComboanimParams, &aCount);
	theConfig->mComboanimParamsCount = aCount >> 1;
	AllocateList(aText, GTC(0x0046c4e8), &theConfig->mCombostarParams, &aCount);
	theConfig->mCombostarParamsCount = aCount >> 2;
	AllocateList(aText, GTC(0x0046c4d4), &theConfig->mStarburstParams, &aCount);
	theConfig->mStarburstParamsCount = aCount >> 2;
	ReadList(aText, GTC(0x0046c4c8), nullptr, &aCount);
	if (aCount != 0)
	{
		theConfig->mParticles = (Particle*)Allocate(aCount / 0xd * sizeof(Particle));
		ReadParticles(aText, GTC(0x0046c4c8), theConfig->mParticles, aCount / 0xd);
		gParticleState = (uint8_t**)calloc(aCount / 0xd, sizeof(uint8_t*));
		if (gParticleState == nullptr)
			NoMemory();
	}
	theConfig->mParticleCount = aCount / 0xd;
	ReadList(aText, GTC(0x0046c4b4), nullptr, &aCount);
	if (aCount != 0)
	{
		theConfig->mColumnnukeEffects.mEffects = (Effect*)Allocate((aCount - 2) / 10 * sizeof(Effect));
		ReadEffects(aText, GTC(0x0046c4b4), &theConfig->mColumnnukeEffects, theConfig->mColumnnukeEffects.mEffects,
			(aCount - 2) / 10);
	}
	ReadList(aText, GTC(0x0046c4a0), nullptr, &aCount);
	if (aCount != 0)
	{
		theConfig->mColorbombEffects.mEffects = (Effect*)Allocate((aCount - 2) / 10 * sizeof(Effect));
		ReadEffects(aText, GTC(0x0046c4a0), &theConfig->mColorbombEffects, theConfig->mColorbombEffects.mEffects,
			(aCount - 2) / 10);
	}
	theConfig->mBlockgridCenterX = theConfig->mBlockgridX;
	theConfig->mBlockgridCenterX =
		(int32_t)((uint32_t)(theConfig->mBlockWidth * theConfig->mNumColumns) >> 1) + theConfig->mBlockgridCenterX;
	theConfig->mBlockgridCenterY = theConfig->mBlockgridY;
	theConfig->mBlockgridCenterY =
		(int32_t)((uint32_t)(theConfig->mBlockHeight * theConfig->mNumRows) >> 1) + theConfig->mBlockgridCenterY;
	if (theConfig->mTurnbased != 0)
		theConfig->mDequeueFloatDelay = 0;
	gLevelCount[0] = ReadLevels(&gLevels[0], aText.c_str(), 0);
	gLevelCount[1] = ReadLevels(&gLevels[1], aText.c_str(), 1);
	gLevelCount[2] = ReadLevels(&gLevels[2], aText.c_str(), 2);
	gLevelCount[3] = ReadLevels(&gLevels[3], aText.c_str(), 3);
	return 0;
}

} // namespace tiptop
