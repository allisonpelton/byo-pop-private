#include "ProfileHash.h"

#include <cstdint>
#include <cstring>

using namespace Sexy;

namespace
{

// MD5 (RFC 1321), the exe's own at 0x4f0a10 (start), 0x4f1cd0 (add bytes) and 0x4f1dc0 (finish)
class Md5
{
public:
	Md5() :
		mByteCount(0)
	{
		mState[0] = 0x67452301;
		mState[1] = 0xefcdab89;
		mState[2] = 0x98badcfe;
		mState[3] = 0x10325476;
	}

	void Add(const unsigned char* theData, size_t theLength)
	{
		for (size_t i = 0; i < theLength; i++)
		{
			mBlock[mByteCount % 64] = theData[i];
			mByteCount++;
			if (mByteCount % 64 == 0)
				Transform(mBlock);
		}
	}

	void Finish(unsigned char theDigest[16])
	{
		uint64_t aBitCount = mByteCount * 8;
		unsigned char aPad = 0x80;
		Add(&aPad, 1);
		aPad = 0;
		while (mByteCount % 64 != 56)
			Add(&aPad, 1);
		unsigned char aLength[8];
		for (int i = 0; i < 8; i++)
			aLength[i] = (unsigned char)(aBitCount >> (8 * i));
		Add(aLength, 8);

		for (int i = 0; i < 4; i++)
		{
			theDigest[i * 4] = (unsigned char)mState[i];
			theDigest[i * 4 + 1] = (unsigned char)(mState[i] >> 8);
			theDigest[i * 4 + 2] = (unsigned char)(mState[i] >> 16);
			theDigest[i * 4 + 3] = (unsigned char)(mState[i] >> 24);
		}
	}

private:
	uint32_t		mState[4];
	uint64_t		mByteCount;
	unsigned char	mBlock[64];

	static uint32_t RotateLeft(uint32_t theValue, int theBits)
	{
		return (theValue << theBits) | (theValue >> (32 - theBits));
	}

	void Transform(const unsigned char* theBlock)
	{
		static const uint32_t aConstants[64] = {
			0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
			0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
			0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
			0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
			0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
			0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
			0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
			0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
		};
		static const int aShifts[64] = {
			7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
			5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
			4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
			6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
		};

		uint32_t aWords[16];
		for (int i = 0; i < 16; i++)
			aWords[i] = (uint32_t)theBlock[i * 4] | ((uint32_t)theBlock[i * 4 + 1] << 8) | ((uint32_t)theBlock[i * 4 + 2] << 16) | ((uint32_t)theBlock[i * 4 + 3] << 24);

		uint32_t a = mState[0];
		uint32_t b = mState[1];
		uint32_t c = mState[2];
		uint32_t d = mState[3];
		for (int i = 0; i < 64; i++)
		{
			uint32_t aMix;
			int aWord;
			if (i < 16)
			{
				aMix = (b & c) | (~b & d);
				aWord = i;
			}
			else if (i < 32)
			{
				aMix = (d & b) | (~d & c);
				aWord = (5 * i + 1) % 16;
			}
			else if (i < 48)
			{
				aMix = b ^ c ^ d;
				aWord = (3 * i + 5) % 16;
			}
			else
			{
				aMix = c ^ (b | ~d);
				aWord = (7 * i) % 16;
			}
			uint32_t aNext = d;
			d = c;
			c = b;
			b = b + RotateLeft(a + aMix + aConstants[i] + aWords[aWord], aShifts[i]);
			a = aNext;
		}
		mState[0] += a;
		mState[1] += b;
		mState[2] += c;
		mState[3] += d;
	}
};

}

// 0x46a120: the digest's sixteen bytes make one number, the first byte the most significant; it is printed in capital
// hexadecimal letters with no leading zeros (and "0" when it is zero)
std::string Sexy::HashProfileText(const std::string& theText)
{
	Md5 aMd5;
	aMd5.Add((const unsigned char*)theText.data(), theText.size());
	unsigned char aDigest[16];
	aMd5.Finish(aDigest);

	static const char aHexDigits[] = "0123456789ABCDEF";
	std::string aHash;
	for (int i = 0; i < 16; i++)
	{
		if (aHash.empty() && aDigest[i] == 0)
			continue;
		if (aHash.empty() && aDigest[i] < 16)
		{
			aHash += aHexDigits[aDigest[i] & 15];
			continue;
		}
		aHash += aHexDigits[aDigest[i] >> 4];
		aHash += aHexDigits[aDigest[i] & 15];
	}
	if (aHash.empty())
		aHash = "0";
	return aHash;
}
