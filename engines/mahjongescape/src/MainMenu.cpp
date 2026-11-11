#include "MainMenu.h"

#include "Animation.h"
#include "App.h"
#include "Edition.h"
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

namespace mahjong
{

// 0x410ef0 (mahjongchina), 0x411420 (mahjongjapan)
void MenuButton::OnInit()
{
	mFont = jb::GetDllFont(FONT_MENU_HINT, FONT_MENU_HINT_MAP);
	RegUpdate(true);
}

// 0x410f20 (mahjongchina), 0x411450 (mahjongjapan): the caption fades in over half a second while hovered
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

// 0x410f70 (mahjongchina), 0x4114a0 (mahjongjapan)
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
		mFont->DrawCentred(this, GetFrameWidth() / 2 + edition::MENU_HINT_X,
			mTextId != 0x77 ? edition::MENU_HINT_Y : edition::MENU_PLAYER_HINT_Y, aText.c_str(), 0, 100, 100, mAlpha);
		SetTrackDirty(true);
		return;
	}
	mAlpha = 0;
}

// 0x411140 (mahjongchina), 0x411690 (mahjongjapan)
void PlayerButton::OnInit()
{
	mText = jb::LoadDllString(STRING_CHANGE_PLAYER);
	mFont = jb::GetDllFont(FONT_CAPTION, FONT_CAPTION_MAP);
	mHoverFont = jb::GetDllFont(FONT_MENU_PLAYER_HOVER, FONT_MENU_PLAYER_HOVER_MAP);
}

// 0x4111a0 (mahjongchina), 0x411700 (mahjongjapan)
void PlayerButton::DrawNormal()
{
	mFont->DrawCentred(this, 0, 0, mText.c_str(), jb::SCREEN_WIDTH, 100, 100, 0xff);
}

// 0x4111d0 (mahjongchina), 0x411730 (mahjongjapan)
void PlayerButton::DrawHover()
{
	mHoverFont->DrawCentred(this, 0, 0, mText.c_str(), jb::SCREEN_WIDTH, 100, 100, 0xff);
}

// 0x411250 (mahjongchina), 0x4117b0 (mahjongjapan)
void MenuBackdrop::Draw()
{
	DrawImage(0, 0, IMAGE_MENU_BACKGROUND, nullptr, true, 100, 100, 0xff);
	std::string aText = jb::FormatDll(STRING_WELCOME, gUser.mName.c_str());
	mFont->DrawCentred(this, 0, edition::MENU_WELCOME_Y, aText.c_str(), jb::SCREEN_WIDTH, 100, 100, 0xff);
}

// 0x4112f0 (mahjongchina), 0x411860 (mahjongjapan)
void MenuBackdrop::Init()
{
	jb::Animation* aLeft = new jb::Animation();
	Attach(aLeft);
	jb::Animation* aRight = new jb::Animation();
	Attach(aRight);
	aLeft->SetImage(edition::MENU_LANTERN_LEFT_X, edition::MENU_LANTERN_Y, IMAGE_MENU_LANTERN_LEFT,
		edition::MENU_LANTERN_WIDTH);
	aLeft->Setup(100, 10, 4, 4, 100, -1, 0, true);
	aRight->SetImage(edition::MENU_LANTERN_RIGHT_X, edition::MENU_LANTERN_Y, IMAGE_MENU_LANTERN_RIGHT,
		edition::MENU_LANTERN_WIDTH);
	aRight->Setup(100, 10, 4, 4, 100, -1, 0, true);
	mFont = jb::GetDllFont(FONT_HEADING, FONT_HEADING_MAP);
	RegUpdate(true);
}

// 0x411460 (mahjongchina), 0x4119d0 (mahjongjapan)
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
	Add(aButton, edition::MENU_ADVENTURE_X, edition::MENU_ADVENTURE_Y);
	aButton->SetLabel(STRING_ADVENTURE, FONT_MENU_BUTTON, FONT_MENU_BUTTON_MAP, edition::MENU_ADVENTURE_LABEL_X,
		edition::MENU_ADVENTURE_LABEL_RIGHT, edition::MENU_ADVENTURE_LABEL_Y, 4, 4, 0xff);
	aButton = new MenuButton(IMAGE_MENU_CLASSIC_BUTTON, STRING_CLASSIC_HINT);
	Add(aButton, edition::MENU_CLASSIC_X, edition::MENU_CLASSIC_Y);
	aButton->SetLabel(STRING_CLASSIC, FONT_MENU_BUTTON, FONT_MENU_BUTTON_MAP, edition::MENU_CLASSIC_LABEL_X,
		edition::MENU_CLASSIC_LABEL_RIGHT, 0x2f, 4, 4, 0xff);
	jb::Button* aSmall = new jb::Button(IMAGE_MENU_OPTIONS_BUTTON, 3, false);
	Add(aSmall, edition::MENU_OPTIONS_X, edition::MENU_SMALL_Y);
	aSmall->SetLabel(STRING_OPTIONS, edition::MENU_SMALL_FONT, edition::MENU_SMALL_FONT_MAP,
		edition::MENU_OPTIONS_LABEL_X, edition::MENU_OPTIONS_LABEL_RIGHT, edition::MENU_SMALL_LABEL_Y, 2, 2, 0xff);
	aSmall = new jb::Button(IMAGE_MENU_INSTRUCTIONS_BUTTON, 3, false);
	Add(aSmall, edition::MENU_INSTRUCTIONS_X, edition::MENU_SMALL_Y);
	aSmall->SetLabel(STRING_INSTRUCTIONS, edition::MENU_SMALL_FONT, edition::MENU_SMALL_FONT_MAP,
		edition::MENU_INSTRUCTIONS_LABEL_X, edition::MENU_INSTRUCTIONS_LABEL_RIGHT, edition::MENU_SMALL_LABEL_Y, 2, 2,
		0xff);
	aSmall = new jb::Button(IMAGE_MENU_EXIT_BUTTON, 3, false);
	Add(aSmall, edition::MENU_EXIT_X, edition::MENU_SMALL_Y);
	aSmall->SetLabel(STRING_EXIT, edition::MENU_SMALL_FONT, edition::MENU_SMALL_FONT_MAP, edition::MENU_EXIT_LABEL_X,
		edition::MENU_EXIT_LABEL_RIGHT, edition::MENU_SMALL_LABEL_Y, 2, 2, 0xff);
	Add(new PlayerButton(), 0, edition::MENU_PLAYER_BUTTON_Y);
	jb::gManager->LoadMusic(MUSIC_MENU);
	jb::gManager->PlayMusic(MUSIC_MENU, false, 0x44c, 0);
}

// 0x411810 (mahjongchina), 0x411db0 (mahjongjapan)
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

// 0x411aa0 (mahjongchina), 0x412020 (mahjongjapan)
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

// 0x411af0 (mahjongchina), 0x412080 (mahjongjapan)
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

} // namespace mahjong
