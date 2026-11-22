#include "GenericUserSelectScreen.h"

#include "DrawContext.h"
#include "UserManager.h"

using namespace engine;

namespace
{
// 0x566bc0 (feedingfrenzy2): the colour added to a button that cannot be used
const float UNUSABLE_SHADE = -0.2f;
} // namespace

void GenericUserSelectScreen::enable()
{
	ScreenLayout::enable();
	mUserList->setSelected(UserManager::instance()->getCurrentIndex());
	updateButtons();
}

void GenericUserSelectScreen::init()
{
	mUserList = static_cast<ListBox*>(findComponent("userList"));
	mUserList->setModel(new UserListModel());
	mUserList->setSelected(UserManager::instance()->getCurrentIndex());
	updateButtons();
}

void GenericUserSelectScreen::selectUser()
{
	UserManager::instance()->setCurrentUser(mUserList->getSelected());
}

void GenericUserSelectScreen::deleteUser()
{
	UserManager* aUsers = UserManager::instance();
	aUsers->removeUser(mUserList->getSelected());
	mUserList->setSelected(aUsers->getCurrentIndex());
	updateButtons();
}

bool GenericUserSelectScreen::isFull()
{
	return UserManager::instance()->getUserCount() >= mUserList->getVisibleRows();
}

int GenericUserSelectScreen::getRows()
{
	return mUserList->getVisibleRows();
}

void GenericUserSelectScreen::updateButtons()
{
	Component* aNewButton = findComponent("new");
	Component* aDeleteButton = findComponent("delete");
	Component* aSelectButton = findComponent("select");
	if (aNewButton != nullptr)
		setUsable(aNewButton, mUserList->getCount() < mUserList->getVisibleRows());
	if (aDeleteButton != nullptr)
		setUsable(aDeleteButton, mUserList->getCount() > 0);
	if (aSelectButton != nullptr)
		setUsable(aSelectButton, mUserList->getCount() > 0);
}

void GenericUserSelectScreen::setUsable(Component* theButton, bool theUsable)
{
	if (theUsable)
	{
		theButton->enable();
		theButton->setColorMode(COLOR_MODE_NONE);
		return;
	}
	theButton->disable();
	theButton->setColorMode(COLOR_MODE_ADD);
	theButton->setColor(UNUSABLE_SHADE, UNUSABLE_SHADE, UNUSABLE_SHADE);
}
