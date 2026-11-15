#include "GenericUserSelectScreen.h"
#include "Players.h"
#include "UserListModel.h"

using namespace engine;

namespace
{
// A button off: its connections are dropped and it is drawn darker
void switchOff(Node* theButton)
{
	theButton->disconnect();
	theButton->setColorMode(1);
	theButton->setColor(-0.2f, -0.2f, -0.2f);
}

// A button on
void switchOn(Node* theButton)
{
	theButton->connect();
	theButton->setColorMode(0);
}
}

// 0x473190 (pizzafrenzy)
void GenericUserSelectScreen::connect()
{
	GenericScreen::connect();
	mUserList->setSelectedIndex(Players::instance()->currentIndex());
	refreshButtons();
}

// 0x4738a0 (pizzafrenzy): the screen's action signal goes to an empty slot (0x492310 (pizzafrenzy)) in the original
void GenericUserSelectScreen::setup()
{
	mUserList = static_cast<ListBox*>(getNode("userList"));
	mUserList->setModel(new UserListModel());
	mUserList->setSelectedIndex(Players::instance()->currentIndex());
	mVisibleRows = mUserList->getVisibleRows();
	mUserList->mSelectedSignal.connect(this, &GenericUserSelectScreen::onRowPicked);
	refreshButtons();
}

// 0x4731c0 (pizzafrenzy)
void GenericUserSelectScreen::choosePlayer()
{
	Players::instance()->choose(mUserList->getSelectedIndex());
}

// 0x4731e0 (pizzafrenzy)
void GenericUserSelectScreen::removePlayer()
{
	Players::instance()->remove(mUserList->getSelectedIndex());
	refreshButtons();
}

// 0x473220 (pizzafrenzy)
bool GenericUserSelectScreen::isListFull()
{
	return mVisibleRows <= Players::instance()->count();
}

// 0x473460 (pizzafrenzy)
void GenericUserSelectScreen::refreshButtons()
{
	Node* aNew = getNode("new");
	Node* aDelete = getNode("delete");
	Node* aOk = getNode("ok");
	if (aNew != nullptr)
	{
		if (mUserList->getRowCount() < mUserList->getVisibleRows())
			switchOn(aNew);
		else
			switchOff(aNew);
	}
	if (aDelete != nullptr)
	{
		if (mUserList->getRowCount() < 1 || mUserList->getSelectedIndex() < 0)
			switchOff(aDelete);
		else
			switchOn(aDelete);
	}
	if (aOk != nullptr)
	{
		if (-1 < mUserList->getSelectedIndex())
			switchOn(aOk);
		else
			switchOff(aOk);
	}
}

// 0x4736e0 (pizzafrenzy)
std::string GenericUserSelectScreen::getPickedName()
{
	UserInfo* aUser = Players::instance()->getUser(mUserList->getSelectedIndex());
	if (aUser != nullptr)
		return aUser->getName();
	return "";
}
