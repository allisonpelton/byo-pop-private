#include "ValueList.h"

#include <cstdio>
#include <cstdlib>

using namespace Sexy;

ValueList::ValueList() : mPos(0)
{
}

// 0x41e090: the text is cut at each ";" (a backslash keeps the next character), a "," in a number is read as a point,
// and each piece is read by atof in the English locale
ValueList::ValueList(const std::string& theText) : mPos(0)
{
	std::vector<std::string> aPieces;
	std::string aPiece;
	bool anEscaped = false;
	for (size_t i = 0; i < theText.size(); i++)
	{
		char aChar = theText[i];
		if (anEscaped)
		{
			aPiece += aChar;
			anEscaped = false;
		}
		else if (aChar == '\\')
			anEscaped = true;
		else if (aChar == ';')
		{
			aPieces.push_back(aPiece);
			aPiece.clear();
		}
		else
			aPiece += aChar;
	}
	if (!aPiece.empty() || !aPieces.empty())
		aPieces.push_back(aPiece);

	for (size_t i = 0; i < aPieces.size(); i++)
	{
		for (size_t j = 0; j < aPieces[i].size(); j++)
		{
			if (aPieces[i][j] == ',')
				aPieces[i][j] = '.';
		}
		mValues.push_back(atof(aPieces[i].c_str()));
	}
}

void ValueList::Add(double theValue)
{
	mValues.push_back(theValue);
}

void ValueList::AddInt(int theValue)
{
	mValues.push_back((double)theValue);
}

void ValueList::AddBool(bool theValue)
{
	mValues.push_back((double)(theValue != false));
}

double ValueList::NextDouble()
{
	int anIndex = mPos++;
	if (anIndex < 0 || anIndex >= (int)mValues.size())
		return 0.0;
	return mValues[anIndex];
}

int ValueList::NextInt()
{
	return (int)NextDouble();
}

bool ValueList::NextBool()
{
	return NextInt() != 0;
}

std::string ValueList::ToString() const
{
	std::string aText;
	for (size_t i = 0; i < mValues.size(); i++)
	{
		char aBuffer[400];
		snprintf(aBuffer, sizeof(aBuffer), "%f", mValues[i]);
		aText += aBuffer;
		if (i + 1 < mValues.size())
			aText += ";";
	}
	return aText;
}
