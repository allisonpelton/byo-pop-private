#include "BoardDataReader.h"

#include "Engine.h"

// 0x5affd0: a pot's properties
PotClass BoardDataReader::readPot(const engine::Params& p) const
{
	PotClass aPot;
	aPot.sourcePoolId = p.getString("sourcepoolid", "");
	aPot.spawningPoolId = p.getString("spawningpoolid", "");
	aPot.sidePotPathId = p.getString("sidepotpathid", "");
	aPot.sidePot = p.getBool("sidepot", false);
	aPot.overlayImage = p.getString("overlayimg", "");
	aPot.stayEmpty = p.getBool("stayempty", false);
	aPot.solid = p.getBool("solid", false);
	aPot.initialCellClass = p.getString("initialcellclass", "");
	aPot.initialPathClass = p.getString("initialpathclass", "");
	aPot.initialPathId = p.getString("initialpathid", "");
	aPot.spawnTags = p.getString("spawntags", "");
	aPot.tooltipDelay = p.getFloat("tooltipdelay", 0.0f);
	aPot.tooltip = p.getString("tooltip", "");
	// 0x5b1830: the repeat count, one by default; 0x5b1aa0 and the layout's pots: the position
	aPot.repeat = p.getInt("repeat", 1);
	aPot.position = p.getIntPair("pos", engine::Vector2<int>(0, 0));
	return aPot;
}

bool BoardDataReader::startElement(const std::string& theName, const engine::Params& p)
{
	if (theName == "board")
	{
		// 0x5b0e00: a board that names a baseboard starts as a copy of it
		mBoard = BoardClass();
		std::string aBase = p.getString("baseboard", "");
		if (!aBase.empty())
		{
			std::map<std::string, BoardClass>::iterator aBaseIt = mData.boards.find(aBase);
			if (aBaseIt == mData.boards.end())
				engine::getEngine()->log("BOARD CLASS %s CANNOT FIND BASE CLASS %s\n", p.getString("name", "").c_str(),
										 aBase.c_str());
			else
				mBoard = aBaseIt->second;
		}
		mBoard.name = p.getString("name", "");
		mBoard.baseBoard = aBase;
		mBoard.offset = p.getIntPair("offset", mBoard.offset);
		mInBoard = true;
		mInLayout = false;
		return true;
	}
	if (!mInBoard)
		return false;
	if (theName == "potprops")
		mBoard.potSize = p.getIntPair("size", mBoard.potSize);
	else if (theName == "cellspawningpool")
		mBoard.cellSpawningPools[p.getString("name", "")] = p.getString("class", "");
	else if (theName == "pathidpool")
		mBoard.pathIdPools[p.getString("name", "")] = p.getString("class", "");
	else if (theName == "layout")
		mInLayout = true;
	else if (theName == "vspanclass")
		mColumnTemplate = p.getString("name", "");
	else if (theName == "vspan")
	{
		VSpanClass aColumn;
		aColumn.templateName = p.getString("class", "");
		aColumn.position = p.getIntPair("pos", engine::Vector2<int>(0, 0));
		mBoard.columns.push_back(aColumn);
	}
	else if (theName == "pot")
	{
		// 0x5b1830: in a column template the pot is added to the template, in the layout it is a pot of the board
		PotClass aPot = readPot(p);
		if (!mColumnTemplate.empty())
			mBoard.columnTemplates[mColumnTemplate].push_back(aPot);
		else
			mBoard.pots.push_back(aPot);
	}
	else
		return false;
	return true;
}

bool BoardDataReader::endElement(const std::string& theName)
{
	if (theName == "board" && mInBoard)
	{
		mData.boards[mBoard.name] = mBoard;
		mInBoard = false;
	}
	else if (theName == "layout" && mInBoard)
		mInLayout = false;
	else if (theName == "vspanclass" && mInBoard)
		mColumnTemplate.clear();
	else if (mInBoard && (theName == "potprops" || theName == "cellspawningpool" || theName == "pathidpool" ||
						  theName == "vspan" || theName == "pot"))
		return true;
	else
		return false;
	return true;
}
