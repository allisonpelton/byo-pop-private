#ifndef __TALISMANIA_BOARDDATA_H__
#define __TALISMANIA_BOARDDATA_H__

// The game boards the data sets describe ("board", GameboardClass, loader 0x5b0e00, its children registered by 0x508c60
// and read by 0x5b0ff0, 0x5b1220, 0x5b1830, 0x5b1aa0 and 0x5affd0): a grid of pots, each a place a cell sits in,
// laid out from named column templates (vspanclass) placed at grid positions (vspan) and from single pots given by their
// position. A board that names a baseboard starts as a copy of it.

#include "Geometry.h"

#include <map>
#include <string>
#include <vector>

// <pot> (0x5affd0): one pot's properties, as the column template or the layout gives them
struct PotClass
{
	std::string sourcePoolId;
	std::string spawningPoolId;
	std::string sidePotPathId;
	bool sidePot = false;
	std::string overlayImage;
	bool stayEmpty = false;
	bool solid = false;
	std::string initialCellClass;
	std::string initialPathClass;
	std::string initialPathId;
	std::string spawnTags;
	float tooltipDelay = 0.0f;
	std::string tooltip;
	int repeat = 1;  // in a column template: how many pots in a row have these properties
	engine::Vector2<int> position;  // in the layout: where this pot is (the properties override the template's)
};

// <vspan class= pos=> (0x5b1aa0): a column template placed with its first pot at the position
struct VSpanClass
{
	std::string templateName;
	engine::Vector2<int> position;
};

// "board" (0x5b0e00)
struct BoardClass
{
	std::string name;
	std::string baseBoard;
	engine::Vector2<int> offset;
	engine::Vector2<int> potSize;  // <potprops size=>
	std::map<std::string, std::string> cellSpawningPools;  // pool name to its class
	std::map<std::string, std::string> pathIdPools;
	std::map<std::string, std::vector<PotClass>> columnTemplates;  // <vspanclass name=>, its pots in order
	std::vector<VSpanClass> columns;                               // <vspan> in the layout's order
	std::vector<PotClass> pots;                                    // <pot pos=> straight in the layout
};

#endif
