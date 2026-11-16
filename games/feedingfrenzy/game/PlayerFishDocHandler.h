#ifndef __FEEDINGFRENZY_PLAYERFISHDOCHANDLER_H__
#define __FEEDINGFRENZY_PLAYERFISHDOCHANDLER_H__

// The reader of a player fish's file (PlayerFishDocHandler): the <playerFish> element names the kind of fish (its
// class), how it moves, its pictures and its rectangles. It makes the fish and gives it to the stage.

#include "DocHandler.h"

namespace feedingfrenzy
{

class PlayerFish;
class StageMgr;

class PlayerFishDocHandler : public engine::BaseDocHandler
{
public:
	PlayerFishDocHandler(const std::string& theName, engine::DocHandlerStack* theParent, StageMgr* theStage) :
	    BaseDocHandler(theName, theParent), mStage(theStage)
	{
	}

	// 0x4182a0
	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override;

private:
	void readPlayer(const engine::XmlAttributes& theAttributes);  // 0x417530

	StageMgr* mStage;  // +0x14
};

} // namespace feedingfrenzy

#endif
