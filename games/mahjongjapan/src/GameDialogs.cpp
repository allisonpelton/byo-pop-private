#include "GameDialogs.h"

#include "Board.h"
#include "Font.h"
#include "Help.h"
#include "LayoutSelect.h"
#include "MainMenu.h"
#include "Manager.h"
#include "Options.h"
#include "PlayLayer.h"
#include "Res.h"
#include "User.h"

namespace mahjongjapan
{

// The places of the tips (0x51cb30)
static const int TIP_POS[9][2] = {{0xa0, 0x55}, {0xdd, 0xc3}, {0xe3, 0x8c}, {0xe1, 0x8c}, {0xca, 0xaa},
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
	aButton->SetText(STRING_MENU_MAIN_MENU);
	aButton = new DialogButton(0xc, 0xbe);
	Add(aButton, 0x26, 0x51);
	aButton->SetText(STRING_MENU_NEW_LAYOUT);
	aButton = new DialogButton(0xf, 0xbe);
	Add(aButton, 0x26, 0x8f);
	aButton->SetText(STRING_MENU_OPTIONS);
	aButton = new DialogButton(0x10, 0xbe);
	Add(aButton, 0x26, 0xbc);
	aButton->SetText(STRING_MENU_TILE_SET);
	aButton = new DialogButton(0x11, 0xbe);
	Add(aButton, 0x26, 0xe9);
	aButton->SetText(STRING_MENU_BACKGROUND);
	aButton = new DialogButton(0xe, 0xbe);
	Add(aButton, 0x26, 0x127);
	aButton->SetText(STRING_MENU_INSTRUCTIONS);
	aButton = new DialogButton(0x12, 0xbe);
	Add(aButton, 0x26, 0x154);
	aButton->SetText(STRING_MENU_RESUME);
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
		aBox = new YesNoBox(mOwner, 0, 0xb, STRING_YES, STRING_NO, true, 0x66, 0x66);
		break;
	case 1:
		if (!mOwner->HasSave())
		{
			jb::gManager->SlideMusic(0, 0x44c);
			jb::gManager->SwitchTo(new LayoutSelect());
			return true;
		}
		aBox = new YesNoBox(mOwner, 0, 0xc, STRING_YES, STRING_NO, true, 0x66, 0x66);
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
	aBox->SetTitle(STRING_LEAVE_TITLE);
	aBox->AddText(STRING_LEAVE_TEXT, 0, 0, 0x50, false);
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
		theMessage = jb::FormatDll(STRING_NEW_BEST, aText.c_str());
		return;
	}
format:
	theMessage = jb::FormatDll(STRING_NEW_BEST, "");
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
	SetTitle(STRING_LAYOUT_COMPLETED);
	std::string aLabel = jb::LoadDllString(STRING_CURRENT_TIME_LABEL);
	AddText(aLabel.c_str(), 1, 0x32, 0x55, false);
	AddText(mTime.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x38, 0x55, true);
	aLabel = jb::LoadDllString(STRING_BEST_TIME_LABEL);
	AddText(aLabel.c_str(), 1, 300, 0x55, false);
	AddText(mBestTime.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x132, 0x55, true);
	aLabel = jb::LoadDllString(STRING_CURRENT_SCORE_LABEL);
	AddText(aLabel.c_str(), 1, 0x32, 0x73, false);
	AddText(mScore.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x38, 0x73, true);
	aLabel = jb::LoadDllString(STRING_BEST_SCORE_LABEL);
	AddText(aLabel.c_str(), 1, 300, 0x73, false);
	AddText(mBestScore.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x132, 0x73, true);
	if (!mMessage.empty())
		AddText(mMessage.c_str(), 0, 0, 0x9b, false);
	DialogButton* aButton = new DialogButton(1, 200);
	Add(aButton, 0x3e, 0xaf);
	aButton->SetText(STRING_RESULT_NEW_LAYOUT);
	aButton = new DialogButton(2, 200);
	Add(aButton, 0x10b, 0xaf);
	aButton->SetText(STRING_RESULT_NEXT_LAYOUT);
	jb::LoadSound(SOUND_RESULT, false);
	jb::PlaySound(SOUND_RESULT);
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
	SetTitle(STRING_LEAVE_ADVENTURE_TITLE);
	AddText(STRING_PROGRESS_SAVED, 0, 0, 0x5a, false);
	mDontShow = new jb::CheckBox(IMAGE_CHECK_BOX, 2);
	Add(mDontShow, 0x2d, 0x82);
	jb::SetCheckLabel(mDontShow, STRING_DONT_SHOW_AGAIN, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x1f, 0xff);
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0x131, 0x8c);
	aButton->SetText(STRING_DIALOG_OK);
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
	aButton->SetText(STRING_MENU_MAIN_MENU);
	aButton = new DialogButton(3, 0x96);
	Add(aButton, 0x23, 0x67);
	aButton->SetText(STRING_MENU_OPTIONS);
	aButton = new DialogButton(4, 0x96);
	Add(aButton, 0x23, 0x94);
	aButton->SetText(STRING_MENU_TILE_SET);
	aButton = new DialogButton(2, 0x96);
	Add(aButton, 0x23, 0xd2);
	aButton->SetText(STRING_MENU_INSTRUCTIONS);
	aButton = new DialogButton(5, 0x96);
	Add(aButton, 0x23, 0xff);
	aButton->SetText(STRING_MENU_RESUME);
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
			YesNoBox* aBox = new YesNoBox(mOwner, 0, 1, STRING_YES, STRING_NO, true, 0x66, 0x66);
			jb::gManager->Replace(aBox, false, true);
			aBox->SetTitle(STRING_LEAVE_TITLE);
			aBox->AddText(STRING_LEAVE_TEXT, 0, 0, 0x50, false);
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

// 0x407cd0
LevelDone::LevelDone(int theTime, int theBestTime, bool theNewTime, int theScore, int theBestScore,
	bool theNewScore, const char* theNextTitle) : Layer(100, 0x46), mNextTitle(theNextTitle)
{
	mTime = jb::FormatTime(theTime);
	mScore = jb::FormatNumber(theScore);
	mBestScore = jb::FormatNumber(theBestScore);
	mBestTime = jb::FormatTime(theBestTime);
	FormatRecords(mMessage, theNewTime, theNewScore);
}

// 0x407ee0
void LevelDone::Load()
{
	Add(new LevelDoneScroll(mNextTitle.c_str(), mTime.c_str(), mScore.c_str(), mBestTime.c_str(),
		mBestScore.c_str(), mMessage.c_str()), 0, 0);
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0x100, 0x13a);
	aButton->SetText(STRING_LEVEL_DONE_OK);
	jb::LoadSound(SOUND_FANFARE, false);
	jb::PlaySound(SOUND_FANFARE);
}

// 0x407ff0
bool LevelDone::OnCommand(int theCmd)
{
	if (theCmd - 1 != 0)
		return false;
	PlayLayer* aPlay = (PlayLayer*)jb::gManager->Pop(true, 1);
	aPlay->ShowMap();
	return true;
}

// 0x408020
LevelDoneScroll::LevelDoneScroll(const char* theNextTitle, const char* theTime, const char* theScore,
	const char* theBestTime, const char* theBestScore, const char* theMessage)
	: mTime(theTime), mScore(theScore), mNextTitle(theNextTitle), mBestTime(theBestTime), mBestScore(theBestScore),
	  mMessage(theMessage)
{
}

// 0x408270
void LevelDoneScroll::Draw()
{
	DrawImage(0, 0, IMAGE_LEVEL_DONE_SCROLL, nullptr, true, 100, 100, 0xff);
	mHeadFont->DrawCentred(this, 0x80, 0x82, mHeading.c_str(), 0x14f, 100, 100, 0xff);
	mValueFont->DrawLeft(this, 0x94, 0x9d, mLabelTime.c_str(), -1, 100, 100, 0xff);
	mValueFont->DrawLeft(this, 0xcc, 0x9d, mTime.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawLeft(this, 0xa1, 0xad, mLabelBest.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawLeft(this, 0xcd, 0xad, mBestTime.c_str(), -1, 100, 100, 0xff);
	mValueFont->DrawLeft(this, 0x134, 0x9d, mLabelScore.c_str(), -1, 100, 100, 0xff);
	mValueFont->DrawLeft(this, 0x177, 0x9d, mScore.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawLeft(this, 0x14d, 0xad, mLabelBest.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawLeft(this, 0x178, 0xad, mBestScore.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawCentred(this, 0x80, 0xbf, mMessage.c_str(), 0x14f, 100, 100, 0xff);
	mTextFont->DrawCentred(this, 0x80, 0xe1, mNextLabel.c_str(), 0x14f, 100, 100, mAlpha);
	mTextFont->DrawWrappedCentred(this, 0x8f, mLines < 3 ? 0x10c : 0x100, 0x136, 1000, mNextTitle.c_str(), 0x15,
		false, mAlpha);
}

// 0x4084e0
void LevelDoneScroll::Init()
{
	mHeading = jb::LoadDllString(STRING_LEVEL_COMPLETED);
	mLabelTime = jb::LoadDllString(STRING_TIME_LABEL);
	mLabelScore = jb::LoadDllString(STRING_SCORE_LABEL);
	mNextLabel = jb::LoadDllString(STRING_WISDOM_UNLOCKED);
	mLabelBest = jb::LoadDllString(STRING_BEST_PREFIX);
	mTextFont = jb::GetDllFont(FONT_RESULT, FONT_RESULT_MAP);
	mHeadFont = jb::GetDllFont(FONT_HEADING, FONT_HEADING_MAP);
	mValueFont = jb::GetDllFont(FONT_TEXT, FONT_TEXT_MAP);
	mClockFont = jb::GetDllFont(FONT_CLOCK, FONT_CLOCK_MAP);
	mSmallFont = jb::GetDllFont(FONT_SMALL, FONT_SMALL_MAP);
	mLines = mTextFont->DrawWrappedCentred(this, 0x8f, 0, 0x136, 1000, mNextTitle.c_str(), 0x14, true, 0xff);
	RegUpdate(true);
}

// 0x4085f0
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
		std::string aText = jb::LoadDllString(STRING_PICTURE_SEASON_NAME);
		int i = 0;
		int aCount = 4;
		do
		{
			mNames1[i] = jb::Tokenize(aText, 9, aPos);
			i++;
			aCount--;
		} while (aCount != 0);
		aPos = 0;
		aText = jb::LoadDllString(STRING_PICTURE_FLOWER_NAME);
		i = 0;
		aCount = 4;
		do
		{
			mNames2[i] = jb::Tokenize(aText, 9, aPos);
			i++;
			aCount--;
		} while (aCount != 0);
		mFont = jb::GetDllFont(FONT_TEXT_ALT, FONT_TEXT_ALT_MAP);
		return;
	}
	if (mImage != IMAGE_TIP_ARROW)
	{
		mLabel1 = jb::LoadDllString(STRING_PICTURE_FREE_TILE);
		mLabel2 = jb::LoadDllString(STRING_PICTURE_NOT_FREE);
		mFont = jb::GetDllFont(FONT_TEXT_ALT, FONT_TEXT_ALT_MAP);
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
	if (anImage != IMAGE_TIP_ARROW)
	{
		DrawImage(0xf, 0xb, anImage, nullptr, true, 100, 100, 0xff);
		mFont->DrawRight(this, 0x23, 0x76, mLabel1.c_str(), 100, 100, 0xff);
		mFont->DrawLeft(this, 0xa0, 0x75, mLabel1.c_str(), -1, 100, 100, 0xff);
		mFont->DrawRight(this, 0x87, 0x19, mLabel2.c_str(), 100, 100, 0xff);
		mFont->DrawLeft(this, 0xfa, 0x32, mLabel1.c_str(), -1, 100, 100, 0xff);
		return;
	}
	DrawImage(-0x11, mBob, IMAGE_TIP_ARROW, nullptr, true, 100, 100, 0xff);
}

// 0x4091b0
void TipPicture::DrawTile(int theX, int theY, int theKind)
{
	jb::Rect aRect = {0, 0, 0x30, 0x3a};
	DrawImageKeyed(theX, theY, IMAGE_SPECIAL_FACES, 0xff00ff, &aRect, true, 100, 100, 0xff);
	aRect.left = ((theKind - 1) % 6) * 0x26;
	aRect.right = aRect.left + 0x26;
	aRect.top = ((theKind - 1) / 6) * 0x30;
	aRect.bottom = aRect.top + 0x30;
	DrawImage(theX + 7, theY + 3, IMAGE_FACES_IVORY, &aRect, true, 100, 100, 0xff);
}

// 0x408670
TipBox::TipBox(int theId, Board* theBoard) : Dialog(TIP_POS[theId][0], TIP_POS[theId][1]), mId(theId),
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
		SetTitle(STRING_TIP_PLAY_TITLE);
		AddText(STRING_TIP_PLAY_TEXT, 1, 0x32, 0x50, false);
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
		SetTitle(STRING_TIP_GOLD_TITLE);
		AddText(STRING_TIP_GOLD_TEXT, 1, 0x28, 0x50, false);
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
		SetTitle(STRING_TIP_KEY_TITLE);
		AddText(STRING_TIP_KEY_TEXT, 1, 0x37, 0x50, false);
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
		SetTitle(STRING_TIP_MAGNET_TITLE);
		AddText(STRING_TIP_MAGNET_TEXT, 1, 0x2d, 0x53, false);
		break;
	case 4:
		anX = 0x98;
		aY = 0x9e;
		aLeft[0] = 3;
		aLeft[1] = 0;
		aRight[0] = 3;
		aRight[1] = 0;
		SetTitle(STRING_TIP_JOKER_TITLE);
		AddText(STRING_TIP_JOKER_TEXT, 1, 0x2d, 0x53, false);
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
		AddText(STRING_TIP_HINT_TEXT, 0, 0x28, 0x3c, false);
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
		SetTitle(STRING_TIP_FREE_TILES_TITLE);
		AddText(STRING_TIP_FREE_TILES_TEXT, 1, 0x28, 0x50, false);
		Add(new TipPicture(IMAGE_TIP_ARROW), 0x4c, 0xc3);
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
		SetTitle(STRING_TIP_UNIQUE_TITLE);
		AddText(STRING_TIP_FLOWERS_TEXT, 0, 0x32, 0x50, false);
		AddText(STRING_TIP_SEASONS_TEXT, 0, 0x32, 0xe1, false);
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
	aButton->SetText(STRING_DIALOG_OK);
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
	SetTitle(STRING_NO_MOVES_TITLE);
	jb::Toggle* aToggle = new jb::Toggle(IMAGE_RADIO_BUTTON, &mGroup, 0);
	Add(aToggle, 0x32, 0x3c);
	jb::SetCheckLabel(aToggle, STRING_NO_MOVES_UNDO, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x14, 0xff);
	aToggle = new jb::Toggle(IMAGE_RADIO_BUTTON, &mGroup, 1);
	Add(aToggle, 0x32, 0x5a);
	jb::SetCheckLabel(aToggle, STRING_NO_MOVES_SHUFFLE, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x14, 0xff);
	aToggle = new jb::Toggle(IMAGE_RADIO_BUTTON, &mGroup, 2);
	Add(aToggle, 0x32, 0x78);
	jb::SetCheckLabel(aToggle, STRING_NO_MOVES_RESTART, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x14, 0xff);
	mGroup.Select(0);
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0x8d, 0xa0);
	aButton->SetText(STRING_NO_MOVES_OK);
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
	aButton->SetText(STRING_DIALOG_OK);
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

} // namespace mahjongjapan
