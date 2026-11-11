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

namespace mahjongchina
{

// 0x51be48: the progress at which each stage's picture shows
static const int STAGE_LEVELS[11] = {3, 7, 12, 17, 22, 27, 32, 37, 42, 47, 52};
// 0x51be78: the pictures' rects (the left and top place them; a click inside picks the stage's first level)
static const int STAGE_RECTS[11][4] = {
	{42, 83, 152, 165}, {120, 202, 213, 302}, {190, 338, 272, 450}, {265, 489, 352, 571}, {313, 349, 446, 447}, {352, 191, 488, 294}, {428, 49, 560, 133}, {500, 269, 627, 341}, {586, 450, 659, 562}, {662, 294, 747, 402}, {651, 61, 788, 205}};
// 0x51bf28
static const int STAGE_IMAGES[11] = {244, 240, 235, 238, 236, 242, 243, 241, 245, 237, 239};
// 0x51bf58: the fifty-two levels' places on the path, then (0x51c0f8) the progress bar's width at each progress. One
// table, as in the exe: the map reads the place of level 52 (the bar's first widths) while the last move ends, and
// the bar's width before progress 0 (the last level's y) while the first move runs.
static const int MAP_PATH[157] = {
	72, 69, 94, 76, 114, 88, 148, 125, 161, 145,
	173, 167, 184, 191, 208, 253, 214, 275, 219, 297,
	223, 319, 227, 342, 237, 451, 239, 468, 243, 486,
	250, 503, 262, 517, 350, 522, 365, 507, 374, 486,
	380, 466, 383, 447, 391, 368, 393, 349, 395, 329,
	397, 311, 399, 293, 419, 186, 424, 167, 432, 150,
	442, 134, 455, 120, 547, 137, 555, 166, 560, 197,
	562, 227, 562, 257, 561, 349, 562, 377, 566, 406,
	572, 434, 581, 461, 660, 490, 679, 473, 690, 452,
	699, 429, 706, 405, 724, 319, 729, 291, 735, 263,
	740, 234, 746, 204, 22, 44, 66, 87, 103, 120,
	136, 153, 166, 179, 192, 205, 218, 231, 244, 257,
	270, 283, 296, 309, 322, 335, 348, 361, 374, 387,
	400, 413, 426, 439, 452, 465, 479, 492, 505, 518,
	531, 544, 557, 570, 583, 596, 609, 622, 635, 648,
	661, 674, 687, 700, 713, 726, 761};
static int PointX(int theLevel) { return MAP_PATH[theLevel * 2]; }
static int PointY(int theLevel) { return MAP_PATH[theLevel * 2 + 1]; }
static int BarWidth(int theProgress) { return MAP_PATH[104 + theProgress]; }
// 0x51c228: each stage's two figures for the play layer
static const int STAGE_STATS[11][2] = {
	{-2000, -1500}, {-1500, -1045}, {-1045, -221}, {-221, -207}, {-206, 220}, {589, 617}, {618, 907}, {960, 1279}, {1279, 1368}, {1368, 1644}, {1644, 1912}};
// 0x51c280: each level's layout
static const int LEVEL_BACKGROUNDS[52] = {
	385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397,
	398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410,
	411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423,
	424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436};
// 0x5240d0: the stages' names in the exe
static const uint32_t STAGE_NAMES[11] = {
	GTA(0x524140), GTA(0x524138), GTA(0x524130), GTA(0x52412c), GTA(0x524128), GTA(0x524124), GTA(0x52411c), GTA(0x524114), GTA(0x52410c), GTA(0x524104), GTA(0x5240fc)};

// 0x40f7d0
void MapView::Draw()
{
	bool anEnd;
	if ((!mMoving && !mBack) || (anEnd = true, gUser.mProgress != 0x34))
		anEnd = false;
	int aShown = gUser.mProgress;
	if (mMoving || mBack)
		aShown = gUser.mProgress - 1;
	DrawImage(0, 0, IMAGE_MAP, nullptr, true, 100, 100, 0xff);
	mFont->DrawLeft(this, 8, 0x3c, mStartText.c_str(), -1, 100, 100, 0xff);
	mFont->DrawRight(this, 0x312, 0x3c, mEndText.c_str(), 100, 100, 0xff);
	int i = 0;
	do
	{
		if (aShown < STAGE_LEVELS[i])
			break;
		DrawImage(STAGE_RECTS[i][0], STAGE_RECTS[i][1], STAGE_IMAGES[i], nullptr, true, 100, 100, 0xff);
		i++;
	} while (i < 11);
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
	if (aShown < 0x34)
		DrawImage(PointX(aLast) - 7, PointY(aLast) - 7, IMAGE_MAP_POINT_RING, nullptr, true, 100, 100, 0xff);
	if (mMoving && (aShown = aShown + 1, mNewStage) && i < 0xb)
		DrawImage(STAGE_RECTS[i][0], STAGE_RECTS[i][1], STAGE_IMAGES[i], nullptr, true, 100, 100, mMoveAlpha);
	jb::Rect aBar;
	aBar.left = 0;
	aBar.top = 0;
	aBar.right = BarWidth(aShown);
	aBar.bottom = 0x18;
	if (mMoving)
		aBar.right = ((aBar.right - BarWidth(aShown - 1)) * mMoveAlpha) / 0xff + BarWidth(aShown - 1);
	DrawImage(0x13, 0x17, IMAGE_MAP_BAR, &aBar, true, 100, 100, 0xff);
	if (0x2e3 < aBar.right)
		aBar.right = 0x2e3;
	DrawImage(aBar.right + 6, 8, IMAGE_MAP_MARKER, nullptr, true, 100, 100, 0xff);
	uint32_t aBlink = mTick % 0x28;
	if (0x14 < aBlink)
		aBlink = 0x28 - aBlink;
	int aY;
	int aX;
	if (anEnd)
	{
		aY = 0xb0;
		aX = 0x2ce;
	}
	else if (!mMoving)
	{
		aY = PointY(mLevel) - 0x1c;
		aX = PointX(mLevel) - 0x1c;
	}
	else
	{
		if (aShown < 1)
		{
			aX = 0x34;
			aY = 0x31;
		}
		else
		{
			aX = PointX(aShown - 1);
			aY = PointY(aShown - 1);
		}
		aY = ((PointY(aShown) - aY) * mMoveAlpha) / 0xff + -0x1c + aY;
		aX = ((PointX(aShown) - aX) * mMoveAlpha) / 0xff + -0x1c + aX;
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
	if (0x33 < aProgress)
		mLevel = 0x33;
	jb::LoadSound(SOUND_STAGE_TEXT_GONE, false);
	jb::LoadSound(SOUND_MAP_MOVE, false);
	jb::LoadSound(SOUND_NEW_STAGE, false);
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
			if (gUser.mProgress == 0x34)
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
				if (0x33 < i)
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
		} while (i < 10);
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
	Add(mMenu, 10, 0x221);
	mMenu->SetLabel(STRING_MAP_MAIN_MENU, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x7e, 0x18, 2, 2, 0xff);
	mContinue = new jb::Button(IMAGE_MAP_PLAY_BUTTON, 4, false);
	Add(mContinue, 0x299, 0x21f);
	mContinue->SetLabel(STRING_MAP_PLAY, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x80, 0x1a, 2, 2, 0xff);
	mCurtain = new MapCurtain(mPlay);
	Add(mCurtain, 0, 0);
	mEnd = new jb::Button(IMAGE_MAP_SHOWCASE_BUTTON, 4, false);
	Add(mEnd, 0x158, 0x21f);
	mEnd->SetLabel(STRING_MAP_SHOWCASE, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x78, 0x1a, 2, 2, 0xff);
	if (aProgress < 0x34)
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
		if (gUser.mProgress == 0x34)
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
		jb::gManager->SetMusicLoop(4, aLevel % 2 != 0 ? 0 : 0xd);
}

// 0x410aa0
void MapLayer::Continue()
{
	int aLevel = mView->mLevel;
	if (mView->mMoving)
		aLevel = aLevel + 1;
	if (0x33 < aLevel)
		aLevel = 0x33;
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
	// China asks only whether to resume (Japan also asked whether to start the escape again)
	StartLevel(true);
	return true;
}

// 0x410c10
bool MapLayer::OnNo(int theId)
{
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

// 0x410d60 (the original has no bound on the lines; TXT 0x17f has fifty-two)
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
			if (!aLine.empty() && aLevel < mLevels + 52)
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
	*theStage = (theLevel + -7) / 5 + 2;
	*theIndex = (theLevel + -7) % 5;
	*theCount = 5;
}

// 0x410fe0
void MapLayer::StartProgress()
{
	if (mProgressed)
	{
		if (gUser.mProgress == 0x34)
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
	return theLevel == 2 || theLevel == 6 || (theLevel - 7) % 5 == 4;
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

} // namespace mahjongchina
