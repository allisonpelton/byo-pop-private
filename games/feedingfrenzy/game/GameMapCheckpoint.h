#ifndef __FEEDINGFRENZY_GAMEMAPCHECKPOINT_H__
#define __FEEDINGFRENZY_GAMEMAPCHECKPOINT_H__

// A stage's place on the map (GameMapCheckpoint, 0x144 bytes, constructor 0x422320, vtable 0x4a4f40): a dot at the stage's
// position that shows whether the stage is not reached, passed, the one that loads, or can be chosen. The dot of the
// stage that loads blinks. A dot that can be chosen is a button that tells the map screen it was pressed.

#include "Node.h"

namespace engine
{
class ImageButton;
class ImageNode;
class ScreenLayout;
}

namespace feedingfrenzy
{

class StageInfo;

// The states of a checkpoint's dot (the values of setState)
enum CheckpointState
{
	CHECKPOINT_OFF = 0,        // not reached: a plain dot
	CHECKPOINT_PASSED = 1,     // beaten: a checked dot
	CHECKPOINT_LOADING = 2,    // the stage that loads: a dot that blinks
	CHECKPOINT_CHOOSABLE = 3   // a button that starts the stage
};

class GameMapCheckpoint : public engine::Container
{
public:
	GameMapCheckpoint();  // 0x422320

	// 0x421f80: the dot's images and its button are made for a stage, put at the stage's place on the map, and the
	// screen listens to the button under the action "mapBtn" with the stage's number
	void build(StageInfo* theStage, engine::ScreenLayout* theScreen);
	// 0x421d10: the dot shows one of the states
	void setState(int theState);
	// 0x421ea0: the blinking dot of the stage that loads changes between its two looks every fifth of a second
	void update(engine::UpdateInfo& theInfo) override;

private:
	int mState;                      // +0x11c
	engine::ImageNode* mChecked;     // +0x120: the checked dot
	engine::ImageNode* mOff;         // +0x124: the plain dot
	engine::ImageNode* mHigh;        // +0x128: the dot that is lit
	engine::Container* mDots;        // +0x12c: the three dots
	engine::ImageButton* mButton;    // +0x130: the dot that can be chosen
	float mBlinkSeconds;             // +0x134: the seconds until the blinking dot changes
	engine::ScreenLayout* mScreen;   // +0x138: the screen the button is listened to by; not counted
};

} // namespace feedingfrenzy

#endif
