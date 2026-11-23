#ifndef __TALISMANIA_PATHDATA_H__
#define __TALISMANIA_PATHDATA_H__

// The paths and the pools the data sets describe: path classes (pathclass, loader 0x5dcfb0, with the energizer's own
// attribute 0x497530), path image classes (pathimageclass, 0x5b8030, its endpoint and connection pictures 0x5b87e0 and
// 0x5b8b20), the pools that pick the next path id (pathidpoolclass) and the pools that pick the next cell
// (cellspawningpoolclass, its items 0x47c120). A class that names a baseclass starts as a copy of it.

#include <map>
#include <string>
#include <vector>

// The number of directions a cell has (a hexagon's sides); an endpoint or connection direction is taken modulo it
const int PATH_DIRECTION_COUNT = 6;

// "pathclass" (0x5dcfb0)
struct PathClass
{
	std::string name;
	std::string baseClass;
	std::string type;  // "normal", "combo" or "energizer"
	std::string pathImageClass;
	std::string energizingPathImageClass;  // an energizer's picture for the paths it energizes
	int constEnergizeRank = 0;
};

// The picture of one end or one link of a path, and the effect that plays with it
struct PathPicture
{
	std::string image;
	std::string fx;  // endpoints only
};

// "pathimageclass" (0x5b8030)
struct PathImageClass
{
	std::string name;
	std::string baseClass;
	float overlayPriority = 0.0f;
	int energizeRank = 0;
	std::string energizeColour;
	std::string popFx;
	std::vector<std::string> compatibleWith;
	std::string clashFx;
	std::map<int, PathPicture> endpoints;    // by direction
	std::map<int, std::string> connections;  // by direction, the picture's name
};

// <item> of a path id pool (0x49d350): a path id, its rotation ("__rand" draws one) and its weight
struct PathIdItem
{
	std::string pathId;
	std::string rotation;
	float weight = 0.0f;
};

// <item> of a cell spawning pool (0x47c120)
struct SpawnItem
{
	std::string cellClass;
	std::string pathClass;
	std::string pathId;
	std::string spawnTags;
	float weight = 0.0f;
};

// How a pool's items are taken: one at a time in order, a random one by weight, or one after the other cycling
enum PoolSectionKind
{
	POOL_SEQUENCE = 0,
	POOL_RANDOM = 1,
	POOL_SELECTOR = 2
};

// A <random> or <selector> element with its items, or one item alone outside both
template <class Item>
struct PoolSection
{
	int kind = POOL_SEQUENCE;
	std::vector<Item> items;
};

// "pathidpoolclass"
struct PathIdPoolClass
{
	std::string name;
	std::vector<PoolSection<PathIdItem>> sections;
};

// "cellspawningpoolclass"
struct CellSpawningPoolClass
{
	std::string name;
	std::vector<PoolSection<SpawnItem>> sections;
};

#endif
