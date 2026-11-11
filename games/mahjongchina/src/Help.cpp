#include "Help.h"

#include "Font.h"
#include "JBBase.h"
#include "Manager.h"
#include "Res.h"

namespace mahjongchina
{

// 0x40eac0
void HelpExample::Init()
{
	mPointsText = jb::LoadDllString(STRING_PICTURE_FREE_TILE);
	mTimeText = jb::LoadDllString(STRING_PICTURE_NOT_FREE);
	int aPos = 0;
	std::string aText = jb::LoadDllString(STRING_PICTURE_SEASON_NAME);
	std::string* aName = mNames1;
	int i = 4;
	do
	{
		*aName = jb::Tokenize(aText, 9, aPos);
		aName = aName + 1;
		i = i - 1;
	} while (i != 0);
	aPos = 0;
	aText = jb::LoadDllString(STRING_PICTURE_FLOWER_NAME);
	aName = mNames2;
	i = 4;
	do
	{
		*aName = jb::Tokenize(aText, 9, aPos);
		aName = aName + 1;
		i = i - 1;
	} while (i != 0);
	mFont = jb::GetDllFont(FONT_TEXT_ALT, FONT_TEXT_ALT_MAP);
}

// 0x40ebf0 (the points example draws text 0x5e in three places and 0x67 in one, as the original does)
void HelpExample::Draw()
{
	switch (mExample)
	{
	case 1:
	case 4:
		DrawImage(0xf, 0xb, mExample == 1 ? 0x166 : 0x167, nullptr, true, 100, 100, 0xff);
		mFont->DrawRight(this, 0x23, 0x76, mPointsText.c_str(), 100, 100, 0xff);
		mFont->DrawLeft(this, 0xa0, 0x75, mPointsText.c_str(), -1, 100, 100, 0xff);
		mFont->DrawRight(this, 0x87, 0x19, mTimeText.c_str(), 100, 100, 0xff);
		mFont->DrawLeft(this, 0xfa, 0x32, mPointsText.c_str(), -1, 100, 100, 0xff);
		return;
	case 2:
	case 5:
		DrawTile(0, 0, 0x23);
		mFont->DrawCentred(this, 0, 0x4b, mNames1[0].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0x50, 0, 0x24);
		mFont->DrawCentred(this, 0x50, 0x4b, mNames1[1].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0xa0, 0, 0x25);
		mFont->DrawCentred(this, 0xa0, 0x4b, mNames1[2].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0xf0, 0, 0x26);
		mFont->DrawCentred(this, 0xf0, 0x4b, mNames1[3].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0, 0x55, 0x27);
		mFont->DrawCentred(this, 0, 0xa0, mNames2[0].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0x50, 0x55, 0x28);
		mFont->DrawCentred(this, 0x50, 0xa0, mNames2[1].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0xa0, 0x55, 0x29);
		mFont->DrawCentred(this, 0xa0, 0xa0, mNames2[2].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0xf0, 0x55, 0x2a);
		mFont->DrawCentred(this, 0xf0, 0xa0, mNames2[3].c_str(), 0x30, 100, 100, 0xff);
		return;
	case 3:
		DrawSpecial(-5, -0x3f, 0x2b);
		DrawSpecial(-0x2f, -0x3f, 0x2b);
		return;
	case 6:
		DrawSpecial(-5, -0x21, 0x2b);
		DrawSpecial(-0x2f, -0x21, 0x2b);
		DrawSpecial(-5, 0x48, 0x2c);
		DrawSpecial(-0x2f, 0x48, 0x2d);
		return;
	case 7:
		DrawSpecial(-0x2a, -0x49, 0x2f);
		DrawSpecial(-0x2a, 0x2a, 0x2e);
	}
}

// 0x40ef30
void HelpExample::DrawSpecial(int theX, int theY, int theKind)
{
	jb::Rect aRect;
	aRect.left = 0;
	if (theKind == 0x2b)
		aRect.left = 1;
	else if (0x2b < theKind)
		aRect.left = theKind - 0x2a;
	aRect.left = aRect.left * 0x30;
	aRect.right = aRect.left + 0x30;
	aRect.top = 0;
	aRect.bottom = 0x3a;
	DrawImageKeyed(theX, theY, IMAGE_SPECIAL_FACES, 0xff00ff, &aRect, true, 100, 100, 0xff);
}

// 0x40efa0
void HelpExample::DrawTile(int theX, int theY, int theKind)
{
	jb::Rect aRect;
	aRect.left = 0;
	aRect.top = 0;
	aRect.right = 0x30;
	aRect.bottom = 0x3a;
	DrawImageKeyed(theX, theY, IMAGE_SPECIAL_FACES, 0xff00ff, &aRect, true, 100, 100, 0xff);
	aRect.left = ((theKind - 1) % 6) * 0x26;
	aRect.right = aRect.left + 0x26;
	aRect.top = ((theKind - 1) / 6) * 0x30;
	aRect.bottom = aRect.top + 0x30;
	DrawImage(theX + 7, theY + 3, mTiles, &aRect, true, 100, 100, 0xff);
}

// 0x40e150
void HelpFree::Create()
{
	int aLeft[] = {3, 3, 3, 0};
	int aRight[] = {3, 3, 3, 0};
	int aTop[] = {2, 2, 3, 3, 0};
	int aBottom[] = {3, 2, 2, 3, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* anOk = new DialogButton(1);
	Add(anOk, 200, 0x14a);
	anOk->SetText(STRING_TIP_DONE);
	mPrevious = new jb::Button(IMAGE_HELP_PREV_BUTTON, 4, false);
	Add(mPrevious, 0x32, 0x147);
	mPrevious->SetEnabled(false);
	mNext = new jb::Button(IMAGE_HELP_NEXT_BUTTON, 4, false);
	Add(mNext, 0x17c, 0x147);
	mExample = new HelpExample(0xd7);
	Add(mExample, 0x61, 0x9b);
	ShowPage();
}

// 0x40e330
bool HelpFree::OnCommand(int theCmd)
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
		if (mPage < 4)
		{
			mPage = mPage + 1;
			ShowPage();
			return true;
		}
	}
	return true;
}

// 0x40e390
void HelpFree::ShowPage()
{
	ClearTexts();
	switch (mPage)
	{
	case 0:
		SetTitle(STRING_HELP_OVERVIEW_TITLE);
		AddText(STRING_HELP_OVERVIEW_TEXT, 1, 0x32, 0x50, false);
		mExample->mExample = 1;
		break;
	case 1:
		SetTitle(STRING_HELP_FAMILIES_TITLE);
		AddText(STRING_HELP_FAMILIES_TEXT, 1, 0x28, 0x50, false);
		mExample->mExample = 2;
		break;
	case 2:
		SetTitle(STRING_HELP_HINT_TITLE);
		AddText(STRING_HELP_HINT_TEXT, 1, 0x2d, 0x55, false);
		mExample->mExample = 0;
		break;
	case 3:
		SetTitle(STRING_HELP_SAVED_TITLE);
		AddText(STRING_HELP_SAVED_TEXT, 1, 0x2d, 0x55, false);
		mExample->mExample = 0;
		break;
	case 4:
		SetTitle(STRING_HELP_TIPS_TITLE);
		AddText(STRING_HELP_TIPS_TEXT, 1, 0x32, 0x55, false);
		mExample->mExample = 0;
	}
	mPrevious->SetEnabled(mPage != 0);
	mNext->SetEnabled(mPage != 4);
}

// 0x40e520
void HelpEscape::Create()
{
	int aLeft[] = {3, 3, 3, 0};
	int aRight[] = {3, 3, 3, 0};
	int aTop[] = {2, 2, 3, 3, 0};
	int aBottom[] = {3, 2, 2, 3, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* anOk = new DialogButton(1);
	Add(anOk, 200, 0x14a);
	anOk->SetText(STRING_HELP_DONE);
	mPrevious = new jb::Button(IMAGE_HELP_PREV_BUTTON, 4, false);
	Add(mPrevious, 0x32, 0x147);
	mPrevious->SetEnabled(false);
	mNext = new jb::Button(IMAGE_HELP_NEXT_BUTTON, 4, false);
	Add(mNext, 0x17c, 0x147);
	mExample = new HelpExample(0x65);
	Add(mExample, 0x61, 0x9b);
	ShowPage();
}

// 0x40e700
bool HelpEscape::OnCommand(int theCmd)
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
		if (mPage < 5)
		{
			mPage = mPage + 1;
			ShowPage();
			return true;
		}
	}
	return true;
}

// 0x40e760
void HelpEscape::ShowPage()
{
	ClearTexts();
	switch (mPage)
	{
	case 0:
		SetTitle(STRING_HELP_ADVENTURE_TITLE);
		AddText(STRING_HELP_ADVENTURE_GOAL, 1, 0xa0, 0x5f, false);
		AddText(STRING_HELP_ADVENTURE_TEXT, 1, 0x32, 0xd2, false);
		mExample->mExample = 3;
		break;
	case 1:
		SetTitle(STRING_HELP_MATCHING_TITLE);
		AddText(STRING_HELP_MATCHING_TEXT, 1, 0x32, 0x50, false);
		mExample->mExample = 4;
		break;
	case 2:
		SetTitle(STRING_HELP_FAMILIES_TITLE);
		AddText(STRING_HELP_FAMILIES_TEXT, 1, 0x28, 0x50, false);
		mExample->mExample = 5;
		break;
	case 3:
		SetTitle(STRING_HELP_MAGIC_TITLE);
		AddText(STRING_HELP_MAGIC_INTRO, 1, 0x32, 0x55, false);
		AddText(STRING_HELP_MAGIC_GOLD, 1, 0x9b, 0x87, false);
		AddText(STRING_HELP_MAGIC_KEY, 1, 0x9b, 0xf0, false);
		mExample->mExample = 6;
		break;
	case 4:
		SetTitle(STRING_HELP_MAGIC_TITLE);
		AddText(STRING_HELP_MAGIC_MAGNET, 1, 0x7d, 0x5f, false);
		AddText(STRING_HELP_MAGIC_JOKER, 1, 0x7d, 0xd2, false);
		AddText(STRING_HELP_MAGIC_HINT, 1, 0x37, 0x136, false);
		mExample->mExample = 7;
		break;
	case 5:
		SetTitle(STRING_HELP_SCORING_TITLE);
		AddText(STRING_HELP_SCORING_TEXT, 1, 0x32, 100, false);
		mExample->mExample = 0;
	}
	mPrevious->SetEnabled(mPage != 0);
	mNext->SetEnabled(mPage != 5);
}

// 0x40f450
void HelpPicture::Draw()
{
	if (mPicture == 1)
		DrawImage(9, 1, IMAGE_HELP_MAP_PICTURE, nullptr, true, 100, 100, 0xff);
	else if (mPicture == 2)
		DrawImage(0, 0, IMAGE_HELP_LAYOUT_PICTURE, nullptr, true, 100, 100, 0xff);
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
	anOk->SetText(STRING_HELP_MENU_DONE);
	mPrevious = new jb::Button(IMAGE_HELP_PREV_BUTTON, 4, false);
	Add(mPrevious, 0x32, 0x131);
	mPrevious->SetEnabled(false);
	mNext = new jb::Button(IMAGE_HELP_NEXT_BUTTON, 4, false);
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
		SetTitle(STRING_HELP_INTRO_TITLE);
		AddText(STRING_HELP_INTRO_TEXT, 0, 0x28, 0x50, false);
		mPicture->mPicture = 1;
		break;
	case 1:
		SetTitle(STRING_HELP_CLASSIC_TITLE);
		AddText(STRING_HELP_CLASSIC_TEXT, 0, 0x28, 0x50, false);
		mPicture->mPicture = 2;
		break;
	case 2:
		SetTitle(STRING_HELP_HINTS_TITLE);
		AddText(STRING_HELP_HINTS_TEXT, 1, 0x32, 0x55, false);
		mPicture->mPicture = 0;
		break;
	case 3:
		SetTitle(STRING_HELP_HINTS_TITLE);
		AddText(STRING_HELP_HINTS_MORE, 1, 0x32, 0x55, false);
		mPicture->mPicture = 0;
	}
	mPrevious->SetEnabled(mPage != 0);
	mNext->SetEnabled(mPage != 3);
}

} // namespace mahjongchina
