#include "CellDataReader.h"

#include "Engine.h"

namespace
{

// The comma list of an attribute (0x5a3990 and 0x425ed0): the attribute's tokens, empty when it is not there
std::vector<std::string> listOf(const engine::Params& theParams, const char* theName)
{
	if (!theParams.has(theName))
		return std::vector<std::string>();
	return splitList(theParams.getString(theName, ""));
}

// The elements inside a cell class the data files use that are not yet translated
bool isUntranslatedChild(const std::string& theName)
{
	return theName == "giveactions" || theName == "prepareactions" || theName == "action" || theName == "gamestats";
}

const float DEGREES_TO_RADIANS = 0.01745328f;  // the constant the layer reader multiplies rotspeed by

} // namespace

bool CellDataReader::startElement(const std::string& theName, const engine::Params& p)
{
	if (theName == "cellclass")
	{
		beginCell(p);
		mInCell = true;
		return true;
	}
	if (!mInCell)
		return false;
	if (theName == "layers")
	{
		// 0x5abf10: the layers the base class gave are dropped when asked
		if (p.getBool("clearbaselayers", false))
			mCell.layers.clear();
	}
	else if (theName == "layer")
		mCell.layers.push_back(readLayer(p));
	else if (theName == "sounds")
		mCell.sounds.clear();
	else if (theName == "soundevent")
	{
		CellSoundEvent anEvent;
		anEvent.onEvent = p.getString("onevent", "");
		anEvent.playSound = p.getString("playsound", "");
		anEvent.pitch = p.getRange("pitch", engine::FloatRange(1.0f, 1.0f));
		anEvent.volume = p.getFloat("volume", 1.0f);
		mCell.sounds.push_back(anEvent);
	}
	else if (theName == "energizemap")
		mCell.energizeMap.clear();
	else if (theName == "energizeparms")
	{
		EnergizeParms aParms;
		aParms.energizeColour = p.getString("energizecolour", "");
		aParms.explodeFx = p.getString("explodefx", "");
		aParms.switchToOnPop = p.getString("switchtoonpop", "");
		aParms.reward = p.getString("reward", "");
		aParms.extraReward = p.getString("extrareward", "");
		mCell.energizeMap.push_back(aParms);
	}
	else if (isUntranslatedChild(theName))
	{
		if (mReported.insert(theName).second)
			engine::getEngine()->log("Cell class element \"%s\" not yet translated\n", theName.c_str());
	}
	else
		return false;
	return true;
}

bool CellDataReader::endElement(const std::string& theName)
{
	if (theName == "cellclass")
	{
		mData.cellClasses[mCell.name] = mCell;
		mInCell = false;
		return true;
	}
	if (!mInCell)
		return false;
	return theName == "layers" || theName == "layer" || theName == "sounds" || theName == "soundevent" ||
		   theName == "energizemap" || theName == "energizeparms" || isUntranslatedChild(theName);
}

// 0x5a9a10
void CellDataReader::beginCell(const engine::Params& p)
{
	std::string aName = p.getString("name", "");
	mCell = CellClass();
	std::string aBase = p.getString("baseclass", "");
	if (!aBase.empty())
	{
		std::map<std::string, CellClass>::iterator aBaseIt = mData.cellClasses.find(aBase);
		if (aBaseIt == mData.cellClasses.end())
			engine::getEngine()->log("CELL CLASS %s CANNOT FIND BASE CLASS %s\n", aName.c_str(), aBase.c_str());
		else
			mCell = aBaseIt->second;
	}
	CellClass& c = mCell;
	c.name = aName;
	c.baseClass = aBase;
	c.type = p.getString("type", c.type);
	c.canHostPaths = p.getBool("canhostpaths", c.canHostPaths);
	c.pathVisible = p.getBool("pathvisible", c.pathVisible);
	c.explicitPathId = p.getString("explicitpathid", c.explicitPathId);
	c.explicitPathClass = p.getString("explicitpathclass", c.explicitPathClass);
	c.indestructable = p.getBool("indestructable", c.indestructable);
	c.noRotate = p.getBool("norotate", c.noRotate);
	c.unlockProcessor = p.getString("unlockprocessor", c.unlockProcessor);
	c.previewPathImageClass = p.getString("previewpathimageclass", c.previewPathImageClass);
	c.sparklePathImageClass = p.getString("sparklepathimageclass", c.sparklePathImageClass);
	c.endpointRank = p.getInt("endpointrank", c.endpointRank);
	c.cloneSpawn = p.getString("clonespawn", "");  // the loader's own, not kept from the base class
	c.scaleUnderPathOnMouseOver = p.getBool("scaleunderpathonmouseover", c.scaleUnderPathOnMouseOver);
	c.scaleOverPathOnMouseOver = p.getBool("scaleoverpathonmouseover", c.scaleOverPathOnMouseOver);
	c.timeBombTimeRange = p.getRange("timebombtimerange", c.timeBombTimeRange);
	c.explodeFx = p.getString("explodefx", c.explodeFx);
	c.cellSwitchAllowed = p.getBool("cellswitchallowed", c.cellSwitchAllowed);
	c.rotateSpeed = p.getFloat("rotatespeed", c.rotateSpeed);
	// The fast speed is twice the slow one unless given: its default is the speed just read, doubled
	c.fastRotateSpeed = p.getFloat("fastrotatespeed", p.has("rotatespeed") ? c.rotateSpeed * 2.0f : c.fastRotateSpeed);
	c.advancesPathMeter = p.getBool("advancespathmeter", c.advancesPathMeter);
	c.midasSpawnEvent = p.getString("midasspawnevent", c.midasSpawnEvent);
	c.midasPopEvent = p.getString("midaspopevent", c.midasPopEvent);
	c.switchFx = p.getString("switchfx", c.switchFx);
	c.tooltip = p.getString("tooltip", c.tooltip);
	c.canFall = p.getBool("canfall", c.canFall);
	c.fallAccel = p.getRange("fallaccel", c.fallAccel);
	c.fallStartSpeed = p.getRange("fallstartspeed", c.fallStartSpeed);
	c.minotaurCrushable = p.getBool("minotaurcrushable", c.minotaurCrushable);
	c.lifetime = p.getRange("lifetime", c.lifetime);
	c.limitPathIds = p.getBool("limitpathids", c.limitPathIds);
	c.betaNotifyTag = p.getString("betanotifytag", c.betaNotifyTag);
	c.frenzyReward = p.getString("frenzyreward", c.frenzyReward);
	c.notifyMouseCursorMgr = p.getBool("notifymousecursormgr", c.notifyMouseCursorMgr);
	c.countInPathLength = p.getBool("countinpathlength", c.countInPathLength);
	c.canMedusaZap = p.getBool("canmedusazap", c.canMedusaZap);
	c.switchToOnPop = p.getString("switchtoonpop", c.switchToOnPop);
	c.pathTags = p.getString("pathtags", c.pathTags);
	c.spawnAvoidClasses = p.getString("spawnavoidclasses", c.spawnAvoidClasses);
}

// 0x5ac1c0
CellLayerClass CellDataReader::readLayer(const engine::Params& p) const
{
	CellLayerClass l;
	l.name = p.getString("name", "");
	l.resource = p.getString("resource", "");
	l.anim = p.getString("anim", "");
	std::string anAnimMode = p.getString("animmode", "");
	l.animMode = anAnimMode == "loop" ? 2 : anAnimMode == "shuttle" ? 1 : 0;
	l.autoHideTime = p.getFloat("autohidetime", 0.0f);
	l.viewGroupsOnHide = listOf(p, "viewgroupsonhide");
	l.fx = listOf(p, "fx");
	l.overlayFx = p.getBool("overlayfx", true);
	// A layer holds the path, the sparkle or the reward when its name says so or the data asks for it
	l.container = p.getBool("container", false) || l.name == "path" || l.name == "sparkle" || l.name == "reward";
	l.opacity = p.getFloat("opacity", 1.0f);
	l.scale = p.getFloat("scale", 1.0f);
	l.offset = p.getFloatPair("offset", engine::Vector2f(0.0f, 0.0f));
	l.rotation = p.getFloat("rotation", 0.0f);
	l.delay = p.getFloat("delay", -1.0f);
	l.overwriteInPlace = p.getBool("overwriteinplace", false);
	l.insertAfter = p.getString("insertafter", "");
	l.revealSounds = listOf(p, "revealsounds");
	l.randOffsetX = p.getRange("randoffsetx", engine::FloatRange(0.0f, 0.0f));
	l.randOffsetY = p.getRange("randoffsety", engine::FloatRange(0.0f, 0.0f));
	l.noRandOnZeroth = p.getBool("norandonzeroth", false);
	l.randScaleMult = p.getFloatPair("randscalemult", engine::Vector2f(1.0f, 1.0f));
	l.trackPos = p.getBool("trackpos", false);
	l.rotSpeed = p.getFloat("rotspeed", 0.0f) * DEGREES_TO_RADIANS;
	l.screenShakePulseMag = p.getFloat("screenshakepulsemag", 0.0f);
	l.screenShakePulseFalloff = p.getFloat("screenshakepulsefalloff", 1.0e5f);
	l.scalePulseGrowSpeed = p.getFloat("scalepulsegrowspeed", 1.0e4f);
	l.scalePulseMag = p.getFloat("scalepulsemag", l.scale);
	l.scalePulseSpeedMult = p.getFloat("scalepulsespeedmult", 1.0e4f);
	l.scalePulseMinSpeed = p.getFloat("scalepulseminspeed", 1.0e4f);
	l.scalePulseStartScale = p.getFloat("scalepulsestartscale", l.scale);
	l.plmSpecial = p.getBool("plmspecial", false);
	l.viewGroups = listOf(p, "viewgroups");
	l.viewGroupCombineAnd = p.getString("viewgroupcombine", "") == "and";
	std::string aDefaultClick = l.name == "path" ? "rotatesnap" : l.container ? "rotate" : "nothing";
	std::string aClick = p.getString("onclick", aDefaultClick);
	l.onClick = aClick == "rotate"       ? CLICK_ROTATE
				: aClick == "rotatesnap" ? CLICK_ROTATE_SNAP
				: aClick == "animate"    ? CLICK_ANIMATE
										 : CLICK_NOTHING;
	if (p.has("viewmodes"))
		l.viewModes = listOf(p, "viewmodes");
	else if (l.container)
		l.viewModes.push_back("__codecontrolled");
	l.viewAntiModes = listOf(p, "viewantimodes");
	l.smooth = p.getString("smooth", "") == "all";  // the string at 0x75174c
	return l;
}
