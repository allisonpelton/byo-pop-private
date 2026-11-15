#ifndef __SPROUT_USERLISTMODEL_H__
#define __SPROUT_USERLISTMODEL_H__

// The players as a list (the exe's model of the user list, 0x28 bytes, constructor 0x492210 (pizzafrenzy)): a row for
// each player, with the player's name

#include "ListModel.h"
#include "Players.h"

namespace engine
{

class UserListModel : public ListModel
{
public:
	UserListModel() : mPlayers(Players::instance()) {}  // 0x492210 (pizzafrenzy)

	// 0x04: 0x4920f0 (pizzafrenzy)
	int rowCount() override { return mPlayers->count(); }
	// 0x08: 0x492100 (pizzafrenzy): ">_<" for a row that is no player's
	std::string getText(int theRow) override;

private:
	Players* mPlayers;  // +0x1c
};

} // namespace engine

#endif
