#ifndef __FEEDINGFRENZY_STAGEMGR_H__
#define __FEEDINGFRENZY_STAGEMGR_H__

// A stage (StageMgr, constructor 0x43bcc0, vtable 0x4a7060): the container that holds a stage's backgrounds, its fish
// and the player, and plays it.

#include "Geometry.h"
#include "Node.h"
#include <vector>

namespace feedingfrenzy
{

class FishBase;
class FishSchool;
class PlayerFish;

class StageMgr : public engine::Container
{
public:
	// 0x1b0 and 0x1b4: 0x42bc30 and 0x437fb0: the size of the stage the fish swim in
	int getWorldWidth() const { return mWorldWidth; }
	int getWorldHeight() const { return mWorldHeight; }
	// 0x1b8 and 0x1bc: 0x42be10 and 0x437fc0: the size of the view of it that shows
	int getScreenWidth() const { return mViewWidth; }
	int getScreenHeight() const { return mViewHeight; }
	// 0x1c0: 0x436700: where a place of the stage shows on the screen; 0x1c4: 0x4367a0: the reverse
	engine::Point worldToScreen(const engine::Point& thePoint) const;
	engine::Point screenToWorld(const engine::Point& thePoint) const;
	// 0x1a8: 0x436900: the fish's rectangle overlaps the stage; 0x1ac: 0x4369c0: it overlaps the view
	bool isFishInStage(FishBase* theFish);
	bool isFishOnScreen(FishBase* theFish);
	// 0x43ccc0: a fish joins the stage's list and the group of its layer (0 front, 1 middle, 2 back)
	void addFish(FishBase* theFish, int theLayer);
	// 0x43ce10 and 0x4385b0: a school joins the stage's list, and leaves it
	void addSchool(FishSchool* theSchool);
	void removeSchool(FishSchool* theSchool);
	PlayerFish* getPlayer() { return mPlayer; }                          // 0x436fe0
	const std::vector<FishBase*>& getFishList() const { return mFish; }  // 0x436fd0

private:
	engine::Ref<PlayerFish> mPlayer;  // +0x128
	engine::Ref<engine::Container> mFrontGroup;       // +0x134
	engine::Ref<engine::Container> mMiddleGroup;      // +0x138
	engine::Ref<engine::Container> mBackGroup;        // +0x13c
	std::vector<FishBase*> mFish;      // +0x190
	std::vector<FishSchool*> mSchools; // +0x1a4
	int mWorldWidth;                   // +0x1cc
	int mWorldHeight;                  // +0x1d0
	float mCameraX;                    // +0x1dc
	float mCameraY;                    // +0x1e0
	int mViewWidth;                    // +0x1e4
	int mViewHeight;                   // +0x1e8
	float mVerticalOffset;             // +0x2c0
};

} // namespace feedingfrenzy

#endif
