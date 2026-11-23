#include "Gameboard.h"

#include "AtlantisApp.h"
#include "Image.h"
#include "Random.h"

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
std::string Gameboard::pickCellClass(const PotClass& theProps) const
{
	if (!theProps.initialCellClass.empty())
		return theProps.initialCellClass;
	std::map<std::string, std::string>::const_iterator aPoolName = mClass.cellSpawningPools.find("default");
	if (aPoolName == mClass.cellSpawningPools.end())
		return std::string();
	std::map<std::string, CellSpawningPoolClass>::const_iterator aPool = mApp->mData.cellSpawningPools.find(aPoolName->second);
	if (aPool == mApp->mData.cellSpawningPools.end() || aPool->second.sections.empty())
		return std::string();
	const PoolSection<SpawnItem>& aSection = aPool->second.sections[0];
	float aTotal = 0.0f;
	for (size_t i = 0; i < aSection.items.size(); i++)
		aTotal += aSection.items[i].weight;
	float aPick = engine::randomFloat(0.0f, aTotal);
	for (size_t i = 0; i < aSection.items.size(); i++)
	{
		aPick -= aSection.items[i].weight;
		if (aPick <= 0.0f)
			return aSection.items[i].cellClass;
	}
	return aSection.items.empty() ? std::string() : aSection.items.back().cellClass;
}

// A pot's cell drawn: each layer of its class shown in the normal view mode is its picture at the pot's middle
void Gameboard::drawCell(const BoardPot& thePot)
{
	if (thePot.props.sidePot || thePot.props.stayEmpty || thePot.props.solid)
		return;
	std::string aCellClassName = pickCellClass(thePot.props);
	std::map<std::string, CellClass>::const_iterator aCell = mApp->mData.cellClasses.find(aCellClassName);
	if (aCell == mApp->mData.cellClasses.end())
		return;
	engine::Vector2f aMiddle = getPotPosition(thePot.position);
	for (size_t i = 0; i < aCell->second.layers.size(); i++)
	{
		const CellLayerClass& aLayer = aCell->second.layers[i];
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
