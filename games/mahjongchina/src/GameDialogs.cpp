#include "GameDialogs.h"

#include "Board.h"
#include "Font.h"
#include "Help.h"
#include "LayoutSelect.h"
#include "MainMenu.h"
#include "Manager.h"
#include "Options.h"
#include "PlayLayer.h"
#include "User.h"

namespace mahjongchina
{

// The places of the tips (0x51bab8)
static const int gTipPos[9][2] = {{0xa0, 0x55}, {5, 0xc3}, {10, 0x8c}, {0x8c, 0x50}, {0xca, 0xaa},
	{0xe6, 0x78}, {0x82, 0x118}, {0x15e, 0x50}, {0xa0, 0x55}};

// 0x4069a0
void FreeMenu::Create()
{
	SetClickCmd(0x12);
	int aLeft[] = {3, 1, 3, 2, 0};
	int aRight[] = {1, 2, 3, 3, 0};
	int aTop[] = {2, 1, 0};
	int aBottom[] = {1, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* aButton = new DialogButton(0xb, 0xbe);
	Add(aButton, 0x26, 0x24);
	aButton->SetText(0x2c);
	aButton = new DialogButton(0xc, 0xbe);
	Add(aButton, 0x26, 0x51);
	aButton->SetText(0x2d);
	aButton = new DialogButton(0xf, 0xbe);
	Add(aButton, 0x26, 0x8f);
	aButton->SetText(0x2f);
	aButton = new DialogButton(0x10, 0xbe);
	Add(aButton, 0x26, 0xbc);
	aButton->SetText(0x30);
	aButton = new DialogButton(0x11, 0xbe);
	Add(aButton, 0x26, 0xe9);
	aButton->SetText(0x29);
	aButton = new DialogButton(0xe, 0xbe);
	Add(aButton, 0x26, 0x127);
	aButton->SetText(0x2b);
	aButton = new DialogButton(0x12, 0xbe);
	Add(aButton, 0x26, 0x154);
	aButton->SetText(0x2a);
}

// 0x406c70
bool FreeMenu::OnCommand(int theCmd)
{
	jb::Layer* aLayer;
	YesNoBox* aBox;
	switch (theCmd - 0xb)
	{
	case 0:
		if (!mOwner->HasSave())
		{
			jb::gManager->SlideMusic(0, 0x44c);
			jb::gManager->SwitchTo(new MainMenu());
			return true;
		}
		aBox = new YesNoBox(mOwner, 0, 0xb, 0x1b, 0x16, true, 0x66, 0x66);
		break;
	case 1:
		if (!mOwner->HasSave())
		{
			jb::gManager->SlideMusic(0, 0x44c);
			jb::gManager->SwitchTo(new LayoutSelect());
			return true;
		}
		aBox = new YesNoBox(mOwner, 0, 0xc, 0x1b, 0x16, true, 0x66, 0x66);
		break;
	default:
		return false;
	case 3:
		jb::gManager->Pop(true, 1);
		jb::gManager->Push(new HelpFree(), true, true, true);
		return true;
	case 4:
		jb::gManager->Pop(true, 1);
		aLayer = new OptionsBox(0);
		goto push;
	case 5:
		jb::gManager->Pop(true, 1);
		aLayer = new StyleBox(0);
		goto push;
	case 6:
		jb::gManager->Pop(true, 1);
		aLayer = new BackgroundBox();
		goto push;
	case 7:
		jb::gManager->Pop(true, 1);
		return true;
	}
	jb::gManager->Replace(aBox, false, true);
	aBox->SetTitle(0x18);
	aBox->AddText(0x17, 0, 0, 0x50, false, nullptr, 0);
	return true;
push:
	jb::gManager->Push(aLayer, true, false, true);
	return true;
}

// The message of new records (0x406f90, 0x407cd0): 0xaf both, 0xb0 the score, 0xb2 the time
static void FormatRecords(std::string& theMessage, bool theNewTime, bool theNewScore)
{
	int anId;
	if (!theNewScore && !theNewTime)
		return;
	if (!theNewScore)
	{
		if (!theNewTime)
			goto format;
		anId = 0xb2;
	}
	else if (!theNewTime)
		anId = 0xb0;
	else
		anId = 0xaf;
	{
		std::string aText = jb::LoadDllString(anId);
		theMessage = jb::FormatDll(0xae, aText.c_str());
		return;
	}
format:
	theMessage = jb::FormatDll(0xae, "");
}

// 0x406f90
ResultBox::ResultBox(int theTime, int theBestTime, bool theNewTime, int theScore, int theBestScore,
	bool theNewScore) : Dialog(0x87, 100)
{
	mTime = jb::FormatTime(theTime);
	mScore = jb::FormatNumber(theScore);
	mBestScore = jb::FormatNumber(theBestScore);
	mBestTime = jb::FormatTime(theBestTime);
	FormatRecords(mMessage, theNewTime, theNewScore);
}

// 0x407180
void ResultBox::Create()
{
	int aLeft[] = {2, 1, 0};
	int aRight[] = {2, 1, 0};
	int aTop[] = {1, 3, 2, 3, 2, 0};
	int aBottom[] = {3, 1, 2, 3, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(0xb8);
	std::string aLabel = jb::LoadDllString(0xb7);
	AddText(aLabel.c_str(), 1, 0x32, 0x55, false, nullptr, 0);
	AddText(mTime.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x38, 0x55, true, nullptr, 0);
	aLabel = jb::LoadDllString(0xb3);
	AddText(aLabel.c_str(), 1, 300, 0x55, false, nullptr, 0);
	AddText(mBestTime.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x132, 0x55, true, nullptr, 0);
	aLabel = jb::LoadDllString(0xb6);
	AddText(aLabel.c_str(), 1, 0x32, 0x73, false, nullptr, 0);
	AddText(mScore.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x38, 0x73, true, nullptr, 0);
	aLabel = jb::LoadDllString(0xb1);
	AddText(aLabel.c_str(), 1, 300, 0x73, false, nullptr, 0);
	AddText(mBestScore.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x132, 0x73, true, nullptr, 0);
	if (!mMessage.empty())
		AddText(mMessage.c_str(), 0, 0, 0x9b, false, nullptr, 0);
	DialogButton* aButton = new DialogButton(1, 200);
	Add(aButton, 0x3e, 0xaf);
	aButton->SetText(0xb4);
	aButton = new DialogButton(2, 200);
	Add(aButton, 0x10b, 0xaf);
	aButton->SetText(0xb5);
	jb::LoadSound(0x177, false);
	jb::PlaySound(0x177);
}

// 0x407490
bool ResultBox::OnCommand(int theCmd)
{
	if (theCmd == 1)
	{
		jb::gManager->SwitchTo(new LayoutSelect());
		return true;
	}
	if (theCmd - 2 != 0)
		return false;
	PlayLayer* aPlay = (PlayLayer*)jb::gManager->Pop(true, 1);
	aPlay->PlayNext();
	return true;
}

// 0x407590
void QuitNote::Create()
{
	int aLeft[] = {2, 0};
	int aRight[] = {2, 0};
	int aTop[] = {3, 3, 3, 0};
	int aBottom[] = {3, 3, 3, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(0x1f);
	AddText(0x1d, 0, 0, 0x5a, false, nullptr, 0);
	mDontShow = new jb::CheckBox(0x12a, 2);
	Add(mDontShow, 0x2d, 0x82);
	jb::SetCheckLabel(mDontShow, 0x1e, 0x122, 0x121, 0x20, 0x1f, 0xff);
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0x131, 0x8c);
	aButton->SetText(0x7c);
}

// 0x4076f0
bool QuitNote::OnCommand(int theCmd)
{
	if (theCmd == 1)
	{
		gUser.mShowQuitNote = !mDontShow->IsChecked();
		jb::gManager->Pop(true, 1);
		jb::gManager->SlideMusic(0, 0x44c);
		jb::gManager->SwitchTo(new MainMenu());
		return true;
	}
	return false;
}

// 0x407800
void EscapeMenu::Create()
{
	SetClickCmd(5);
	int aLeft[] = {2, 3, 1, 0};
	int aRight[] = {3, 1, 2, 0};
	int aTop[] = {2, 0};
	int aBottom[] = {2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* aButton = new DialogButton(1, 0x96);
	Add(aButton, 0x23, 0x29);
	aButton->SetText(0x2c);
	aButton = new DialogButton(3, 0x96);
	Add(aButton, 0x23, 0x67);
	aButton->SetText(0x2f);
	aButton = new DialogButton(4, 0x96);
	Add(aButton, 0x23, 0x94);
	aButton->SetText(0x30);
	aButton = new DialogButton(2, 0x96);
	Add(aButton, 0x23, 0xd2);
	aButton->SetText(0x2b);
	aButton = new DialogButton(5, 0x96);
	Add(aButton, 0x23, 0xff);
	aButton->SetText(0x2a);
}

// 0x407a20
bool EscapeMenu::OnCommand(int theCmd)
{
	jb::Layer* aLayer;
	switch (theCmd - 1)
	{
	case 0:
		if (mOwner->HasSave())
		{
			YesNoBox* aBox = new YesNoBox(mOwner, 0, 1, 0x1b, 0x16, true, 0x66, 0x66);
			jb::gManager->Replace(aBox, false, true);
			aBox->SetTitle(0x18);
			aBox->AddText(0x17, 0, 0, 0x50, false, nullptr, 0);
			return true;
		}
		if (!gUser.mShowQuitNote)
		{
			jb::gManager->SlideMusic(0, 0x44c);
			jb::gManager->SwitchTo(new MainMenu());
			return true;
		}
		jb::gManager->Replace(new QuitNote(), false, true);
		return true;
	case 1:
		jb::gManager->Pop(true, 1);
		jb::gManager->Push(new HelpEscape(), true, true, true);
		return true;
	case 2:
		jb::gManager->Pop(true, 1);
		aLayer = new OptionsBox(0);
		break;
	case 3:
		jb::gManager->Pop(true, 1);
		aLayer = new StyleBox(1);
		break;
	case 4:
		jb::gManager->Pop(true, 1);
		return true;
	default:
		return false;
	}
	jb::gManager->Push(aLayer, true, false, true);
	return true;
}

// 0x407f10
LevelDone::LevelDone(int theTime, int theScore, const char* theNextTitle) : Dialog(0x7d, 0x96),
	mNextTitle(theNextTitle)
{
	mTime = jb::FormatTime(theTime);
	mScore = jb::FormatNumber(theScore);
}

// 0x408040
void LevelDone::Create()
{
	int aLeft[] = {3, 3, 0};
	int aRight[] = {3, 3, 0};
	int aTop[] = {2, 2, 1, 3, 0};
	int aBottom[] = {3, 2, 2, 1, 0};
	SetCorners(2, 2, 1, 1);
	SetEdges(aLeft, aRight, aTop, aBottom);
	std::string aLabel;
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0xea, 0xe0);
	aButton->SetText(0x23);
	SetTitle(0x26);
	aLabel = jb::LoadDllString(0x25);
	AddText(aLabel.c_str(), 1, 0x82, 0x55, false, nullptr, 0);
	AddText(mTime.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x88, 0x55, true, nullptr, 0);
	aLabel = jb::LoadDllString(0x24);
	AddText(aLabel.c_str(), 1, 0x122, 0x55, false, nullptr, 0);
	AddText(mScore.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x128, 0x55, true, nullptr, 0);
	Add(new LevelDoneScroll(mNextTitle.c_str()), 0x78, 0x67);
	jb::LoadSound(0x175, false);
	jb::PlaySound(0x175);
}

// 0x408240
bool LevelDone::OnCommand(int theCmd)
{
	if (theCmd - 1 != 0)
		return false;
	PlayLayer* aPlay = (PlayLayer*)jb::gManager->Pop(true, 1);
	aPlay->ShowMap();
	return true;
}

// 0x408380
void LevelDoneScroll::Draw()
{
	DrawImage(0, 0, 0x12d, nullptr, true, 100, 100, mAlpha);
	mTextFont->DrawCentred(this, -3, 0x16, mNextLabel.c_str(), 0x136, 100, 100, mAlpha);
	mTextFont->DrawWrappedCentred(this, 0x14, mLines < 3 ? 0x36 : 0x2c, 0x10e, 1000, mNextTitle.c_str(), 0x15,
		false, mAlpha);
}

// 0x408410
void LevelDoneScroll::Init()
{
	mNextLabel = jb::LoadDllString(0x22);
	mTextFont = jb::GetDllFont(0x12e, 0x12f, -1);
	mLines = mTextFont->DrawWrappedCentred(this, 0x14, 0x38, 0x10e, 1000, mNextTitle.c_str(), 0x14, true, 0xff);
	RegUpdate(true);
}

// 0x408470
bool LevelDoneScroll::Update()
{
	if (0 < mDelay)
	{
		mDelay = mDelay - GetDelta();
		return false;
	}
	if (mAlpha < 0xff)
	{
		mAlpha = mAlpha + (GetDelta() * 0xff) / 0x5dc;
		if (0xff < mAlpha)
		{
			mAlpha = 0xff;
			RegUpdate(false);
		}
	}
	return true;
}

// 0x408d20
void TipPicture::Init()
{
	if (mImage == 0x3f)
	{
		int aPos = 0;
		std::string aText = jb::LoadDllString(0x69);
		int i = 0;
		int aCount = 4;
		do
		{
			mNames1[i] = jb::Tokenize(aText, 9, aPos);
			i++;
			aCount--;
		} while (aCount != 0);
		aPos = 0;
		aText = jb::LoadDllString(0x5d);
		i = 0;
		aCount = 4;
		do
		{
			mNames2[i] = jb::Tokenize(aText, 9, aPos);
			i++;
			aCount--;
		} while (aCount != 0);
		mFont = jb::GetDllFont(0x133, 0x132, -1);
		return;
	}
	if (mImage != 0x172)
	{
		mLabel1 = jb::LoadDllString(0x5e);
		mLabel2 = jb::LoadDllString(0x67);
		mFont = jb::GetDllFont(0x133, 0x132, -1);
		return;
	}
	RegUpdate(true);
}

// 0x408ea0: the arrow bobs 10 pixels each way, a pixel every 25 ms
bool TipPicture::Update()
{
	uint32_t aStep = (uint32_t)GetTime() / 0x19 % 0x14;
	mBob = (int)aStep;
	if (10 < aStep)
		mBob = 0x14 - (int)aStep;
	return true;
}

// 0x408ee0
void TipPicture::Draw()
{
	int anImage = mImage;
	if (anImage == 0x3f)
	{
		DrawTile(0, 0, 0x27);
		mFont->DrawCentred(this, 0, 0x4b, mNames2[0].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0x50, 0, 0x28);
		mFont->DrawCentred(this, 0x50, 0x4b, mNames2[1].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0xa0, 0, 0x29);
		mFont->DrawCentred(this, 0xa0, 0x4b, mNames2[2].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0xf0, 0, 0x2a);
		mFont->DrawCentred(this, 0xf0, 0x4b, mNames2[3].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0, 0x91, 0x23);
		mFont->DrawCentred(this, 0, 0xdc, mNames1[0].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0x50, 0x91, 0x24);
		mFont->DrawCentred(this, 0x50, 0xdc, mNames1[1].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0xa0, 0x91, 0x25);
		mFont->DrawCentred(this, 0xa0, 0xdc, mNames1[2].c_str(), 0x30, 100, 100, 0xff);
		DrawTile(0xf0, 0x91, 0x26);
		mFont->DrawCentred(this, 0xf0, 0xdc, mNames1[3].c_str(), 0x30, 100, 100, 0xff);
		return;
	}
	if (anImage != 0x172)
	{
		DrawImage(0xf, 0xb, anImage, nullptr, true, 100, 100, 0xff);
		mFont->DrawRight(this, 0x23, 0x76, mLabel1.c_str(), 100, 100, 0xff);
		mFont->DrawLeft(this, 0xa0, 0x75, mLabel1.c_str(), -1, 100, 100, 0xff);
		mFont->DrawRight(this, 0x87, 0x19, mLabel2.c_str(), 100, 100, 0xff);
		mFont->DrawLeft(this, 0xfa, 0x32, mLabel1.c_str(), -1, 100, 100, 0xff);
		return;
	}
	DrawImage(0, mBob, 0x172, nullptr, true, 100, 100, 0xff);
}

// 0x4091b0
void TipPicture::DrawTile(int theX, int theY, int theKind)
{
	jb::Rect aRect = {0, 0, 0x30, 0x3a};
	DrawImageKeyed(theX, theY, 0x66, 0xff00ff, &aRect, true, 100, 100, 0xff);
	aRect.left = ((theKind - 1) % 6) * 0x26;
	aRect.right = aRect.left + 0x26;
	aRect.top = ((theKind - 1) / 6) * 0x30;
	aRect.bottom = aRect.top + 0x30;
	DrawImage(theX + 7, theY + 3, 0x65, &aRect, true, 100, 100, 0xff);
}

// 0x408670
TipBox::TipBox(int theId, Board* theBoard) : Dialog(gTipPos[theId][0], gTipPos[theId][1]), mId(theId),
	mBoard(theBoard)
{
}

// 0x4086e0
void TipBox::Create()
{
	SetCorners(0, 1, 1, 0);
	int aLeft[] = {2, 2, 0, 0, 0};
	int aRight[] = {2, 2, 0, 0, 0};
	int aTop[] = {2, 2, 3, 0, 0};
	int aBottom[] = {3, 2, 2, 0, 0};
	int anX = 0;  // the ids are 0-8: the original never leaves these unset
	int aY = 0;
	switch (mId)
	{
	case 0:
	case 8:
		anX = 200;
		aY = 0x132;
		aTop[3] = 3;
		aBottom[3] = 3;
		aLeft[1] = 3;
		aLeft[2] = 3;
		aRight[1] = 3;
		aRight[2] = 3;
		SetTitle(0x38);
		AddText(0x37, 1, 0x32, 0x50, false, nullptr, 0);
		Add(new TipPicture(0x167 - (mId != 0)), 0x61, 0x87);
		goto edges;
	case 1:
		anX = 0x8e;
		aY = 0x76;
		aTop[0] = 1;
		aBottom[1] = 1;
		aLeft[0] = 1;
		aLeft[1] = 0;
		aRight[0] = 1;
		aRight[1] = 0;
		SetTitle(0x34);
		AddText(0x33, 1, 0x28, 0x50, false, nullptr, 0);
		break;
	case 2:
		anX = 0x8e;
		aY = 0x9e;
		aTop[0] = 1;
		aBottom[1] = 1;
		aLeft[0] = 3;
		aLeft[1] = 0;
		aRight[0] = 3;
		aRight[1] = 0;
		SetTitle(0x36);
		AddText(0x35, 1, 0x37, 0x50, false, nullptr, 0);
		break;
	case 3:
		anX = 0x99;
		aY = 0xa0;
		aTop[0] = 2;
		aBottom[1] = 2;
		aLeft[0] = 3;
		aLeft[1] = 0;
		aRight[0] = 3;
		aRight[1] = 0;
		SetTitle(0x3a);
		AddText(0x39, 1, 0x2d, 0x53, false, nullptr, 0);
		break;
	case 4:
		anX = 0x98;
		aY = 0x9e;
		aLeft[0] = 3;
		aLeft[1] = 0;
		aRight[0] = 3;
		aRight[1] = 0;
		SetTitle(0x3c);
		AddText(0x3b, 1, 0x2d, 0x53, false, nullptr, 0);
		break;
	case 5:
		anX = 0x82;
		aY = 0x6e;
		aTop[0] = 1;
		aTop[1] = 1;
		aBottom[1] = 1;
		aBottom[2] = 1;
		aLeft[0] = 1;
		aLeft[1] = 0;
		aRight[0] = 1;
		aRight[1] = 0;
		AddText(0x40, 0, 0x28, 0x3c, false, nullptr, 0);
		goto edges;
	case 6:
		anX = 0x8e;
		aY = 0x8a;
		aTop[0] = 1;
		aBottom[1] = 1;
		aLeft[0] = 2;
		aLeft[1] = 0;
		aRight[0] = 2;
		aRight[1] = 0;
		SetTitle(0x42);
		AddText(0x41, 1, 0x28, 0x50, false, nullptr, 0);
		Add(new TipPicture(0x172), 0x4c, 0xc3);
		goto edges;
	case 7:
		anX = 0xb1;
		aY = 0x15a;
		aTop[0] = 3;
		aTop[1] = 3;
		aBottom[1] = 3;
		aBottom[2] = 3;
		aLeft[0] = 3;
		aLeft[1] = 3;
		aLeft[2] = 2;
		aLeft[3] = 1;
		aRight[0] = 3;
		aRight[1] = 3;
		aRight[2] = 2;
		aRight[3] = 1;
		SetTitle(0x3f);
		AddText(0x3d, 0, 0x32, 0x50, false, nullptr, 0);
		AddText(0x3e, 0, 0x32, 0xe1, false, nullptr, 0);
		Add(new TipPicture(0x3f), 0x49, 0x6e);
		break;
	default:
		goto edges;
	}
	SetAlwaysUpdate(true);
edges:
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, anX, aY);
	aButton->SetText(0x7c);
}

// 0x408b70
bool TipBox::OnCommand(int theCmd)
{
	if (theCmd == 1)
	{
		Board* aBoard = mBoard;
		jb::gManager->Pop(true, 1);
		aBoard->EndHighlight();
		return true;
	}
	return false;
}

// 0x409e70
void NoMovesBox::Create()
{
	int aLeft[] = {3, 0};
	int aRight[] = {3, 0};
	int aTop[] = {1, 3, 2, 0};
	int aBottom[] = {3, 1, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(0x7b);
	jb::Toggle* aToggle = new jb::Toggle(0x123, &mGroup, 0);
	Add(aToggle, 0x32, 0x3c);
	jb::SetCheckLabel(aToggle, 0x8a, 0x122, 0x121, 0x20, 0x14, 0xff);
	aToggle = new jb::Toggle(0x123, &mGroup, 1);
	Add(aToggle, 0x32, 0x5a);
	jb::SetCheckLabel(aToggle, 0x89, 0x122, 0x121, 0x20, 0x14, 0xff);
	aToggle = new jb::Toggle(0x123, &mGroup, 2);
	Add(aToggle, 0x32, 0x78);
	jb::SetCheckLabel(aToggle, 0x88, 0x122, 0x121, 0x20, 0x14, 0xff);
	mGroup.Select(0);
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0x8d, 0xa0);
	aButton->SetText(0x7a);
}

// 0x40a090
bool NoMovesBox::OnCommand(int theCmd)
{
	if (theCmd != 1)
		return false;
	int aChoice = mGroup.GetSelected();
	PlayLayer* aPlay = (PlayLayer*)jb::gManager->Pop(true, 1);
	if (aChoice == 0)
		aPlay->Undo();
	else if (aChoice == 1)
		aPlay->Shuffle();
	else if (aChoice == 2)
		aPlay->Restart();
	return true;
}

// 0x412160
void ErrorBox::Create()
{
	int aLeft[] = {2, 2, 0};
	int aRight[] = {2, 2, 0};
	int aTop[] = {2, 2, 3, 0};
	int aBottom[] = {3, 2, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0x98, 0xcb);
	aButton->SetText(0x7c);
}

// 0x412240
bool ErrorBox::OnCommand(int theCmd)
{
	if (theCmd == 1)
	{
		jb::gManager->Pop(true, 1);
		return true;
	}
	return false;
}

} // namespace mahjongchina
