#include "UserListModel.h"

using namespace engine;

// 0x492100 (pizzafrenzy)
std::string UserListModel::getText(int theRow)
{
	UserInfo* aUser = mPlayers->getUser(theRow);
	if (aUser != nullptr)
		return aUser->getName();
	return ">_<";
}
