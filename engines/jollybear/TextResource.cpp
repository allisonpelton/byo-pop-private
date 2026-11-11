#include "TextResource.h"

namespace jb
{

// 0x42b860 (mahjongchina), 0x425470 (mahjongjapan)
bool TextResource::Load(int theId, int theCharSize, Module theModule)
{
	if (theCharSize == 0)
		mCharSize = 1;
	else
		mCharSize = theCharSize;
	uint32_t aSize = 0;
	const uint8_t* aData = FindResourceData(theModule, "TXT", theId & 0xffff, &aSize);
	if (aData == nullptr)
		return false;
	mSize = (int)aSize;
	if (mCharSize == 2)
		mSize = (int)aSize / 2;
	mData = aData;
	return true;
}

// 0x42b900 (mahjongchina), 0x425510 (mahjongjapan) (the original's strcspn and wcscspn could run past the resource when
// it lacks a NUL; these stop at its end)
bool TextResource::ReadLine(int& thePos, std::string& theLine)
{
	if (mSize <= thePos)
		return false;
	if (mCharSize == 2)
	{
		const uint16_t* aText = (const uint16_t*)mData;
		int anEnd = thePos;
		while (anEnd < mSize && aText[anEnd] != 0 && aText[anEnd] != '\r' && aText[anEnd] != '\n')
			anEnd++;
		// The original converted with the ANSI code page; the game's TXT resources are ANSI, so this keeps the low
		// byte of each character
		std::string aLine;
		int aStop = anEnd;
		if (anEnd == mSize)
		{
			aStop = thePos;
			while (aStop < mSize && aText[aStop] != 0)
				aStop++;
		}
		for (int i = thePos; i < aStop; i++)
			aLine += (char)(aText[i] & 0xff);
		theLine = aLine;
		if (anEnd == mSize)
		{
			thePos = mSize;
			return true;
		}
		thePos = anEnd + 1;
		if (thePos < mSize)
		{
			while (aText[thePos] == '\r' || aText[thePos] == '\n')
			{
				thePos = thePos + 1;
				if (mSize <= thePos)
					return true;
			}
		}
		return true;
	}
	const char* aText = (const char*)mData;
	int anEnd = thePos;
	while (anEnd < mSize && aText[anEnd] != '\0' && aText[anEnd] != '\r' && aText[anEnd] != '\n')
		anEnd++;
	if (anEnd == mSize)
	{
		theLine.assign(aText + thePos, anEnd - thePos);
		thePos = mSize;
		return true;
	}
	theLine.assign(aText + thePos, anEnd - thePos);
	thePos = anEnd + 1;
	if (thePos < mSize)
	{
		do
		{
			if (aText[thePos] != '\r' && aText[thePos] != '\n')
				break;
			thePos = thePos + 1;
		} while (thePos < mSize);
	}
	return true;
}

} // namespace jb
