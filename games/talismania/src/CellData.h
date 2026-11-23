#ifndef __TALISMANIA_CELLDATA_H__
#define __TALISMANIA_CELLDATA_H__

// The cells the data sets describe ("cellclass", CellClass, loader 0x5a9a10, its layers read by 0x5ac1c0, its sounds by
// 0x5ae5e0 and its energize map by the readers it registers): what a cell is made of (the layers it draws, shown by view
// modes), how it behaves (paths, falling, rotating, switching), the sounds it makes and what it turns into and pays out
// when energized. A class that names a baseclass starts as a copy of it.

#include "Geometry.h"

#include <map>
#include <string>
#include <vector>

// How a layer answers a click on its cell (the layer's onclick, 0x5ac1c0)
enum LayerClickMode
{
	CLICK_NOTHING = 0,
	CLICK_ROTATE = 1,
	CLICK_ROTATE_SNAP = 2,
	CLICK_ANIMATE = 3  // the loader's fourth value; the data's only other word for it is "animate"
};

// <layer> (0x5ac1c0): one picture or animation of a cell, drawn while the cell is in one of its view modes
struct CellLayerClass
{
	std::string name;
	std::string resource;
	std::string anim;
	int animMode = 0;  // 2 loops, 1 shuttles, 0 plays once
	float autoHideTime = 0.0f;
	std::vector<std::string> viewGroupsOnHide;
	std::vector<std::string> fx;
	bool overlayFx = true;
	bool container = false;  // the layer holds the cell's path or sparkle or reward, not a picture of its own
	float opacity = 1.0f;
	float scale = 1.0f;
	engine::Vector2f offset;
	float rotation = 0.0f;  // degrees, as the data gives it
	float delay = -1.0f;
	bool overwriteInPlace = false;
	std::string insertAfter;
	std::vector<std::string> revealSounds;
	engine::FloatRange randOffsetX;
	engine::FloatRange randOffsetY;
	bool noRandOnZeroth = false;
	engine::Vector2f randScaleMult = engine::Vector2f(1.0f, 1.0f);
	bool trackPos = false;
	float rotSpeed = 0.0f;  // radians a second (the data gives degrees)
	float screenShakePulseMag = 0.0f;
	float screenShakePulseFalloff = 1.0e5f;
	float scalePulseGrowSpeed = 1.0e4f;
	float scalePulseMag = 1.0f;  // the layer's scale when not given
	float scalePulseSpeedMult = 1.0e4f;
	float scalePulseMinSpeed = 1.0e4f;
	float scalePulseStartScale = 1.0f;  // the layer's scale when not given
	bool plmSpecial = false;
	std::vector<std::string> viewGroups;
	bool viewGroupCombineAnd = false;  // "and" needs every group, anything else any group
	int onClick = CLICK_NOTHING;
	std::vector<std::string> viewModes;
	std::vector<std::string> viewAntiModes;
	bool smooth = false;
};

// <soundevent> (0x5ae5e0): a sound a cell plays on one of its events
struct CellSoundEvent
{
	std::string onEvent;
	std::string playSound;
	engine::FloatRange pitch = engine::FloatRange(1.0f, 1.0f);
	float volume = 1.0f;
};

// <energizeparms>: what a cell turns into when a path of one colour energizes it
struct EnergizeParms
{
	std::string energizeColour;
	std::string explodeFx;
	std::string switchToOnPop;
	std::string reward;
	std::string extraReward;
};

// "cellclass" (CellClass)
struct CellClass
{
	std::string name;
	std::string baseClass;
	std::string type;
	bool canHostPaths = false;
	bool pathVisible = true;
	std::string explicitPathId = "invalid";
	std::string explicitPathClass = "normal";
	bool indestructable = false;
	bool noRotate = false;
	std::string unlockProcessor = "default";
	std::string previewPathImageClass = "preview";
	std::string sparklePathImageClass = "sparkle";
	int endpointRank = 0;
	std::string cloneSpawn;
	bool scaleUnderPathOnMouseOver = true;
	bool scaleOverPathOnMouseOver = true;
	engine::FloatRange timeBombTimeRange = engine::FloatRange(0.1f, 0.1f);
	std::string explodeFx = "regCellPop";
	bool cellSwitchAllowed = false;
	float rotateSpeed = 6.2831853f;  // radians a second
	float fastRotateSpeed = 12.566371f;
	bool advancesPathMeter = true;
	std::string midasSpawnEvent;
	std::string midasPopEvent;
	std::string switchFx = "bonusFx";
	std::string tooltip;
	bool canFall = true;
	engine::FloatRange fallAccel = engine::FloatRange(720.0f, 1080.0f);
	engine::FloatRange fallStartSpeed = engine::FloatRange(48.0f, 72.0f);
	bool minotaurCrushable = true;
	engine::FloatRange lifetime = engine::FloatRange(6.3072e7f, 6.3072e7f);
	bool limitPathIds = true;
	std::string betaNotifyTag;
	std::string frenzyReward = "frenzyreward";
	bool notifyMouseCursorMgr = true;
	bool countInPathLength = true;
	bool canMedusaZap = true;
	std::string switchToOnPop;
	std::string pathTags;
	std::string spawnAvoidClasses;
	std::vector<CellLayerClass> layers;
	std::vector<CellSoundEvent> sounds;
	std::vector<EnergizeParms> energizeMap;
};

#endif
