#ifndef __FEEDINGFRENZY_STAGEINFOSCREEN_H__
#define __FEEDINGFRENZY_STAGEINFOSCREEN_H__

// The screens a stage shows over itself before and after play: its start screen, the one that offers a continue and
// the game over screen (StageInfoScreen, 0x15c bytes, constructor 0x429db0, vtable 0x4a5bd0)

#include "ScreenLayout.h"

#include <string>

namespace feedingfrenzy
{

class StageMgr;

class StageInfoScreen : public engine::ScreenLayout
{
public:
	explicit StageInfoScreen(StageMgr* theStage);  // 0x429db0

	void setAcceptable(bool theAcceptable);  // 0x429c00
	void setText(const std::string& theNodeName, const std::string& theText);
};

} // namespace feedingfrenzy

#endif
