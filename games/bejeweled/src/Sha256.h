#ifndef __SHA256_H__
#define __SHA256_H__

// port(variant): SHA-256 (FIPS 180-4) for the RulesId (games/bejeweled/docs/CODE_MODS_DESIGN.md 3.7).
// Byte-oriented and free of platform dependencies, so native and wasm hash identically. Not part of the
// original game.

#include <cstddef>
#include <cstdint>
#include <string>

namespace Sexy
{

class Sha256
{
public:
	enum { DIGEST_SIZE = 32 };

	Sha256();
	void			Update(const void* theData, size_t theSize);
	void			Final(uint8_t theDigest[DIGEST_SIZE]);

	// One-shot helpers
	static void		Hash(const void* theData, size_t theSize, uint8_t theDigest[DIGEST_SIZE]);
	static std::string HashHex(const std::string& theData);  // 64 lower-case hex digits

private:
	void			Block(const uint8_t theBlock[64]);

	uint32_t		mState[8];
	uint64_t		mLength;     // bytes hashed so far
	uint8_t			mBuffer[64];
	size_t			mBufferLen;
};

}

#endif
