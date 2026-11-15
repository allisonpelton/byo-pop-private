#include "Table.h"

using namespace engine;

// 0x490150 (pizzafrenzy)
Table::Table()
{
	mModel = nullptr;
	mRows = 0;
	mTopRow = 0;
	addOwnFlags(NODE_DRAWABLE | NODE_LISTENERS);
}

// 0x490490 (pizzafrenzy): the columns grow by one (a new column is empty), and the bounds need a layout
int Table::addColumn()
{
	int aCount = (int)mColumns.size();
	mColumns.resize(aCount + 1);
	setFlags(NODE_LAYOUT);
	return aCount;
}

// 0x48f940 (pizzafrenzy)
void Table::setColumnName(int theColumn, const std::string& theName)
{
	if (-1 < theColumn && theColumn < (int)mColumns.size())
		mColumns[theColumn].mName = theName;
}

// 0x48f280 (pizzafrenzy): the bounds need a layout
void Table::setColumnWidth(int theColumn, int theWidth)
{
	if (-1 < theColumn && theColumn < (int)mColumns.size())
	{
		mColumns[theColumn].mWidth = theWidth;
		setFlags(NODE_LAYOUT);
	}
}

// 0x48f2e0 (pizzafrenzy)
void Table::setColumnAlign(int theColumn, int theAlign)
{
	if (-1 < theColumn && theColumn < (int)mColumns.size())
		mColumns[theColumn].mAlign = theAlign;
}

// 0x48f010 (pizzafrenzy): the bounds need a layout
void Table::setRows(int theRows)
{
	mRows = theRows;
	setFlags(NODE_LAYOUT);
}
