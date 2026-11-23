#ifndef __FEEDINGFRENZY_FUNFACTS_H__
#define __FEEDINGFRENZY_FUNFACTS_H__

// The facts about the sea that the results of a stage show (FunFacts, 0x10 bytes, constructor 0x42b7f0; the file
// config\toolTips\funFacts.xml, read by 0x42ab70 with the handler 0x42a930): each is set in lines no wider than the
// text it goes in.

#include <string>
#include <vector>

namespace engine
{
class Font;
}

namespace feedingfrenzy
{

class FunFacts
{
public:
	FunFacts();  // 0x42b7f0

	// 0x42ab70: the facts of the file are read
	void load(const std::string& theFile);
	// 0x42b890: a fact is added after the others
	void add(const std::string& theFact);

	// 0x42ad90: the fact with a number from 0 (it is not checked that there is one), with a break wherever a line
	// would grow wider than theWidth in theFont (a break is the two letters backslash and n, which the text node turns
	// into a line)
	std::string getFact(int theIndex, engine::Font* theFont, float theWidth);
	// 0x42b280: a fact at random, set in the same way; the text "NO FACTS LOADED" when the file gave none
	std::string getRandomFact(engine::Font* theFont, float theWidth);

private:
	std::vector<std::string> mFacts;  // +0x04
};

} // namespace feedingfrenzy

#endif
