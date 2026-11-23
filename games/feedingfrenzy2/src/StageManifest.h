#ifndef __FEEDINGFRENZY2_STAGEMANIFEST_H__
#define __FEEDINGFRENZY2_STAGEMANIFEST_H__

// A list of stages read from a manifest file (StageManifest, 0x34 bytes, vtable 0x55affc (feedingfrenzy2); its
// constructor 0x4a0060 (feedingfrenzy2)): the game's (resource/Stages/stageManifest.xml) or the menu's backgrounds
// (menuManifest.xml, or the screensaver's saverManifest.xml). A <stage> names a stage's file and its times; a
// <levelset> starts a group of the stages after it on the game map, with its buttons' images and the spline their
// pins follow. Its handler (StageManifest::DocHandler, 0x5c bytes, vtable 0x558988 (feedingfrenzy2)) reads the
// elements under <stageManifest>.
//
// The original's manifest is an engine object; the game data holds two as members, so the port's is a plain class.

#include "Geometry.h"
#include "MasterDocHandler.h"
#include "SplinePath.h"

#include <string>
#include <vector>

class StageManifest
{
public:
	// A <stage> (0x4c bytes)
	struct Stage
	{
		std::string mId;              // +0x00: "NO_ID" when the element has none
		std::string mResource;        // +0x1c: the stage's file
		engine::Vector2f mMapPos;     // +0x38: its pin on the game map
		int mStageTime;               // +0x40: the stage's seconds
		int mNewGameTime;             // +0x44
		bool mCanSuck;                // +0x48: true unless the element says false
		bool mLargePin;               // +0x49

		Stage() : mMapPos(0, 0), mStageTime(0), mNewGameTime(0), mCanSuck(true), mLargePin(false) {}
	};

	// A <levelset> (0x48 bytes)
	struct LevelSet
	{
		int mFirstStage;              // +0x00: the index of its first stage (the count of those read before it)
		engine::Vector2f mStart;      // +0x04: where its spline starts on the map (set with the pins)
		std::string mImageOff;        // +0x0c: its stages' buttons
		std::string mImageOn;         // +0x28
		engine::Ref<SplinePath> mSpline;  // +0x44: its stages' pins are spaced along it; null for none

		LevelSet() : mFirstStage(0), mStart(0, 0) {}
	};

	// 0x48c290 (feedingfrenzy2): the file read; when it has level sets, their stages' pins placed along their
	// splines
	void load(const std::string& thePath);

	const std::vector<Stage>& getStages() const { return mStages; }
	// 0x483b90 (feedingfrenzy2): the place of the stage of the id, -1 for none
	int findStage(const std::string& theId) const;
	const std::vector<LevelSet>& getLevelSets() const { return mLevelSets; }

private:
	friend class StageManifestDocHandler;

	// 0x41ad50 (feedingfrenzy2): each level set but the last starts at its spline's first node, and its stages' pins
	// are spaced evenly along it, none at its ends; the last set, which has no stages, starts at the end of the
	// spline before it
	void placePins();

	std::vector<Stage> mStages;        // +0x0c
	std::vector<LevelSet> mLevelSets;  // +0x1c
};

// StageManifest::DocHandler
class StageManifestDocHandler : public engine::SubDocHandler
{
public:
	explicit StageManifestDocHandler(StageManifest* theManifest) : mManifest(theManifest) {}

	// 0x04 of its own table (0x5014fa (feedingfrenzy2)): nothing
	void begin(engine::Params& theAttributes) override {}
	// 0x00 (0x461fd0 (feedingfrenzy2)): a <stage> or a <levelset> added
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x5014fa (feedingfrenzy2)): nothing
	void endElement(const std::string& theName) override {}

private:
	StageManifest* mManifest;  // +0x0c (of the handler's base)
};

#endif
