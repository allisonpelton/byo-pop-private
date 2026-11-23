#include "LevelDataReader.h"

namespace
{

// The comma list of an attribute (0x5a3990 and 0x425ed0): when the attribute is there, the list is its tokens; when it
// is not, the list is left as it was
void readList(const engine::Params& theParams, const char* theName, std::vector<std::string>& theList)
{
	if (!theParams.has(theName))
		return;
	theList = splitList(theParams.getString(theName, ""));
}

// The elements the level and chapter loaders list as their children that the game's data files do not use
bool isUntranslatedChild(const std::string& theName)
{
	return theName == "cellspawningpool" || theName == "pathidpool" || theName == "autospawner" || theName == "board" ||
		   theName == "bonuslevel" || theName == "autogamestats" || theName == "autostat";
}

} // namespace

bool LevelDataReader::startElement(const std::string& theName, const engine::Params& p)
{
	if (theName == "chapter")
		beginChapter(p);
	else if (theName == "gamelevel")
		beginLevel(p);
	else if (theName == "wreathworldpointmap")
		mChapter.wreathWorldPointMap.clear();
	else if (theName == "tier")
	{
		WreathTier aTier;
		aTier.worldPoints = p.getInt("worldpoints", 0);
		aTier.image = p.getString("image", "");
		mChapter.wreathWorldPointMap.push_back(aTier);
	}
	else if (theName == "untimedbuildingcosts" || theName == "timedbuildingcosts")
	{
		mCosts = theName == "untimedbuildingcosts" ? &mLevel.untimedBuildingCosts : &mLevel.timedBuildingCosts;
		mCosts->clear();
	}
	else if (theName == "rank")
	{
		BuildRankCost aRank;
		aRank.buildRank = p.getString("buildrank", "");
		aRank.cost = p.getInt("cost", 0);
		if (mCosts != nullptr)
			mCosts->push_back(aRank);
	}
	else if (theName == "untimedmeterzones" || theName == "timedmeterzones")
	{
		mZones = theName == "untimedmeterzones" ? &mLevel.untimedMeterZones : &mLevel.timedMeterZones;
		mZones->clear();
	}
	else if (theName == "zonestart")
	{
		ZoneStartClass aZone;
		aZone.name = p.getString("name", "");
		aZone.percent = p.getFloat("pct", 0.0f);
		if (mZones != nullptr)
			mZones->push_back(aZone);
	}
	else if (theName == "celllimiter")
	{
		CellLimiterClass aLimiter;
		aLimiter.checkClasses = p.getString("checkclasses", "");
		aLimiter.equalizeClasses = p.getString("equalizeclasses", "");
		aLimiter.convertToClass = p.getString("converttoclass", "");
		aLimiter.max = p.getInt("max", 0);
		mLevel.cellLimiters.push_back(aLimiter);
	}
	else if (theName == "cellcooldown")
	{
		CellCooldownClass aCooldown;
		aCooldown.cellClass = p.getString("class", "");
		aCooldown.cooldownTurns = p.getInt("cooldownturns", 0);
		aCooldown.convertToClass = p.getString("converttoclass", "");
		mLevel.cellCooldowns.push_back(aCooldown);
	}
	else if (theName == "pathidlimiter")
	{
		PathIdLimiterClass aLimiter;
		aLimiter.pathId = p.getString("pathid", "");
		aLimiter.max = p.getInt("max", 0);
		mLevel.pathIdLimiters.push_back(aLimiter);
	}
	else if (isUntranslatedChild(theName))
	{
		if (mReported.insert(theName).second)
			engine::getEngine()->log("Level data element \"%s\" not yet translated\n", theName.c_str());
	}
	else
		return false;
	return true;
}

bool LevelDataReader::endElement(const std::string& theName)
{
	if (theName == "gamelevel")
	{
		mData.gameLevels[mLevel.name] = mLevel;
		mChapter.levelNames.push_back(mLevel.name);
		mCosts = nullptr;
		mZones = nullptr;
	}
	else if (theName == "chapter")
		mData.chapters[mChapter.name] = mChapter;
	else if (theName == "untimedbuildingcosts" || theName == "timedbuildingcosts")
		mCosts = nullptr;
	else if (theName == "untimedmeterzones" || theName == "timedmeterzones")
		mZones = nullptr;
	else if (theName == "wreathworldpointmap" || theName == "tier" || theName == "rank" || theName == "zonestart" ||
			 theName == "celllimiter" || theName == "cellcooldown" || theName == "pathidlimiter" ||
			 isUntranslatedChild(theName))
		return true;
	else
		return false;
	return true;
}

// 0x5a2350
void LevelDataReader::beginChapter(const engine::Params& p)
{
	std::string aName = p.getString("name", "");
	mChapter = ChapterClass();
	std::string aBase = p.getString("baseclass", "");
	if (!aBase.empty())
	{
		std::map<std::string, ChapterClass>::iterator aBaseIt = mData.chapters.find(aBase);
		if (aBaseIt == mData.chapters.end())
			engine::getEngine()->log("CHAPTER CLASS %s CANNOT FIND BASE CLASS %s\n", aName.c_str(), aBase.c_str());
		else
			mChapter = aBaseIt->second;
	}
	ChapterClass& c = mChapter;
	c.name = aName;
	c.baseClass = aBase;
	c.levelNames.clear();
	c.explicitSelectBonusLevel = p.getString("explicitselectbonuslevel", c.explicitSelectBonusLevel);
	c.introLayout = p.getString("introlayout", c.introLayout);
	c.chapterDataSet = p.getString("chapterdataset", c.chapterDataSet);
	c.chapterPreDataSet = p.getString("chapterpredataset", c.chapterPreDataSet);
	c.outroLayout = p.getString("outrolayout", c.outroLayout);
	c.buildingScript = p.getString("buildingscript", c.buildingScript);
	c.buildingLayout = p.getString("buildinglayout", c.buildingLayout);
	c.preChapterDelay = p.getFloat("prechapterdelay", c.preChapterDelay);
	c.minPostChapterDelay = p.getFloat("minpostchapterdelay", c.minPostChapterDelay);
	c.introBackgroundLayout = p.getString("introbackgroundlayout", c.introBackgroundLayout);
	c.unlockHeroMode = p.getBool("unlockheromode", c.unlockHeroMode);
	c.description = p.getString("description", c.description);
	c.mapKeyPoint = p.getString("mapkeypoint", c.mapKeyPoint);
	c.successPostLevelLayout = p.getString("successpostlevellayout", c.successPostLevelLayout);
	readList(p, "levelprogression", c.levelProgression);
	readList(p, "introjournalpages", c.introJournalPages);
	readList(p, "outrojournalpages", c.outroJournalPages);
	c.introMusic = p.getString("intromusic", c.introMusic);
	c.outroMusic = p.getString("outromusic", c.outroMusic);
}

// 0x5b2b00
void LevelDataReader::beginLevel(const engine::Params& p)
{
	std::string aName = p.getString("name", "");
	mLevel = GameLevelClass();
	std::string aBase = p.getString("baseclass", "");
	if (!aBase.empty())
	{
		std::map<std::string, GameLevelClass>::iterator aBaseIt = mData.gameLevels.find(aBase);
		if (aBaseIt != mData.gameLevels.end())
			mLevel = aBaseIt->second;
	}
	GameLevelClass& c = mLevel;
	c.name = aName;
	c.baseClass = aBase;
	c.jackpotTarget = p.getInt("jackpottarget", c.jackpotTarget);
	c.jackpotBonus = p.getInt("jackpotbonus", c.jackpotBonus);
	c.boardId = p.getString("boardid", c.boardId);
	c.music = p.getString("music", c.music);
	c.hurryUpMusic = p.getString("hurryupmusic", c.hurryUpMusic);
	c.levelPlayTip = p.getString("levelplaytip", c.levelPlayTip);
	c.coinsToComplete = p.getInt("coinstocomplete", c.coinsToComplete);
	c.desiredEndpoints = p.getInt("desiredendpoints", c.desiredEndpoints);
	c.specialSpawningPool = p.getString("specialspawningpool", c.specialSpawningPool);
	c.specialSpawnPoolInterval = p.getFloat("specialspawnpoolinterval", c.specialSpawnPoolInterval);
	c.specialSpawnPoolNumPaths = p.getInt("specialspawnpoolnumpaths", c.specialSpawnPoolNumPaths);
	c.specialSpawnPoolIsTimed = p.getBool("specialspawnpoolistimed", c.specialSpawnPoolIsTimed);
	c.pathLengthMeterClass = p.getString("pathlengthmeterclass", c.pathLengthMeterClass);
	c.untimedLevel = p.getBool("untimedlevel", c.untimedLevel);
	c.levelLayoutFile = p.getString("levellayoutfile", c.levelLayoutFile);
	c.buildingTierName = p.getString("buildingtiername", c.buildingTierName);
	c.levelIntroSound = p.getString("levelintrosound", c.levelIntroSound);
	c.warmUpTime = p.getFloat("warmuptime", c.warmUpTime);
	c.minCoolDownTime = p.getFloat("mincooldowntime", c.minCoolDownTime);
	c.boardSettledTime = p.getFloat("boardsettledtime", c.boardSettledTime);
	c.spawnEndpointDelay = p.getFloat("spawnendpointdelay", c.spawnEndpointDelay);
	c.fadeTime = p.getFloat("fadetime", c.fadeTime);
	c.fxOnCoinLand = p.getString("fxoncoinland", c.fxOnCoinLand);
	c.timeTickSound = p.getString("timeticksound", c.timeTickSound);
	c.timeDoneFx = p.getString("timedonefx", c.timeDoneFx);
	c.timerCountSpeed = p.getFloat("timercountspeed", c.timerCountSpeed);
	c.pointsPerSecond = p.getFloat("pointspersecond", c.pointsPerSecond);
	c.coinsPerSecond = p.getFloat("coinspersecond", c.coinsPerSecond);
	c.fundagePerSecond = p.getFloat("fundagepersecond", c.fundagePerSecond);
	c.buildingMusic = p.getString("buildingmusic", c.buildingMusic);
	c.musicDelay = p.getFloat("musicdelay", c.musicDelay);
	c.fallbackPathId = p.getString("fallbackpathid", c.fallbackPathId);
	c.bonusTimeGrowStarScale = p.getFloat("bonustimegrowstarscale", c.bonusTimeGrowStarScale);
	c.bonusTimeBonusStarPct = p.getFloat("bonustimebonusstarpct", c.bonusTimeBonusStarPct);
	c.useJackpotChest = p.getBool("usejackpotchest", c.useJackpotChest);
	c.ignoreFundage = p.getInt("ignorefundage", c.ignoreFundage);
	c.bonusStarExtraValue = p.getFloat("bonusstarextravalue", c.bonusStarExtraValue);
	c.coinMeterCountTime = p.getFloat("coinmetercounttime", c.coinMeterCountTime);
	c.firstTalismanSpawnTooltip = p.getString("firsttalismanspawntooltip", c.firstTalismanSpawnTooltip);
	c.successPostLevelLayout = p.getString("successpostlevellayout", c.successPostLevelLayout);
	// 0x5b2b00: a level time that is not given, or is "__untimed", is the largest time (1e8 seconds)
	if (p.getString("leveltime", "__untimed") == "__untimed")
		c.levelTime = 1.0e8f;
	else
		c.levelTime = p.getFloat("leveltime", c.levelTime);
	readList(p, "onscalethrowsounds", c.onScaleThrowSounds);
	readList(p, "timedonesounds", c.timeDoneSounds);
	readList(p, "neverlimit", c.neverLimit);
	readList(p, "levelcompletesounds", c.levelCompleteSounds);
	readList(p, "levelfailsounds", c.levelFailSounds);
	readList(p, "levelpresentsounds", c.levelPresentSounds);
	readList(p, "soundsoncoinland", c.soundsOnCoinLand);
	readList(p, "soundsonbarmove", c.soundsOnBarMove);
	readList(p, "soundsoncointhrow", c.soundsOnCoinThrow);
	readList(p, "prohibitedtooltips", c.prohibitedTooltips);
	readList(p, "endpointspawntags", c.endpointSpawnTags);
}
