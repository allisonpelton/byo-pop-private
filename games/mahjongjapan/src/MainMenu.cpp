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
#include "User.h"

namespace mahjongjapan
{

// 0x411420
void MenuButton::OnInit()
{
	mFont = jb::GetDllFont(0x24b, 0x24a, -1);
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
		mFont->DrawCentred(this, GetFrameWidth() / 2 - 6, mTextId != 0x77 ? 0x78 : 0x69, aText.c_str(), 0, 100,
			100, mAlpha);
		SetTrackDirty(true);
		return;
	}
	mAlpha = 0;
}

// 0x411690
void PlayerButton::OnInit()
{
	mText = jb::LoadDllString(0xd);
	mFont = jb::GetDllFont(0x11c, 0x11b, -1);
	mHoverFont = jb::GetDllFont(0x16e, 0x16d, -1);
	mTextWidth = mHoverFont->GetWidth(mText.c_str(), 100);
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
	DrawImage(0, 0, 0x6d, nullptr, true, 100, 100, 0xff);
	std::string aText = jb::FormatDll(0x71, gUser.mName.c_str());
	mFont->DrawCentred(this, 0, 0x1ec, aText.c_str(), 800, 100, 100, 0xff);
}

// 0x411860
void MenuBackdrop::Init()
{
	jb::Animation* aLeft = new jb::Animation();
	Attach(aLeft);
	jb::Animation* aRight = new jb::Animation();
	Attach(aRight);
	aLeft->SetImage(0x1c, 0x20, 0x254, 0x82);
	aLeft->Setup(100, 10, 4, 4, 100, -1, 0, true);
	aRight->SetImage(0x280, 0x20, 0x253, 0x82);
	aRight->Setup(100, 10, 4, 4, 100, -1, 0, true);
	mFont = jb::GetDllFont(0x11e, 0x11d, -1);
	RegUpdate(true);
}

// 0x4119d0
void MainMenu::Load()
{
	Add(new MenuBackdrop(), 0, 0);
	std::string aPath = jb::GetExeFolder() + "MenuLogo";
	if (jb::gManager->LoadFileImage(0xff03, aPath.c_str()))
	{
		jb::ImageInfo anInfo;
		jb::gManager->GetImageInfo(0xff03, anInfo);
		std::string anUrl = jb::gApp->RegGetString(nullptr, "PartnerURL", "", true);
		jb::LinkButton* aLogo;
		if (!anUrl.empty() && 0 < (int)anUrl.length())
			aLogo = new jb::LinkButton(0xff03, anUrl.c_str());
		else
			aLogo = new jb::LinkButton(0xff03, nullptr);
		Add(aLogo, 0x14, 0x244 - anInfo.mHeight);
	}
	MenuButton* aButton = new MenuButton(0x8e, 0x78);
	Add(aButton, 0xc0, 0xe2);
	aButton->SetLabel(0x73, 0x9a, 0x9b, 0, 0x198, 0x34, 4, 4, 0xff);
	aButton = new MenuButton(0x84, 0x77);
	Add(aButton, 0x115, 0x158);
	aButton->SetLabel(0x72, 0x9a, 0x9b, 0, 0xf0, 0x2f, 4, 4, 0xff);
	jb::Button* aSmall = new jb::Button(0x83, 3, false);
	Add(aSmall, 0xb2, 0x218);
	aSmall->SetLabel(0x76, 0xa0, 0x9f, 0, 0x72, 0x19, 2, 2, 0xff);
	aSmall = new jb::Button(0x87, 3, false);
	Add(aSmall, 0x140, 0x218);
	aSmall->SetLabel(0x75, 0xa0, 0x9f, 0, 0x9b, 0x19, 2, 2, 0xff);
	aSmall = new jb::Button(0x86, 3, false);
	Add(aSmall, 0x1fe, 0x218);
	aSmall->SetLabel(0x74, 0xa0, 0x9f, 4, 0x6b, 0x19, 2, 2, 0xff);
	Add(new PlayerButton(), 0, 0x200);
	jb::gManager->LoadMusic(0x14f);
	jb::gManager->PlayMusic(0x14f, false, 0x44c, 0);
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

} // namespace mahjongjapan
