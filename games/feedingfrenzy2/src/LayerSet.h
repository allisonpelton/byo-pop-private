#ifndef __FEEDINGFRENZY2_LAYERSET_H__
#define __FEEDINGFRENZY2_LAYERSET_H__

// A stage's backdrop: a set of layers read from a set file (resource/backdrops/*.xml, named by the stage's file),
// each a screen layout panned by its own share of the camera's movement, so that the layers further back move less
// (LayerSet, 0x144 bytes, vtable 0x557c7c (feedingfrenzy2); its constructor 0x4892a0 (feedingfrenzy2)). The set is a
// container of its layers' layouts and overrides only its destructor. Each <layer> of the file is kept as an info
// (LayerInfo, by the element's type), and each info makes its layer (Layer).
//
// Only the layoutPan kind is translated; the panelScroll and propScroll kinds (PanelScrollInfo, PropScrollInfo and
// their layers) are not yet, and their layers are left out with a line in the log.

#include "Container.h"
#include "MasterDocHandler.h"
#include "Object.h"
#include "ScreenLayout.h"

#include <string>
#include <vector>

class Layer;

// What a <layer> says (LayerInfo, vtable 0x557db0 (feedingfrenzy2); its constructor 0x4898c0 (feedingfrenzy2))
class LayerInfo : public engine::Object
{
public:
	LayerInfo();

	// 0x04 (0x478650 (feedingfrenzy2)): color, colorMode, blur, panSize and scrollRate
	virtual void read(engine::Params& theAttributes);
	// 0x08: the layer this info makes, given the set's water level
	virtual Layer* createLayer(float theWaterLevel) = 0;

	// The colour laid over the layer's water, white until read (its alpha above 0 lays it)
	const engine::Color& getColor() const { return mColor; }
	int getColorMode() const { return mColorMode; }
	const engine::Vector2f& getPanSize() const { return mPanSize; }

private:
	engine::Color mColor;          // +0x0c
	float mBlur;                   // +0x1c: read only
	engine::Vector2f mPanSize;     // +0x20: the layer's size the camera pans over
	float mScrollRate;             // +0x28: read only (the scroll kinds use theirs)
	int mColorMode;                // +0x2c: LayerColorMode
};

// A colorMode's value (the colour's mode once the effect is translated)
enum LayerColorMode
{
	LAYER_COLOR_NONE = 0,
	LAYER_COLOR_ADD = 1,
	LAYER_COLOR_MULTIPLY = 2,
	LAYER_COLOR_BLEND = 3
};

// A layer of one screen layout (LayoutPanInfo, 0x54 bytes, vtable 0x557e04 (feedingfrenzy2); its constructor
// 0x489880 (feedingfrenzy2))
class LayoutPanInfo : public LayerInfo
{
public:
	// 0x04 (0x417680 (feedingfrenzy2)): the info's, and the layout's file
	void read(engine::Params& theAttributes) override;
	// 0x08 (0x40a0a0 (feedingfrenzy2))
	Layer* createLayer(float theWaterLevel) override;

	const std::string& getLayoutPath() const { return mLayoutPath; }

private:
	std::string mLayoutPath;       // +0x30
};

// A layer's handler (LayoutPanInfo::DocHandler, 0x5c bytes, vtable 0x557ac8 (feedingfrenzy2); its constructor
// 0x489900 (feedingfrenzy2))
class LayerInfoDocHandler : public engine::SubDocHandler
{
public:
	explicit LayerInfoDocHandler(LayerInfo* theInfo) : mInfo(theInfo) {}

	// 0x04 of its own table (0x473d20 (feedingfrenzy2)): the info reads the element
	void begin(engine::Params& theAttributes) override { mInfo->read(theAttributes); }
	void startElement(const std::string& theName, engine::Params& theAttributes) override {}
	void endElement(const std::string& theName) override {}

private:
	LayerInfo* mInfo;  // +0x50
};

// A layer of the set (Layer, vtable 0x5579b8 (feedingfrenzy2); its constructor 0x48a200 (feedingfrenzy2))
class Layer : public engine::Object
{
public:
	Layer() : mPanFactor(1.0f, 1.0f) {}

	// 0x04: made, given the set's water level
	virtual void init(float theWaterLevel) = 0;
	// 0x08: what the set shows
	virtual engine::Component* getComponent() = 0;
	// 0x0c, 0x10 and 0x14: the containers the stage's actors, players and overlays go in
	virtual engine::Container* getActors() = 0;
	virtual engine::Container* getPlayers() = 0;
	virtual engine::Container* getOverlays() = 0;
	// 0x18: where it is
	virtual engine::Vector2f getPosition() = 0;
	// 0x1c: moved for the camera at the position
	virtual void pan(const engine::Vector2f& theCameraPosition) = 0;

	// 0x4858d0 (feedingfrenzy2) sets it: the share of the camera's movement the layer follows
	void setPanFactor(const engine::Vector2f& theFactor) { mPanFactor = theFactor; }

protected:
	engine::Vector2f mPanFactor;   // +0x0c
};

// A layer that is one screen layout (LayoutPan, 0x34 bytes, vtable 0x5579f0 (feedingfrenzy2); its constructor
// 0x40a0f0 (feedingfrenzy2))
class LayoutPan : public Layer
{
public:
	// 0x480e90 (feedingfrenzy2): the info kept, the three containers made and named, and the layer made
	LayoutPan(LayoutPanInfo* theInfo, float theWaterLevel);

	// 0x04 (0x411050 (feedingfrenzy2)): the layout read from the info's file (one read before taken out first), the
	// info's colour laid over its water when the colour shows, and the three containers put on top
	void init(float theWaterLevel) override;
	// 0x08 (0x48e160 (feedingfrenzy2))
	engine::Component* getComponent() override { return mLayout.get(); }
	// 0x0c (0x50a8d6), 0x10 (0x50a8d2) and 0x14 (0x5015ab) (feedingfrenzy2)
	engine::Container* getActors() override { return mActors.get(); }
	engine::Container* getPlayers() override { return mPlayers.get(); }
	engine::Container* getOverlays() override { return mOverlays.get(); }
	// 0x18 (0x50a8b8 (feedingfrenzy2)): the layout's position
	engine::Vector2f getPosition() override { return mLayout->getPosition(); }
	// 0x1c (0x43c090 (feedingfrenzy2)): the layout at the camera's position times the factors, the other way
	void pan(const engine::Vector2f& theCameraPosition) override;

private:
	engine::Ref<LayoutPanInfo> mInfo;              // +0x18
	engine::Ref<engine::ScreenLayout> mLayout;     // +0x1c
	engine::Ref<engine::Container> mActors;        // +0x20: "PanLayerActors"
	engine::Ref<engine::Container> mPlayers;       // +0x24: "PanLayerPlayers"
	engine::Ref<engine::Container> mOverlays;      // +0x28: "PanLayerOverlays"
};

class LayerSet : public engine::Container
{
public:
	LayerSet();

	// 0x40a4e0 (feedingfrenzy2): the set's file read; read, each info's layer made at the set's water level and its
	// layout added
	void load(const std::string& thePath);
	// 0x4858d0 (feedingfrenzy2): the camera's limits (the bounds less the margins) and the base layer's range, and
	// each layer's share of it from its pan size (all of it where the range is none)
	void fit(float theMarginX, float theMarginY, const engine::RectF& theBounds);
	// 0x434b30 (feedingfrenzy2): the camera at the point, kept in the pan range, and each layer panned for it
	void setCamera(const engine::Vector2f& thePosition);
	// 0x43b960 (feedingfrenzy2): the camera moved towards the target (a point on the display) by what lies outside the
	// camera's limits, at most 12 times that distance plus 150 a second, held in the pan range, and each layer panned
	void followCamera(const engine::Vector2f& theTarget, float theElapsed);

	int getBaseLayer() const { return mBaseLayer; }
	int getWaterLevel() const { return mWaterLevel; }
	int getSkyCeiling() const { return mSkyCeiling; }
	int getLayerCount() const { return (int)mLayers.size(); }
	Layer* getLayer(int theIndex) { return mLayers[theIndex].get(); }
	LayerInfo* getInfo(int theIndex) { return mInfos[theIndex].get(); }
	// 0x50a65d (feedingfrenzy2): the layer's position, of those from the base layer on, less the base layer's
	engine::Vector2f getLayerOffset(int theLayer);
	float getNightColorLightPct() const { return mNightColorLightPct; }
	const engine::Color& getNightColorLight() const { return mNightColorLight; }
	const engine::Color& getNightColorDark() const { return mNightColorDark; }
	const engine::RectF& getCameraLimits() const { return mCameraLimits; }
	const engine::RectF& getPanRange() const { return mPanRange; }
	const engine::Vector2f& getPanPosition() const { return mPanPosition; }

private:
	friend class LayerSetDocHandler;

	// The camera's speed in following: per unit of distance outside its limits (0x5669c0 (feedingfrenzy2)), and
	// beyond that (0x5669b8 (feedingfrenzy2))
	static constexpr double FOLLOW_SPEED_PER_DISTANCE = 12.0;
	static constexpr double FOLLOW_SPEED = 150.0;

	std::vector<engine::Ref<LayerInfo>> mInfos;    // +0xb8
	std::vector<engine::Ref<Layer>> mLayers;       // +0xc8
	int mBaseLayer;                                // +0xd8: the layer the camera's range is the base layer's
	int mWaterLevel;                               // +0xdc
	int mSkyCeiling;                               // +0xe0
	engine::RectF mCameraLimits;                   // +0xe4
	engine::Vector2f mCameraPosition;              // +0xf4: (0, 0) once fitted
	engine::RectF mPanRange;                       // +0x104: from (0, 0) to the base layer's pan size less the view
	engine::Vector2f mPanPosition;                 // +0xfc: the camera's position the layers are panned for
	// +0x114, a float at 0: from the constructor, and read by nothing translated yet: left out
	engine::Color mNightColorLight;                // +0x118
	engine::Color mNightColorDark;                 // +0x128
	float mNightColorLightPct;                     // +0x138
};

// The set's handler (LayerSet::DocHandler, 0x5c bytes, vtable 0x557dd0 (feedingfrenzy2); its constructor 0x494650
// (feedingfrenzy2))
class LayerSetDocHandler : public engine::SubDocHandler
{
public:
	explicit LayerSetDocHandler(LayerSet* theSet) : mSet(theSet) {}

	// 0x04 of its own table (0x414030 (feedingfrenzy2)): baseLayer, waterLevel, skyCeiling and the night's colours
	void begin(engine::Params& theAttributes) override;
	// 0x00 (0x4193b0 (feedingfrenzy2)): a <layer> kept as the info of its type, its handler reading it
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	void endElement(const std::string& theName) override {}

private:
	LayerSet* mSet;  // +0x50
};

#endif
