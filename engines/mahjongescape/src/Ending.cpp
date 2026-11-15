#include "Ending.h"

#include "Edition.h"
#include "Font.h"
#include "JBBase.h"
#include "MainMenu.h"
#include "Manager.h"
#include "Res.h"
#include "User.h"

namespace mahjong
{

// 0x412750 (mahjongchina), 0x412cb0 (mahjongjapan)
void EndPicture::Draw()
{
	DrawImage(0, 0, IMAGE_ENDING_BACKGROUND, nullptr, true, 100, 100, 0xff);
	mTitleFont->DrawLinesCentred(this, 0, edition::ENDING_TITLE_Y, mTitle.c_str(), jb::SCREEN_WIDTH,
		mTitleFont->GetLineHeight() + edition::ENDING_TITLE_LINE_GAP, 100, 100, 0xff);
	if (mLine != 0)
	{
		DrawImage(edition::ENDING_SHOWCASE_X, edition::ENDING_SHOWCASE_Y, IMAGE_ENDING_SHOWCASE, nullptr, true, 100,
			100, 0xff);
		int i = 0;
		if (mLine != 1 && -1 < mLine + -1)
		{
			do
			{
				edition::DrawShowcaseLine(this, mFont, edition::SHOWCASE_LINE_POS[i][0],
					edition::SHOWCASE_LINE_POS[i][1], mLines[i].c_str(), 0xff);
				i = i + 1;
			} while (i < mLine + -1);
		}
		if (mLine < edition::NUM_SHOWCASE_LINES + 1)
			edition::DrawShowcaseLine(this, mFont, edition::SHOWCASE_LINE_POS[mLine - 1][0],
				edition::SHOWCASE_LINE_POS[mLine - 1][1], mLines[mLine - 1].c_str(), mAlpha);
		return;
	}
	DrawImage(edition::ENDING_SHOWCASE_X, edition::ENDING_SHOWCASE_Y, IMAGE_ENDING_SHOWCASE, nullptr, true, 100, 100,
		mAlpha);
}

// 0x412880 (mahjongchina), 0x412df0 (mahjongjapan)
void EndPicture::Init()
{
	mTitle = jb::FormatDll(STRING_ENDING_TEXT, gUser.mName.c_str());
	mTitleFont = jb::GetDllFont(edition::ENDING_TITLE_FONT, edition::ENDING_TITLE_FONT_MAP);
	mFont = jb::GetDllFont(edition::ENDING_LINE_FONT, edition::ENDING_LINE_FONT_MAP);
	int aPos = 0;
	std::string aText = jb::LoadDllString(STRING_SHOWCASE_NAMES);
	std::string* aLine = mLines;
	int i = edition::NUM_SHOWCASE_LINES;
	do
	{
		*aLine = jb::Tokenize(aText, 9, aPos);
		aLine = aLine + 1;
		i = i - 1;
	} while (i != 0);
	RegUpdate(true);
}

// 0x412980 (mahjongchina), 0x412ef0 (mahjongjapan)
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
			if (edition::NUM_SHOWCASE_LINES < mLine)
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

// 0x412ab0 (mahjongchina), 0x413030 (mahjongjapan)
void EndLayer::Load()
{
	mPicture = new EndPicture();
	Add(mPicture, 0, 0);
	jb::Button* aButton = new jb::Button(IMAGE_ENDING_MAP_BUTTON, 3, false);
	Add(aButton, edition::ENDING_MAP_BUTTON_X, edition::ENDING_BUTTONS_Y);
	aButton->SetLabel(STRING_BACK_TO_MAP, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::ENDING_MAP_LABEL_RIGHT,
		edition::ENDING_LABEL_Y, 2, 2, 0xff);
	aButton = new jb::Button(IMAGE_ENDING_MENU_BUTTON, 3, false);
	Add(aButton, edition::ENDING_MENU_BUTTON_X, edition::ENDING_BUTTONS_Y);
	aButton->SetLabel(STRING_ENDING_MAIN_MENU, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::ENDING_MENU_LABEL_RIGHT,
		edition::ENDING_LABEL_Y, 2, 2, 0xff);
	jb::gManager->LoadSound(SOUND_FANFARE, false);
	jb::gManager->PlaySound(SOUND_FANFARE);
}

// 0x412c20 (mahjongchina), 0x4131a0 (mahjongjapan)
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

} // namespace mahjong
