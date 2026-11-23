#include "ListModel.h"

#include "UserManager.h"

using namespace engine;

int UserListModel::getCount()
{
	return UserManager::instance()->getUserCount();
}

std::string UserListModel::getItem(int theIndex)
{
	UserInfo* aUser = UserManager::instance()->getUser(theIndex);
	return aUser != nullptr ? aUser->getName() : std::string();
}
