#include "MainMenu.h"

#include "Animation.h"
#include "App.h"
#include "Font.h"
#include "Help.h"
#include "Image.h"
#include "LayoutSelect.h"
#include "Manager.h"
#include "Options.h"
#include "PlayLayer.h"
#include "Players.h"
#include "Res.h"
#include "User.h"

namespace mahjongchina
{

// 0x411420
void MenuButton::OnInit()
{
	mFont = jb::GetDllFont(FONT_MENU_HINT, FONT_MENU_HINT_MAP);
	RegUpdate(true);
}

// 0x411450: the caption fades in over half a second while hovered
bool MenuButton::Update()
{
	if (IsHover() && mAlpha < 0xff)
	{
		mAlpha = mAlpha + GetDelta() / 2;
		if (0xff < mAlpha)
			mAlpha = 0xff;
		return true;
	}
	return false;
}

// 0x4114a0
void MenuButton::Draw()
{
	Button::Draw();
	if (IsHover())
	{
		int anId = 0x77;
		if (mTextId != 0x77)
			anId = (gUser.mProgress != 0) + 0x78;
		std::string aText = jb::LoadDllString(anId);
		SetTrackDirty(false);
		mFont->DrawCentred(this, GetFrameWidth() / 2, mTextId != 0x77 ? 0x5b : 0x56, aText.c_str(), 0, 100,
			100, mAlpha);
		SetTrackDirty(true);
		return;
	}
	mAlpha = 0;
}

// 0x411690
void PlayerButton::OnInit()
{
	mText = jb::LoadDllString(STRING_CHANGE_PLAYER);
	mFont = jb::GetDllFont(FONT_CAPTION, FONT_CAPTION_MAP);
	mHoverFont = jb::GetDllFont(FONT_MENU_PLAYER_HOVER, FONT_MENU_PLAYER_HOVER_MAP);
}

// 0x411700
void PlayerButton::DrawNormal()
{
	mFont->DrawCentred(this, 0, 0, mText.c_str(), 800, 100, 100, 0xff);
}

// 0x411730
void PlayerButton::DrawHover()
{
	mHoverFont->DrawCentred(this, 0, 0, mText.c_str(), 800, 100, 100, 0xff);
}

// 0x4117b0
void MenuBackdrop::Draw()
{
	DrawImage(0, 0, IMAGE_MENU_BACKGROUND, nullptr, true, 100, 100, 0xff);
	std::string aText = jb::FormatDll(STRING_WELCOME, gUser.mName.c_str());
	mFont->DrawCentred(this, 0, 0x1e2, aText.c_str(), 800, 100, 100, 0xff);
}

// 0x411860
void MenuBackdrop::Init()
{
	jb::Animation* aLeft = new jb::Animation();
	Attach(aLeft);
	jb::Animation* aRight = new jb::Animation();
	Attach(aRight);
	aLeft->SetImage(0x95, 0x146, IMAGE_MENU_LANTERN_LEFT, 0x22);
	aLeft->Setup(100, 10, 4, 4, 100, -1, 0, true);
	aRight->SetImage(0x274, 0x146, IMAGE_MENU_LANTERN_RIGHT, 0x22);
	aRight->Setup(100, 10, 4, 4, 100, -1, 0, true);
	mFont = jb::GetDllFont(FONT_HEADING, FONT_HEADING_MAP);
	RegUpdate(true);
}

// 0x4119d0
void MainMenu::Load()
{
	Add(new MenuBackdrop(), 0, 0);
	std::string aPath = jb::GetExeFolder() + "MenuLogo";
	if (jb::gManager->LoadFileImage(FILE_IMAGE_MENU_LOGO, aPath.c_str()))
	{
		jb::ImageInfo anInfo;
		jb::gManager->GetImageInfo(FILE_IMAGE_MENU_LOGO, anInfo);
		// the original makes it a button that opens the registry's PartnerURL; without one (this install has none) it
		// is disabled, and here it is a picture that goes nowhere
		jb::Button* aLogo = new jb::Button(FILE_IMAGE_MENU_LOGO, 1, false);
		Add(aLogo, 0x14, 0x244 - anInfo.mHeight);
		aLogo->SetEnabled(false);
	}
	MenuButton* aButton = new MenuButton(IMAGE_MENU_ADVENTURE_BUTTON, STRING_ADVENTURE_BEGIN_HINT);
	Add(aButton, 0xef, 0xdf);
	aButton->SetLabel(STRING_ADVENTURE, FONT_MENU_BUTTON, FONT_MENU_BUTTON_MAP, 10, 0x140, 0x2f, 4, 4, 0xff);
	aButton = new MenuButton(IMAGE_MENU_CLASSIC_BUTTON, STRING_CLASSIC_HINT);
	Add(aButton, 0x134, 0x144);
	aButton->SetLabel(STRING_CLASSIC, FONT_MENU_BUTTON, FONT_MENU_BUTTON_MAP, 10, 0xaa, 0x2f, 4, 4, 0xff);
	jb::Button* aSmall = new jb::Button(IMAGE_MENU_OPTIONS_BUTTON, 3, false);
	Add(aSmall, 0xae, 0x209);
	aSmall->SetLabel(STRING_OPTIONS, FONT_MENU_SMALL_BUTTON, FONT_MENU_SMALL_BUTTON_MAP, 8, 0x6e, 0x1c, 2, 2, 0xff);
	aSmall = new jb::Button(IMAGE_MENU_INSTRUCTIONS_BUTTON, 3, false);
	Add(aSmall, 0x13c, 0x209);
	aSmall->SetLabel(STRING_INSTRUCTIONS, FONT_MENU_SMALL_BUTTON, FONT_MENU_SMALL_BUTTON_MAP, 0xd, 0xaa, 0x1c, 2, 2, 0xff);
	aSmall = new jb::Button(IMAGE_MENU_EXIT_BUTTON, 3, false);
	Add(aSmall, 0x20f, 0x209);
	aSmall->SetLabel(STRING_EXIT, FONT_MENU_SMALL_BUTTON, FONT_MENU_SMALL_BUTTON_MAP, 0xd, 0x5a, 0x1c, 2, 2, 0xff);
	Add(new PlayerButton(), 0, 0x1f6);
	jb::gManager->LoadMusic(MUSIC_MENU);
	jb::gManager->PlayMusic(MUSIC_MENU, false, 0x44c, 0);
}

// 0x411db0
bool MainMenu::OnCommand(int theCmd)
{
	jb::Layer* aLayer;
	switch (theCmd)
	{
	case 1:
		gUser.Save();
		aLayer = new PlayerBox();
		break;
	default:
		return false;
	case 0x83:
		if (!HasPlayer(0x83))
			return true;
		aLayer = new OptionsBox(true);
		break;
	case 0x84:
		if (!HasPlayer(0x84))
			return true;
		jb::gManager->SlideMusic(0, 0x44c);
		jb::gManager->SwitchTo(new LayoutSelect());
		return true;
	case 0x86:
		jb::gApp->Quit();
		return true;
	case 0x87:
		aLayer = new HelpMenu();
		break;
	case 0x8e:
		if (!HasPlayer(0x8e))
			return true;
		jb::gManager->SlideMusic(0, 0x44c);
		jb::gManager->SwitchTo(new PlayLayer(0));
		return true;
	}
	jb::gManager->Push(aLayer, true, true, true);
	return true;
}

// 0x412020
void MainMenu::AfterActiveChange()
{
	int aCurrent = gPlayers.mCurrent;
	if (IsActive() && gPlayers.mChanged)
	{
		gPlayers.mChanged = false;
		if (gPlayers.mCurrent != -1)
			gUser.Load(aCurrent, gPlayers.GetName(aCurrent));
		if (mPending != -1)
			OnCommand(mPending);
	}
}

// 0x412080
bool MainMenu::HasPlayer(int theCmd)
{
	if (gPlayers.mCurrent == -1)
	{
		jb::gManager->Push(new NewPlayerBox(true), true, true, true);
		mPending = theCmd;
		return false;
	}
	return true;
}

} // namespace mahjongchina
