#ifndef __FEEDINGFRENZY_STAGEINFOSCREEN_H__
#define __FEEDINGFRENZY_STAGEINFOSCREEN_H__

// The screens a stage shows over itself before and after play: its start screen, the one that offers a continue and
// the game over screen (StageInfoScreen, 0x15c bytes, constructor 0x429db0, vtable 0x4a5bd0). Its buttons act on the
// stage that owns it, not through the controller's listener (the screen's button listener, +0x138, is replaced).

#include "ScreenLayout.h"

#include <string>

namespace feedingfrenzy
{

class StageMgr;

class StageInfoScreen : public engine::ScreenLayout
{
public:
	explicit StageInfoScreen(StageMgr* theStage);  // 0x429db0

	// 0x429c00: the screen takes no more of the mouse (a stage that goes, or the options on top of it)
	void setAcceptable(bool theAcceptable) { mAcceptable = theAcceptable; }
	// 0x429c10: nothing is under the mouse when the screen is not acceptable
	engine::Node* hitTest(unsigned int theFlags, engine::Point& thePoint) override;

	// The text of a node of the screen is put
	void setText(const std::string& theNodeName, const std::string& theText);

	// The button listener's slots: 0x429c30 (a button the mouse is on: nothing), 0x429c50 (a press)
	void buttonEnter(std::string theName) override {}
	void buttonPress(std::string theName) override;
	void buttonExit(std::string theName) override {}

protected:
	StageMgr* mStage;   // +0x14c: not counted
	bool mAcceptable;   // +0x150
};

} // namespace feedingfrenzy

#endif
