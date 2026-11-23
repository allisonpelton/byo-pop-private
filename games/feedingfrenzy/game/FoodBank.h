#ifndef __FEEDINGFRENZY_FOODBANK_H__
#define __FEEDINGFRENZY_FOODBANK_H__

// The fish the player has eaten in all the games (FoodBank, constructor 0x431650, the controller's +0x1a8): one count
// that every stage's eaten fish are added to, kept in the registry (Software\GameHouse\FeedingFrenzy\foodBank, the
// value foodCount). Its rank is how many of the five ranks the end screen shows the count has reached.

#include <string>

namespace feedingfrenzy
{

class FoodBank
{
public:
	FoodBank() : mCount(0) {}  // 0x4315d0

	// 0x431650: the count is read from the key
	void load(const std::string& theKeyName);
	// 0x431790: and written to it
	void save(const std::string& theKeyName) const;

	int getCount() const { return mCount; }  // 0x4315e0
	// 0x4315f0: the count grows, up to ten million
	void add(int theCount);
	// 0x431610: 0 below 500, then 1 below 1500, 2 below 3000, 3 below 4000, 4 below 10000, and 5
	int getRank() const;

private:
	unsigned int mCount;  // +0x00
};

} // namespace feedingfrenzy

#endif
