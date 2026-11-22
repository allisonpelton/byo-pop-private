#ifndef __SPROUT2_LISTMODEL_H__
#define __SPROUT2_LISTMODEL_H__

// What a list box shows (engine::ListModel, an Object): a count of rows and each row's text, with a signal its owner
// emits when they change. UserListModel (0x28 bytes, its constructor 0x4951c0 (feedingfrenzy2)) shows the players'
// names in their order.

#include "Object.h"
#include "sigslot.h"

#include <string>

namespace engine
{

class ListModel : public Object
{
public:
	// 0x04: the rows
	virtual int getCount() = 0;
	// 0x08: a row's text
	virtual std::string getItem(int theIndex) = 0;

	sigslot::signal0 mChangedSignal;  // +0x0c: the rows changed (a list box connected to it recomputes its bounds)
};

class UserListModel : public ListModel
{
public:
	// 0x4951c0 (feedingfrenzy2): the players' (UserManager::instance, kept at +0x1c)
	UserListModel() {}

	// 0x04 (0x484860 (feedingfrenzy2)): the players' count
	int getCount() override;
	// 0x08 (0x481960 (feedingfrenzy2)): the player's name, empty for an index out of range
	std::string getItem(int theIndex) override;
};

} // namespace engine

#endif
