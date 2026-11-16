#ifndef __FEEDINGFRENZY_STAGEHUD_H__
#define __FEEDINGFRENZY_STAGEHUD_H__

// The display over a stage: the score, the time and the meter of the player's growth (0x60 bytes, constructor
// 0x431f00)

#include "ScreenLayout.h"

namespace feedingfrenzy
{

class StageMgr;

class StageHud
{
public:
	StageHud();                                              // 0x431f00
	void load(const std::string& theFile, StageMgr* theStage);  // 0x431f30
	void update();                                           // 0x4327c0
	engine::ScreenLayout* getRoot() { return mRoot; }         // 0x4315e0

private:
	engine::Ref<engine::ScreenLayout> mRoot;
};

} // namespace feedingfrenzy

#endif
