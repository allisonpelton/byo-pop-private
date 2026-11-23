#ifndef __FEEDINGFRENZY_STAGEINFO_H__
#define __FEEDINGFRENZY_STAGEINFO_H__

// A stage of the game as the stage manifest lists it (StageInfo, 0x4c bytes, constructor 0x435e00, vtable 0x4a6fa4): its
// name, the folder of its files (config\stages\<path>), its number from 1 and its place on the map of the stages.
// The manifest (config\stages\stageManifest.xml, read by 0x4360c0) is the list of the stages in order, with
// the player fish's changes (fishChange: the fish from the stages that follow, up to the next change) between them.

#include "Geometry.h"

#include <memory>
#include <string>
#include <vector>

namespace feedingfrenzy
{

class StageInfo
{
public:
	StageInfo() : mBeaten(false), mUnlocked(false), mIndex(0) {}  // 0x435e00

	void setName(const std::string& theName) { mName = theName; }  // 0x00: 0x435ed0
	const std::string& getName() const { return mName; }           // 0x04: 0x435e70 (+0x04)
	void setPath(const std::string& thePath) { mPath = thePath; }  // 0x08: 0x435f30
	const std::string& getPath() const { return mPath; }           // 0x0c: 0x435ea0 (+0x20)
	void setBeaten(bool theBeaten) { mBeaten = theBeaten; }        // 0x10: 0x435d80 (+0x3c)
	bool isBeaten() const { return mBeaten; }                      // 0x14: 0x435d90
	void setUnlocked(bool theUnlocked) { mUnlocked = theUnlocked; }  // 0x18: 0x435da0 (+0x3d)
	bool isUnlocked() const { return mUnlocked; }                  // 0x1c: 0x435db0
	void setIndex(int theIndex) { mIndex = theIndex; }             // 0x20: 0x435dc0 (+0x40)
	int getIndex() const { return mIndex; }                        // 0x24: 0x435dd0
	void setMapPosition(const engine::Point& thePosition) { mMapPosition = thePosition; }  // 0x28: 0x435de0 (+0x44)
	const engine::Point& getMapPosition() const { return mMapPosition; }                   // 0x2c: 0x435df0

private:
	std::string mName;           // +0x04
	std::string mPath;           // +0x20
	bool mBeaten;                // +0x3c
	bool mUnlocked;              // +0x3d
	int mIndex;                  // +0x40: from 1
	engine::Point mMapPosition;  // +0x44
};

// A change of the player's fish in the manifest (0x24 bytes, built by 0x431490): the fish's name and the first and last
// stage (counting from 0) it plays
struct FishChange
{
	std::string mName;  // +0x00
	int mFirstStage;    // +0x1c
	int mLastStage;     // +0x20
};

// The manifest's lists (the controller's +0x1ec and +0x1fc)
struct StageManifest
{
	std::string mVersion;  // gameInfo: the controller's +0x214 ("NO VERSION" until it is read)
	std::vector<std::unique_ptr<StageInfo>> mStages;
	std::vector<FishChange> mFishChanges;
};

// 0x42c860: the stage manifest file read into the lists (the handler is 0x4360c0)
void loadStageManifest(const std::string& theFile, StageManifest& theManifest);

} // namespace feedingfrenzy

#endif
