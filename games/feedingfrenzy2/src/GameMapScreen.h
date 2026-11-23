#ifndef __FEEDINGFRENZY2_GAMEMAPSCREEN_H__
#define __FEEDINGFRENZY2_GAMEMAPSCREEN_H__

// The game's map (GameMapScreen, 0x1e0 bytes, vtable 0x55a19c (feedingfrenzy2); its constructor 0x49c170
// (feedingfrenzy2)), the application's +0x84, read from resource/shell/gameMapBase.xml: a checkpoint for each stage of
// the manifest at its pin, shown either for the player to pick a stage reached (each a hidden button whose action is
// "levelStart:<id>") or, while a stage loads, with the stages played checked and the current one pulsing. The stage
// under the mouse is named in the map's stageText.

#include "Container.h"
#include "GenericScreen.h"
#include "Image.h"
#include "ImageButton.h"
#include "StageManifest.h"
#include "TextItem.h"

#include <vector>

class GameMapScreen;

// What a checkpoint shows (its +0xdc)
enum CheckpointState
{
	CHECKPOINT_AHEAD = 0,      // not yet reached: the off image
	CHECKPOINT_PASSED = 1,     // played before the current stage: the on image
	CHECKPOINT_CURRENT = 2,    // the stage loading: the off image under the pulsing high image
	CHECKPOINT_SELECTABLE = 3  // reached, to be picked: the on image, the checked one while the mouse is over it,
	                           // and the hit box's button in place
};

// A stage's pin on the map (GameMapCheckpoint, 0xe8 bytes, vtable 0x556934 (feedingfrenzy2); its constructor 0x41f810
// (feedingfrenzy2)): four images of the shell theme's, one shown at a time (the high one over the off one), and an
// invisible button the size of the hit box that the map hears the clicks of
class GameMapCheckpoint : public engine::Container, public engine::IButtonListener
{
public:
	GameMapCheckpoint() : mState(CHECKPOINT_AHEAD) {}

	// 0x433e00 (feedingfrenzy2): the images made (the large pin's when asked), the high one pulsing, the button made
	// with the action and given to the map, and the checkpoint put at the pin, moved by the theme's checkpointOffset
	void setup(GameMapScreen* theMap, const std::string& theAction, bool isLarge, const engine::Vector2f& thePin);
	// 0x46c370 (feedingfrenzy2): the images for the state, and the button in place only when selectable
	void setState(int theState);

	// 0x00 of its button table (0x48d850 (feedingfrenzy2)): a selectable checkpoint shows its checked image
	void buttonEnter(const std::string& theAction) override;
	// 0x04 (0x48efa0 (feedingfrenzy2)): and its on image again
	void buttonLeave(const std::string& theAction) override;
	void buttonDown(const std::string& theAction) override {}
	void buttonUp(const std::string& theAction) override {}
	void buttonAction(const std::string& theAction) override {}

private:
	engine::Ref<engine::Image> mOnImage;        // +0xbc: checkpointOn
	engine::Ref<engine::Image> mOffImage;       // +0xc0: checkpointOff
	engine::Ref<engine::Image> mCheckedImage;   // +0xc4: checkpointChecked
	engine::Ref<engine::Image> mHighImage;      // +0xc8: checkpointHigh
	engine::Ref<engine::ImageButton> mButton;   // +0xcc: over checkpointHitBox
	engine::Vector2f mOffset;                   // +0xd0: the theme's checkpointOffset
	GameMapScreen* mMap;                        // +0xd8
	int mState;                                 // +0xdc: CheckpointState
};

class GameMapScreen : public engine::GenericScreen
{
public:
	GameMapScreen() : mStages(nullptr), mStageTextTime(0.0f) {}

	// 0x406200 (feedingfrenzy2), after the map's file is read: the stages kept, the checkpoints made in the map's
	// checkpointGrp, its texts, cancel button and load bar found, and the screen named
	void setup(StageManifest* theStages);
	// 0x478d90 (feedingfrenzy2): the map for a stage's load (the stage named, the stages before it passed and it
	// current), or to pick a stage of the mode reached (each selectable), the load bar emptied
	void showFor(bool isChoosing, int theMode);

	// 0xa4 (0x435620 (feedingfrenzy2)): the screen's, then once the stage's name has been left for its time, the
	// instructions back
	void update(engine::UpdateContext& theContext) override;
	// 0x00 of its button table (0x40eea0 (feedingfrenzy2)): the stage under the mouse named (by its intro's title,
	// "%d. %s", or by its id when it has none), or the cancel button's caption, in place of the instructions
	void buttonEnter(const std::string& theAction) override;
	// 0x04 (0x491800 (feedingfrenzy2)): the name left three quarters of a second (the cancel's a twentieth)
	void buttonLeave(const std::string& theAction) override;

	engine::Image* getLoadBar() { return mLoadBar; }

private:
	// 0x423ac0 (feedingfrenzy2): a checkpoint for each stage, the large pin's scaled by three quarters and placed
	// smoothly
	void makeCheckpoints();
	// 0x51698d (feedingfrenzy2)
	void clearCheckpoints();

	// +0x1a0: set to 0 by the setup, its readers not yet found
	StageManifest* mStages;                                  // +0x1a4: the game data's
	engine::Ref<engine::Container> mCheckpointGroup;         // +0x1a8: "checkpointGrp"
	std::vector<engine::Ref<GameMapCheckpoint>> mCheckpoints;  // +0x1ac
	engine::TextItem* mLoadText;                             // +0x1bc: "loadText"
	engine::TextItem* mNewGameText;                          // +0x1c0: "newGameText"
	engine::TextItem* mInstructText;                         // +0x1c4: "instructText"
	engine::TextItem* mStageText;                            // +0x1c8: "stageText"
	engine::Component* mCancelButton;                        // +0x1cc: "gameMapCancelBtn"
	engine::Image* mLoadBar;                                 // +0x1d0: "loadBar"
	float mStageTextTime;                                    // +0x1d4: the seconds the stage's name is left for, -1
	                                                         // while the mouse is on a button
};

#endif
