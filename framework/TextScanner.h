#ifndef __TEXTSCANNER_H__
#define __TEXTSCANNER_H__

// port: reads words, whole numbers and lines out of a string exactly as std::istringstream does for the three things the
// framework asks of one (operator>> into an int or a std::string, std::getline, and a saved position), without the
// stream: libc++'s iostream and locale code is some 250 KB of a release exe that nothing else calls. The rules that the
// stream follows, and this follows:
// - a read first fails when an earlier one failed or reached the end (the stream's sentry), and leaves its value alone;
// - a word or a number skips the white space before it (space, \t, \n, \v, \f, \r), and a word ends at the next;
// - a whole number is an optional + or -, then decimal digits, and stops at the first other character, which stays;
//   no digit makes it fail with 0 written, and a value outside int makes it fail with INT_MAX or INT_MIN written;
// - reaching the end of the text while reading sets the end flag (a later read then fails) and, when nothing was read,
//   the failure flag as well;
// - a line ends at its delimiter, which is taken but not returned, and the end of the text is a line only when it has
//   a character in it.

#include <climits>
#include <cstddef>
#include <string>

namespace Sexy
{

class TextScanner
{
public:
	explicit TextScanner(const std::string& theText) : mText(theText) {}

	// like the stream, true while no read has failed
	explicit operator bool() const { return !mFailed; }
	bool operator!() const { return mFailed; }

	TextScanner& operator>>(int& theValue)
	{
		if (!SkipSpace())
			return *this;
		size_t aStart = mPos;
		if (mPos < mText.size() && (mText[mPos] == '+' || mText[mPos] == '-'))
			mPos++;
		size_t aDigitsStart = mPos;
		while (mPos < mText.size() && mText[mPos] >= '0' && mText[mPos] <= '9')
			mPos++;
		if (mPos >= mText.size())
			mAtEnd = true;
		if (mPos == aDigitsStart)
		{
			theValue = 0;
			mFailed = true;
			return *this;
		}
		// the digits as a long long, which holds any value that int does not and stops growing once it is past int
		bool isNegative = mText[aStart] == '-';
		long long aValue = 0;
		for (size_t i = aDigitsStart; i < mPos; i++)
		{
			if (aValue <= 0x7FFFFFFFFLL)
				aValue = aValue * 10 + (mText[i] - '0');
		}
		if (isNegative)
			aValue = -aValue;
		if (aValue > INT_MAX)
		{
			theValue = INT_MAX;
			mFailed = true;
		}
		else if (aValue < INT_MIN)
		{
			theValue = INT_MIN;
			mFailed = true;
		}
		else
			theValue = (int) aValue;
		return *this;
	}

	TextScanner& operator>>(std::string& theWord)
	{
		if (!SkipSpace())
			return *this;
		theWord.clear();
		while (mPos < mText.size() && !IsSpace(mText[mPos]))
			theWord += mText[mPos++];
		if (mPos >= mText.size())
			mAtEnd = true;
		return *this;
	}

	// std::getline(stream, theLine, theDelimiter)
	bool GetLine(std::string& theLine, char theDelimiter = '\n')
	{
		if (mAtEnd || mFailed)
		{
			mFailed = true;
			return false;
		}
		if (mPos >= mText.size())
		{
			mAtEnd = true;
			mFailed = true;
			return false;
		}
		theLine.clear();
		while (mPos < mText.size() && mText[mPos] != theDelimiter)
			theLine += mText[mPos++];
		if (mPos >= mText.size())
			mAtEnd = true;
		else
			mPos++;
		return true;
	}

	// tellg() and clear() followed by seekg()
	size_t Tell() const { return mPos; }
	void SeekTo(size_t thePos)
	{
		mFailed = false;
		mAtEnd = false;
		mPos = thePos;
	}

private:
	static bool IsSpace(char theChar)
	{
		return (theChar == ' ') || (theChar == '\t') || (theChar == '\n') || (theChar == '\v') || (theChar == '\f') || (theChar == '\r');
	}

	// the stream's sentry for a read that skips white space: false when the read cannot go on
	bool SkipSpace()
	{
		if (mAtEnd || mFailed)
		{
			mFailed = true;
			return false;
		}
		while (mPos < mText.size() && IsSpace(mText[mPos]))
			mPos++;
		if (mPos >= mText.size())
		{
			mAtEnd = true;
			mFailed = true;
			return false;
		}
		return true;
	}

	std::string mText;
	size_t mPos = 0;
	bool mAtEnd = false;
	bool mFailed = false;
};

}

#endif
