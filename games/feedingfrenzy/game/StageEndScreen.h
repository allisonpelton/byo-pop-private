#ifndef __FEEDINGFRENZY_STAGEENDSCREEN_H__
#define __FEEDINGFRENZY_STAGEENDSCREEN_H__

// The screen with a stage's results (StageEndScreen, 0x2f8 bytes, constructor 0x429a50, vtable 0x4a5a68)

#include "StageInfoScreen.h"

#include "SplatFactory.h"

namespace feedingfrenzy
{

class StageEndScreen : public StageInfoScreen
{
public:
	explicit StageEndScreen(StageMgr* theStage);  // 0x429a50

	void setUpResults(int theStageType, int theGameMode, engine::SplatFactory* theEatSplats);  // 0x4290f0
};

} // namespace feedingfrenzy

#endif
