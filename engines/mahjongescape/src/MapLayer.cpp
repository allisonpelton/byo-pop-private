#include "MapLayer.h"

#include "Edition.h"
#include "Ending.h"
#include "EngineRes.h"
#include "Font.h"
#include "GameDialogs.h"
#include "GameText.h"
#include "JBBase.h"
#include "MainMenu.h"
#include "Manager.h"
#include "PlayLayer.h"
#include "Res.h"
#include "TextResource.h"
#include "User.h"

#include <cstdlib>

namespace mahjong
{

using edition::LEVEL_BACKGROUNDS;
using edition::MAP_PATH;
using edition::STAGE_IMAGES;
using edition::STAGE_LEVELS;
using edition::STAGE_NAMES;
using edition::STAGE_RECTS;
using edition::STAGE_STATS;

static int PointX(int theLevel) { return MAP_PATH[theLevel * 2]; }
static int PointY(int theLevel) { return MAP_PATH[theLevel * 2 + 1]; }
static int BarWidth(int theProgress) { return MAP_PATH[2 * edition::NUM_LEVELS + theProgress]; }

// 0x40f770 (mahjongchina), 0x40f7d0 (mahjongjapan)
void MapView::Draw()
{
	bool anEnd;
	if ((!mMoving && !mBack) || (anEnd = true, gUser.mProgress != edition::NUM_LEVELS))
		anEnd = false;
	int aShown = gUser.mProgress;
	if (mMoving || mBack)
		aShown = gUser.mProgress - 1;
	DrawImage(0, 0, IMAGE_MAP, nullptr, true, 100, 100, 0xff);
	if (edition::MAP_ENDS_TEXT_FIRST)
	{
		mFont->DrawLeft(this, 8, edition::MAP_ENDS_TEXT_Y, mStartText.c_str(), -1, 100, 100, 0xff);
		mFont->DrawRight(this, 0x312, edition::MAP_ENDS_TEXT_Y, mEndText.c_str(), 100, 100, 0xff);
	}
	int i = 0;
	do
	{
		if (aShown < STAGE_LEVELS[i])
			break;
		DrawImage(STAGE_RECTS[i][0], STAGE_RECTS[i][1], STAGE_IMAGES[i], nullptr, true, 100, 100, 0xff);
		i++;
	} while (i < edition::NUM_STAGES);
	int aLast = 0;
	if (0 < aShown)
	{
		int k = 0;
		int aCount = aShown;
		do
		{
			DrawImage(PointX(k), PointY(k), IMAGE_MAP_POINT, nullptr, true, 100, 100, 0xff);
			k++;
			aCount--;
			aLast = aShown;
		} while (aCount != 0);
	}
	if (aShown < edition::NUM_LEVELS)
		DrawImage(PointX(aLast) + edition::MAP_RING_SHIFT, PointY(aLast) + edition::MAP_RING_SHIFT,
			IMAGE_MAP_POINT_RING, nullptr, true, 100, 100, 0xff);
	if (mMoving && (aShown = aShown + 1, mNewStage) && i < edition::NUM_STAGES)
		DrawImage(STAGE_RECTS[i][0], STAGE_RECTS[i][1], STAGE_IMAGES[i], nullptr, true, 100, 100, mMoveAlpha);
	jb::Rect aBar;
	aBar.left = 0;
	aBar.top = 0;
	aBar.right = BarWidth(aShown);
	aBar.bottom = edition::MAP_BAR_HEIGHT;
	if (mMoving)
		aBar.right = ((aBar.right - BarWidth(aShown - 1)) * mMoveAlpha) / 0xff + BarWidth(aShown - 1);
	DrawImage(edition::MAP_BAR_X, edition::MAP_BAR_Y, IMAGE_MAP_BAR, &aBar, true, 100, 100, 0xff);
	if (edition::MAP_BAR_LAST < aBar.right)
		aBar.right = edition::MAP_BAR_LAST;
	DrawImage(aBar.right + edition::MAP_MARKER_SHIFT_X, edition::MAP_MARKER_Y, IMAGE_MAP_MARKER, nullptr, true, 100,
		100, 0xff);
	if (!edition::MAP_ENDS_TEXT_FIRST)
	{
		mFont->DrawLeft(this, 8, edition::MAP_ENDS_TEXT_Y, mStartText.c_str(), -1, 100, 100, 0xff);
		mFont->DrawRight(this, 0x312, edition::MAP_ENDS_TEXT_Y, mEndText.c_str(), 100, 100, 0xff);
	}
	uint32_t aBlink = mTick % 0x28;
	if (0x14 < aBlink)
		aBlink = 0x28 - aBlink;
	int aY;
	int aX;
	if (anEnd)
	{
		aY = edition::MAP_END_GLOW_Y;
		aX = edition::MAP_END_GLOW_X;
	}
	else if (!mMoving)
	{
		aY = PointY(mLevel) - edition::MAP_GLOW_SHIFT;
		aX = PointX(mLevel) - edition::MAP_GLOW_SHIFT;
	}
	else
	{
		if (aShown < 1)
		{
			aX = edition::MAP_FIRST_GLOW_X;
			aY = edition::MAP_FIRST_GLOW_Y;
		}
		else
		{
			aX = PointX(aShown - 1);
			aY = PointY(aShown - 1);
		}
		aY = ((PointY(aShown) - aY) * mMoveAlpha) / 0xff + -edition::MAP_GLOW_SHIFT + aY;
		aX = ((PointX(aShown) - aX) * mMoveAlpha) / 0xff + -edition::MAP_GLOW_SHIFT + aX;
	}
	DrawImage(aX, aY, IMAGE_MAP_GLOW, nullptr, true, 100, 100, (int)(aBlink * 200) / 0x14 + 0x37);
	if (aShown == 0 && 0 < mTextAlpha)
	{
		DrawImage(0x2a, 10, IMAGE_MAP_TEXT_SHADE, nullptr, true, 400, 400, mTextAlpha);
		mIntroFont->DrawLinesCentred(this, 0, 200, mIntroText.c_str(), jb::SCREEN_WIDTH, 0x1a, 100, 100, mTextAlpha);
	}
	if (mNewStage && 0 < mTextAlpha)
	{
		DrawImage(0x80, 0x82, IMAGE_MAP_TEXT_TOP, nullptr, true, 100, 100, mTextAlpha);
		mStageFont->DrawLinesCentred(this, 0, 0xec, mStageText.c_str(), jb::SCREEN_WIDTH, 0x26, 100, 100, mTextAlpha);
		DrawImage(0x80, 0x1a4, IMAGE_MAP_TEXT_BOTTOM, nullptr, true, 100, 100, mTextAlpha);
	}
}

// 0x40fc00 (mahjongchina), 0x40fc70 (mahjongjapan)
void MapView::Init()
{
	mStartText = jb::LoadDllString(STRING_FIRST_YEAR);
	mEndText = jb::LoadDllString(STRING_LAST_YEAR);
	mFont = jb::GetDllFont(FONT_SMALL, FONT_SMALL_MAP);
	mIntroFont = jb::GetDllFont(FONT_MAP_INTRO, FONT_MAP_INTRO_MAP);
	mIntroText = jb::LoadDllString(STRING_MAP_INTRO);
	mStageFont = jb::GetDllFont(FONT_MAP_STAGE, FONT_MAP_STAGE_MAP);
	int aProgress = gUser.mProgress;
	mLevel = aProgress;
	if (edition::NUM_LEVELS - 1 < aProgress)
		mLevel = edition::NUM_LEVELS - 1;
	jb::LoadSound(SOUND_STAGE_TEXT_GONE, false);
	if (edition::MAP_MOVE_SOUND_FIRST)
		jb::LoadSound(SOUND_MAP_MOVE, false);
	jb::LoadSound(SOUND_NEW_STAGE, false);
	if (!edition::MAP_MOVE_SOUND_FIRST)
		jb::LoadSound(SOUND_MAP_MOVE, false);
	RegUpdate(true);
	RegClick(true);
	RegMove(true);
	RegHover(true);
	SetUserFlag(true);
}

// 0x40fcf0 (mahjongchina), 0x40fd60 (mahjongjapan)
bool MapView::Update()
{
	if (mFadeIn)
	{
		int anAlpha = mTextAlpha + GetDelta() / 3;
		mTextAlpha = anAlpha;
		if (0xfe < anAlpha)
		{
			mTextAlpha = 0xff;
			mFadeIn = false;
		}
	}
	else if (mNewStage && 0 < mTextHold)
		mTextHold = mTextHold - GetDelta();
	else if (mNewStage && 0 < mTextAlpha)
	{
		int anAlpha = mTextAlpha + GetDelta() / -3;
		mTextAlpha = anAlpha;
		if (anAlpha < 0)
		{
			mTextAlpha = 0;
			jb::PlaySound(SOUND_STAGE_TEXT_GONE);
		}
	}
	else if (mMoving)
	{
		int anAlpha = mMoveAlpha + GetDelta() / 5;
		mMoveAlpha = anAlpha;
		if (0xfe < anAlpha)
		{
			mMoveAlpha = 0xff;
			mMoving = false;
			mLevel = mLevel + 1;
			if (gUser.mProgress == edition::NUM_LEVELS)
				AddTimer(1, 1000, 1, 1);
		}
	}
	uint32_t aTick = mTick + 1;
	mTick = aTick;
	return (~aTick & 1) != 0;
}

// 0x40fe50 (mahjongchina), 0x40fec0 (mahjongjapan)
bool MapView::OnTimer(int theId, int theIndex)
{
	if (theId == 1)
	{
		static_cast<MapLayer*>(mLayer)->ShowEnd();
		jb::gManager->Push(new EndLayer(), true, false, true);
	}
	return true;
}

// 0x40fed0 (mahjongchina), 0x40ff40 (mahjongjapan)
bool MapView::OnLButtonDown(int theFlags, int theX, int theY)
{
	if (mMoving)
		return true;
	int aLevel = HitTest(theX, theY);
	if (aLevel != -1)
	{
		jb::PlaySound(jb::SOUND_CLICK);
		mLevel = aLevel;
		jb::Invalidate();
		return true;
	}
	return false;
}

// 0x40ff20 (mahjongchina), 0x40ffa0 (mahjongjapan)
bool MapView::OnLButtonDblClk(int theFlags, int theX, int theY)
{
	if (mMoving)
		return true;
	int aLevel = HitTest(theX, theY);
	if (aLevel != -1)
	{
		mLevel = aLevel;
		jb::PlaySound(jb::SOUND_CLICK);
		static_cast<MapLayer*>(mLayer)->Continue();
		return true;
	}
	return false;
}

// 0x40ff70 (mahjongchina), 0x410000 (mahjongjapan) (the last stage's picture, reached only at the end, is not tested)
int MapView::HitTest(int theX, int theY)
{
	if (gUser.mProgress != 0)
	{
		int i = 0;
		if (-1 < gUser.mProgress)
		{
			do
			{
				if (edition::NUM_LEVELS - 1 < i)
					break;
				if (PointX(i) - 8 < theX && theX < PointX(i) + 0xe && PointY(i) - 8 < theY && theY < PointY(i) + 0xe)
					return i;
				i = i + 1;
			} while (i <= gUser.mProgress);
		}
		i = 0;
		do
		{
			if (STAGE_LEVELS[i] <= gUser.mProgress && STAGE_RECTS[i][0] < theX && theX < STAGE_RECTS[i][2] &&
				STAGE_RECTS[i][1] < theY && theY < STAGE_RECTS[i][3])
				return STAGE_LEVELS[i];
			i = i + 1;
		} while (i < edition::NUM_STAGES - 1);
	}
	return -1;
}

// 0x410010 (mahjongchina), 0x4100a0 (mahjongjapan)
void MapView::OnMouseMove(int theFlags, int theX, int theY)
{
	if (HitTest(theX, theY) != -1)
	{
		SetCursorKind(jb::CURSOR_KIND_HAND, true);
		return;
	}
	SetCursorKind(jb::CURSOR_KIND_ARROW, true);
}

// 0x410050 (mahjongchina), 0x4100e0 (mahjongjapan)
void MapView::StepBack()
{
	mBack = true;
	mLevel = mLevel - 1;
}

// 0x410070 (mahjongchina), 0x410100 (mahjongjapan)
void MapView::StartMove(bool theNewStage, const char* theStageName)
{
	mMoving = true;
	mBack = false;
	mNewStage = theNewStage;
	if (theNewStage)
	{
		mTextAlpha = 0;
		mTextHold = 3000;
		mStageText = jb::FormatDll(STRING_STAGE_COMPLETED, theStageName);
		mFadeIn = true;
		jb::PlaySound(SOUND_NEW_STAGE);
		return;
	}
	jb::PlaySound(SOUND_MAP_MOVE);
}

// 0x410cc0 (mahjongchina), 0x4111e0 (mahjongjapan)
void MapCurtain::Draw()
{
	if (mX != 0xffff)
		DrawImage(mX, 0, IMAGE_MAP_CURTAIN, nullptr, true, 100, 100, 0xff);
}

// 0x410cf0 (mahjongchina), 0x411210 (mahjongjapan)
void MapCurtain::Init()
{
	mManager->PreloadDllImage(IMAGE_MAP_CURTAIN);
	SetUseClip(false);
}

// 0x410d10 (mahjongchina), 0x411230 (mahjongjapan)
bool MapCurtain::Update()
{
	if (mX != 0xffff)
	{
		int aX;
		if (!mClosing)
			aX = mX - GetDelta();
		else
			aX = mX + GetDelta();
		mX = aX;
		if (!mClosing ? -0x90 < aX : mX < jb::SCREEN_WIDTH)
		{
			mClip.mRect = {0, 0, mX + 0x2e, jb::SCREEN_HEIGHT};
			mLayer->SetClip(&mClip);
			mPlayClip.mRect = {mX + 0x6c, 0, jb::SCREEN_WIDTH, jb::SCREEN_HEIGHT};
			mPlay->SetClip(&mPlayClip);
			return true;
		}
		mX = 0xffff;
		RegUpdate(false);
		jb::gManager->SetVBlankWait(false);
		static_cast<MapLayer*>(mLayer)->CurtainDone();
	}
	return false;
}

// 0x410e00 (mahjongchina), 0x411320 (mahjongjapan)
void MapCurtain::Open()
{
	mClosing = false;
	mX = 0x348;
	jb::gManager->SetVBlankWait(true);
	RegUpdate(true);
}

// 0x410e30 (mahjongchina), 0x411350 (mahjongjapan)
void MapCurtain::Close()
{
	mClip.mRect = {0, 0, 0, jb::SCREEN_HEIGHT};
	mLayer->SetClip(&mClip);
	mClosing = true;
	mX = -0xb8;
	jb::gManager->SetVBlankWait(true);
	RegUpdate(true);
}

#if EDITION_RESTART_BOX
// 0x4101d0 (mahjongjapan)
void RestartBox::Create()
{
	int aLeft[] = {2, 3, 0};
	int aRight[] = {3, 2, 0};
	int aTop[] = {2, 3, 3, 3, 0};
	int aBottom[] = {3, 3, 2, 3, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(STRING_REPLAY_TITLE);
	AddText(STRING_REPLAY_TEXT, 0, 0x28, 0x5a, false);
	DialogButton* aButton = new DialogButton(0x1b);
	Add(aButton, 0xa0, 0xd2);
	aButton->SetText(STRING_YES);
	aButton = new DialogButton(0x16);
	Add(aButton, 0x104, 0xd2);
	aButton->SetText(STRING_NO);
}

// 0x410330 (mahjongjapan)
bool RestartBox::OnCommand(int theCmd)
{
	if (theCmd == 0x16)
	{
		jb::gManager->Pop(true, 1);
		return true;
	}
	if (theCmd == 0x1b)
	{
		YesNoBox* aBox = new YesNoBox(mMap, 2, 2, STRING_YES, STRING_NO, true, 0x66, 0x66);
		jb::gManager->Push(aBox, true, false, true);
		aBox->SetTitle(STRING_RESTART_GAME_TITLE);
		std::string aText = jb::FormatDll(STRING_RESTART_GAME_TEXT, gUser.mName.c_str());
		aBox->AddText(aText.c_str(), 0, 0, 0x5a, false);
		return true;
	}
	return false;
}

#endif

// 0x4101a0 (mahjongchina), 0x410530 (mahjongjapan)
MapLayer::~MapLayer()
{
	jb::gManager->StopSoundsExcept(-1);
}

// 0x410280 (mahjongchina), 0x410610 (mahjongjapan)
void MapLayer::Load()
{
	int aProgress = gUser.mProgress;
	mView = new MapView();
	Add(mView, 0, 0);
	mMenu = new jb::Button(IMAGE_MAP_MENU_BUTTON, 4, false);
	Add(mMenu, 10, edition::MAP_MENU_Y);
	mMenu->SetLabel(STRING_MAP_MAIN_MENU, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::MAP_MENU_LABEL_X,
		edition::MAP_MENU_LABEL_Y, 2, 2, 0xff);
	mContinue = new jb::Button(IMAGE_MAP_PLAY_BUTTON, 4, false);
	Add(mContinue, edition::MAP_PLAY_X, edition::MAP_PLAY_Y);
	mContinue->SetLabel(STRING_MAP_PLAY, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::MAP_PLAY_LABEL_X, 0x1a, 2, 2, 0xff);
	mCurtain = new MapCurtain(mPlay);
	Add(mCurtain, 0, 0);
	mEnd = new jb::Button(IMAGE_MAP_SHOWCASE_BUTTON, 4, false);
	Add(mEnd, edition::MAP_SHOWCASE_X, edition::MAP_SHOWCASE_Y);
	mEnd->SetLabel(STRING_MAP_SHOWCASE, FONT_BUTTON, FONT_BUTTON_MAP, 0, edition::MAP_SHOWCASE_LABEL_X, 0x1a, 2, 2,
		0xff);
	if (aProgress < edition::NUM_LEVELS)
		mEnd->SetVisible(false);
	LoadLevels();
	jb::gManager->LoadSound(SOUND_MAP_PROGRESS, false);
}

// 0x4104a0 (mahjongchina), 0x410840 (mahjongjapan)
bool MapLayer::OnCommand(int theCmd)
{
	jb::Layer* aLayer;
	if (theCmd == 0xe4)
	{
		if (!gUser.mShowQuitNote)
		{
			jb::gManager->SlideMusic(0, 0x44c);
			jb::gManager->SwitchTo(new MainMenu());
			return true;
		}
		aLayer = new QuitNote();
	}
	else
	{
		if (theCmd == 0xe6)
		{
			Continue();
			return true;
		}
		if (theCmd - 0x16c != 0)
			return false;
		aLayer = new EndLayer();
	}
	jb::gManager->Push(aLayer, true, false, true);
	return true;
}

// 0x4105e0 (mahjongchina), 0x410970 (mahjongjapan)
void MapLayer::OnShown()
{
	if (gUser.mProgress == 0)
		mView->mFadeIn = true;
#if EDITION_RESTART_BOX
	if (edition::NUM_LEVELS - 1 < gUser.mProgress)
		jb::gManager->Push(new RestartBox(this), true, true, true);
#endif
}

// 0x410220 (mahjongchina), 0x4105b0 (mahjongjapan)
void MapLayer::SetProgressed(bool theProgressed)
{
	mProgressed = theProgressed;
	mShowing = true;
	mFromPlay = true;
	if (theProgressed)
	{
		mView->StepBack();
		if (gUser.mProgress == edition::NUM_LEVELS)
			mEnd->SetVisible(false);
	}
	mCurtain->Close();
	EnableAll(false);
	jb::gManager->PlaySound(SOUND_MAP_PROGRESS);
}

// 0x410600 (mahjongchina), 0x410a00 (mahjongjapan)
void MapLayer::CurtainDone()
{
	if (mShowing)
	{
		SetOpaque(true);
		SetClip(nullptr);
		mShowing = false;
		EnableAll(true);
		StartProgress();
		return;
	}
	bool aFromPlay = mFromPlay;
	PlayLayer* aPlay = mPlay;
	int aLevel = mView->mLevel;
	if (mView->mMoving)
		aLevel = aLevel + 1;
	jb::gManager->Pop(false, 1);
	aPlay->SetClip(nullptr);
	aPlay->Begin();
	if (aFromPlay)
		jb::gManager->SetMusicLoop(4, aLevel % 2 != 0 ? 0 : edition::MUSIC_LOOP_EVEN);
}

// 0x4106a0 (mahjongchina), 0x410aa0 (mahjongjapan)
void MapLayer::Continue()
{
	int aLevel = mView->mLevel;
	if (mView->mMoving)
		aLevel = aLevel + 1;
	if (edition::NUM_LEVELS - 1 < aLevel)
		aLevel = edition::NUM_LEVELS - 1;
	std::string aPath;
	gUser.GetEscapeSave(aLevel, aPath);
	if (!jb::FileExists(aPath.c_str()))
		StartLevel(false);
	else
	{
		YesNoBox* aBox = new YesNoBox(this, 2, 1, STRING_CONTINUE, STRING_RESTART, false, 0x78, 0x78);
		jb::gManager->Push(aBox, true, true, true);
		aBox->SetTitle(STRING_SAVED_GAME_TITLE);
		aBox->AddText(STRING_SAVED_GAME_TEXT, 0, 0, 0x5a, false);
	}
}

// 0x4107c0 (mahjongchina), 0x410bc0 (mahjongjapan)
bool MapLayer::OnYes(int theId)
{
	if (theId == 1)
	{
		StartLevel(true);
		return true;
	}
#if EDITION_RESTART_BOX
	if (theId == 2)
	{
		gUser.mProgress = 0;
		mView->mLevel = 0;
		mView->mFadeIn = true;
		mEnd->SetVisible(false);
		jb::gManager->Pop(true, 1);
	}
#endif
	return true;
}

// 0x4107d0 (mahjongchina), 0x410c10 (mahjongjapan)
bool MapLayer::OnNo(int theId)
{
	if (theId == 1)
		StartLevel(false);
	return true;
}

// 0x4107e0 (mahjongchina), 0x410c30 (mahjongjapan)
void MapLayer::StartLevel(bool theResume)
{
	SetOpaque(false);
	EnableAll(false);
	int aLevel = mView->mLevel;
	if (mView->mMoving)
		aLevel = aLevel + 1;
	int aStage;
	int anIndex;
	int aCount;
	GetStage(aLevel, &aStage, &anIndex, &aCount);
	std::string aTitle = jb::FormatDll(STRING_STAGE_NAME, GT(STAGE_NAMES[aStage]));
	mPlay->StartLevel(aLevel, theResume, LEVEL_BACKGROUNDS[aLevel], aTitle.c_str(), aStage, STAGE_STATS[aStage][0],
		STAGE_STATS[aStage][1], mLevels[aLevel].mQuadPercent, mLevels[aLevel].mSeed, mLevels[aLevel].mTitle.c_str());
	mPlay->SetProgress(aCount, anIndex);
	mCurtain->Open();
	jb::gManager->PlaySound(SOUND_MAP_PROGRESS);
}

// 0x410900 (mahjongchina), 0x410d60 (mahjongjapan) (the original has no bound on the lines; TXT 0x17f has one for each
// level)
bool MapLayer::LoadLevels()
{
	jb::TextResource aText;
	if (!aText.Load(0x17f, 0, jb::Module::DLL))
		return false;
	std::string aLine;
	int aPos = 0;
	if (aText.ReadLine(aPos, aLine))
	{
		Level* aLevel = mLevels;
		do
		{
			if (!aLine.empty() && aLevel < mLevels + edition::NUM_LEVELS)
			{
				int aTab = 0;
				aLevel->mQuadPercent = (int)strtol(jb::Tokenize(aLine, 9, aTab).c_str(), nullptr, 10);
				aLevel->mSeed = (uint32_t)strtoul(jb::Tokenize(aLine, 9, aTab).c_str(), nullptr, 10);
				aLevel->mTitle = jb::Tokenize(aLine, 9, aTab);
				aLevel = aLevel + 1;
			}
		} while (aText.ReadLine(aPos, aLine));
	}
	return true;
}

// 0x410a90 (mahjongchina), 0x410ef0 (mahjongjapan)
void GetStage(int theLevel, int* theStage, int* theIndex, int* theCount)
{
	if (theLevel < 3)
	{
		*theStage = 0;
		*theIndex = theLevel;
		*theCount = 3;
		return;
	}
	if (theLevel < 7)
	{
		*theStage = 1;
		*theIndex = theLevel + -3;
		*theCount = 4;
		return;
	}
#if EDITION_UNEVEN_STAGES
	if (0x3b < theLevel)
	{
		*theStage = 0xc;
		*theIndex = 0;
		*theCount = 0;
		return;
	}
	if (0x34 < theLevel)
	{
		*theStage = 0xb;
		*theIndex = theLevel + -0x35;
		*theCount = 7;
		return;
	}
	if (0x2e < theLevel)
	{
		*theStage = 10;
		*theIndex = theLevel + -0x2f;
		*theCount = 6;
		return;
	}
#endif
	*theStage = (theLevel + -7) / 5 + 2;
	*theIndex = (theLevel + -7) % 5;
	*theCount = 5;
}

// 0x410b10 (mahjongchina), 0x410fe0 (mahjongjapan)
void MapLayer::StartProgress()
{
	if (mProgressed)
	{
		if (gUser.mProgress == edition::NUM_LEVELS)
		{
			mContinue->SetEnabled(false);
			mMenu->SetEnabled(false);
		}
		int aLevel = gUser.mProgress + -1;
		bool aNewStage = IsStageEnd(aLevel);
		int aStage;
		int anIndex;
		int aCount;
		GetStage(aLevel, &aStage, &anIndex, &aCount);
		mView->StartMove(aNewStage, GT(STAGE_NAMES[aStage]));
	}
}

// 0x410b90 (mahjongchina), 0x411060 (mahjongjapan)
bool IsStageEnd(int theLevel)
{
#if EDITION_UNEVEN_STAGES
	return theLevel == 2 || theLevel == 6 || theLevel == 0xb || theLevel == 0x10 || theLevel == 0x15 ||
		theLevel == 0x1a || theLevel == 0x1f || theLevel == 0x24 || theLevel == 0x29 || theLevel == 0x2e ||
		theLevel == 0x34 || theLevel == 0x3b;
#else
	return theLevel == 2 || theLevel == 6 || (theLevel - 7) % 5 == 4;
#endif
}

// 0x410bc0 (mahjongchina), 0x4110b0 (mahjongjapan)
int GetProgressStage()
{
	int aStage;
	int anIndex;
	int aCount;
	GetStage(gUser.mProgress, &aStage, &anIndex, &aCount);
	return aStage;
}

// 0x410bf0 (mahjongchina), 0x4110e0 (mahjongjapan)
void MapLayer::EnableAll(bool theEnabled)
{
	mView->SetEnabled(theEnabled);
	mMenu->SetEnabled(theEnabled);
	mContinue->SetEnabled(theEnabled);
	mEnd->SetEnabled(theEnabled);
}

// 0x410c30 (mahjongchina), 0x411120 (mahjongjapan)
void MapLayer::ShowEnd()
{
	mEnd->SetVisible(true);
	EnableAll(true);
}

} // namespace mahjong
