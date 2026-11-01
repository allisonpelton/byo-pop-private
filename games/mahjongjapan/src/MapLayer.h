#ifndef __MAHJONGJAPAN_MAPLAYER_H__
#define __MAHJONGJAPAN_MAPLAYER_H__

// The escape's map (0x410470): the stages and their levels, and the path walked

#include "Dialog.h"
#include "Layer.h"

namespace mahjongjapan
{

class PlayLayer;

// 0x410ef0: a level's stage, its place in the stage and the stage's levels
void GetStage(int theLevel, int* theStage, int* theIndex, int* theCount);
// 0x4110b0: the stage of the escape's progress
int GetProgressStage();

// 0x410470 (vtable 0x51d468, 0x430 bytes)
class MapLayer : public jb::Layer, public DialogListener
{
public:
	PlayLayer* mPlay;  // +0x58

	// 0x410470
	explicit MapLayer(PlayLayer* thePlay);
	~MapLayer() override;

	void Load() override;
	bool OnCommand(int theCmd) override;
	bool OnYes(int theId) override;
	bool OnNo(int theId) override;

	// 0x4105b0
	void SetProgressed(bool theProgressed);
};

} // namespace mahjongjapan

#endif
