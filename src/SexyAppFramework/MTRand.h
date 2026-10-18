#ifndef __MTRAND_H__
#define __MTRAND_H__

#include <cstdint>
#include <string>

namespace Sexy
{

#define MTRAND_N 624

class MTRand
{
	unsigned long mt[MTRAND_N]; /* the array for the state vector  */
	int mti;

public:
	MTRand(const std::string& theSerialData);
	MTRand(unsigned long seed);
	MTRand();

	void SRand(const std::string& theSerialData);
	void SRand(unsigned long seed);
	unsigned long NextNoAssert();
	unsigned long Next();
	unsigned long NextNoAssert(unsigned long range);
	unsigned long Next(unsigned long range);
	float NextNoAssert(float range);
	float Next( float range );

	// port: note that this is not a resumable state. It holds the 624 state words but not mti, the index
	// of the next word; MTRand(theSerialData) sets mti = MTRAND_N+1, which NextNoAssert treats as a spent
	// block, so the copy goes on from the next block and repeats the original's stream only if that was
	// serialised at a block boundary (SRand(theSerialData) on its own keeps the old mti). Kept as it is:
	// the -tracehash FULL hash hashes these bytes (Sexy::GetRandState). Use GetState/SetState instead.
	std::string Serialize();

	// port: the complete state, the state words and mti (0..MTRAND_N+1), for a board that owns its RNG
	// (docs/CODE_MODS_DESIGN.md 2.3): SetState(GetState()) goes on with exactly the same draws. SetState
	// returns false, and changes nothing, for an index outside that range or the all-zero state (the top
	// bit of the first word and every other word 0), from which every draw is 0.
	void GetState(uint32_t theState[MTRAND_N], int* theIndex) const;
	bool SetState(const uint32_t theState[MTRAND_N], int theIndex);

	static void SetRandAllowed(bool allowed);
};

struct MTAutoDisallowRand
{
	MTAutoDisallowRand() { MTRand::SetRandAllowed(false); }
	~MTAutoDisallowRand() { MTRand::SetRandAllowed(true); }
};

}

#endif //__MTRAND_H__