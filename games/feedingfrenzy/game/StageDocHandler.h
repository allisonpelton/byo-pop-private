#ifndef __FEEDINGFRENZY_STAGEDOCHANDLER_H__
#define __FEEDINGFRENZY_STAGEDOCHANDLER_H__

// The reader of a stage's file (StageDocHandler, 0x4240f0, vtable 0x4a54ec): the <stage> element gives the stage's name,
// what follows it, its size, its kind and its time; <playerFish> names the file of the player's fish and the <size>
// elements after it the sizes the fish grows through; each <enemyFish> is a kind of fish with the <spawnInfo> rules for
// when its fish come and the <staticSpawn> places where some stand when the stage starts; <environment> names the
// layouts of the pictures behind and in front, and <music> the sounds.

#include "DocHandler.h"

namespace feedingfrenzy
{

class FishSpawner;
class StageMgr;

class StageDocHandler : public engine::BaseDocHandler
{
public:
	StageDocHandler(const std::string& theName, engine::DocHandlerStack* theParent, StageMgr* theStage) :
	    BaseDocHandler(theName, theParent), mStage(theStage), mSpawner(nullptr)
	{
	}

	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override;  // 0x425ba0
	void endElement(const std::string& theName) override;                                                  // 0x4241c0

private:
	void readStage(const engine::XmlAttributes& theAttributes);        // 0x424330
	void readPlayerFile(const engine::XmlAttributes& theAttributes);   // 0x424970
	void readGrowthLevel(const engine::XmlAttributes& theAttributes);  // 0x424bd0
	void readEnemyFish(const engine::XmlAttributes& theAttributes);    // 0x424d50
	void readSpawnRule(const engine::XmlAttributes& theAttributes);    // 0x425130
	void readStaticSpawn(const engine::XmlAttributes& theAttributes);
	void readEnvironment(const engine::XmlAttributes& theAttributes);  // 0x4252d0
	void readMusic(const engine::XmlAttributes& theAttributes);        // 0x4256b0

	StageMgr* mStage;        // +0x14
	FishSpawner* mSpawner;   // +0x18: the kind of fish whose element is open (the stage takes it when it ends)
};

// 0x424330 and the rest: the stage reads its file
void loadStageFile(const std::string& theFile, StageMgr* theStage);

} // namespace feedingfrenzy

#endif
