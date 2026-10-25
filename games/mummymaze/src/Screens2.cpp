// The screens that follow a level: LevelCompleteScreen (WinMM.exe 0x422480, drawn by 0x422b00) and HighScoreScreen
// (0x4143a0, drawn by 0x415250). See Screens.h.

#include "Screens.h"
#include "MummyApp.h"
#include "Board.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "WidgetManager.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

#include "Tables.inc"

// ---------------------------------------------------------------------------------------------------------
// LevelCompleteScreen

static HyperlinkWidget* MakeYellowLink(MummyApp* theApp, int theId, ButtonListener* theListener, const SexyString& theLabel)
{
	HyperlinkWidget* aLink = new HyperlinkWidget(theId, theListener);
	aLink->mLabel = theLabel;
	aLink->mColor = Color(0xff, 0xff, 0);
	aLink->mUnderlineSize = 1;
	aLink->SetFont(theApp->mHeaderFont);
	return aLink;
}

// 0x422480
LevelCompleteScreen::LevelCompleteScreen(MummyApp* theApp) :
	mApp(theApp)
{
	mDoFinger = true;
	bool aNextChamber = (mApp->mAdvChamber < 0xe && mApp->mGameMode == MODE_ADVENTURE) ||
		(mApp->mClassicChamber < 0xe && mApp->mGameMode == MODE_CLASSIC);
	mNextLink = MakeYellowLink(mApp, 0, this, aNextChamber ? GT(0x00494c24) : GT(0x00494c08));
	mMapLink = MakeYellowLink(mApp, 1, this, GT(0x00494bf4));
	mApp->SetMusicOrder(0x28, false);
	mApp->SetMusicOrder(0x28, false);
	if (mApp->mBoard == NULL)
		mSeconds = 0;
	else
		mSeconds = (int) (mApp->mElapsedBase / 100 + (GetTickCount() - mApp->mBoard->mStartTick) / 1000);
	mApp->mTotalTime = (double) mSeconds + mApp->mTotalTime;
	mBase = 0;
	mBonus = 0;
	if (mApp->mGameMode == MODE_CLASSIC)
	{
		mBase = (mApp->mClassicPyramid + 2) * 0x32 + mApp->mClassicChamber * 5;
		int aLimit = mApp->mClassicPyramid * 0x14 + 0x32;
		double aRatio = 0.0;
		if (mSeconds < aLimit)
			aRatio = (double) (aLimit - mSeconds) / (double) aLimit;
		mBonus = (int) ((double) mBase * aRatio * 0.1) * 10;
		if (mApp->mClassicChamber == 0xe)
			mBonus += (mApp->mClassicPyramid + 1) * 500;
		mApp->mScore += mBonus + mBase;
	}
	mVariant = CrtRand() % 3;
	mFrame = 0;
	mApp->RemoveBoard();
}

LevelCompleteScreen::~LevelCompleteScreen()
{
	delete mNextLink;
	delete mMapLink;
}

void LevelCompleteScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNextLink);
	theWidgetManager->AddWidget(mMapLink);
}

void LevelCompleteScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNextLink);
	theWidgetManager->RemoveWidget(mMapLink);
}

// 0x424f00
void LevelCompleteScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mNextLink->mFont->StringWidth(mNextLink->mLabel);
	MummyApp* anApp = mApp;
	if (anApp->mGameMode == MODE_CLASSIC)
	{
		mNextLink->Resize((mWidth - aWidth) / 2 + mX, mHeight - 0x37, aWidth, 0x1d);
		mMapLink->SetVisible(false);
		return;
	}
	if (anApp->mGameMode != MODE_ADVENTURE)
		return;
	mMapLink->Resize((mWidth - aWidth) / 2 + mX, mHeight - 0x5f, aWidth, 0x1d);
	if (anApp->mAdvChamber > 0xd)
		return;
	mNextLink->Resize((mWidth - aWidth) / 2 + mX, mHeight - 0x37, aWidth, 0x1d);
}

// 0x422a30
void LevelCompleteScreen::Update()
{
	Widget::Update();
	if (mVariant == 1)
	{
		mFrame = (mUpdateCnt / 7) % 0x40;
		if (mFrame > 0x1f)
			mFrame = 0x3f - mFrame;
		if (mUpdateCnt % 7 == 0)
			MarkDirty();
	}
	else
	{
		if (mVariant == 2)
		{
			mFrame = (mUpdateCnt / 9) % 0x18;
			if (mFrame > 0xb)
				mFrame = 0x17 - mFrame;
		}
		else
			mFrame = (mUpdateCnt / 9) % 5;
		if (mUpdateCnt % 9 == 0)
			MarkDirty();
	}
}

static void DrawCentered(Graphics* g, const SexyString& theText, int theWidth, int theY)
{
	g->DrawString(theText, (theWidth - g->GetFont()->StringWidth(theText)) / 2, theY);
}

// 0x422b00 (approximately: the layout of the lines is from the decompilation, the pictures' places are estimated)
void LevelCompleteScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], -0x96, 0);
	{
		Graphics gc(*g);
		gc.ClipRect(0, 0, 0x1e7, 0x1dd);
		gc.DrawImage(mApp->mImages[IMAGE_NEXTLEVEL], -5, 5);
	}
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0x66, 0x5c, 0x38));
	bool aTreasure = (mApp->mGameMode == MODE_CLASSIC && mApp->mClassicChamber == 0xe) ||
		(mApp->mGameMode == MODE_ADVENTURE && mApp->mAdvChamber == 0xe);
	SexyString aTitle = aTreasure ? GT(0x00495090) : GT(0x00495074);
	DrawCentered(g, aTitle, mWidth, 0x6e);
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0x2e, 0x14, 0));
	char aBuf[128];
	int aMinutes = mSeconds / 60;
	int aSecs = mSeconds % 60;
	sprintf(aBuf, aSecs < 10 ? "%d:0%d" : "%d:%d", aMinutes, aSecs);
	DrawCentered(g, GT(0x00494fec) + SexyString(" ") + aBuf, mWidth, 0xb4);
	if (mApp->mGameMode == MODE_CLASSIC)
	{
		sprintf(aBuf, GTC(0x00494fd8), mBase);
		DrawCentered(g, aBuf, mWidth, 0xdc);
		sprintf(aBuf, GTC(0x00494fb4), mBonus);
		DrawCentered(g, aBuf, mWidth, 0x104);
		int aRank = 0;
		while (aRank < 22 && kRankScore[aRank] < mApp->mScore)
			aRank++;
		g->SetFont(mApp->mHeaderFont);
		g->SetColor(Color(0x66, 0x5c, 0x38));
		DrawCentered(g, GT(0x00494f90), mWidth, 0x138);
		g->SetColor(Color(0x46, 0x20, 0));
		DrawCentered(g, GT(kRankText[aRank]), mWidth, 0x158);
		g->SetFont(mApp->mFont1);
		if (aRank < 22)
		{
			sprintf(aBuf, GTC(0x00494f78), kRankScore[aRank]);
			DrawCentered(g, aBuf, mWidth, 0x178);
		}
	}
	else
	{
		int aTotal = (int) mApp->mTotalTime;
		int aHours = aTotal / 3600;
		int aMins = (aTotal / 60) % 60;
		int aSeconds = aTotal % 60;
		char aLine[200];
		sprintf(aLine, GTC(0x00494f64), aHours);
		SexyString aText = aLine;
		sprintf(aLine, GTC(0x00494f58), aMins);
		aText += aLine;
		DrawCentered(g, GT(0x00494f48), mWidth, 0xdc);
		DrawCentered(g, aText, mWidth, 0xf4);
		sprintf(aLine, GTC(0x00494f38), aSeconds);
		DrawCentered(g, aLine, mWidth, 0x10c);
	}
}

// 0x424fa0
void LevelCompleteScreen::ButtonDepress(int theId)
{
	if (theId == 0)
	{
		if (mApp->mGameMode == MODE_CLASSIC)
		{
			if (mApp->mClassicChamber == 0 && mApp->mClassicPyramid == 0)
				mApp->mTimesPlayed++;
			if (mApp->mClassicChamber == 0xe)
			{
				mApp->mGameMode = MODE_CLASSIC_DONE;
				mApp->RemoveLevelComplete();
				return;
			}
			mApp->mClassicChamber++;
			mApp->RemoveLevelComplete();
			return;
		}
		if (mApp->mGameMode == MODE_ADVENTURE)
		{
			double aTotal = mApp->mTotalTime;
			mApp->LoadAdventure(mApp->mCurrentSlot, true);
			mApp->mTotalTime = aTotal;
		}
	}
	else if (theId == 1)
	{
		double aTotal = mApp->mTotalTime;
		mApp->LoadAdventure(mApp->mCurrentSlot, false);
		mApp->mTotalTime = aTotal;
		mApp->RemoveLevelComplete();
		return;
	}
	mApp->RemoveLevelComplete();
}

void LevelCompleteScreen::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}

// ---------------------------------------------------------------------------------------------------------
// HighScoreScreen

// 0x4143a0
HighScoreScreen::HighScoreScreen(MummyApp* theApp) :
	mApp(theApp),
	mRank(-1),
	mFade(0.0)
{
	mDoFinger = true;
	mMenuLink = new HyperlinkWidget(0, this);
	mMenuLink->mLabel = GT(0x0048d224);
	mMenuLink->mColor = Color(0xff, 0xff, 0);
	mMenuLink->mUnderlineSize = 1;
	mMenuLink->SetFont(mApp->mHeaderFont);
	for (int i = 0; i < 5; i++)
		mScores[i] = 0;
	Load();
}

HighScoreScreen::~HighScoreScreen()
{
	delete mMenuLink;
}

void HighScoreScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuLink);
}

void HighScoreScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuLink);
}

void HighScoreScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mMenuLink->mFont->StringWidth(mMenuLink->mLabel);
	mMenuLink->Resize((mWidth - aWidth) / 2 + mX, mHeight - 0x37, aWidth, 0x1d);
}

// the list: five records of [u16 length][name][i32 score][i32 check], the check being score ^ 0x42beef with every
// character times 0x11 xored in (0x4148e0 writes it, 0x414850.. reads it)
static const char* const kDefaultNames[5] = {"Jason", "Brian", "Kimberly", "Jason", "Brian"};
static const int kDefaultScores[5] = {5000, 4000, 3000, 2000, 1000};

void HighScoreScreen::Load()
{
	for (int i = 0; i < 5; i++)
	{
		mNames[i] = kDefaultNames[i];
		mScores[i] = kDefaultScores[i];
	}
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/hiscores.dat", &aData))
		return;
	size_t aPos = 0;
	std::string aNames[5];
	int aScores[5];
	for (int i = 0; i < 5; i++)
	{
		if (aPos + 2 > aData.size())
			return;
		unsigned short aLen;
		memcpy(&aLen, aData.data() + aPos, 2);
		aPos += 2;
		if (aPos + aLen + 8 > aData.size())
			return;
		aNames[i] = aData.substr(aPos, aLen);
		aPos += aLen;
		int aScore, aCheck;
		memcpy(&aScore, aData.data() + aPos, 4);
		memcpy(&aCheck, aData.data() + aPos + 4, 4);
		aPos += 8;
		int aExpect = aScore ^ 0x42beef;
		for (size_t j = 0; j < aNames[i].size(); j++)
			aExpect ^= (signed char) aNames[i][j] * 0x11;
		aScores[i] = aScore;
		if (aExpect != aCheck)
			return;
	}
	for (int i = 0; i < 5; i++)
	{
		mNames[i] = aNames[i];
		mScores[i] = aScores[i];
	}
}

void HighScoreScreen::Save()
{
	std::string aData;
	for (int i = 0; i < 5; i++)
	{
		unsigned short aLen = (unsigned short) mNames[i].size();
		aData.append((const char*) &aLen, 2);
		aData.append(mNames[i]);
		int aScore = mScores[i];
		aData.append((const char*) &aScore, 4);
		int aCheck = aScore ^ 0x42beef;
		for (size_t j = 0; j < mNames[i].size(); j++)
			aCheck ^= (signed char) mNames[i][j] * 0x11;
		aData.append((const char*) &aCheck, 4);
	}
	Platform::WriteUserFileAtomic("hiscores.dat", aData);
}

int HighScoreScreen::RankOf(int theScore)
{
	for (int i = 0; i < 5; i++)
		if (theScore > mScores[i])
			return i;
	return -1;
}

void HighScoreScreen::AddScore(const std::string& theName)
{
	int aRank = RankOf(mApp->mScore);
	if (aRank < 0)
		return;
	for (int i = 4; i > aRank; i--)
	{
		mNames[i] = mNames[i - 1];
		mScores[i] = mScores[i - 1];
	}
	mNames[aRank] = theName;
	mScores[aRank] = mApp->mScore;
	mRank = aRank;
	Save();
}

void HighScoreScreen::Update()
{
	Widget::Update();
	if (mFade < 1.0)
	{
		mFade += 0.02;
		if (mFade > 1.0)
			mFade = 1.0;
		MarkDirty();
	}
}

// 0x415250
void HighScoreScreen::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, (int) (mFade * 255.0)));
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], -0x96, 0);
	{
		Graphics gc(*g);
		gc.ClipRect(0, 0, 0x1e7, 0x1dd);
		gc.DrawImage(mApp->mImages[IMAGE_NEXTLEVEL], -5, 5);
	}
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0x66, 0x5c, 0x38));
	DrawCentered(g, GT(0x004921b8), mWidth, 0x5a);
	char aBuf[64];
	sprintf(aBuf, GTC(0x004921ac), mApp->mScore);
	g->SetColor(Color(0x46, 0x20, 0));
	DrawCentered(g, aBuf, mWidth, 0x78);
	g->SetColor(Color(0x66, 0x5c, 0x38));
	DrawCentered(g, GT(0x00492194), mWidth, 0x96);
	int aRank = 0;
	while (aRank < 22 && kRankScore[aRank] < mApp->mScore)
		aRank++;
	g->SetColor(Color(0xff, 0xff, 0, (int) (mFade * 255.0)));
	DrawCentered(g, GT(kRankText[aRank]), mWidth, 0x8f + 0x20);
	g->SetColorizeImages(false);
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0x76, 0x27, 0));
	DrawCentered(g, GT(0x00491f84), mWidth, 0xd1);
	int aY = 0xf7;
	for (int i = 0; i < 5; i++)
	{
		g->SetColor(Color(0, 0, 0));
		g->DrawString(mNames[i], 0x50, aY);
		g->SetColor(Color(0x46, 0x20, 0));
		sprintf(aBuf, "%d", mScores[i]);
		SexyString aScore = aBuf;
		g->DrawString(aScore, (mWidth - g->GetFont()->StringWidth(aScore)) - 0x50, aY);
		aY += 0x1b;
	}
}

void HighScoreScreen::ButtonDepress(int theId)
{
	(void) theId;
	mApp->ShowMainMenu(2, true);
}

void HighScoreScreen::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}
