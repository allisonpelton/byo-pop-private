#ifndef __FEEDINGFRENZY2_STAGE_H__
#define __FEEDINGFRENZY2_STAGE_H__

// The game's world (Stage, 0x3e0 bytes, vtable 0x5585c4 (feedingfrenzy2); its constructor 0x403aa0 (feedingfrenzy2)):
// a stage's file read (resource/Stages/*.xml, named by a manifest's entry), its backdrop's layers, its fish and its
// player, under the game's screens. The menu's backgrounds are stages too (the menu manifest's).
//
// Translated so far: the stage's file's attributes, and the backdrop: the play bounds, the mask container over the
// layers and the layer set. Each untranslated step says so where it would run.

#include "Container.h"
#include "LayerSet.h"
#include "MasterDocHandler.h"
#include "StageManifest.h"
#include "Theme.h"

#include <string>

class GameData;

// The container the stage's layers are drawn in, darkened through a mask on a night stage (MaskContainer, 0x1a8
// bytes, vtable for its Interface overriding the destructor, 0x90, 0x94, 0xa0 and 0xa4 (feedingfrenzy2); its
// constructor 0x409200 (feedingfrenzy2))
class MaskContainer : public engine::Container
{
public:
	MaskContainer() : mMaskOn(false) {}

	// 0x90 (0x51609a (feedingfrenzy2)): the mask brought in, once
	void enable() override;
	// 0xa0 (0x44f1a0 (feedingfrenzy2)): the container's own while the mask is off and not fading
	void paint(engine::DrawContext* theContext) override;

	// The stage's slot 13 sets it directly (a night stage starts with the mask on)
	void setMaskOn(bool theMaskOn) { mMaskOn = theMaskOn; }

private:
	bool mMaskOn;   // +0xc4
	// +0x17c, the mask's fade: not yet translated (0, none, until it is)
};

class Stage : public engine::Object
{
public:
	// 0x403aa0 (feedingfrenzy2): the game data and the container the stage goes in kept, and its init (0x4038d0
	// (feedingfrenzy2))
	Stage(GameData* theGameData, engine::Container* theContainer);

	// 0x04 (0x410980 (feedingfrenzy2)): the entry's file read and the stage set up from it; false when it cannot be
	// read
	virtual bool load(const StageManifest::Stage& theEntry);

	// 0x34 (0x4146b0 (feedingfrenzy2)): the play bounds, the mask container and the layer set
	virtual void setupLayers();

	LayerSet* getLayerSet() { return mLayerSet.get(); }
	const engine::RectF& getBounds() const { return mBounds; }

private:
	friend class StageDocHandler;

	engine::Ref<GameData> mGameData;               // +0x68
	engine::RectF mBounds;                         // +0x6c: the play bounds, the display's less the HUD's band
	StageManifest::Stage mEntry;                   // +0x1c: the manifest's entry, its file at +0x38
	float mScrollRate;                             // +0x184
	float mSizeUnit;                               // +0x188
	bool mBonus;                                   // +0x18c
	engine::Color mWaterColor;                     // +0x1a0
	std::string mSetPath;                          // +0x1b0: the backdrop's set file
	std::string mAmbientSound;                     // +0x1d4
	std::string mGameMusic;                        // +0x1f0
	engine::Ref<engine::Theme> mTheme;             // +0x2d4: resource/themes/baseTheme.xml
	engine::Ref<LayerSet> mLayerSet;               // +0x2d8
	engine::Ref<engine::Container> mContainer;     // +0x2dc: the application's stage container
	engine::Ref<MaskContainer> mMaskContainer;     // +0x2ec
	bool mNight;                                   // +0x2f0
	float mLight;                                  // +0x2f4: the night's light now, from minLight
	float mLightTarget;                            // +0x2f8
	float mMinLight;                               // +0x2fc
	float mMaxLight;                               // +0x300
	bool mBoss;                                    // +0x392
	bool mFinalBoss;                               // +0x393
	std::string mBossLayout;                       // +0x394
};

// The stage file's handler (Stage::DocHandler, its begin 0x47c730 (feedingfrenzy2))
class StageDocHandler : public engine::SubDocHandler
{
public:
	explicit StageDocHandler(Stage* theStage) : mStage(theStage) {}

	// 0x04 of its own table (0x47c730 (feedingfrenzy2)): the <stage> element's attributes
	void begin(engine::Params& theAttributes) override;
	// The <spawn> elements and their phases: not yet translated
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	void endElement(const std::string& theName) override {}

private:
	Stage* mStage;  // +0x58
};

#endif
