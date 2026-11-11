#include "Ending.h"

#include "Font.h"
#include "JBBase.h"
#include "MainMenu.h"
#include "Manager.h"
#include "Res.h"
#include "User.h"

namespace mahjongjapan
{

// 0x51d718: the lines' places
static const int LINE_POS[12][2] = {
	{0x1ce, 0x77}, {0x27d, 0xdf}, {0x2a4, 0x150}, {0x25f, 0x1ba}, {0x207, 0x214}, {0x187, 0x232},
	{0xe7, 0x225}, {0x65, 0x1e3}, {0x20, 0x167}, {0x58, 0x116}, {0x91, 0x84}, {0x12b, 0x77}};

// 0x412cb0
void EndPicture::Draw()
{
	DrawImage(0, 0, IMAGE_ENDING_BACKGROUND, nullptr, true, 100, 100, 0xff);
	mTitleFont->DrawLinesCentred(this, 0, 0x12, mTitle.c_str(), jb::SCREEN_WIDTH, mTitleFont->GetLineHeight() + 10, 100,
		100, 0xff);
	if (mLine != 0)
	{
		DrawImage(0x72, 0x5a, IMAGE_ENDING_SHOWCASE, nullptr, true, 100, 100, 0xff);
		int i = 0;
		if (mLine != 1 && -1 < mLine + -1)
		{
			do
			{
				mFont->DrawLeft(this, LINE_POS[i][0], LINE_POS[i][1], mLines[i].c_str(), -1, 100, 100, 0xff);
				i = i + 1;
			} while (i < mLine + -1);
		}
		if (mLine < 0xd)
			mFont->DrawLeft(this, LINE_POS[mLine - 1][0], LINE_POS[mLine - 1][1], mLines[mLine - 1].c_str(), -1, 100,
				100, mAlpha);
		return;
	}
	DrawImage(0x72, 0x5a, IMAGE_ENDING_SHOWCASE, nullptr, true, 100, 100, mAlpha);
}

// 0x412df0
void EndPicture::Init()
{
	mTitle = jb::FormatDll(STRING_ENDING_TEXT, gUser.mName.c_str());
	mTitleFont = jb::GetDllFont(FONT_TITLE, FONT_TITLE_MAP);
	mFont = jb::GetDllFont(FONT_LABEL, FONT_LABEL_MAP);
	int aPos = 0;
	std::string aText = jb::LoadDllString(STRING_SHOWCASE_NAMES);
	std::string* aLine = mLines;
	int i = 0xc;
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
			if (0xc < mLine)
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
	Add(aButton, 0x18, 0x214);
	aButton->SetLabel(STRING_BACK_TO_MAP, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x9c, 0x1b, 2, 2, 0xff);
	aButton = new jb::Button(IMAGE_ENDING_MENU_BUTTON, 3, false);
	Add(aButton, 0x278, 0x214);
	aButton->SetLabel(STRING_ENDING_MAIN_MENU, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x90, 0x1b, 2, 2, 0xff);
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

} // namespace mahjongjapan
