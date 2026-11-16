#include "FoodBankRank.h"
#include "FishFood.h"
#include "GameStrings.h"

#include <stdio.h>

using namespace feedingfrenzy;

namespace
{
// The rank texts are named for their number; a rank is held up to the count where the next begins, and the count
// that reaches the last of these is in the twentieth rank
const char* const RANK_TEXT_FORMAT = "rankString%d";
const int RANK_LIMITS[] = {100,  300,  500,  800,  1000, 1250, 1500, 2000, 2500, 3000, 3500,
                           4000, 4500, 5000, 5500, 6000, 6500, 7000, 8000, 9000};
const int RANK_LIMIT_COUNT = 20;
}

// 0x4318d0
std::string feedingfrenzy::getFoodBankRankName(const FoodBank& theFoodBank)
{
	int aCount = theFoodBank.getCount();
	int aRank = 0;
	while (aRank < RANK_LIMIT_COUNT && aCount >= RANK_LIMITS[aRank])
		aRank++;
	char aName[32];
	snprintf(aName, sizeof(aName), RANK_TEXT_FORMAT, aRank);
	return FishFood::instance()->getStrings().get(aName);
}
