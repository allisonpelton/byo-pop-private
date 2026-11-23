#include "Gameboard.h"

#include "AtlantisApp.h"
#include "Image.h"
#include "Random.h"

#include <cstdlib>

namespace
{

// The pot class's own size is the board's (<potprops size=>): a column's pots this far apart (up) and the columns this far
// apart (across)
const float ODD_COLUMN_RAISE = 0.5f;  // the odd columns stand half a pot higher

const char* const NORMAL_VIEW_MODE = "normal";

bool hasMode(const std::vector<std::string>& theModes, const char* theMode)
{
	for (size_t i = 0; i < theModes.size(); i++)
		if (theModes[i] == theMode)
			return true;
	return false;
}

} // namespace

Gameboard::Gameboard(AtlantisApp* theApp, const BoardClass& theClass, engine::Container* theCells)
	: mApp(theApp), mClass(theClass), mCells(theCells)
{
	// A board that names a base board starts as a copy of it: the pots and templates it does not give are the base's
	layOut();
	for (size_t i = 0; i < mPots.size(); i++)
		drawCell(mPots[i]);
}

engine::Vector2f Gameboard::getPotPosition(const engine::Vector2<int>& thePosition) const
{
	float aColumnRaise = (thePosition.x & 1) != 0 ? ODD_COLUMN_RAISE : 0.0f;
	return engine::Vector2f((float)(thePosition.x * mClass.potSize.x),
							-((float)thePosition.y + aColumnRaise) * (float)mClass.potSize.y);
}

// 0x4aae10: the column templates placed at their positions, a template's pots going up from the first row, then the
// layout's own pots over them. (Only the properties a layout pot gives are laid over the template's; the original
// merges attribute by attribute.)
void Gameboard::layOut()
{
	std::map<std::pair<int, int>, size_t> aIndex;
	for (size_t i = 0; i < mClass.columns.size(); i++)
	{
		const VSpanClass& aSpan = mClass.columns[i];
		std::map<std::string, std::vector<PotClass>>::const_iterator aTemplate = mClass.columnTemplates.find(aSpan.templateName);
		if (aTemplate == mClass.columnTemplates.end())
			continue;
		int aRow = aSpan.position.y;
		for (size_t j = 0; j < aTemplate->second.size(); j++)
		{
			for (int aRepeat = 0; aRepeat < aTemplate->second[j].repeat; aRepeat++)
			{
				BoardPot aPot;
				aPot.position = engine::Vector2<int>(aSpan.position.x, aRow++);
				aPot.props = aTemplate->second[j];
				aIndex[std::make_pair(aPot.position.x, aPot.position.y)] = mPots.size();
				mPots.push_back(aPot);
			}
		}
	}
	for (size_t i = 0; i < mClass.pots.size(); i++)
	{
		const PotClass& aGiven = mClass.pots[i];
		std::map<std::pair<int, int>, size_t>::iterator aFound = aIndex.find(std::make_pair(aGiven.position.x, aGiven.position.y));
		if (aFound == aIndex.end())
		{
			BoardPot aPot;
			aPot.position = aGiven.position;
			aPot.props = aGiven;
			aIndex[std::make_pair(aPot.position.x, aPot.position.y)] = mPots.size();
			mPots.push_back(aPot);
			continue;
		}
		PotClass& aProps = mPots[aFound->second].props;
		if (!aGiven.initialCellClass.empty())
			aProps.initialCellClass = aGiven.initialCellClass;
		if (!aGiven.initialPathClass.empty())
			aProps.initialPathClass = aGiven.initialPathClass;
		if (!aGiven.initialPathId.empty())
			aProps.initialPathId = aGiven.initialPathId;
		if (!aGiven.tooltip.empty())
			aProps.tooltip = aGiven.tooltip;
	}
}

// The class a pot's first cell has: the layout's, else one drawn from the board's default pool by weight
SpawnItem Gameboard::pickSpawn(const PotClass& theProps) const
{
	SpawnItem aNone;
	if (!theProps.initialCellClass.empty())
	{
		aNone.cellClass = theProps.initialCellClass;
		return aNone;
	}
	std::map<std::string, std::string>::const_iterator aPoolName = mClass.cellSpawningPools.find("default");
	if (aPoolName == mClass.cellSpawningPools.end())
		return aNone;
	std::map<std::string, CellSpawningPoolClass>::const_iterator aPool = mApp->mData.cellSpawningPools.find(aPoolName->second);
	if (aPool == mApp->mData.cellSpawningPools.end() || aPool->second.sections.empty())
		return aNone;
	const PoolSection<SpawnItem>& aSection = aPool->second.sections[0];
	float aTotal = 0.0f;
	for (size_t i = 0; i < aSection.items.size(); i++)
		aTotal += aSection.items[i].weight;
	float aPick = engine::randomFloat(0.0f, aTotal);
	for (size_t i = 0; i < aSection.items.size(); i++)
	{
		aPick -= aSection.items[i].weight;
		if (aPick <= 0.0f)
			return aSection.items[i];
	}
	return aSection.items.empty() ? aNone : aSection.items.back();
}

// A pot's cell drawn: each layer of its class shown in the normal view mode is its picture at the pot's middle
void Gameboard::drawCell(const BoardPot& thePot)
{
	if (thePot.props.sidePot || thePot.props.stayEmpty || thePot.props.solid)
		return;
	SpawnItem aSpawn = pickSpawn(thePot.props);
	std::map<std::string, CellClass>::const_iterator aCell = mApp->mData.cellClasses.find(aSpawn.cellClass);
	if (aCell == mApp->mData.cellClasses.end())
		return;
	engine::Vector2f aMiddle = getPotPosition(thePot.position);
	// The cell's path: the layout's, else the spawn item's (its id "default" is drawn from the board's pool of that name),
	// else the cell class's own
	std::string aPathClass = !thePot.props.initialPathClass.empty() ? thePot.props.initialPathClass
							 : !aSpawn.pathClass.empty()            ? aSpawn.pathClass
																	: aCell->second.explicitPathClass;
	std::string aPathId = !thePot.props.initialPathId.empty() ? thePot.props.initialPathId : aSpawn.pathId;
	if (aPathId.empty() && aCell->second.explicitPathId != "invalid")
		aPathId = aCell->second.explicitPathId;
	if (aPathId == "default")
		aPathId = pickPathId("default");
	for (size_t i = 0; i < aCell->second.layers.size(); i++)
	{
		const CellLayerClass& aLayer = aCell->second.layers[i];
		if (aLayer.name == "path")
		{
			if (aCell->second.pathVisible && !aPathId.empty())
				drawPath(aPathClass, aPathId, aMiddle);
			continue;
		}
		if (aLayer.container || aLayer.resource.empty())
			continue;
		if (!aLayer.viewModes.empty() && !hasMode(aLayer.viewModes, NORMAL_VIEW_MODE))
			continue;
		engine::Surface* aSurface = mApp->mTheme->getImage(aLayer.resource);
		if (aSurface == nullptr)
			continue;
		engine::Image* anImage = new engine::Image(aSurface);
		anImage->setPosition(aMiddle.x + aLayer.offset.x, aMiddle.y + aLayer.offset.y);
		anImage->setScale(aLayer.scale);
		anImage->setOpacity(aLayer.opacity);
		mCells->addChild(anImage);
	}
}

// A path id from the board's pool of that name: its first section's items by weight, the rotation "__rand" turning the
// id's directions by a drawn number of sixths
std::string Gameboard::pickPathId(const std::string& thePoolName) const
{
	std::map<std::string, std::string>::const_iterator aPoolName = mClass.pathIdPools.find(thePoolName);
	if (aPoolName == mClass.pathIdPools.end())
		return std::string();
	std::map<std::string, PathIdPoolClass>::const_iterator aPool = mApp->mData.pathIdPools.find(aPoolName->second);
	if (aPool == mApp->mData.pathIdPools.end() || aPool->second.sections.empty() || aPool->second.sections[0].items.empty())
		return std::string();
	const std::vector<PathIdItem>& anItems = aPool->second.sections[0].items;
	float aTotal = 0.0f;
	for (size_t i = 0; i < anItems.size(); i++)
		aTotal += anItems[i].weight;
	float aPick = engine::randomFloat(0.0f, aTotal);
	const PathIdItem* aChosen = &anItems.back();
	for (size_t i = 0; i < anItems.size(); i++)
	{
		aPick -= anItems[i].weight;
		if (aPick <= 0.0f)
		{
			aChosen = &anItems[i];
			break;
		}
	}
	int aTurn = aChosen->rotation == "__rand" ? engine::crtRand() % PATH_DIRECTION_COUNT : atoi(aChosen->rotation.c_str());
	std::string anId;
	for (size_t i = 0; i < aChosen->pathId.size(); i++)
		anId += (char)('0' + (aChosen->pathId[i] - '0' + aTurn) % PATH_DIRECTION_COUNT);
	return anId;
}

// A path drawn: one endpoint picture for each direction in the id, and, where two directions face each other, the
// connection picture between them (the data's connections 0 to 2 are the three straight lines)
void Gameboard::drawPath(const std::string& thePathClass, const std::string& thePathId, const engine::Vector2f& theMiddle)
{
	std::map<std::string, PathClass>::const_iterator aPath = mApp->mData.pathClasses.find(thePathClass);
	if (aPath == mApp->mData.pathClasses.end())
		return;
	std::map<std::string, PathImageClass>::const_iterator anImages = mApp->mData.pathImageClasses.find(aPath->second.pathImageClass);
	if (anImages == mApp->mData.pathImageClasses.end())
		return;
	bool aHas[PATH_DIRECTION_COUNT] = {false, false, false, false, false, false};
	for (size_t i = 0; i < thePathId.size(); i++)
		aHas[(thePathId[i] - '0') % PATH_DIRECTION_COUNT] = true;
	for (int d = 0; d < PATH_DIRECTION_COUNT; d++)
	{
		std::string anImageName;
		if (aHas[d] && aHas[(d + 3) % PATH_DIRECTION_COUNT] && d < 3)
		{
			std::map<int, std::string>::const_iterator aLink = anImages->second.connections.find(d);
			if (aLink != anImages->second.connections.end())
				anImageName = aLink->second;
		}
		else if (aHas[d])
		{
			std::map<int, PathPicture>::const_iterator anEnd = anImages->second.endpoints.find(d);
			if (anEnd != anImages->second.endpoints.end())
				anImageName = anEnd->second.image;
		}
		if (d >= 3 && aHas[d] && aHas[d - 3])
			continue;
		engine::Surface* aSurface = anImageName.empty() ? nullptr : mApp->mTheme->getImage(anImageName);
		if (aSurface == nullptr)
			continue;
		engine::Image* anImage = new engine::Image(aSurface);
		anImage->setPosition(theMiddle.x, theMiddle.y);
		mCells->addChild(anImage);
	}
}
