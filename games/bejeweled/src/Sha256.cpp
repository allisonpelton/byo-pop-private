// port(variant): SHA-256 (FIPS 180-4), see Sha256.h.

#include "Sha256.h"

#include <cstring>

using namespace Sexy;

static const uint32_t gSha256K[64] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static inline uint32_t RotR(uint32_t theValue, int theBits)
{
	return (theValue >> theBits) | (theValue << (32 - theBits));
}

Sha256::Sha256()
{
	static const uint32_t aInit[8] =
	{
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
	};
	memcpy(mState, aInit, sizeof(mState));
	mLength = 0;
	mBufferLen = 0;
}

void Sha256::Block(const uint8_t theBlock[64])
{
	uint32_t w[64];
	for (int i = 0; i < 16; i++)
		w[i] = ((uint32_t) theBlock[i * 4] << 24) | ((uint32_t) theBlock[i * 4 + 1] << 16) |
			((uint32_t) theBlock[i * 4 + 2] << 8) | (uint32_t) theBlock[i * 4 + 3];
	for (int i = 16; i < 64; i++)
	{
		uint32_t s0 = RotR(w[i - 15], 7) ^ RotR(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = RotR(w[i - 2], 17) ^ RotR(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	uint32_t a = mState[0], b = mState[1], c = mState[2], d = mState[3];
	uint32_t e = mState[4], f = mState[5], g = mState[6], h = mState[7];
	for (int i = 0; i < 64; i++)
	{
		uint32_t s1 = RotR(e, 6) ^ RotR(e, 11) ^ RotR(e, 25);
		uint32_t ch = (e & f) ^ (~e & g);
		uint32_t t1 = h + s1 + ch + gSha256K[i] + w[i];
		uint32_t s0 = RotR(a, 2) ^ RotR(a, 13) ^ RotR(a, 22);
		uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
		uint32_t t2 = s0 + maj;
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}
	mState[0] += a;
	mState[1] += b;
	mState[2] += c;
	mState[3] += d;
	mState[4] += e;
	mState[5] += f;
	mState[6] += g;
	mState[7] += h;
}

void Sha256::Update(const void* theData, size_t theSize)
{
	const uint8_t* aData = (const uint8_t*) theData;
	mLength += theSize;
	while (theSize > 0)
	{
		size_t aCount = 64 - mBufferLen;
		if (aCount > theSize)
			aCount = theSize;
		memcpy(mBuffer + mBufferLen, aData, aCount);
		mBufferLen += aCount;
		aData += aCount;
		theSize -= aCount;
		if (mBufferLen == 64)
		{
			Block(mBuffer);
			mBufferLen = 0;
		}
	}
}

void Sha256::Final(uint8_t theDigest[DIGEST_SIZE])
{
	uint64_t aBits = mLength * 8;
	uint8_t aPad = 0x80;
	Update(&aPad, 1);
	aPad = 0;
	while (mBufferLen != 56)
		Update(&aPad, 1);
	uint8_t aLength[8];
	for (int i = 0; i < 8; i++)
		aLength[i] = (uint8_t) (aBits >> (56 - i * 8));
	Update(aLength, 8);
	for (int i = 0; i < 8; i++)
	{
		theDigest[i * 4] = (uint8_t) (mState[i] >> 24);
		theDigest[i * 4 + 1] = (uint8_t) (mState[i] >> 16);
		theDigest[i * 4 + 2] = (uint8_t) (mState[i] >> 8);
		theDigest[i * 4 + 3] = (uint8_t) mState[i];
	}
}

void Sha256::Hash(const void* theData, size_t theSize, uint8_t theDigest[DIGEST_SIZE])
{
	Sha256 aHash;
	aHash.Update(theData, theSize);
	aHash.Final(theDigest);
}

std::string Sha256::HashHex(const std::string& theData)
{
	uint8_t aDigest[DIGEST_SIZE];
	Hash(theData.data(), theData.size(), aDigest);
	static const char aHex[] = "0123456789abcdef";
	std::string aText;
	for (int i = 0; i < DIGEST_SIZE; i++)
	{
		aText += aHex[aDigest[i] >> 4];
		aText += aHex[aDigest[i] & 15];
	}
	return aText;
}
