#include "Ending.h"

#include "Font.h"
#include "JBBase.h"
#include "MainMenu.h"
#include "Manager.h"
#include "Res.h"
#include "User.h"

namespace mahjongchina
{

// 0x51c608: the lines' places (each line is centred on its x)
static const int LINE_POS[11][2] = {
	{409, 115}, {538, 136}, {646, 203}, {633, 347}, {524, 390}, {403, 433},
	{261, 418}, {145, 334}, {146, 181}, {254, 120}, {383, 231}};

// 0x412cb0
void EndPicture::Draw()
{
	DrawImage(0, 0, IMAGE_ENDING_BACKGROUND, nullptr, true, 100, 100, 0xff);
	mTitleFont->DrawLinesCentred(this, 0, 0x2d, mTitle.c_str(), jb::SCREEN_WIDTH, mTitleFont->GetLineHeight() - 5, 100, 100,
		0xff);
	if (mLine != 0)
	{
		DrawImage(0x44, 0x77, IMAGE_ENDING_SHOWCASE, nullptr, true, 100, 100, 0xff);
		int i = 0;
		if (mLine != 1 && -1 < mLine + -1)
		{
			do
			{
				mFont->DrawLinesCentred(this, LINE_POS[i][0], LINE_POS[i][1], mLines[i].c_str(), 0,
					mFont->GetLineHeight() - 7, 100, 100, 0xff);
				i = i + 1;
			} while (i < mLine + -1);
		}
		if (mLine < 0xc)
			mFont->DrawLinesCentred(this, LINE_POS[mLine - 1][0], LINE_POS[mLine - 1][1], mLines[mLine - 1].c_str(), 0,
				mFont->GetLineHeight() - 7, 100, 100, mAlpha);
		return;
	}
	DrawImage(0x44, 0x77, IMAGE_ENDING_SHOWCASE, nullptr, true, 100, 100, mAlpha);
}

// 0x412df0
void EndPicture::Init()
{
	mTitle = jb::FormatDll(STRING_ENDING_TEXT, gUser.mName.c_str());
	mTitleFont = jb::GetDllFont(FONT_ESCAPE_TITLE, FONT_ESCAPE_TITLE_MAP);
	mFont = jb::GetDllFont(FONT_RESULT, FONT_RESULT_MAP);
	int aPos = 0;
	std::string aText = jb::LoadDllString(STRING_SHOWCASE_NAMES);
	std::string* aLine = mLines;
	int i = 0xb;
	do
	{
		*aLine = jb::Tokenize(aText, 9, aPos);
		aLine = aLine + 1;
		i = i - 1;
	} while (i != 0);
	RegUpdate(true);
}

// 0x412ef0
bool EndPicture::Update()
{
	if (mDelay < 1)
	{
		int aStep;
		if (mLine == 0)
		{
			if (mFirst)
			{
				mLayer->SetOpaque(true);
				mFirst = false;
			}
			aStep = (GetDelta() * 0xff) / 3000;
		}
		else
			aStep = (GetDelta() * 0xff) / 1000;
		int anAlpha = mAlpha + aStep;
		mAlpha = anAlpha;
		if (0xff < anAlpha)
		{
			mAlpha = 0;
			mLine = mLine + 1;
			if (0xb < mLine)
				RegUpdate(false);
		}
		return true;
	}
	if (mDelay == 1000)
	{
		mDelay = 999;
		return false;
	}
	mDelay = mDelay - GetDelta();
	return false;
}

// 0x413030
void EndLayer::Load()
{
	mPicture = new EndPicture();
	Add(mPicture, 0, 0);
	jb::Button* aButton = new jb::Button(IMAGE_ENDING_MAP_BUTTON, 3, false);
	Add(aButton, 0xe, 0x221);
	aButton->SetLabel(STRING_BACK_TO_MAP, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x90, 0x18, 2, 2, 0xff);
	aButton = new jb::Button(IMAGE_ENDING_MENU_BUTTON, 3, false);
	Add(aButton, 0x295, 0x221);
	aButton->SetLabel(STRING_ENDING_MAIN_MENU, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x7c, 0x18, 2, 2, 0xff);
	jb::gManager->LoadSound(SOUND_FANFARE, false);
	jb::gManager->PlaySound(SOUND_FANFARE);
}

// 0x4131a0
bool EndLayer::OnCommand(int theCmd)
{
	if (theCmd == 0x173)
	{
		jb::gManager->SlideMusic(0, 0x44c);
		jb::gManager->SwitchTo(new MainMenu());
		return true;
	}
	if (theCmd - 0x174 != 0)
		return false;
	SetOpaque(false);
	jb::gManager->Pop(true, 1);
	return true;
}

} // namespace mahjongchina
