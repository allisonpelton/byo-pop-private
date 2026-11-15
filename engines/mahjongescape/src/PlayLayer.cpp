#include "PlayLayer.h"

#include "Board.h"
#include "Edition.h"
#include "Font.h"
#include "GameDialogs.h"
#include "Globals.h"
#include "LayoutSelect.h"
#include "MainMenu.h"
#include "Manager.h"
#include "MapLayer.h"
#include "Res.h"
#include "Stream.h"
#include "User.h"

#include <cstdlib>

namespace mahjong
{

// 0x4051d0 (mahjongchina), 0x404f30 (mahjongjapan)
Background::~Background()
{
	jb::FreeImage(mImage);
}

// 0x405230 (mahjongchina), 0x404f90 (mahjongjapan)
void Background::Draw()
{
	if (mImage != 0)
		DrawImage(0, 0, mImage, nullptr, true, 100, 100, 0xff);
}

// 0x405260 (mahjongchina), 0x404fc0 (mahjongjapan)
void Background::Init()
{
	if (mImage != 0)
		mManager->PreloadDllImage(mImage);
}

// 0x406a60 (mahjongchina), 0x406800 (mahjongjapan)
void ShadeBox::Draw()
{
	int anAlpha;
	Button::Draw();
	if (!IsEnabled() && GetShowDisabled())
	{
		mFont->DrawCentred(this, edition::PANEL_BUTTON_TEXT_X, -1, mLine1.c_str(), 0, 100, 100, 0x40);
		SetTrackDirty(false);
		anAlpha = 0x40;
	}
	else
	{
		mFont->DrawCentred(this, edition::PANEL_BUTTON_TEXT_X, -1, mLine1.c_str(), 0, 100, 100, 0xff);
		SetTrackDirty(false);
		anAlpha = 0xff;
	}
	mFont->DrawCentred(this, edition::PANEL_BUTTON_TEXT_X, edition::PANEL_BUTTON_LINE2_Y, mLine2.c_str(), 0, 100, 100,
		anAlpha);
	SetTrackDirty(true);
}

// 0x406b10 (mahjongchina), 0x4068c0 (mahjongjapan)
void ShadeBox::OnInit()
{
	mFont = jb::GetDllFont(FONT_LABEL, FONT_LABEL_MAP);
	mLine1 = jb::LoadDllString(STRING_SHOW);
	mLine2 = jb::LoadDllString(STRING_FREE_TILES);
}

// 0x406b50 (mahjongchina), 0x406900 (mahjongjapan)
void ShadeBox::OnClick()
{
	CheckBox::OnClick();
	gUser.mShadeCovered = IsChecked();
}

// 0x406b70 (mahjongchina), 0x406920 (mahjongjapan)
void ShadeBox::OnEnabledChanged()
{
	if (IsEnabled())
		SetShowDisabled(true);
}

// 0x4090b0 (mahjongchina), 0x409270 (mahjongjapan)
Panel::Panel(int theMode) : mMode(theMode)
{
	ResetClock();
}

// 0x409800 (mahjongchina), 0x409970 (mahjongjapan)
void Panel::Init()
{
	mLabelTime = jb::LoadDllString(STRING_PANEL_TIME);
	mLabelScore = jb::LoadDllString(STRING_PANEL_SCORE);
	mLabelBest = jb::LoadDllString(STRING_PANEL_BEST);
	mLabelPairs = jb::LoadDllString(STRING_PANEL_MATCHES);
	mLabelLeft = jb::LoadDllString(STRING_PANEL_TILES);
	mClockFont = jb::GetDllFont(FONT_CLOCK, FONT_CLOCK_MAP);
	mSmallFont = jb::GetDllFont(FONT_SMALL, FONT_SMALL_MAP);
	mTitleFont = jb::GetDllFont(FONT_TITLE, FONT_TITLE_MAP);
#if EDITION_ESCAPE_TITLE_FONT
	if (mMode == 1)
		mEscapeTitleFont = jb::GetDllFont(FONT_ESCAPE_TITLE, FONT_ESCAPE_TITLE_MAP);
#endif
	mLabelFont = jb::GetDllFont(FONT_LABEL, FONT_LABEL_MAP);
	if (edition::PANEL_SMALL_FONT_FIRST)
		mManager->PreloadDllImage(FONT_SMALL);
	mManager->PreloadDllImage(IMAGE_PANEL_TOP);
	mManager->PreloadDllImage(IMAGE_PANEL_TOP_SHADOW);
	mManager->PreloadDllImage(IMAGE_PANEL_BOTTOM_EDGE);
	mManager->PreloadDllImage(IMAGE_PANEL_BOTTOM);
	mManager->PreloadDllImage(IMAGE_PROGRESS_BAR);
	mManager->PreloadDllImage(IMAGE_PROGRESS_TICK);
	mManager->PreloadDllImage(IMAGE_PROGRESS_MARKER);
	mManager->PreloadDllImage(FONT_CLOCK);
	if (!edition::PANEL_SMALL_FONT_FIRST)
		mManager->PreloadDllImage(FONT_SMALL);
	mManager->PreloadDllImage(FONT_TITLE);
	mManager->PreloadDllImage(FONT_LABEL);
	RegUpdate(true);
}

// 0x409970 (mahjongchina), 0x409ab0 (mahjongjapan)
bool Panel::Update()
{
	if (mRunning)
	{
		int aSeconds = GetSeconds();
		if (aSeconds != mShown)
		{
			mShown = aSeconds;
			mTime = jb::FormatTime(aSeconds);
			return true;
		}
	}
	return false;
}

// 0x4099d0 (mahjongchina), 0x409b10 (mahjongjapan)
void Panel::SetLevel(const char* theTitle, int theStat1, int theStat2)
{
	mTitle = theTitle;
	if (theStat1 < 0)
		mStat1 = jb::FormatDll(STRING_YEAR_BC, -theStat1);
	else
		mStat1 = jb::FormatDll(STRING_YEAR_AD, theStat1);
	if (theStat2 < 0)
		mStat2 = jb::FormatDll(STRING_YEAR_BC, -theStat2);
	else
		mStat2 = jb::FormatDll(STRING_YEAR_AD, theStat2);
}

// 0x409a50 (mahjongchina), 0x409b90 (mahjongjapan)
void Panel::SetMoves(int thePairs, int theLeft)
{
	if (thePairs == -1)
		mPairs.clear();
	else
		mPairs = jb::FormatNumber(thePairs);
	if (theLeft == -1)
		mLeft.clear();
	else
		mLeft = jb::FormatNumber(theLeft);
}

// 0x409ae0 (mahjongchina), 0x409c20 (mahjongjapan)
void Panel::SetBestTime(int theSeconds)
{
	if (theSeconds == 36000)
	{
		mBestTime = jb::LoadDllString(STRING_NO_TIME);
		return;
	}
	mBestTime = jb::FormatTime(theSeconds);
}

// 0x409b10 (mahjongchina), 0x409c50 (mahjongjapan)
void Panel::ResetClock()
{
	mTime = jb::FormatTime(0);
	mSeconds = 0;
	mRunning = false;
}

// 0x409b40 (mahjongchina), 0x409c80 (mahjongjapan)
void Panel::StartClock(int theSeconds)
{
	int aNow = GetTime();
	mRunning = true;
	mStart = aNow + theSeconds * -1000;
}

// 0x409b70 (mahjongchina), 0x409cb0 (mahjongjapan)
void Panel::StopClock()
{
	uint32_t aSeconds = (uint32_t)(GetTime() - mStart) / 1000;
	mSeconds = (int)aSeconds;
	if (35999 < aSeconds)
		mSeconds = 35999;
	mRunning = false;
}

// 0x409bb0 (mahjongchina), 0x409cf0 (mahjongjapan)
int Panel::GetSeconds()
{
	uint32_t aSeconds;
	if (!mRunning)
		aSeconds = (uint32_t)mSeconds;
	else
	{
		aSeconds = (uint32_t)(GetTime() - mStart) / 1000;
		if (35999 < aSeconds)
			return 35999;
	}
	return (int)aSeconds;
}

// 0x409bf0 (mahjongchina), 0x409d30 (mahjongjapan)
void Panel::AddSeconds(int theSeconds)
{
	if (mRunning)
	{
		mStart = mStart + theSeconds * -1000;
		return;
	}
	mSeconds = mSeconds + theSeconds * 1000;
}

// 0x40a070 (mahjongchina), 0x40a1c0 (mahjongjapan)
void PauseText::Draw()
{
	mFont->DrawLinesCentred(this, 0, 200, mText.c_str(), jb::SCREEN_WIDTH, mFont->GetLineHeight() - 1, 100, 100, 0xff);
	jb::Rect aRect = {0, 0, jb::SCREEN_WIDTH, jb::SCREEN_HEIGHT};
	AddDirty(aRect);
}

// 0x40a0e0 (mahjongchina), 0x40a240 (mahjongjapan) (it also reads the manager's period, unused)
void PauseText::Init()
{
	mText = jb::LoadDllString(STRING_PAUSED);
	mFont = jb::GetDllFont(FONT_HEADING, FONT_HEADING_MAP);
	RegClick(true);
	jb::gManager->PauseMusic(true);
}

// 0x40a120 (mahjongchina), 0x40a290 (mahjongjapan)
bool PauseText::OnLButtonUp(int theFlags, int theX, int theY)
{
	jb::gManager->PauseMusic(false);
	jb::gManager->Pop(true, 1);
	return true;
}

// 0x40a190 (mahjongchina), 0x40a310 (mahjongjapan)
void PauseLayer::Load()
{
	mText = new PauseText();
	Add(mText, 0, 0);
}

// 0x405280 (mahjongchina), 0x404fe0 (mahjongjapan)
PlayLayer::PlayLayer(int theMode) : Layer(0, 0), mMode(theMode)
{
}

// 0x405330 (mahjongchina), 0x405090 (mahjongjapan)
PlayLayer::~PlayLayer()
{
	jb::gManager->StopSoundsExcept(-1);
}

// 0x4053b0 (mahjongchina), 0x405110 (mahjongjapan)
void PlayLayer::SetLayout(int theLevel, bool theResume, int theLayoutId, const char* theName)
{
	mLevel = theLevel;
	mResume = theResume;
	mLayoutId = theLayoutId;
	mLayoutName = theName;
	if (!gUser.mFreeTipShown)
	{
		mTip = 8;
		gUser.mFreeTipShown = true;
	}
}

// 0x405400 (mahjongchina), 0x405160 (mahjongjapan)
void PlayLayer::PlayNext()
{
	int aNext = mLevel + 1;
	gUser.mSelectedLayout = gLayoutCount <= aNext ? 0 : aNext;
	mLevel = gUser.mSelectedLayout;
	std::string aPath;
	gUser.GetFreeSave(mLevel, aPath);
	if (!jb::FileExists(aPath.c_str()))
		StartLayout(false);
	else
	{
		YesNoBox* aBox = new YesNoBox(this, 2, 0x1e, STRING_CONTINUE, STRING_RESTART, false, 0x78, 0x78);
		jb::gManager->Push(aBox, true, true, true);
		aBox->SetTitle(STRING_SAVED_GAME_TITLE);
		aBox->AddText(STRING_SAVED_GAME_TEXT, 0, 0, 0x5a, false);
	}
}

// 0x405520 (mahjongchina), 0x405280 (mahjongjapan)
void PlayLayer::StartLayout(bool theResume)
{
	mResume = theResume;
	mBoard->SetLayout(GetLayoutId(mLevel), gUser.mQuadPercent, 0xffffffff);
	mPanel->SetMoves(-1, -1);
	mPanel->SetBestTime(gUser.mBestTimes[mLevel]);
	mPanel->SetBestScore(gUser.mBestScores[mLevel]);
	mBoard->LoadLayout();
	std::string aTitle = jb::FormatDll(STRING_LAYOUT_NUMBERED, mLevel + 1, mBoard->GetLayoutName().c_str());
	mPanel->SetTitle(aTitle.c_str());
	ResetGame();
	Begin();
}

// 0x405630 (mahjongchina), 0x405390 (mahjongjapan)
void PlayLayer::StartLevel(int theLevel, bool theResume, int theLayoutId, const char* theTitle, int theBackground,
	int theStat1, int theStat2, int theQuadPercent, uint32_t theSeed, const char* theLevelTitle)
{
	mLevel = theLevel;
	mResume = theResume;
	mBoard->SetLayout(theLayoutId, theQuadPercent, theSeed);
	mPanel->SetLevel(theTitle, theStat1, theStat2);
	mPanel->SetMoves(-1, -1);
	mPanel->SetBestTime(gUser.mEscBestTimes[mLevel]);
	mPanel->SetBestScore(gUser.mEscBestScores[mLevel]);
	mBoard->LoadLayout();
	mBackground->mImage = edition::BACKGROUNDS[theBackground + edition::BACKGROUNDS_AT_START];
	mLevelTitle = theLevelTitle;
	mTip = -1;
	int aProgress = gUser.mProgress;
	if (!theResume)
	{
		int aLevel = mLevel;
		if (aLevel == 0 && gUser.mProgress == 0)
			mTip = 0;
		else if (aLevel == 1 && gUser.mProgress == 1)
			mTip = 6;
		else if (aLevel == edition::TIP_LEVELS[0] && gUser.mProgress == edition::TIP_LEVELS[0])
			mTip = 2;
		else if (aLevel == edition::TIP_LEVELS[1] && gUser.mProgress == edition::TIP_LEVELS[1])
			mTip = 3;
		else if (aLevel == edition::TIP_LEVELS[2] && gUser.mProgress == edition::TIP_LEVELS[2])
			mTip = 4;
	}
	if (mLevel == 0 && aProgress == 0)
		mBoard->mTipGold = true;
	if (mLevel == 1 && aProgress == 1)
		mBoard->mTipFlowers = true;
	ResetGame();
}

// 0x405780 (mahjongchina), 0x4054e0 (mahjongjapan)
void PlayLayer::SetProgress(int theSteps, int theStep)
{
	mPanel->mSteps = theSteps;
	mPanel->mStep = theStep;
}

// 0x4057a0 (mahjongchina), 0x405500 (mahjongjapan)
void PlayLayer::Begin()
{
	mBoard->Start(true, mResume, false);
	if (!AppActive())
		ShowPause();
}

// 0x4057d0 (mahjongchina), 0x405530 (mahjongjapan)
void PlayLayer::Restart()
{
	mResume = false;
	mBoard->ApplyMagnetLinks();
	ResetGame();
	if (mMode == 0)
	{
		mBoard->Start(false, false, false);
		return;
	}
	mBoard->Start(true, false, true);
}

// 0x405810 (mahjongchina), 0x405570 (mahjongjapan)
void PlayLayer::Undo()
{
	mBoard->Undo();
}

// 0x405820 (mahjongchina), 0x405580 (mahjongjapan)
void PlayLayer::Shuffle()
{
	mBoard->Shuffle();
}

// 0x405830 (mahjongchina), 0x405590 (mahjongjapan)
void PlayLayer::Load()
{
	mBackground = new Background();
	Add(mBackground, 0, 0x3c);
	mPanel = new Panel(mMode);
	Add(mPanel, 0, 0);
	mShade = new ShadeBox(0x170 - (mMode != 0));
	Add(mShade, edition::SHADE_SPOT[mMode != 0].x, edition::SHADE_SPOT[mMode != 0].y);
	mShade->SetChecked(gUser.mShadeCovered);
	mShade->SetEnabled(false);

	mUndo = new jb::Button(mMode != 0 ? 0x10d : 0x88, 4, false);
	Add(mUndo, edition::UNDO_SPOT[mMode != 0].x, edition::UNDO_SPOT[mMode != 0].y);
	mUndo->SetLabel(STRING_PANEL_UNDO, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::PANEL_UNDO_LABEL_X,
		edition::PANEL_UNDO_LABEL_Y, 2, 2, 0x40);
	mUndo->SetEnabled(false);

	mHint = new jb::Button(mMode != 0 ? 0x108 : 0x90, 4, false);
	Add(mHint, edition::HINT_SPOT[mMode != 0].x, edition::HINT_SPOT[mMode != 0].y);
	mHint->SetLabel(STRING_PANEL_HINT, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::PANEL_HINT_LABEL_X,
		edition::PANEL_HINT_LABEL_Y, 2, 2, 0x40);
	mHint->SetEnabled(false);

	mShuffle = new jb::Button(mMode != 0 ? 0x10c : 0x94, 4, false);
	Add(mShuffle, edition::SHUFFLE_SPOT[mMode != 0].x, edition::SHUFFLE_SPOT[mMode != 0].y);
	mShuffle->SetLabel(STRING_PANEL_SHUFFLE, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::PANEL_SHUFFLE_LABEL_X,
		edition::PANEL_SHUFFLE_LABEL_Y, 2, 2, 0x40);
	mShuffle->SetEnabled(false);

	mMenu = new jb::Button(mMode != 0 ? 0x109 : 0x91, 4, false);
	Add(mMenu, edition::MENU_SPOT[mMode != 0].x, edition::MENU_SPOT[mMode != 0].y);
	mMenu->SetLabel(STRING_PANEL_MENU, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::PANEL_MENU_LABEL_X,
		edition::PANEL_MENU_LABEL_Y, 2, 2, 0x40);
	mMenu->SetEnabled(false);

	mPause = new jb::Button(mMode != 0 ? 0x10a : 0x92, 4, false);
	Add(mPause, edition::PAUSE_SPOT[mMode != 0].x, edition::PAUSE_SPOT[mMode != 0].y);
	mPause->SetLabel(STRING_PANEL_PAUSE, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::PANEL_PAUSE_LABEL_X,
		edition::PANEL_PAUSE_LABEL_Y, 2, 2, 0x40);
	mPause->SetEnabled(false);

	mBoard = new Board(mMode);
	Add(mBoard, 0, 0x42);
	SetStyle();
	if (mMode == 0)
		jb::gManager->LoadMusic(MUSIC_GAME);
}

// 0x405c30 (mahjongchina), 0x405a00 (mahjongjapan)
void PlayLayer::SetStyle()
{
	if (mBoard != nullptr)
	{
		if (mMode == 1)
		{
			mBoard->SetStyle(gUser.mStyleFree);
			return;
		}
		mBoard->SetStyle(gUser.mStyleEscape);
	}
}

// 0x405c60 (mahjongchina), 0x405a30 (mahjongjapan)
bool PlayLayer::OnCommand(int theCmd)
{
	switch (theCmd)
	{
	case 0x91:
	case 0x109:
		break;
	case 0x92:
	case 0x10a:
		ShowPause();
		return true;
	default:
		return false;
	}
	jb::Layer* aMenu;
	if (mMode == 0)
		aMenu = new EscapeMenu(this);
	else
		aMenu = new FreeMenu(this);
	jb::gManager->Push(aMenu, true, false, true);
	return true;
}

// 0x405dd0 (mahjongchina), 0x405ba0 (mahjongjapan)
void PlayLayer::OnAppActivate(bool theActive)
{
	if (!theActive)
	{
		if (mPause->IsEnabled())
			ShowPause();
	}
}

// 0x405e00 (mahjongchina), 0x405bd0 (mahjongjapan)
void PlayLayer::ShowPause()
{
	if (IsActive())
		jb::gManager->Push(new PauseLayer(), true, true, false);
}

// 0x405e80 (mahjongchina), 0x405c50 (mahjongjapan)
void PlayLayer::ResetGame()
{
	mBoard->Reset();
	if (mResume)
	{
		if (!LoadSave())
			mResume = false;
		if (mResume)
			goto done;
	}
	mScore = 0;
	mPanel->SetScore(0);
	mPanel->ResetClock();
done:
	DeleteSave();
}

// 0x405ee0 (mahjongchina), 0x405cb0 (mahjongjapan)
void PlayLayer::SetMoves(int thePairs, int theLeft)
{
	mPanel->SetMoves(thePairs, theLeft);
}

// 0x405f00 (mahjongchina), 0x405cd0 (mahjongjapan)
void PlayLayer::AddScore(int thePoints)
{
	mScore = mScore + thePoints;
	if (mScore < 0)
		mScore = 0;
	mPanel->SetScore(mScore);
}

// 0x405f30 (mahjongchina), 0x405d00 (mahjongjapan)
void PlayLayer::AddSeconds(int theSeconds)
{
	mPanel->AddSeconds(theSeconds);
}

// 0x405f40 (mahjongchina), 0x405d10 (mahjongjapan)
void PlayLayer::SetClockRunning(bool theOn)
{
	if (theOn)
	{
		mPanel->StartClock(0);
		return;
	}
	mPanel->StopClock();
}

// 0x405f60 (mahjongchina), 0x405d30 (mahjongjapan)
void PlayLayer::EnableButtons(bool theOn)
{
	mShade->SetEnabled(theOn);
	mUndo->SetEnabled(theOn);
	mHint->SetEnabled(theOn);
	mShuffle->SetEnabled(theOn);
	mMenu->SetEnabled(theOn);
	mPause->SetEnabled(theOn);
}

// 0x405fb0 (mahjongchina), 0x405d80 (mahjongjapan)
void PlayLayer::OnEntered()
{
	if (mMode == 0)
	{
		jb::gManager->Push(new MapLayer(this), false, false, false);
		jb::gManager->PlayMusic(MUSIC_GAME, false, 0x44c, gUser.mProgress % 2 != 0 ? 0 : edition::MUSIC_LOOP_EVEN);
		return;
	}
	mBoard->SetLayout(mLayoutId, gUser.mQuadPercent, 0xffffffff);
	mPanel->SetTitle(mLayoutName.c_str());
	mPanel->SetMoves(-1, -1);
	mPanel->SetBestTime(gUser.mBestTimes[mLevel]);
	mPanel->SetBestScore(gUser.mBestScores[mLevel]);
	mBoard->LoadLayout();
	NextBackground();
	ResetGame();
}

// 0x4060d0 (mahjongchina), 0x405ea0 (mahjongjapan)
void PlayLayer::NextBackground()
{
	if (gUser.mBackground != -1)
	{
		mBackground->mImage = edition::BACKGROUNDS[gUser.mBackground];
		return;
	}
	gBgCycle = gBgCycle + 1;
	if (gBgCycle < edition::NUM_BACKGROUNDS)
	{
		if (gBgCycle < CountBackgrounds())
			goto chosen;
	}
	gBgCycle = 0;
chosen:
	mBackground->mImage = edition::BACKGROUNDS[gBgCycle];
}

// 0x406130 (mahjongchina), 0x405f00 (mahjongjapan)
void PlayLayer::OnShown()
{
	if (mMode == 1)
	{
		Begin();
		return;
	}
}

// 0x406140 (mahjongchina), 0x405f10 (mahjongjapan)
void PlayLayer::OnBoardShown()
{
	EnableButtons(true);
	if (mTip != -1)
	{
		mBoard->ShowTip(mTip);
		if (mTip == 6)
			mShade->SetShowDisabled(false);
		mTip = -1;
	}
}

// 0x406180 (mahjongchina), 0x405f50 (mahjongjapan)
void PlayLayer::CheckBestTime()
{
	int aSeconds = mPanel->GetSeconds();
	if (aSeconds < gUser.mEscBestTimes[mLevel])
	{
		gUser.mEscBestTimes[mLevel] = aSeconds;
		mPanel->SetBestTime(aSeconds);
		mNewBestTime = true;
		return;
	}
	mNewBestTime = false;
}

// 0x4061b0 (mahjongchina), 0x405f90 (mahjongjapan)
void PlayLayer::CheckBestScore()
{
	if (gUser.mEscBestScores[mLevel] < mScore)
	{
		gUser.mEscBestScores[mLevel] = mScore;
		mPanel->SetBestScore(mScore);
		mNewBestScore = true;
		return;
	}
	mNewBestScore = false;
}

// 0x4061e0 (mahjongchina), 0x405fd0 (mahjongjapan)
void PlayLayer::ShowMap()
{
	MapLayer* aMap = new MapLayer(this);
	jb::gManager->Push(aMap, true, false, false);
	aMap->SetProgressed(mProgressed);
}

// 0x406260 (mahjongchina), 0x406050 (mahjongjapan)
bool PlayLayer::HasSave()
{
	return mBoard->GetSaveLayout() != nullptr;
}

// 0x406270 (mahjongchina), 0x406060 (mahjongjapan)
void PlayLayer::SaveGame()
{
	Layout* aLayout = mBoard->GetSaveLayout();
	if (aLayout != nullptr)
	{
		SavedGame aGame(aLayout);
		aGame.mMode = mMode;
		aGame.mLevel = mLevel;
		aGame.mScore = mScore;
		aGame.mProgress = gUser.mId;
		aGame.mTime = mPanel->GetSeconds();
		aGame.mMove = mBoard->mMove;
		jb::Stream aStream;
		std::string aPath;
		if (mMode == 0)
			gUser.GetEscapeSave(mLevel, aPath);
		else
			gUser.GetFreeSave(mLevel, aPath);
		if (aStream.OpenWrite(aPath.c_str()))
			aStream.Serialize(&aGame);
	}
}

// 0x406390 (mahjongchina), 0x406190 (mahjongjapan)
bool PlayLayer::LoadSave()
{
	jb::Stream aStream;
	std::string aPath;
	if (mMode == 0)
		gUser.GetEscapeSave(mLevel, aPath);
	else
		gUser.GetFreeSave(mLevel, aPath);
	SavedGame aGame(&mBoard->mLayout);
	if (aStream.OpenRead(aPath.c_str()))
	{
		aStream.Serialize(&aGame);
		if (aGame.IsValid() && aGame.mMode == mMode && aGame.mLevel == mLevel && aGame.mProgress == gUser.mId)
		{
			if (aStream.ChecksumOk())
			{
				mScore = aGame.mScore;
				mPanel->SetScore(aGame.mScore);
				mPanel->StartClock(aGame.mTime);
				mBoard->Resume(aGame.mMove);
				return true;
			}
		}
	}
	return false;
}

// 0x406530 (mahjongchina), 0x406330 (mahjongjapan)
void PlayLayer::DeleteSave()
{
	std::string aPath;
	if (mMode == 0)
		gUser.GetEscapeSave(mLevel, aPath);
	else
		gUser.GetFreeSave(mLevel, aPath);
	jb::RemoveFile(aPath.c_str());
}

#if EDITION_TIPS_OFF
// 0x4063c0 (mahjongjapan)
bool PlayLayer::TipsOff()
{
	return 0 < strtol(mPanel->mBestScore.c_str(), nullptr, 10);
}
#endif

// 0x4065c0 (mahjongchina), 0x4063e0 (mahjongjapan)
bool PlayLayer::OnYes(int theId)
{
	jb::Layer* aLayer;
	switch (theId)
	{
	case 1:
	case 0xb:
		jb::gManager->SlideMusic(0, 0x44c);
		aLayer = new MainMenu();
		break;
	default:
		return false;
	case 0xc:
		jb::gManager->SlideMusic(0, 0x44c);
		aLayer = new LayoutSelect();
		break;
	case 0x1e:
		StartLayout(true);
		return true;
#if EDITION_MENU_RESTART
	case 0x93:
		jb::gManager->Pop(true, 1);
		Restart();
		return false;
#endif
	}
	jb::gManager->SwitchTo(aLayer);
	return false;
}

// 0x406780 (mahjongchina), 0x4064f0 (mahjongjapan)
bool PlayLayer::OnNo(int theId)
{
	if (theId == 0x1e)
		StartLayout(false);
	return true;
}

// 0x4067a0 (mahjongchina), 0x406510 (mahjongjapan)
void PlayLayer::ShowResult()
{
	int aSeconds = mPanel->GetSeconds();
	int aScore = mScore;
	int aBestScore = gUser.mBestScores[mLevel];
	if (aBestScore < aScore)
	{
		gUser.mBestScores[mLevel] = aScore;
		mPanel->SetBestScore(gUser.mBestScores[mLevel]);
	}
	int aBestTime = gUser.mBestTimes[mLevel];
	if (aSeconds < aBestTime)
	{
		gUser.mBestTimes[mLevel] = aSeconds;
		mPanel->SetBestTime(gUser.mBestTimes[mLevel]);
	}
	jb::gManager->Push(new ResultBox(aSeconds, gUser.mBestTimes[mLevel], aSeconds < aBestTime, mScore,
		gUser.mBestScores[mLevel], aBestScore < aScore), true, false, true);
}

// 0x4068a0 (mahjongchina), 0x406610 (mahjongjapan)
void PlayLayer::ShowLevelDone()
{
	int aSeconds = mPanel->GetSeconds();
	jb::gManager->StopSoundsExcept(-1);
	jb::gManager->Push(new LevelDone(aSeconds, gUser.mEscBestTimes[mLevel], mNewBestTime, mScore,
		gUser.mEscBestScores[mLevel], mNewBestScore, mLevelTitle.c_str()), true, false, true);
}

// 0x406930 (mahjongchina), 0x4066d0 (mahjongjapan)
void PlayLayer::Advance()
{
	if (mLevel == gUser.mProgress)
	{
		gUser.mProgress = gUser.mProgress + 1;
		mProgressed = true;
		return;
	}
	mProgressed = false;
}

} // namespace mahjong
