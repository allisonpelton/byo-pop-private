#ifndef __TALISMANIA_LEVELDATA_H__
#define __TALISMANIA_LEVELDATA_H__

// The chapters and levels the game's data sets describe: "chapter" (AtlantisChapterClass, loader 0x5a2350, its children
// read from 0x5063a0) and "gamelevel" (GameLevelClass, loader 0x5b2b00, its children from 0x5093f0). A class that names a
// baseclass starts as a copy of it and each attribute then defaults to what the class holds. Translated so far: the
// attributes and the children that the game's data files use (the cell limiters and cooldowns, the build costs, the
// zone starts and the world point tiers); the children the data does not use (cellspawningpool, pathidpool, autospawner,
// board, bonuslevel) are not read.

#include <map>
#include <string>
#include <vector>

// <rank buildrank= cost=> (0x5b6e80, 0x5b7040): what building at a rank costs
struct BuildRankCost
{
	std::string buildRank;
	int cost = 0;
};

// <celllimiter checkclasses= converttoclass= max=> (0x5b67f0): at most max cells of the checked classes on the board,
// the rest turned into the other class
struct CellLimiterClass
{
	std::string checkClasses;
	std::string equalizeClasses;
	std::string convertToClass;
	int max = 0;
};

// <cellcooldown class= cooldownturns= converttoclass=> (0x5b55b0): a cell of the class is turned into another for that
// many turns after it spawns
struct CellCooldownClass
{
	std::string cellClass;
	int cooldownTurns = 0;
	std::string convertToClass;
};

// <pathidlimiter pathid= max=> (0x5b6c10)
struct PathIdLimiterClass
{
	std::string pathId;
	int max = 0;
};

// <zonestart name= pct=>: where a meter zone starts, in percent of the meter
struct ZoneStartClass
{
	std::string name;
	float percent = 0.0f;
};

// "gamelevel" (0x5b2b00)
struct GameLevelClass
{
	std::string name;
	std::string baseClass;
	bool untimedLevel = false;
	float levelTime = 30.0f;  // seconds; 1e8 when the level is untimed ("__untimed")
	int jackpotTarget = 0;
	int jackpotBonus = 0;
	std::string boardId;
	std::string music;
	std::string hurryUpMusic;
	std::string levelPlayTip;
	int coinsToComplete = 100;
	int desiredEndpoints = 4;
	std::string specialSpawningPool = "special";
	float specialSpawnPoolInterval = 20.0f;
	int specialSpawnPoolNumPaths = 5;
	bool specialSpawnPoolIsTimed = false;
	std::string pathLengthMeterClass = "std";
	// A field with no attribute: the level clock layout (the constant 0x5b2b00 sets)
	std::string postLevelTimeCounterLayout = "config/screens/postleveltimecounter.xml";
	std::string levelLayoutFile = "config/screens/levellayouts/5x4level.xml";
	std::string buildingTierName;
	std::string levelIntroSound = "levelintro";
	float warmUpTime = 0.3f;
	float minCoolDownTime = 0.3f;
	float boardSettledTime = 0.1f;
	float spawnEndpointDelay = 0.1f;
	float fadeTime = 1.0f;
	std::string fxOnCoinLand;
	std::string timeTickSound;
	std::string timeDoneFx;
	float timerCountSpeed = 1.0f;
	float pointsPerSecond = 1.0f;
	float coinsPerSecond = 100.0f;
	float fundagePerSecond = 1.0f;
	std::string buildingMusic;
	float musicDelay = 0.0f;
	std::string fallbackPathId = "02";
	float bonusTimeGrowStarScale = 1.0f;
	float bonusTimeBonusStarPct = 0.5f;
	bool useJackpotChest = true;
	int ignoreFundage = 0;
	float bonusStarExtraValue = 10.0f;
	float coinMeterCountTime = 5.0f;
	std::string firstTalismanSpawnTooltip;
	std::string successPostLevelLayout = "config/screens/levelcomplete/levelcomplete.xml";
	std::vector<std::string> onScaleThrowSounds;
	std::vector<std::string> timeDoneSounds;
	std::vector<std::string> neverLimit;
	std::vector<std::string> levelCompleteSounds;
	std::vector<std::string> levelFailSounds;
	std::vector<std::string> levelPresentSounds;
	std::vector<std::string> soundsOnCoinLand;
	std::vector<std::string> soundsOnBarMove;
	std::vector<std::string> soundsOnCoinThrow;
	std::vector<std::string> prohibitedTooltips;
	std::vector<std::string> endpointSpawnTags;
	std::vector<BuildRankCost> untimedBuildingCosts;
	std::vector<BuildRankCost> timedBuildingCosts;
	std::vector<CellLimiterClass> cellLimiters;
	std::vector<CellCooldownClass> cellCooldowns;
	std::vector<PathIdLimiterClass> pathIdLimiters;
	std::vector<ZoneStartClass> untimedMeterZones;
	std::vector<ZoneStartClass> timedMeterZones;
};

// <tier worldpoints= image=> of a <wreathworldpointmap> (0x5a1bc0): the wreath image from that many world points
struct WreathTier
{
	int worldPoints = 0;
	std::string image;
};

// "chapter" (0x5a2350)
struct ChapterClass
{
	std::string name;
	std::string baseClass;
	std::string mapKeyPoint;
	std::string explicitSelectBonusLevel;
	std::string introLayout;
	std::string chapterDataSet;
	std::string chapterPreDataSet;
	std::string outroLayout;
	std::string buildingScript;
	std::string buildingLayout;
	float preChapterDelay = 0.01f;
	float minPostChapterDelay = 0.01f;
	std::string introBackgroundLayout;
	bool unlockHeroMode = false;
	std::string description;
	std::vector<std::string> levelProgression;  // level names in order, "__bonus" for a bonus level
	std::vector<std::string> introJournalPages;
	std::vector<std::string> outroJournalPages;
	std::string introMusic;
	std::string outroMusic;
	std::string successPostLevelLayout;
	std::vector<WreathTier> wreathWorldPointMap;
	std::vector<std::string> levelNames;  // the levels defined inside the chapter, in the file's order
};

#endif
