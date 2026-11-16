#ifndef __FEEDINGFRENZY_FUNFACTS_H__
#define __FEEDINGFRENZY_FUNFACTS_H__

// The facts about the sea that the loading map shows (FunFacts, 0x10 bytes, constructor 0x42b7f0; the file
// config\toolTips\funFacts.xml, read by 0x42ab70 with the handler 0x42a8c0)

#include <string>

namespace feedingfrenzy
{

class FunFacts
{
public:
	FunFacts();  // 0x42b7f0

	// 0x42ab70: the facts of the file are read
	void load(const std::string& theFile);
};

} // namespace feedingfrenzy

#endif
