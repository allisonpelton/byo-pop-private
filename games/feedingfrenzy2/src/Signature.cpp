#include "Signature.h"

#include <cstring>

namespace
{

// 0x5adab8 (feedingfrenzy2), set from 0x5606e8 (feedingfrenzy2)
const char BASE64_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// 0x5a6318 (feedingfrenzy2): the padding, a 0x80 then zeroes
const uint8_t PADDING[64] = { 0x80 };

uint32_t rotateLeft(uint32_t theValue, int theCount)
{
	return (theValue << theCount) | (theValue >> (32 - theCount));
}

// RFC 1321's four rounds: each step's function, word, shift and constant
uint32_t roundF(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
uint32_t roundG(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
uint32_t roundH(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
uint32_t roundI(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

const int SHIFTS[4][4] = { { 7, 12, 17, 22 }, { 5, 9, 14, 20 }, { 4, 11, 16, 23 }, { 6, 10, 15, 21 } };

const uint32_t CONSTANTS[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

// 0x451e10 (feedingfrenzy2): the alphabet's find of the character, npos (-1) when it is not there
int findInAlphabet(char theChar)
{
	const char* aFound = theChar == '\0' ? nullptr : strchr(BASE64_ALPHABET, theChar);
	return aFound == nullptr ? -1 : (int)(aFound - BASE64_ALPHABET);
}

} // namespace

SignatureDigest::SignatureDigest()
{
	mBitCount[0] = 0;
	mBitCount[1] = 0;
	mState[0] = 0x7955e069;
	mState[1] = 0x4d227211;
	mState[2] = 0x8fad5a16;
	mState[3] = 0x5268daae;
}

void SignatureDigest::update(const void* theData, size_t theSize)
{
	const uint8_t* aData = (const uint8_t*)theData;
	uint32_t anIndex = (mBitCount[0] >> 3) & 0x3f;
	uint32_t aNewCount = mBitCount[0] + (uint32_t)theSize * 8;
	if (aNewCount < mBitCount[0])
		mBitCount[1]++;
	mBitCount[1] += (uint32_t)theSize >> 29;
	mBitCount[0] = aNewCount;
	while (theSize != 0)
	{
		theSize--;
		mBlock[anIndex++] = *aData++;
		if (anIndex == 64)
		{
			transform(mBlock);
			anIndex = 0;
		}
	}
}

std::string SignatureDigest::finish()
{
	uint8_t aCount[8];
	for (int i = 0; i < 4; i++)
	{
		aCount[i] = (uint8_t)(mBitCount[0] >> (i * 8));
		aCount[i + 4] = (uint8_t)(mBitCount[1] >> (i * 8));
	}
	uint32_t anIndex = (mBitCount[0] >> 3) & 0x3f;
	update(PADDING, (anIndex < 56 ? 56 : 120) - anIndex);
	memcpy(mBlock + 56, aCount, 8);
	transform(mBlock);
	for (int i = 0; i < 4; i++)
	{
		for (int aByte = 0; aByte < 4; aByte++)
			mDigest[i * 4 + aByte] = (uint8_t)(mState[i] >> (aByte * 8));
	}
	return std::string((const char*)mDigest, sizeof(mDigest));
}

void SignatureDigest::transform(const uint8_t* theBlock)
{
	uint32_t aWords[16];
	for (int i = 0; i < 16; i++)
	{
		aWords[i] = (uint32_t)theBlock[i * 4] | ((uint32_t)theBlock[i * 4 + 1] << 8) |
			((uint32_t)theBlock[i * 4 + 2] << 16) | ((uint32_t)theBlock[i * 4 + 3] << 24);
	}
	uint32_t a = mState[0];
	uint32_t b = mState[1];
	uint32_t c = mState[2];
	uint32_t d = mState[3];
	for (int aStep = 0; aStep < 64; aStep++)
	{
		int aRound = aStep / 16;
		uint32_t aMixed;
		int aWord;
		switch (aRound)
		{
		case 0:
			aMixed = roundF(b, c, d);
			aWord = aStep;
			break;
		case 1:
			aMixed = roundG(b, c, d);
			aWord = (5 * aStep + 1) % 16;
			break;
		case 2:
			aMixed = roundH(b, c, d);
			aWord = (3 * aStep + 5) % 16;
			break;
		default:
			aMixed = roundI(b, c, d);
			aWord = (7 * aStep) % 16;
			break;
		}
		uint32_t aNext = b + rotateLeft(a + aMixed + aWords[aWord] + CONSTANTS[aStep], SHIFTS[aRound][aStep % 4]);
		a = d;
		d = c;
		c = b;
		b = aNext;
	}
	mState[0] += a;
	mState[1] += b;
	mState[2] += c;
	mState[3] += d;
}

std::string encodeBase64(const std::string& theData)
{
	std::string aText;
	size_t aSize = theData.size();
	for (size_t i = 0; i < aSize; i += 3)
	{
		uint8_t aFirst = (uint8_t)theData[i];
		aText += BASE64_ALPHABET[aFirst >> 2];
		int aSecondIndex = (aFirst & 3) << 4;
		if (i + 1 < aSize)
			aSecondIndex |= ((uint8_t)theData[i + 1] >> 4) & 0xf;
		aText += BASE64_ALPHABET[aSecondIndex];
		if (i + 1 < aSize)
		{
			int aThirdIndex = ((uint8_t)theData[i + 1] & 0xf) << 2;
			if (i + 2 < aSize)
				aThirdIndex |= ((uint8_t)theData[i + 2] >> 6) & 3;
			aText += BASE64_ALPHABET[aThirdIndex];
		}
		else
			aText += '=';
		if (i + 2 < aSize)
			aText += BASE64_ALPHABET[(uint8_t)theData[i + 2] & 0x3f];
		else
			aText += '=';
	}
	return aText;
}

std::string decodeBase64(const std::string& theText)
{
	std::string aData;
	size_t aSize = theText.size();
	const char* aChars = theText.c_str();  // the original reads the second of a group past the end: the terminator
	for (size_t i = 0; i < aSize; i += 4)
	{
		int8_t aFirst = (int8_t)findInAlphabet(aChars[i]);
		int8_t aSecond = (int8_t)findInAlphabet(aChars[i + 1]);
		aData += (char)(((aSecond >> 4) & 3) | (uint8_t)(aFirst << 2));
		int8_t aThird = 0;
		if (i + 2 < aSize)
		{
			if (aChars[i + 2] == '=')
				break;
			aThird = (int8_t)findInAlphabet(aChars[i + 2]);
			aData += (char)(((aThird >> 2) & 0xf) | (uint8_t)(aSecond << 4));
		}
		if (i + 3 < aSize)
		{
			if (aChars[i + 3] == '=')
				break;
			int8_t aFourth = (int8_t)findInAlphabet(aChars[i + 3]);
			aData += (char)((uint8_t)(aThird << 6) | (uint8_t)aFourth);
		}
	}
	return aData;
}
