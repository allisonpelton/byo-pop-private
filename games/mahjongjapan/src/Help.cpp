#include "Help.h"

#include "Manager.h"

namespace mahjongjapan
{

// 0x40f450
void HelpPicture::Draw()
{
	if (mPicture == 1)
		DrawImage(9, 1, 0x164, nullptr, true, 100, 100, 0xff);
	else if (mPicture == 2)
		DrawImage(0, 0, 0x165, nullptr, true, 100, 100, 0xff);
}

// 0x40f0b0
void HelpMenu::Create()
{
	int aLeft[] = {2, 3, 3, 0};
	int aRight[] = {3, 2, 3, 0};
	int aTop[] = {2, 2, 3, 3, 0};
	int aBottom[] = {3, 2, 2, 3, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* anOk = new DialogButton(1);
	Add(anOk, 200, 0x134);
	anOk->SetText(0x61);
	mPrevious = new jb::Button(0x162, 4, false);
	Add(mPrevious, 0x32, 0x131);
	mPrevious->SetEnabled(false);
	mNext = new jb::Button(0x163, 4, false);
	Add(mNext, 0x17c, 0x131);
	mPicture = new HelpPicture();
	Add(mPicture, 0x96, 0xaa);
	ShowPage();
}

// 0x40f290
bool HelpMenu::OnCommand(int theCmd)
{
	if (theCmd == 1)
		jb::gManager->Pop(true, 1);
	else if (theCmd == 0x162)
	{
		if (0 < mPage)
		{
			mPage = mPage - 1;
			ShowPage();
			return true;
		}
	}
	else
	{
		if (theCmd - 0x163 != 0)
			return false;
		if (mPage < 3)
		{
			mPage = mPage + 1;
			ShowPage();
			return true;
		}
	}
	return true;
}

// 0x40f2f0
void HelpMenu::ShowPage()
{
	ClearTexts();
	switch (mPage)
	{
	case 0:
		SetTitle(99);
		AddText(0x62, 0, 0x28, 0x50, false, nullptr, 0);
		mPicture->mPicture = 1;
		break;
	case 1:
		SetTitle(0x60);
		AddText(0x5f, 0, 0x28, 0x50, false, nullptr, 0);
		mPicture->mPicture = 2;
		break;
	case 2:
		SetTitle(0x66);
		AddText(100, 1, 0x32, 0x55, false, nullptr, 0);
		mPicture->mPicture = 0;
		break;
	case 3:
		SetTitle(0x66);
		AddText(0x65, 1, 0x32, 0x55, false, nullptr, 0);
		mPicture->mPicture = 0;
	}
	mPrevious->SetEnabled(mPage != 0);
	mNext->SetEnabled(mPage != 3);
}

} // namespace mahjongjapan
