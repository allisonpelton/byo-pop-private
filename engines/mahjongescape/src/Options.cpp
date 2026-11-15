#include "Options.h"

#include "App.h"
#include "EngineRes.h"
#include "Manager.h"
#include "PlayLayer.h"
#include "Res.h"
#include "User.h"

namespace mahjong
{

// 0x411bd0 (mahjongchina), 0x412160 (mahjongjapan)
void OptionsNote::Create()
{
	int aLeft[] = {2, 2, 0};
	int aRight[] = {2, 2, 0};
	int aTop[] = {2, 2, 3, 0};
	int aBottom[] = {3, 2, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	DialogButton* anOk = new DialogButton(1);
	Add(anOk, 0x98, 0xcb);
	anOk->SetText(STRING_DIALOG_OK);
}

// 0x411cb0 (mahjongchina), 0x412240 (mahjongjapan)
bool OptionsNote::OnCommand(int theCmd)
{
	if (theCmd == 1)
	{
		jb::gManager->Pop(true, 1);
		return true;
	}
	return false;
}

// 0x412b50 (mahjongjapan)
void VolumeSlider::OnValue(int theValue)
{
	if (mKind == 0)
	{
		jb::gManager->SetMusicVolume(theValue);
		return;
	}
	jb::gManager->SetSoundVolume(theValue);
}

// 0x411dc0 (mahjongchina), 0x412350 (mahjongjapan)
void OptionsBox::Create()
{
	int aLeft[] = {3, 3, 3, 0};
	int aRight[] = {3, 3, 3, 0};
	int aTop[] = {1, 3, 2, 1, 0};
	int aBottom[] = {3, 1, 1, 2, 0};
	if (!mFromMenu)
	{
		aLeft[2] = 0;
		aRight[2] = 0;
	}
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(STRING_OPTIONS_TITLE);
	AddText(STRING_OPTION_MUSIC, 1, 0x2d, 0x50, false);
	VolumeSlider* aSlider = new VolumeSlider(0);
	Add(aSlider, 0x75, 0x42);
	aSlider->SetValue(jb::gMusicVolume);
	AddText(STRING_OPTION_SOUNDS, 1, 0x2d, 0x6e, false);
	aSlider = new VolumeSlider(1);
	Add(aSlider, 0x75, 0x60);
	aSlider->SetValue(jb::gSoundsVolume);
	mFullScreen = new jb::CheckBox(IMAGE_CHECK_BOX, 2);
	Add(mFullScreen, 0x28, 0x78);
	mFullScreen->SetLabel(STRING_OPTION_FULL_SCREEN, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x1f, 0xff);
	mFullScreen->SetChecked(jb::gFullScreen);
	if (jb::gWideAvailable)
	{
		mWide = new jb::CheckBox(IMAGE_CHECK_BOX, 2);
		Add(mWide, 0xb9, 0x78);
		mWide->SetLabel(STRING_OPTION_WIDESCREEN, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x1f, 0xff);
		mWide->SetChecked(jb::gWideScreen);
		mWide->SetVisible(jb::gFullScreen);
	}
	mAutoHint = new jb::CheckBox(IMAGE_CHECK_BOX, 2);
	Add(mAutoHint, 0x28, 0xa0);
	mAutoHint->SetLabel(STRING_OPTION_AUTO_HINTS, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x1f, 0xff);
	mAutoHint->SetChecked(gUser.mAutoHint);
	int aY = 200;
	mAccel = new jb::CheckBox(IMAGE_CHECK_BOX, 2);
	Add(mAccel, 0x28, 0xc8);
	mAccel->SetLabel(STRING_OPTION_3D, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x1f, 0xff);
	mAccel->SetChecked(jb::IsAccelerated());
	mAccel->SetCmd(2);
	if (mFromMenu)
	{
		AddText(STRING_OPTION_DIFFICULTY, 1, 0x2d, 0x113, false);
		aY = 0x118;
		jb::Toggle* aToggle = new jb::Toggle(IMAGE_RADIO_BUTTON, &mQuads, 0);
		Add(aToggle, 0x28, 0x118);
		jb::SetCheckLabel(aToggle, STRING_OPTION_RELAXED, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x14, 0xff);
		aToggle = new jb::Toggle(IMAGE_RADIO_BUTTON, &mQuads, 0x19);
		Add(aToggle, 0xa0, 0x118);
		jb::SetCheckLabel(aToggle, STRING_OPTION_NORMAL, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x14, 0xff);
		aToggle = new jb::Toggle(IMAGE_RADIO_BUTTON, &mQuads, 0x55);
		Add(aToggle, 0x118, 0x118);
		jb::SetCheckLabel(aToggle, STRING_OPTION_EXPERT, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x14, 0xff);
		mQuads.Select(gUser.mQuadPercent);
	}
	DialogButton* anOk = new DialogButton(1);
	Add(anOk, 0xa5, aY + 0x2e);
	anOk->SetText(STRING_NO_MOVES_OK);
}

// 0x4122f0 (mahjongchina), 0x412880 (mahjongjapan) (command 2: the port's display always passes the original's 3D test,
// 0x421370 (mahjongjapan), and has no 3D warning, 0x5d8e20 (mahjongjapan), so checking the box shows no note)
bool OptionsBox::OnCommand(int theCmd)
{
	if (theCmd == 1)
	{
		gUser.mAutoHint = mAutoHint->IsChecked();
		if (mFromMenu)
		{
			gUser.mQuadPercent = mQuads.GetSelected();
			if (gUser.mQuadPercent < 0 || 100 < gUser.mQuadPercent)
				gUser.mQuadPercent = 0x19;
		}
		if (mFullScreen->IsChecked() == jb::gFullScreen)
		{
			if (mAccel->IsChecked() == jb::IsAccelerated())
			{
				if (mWide != nullptr && mWide->IsChecked() != jb::gWideScreen)
					jb::gManager->SetWideScreen(mWide->IsChecked());
			}
			else
			{
				if (mWide != nullptr)
					jb::gWideScreen = mWide->IsChecked();
				jb::gManager->SetHardwareAccel(mAccel->IsChecked());
			}
			jb::gManager->Pop(true, 1);
			return true;
		}
		if (mWide != nullptr)
			jb::gWideScreen = mWide->IsChecked();
		jb::gHardwareAccel = mAccel->IsChecked();
		jb::gManager->SetFullScreen(mFullScreen->IsChecked(), true);
		jb::gManager->Pop(true, 1);
		return true;
	}
	if (theCmd != 2)
	{
		if (mWide != nullptr)
			mWide->SetVisible(mFullScreen->IsChecked());
		return false;
	}
	if (jb::gManager->IsAppActive())
	{
		// (the original shows the 3D test's warning here, 0x5d8e20 (mahjongjapan), which the port never has)
		return true;
	}
	mAccel->SetChecked(false);
	OptionsNote* aNote = new OptionsNote();
	jb::gManager->Push(aNote, true, false, true);
	aNote->SetTitle(STRING_NO_3D_TITLE);
	aNote->AddText(STRING_NO_3D_TEXT, 0, 0, 0x5a, false);
	return true;
}

// 0x415460 (mahjongchina), 0x4159f0 (mahjongjapan)
bool StyleToggle::OnLButtonDblClk(int theFlags, int theX, int theY)
{
	jb::PlaySound(jb::SOUND_CLICK);
	static_cast<StyleBox*>(mLayer)->Apply();
	return true;
}

// 0x4152b0 (mahjongchina), 0x415820 (mahjongjapan)
void StyleToggle::DrawNormal()
{
	DrawTiles();
	SetTrackDirty(false);
	if (IsChecked())
		DrawImage(-0x10, -0xf, IMAGE_STYLE_GLOW, nullptr, true, 100, 100, 0xff);
	SetTrackDirty(true);
}

// 0x4152f0 (mahjongchina), 0x415870 (mahjongjapan)
void StyleToggle::DrawTiles()
{
	DrawBack(0x2a, 0x3a);
	DrawBack(0, 6);
	DrawBack(0, 0x3a);
	DrawBack(0x30, 0);
	DrawBack(0x30, 0x34);
	DrawBack(6, 0);
	DrawBack(6, 0x34);
	DrawFace(0x30, 0, 0xe);
	DrawFace(0x30, 0x34, 0x25);
	DrawFace(6, 0, 4);
	DrawFace(6, 0x34, 0x15);
}

// 0x415380 (mahjongchina), 0x415900 (mahjongjapan)
void StyleToggle::DrawBack(int theX, int theY)
{
	jb::Rect aRect;
	aRect.left = mBack * 0x30;
	aRect.right = aRect.left + 0x30;
	aRect.top = 0;
	aRect.bottom = 0x3a;
	DrawImageKeyed(theX, theY, IMAGE_SPECIAL_FACES, 0xff00ff, &aRect, true, 100, 100, 0xff);
}

// 0x4153e0 (mahjongchina), 0x415960 (mahjongjapan)
void StyleToggle::DrawFace(int theX, int theY, int theKind)
{
	jb::Rect aRect;
	aRect.left = ((theKind + -1) % 6) * 0x26;
	aRect.right = aRect.left + 0x26;
	aRect.top = ((theKind + -1) / 6) * 0x30;
	aRect.bottom = aRect.top + 0x30;
	DrawImage(theX + 7, theY + 3, GetImage(), &aRect, true, 100, 100, 0xff);
}

// 0x415560 (mahjongchina), 0x415af0 (mahjongjapan)
void StyleBox::Create()
{
	int aLeft[] = {2, 1, 3, 3, 0};
	int aRight[] = {2, 3, 1, 3, 0};
	int aTop[] = {2, 1, 3, 2, 1, 0};
	int aBottom[] = {2, 3, 1, 1, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(STRING_TILE_SET_TITLE);
	Add(new StyleToggle(IMAGE_FACES_IVORY, 0, &mStyles, 0), 0x34, 0x46);
	Add(new StyleToggle(IMAGE_FACES_GREEN, 0, &mStyles, 1), 0xc0, 0x46);
	Add(new StyleToggle(IMAGE_FACES_RED, 6, &mStyles, 2), 0x14c, 0x46);
	Add(new StyleToggle(IMAGE_FACES_BLACK, 7, &mStyles, 3), 0x34, 0xcf);
	Add(new StyleToggle(IMAGE_FACES_PAINTED, 0, &mStyles, 4), 0xc0, 0xcf);
	Add(new StyleToggle(IMAGE_FACES_NUMBERS, 0, &mStyles, 5), 0x14c, 0xcf);
	int aStyle = gUser.mStyleFree;
	if (mEscape)
		aStyle = gUser.mStyleEscape;
	mStyles.Select(aStyle);
	DialogButton* aButton = new DialogButton(0, 0x66);
	Add(aButton, 0x84, 0x156);
	aButton->SetText(STRING_TILE_SET_OK);
	aButton = new DialogButton(1, 0x66);
	Add(aButton, 0xfc, 0x156);
	aButton->SetText(STRING_TILE_SET_CANCEL);
}

// 0x4158b0 (mahjongchina), 0x415e40 (mahjongjapan) (every command is taken)
bool StyleBox::OnCommand(int theCmd)
{
	if (theCmd == 0)
	{
		Apply();
		return true;
	}
	if (theCmd == 1)
		jb::gManager->Pop(true, 1);
	return true;
}

// 0x4158e0 (mahjongchina), 0x415e70 (mahjongjapan)
void StyleBox::Apply()
{
	int aStyle = mStyles.GetSelected();
	if (aStyle < 0 || 5 < aStyle)
		aStyle = 1;
	int aFree = aStyle;
	if (mEscape)
	{
		aFree = gUser.mStyleFree;
		gUser.mStyleEscape = aStyle;
	}
	gUser.mStyleFree = aFree;
	jb::Layer* aPlay = jb::gManager->Pop(true, 1);
	static_cast<PlayLayer*>(aPlay)->SetStyle();
}

} // namespace mahjong
