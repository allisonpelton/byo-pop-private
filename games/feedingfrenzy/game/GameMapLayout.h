#ifndef __FEEDINGFRENZY_GAMEMAPLAYOUT_H__
#define __FEEDINGFRENZY_GAMEMAPLAYOUT_H__

// The screen with the map of the stages (GameMapLayout, 0x16c bytes, constructor 0x423bd0). The
// controller keeps two: the map the player chooses a way on when a new game starts, and the one shown while a stage's
// files load (it names the stage that is next and shows how far the load has got).

#include "ScreenLayout.h"

#include <vector>

namespace engine
{
class Container;
}

namespace feedingfrenzy
{

class GameMapCheckpoint;
class GrowMeter;

class GameMapLayout : public engine::ScreenLayout
{
public:
	GameMapLayout();  // 0x423bd0

	// slot 0xfc: 0x423820: the layout is read and the stages' dots and the loading bar are made, for the map that is
	// chosen on (true) or the one shown while loading (false)
	virtual void setIsMap(bool theIsMap);
	// slot 0x104: 0x423210: the screen is made ready to show: the stages' dots and the texts, and the fish of the map
	virtual void prepare();
	// slot 0x108: 0x422b90: the stage's files are loaded (true) or not
	virtual void setLoaded(bool theLoaded);

	// 0x7c: 0x4223a0: the loading bar shows how far the stage's files have loaded
	void update(engine::UpdateInfo& theInfo) override;

	// The button listener (0x4a5268, at +0x138): the name of the stage the cursor is on shows, and a press on a stage
	// of the map starts it
	void buttonEnter(std::string theName) override;  // 0x422cf0
	void buttonPress(std::string theName) override;  // 0x422ec0
	void buttonExit(std::string theName) override;   // 0x4224e0

private:
	// 0x422700 (slot 0x10c): the map asks for a stage to be chosen
	void showChooseHeader();
	// 0x422960 (slot 0x110): the loading map names the stage that is next
	void showNextStage();
	// 0x422470 (slot 0x114): the stages that can be chosen get their buttons
	void showChoosableStages();
	// 0x422400 (slot 0x118): the stages are marked as beaten, the one that loads and the ones to come
	void showStageProgress();
	// 0x423030: the picture of a fish of the map shows in colour when the fish can be played and dark when it cannot
	// (theFishIndex is the fish's place in the manifest's list of changes); its button is taken off
	void showFish(int theFishIndex, std::string theName);
	// 0x422660: a node is hidden and takes no more of the mouse
	void hideButton(const std::string& theName);

	std::vector<GameMapCheckpoint*> mCheckpoints;  // +0x150: the stages' dots, in the manifest's order
	GrowMeter* mLoadBar;                           // +0x15c: the loading bar, in the node "loadBarGroup"
	bool mIsMap;                                   // +0x160
};

} // namespace feedingfrenzy

#endif
