#include "MapLayer.h"

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

namespace mahjongjapan
{

// 0x51ce98: the progress at which each stage's picture shows
static const int STAGE_LEVELS[12] = {3, 7, 12, 17, 22, 27, 32, 37, 42, 47, 53, 60};
// 0x51cec8: the pictures' rects (the left and top place them; a click inside picks the stage's first level)
static const int STAGE_RECTS[12][4] = {
	{0, 426, 156, 534}, {17, 302, 144, 404}, {30, 175, 157, 278}, {139, 40, 235, 160},
	{236, 100, 376, 141}, {342, 141, 437, 264}, {357, 298, 472, 427}, {414, 427, 544, 529},
	{539, 512, 653, 586}, {570, 333, 695, 479}, {614, 198, 728, 333}, {672, 53, 779, 148}};
// 0x51cf88
static const int STAGE_IMAGES[12] = {571, 574, 573, 567, 576, 570, 572, 575, 568, 569, 566, 577};
// 0x51cfb8: the sixty levels' places on the path, then (0x51d198) the progress bar's width at each progress. One
// table, as in the exe: the map reads the place of level 60 (the bar's first widths) while the last move ends, and
// the bar's width before progress 0 (the last level's y) while the first move runs.
static const int MAP_PATH[181] = {
	14, 516, 27, 504, 37, 489, 65, 434, 71, 418,
	75, 403, 80, 385, 94, 327, 98, 309, 101, 293,
	106, 276, 109, 258, 125, 200, 131, 184, 137, 169,
	146, 154, 155, 140, 200, 105, 216, 100, 234, 97,
	252, 98, 269, 104, 314, 140, 323, 155, 331, 170,
	338, 187, 345, 204, 364, 260, 369, 278, 374, 296,
	379, 314, 384, 332, 400, 390, 406, 408, 413, 425,
	420, 444, 429, 462, 463, 511, 477, 526, 495, 536,
	515, 541, 535, 542, 594, 523, 607, 508, 618, 493,
	628, 476, 636, 458, 656, 400, 661, 382, 665, 365,
	669, 348, 673, 330, 677, 313, 690, 252, 693, 234,
	698, 216, 702, 199, 707, 181, 712, 164, 719, 146,
	20, 40, 60, 81, 96, 111, 126, 142, 154, 166,
	178, 190, 203, 215, 227, 239, 251, 266, 278, 290,
	302, 314, 329, 341, 353, 365, 377, 389, 401, 413,
	425, 437, 451, 463, 475, 487, 499, 513, 525, 537,
	549, 561, 574, 586, 598, 610, 622, 635, 645, 655,
	665, 675, 685, 696, 705, 714, 723, 732, 741, 750,
	781};
static int PointX(int theLevel) { return MAP_PATH[theLevel * 2]; }
static int PointY(int theLevel) { return MAP_PATH[theLevel * 2 + 1]; }
static int BarWidth(int theProgress) { return MAP_PATH[120 + theProgress]; }
// 0x51d310: each stage's two figures for the play layer
static const int STAGE_STATS[12][2] = {
	{-8500, -300}, {-300, 250}, {250, 538}, {538, 710}, {710, 794}, {794, 1185},
	{1185, 1333}, {1333, 1573}, {1573, 1603}, {1603, 1867}, {1867, 1912}, {1912, 1926}};
// 0x51d370: each level's background
static const int LEVEL_BACKGROUNDS[60] = {
	385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399,
	400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414,
	415, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429,
	430, 431, 432, 433, 434, 435, 436, 579, 580, 581, 582, 583, 584, 585, 578};
// 0x5250cc: the stages' names in the exe
static const uint32_t STAGE_NAMES[12] = {
	GTA(0x525160), GTA(0x525158), GTA(0x525150), GTA(0x525148), GTA(0x525140), GTA(0x525138),
	GTA(0x52512c), GTA(0x525120), GTA(0x525110), GTA(0x52510c), GTA(0x525104), GTA(0x5250fc)};

// 0x40f7d0
void MapView::Draw()
{
	bool anEnd;
	if ((!mMoving && !mBack) || (anEnd = true, gUser.mProgress != 0x3c))
		anEnd = false;
	int aShown = gUser.mProgress;
	if (mMoving || mBack)
		aShown = gUser.mProgress - 1;
	DrawImage(0, 0, IMAGE_MAP, nullptr, true, 100, 100, 0xff);
	int i = 0;
	do
	{
		if (aShown < STAGE_LEVELS[i])
			break;
		DrawImage(STAGE_RECTS[i][0], STAGE_RECTS[i][1], STAGE_IMAGES[i], nullptr, true, 100, 100, 0xff);
		i++;
	} while (i < 12);
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
	if (aShown < 0x3c)
		DrawImage(PointX(aLast), PointY(aLast), IMAGE_MAP_POINT_RING, nullptr, true, 100, 100, 0xff);
	if (mMoving && (aShown = aShown + 1, mNewStage) && i < 0xc)
		DrawImage(STAGE_RECTS[i][0], STAGE_RECTS[i][1], STAGE_IMAGES[i], nullptr, true, 100, 100, mMoveAlpha);
	jb::Rect aBar;
	aBar.left = 0;
	aBar.top = 0;
	aBar.right = BarWidth(aShown);
	aBar.bottom = 0x12;
	if (mMoving)
		aBar.right = ((aBar.right - BarWidth(aShown - 1)) * mMoveAlpha) / 0xff + BarWidth(aShown - 1);
	DrawImage(9, 0x10, IMAGE_MAP_BAR, &aBar, true, 100, 100, 0xff);
	if (0x2f7 < aBar.right)
		aBar.right = 0x2f7;
	DrawImage(aBar.right - 4, 0, IMAGE_MAP_MARKER, nullptr, true, 100, 100, 0xff);
	mFont->DrawLeft(this, 8, 0x34, mStartText.c_str(), -1, 100, 100, 0xff);
	mFont->DrawRight(this, 0x312, 0x34, mEndText.c_str(), 100, 100, 0xff);
	uint32_t aBlink = mTick % 0x28;
	if (0x14 < aBlink)
		aBlink = 0x28 - aBlink;
	int aY;
	int aX;
	if (anEnd)
	{
		aY = 0x7d;
		aX = 0x2ba;
	}
	else if (!mMoving)
	{
		aY = PointY(mLevel) - 0x15;
		aX = PointX(mLevel) - 0x15;
	}
	else
	{
		if (aShown < 1)
		{
			aX = -6;
			aY = 0x1f0;
		}
		else
		{
			aX = PointX(aShown - 1);
			aY = PointY(aShown - 1);
		}
		aY = ((PointY(aShown) - aY) * mMoveAlpha) / 0xff + -0x15 + aY;
		aX = ((PointX(aShown) - aX) * mMoveAlpha) / 0xff + -0x15 + aX;
	}
	DrawImage(aX, aY, IMAGE_MAP_GLOW, nullptr, true, 100, 100, (int)(aBlink * 200) / 0x14 + 0x37);
	if (aShown == 0 && 0 < mTextAlpha)
	{
		DrawImage(0x2a, 10, IMAGE_MAP_TEXT_SHADE, nullptr, true, 400, 400, mTextAlpha);
		mIntroFont->DrawLinesCentred(this, 0, 200, mIntroText.c_str(), 800, 0x1a, 100, 100, mTextAlpha);
	}
	if (mNewStage && 0 < mTextAlpha)
	{
		DrawImage(0x80, 0x82, IMAGE_MAP_TEXT_TOP, nullptr, true, 100, 100, mTextAlpha);
		mStageFont->DrawLinesCentred(this, 0, 0xec, mStageText.c_str(), 800, 0x26, 100, 100, mTextAlpha);
		DrawImage(0x80, 0x1a4, IMAGE_MAP_TEXT_BOTTOM, nullptr, true, 100, 100, mTextAlpha);
	}
}

// 0x40fc70
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
	if (0x3b < aProgress)
		mLevel = 0x3b;
	jb::LoadSound(SOUND_STAGE_TEXT_GONE, false);
	jb::LoadSound(SOUND_NEW_STAGE, false);
	jb::LoadSound(SOUND_MAP_MOVE, false);
	RegUpdate(true);
	RegClick(true);
	RegMove(true);
	RegHover(true);
	SetUserFlag(true);
}

// 0x40fd60
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
			if (gUser.mProgress == 0x3c)
				AddTimer(1, 1000, 1, 1);
		}
	}
	uint32_t aTick = mTick + 1;
	mTick = aTick;
	return (~aTick & 1) != 0;
}

// 0x40fec0
bool MapView::OnTimer(int theId, int theIndex)
{
	if (theId == 1)
	{
		static_cast<MapLayer*>(mLayer)->ShowEnd();
		jb::gManager->Push(new EndLayer(), true, false, true);
	}
	return true;
}

// 0x40ff40
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

// 0x40ffa0
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

// 0x410000 (the last stage's picture, reached only at the end, is not tested)
int MapView::HitTest(int theX, int theY)
{
	if (gUser.mProgress != 0)
	{
		int i = 0;
		if (-1 < gUser.mProgress)
		{
			do
			{
				if (0x3b < i)
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
		} while (i < 11);
	}
	return -1;
}

// 0x4100a0
void MapView::OnMouseMove(int theFlags, int theX, int theY)
{
	if (HitTest(theX, theY) != -1)
	{
		SetCursorKind(1, true);
		return;
	}
	SetCursorKind(0, true);
}

// 0x4100e0
void MapView::StepBack()
{
	mBack = true;
	mLevel = mLevel - 1;
}

// 0x410100
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

// 0x4111e0
void MapCurtain::Draw()
{
	if (mX != 0xffff)
		DrawImage(mX, 0, IMAGE_MAP_CURTAIN, nullptr, true, 100, 100, 0xff);
}

// 0x411210
void MapCurtain::Init()
{
	mManager->PreloadDllImage(IMAGE_MAP_CURTAIN);
	SetUseClip(false);
}

// 0x411230
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
		if (!mClosing ? -0x90 < aX : mX < 800)
		{
			mClip.mRect = {0, 0, mX + 0x2e, 600};
			mLayer->SetClip(&mClip);
			mPlayClip.mRect = {mX + 0x6c, 0, 800, 600};
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

// 0x411320
void MapCurtain::Open()
{
	mClosing = false;
	mX = 0x348;
	jb::gManager->SetVBlankWait(true);
	RegUpdate(true);
}

// 0x411350
void MapCurtain::Close()
{
	mClip.mRect = {0, 0, 0, 600};
	mLayer->SetClip(&mClip);
	mClosing = true;
	mX = -0xb8;
	jb::gManager->SetVBlankWait(true);
	RegUpdate(true);
}

// 0x4101d0
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

// 0x410330
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

// 0x410530
MapLayer::~MapLayer()
{
	jb::gManager->StopSoundsExcept(-1);
}

// 0x410610
void MapLayer::Load()
{
	int aProgress = gUser.mProgress;
	mView = new MapView();
	Add(mView, 0, 0);
	mMenu = new jb::Button(IMAGE_MAP_MENU_BUTTON, 4, false);
	Add(mMenu, 10, 0x228);
	mMenu->SetLabel(STRING_MAP_MAIN_MENU, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x8e, 0x1a, 2, 2, 0xff);
	mContinue = new jb::Button(IMAGE_MAP_PLAY_BUTTON, 4, false);
	Add(mContinue, 0x283, 0x228);
	mContinue->SetLabel(STRING_MAP_PLAY, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x94, 0x1a, 2, 2, 0xff);
	mCurtain = new MapCurtain(mPlay);
	Add(mCurtain, 0, 0);
	mEnd = new jb::Button(IMAGE_MAP_SHOWCASE_BUTTON, 4, false);
	Add(mEnd, 0x145, 0x228);
	mEnd->SetLabel(STRING_MAP_SHOWCASE, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x8e, 0x1a, 2, 2, 0xff);
	if (aProgress < 0x3c)
		mEnd->SetVisible(false);
	LoadLevels();
	jb::gManager->LoadSound(SOUND_MAP_PROGRESS, false);
}

// 0x410840
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

// 0x410970
void MapLayer::OnShown()
{
	if (gUser.mProgress == 0)
		mView->mFadeIn = true;
	if (0x3b < gUser.mProgress)
		jb::gManager->Push(new RestartBox(this), true, true, true);
}

// 0x4105b0
void MapLayer::SetProgressed(bool theProgressed)
{
	mProgressed = theProgressed;
	mShowing = true;
	mFromPlay = true;
	if (theProgressed)
	{
		mView->StepBack();
		if (gUser.mProgress == 0x3c)
			mEnd->SetVisible(false);
	}
	mCurtain->Close();
	EnableAll(false);
	jb::gManager->PlaySound(SOUND_MAP_PROGRESS);
}

// 0x410a00
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
		jb::gManager->SetMusicLoop(4, aLevel % 2 != 0 ? 0 : 10);
}

// 0x410aa0
void MapLayer::Continue()
{
	int aLevel = mView->mLevel;
	if (mView->mMoving)
		aLevel = aLevel + 1;
	if (0x3b < aLevel)
		aLevel = 0x3b;
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

// 0x410bc0
bool MapLayer::OnYes(int theId)
{
	if (theId == 1)
	{
		StartLevel(true);
		return true;
	}
	if (theId == 2)
	{
		gUser.mProgress = 0;
		mView->mLevel = 0;
		mView->mFadeIn = true;
		mEnd->SetVisible(false);
		jb::gManager->Pop(true, 1);
	}
	return true;
}

// 0x410c10
bool MapLayer::OnNo(int theId)
{
	if (theId == 1)
		StartLevel(false);
	return true;
}

// 0x410c30
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

// 0x410d60 (the original has no bound on the lines; TXT 0x17f has sixty)
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
			if (!aLine.empty() && aLevel < mLevels + 60)
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

// 0x410ef0
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
	*theStage = (theLevel + -7) / 5 + 2;
	*theIndex = (theLevel + -7) % 5;
	*theCount = 5;
}

// 0x410fe0
void MapLayer::StartProgress()
{
	if (mProgressed)
	{
		if (gUser.mProgress == 0x3c)
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

// 0x411060
bool IsStageEnd(int theLevel)
{
	return theLevel == 2 || theLevel == 6 || theLevel == 0xb || theLevel == 0x10 || theLevel == 0x15 ||
		theLevel == 0x1a || theLevel == 0x1f || theLevel == 0x24 || theLevel == 0x29 || theLevel == 0x2e ||
		theLevel == 0x34 || theLevel == 0x3b;
}

// 0x4110b0
int GetProgressStage()
{
	int aStage;
	int anIndex;
	int aCount;
	GetStage(gUser.mProgress, &aStage, &anIndex, &aCount);
	return aStage;
}

// 0x4110e0
void MapLayer::EnableAll(bool theEnabled)
{
	mView->SetEnabled(theEnabled);
	mMenu->SetEnabled(theEnabled);
	mContinue->SetEnabled(theEnabled);
	mEnd->SetEnabled(theEnabled);
}

// 0x411120
void MapLayer::ShowEnd()
{
	mEnd->SetVisible(true);
	EnableAll(true);
}

} // namespace mahjongjapan
