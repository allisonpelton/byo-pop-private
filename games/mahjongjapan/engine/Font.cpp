#include "Font.h"

#include "Object.h"
#include "TextResource.h"

#include <cstring>

namespace jb
{

// 0x42ef50
CharMap::Row::~Row()
{
	delete[] mX;
	delete[] mWidths;
	if (mKerns != nullptr)
	{
		int i = 0;
		if (0 < mCount)
		{
			do
			{
				Kern* aKern = mKerns[i];
				while (aKern != nullptr)
				{
					Kern* aNext = aKern->mNext;
					delete aKern;
					aKern = aNext;
				}
				i = i + 1;
			} while (i < mCount);
		}
		delete[] mKerns;
	}
}

// 0x42efc0
CharMap::CharMap(int theCapacity) : mCapacity(theCapacity)
{
	mRows = new Row[theCapacity];
}

// 0x42f0d0
CharMap::~CharMap()
{
	delete[] mRows;
}

// 0x42f0f0 (the original had no check of the row count or of the 255 glyphs its buffers hold)
bool CharMap::ParseRow(const std::string& theLine, int thePos)
{
	if (mCapacity <= mUsed)
		return true;
	int anX[255];
	int aWidths[255];
	int anAcc = 0;
	int aLen = (int)theLine.size();
	int aCount = 0;
	Row& aRow = mRows[mUsed];
	aRow.mFirst = (uint16_t)ParseNumber(Tokenize(theLine, ',', thePos));
	do
	{
		std::string aToken = Tokenize(theLine, ',', thePos);
		int aWidth = ParseNumber(aToken);
		if (aCount < 255)
		{
			anX[aCount] = anAcc;
			aWidths[aCount] = aWidth;
		}
		anAcc = anAcc + aWidth;
		aCount = aCount + 1;
	} while (thePos < aLen);
	if (255 < aCount)
		aCount = 255;
	aRow.mCount = aCount;
	aRow.mKerns = new Kern*[aCount];
	int i = 0;
	if (0 < aCount)
	{
		do
		{
			i = i + 1;
			aRow.mKerns[i - 1] = nullptr;
		} while (i < aCount);
	}
	aRow.mX = new int[aCount];
	memcpy(aRow.mX, anX, aCount * sizeof(int));
	aRow.mWidths = new int[aCount];
	memcpy(aRow.mWidths, aWidths, aCount * sizeof(int));
	mUsed = mUsed + 1;
	return true;
}

// 0x42f2e0
bool CharMap::ParseKerns(const std::string& theLine, int thePos)
{
	int aLen = (int)theLine.size();
	uint16_t aChar = (uint16_t)ParseNumber(Tokenize(theLine, ',', thePos));
	int aRowIndex = 0;
	if (0 < mUsed)
	{
		do
		{
			Row& aRow = mRows[aRowIndex];
			if (aRow.mFirst <= aChar && (int)aChar < aRow.mCount + (int)aRow.mFirst)
			{
				int anIndex = aChar - aRow.mFirst;
				// The original's last node starts as the rows' address, so a second "K" line for a character
				// wrote its first node over the first row's count; this appends it to the list instead
				Kern* aLast = aRow.mKerns[anIndex];
				while (aLast != nullptr && aLast->mNext != nullptr)
					aLast = aLast->mNext;
				do
				{
					uint16_t aNext = (uint16_t)ParseNumber(Tokenize(theLine, ',', thePos));
					int aKern = ParseNumber(Tokenize(theLine, ',', thePos));
					Kern* aNode = new Kern;
					aNode->mChar = aNext;
					aNode->mKern = aKern;
					aNode->mNext = nullptr;
					if (aRow.mKerns[anIndex] == nullptr)
						aRow.mKerns[anIndex] = aNode;
					else
						aLast->mNext = aNode;
					aLast = aNode;
				} while (thePos < aLen);
				return true;
			}
			aRowIndex = aRowIndex + 1;
		} while (aRowIndex < mUsed);
	}
	return false;
}

// 0x42f4b0
bool CharMap::Lookup(uint16_t theChar, uint16_t theNext, Glyph& theGlyph)
{
	int aRowIndex = 0;
	if (0 < mUsed)
	{
		do
		{
			Row& aRow = mRows[aRowIndex];
			if (aRow.mFirst <= theChar && (int)theChar < aRow.mCount + (int)aRow.mFirst)
			{
				int anIndex = theChar - aRow.mFirst;
				theGlyph.mRow = aRowIndex;
				theGlyph.mX = aRow.mX[anIndex];
				theGlyph.mKern = 0;
				theGlyph.mWidth = aRow.mWidths[anIndex];
				if (theNext != 0 && theNext != ' ')
				{
					Kern* aKern = aRow.mKerns[anIndex];
					if (aKern != nullptr)
					{
						while (aKern->mChar != theNext)
						{
							aKern = aKern->mNext;
							if (aKern == nullptr)
								return true;
						}
						theGlyph.mKern = aKern->mKern;
					}
				}
				return true;
			}
			aRowIndex = aRowIndex + 1;
		} while (aRowIndex < mUsed);
	}
	return false;
}

// 0x42f580
bool CharMap::Has(uint16_t theChar)
{
	if (theChar == ' ')
		return true;
	int aRowIndex = 0;
	if (0 < mUsed)
	{
		do
		{
			if (mRows[aRowIndex].mFirst <= theChar && (int)theChar < mRows[aRowIndex].mCount + (int)mRows[aRowIndex].mFirst)
				return true;
			aRowIndex = aRowIndex + 1;
		} while (aRowIndex < mUsed);
	}
	return false;
}

// 0x423930
Font::~Font()
{
	delete[] mWidths;
	delete mMap;
}

// 0x423970
bool Font::Load(int theImage, int theTextId, Module theModule)
{
	mImage = theImage;
	TextResource aText;
	if (!aText.Load(theTextId, 1, theModule))
		return false;
	std::string aLine;
	int aPos = 0;
	bool aMore = aText.ReadLine(aPos, aLine);
	while (aMore)
	{
		char aKind = aLine.empty() ? '\0' : aLine[0];
		if (aKind == 'H')
		{
			int aField = 2;
			mLineHeight = ParseNumber(Tokenize(aLine, ',', aField));
			mAscent = ParseNumber(Tokenize(aLine, ',', aField));
			mSpacing = ParseNumber(Tokenize(aLine, ',', aField));
			mSpaceWidth = ParseNumber(Tokenize(aLine, ',', aField));
			int aRows = ParseNumber(Tokenize(aLine, ',', aField));
			mMap = new CharMap(aRows);
		}
		else if (aKind == 'K')
		{
			mMap->ParseKerns(aLine, 2);
		}
		else if (aKind == 'S')
		{
			mMap->ParseRow(aLine, 2);
		}
		aMore = aText.ReadLine(aPos, aLine);
	}
	return true;
}

// 0x423c00 (a character the font lacks adds the previous character's advance again, as in the original)
int Font::GetWidth(const char* theText, int theScale)
{
	if (*theText != '\0')
	{
		int i = 0;
		int anAcc = 0;
		int anAdvance = 0;
		while (true)
		{
			uint8_t aChar = (uint8_t)theText[i];
			if (aChar == ' ')
			{
				anAdvance = mSpaceWidth;
			}
			else if (aChar == '\t')
			{
				anAdvance = 0;  // no tab stops
			}
			else
			{
				CharMap::Glyph aGlyph;
				if (mMap->Lookup(aChar, (uint8_t)theText[i + 1], aGlyph))
					anAdvance = mSpacing + aGlyph.mKern + aGlyph.mWidth;
			}
			int aScaled = anAdvance;
			if (theScale != 100)
				aScaled = MulDiv(anAdvance, theScale, 100);
			anAcc = anAcc + aScaled;
			i = i + 1;
			if (2000 < i)
				break;
			if (theText[i] == '\0')
				return anAcc;
		}
	}
	return 0;
}

// 0x423cc0
bool Font::HasChar(uint8_t theChar)
{
	return mMap->Has(theChar);
}

// 0x423ce0
void Font::Filter(std::string& theText)
{
	std::string aKept;
	int i = 0;
	int aLen = (int)theText.size();
	if (0 < aLen)
	{
		do
		{
			if (mMap->Has((uint8_t)theText[i]))
				aKept += theText[i];
			i = i + 1;
		} while (i < aLen);
	}
	theText = aKept;
}

// 0x423d80 (a first character the font lacks added a stale register in the original, here 0)
int Font::FitCount(const char* theText, int theMaxWidth)
{
	if (*theText == '\0')
		return 0;
	int i = 0;
	int anAcc = 0;
	int anAdvance = 0;
	do
	{
		uint8_t aChar = (uint8_t)theText[i];
		if (aChar == ' ')
		{
			anAdvance = mSpaceWidth;
		}
		else if (aChar == '\t')
		{
			anAdvance = 0;  // no tab stops
		}
		else
		{
			CharMap::Glyph aGlyph;
			if (mMap->Lookup(aChar, (uint8_t)theText[i + 1], aGlyph))
				anAdvance = mSpacing + aGlyph.mKern + aGlyph.mWidth;
		}
		anAcc = anAdvance + anAcc;
	} while (anAcc <= theMaxWidth && (i = i + 1, i < 0x7d1) && theText[i] != '\0');
	return i;
}

// 0x423e20 (a tab is looked up as a glyph here; a first character the font lacks gave a stale register, here 0)
int* Font::GetWidths(const char* theText, int* theLength)
{
	int aLen = (int)strlen(theText);
	if (theLength != nullptr)
		*theLength = aLen;
	if (aLen != 0)
	{
		if (mWidthsSize < aLen)
		{
			delete[] mWidths;
			mWidths = new int[aLen];
			mWidthsSize = aLen;
		}
		int i = 0;
		int anAdvance = 0;
		char aChar = *theText;
		while (aChar != '\0')
		{
			if (theText[i] == ' ')
			{
				anAdvance = mSpaceWidth;
			}
			else
			{
				CharMap::Glyph aGlyph;
				if (mMap->Lookup((uint8_t)theText[i], (uint8_t)theText[i + 1], aGlyph))
					anAdvance = mSpacing + aGlyph.mKern + aGlyph.mWidth;
			}
			i = i + 1;
			mWidths[i - 1] = anAdvance;
			if (2000 < i)
				return nullptr;
			aChar = theText[i];
		}
	}
	return mWidths;
}

// 0x423ef0
int Font::DrawLeft(Object* theObject, int theX, int theY, const char* theText, int theMaxWidth, int theScaleX,
	int theScaleY, int theAlpha)
{
	if (*theText == '\0')
		return 0;
	int i = 0;
	int anAcc = 0;
	do
	{
		uint8_t aChar = (uint8_t)theText[i];
		int anAdvance = 0;
		if (aChar == ' ')
		{
			anAdvance = mSpaceWidth;
		}
		else if (aChar == '\t')
		{
			anAdvance = 0;  // no tab stops
		}
		else
		{
			CharMap::Glyph aGlyph;
			if (mMap->Lookup(aChar, (uint8_t)theText[i + 1], aGlyph))
			{
				anAdvance = mSpacing + aGlyph.mKern + aGlyph.mWidth;
				if (theMaxWidth != -1)
				{
					int aScaled = anAdvance;
					if (theScaleX != 100)
						aScaled = MulDiv(anAdvance, theScaleX, 100);
					if (theMaxWidth < aScaled + anAcc)
						return i;
				}
				Rect aSrc;
				aSrc.top = mLineHeight * aGlyph.mRow;
				aSrc.bottom = mLineHeight + aSrc.top;
				aSrc.left = aGlyph.mX;
				aSrc.right = aGlyph.mX + aGlyph.mWidth;
				theObject->DrawImage(theX + anAcc, theY - mAscent, mImage, &aSrc, true, theScaleX, theScaleY,
					theAlpha);
			}
		}
		if (theScaleX != 100)
			anAdvance = MulDiv(anAdvance, theScaleX, 100);
		anAcc = anAcc + anAdvance;
		i = i + 1;
	} while (i < 0x7d1 && theText[i] != '\0');
	return i;
}

// 0x4240a0
void Font::DrawCentred(Object* theObject, int theX, int theY, const char* theText, int theWidth, int theScaleX,
	int theScaleY, int theAlpha)
{
	int aWidth = GetWidth(theText, theScaleX);
	DrawLeft(theObject, (theWidth - aWidth) / 2 + theX, theY, theText, -1, theScaleX, theScaleY, theAlpha);
}

// 0x4240f0
void Font::DrawRight(Object* theObject, int theX, int theY, const char* theText, int theScaleX, int theScaleY,
	int theAlpha)
{
	int aWidth = GetWidth(theText, theScaleX);
	DrawLeft(theObject, theX - aWidth, theY, theText, -1, theScaleX, theScaleY, theAlpha);
}

// 0x424130
void Font::DrawLines(Object* theObject, int theX, int theY, const char* theText, int theLineStep, int theScaleX,
	int theScaleY, int theAlpha)
{
	std::string aText = theText;
	int aPos = 0;
	int aLen = (int)aText.size();
	std::string aLine = Tokenize(aText, '\n', aPos);
	while (aPos < aLen)
	{
		DrawLeft(theObject, theX, theY, aLine.c_str(), -1, theScaleX, theScaleY, theAlpha);
		aLine = Tokenize(aText, '\n', aPos);
		int aStep = theLineStep;
		if (theScaleY != 100)
			aStep = MulDiv(theLineStep, theScaleY, 100);
		theY = theY + aStep;
	}
	DrawLeft(theObject, theX, theY, aLine.c_str(), -1, theScaleX, theScaleY, theAlpha);
}

// 0x4242a0
void Font::DrawLinesCentred(Object* theObject, int theX, int theY, const char* theText, int theWidth,
	int theLineStep, int theScaleX, int theScaleY, int theAlpha)
{
	std::string aText = theText;
	int aPos = 0;
	int aLen = (int)aText.size();
	std::string aLine = Tokenize(aText, '\n', aPos);
	while (aPos < aLen)
	{
		DrawCentred(theObject, theX, theY, aLine.c_str(), theWidth, theScaleX, theScaleY, theAlpha);
		aLine = Tokenize(aText, '\n', aPos);
		int aStep = theLineStep;
		if (theScaleY != 100)
			aStep = MulDiv(theLineStep, theScaleY, 100);
		theY = theY + aStep;
	}
	DrawCentred(theObject, theX, theY, aLine.c_str(), theWidth, theScaleX, theScaleY, theAlpha);
}

// 0x424460
int Font::DrawWrappedCentred(Object* theObject, int theX, int theY, int theWidth, int theMaxHeight,
	const char* theText, int theLineStep, bool theMeasureOnly, int theAlpha)
{
	return DrawWrappedWorker(theObject, theX, theY, theWidth, theMaxHeight, theText, theLineStep, theMeasureOnly,
		theAlpha, true);
}

// 0x4244a0
int Font::DrawWrappedWorker(Object* theObject, int theX, int theY, int theWidth, int theMaxHeight,
	const char* theText, int theLineStep, bool theMeasureOnly, int theAlpha, bool theCentred)
{
	int anAcc = 0;
	int aLines = 0;
	std::string aRest = theText;
	std::string aWord;
	std::string aLine;
	int aNext = NextWord(0, aRest, aWord);
	int aY = theY;
	int anEnd = aNext;
	int aStart;
	while (aStart = aNext, aStart != -1)
	{
		anAcc = anAcc + GetWidth(aWord.c_str(), 100);
		int aSpaces = TrailingSpaces(aWord.c_str());
		if (theWidth < anAcc - mSpaceWidth * aSpaces && 0 < anEnd)
		{
			aLine = aRest.substr(0, anEnd);
			TrimRight(aLine);
			if (theMaxHeight < aY - theY)
				return aLines;
			if (!theMeasureOnly)
			{
				if (!theCentred)
					DrawLeft(theObject, theX, aY, aLine.c_str(), theWidth + 1, 100, 100, theAlpha);
				else
					DrawCentred(theObject, theX, aY, aLine.c_str(), theWidth, 100, 100, theAlpha);
			}
			aLines = aLines + 1;
			aRest = aRest.substr(anEnd);
			TrimLeft(aRest);
			anAcc = 0;
			aStart = 0;
			aY = aY + theLineStep;
		}
		aNext = NextWord(aStart, aRest, aWord);
		anEnd = aStart;
	}
	TrimRight(aRest);
	if (0 < (int)aRest.size() && aY - theY <= theMaxHeight)
	{
		if (!theMeasureOnly)
		{
			if (!theCentred)
				DrawLeft(theObject, theX, aY, aRest.c_str(), theWidth + 1, 100, 100, theAlpha);
			else
				DrawCentred(theObject, theX, aY, aRest.c_str(), theWidth, 100, 100, theAlpha);
		}
		aLines = aLines + 1;
	}
	return aLines;
}

// 0x424760
int Font::NextWord(int theStart, const std::string& theText, std::string& theWord)
{
	if ((int)theText.size() <= theStart)
	{
		theWord.clear();
		return -1;
	}
	int i = theStart;
	if (theStart < (int)theText.size())
	{
		do
		{
			if (theText[i] != ' ')
				break;
			i = i + 1;
		} while (i < (int)theText.size());
	}
	int anEnd = FindOneOf(theText, i, ".,:; ");
	if (anEnd == -1)
	{
		theWord = theText.substr(theStart);
		return (int)theText.size();
	}
	theWord = theText.substr(theStart, (anEnd + 1) - theStart);
	return anEnd + 1;
}

// 0x424880
int Font::TrailingSpaces(const char* theText)
{
	int aCount = 0;
	int i = (int)strlen(theText) - 1;
	while (-1 < i && theText[i] == ' ')
	{
		i = i + 1;
		aCount = aCount + 1;
	}
	return aCount;
}

} // namespace jb
