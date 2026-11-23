#ifndef __FEEDINGFRENZY_FOODBANKRANK_H__
#define __FEEDINGFRENZY_FOODBANKRANK_H__

// The name of the rank the food bank's count has reached, from the game's twenty one rank texts
// (rankString0 to rankString20 of the string table). The high scores and the results of a stage show it.

#include "FoodBank.h"

#include <string>

namespace feedingfrenzy
{

// 0x4318d0
std::string getFoodBankRankName(const FoodBank& theFoodBank);

} // namespace feedingfrenzy

#endif
