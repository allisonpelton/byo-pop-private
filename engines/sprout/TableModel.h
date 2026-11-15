#ifndef __SPROUT_TABLEMODEL_H__
#define __SPROUT_TABLEMODEL_H__

// What a Table shows (the exe's table model: an object with a signal at +0x0c that tells its rows changed, and five
// slots)

#include "Object.h"
#include "Signal.h"

#include <string>

namespace engine
{

class TableModel : public Object
{
public:
	// Slot 0x04
	virtual int rowCount() = 0;
	// Slot 0x08: the text of a cell, of the column of that name
	virtual std::string getCell(int theRow, const std::string& theColumn) = 0;
	// Slot 0x0c: the rows can be shown
	virtual bool isReady() = 0;
	// Slot 0x10: what the table shows instead of the rows when they are not ready
	virtual std::string getStatus() = 0;
	// Slot 0x14: the row the table shows as picked
	virtual int getSelectedRow() = 0;

	Signal<> mChanged;  // +0x0c
};

} // namespace engine

#endif
