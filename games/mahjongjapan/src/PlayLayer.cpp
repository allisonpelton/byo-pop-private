#include "PlayLayer.h"

#include "Board.h"
#include "Font.h"
#include "GameDialogs.h"
#include "Globals.h"
#include "LayoutSelect.h"
#include "MainMenu.h"
#include "Manager.h"
#include "MapLayer.h"
#include "Stream.h"
#include "User.h"

#include <cstdlib>

namespace mahjongjapan
{

// 0x404f30
Background::~Background()
{
	jb::FreeImage(mImage);
}

// 0x404f90
void Background::Draw()
{
	if (mImage != 0)
		DrawImage(0, 0, mImage, nullptr, true, 100, 100, 0xff);
}

// 0x404fc0
void Background::Init()
{
	if (mImage != 0)
		mManager->PreloadDllImage(mImage);
}

// 0x406800
void ShadeBox::Draw()
{
	int anAlpha;
	Button::Draw();
	if (!IsEnabled() && GetShowDisabled())
	{
		mFont->DrawCentred(this, 0xf, -1, mLine1.c_str(), 0, 100, 100, 0x40);
		SetTrackDirty(false);
		anAlpha = 0x40;
	}
	else
	{
		mFont->DrawCentred(this, 0xf, -1, mLine1.c_str(), 0, 100, 100, 0xff);
		SetTrackDirty(false);
		anAlpha = 0xff;
	}
	mFont->DrawCentred(this, 0xf, 0x2f, mLine2.c_str(), 0, 100, 100, anAlpha);
	SetTrackDirty(true);
}

// 0x4068c0
void ShadeBox::OnInit()
{
	mFont = jb::GetDllFont(0xd5, 0xd4);
	mLine1 = jb::LoadDllString(0x8f);
	mLine2 = jb::LoadDllString(0x90);
}

// 0x406900
void ShadeBox::OnClick()
{
	CheckBox::OnClick();
	gUser.mShadeCovered = IsChecked();
}

// 0x406920
void ShadeBox::OnEnabledChanged()
{
	if (IsEnabled())
		SetShowDisabled(true);
}

// 0x409270
Panel::Panel(int theMode) : mMode(theMode)
{
	ResetClock();
}

// 0x409510
void Panel::Draw()
{
	int aX;
	int aY;
	int anImage;
	if (mMode == 0)
	{
		DrawImage(0, 0, 0x7f, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x41, 0x7d, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x1ef, 0x80, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x1ff, 0x7e, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x4d, 0x235, nullptr, true, 100, 100, 0xff);
		anImage = 0x234;
		aY = 0x4d;
		aX = 0x316;
	}
	else
	{
		DrawImage(0, 0, 0x105, nullptr, true, 100, 100, 0xff);
		anImage = 0x113;
		aY = 0x1ff;
		aX = 0;
	}
	DrawImage(aX, aY, anImage, nullptr, true, 100, 100, 0xff);
	mTitleFont->DrawRight(this, 0x40, 0x18, mLabelTime.c_str(), 100, 100, 0xff);
	mTitleFont->DrawRight(this, 0x2a8, 0x18, mLabelScore.c_str(), 100, 100, 0xff);
	mLabelFont->DrawRight(this, 0x40, 0x2e, mLabelBest.c_str(), 100, 100, 0xff);
	mLabelFont->DrawRight(this, 0x2a8, 0x2e, mLabelBest.c_str(), 100, 100, 0xff);
	mLabelFont->DrawCentred(this, 0x3b, 0x21b, mLabelPairs.c_str(), 0x2a, 100, 100, 0xff);
	mLabelFont->DrawCentred(this, 0x3b, 0x23e, mLabelLeft.c_str(), 0x2a, 100, 100, 0xff);
	mTitleFont->DrawCentred(this, 0, mMode == 1 ? 0x1e : 0x18, mTitle.c_str(), 800, 100, 100, 0xff);
	if (!mStat1.empty())
		mSmallFont->DrawRight(this, 0xf2, 0x2d, mStat1.c_str(), 100, 100, 0xff);
	if (!mStat2.empty())
		mSmallFont->DrawLeft(this, 0x22b, 0x2d, mStat2.c_str(), -1, 100, 100, 0xff);
	if (!mBestTime.empty())
		mSmallFont->DrawCentred(this, 0x55, 0x2d, mBestTime.c_str(), 0x52, 100, 100, 0xff);
	if (!mBestScore.empty())
		mSmallFont->DrawCentred(this, 0x2b7, 0x2d, mBestScore.c_str(), 0x4c, 100, 100, 0xff);
	mClockFont->DrawLeft(this, 0x5f, 0x16, mTime.c_str(), -1, 100, 100, 0xff);
	mClockFont->DrawCentred(this, 0x2b7, 0x16, mScore.c_str(), 0x4c, 100, 100, 0xff);
	mSmallFont->DrawCentred(this, 0x3a, 0x22a, mPairs.c_str(), 0x28, 100, 100, 0xff);
	mSmallFont->DrawCentred(this, 0x3a, 0x24e, mLeft.c_str(), 0x28, 100, 100, 0xff);
	if (0 < mSteps)
	{
		int aWidth = 0x124 / mSteps;
		jb::Rect aRect = {0, 0, mStep * aWidth + 6, 0xe};
		if (0 < mStep)
			DrawImage(0xfe, 0x22, 0xfd, &aRect, true, 100, 100, 0xff);
		int i = 1;
		if (1 < mSteps)
		{
			int anX = aWidth + 0xfc;
			do
			{
				DrawImage(anX, 0x26, 0xff, nullptr, true, 100, 100, 0xff);
				i++;
				anX += aWidth;
			} while (i < mSteps);
		}
		if (0 < mStep)
			DrawImage(mStep * aWidth + 0xfd, 0x20, 0xfe, nullptr, true, 100, 100, 0xff);
	}
}

// 0x409970
void Panel::Init()
{
	mLabelTime = jb::LoadDllString(0xa1);
	mLabelScore = jb::LoadDllString(0x9e);
	mLabelBest = jb::LoadDllString(0x98);
	mLabelPairs = jb::LoadDllString(0x9a);
	mLabelLeft = jb::LoadDllString(0xa0);
	mClockFont = jb::GetDllFont(0xcb, 0xca);
	mSmallFont = jb::GetDllFont(0xa3, 0xa2);
	mTitleFont = jb::GetDllFont(0xd2, 0xd3);
	mLabelFont = jb::GetDllFont(0xd5, 0xd4);
	mManager->PreloadDllImage(0xa3);
	mManager->PreloadDllImage(0x7f);
	mManager->PreloadDllImage(0x7d);
	mManager->PreloadDllImage(0x80);
	mManager->PreloadDllImage(0x7e);
	mManager->PreloadDllImage(0xfd);
	mManager->PreloadDllImage(0xff);
	mManager->PreloadDllImage(0xfe);
	mManager->PreloadDllImage(0xcb);
	mManager->PreloadDllImage(0xd2);
	mManager->PreloadDllImage(0xd5);
	RegUpdate(true);
}

// 0x409ab0
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

// 0x409b10
void Panel::SetLevel(const char* theTitle, int theStat1, int theStat2)
{
	mTitle = theTitle;
	if (theStat1 < 0)
		mStat1 = jb::FormatDll(7, -theStat1);
	else
		mStat1 = jb::FormatDll(6, theStat1);
	if (theStat2 < 0)
		mStat2 = jb::FormatDll(7, -theStat2);
	else
		mStat2 = jb::FormatDll(6, theStat2);
}

// 0x409b90
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

// 0x409c20
void Panel::SetBestTime(int theSeconds)
{
	if (theSeconds == 36000)
	{
		mBestTime = jb::LoadDllString(0x27);
		return;
	}
	mBestTime = jb::FormatTime(theSeconds);
}

// 0x409c50
void Panel::ResetClock()
{
	mTime = jb::FormatTime(0);
	mSeconds = 0;
	mRunning = false;
}

// 0x409c80
void Panel::StartClock(int theSeconds)
{
	int aNow = GetTime();
	mRunning = true;
	mStart = aNow + theSeconds * -1000;
}

// 0x409cb0
void Panel::StopClock()
{
	uint32_t aSeconds = (uint32_t)(GetTime() - mStart) / 1000;
	mSeconds = (int)aSeconds;
	if (35999 < aSeconds)
		mSeconds = 35999;
	mRunning = false;
}

// 0x409cf0
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

// 0x409d30
void Panel::AddSeconds(int theSeconds)
{
	if (mRunning)
	{
		mStart = mStart + theSeconds * -1000;
		return;
	}
	mSeconds = mSeconds + theSeconds * 1000;
}

// 0x40a1c0
void PauseText::Draw()
{
	mFont->DrawLinesCentred(this, 0, 200, mText.c_str(), 800, mFont->GetLineHeight() - 1, 100, 100, 0xff);
	jb::Rect aRect = {0, 0, 800, 600};
	AddDirty(aRect);
}

// 0x40a240 (it also reads the manager's period, unused)
void PauseText::Init()
{
	mText = jb::LoadDllString(0x28);
	mFont = jb::GetDllFont(0x11e, 0x11d);
	RegClick(true);
	jb::gManager->PauseMusic(true);
}

// 0x40a290
bool PauseText::OnLButtonUp(int theFlags, int theX, int theY)
{
	jb::gManager->PauseMusic(false);
	jb::gManager->Pop(true, 1);
	return true;
}

// 0x40a310
void PauseLayer::Load()
{
	mText = new PauseText();
	Add(mText, 0, 0);
}

// 0x404fe0
PlayLayer::PlayLayer(int theMode) : Layer(0, 0), mMode(theMode)
{
}

// 0x405090
PlayLayer::~PlayLayer()
{
	jb::gManager->StopSoundsExcept(-1);
}

// 0x405110
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

// 0x405160
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
		YesNoBox* aBox = new YesNoBox(this, 2, 0x1e, 0x8c, 0x8d, false, 0x78, 0x78);
		jb::gManager->Push(aBox, true, true, true);
		aBox->SetTitle(0x8e);
		aBox->AddText(0x8b, 0, 0, 0x5a, false);
	}
}

// 0x405280
void PlayLayer::StartLayout(bool theResume)
{
	mResume = theResume;
	mBoard->SetLayout(GetLayoutId(mLevel), gUser.mQuadPercent, 0xffffffff);
	mPanel->SetMoves(-1, -1);
	mPanel->SetBestTime(gUser.mBestTimes[mLevel]);
	mPanel->SetBestScore(gUser.mBestScores[mLevel]);
	mBoard->LoadLayout();
	std::string aTitle = jb::FormatDll(0x13, mLevel + 1, mBoard->GetLayoutName().c_str());
	mPanel->SetTitle(aTitle.c_str());
	ResetGame();
	Begin();
}

// 0x405390
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
	mBackground->mImage = gBackgrounds[theBackground + 9];
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
		else if (aLevel == 2 && gUser.mProgress == 2)
			mTip = 2;
		else if (aLevel == 5 && gUser.mProgress == 5)
			mTip = 3;
		else if (aLevel == 9 && gUser.mProgress == 9)
			mTip = 4;
	}
	if (mLevel == 0 && aProgress == 0)
		mBoard->mTipKeys = true;
	if (mLevel == 1 && aProgress == 1)
		mBoard->mTipFlowers = true;
	ResetGame();
}

// 0x4054e0
void PlayLayer::SetProgress(int theSteps, int theStep)
{
	mPanel->mSteps = theSteps;
	mPanel->mStep = theStep;
}

// 0x405500
void PlayLayer::Begin()
{
	mBoard->Start(true, mResume, false);
	if (!AppActive())
		ShowPause();
}

// 0x405530
void PlayLayer::Restart()
{
	mResume = false;
	mBoard->ApplyWildLinks();
	ResetGame();
	if (mMode == 0)
	{
		mBoard->Start(false, false, false);
		return;
	}
	mBoard->Start(true, false, true);
}

// 0x405570
void PlayLayer::Undo()
{
	mBoard->Undo();
}

// 0x405580
void PlayLayer::Shuffle()
{
	mBoard->Shuffle();
}

// 0x405590
void PlayLayer::Load()
{
	mBackground = new Background();
	Add(mBackground, 0, 0x3c);
	mPanel = new Panel(mMode);
	Add(mPanel, 0, 0);
	mShade = new ShadeBox(0x170 - (mMode != 0));
	Add(mShade, mMode == 0 ? 0xbf : 0xc2, 0x222);
	mShade->SetChecked(gUser.mShadeCovered);
	mShade->SetEnabled(false);

	mUndo = new jb::Button(mMode != 0 ? 0x10d : 0x88, 4, false);
	if (mMode == 0)
		Add(mUndo, 0xf9, 0x222);
	else
		Add(mUndo, 0xf8, 0x21c);
	mUndo->SetLabel(0xa2, 0xa0, 0x9f, 0, 0x55, 0x18, 2, 2, 0x40);
	mUndo->SetEnabled(false);

	mHint = new jb::Button(mMode != 0 ? 0x108 : 0x90, 4, false);
	if (mMode == 0)
		Add(mHint, 0x1c8, 0x222);
	else
		Add(mHint, 0x1ca, 0x21c);
	mHint->SetLabel(0x99, 0xa0, 0x9f, 0, 0x56, 0x17, 2, 2, 0x40);
	mHint->SetEnabled(false);

	mShuffle = new jb::Button(mMode != 0 ? 0x10c : 0x94, 4, false);
	if (mMode == 0)
		Add(mShuffle, 0x225, 0x222);
	else
		Add(mShuffle, 0x227, 0x21c);
	mShuffle->SetLabel(0x9f, 0xa0, 0x9f, 0, 0x66, 0x17, 2, 2, 0x40);
	mShuffle->SetEnabled(false);

	mMenu = new jb::Button(mMode != 0 ? 0x109 : 0x91, 4, false);
	if (mMode == 0)
		Add(mMenu, 0x2ad, 0x213);
	else
		Add(mMenu, 0x2b4, 0x20f);
	mMenu->SetLabel(0x9b, 0xa0, 0x9f, 0, 0x5a, 0x14, 2, 2, 0x40);
	mMenu->SetEnabled(false);

	mPause = new jb::Button(mMode != 0 ? 0x10a : 0x92, 4, false);
	if (mMode == 0)
		Add(mPause, 0x2ad, 0x235);
	else
		Add(mPause, 0x2b4, 0x231);
	mPause->SetLabel(0x9c, 0xa0, 0x9f, 0, 0x5a, 0x14, 2, 2, 0x40);
	mPause->SetEnabled(false);

	mBoard = new Board(mMode);
	Add(mBoard, 0, 0x42);
	SetStyle();
	if (mMode == 0)
		jb::gManager->LoadMusic(0x153);
}

// 0x405a00
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

// 0x405a30
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

// 0x405ba0
void PlayLayer::OnAppActivate(bool theActive)
{
	if (!theActive)
	{
		if (mPause->IsEnabled())
			ShowPause();
	}
}

// 0x405bd0
void PlayLayer::ShowPause()
{
	if (IsActive())
		jb::gManager->Push(new PauseLayer(), true, true, false);
}

// 0x405c50
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

// 0x405cb0
void PlayLayer::SetMoves(int thePairs, int theLeft)
{
	mPanel->SetMoves(thePairs, theLeft);
}

// 0x405cd0
void PlayLayer::AddScore(int thePoints)
{
	mScore = mScore + thePoints;
	if (mScore < 0)
		mScore = 0;
	mPanel->SetScore(mScore);
}

// 0x405d00
void PlayLayer::AddSeconds(int theSeconds)
{
	mPanel->AddSeconds(theSeconds);
}

// 0x405d10
void PlayLayer::SetClockRunning(bool theOn)
{
	if (theOn)
	{
		mPanel->StartClock(0);
		return;
	}
	mPanel->StopClock();
}

// 0x405d30
void PlayLayer::EnableButtons(bool theOn)
{
	mShade->SetEnabled(theOn);
	mUndo->SetEnabled(theOn);
	mHint->SetEnabled(theOn);
	mShuffle->SetEnabled(theOn);
	mMenu->SetEnabled(theOn);
	mPause->SetEnabled(theOn);
}

// 0x405d80
void PlayLayer::OnEntered()
{
	if (mMode == 0)
	{
		jb::gManager->Push(new MapLayer(this), false, false, false);
		jb::gManager->PlayMusic(0x153, false, 0x44c, gUser.mProgress % 2 != 0 ? 0 : 10);
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

// 0x405ea0
void PlayLayer::NextBackground()
{
	if (gUser.mBackground != -1)
	{
		mBackground->mImage = gBackgrounds[gUser.mBackground];
		return;
	}
	gBgCycle = gBgCycle + 1;
	if (gBgCycle < 0x15)
	{
		if (gBgCycle < CountBackgrounds())
			goto chosen;
	}
	gBgCycle = 0;
chosen:
	mBackground->mImage = gBackgrounds[gBgCycle];
}

// 0x405f00
void PlayLayer::OnShown()
{
	if (mMode == 1)
	{
		Begin();
		return;
	}
}

// 0x405f10
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

// 0x405f50
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

// 0x405f90
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

// 0x405fd0
void PlayLayer::ShowMap()
{
	MapLayer* aMap = new MapLayer(this);
	jb::gManager->Push(aMap, true, false, false);
	aMap->SetProgressed(mProgressed);
}

// 0x406050
bool PlayLayer::HasSave()
{
	return mBoard->GetSaveLayout() != nullptr;
}

// 0x406060
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

// 0x406190
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

// 0x406330
void PlayLayer::DeleteSave()
{
	std::string aPath;
	if (mMode == 0)
		gUser.GetEscapeSave(mLevel, aPath);
	else
		gUser.GetFreeSave(mLevel, aPath);
	jb::RemoveFile(aPath.c_str());
}

// 0x4063c0
bool PlayLayer::TipsOff()
{
	return 0 < strtol(mPanel->mBestScore.c_str(), nullptr, 10);
}

// 0x4063e0
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
	}
	jb::gManager->SwitchTo(aLayer);
	return false;
}

// 0x4064f0
bool PlayLayer::OnNo(int theId)
{
	if (theId == 0x1e)
		StartLayout(false);
	return true;
}

// 0x406510
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

// 0x406610
void PlayLayer::ShowLevelDone()
{
	int aSeconds = mPanel->GetSeconds();
	jb::gManager->StopSoundsExcept(-1);
	jb::gManager->Push(new LevelDone(aSeconds, gUser.mEscBestTimes[mLevel], mNewBestTime, mScore,
		gUser.mEscBestScores[mLevel], mNewBestScore, mLevelTitle.c_str()), true, false, true);
}

// 0x4066d0
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

} // namespace mahjongjapan
