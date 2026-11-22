#ifndef __FEEDINGFRENZY2_SIGNATURE_H__
#define __FEEDINGFRENZY2_SIGNATURE_H__

// The signatures of a player's saved values (UserData): an MD5 digest (RFC 1321) whose four starting words are the
// game's own, set in place by each caller, over each value's four bytes in turn, kept in the player's parameters as
// the digest's sixteen bytes in base64.

#include <cstddef>
#include <cstdint>
#include <string>

// An MD5 digest in progress (0x68 bytes, on the callers' stacks): the count of bits, the four words, the block being
// filled and, finished, the digest's bytes
class SignatureDigest
{
public:
	// The count 0 and the game's four starting words (0x7955e069, 0x4d227211, 0x8fad5a16, 0x5268daae), as each caller
	// sets them
	SignatureDigest();

	// 0x4550a0 (feedingfrenzy2): the bytes added, a block digested each time one is full
	void update(const void* theData, size_t theSize);
	// 0x468530 (feedingfrenzy2): the padding and the count added and the digest made; then (0x4734b0 (feedingfrenzy2),
	// a string's append) its sixteen bytes as a string
	std::string finish();

private:
	// The block's sixteen words, little endian, digested into the four words (RFC 1321's transform)
	void transform(const uint8_t* theBlock);

	uint32_t mBitCount[2];  // +0x00: low word, high word
	uint32_t mState[4];     // +0x08
	uint8_t mBlock[64];     // +0x18
	uint8_t mDigest[16];    // +0x58
};

// 0x4731f0 (feedingfrenzy2): base64, with '=' to fill the last group
std::string encodeBase64(const std::string& theData);
// 0x473f00 (feedingfrenzy2): base64 read back, stopping at a '='; a character not of the alphabet counts as -1, as the
// original's find gives it
std::string decodeBase64(const std::string& theText);

#endif
