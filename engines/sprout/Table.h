#ifndef __SPROUT_TABLE_H__
#define __SPROUT_TABLE_H__

// The signal build's table (Pizza Frenzy's; Iggle Pop's Mac build names it engine::Table: a component with slots; 0x180
// bytes, constructor 0x490150 (pizzafrenzy), vtable 0x504560 (pizzafrenzy)): columns, each with a name, a width and an
// alignment, and a number of rows of the model's text. A layout's table element makes it and its column elements add
// the columns. Its drawing (0x6c: 0x48f410 (pizzafrenzy)), update (0x70: 0x48f1d0 (pizzafrenzy)) and layout (0x74:
// 0x48f330 (pizzafrenzy)) are here; the placeholder text it draws with no model (0x48f410 (pizzafrenzy)) is not, as
// every table of the game has a model before it is shown.

#include "Color4.h"
#include "Node.h"
#include "Signal.h"
#include "TableModel.h"
#include <string>
#include <vector>

namespace engine
{

// A column (0x28 bytes in the original)
struct TableColumn
{
	std::string mName;  // +0x00
	int mWidth;         // +0x1c
	int mAlign;         // +0x20: 0 left, 1 centre, 2 right
	float mX;           // +0x24: where the layout puts the column's text

	TableColumn() : mWidth(0), mAlign(0), mX(0) {}
};

class Table : public Node, public HasSlots
{
public:
	Table();  // 0x490150 (pizzafrenzy)

	// 0x490490 (pizzafrenzy): a new column at the end; the number it has
	int addColumn();
	// 0x48f940 (pizzafrenzy), 0x48f280 (pizzafrenzy), 0x48f2e0 (pizzafrenzy): a column's name, width and alignment (a
	// column that is not there is left alone)
	void setColumnName(int theColumn, const std::string& theName);
	void setColumnWidth(int theColumn, int theWidth);
	void setColumnAlign(int theColumn, int theAlign);
	// 0x48f010 (pizzafrenzy)
	void setRows(int theRows);
	void setBackgroundColor(const Color4& theColor) { mBackgroundColor = theColor; }   // 0x48f050 (pizzafrenzy)
	const Color4& getBackgroundColor() const { return mBackgroundColor; }              // 0x48f080 (pizzafrenzy)
	void setTextColor(const Color4& theColor) { mTextColor = theColor; }               // 0x48f090 (pizzafrenzy)
	const Color4& getTextColor() const { return mTextColor; }                          // 0x48f0c0 (pizzafrenzy)
	// 0x48f0d0 (pizzafrenzy)
	void setSelectionBackgroundColor(const Color4& theColor) { mSelectionBackgroundColor = theColor; }
	// 0x48f100 (pizzafrenzy)
	void setSelectionTextColor(const Color4& theColor) { mSelectionTextColor = theColor; }

	// 0x48fd90 (pizzafrenzy): the table shows the model
	void setModel(TableModel* theModel);
	TableModel* getModel() { return mModel; }
	int getRows() const { return mRows; }            // 0x48dab0 (pizzafrenzy)
	void setTopRow(int theRow) { mTopRow = theRow; }  // 0x48f030 (pizzafrenzy): the first row shown
	int getTopRow() const { return mTopRow; }         // 0x48f040 (pizzafrenzy)
	int rowCount() { return mModel->rowCount(); }     // 0x48f1c0 (pizzafrenzy)

	// 0x6c: 0x48f410 (pizzafrenzy)
	void draw(Graphics* g) override;
	// 0x70: 0x48f1d0 (pizzafrenzy): the rows shown follow the model's picked row
	void update(UpdateInfo& theInfo) override;
	// 0x74: 0x48f330 (pizzafrenzy)
	void layout() override;

	Ref<TableModel> mModel;              // +0x118: null until the game sets one
	std::vector<TableColumn> mColumns;   // +0x11c
	int mRows;                           // +0x12c
	int mTopRow;                         // +0x130
	int mSelectedRow;                    // +0x134: the model's picked row the top row was last set by
	Color4 mBackgroundColor;             // +0x138
	Color4 mTextColor;                   // +0x148
	Color4 mSelectionBackgroundColor;    // +0x158
	Color4 mSelectionTextColor;          // +0x168
};

} // namespace engine

#endif
