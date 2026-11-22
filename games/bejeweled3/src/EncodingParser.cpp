// Sexy::EncodingParser: see the header.
#include "EncodingParser.h"

#include "Bej3App.h"
#include "Buffer.h"

using namespace Sexy;

bool EncodingParser::OpenFile(const std::string& theFileName)
{
	Buffer aFile;
	if (gApp == nullptr || !gApp->ReadBufferFromFile(theFileName, &aFile))
		return false;
	mBytes.assign(aFile.GetDataPtr(), aFile.GetDataPtr() + aFile.GetDataLen());
	mPosition = 0;
	mPutBack.clear();
	mEncoding = ENCODING_BYTES;
	// 0x47ec60: two bytes ff fe or fe ff are UTF-16 (the original picks one UTF-16 decoder, 0x47e9c0, for both; the
	// port reads each in the byte order its mark gives), and three bytes ef bb bf are UTF-8
	if (mBytes.size() >= 2) {
		if (mBytes[0] == 0xff && mBytes[1] == 0xfe) {
			mEncoding = ENCODING_UTF16_LITTLE_ENDIAN;
			mPosition = 2;
		} else if (mBytes[0] == 0xfe && mBytes[1] == 0xff) {
			mEncoding = ENCODING_UTF16_BIG_ENDIAN;
			mPosition = 2;
		}
	}
	if (mEncoding == ENCODING_BYTES && mBytes.size() > 2 && mBytes[0] == 0xef && mBytes[1] == 0xbb && mBytes[2] == 0xbf) {
		mEncoding = ENCODING_UTF8;
		mPosition = 3;
	}
	mOpen = true;
	return true;
}

int EncodingParser::GetChar(wchar_t* theChar)
{
	if (theChar == nullptr)
		return 3;
	if (!mPutBack.empty()) {
		*theChar = mPutBack.back();
		mPutBack.pop_back();
		return GET_CHAR_OK;
	}
	if (!mOpen || mPosition >= mBytes.size())
		return GET_CHAR_END_OF_FILE;
	switch (mEncoding) {
	case ENCODING_BYTES:
		*theChar = mBytes[mPosition++];
		return GET_CHAR_OK;
	case ENCODING_UTF16_LITTLE_ENDIAN:
	case ENCODING_UTF16_BIG_ENDIAN: {
		if (mPosition + 1 >= mBytes.size())
			return GET_CHAR_END_OF_FILE;
		unsigned first = mBytes[mPosition];
		unsigned second = mBytes[mPosition + 1];
		mPosition += 2;
		*theChar = (wchar_t)(mEncoding == ENCODING_UTF16_LITTLE_ENDIAN ? (second << 8 | first) : (first << 8 | second));
		return GET_CHAR_OK;
	}
	case ENCODING_UTF8: {
		unsigned aLead = mBytes[mPosition++];
		int aFollowing = aLead >= 0xf0 ? 3 : aLead >= 0xe0 ? 2 : aLead >= 0xc0 ? 1 : 0;
		unsigned aValue = aFollowing == 0 ? aLead : aLead & (0x3f >> aFollowing);
		for (int i = 0; i < aFollowing; i++) {
			if (mPosition >= mBytes.size())
				return GET_CHAR_END_OF_FILE;
			aValue = aValue << 6 | (mBytes[mPosition++] & 0x3f);
		}
		*theChar = (wchar_t)aValue;
		return GET_CHAR_OK;
	}
	}
	return GET_CHAR_END_OF_FILE;
}

bool EncodingParser::EndOfFile() const
{
	return mPutBack.empty() && (!mOpen || mPosition >= mBytes.size());
}

bool EncodingParser::CloseFile()
{
	bool wasOpen = mOpen;
	mOpen = false;
	mBytes.clear();
	return wasOpen;
}

std::string Sexy::AppendUTF8(const std::string& theText, wchar_t theChar)
{
	std::string aResult = theText;
	unsigned aValue = (unsigned)theChar;
	if (aValue < 0x80) {
		aResult += (char)aValue;
	} else if (aValue < 0x800) {
		aResult += (char)(0xc0 | aValue >> 6);
		aResult += (char)(0x80 | (aValue & 0x3f));
	} else {
		aResult += (char)(0xe0 | aValue >> 12);
		aResult += (char)(0x80 | ((aValue >> 6) & 0x3f));
		aResult += (char)(0x80 | (aValue & 0x3f));
	}
	return aResult;
}
