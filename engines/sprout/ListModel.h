#ifndef __SPROUT_LISTMODEL_H__
#define __SPROUT_LISTMODEL_H__

// What a ListBox shows (the exe's list model: an object with a signal at +0x0c that tells its rows changed, and two
// slots): the number of rows and the text of one

#include "Object.h"
#include "Signal.h"

#include <string>

namespace engine
{

class ListModel : public Object
{
public:
	// Slot 0x04
	virtual int rowCount() = 0;
	// Slot 0x08
	virtual std::string getText(int theRow) = 0;

	Signal<> mChanged;  // +0x0c
};

} // namespace engine

#endif
