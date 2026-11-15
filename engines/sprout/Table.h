#ifndef __SPROUT_TABLE_H__
#define __SPROUT_TABLE_H__

// The signal build's table (Pizza Frenzy's; Iggle Pop's Mac build names it engine::Table: a component with slots; 0x180
// bytes, constructor 0x490150 (pizzafrenzy), vtable 0x504560 (pizzafrenzy)): columns, each with a name, a width and an
// alignment, and a number of rows of the model's text. A layout's table element makes it and its column elements add
// the columns. Only what the layout reader reaches is here: the constructor, the colours, the rows and the columns. Its
// drawing (0x90: 0x48f410 (pizzafrenzy)), layout (0x98: 0x48f330 (pizzafrenzy)), mouse events, its model (+0x118, the
// original's TableModel, which a game sets) and its signals are not translated, nothing reaches them yet, so a table
// draws nothing here.

#include "Color4.h"
#include "Node.h"
#include "Signal.h"
#include <string>
#include <vector>

namespace engine
{

class TableModel;

// A column (0x28 bytes in the original, whose last word nothing here reads)
struct TableColumn
{
	std::string mName;  // +0x00
	int mWidth;         // +0x1c
	int mAlign;         // +0x20: 0 left, 1 centre, 2 right

	TableColumn() : mWidth(0), mAlign(0) {}
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

	// 0x90: 0x48f410 (pizzafrenzy), not translated
	void draw(Graphics* g) override {}
	// 0x98: 0x48f330 (pizzafrenzy), not translated
	void layout() override {}

	TableModel* mModel;                  // +0x118: null until the game sets one
	std::vector<TableColumn> mColumns;   // +0x11c
	int mRows;                           // +0x12c
	int mTopRow;                         // +0x130: set to 0 by the constructor, nothing translated reads it
	Color4 mBackgroundColor;             // +0x138
	Color4 mTextColor;                   // +0x148
	Color4 mSelectionBackgroundColor;    // +0x158
	Color4 mSelectionTextColor;          // +0x168
};

} // namespace engine

#endif
