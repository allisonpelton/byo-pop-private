#include "Table.h"
#include "Font.h"
#include "Graphics.h"

using namespace engine;

// 0x490150 (pizzafrenzy)
Table::Table()
{
	mRows = 0;
	mTopRow = 0;
	mSelectedRow = -1;  // port: the original leaves it unset
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

// 0x48fd90 (pizzafrenzy): the model's signal goes to a function that does nothing (0x4d0470 (pizzafrenzy)), which the
// port leaves out
void Table::setModel(TableModel* theModel)
{
	mModel = theModel;
}

// 0x48f1d0 (pizzafrenzy)
void Table::update(UpdateInfo& theInfo)
{
	if (mModel != nullptr && mModel->isReady())
	{
		int aSelectedRow = mModel->getSelectedRow();
		if (aSelectedRow != mSelectedRow || mModel->rowCount() <= mTopRow)
		{
			int aTopRow = 0;
			mSelectedRow = aSelectedRow;
			if (mRows <= aSelectedRow)
				aTopRow = (aSelectedRow - mRows) + 1;
			mTopRow = aTopRow;
		}
	}
}

// 0x48f330 (pizzafrenzy): each column's text goes at its left, middle or right, by the column's alignment
void Table::layout()
{
	float aWidth = 0;
	for (size_t i = 0; i < mColumns.size(); i++)
	{
		TableColumn& aColumn = mColumns[i];
		if (aColumn.mAlign == 0)
			aColumn.mX = aWidth;
		else if (aColumn.mAlign == 1)
			aColumn.mX = (float)aColumn.mWidth * 0.5f + aWidth;
		else if (aColumn.mAlign == 2)
			aColumn.mX = (float)aColumn.mWidth + aWidth;
		aWidth = aWidth + (float)aColumn.mWidth;
	}
	float aLineHeight = 10.0f;
	if (mFont != nullptr)
		aLineHeight = (float)mFont->getHeight();
	mBounds.left = mPosition.x;
	mBounds.right = mPosition.x + aWidth;
	mBounds.top = mPosition.y;
	mBounds.bottom = (float)mRows * aLineHeight + mPosition.y;
	clearFlags(NODE_LAYOUT);
}

// 0x48f410 (pizzafrenzy): the background, then the rows from the top row on, the picked one on its own background; the
// model's status instead of the rows while it is not ready. A column's text is drawn at its position by its alignment.
// Without a model the original draws a placeholder text for each cell, at a position it never sets: the port draws
// nothing (every table of the game has a model before it is shown).
void Table::draw(Graphics* g)
{
	g->setColor(mBackgroundColor.r, mBackgroundColor.g, mBackgroundColor.b, mBackgroundColor.a);
	g->pushState();
	g->fillRect(Rect(0, 0, mBounds.right - mBounds.left, mBounds.bottom - mBounds.top));
	g->popState();
	Font* aFont = g->state().mFont;
	float aLineHeight = aFont != nullptr ? (float)aFont->getHeight() : 0.0f;
	if (mModel == nullptr)
		return;
	if (!mModel->isReady())
	{
		std::string aStatus = mModel->getStatus();
		g->setColor(mTextColor.r, mTextColor.g, mTextColor.b, mTextColor.a);
		g->setColorMode(2);
		g->setHAlign(1);
		g->setVAlign(1);
		g->drawString(aStatus.c_str(), (mBounds.right - mBounds.left) * 0.5f, (mBounds.bottom - mBounds.top) * 0.5f);
		return;
	}
	int aRow = mTopRow;
	if (mModel->rowCount() < aRow)
		aRow = mModel->rowCount();
	int aLastRow = mModel->rowCount();
	if (aLastRow > mRows + aRow)
		aLastRow = mRows + aRow;
	float aY = 0;
	for (; aRow < aLastRow; aRow++)
	{
		if (aRow == mSelectedRow)
		{
			g->setColor(mSelectionBackgroundColor.r, mSelectionBackgroundColor.g, mSelectionBackgroundColor.b, mSelectionBackgroundColor.a);
			g->setColorMode(0);
			g->fillRect(Rect(0, aY, mBounds.right - mBounds.left, aY + aLineHeight));
			g->setColor(mSelectionTextColor.r, mSelectionTextColor.g, mSelectionTextColor.b, mSelectionTextColor.a);
		}
		else
			g->setColor(mTextColor.r, mTextColor.g, mTextColor.b, mTextColor.a);
		g->setColorMode(2);
		for (size_t i = 0; i < mColumns.size(); i++)
		{
			std::string aCell = mModel->getCell(aRow, mColumns[i].mName);
			g->setHAlign(mColumns[i].mAlign);
			g->drawString(aCell.c_str(), mColumns[i].mX, aY);
		}
		aY = aY + aLineHeight;
	}
}
