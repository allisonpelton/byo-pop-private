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

#include "GameLevels.h"
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
		mSeconds = (int) (mApp->mElapsedBase / 100 + (mApp->GameTick() - mApp->mBoard->mStartTick) / 1000);
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
		// 0x4347a0, 0x434a40: a byte and the seconds go to the list's data
		mApp->mScoreBuffer.WriteByte(0);
		mApp->mScoreBuffer.WriteLong(mSeconds);
	}
	mVariant = CrtRand() % 3;
	mFrame = 0;
	mApp->RemoveBoard();
}

// 0x4229b0
LevelCompleteScreen::~LevelCompleteScreen()
{
	delete mNextLink;
	delete mMapLink;
}

// 0x424ea0
void LevelCompleteScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNextLink);
	theWidgetManager->AddWidget(mMapLink);
}

// 0x424ed0
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
	// the next chamber's link is the upper one; the link back to the map is below it and is not placed after the last chamber
	mNextLink->Resize((mWidth - aWidth) / 2 + mX, mHeight - 0x5f, aWidth, 0x1d);
	if (anApp->mAdvChamber > 0xd)
		return;
	mMapLink->Resize((mWidth - aWidth) / 2 + mX, mHeight - 0x37, aWidth, 0x1d);
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

void DrawNinePatch(Graphics* g, Image* theImage, int theW, int theH);

static void DrawCentered(Graphics* g, const SexyString& theText, int theWidth, int theY)
{
	g->DrawString(theText, (theWidth - g->GetFont()->StringWidth(theText)) / 2, theY);
}

// 0x422b00: the picture of the explorer, the lines of the result, and the rank (the adventure shows the total time)
void LevelCompleteScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], -0x96, 0);
	{
		Graphics gc(*g);
		gc.ClipRect(0, 0, 0x1e7, 0x1dd);
		gc.DrawImage(mApp->mImages[IMAGE_NEXTLEVEL], -5, 5);
	}
	{
		// the little film: 60 wide frames of one of three strips
		Graphics gc(*g);
		gc.ClipRect((mWidth - 0x3c) / 2, 0x55, 0x3c, 0x3c);
		if (mVariant == 1)
			gc.DrawImage(mApp->mImages[IMAGE_SEARCH6], (mWidth - 0x3c) / 2 + mFrame * -0x3c, 0x55);
		else if (mVariant == 2)
			gc.DrawImage(mApp->mImages[IMAGE_READMAP6], (mWidth - 0x3c) / 2 + (mFrame + 0x1c) * -0x3c, 0x55);
		else
			gc.DrawImage(mApp->mImages[IMAGE_EXPLORER6], (mWidth - 0x3c) / 2 + mFrame * -0x3c, -0x23);
	}
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0xff, 0xff, 0));
	int aMode = mApp->mGameMode;
	if ((aMode == MODE_CLASSIC && mApp->mClassicChamber == 0xe) || (aMode == MODE_ADVENTURE && mApp->mAdvChamber == 0xe))
	{
		int aPyramid = (aMode == MODE_ADVENTURE) ? mApp->mAdvPyramid : mApp->mClassicPyramid;
		DrawCentered(g, GT(0x00495090), mWidth, 0x28);
		DrawCentered(g, GT(kTreasureText[aPyramid]) + SexyString("!"), mWidth, 0x46);
	}
	else
		DrawCentered(g, GT(0x00495074), mWidth, 0x28);

	char aBuf[128];
	int aMinutes = mSeconds / 60;
	int aSecs = mSeconds % 60;
	// the lines of a result: the whole text in the light colour, then its label again over it in the dark one
	if (aMinutes == 1)
		sprintf(aBuf, aSecs == 1 ? GTC(0x00495054) : GTC(0x00495034), aMinutes, aSecs);
	else
		sprintf(aBuf, aSecs == 1 ? GTC(0x00495014) : GTC(0x00494ff4), aMinutes, aSecs);
	int aX = (mWidth - g->GetFont()->StringWidth(aBuf)) / 2;
	g->SetColor(Color(0xff, 0x7c, 0));
	g->DrawString(aBuf, aX, 0xb4);
	g->SetColor(Color(0xd6, 0x55, 0));
	g->DrawString(GT(0x00494fec), aX, 0xb4);
	if (aMode == MODE_CLASSIC)
	{
		sprintf(aBuf, GTC(0x00494fd8), mBase);
		aX = (mWidth - g->GetFont()->StringWidth(aBuf)) / 2;
		g->SetColor(Color(0xff, 0x7c, 0));
		g->DrawString(aBuf, aX, 0xdc);
		g->SetColor(Color(0xd6, 0x55, 0));
		g->DrawString(GT(0x00494fc8), aX, 0xdc);
		sprintf(aBuf, GTC(0x00494fb4), mBonus);
		aX = (mWidth - g->GetFont()->StringWidth(aBuf)) / 2;
		g->SetColor(Color(0xff, 0x7c, 0));
		g->DrawString(aBuf, aX, 0x104);
		g->SetColor(Color(0xd6, 0x55, 0));
		g->DrawString(GT(0x00494fa4), aX, 0x104);
		g->SetColor(Color(0xbc, 0xa5, 0x25));
		DrawCentered(g, GT(0x00494f90), mWidth, 0x140);
		int aRank = 0;
		while (aRank < 22 && kRankScore[aRank] < mApp->mScore)
			aRank++;
		g->SetColor(Color(0xff, 0xff, 0));
		DrawCentered(g, GT(kRankText[aRank]), mWidth, 0x15e);
		if (aRank < 22)
		{
			sprintf(aBuf, GTC(0x00494f78), kRankScore[aRank]);
			g->SetColor(Color(0xc2, 0x2c, 0x14));
			DrawCentered(g, aBuf, mWidth, 0x18b);
		}
	}
	else
	{
		// the adventure: the time of the whole game so far (the lines below it are not ported yet)
		int aTotal = (int) mApp->mTotalTime;
		int aHours = aTotal / 3600;
		int aMins = (aTotal / 60) % 60;
		int aSeconds = aTotal % 60;
		char aLine[200];
		sprintf(aLine, GTC(0x00494f64), aHours);
		SexyString aText = aLine;
		sprintf(aLine, GTC(0x00494f58), aMins);
		aText += aLine;
		aX = (mWidth - g->GetFont()->StringWidth(aText)) / 2;
		g->SetColor(Color(0xff, 0x7c, 0));
		g->DrawString(aText, aX, 0xdc);
		g->SetColor(Color(0xd6, 0x55, 0));
		g->DrawString(GT(0x00494f48), aX, 0xdc);
		sprintf(aLine, GTC(0x00494f38), aSeconds);
		g->SetColor(Color(0xff, 0x7c, 0));
		DrawCentered(g, aLine, mWidth, 0x104);
	}
}

// 0x424fa0
void LevelCompleteScreen::ButtonDepress(int theId)
{
	if (theId == 0)
	{
		if (mApp->mGameMode == MODE_CLASSIC)
		{
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
	mCounter(0),
	mNameAsked(false),
	mFade(0.0)
{
	mDoFinger = true;
	mMenuLink = new HyperlinkWidget(1, this);
	mMenuLink->mLabel = GT(0x0048d224);
	mMenuLink->mColor = Color(0xf8, 0x7a, 2);
	mMenuLink->mOverColor = Color(0xff, 0xff, 0xff);
	mMenuLink->mUnderlineSize = 1;
	mMenuLink->SetFont(mApp->mHeaderFont);
	for (int i = 0; i < 5; i++)
		mScores[i] = 0;
	Load();
}

// 0x414850: the list is written whenever the screen goes (0x4148e0; the victory screen's 0x402190 does the same with 0x4023e0).
// The original shows a message when the file cannot be made; the port leaves that out.
HighScoreScreen::~HighScoreScreen()
{
	delete mMenuLink;
	Save();
}

// 0x414d80 (the original adds a second link, to the Internet score list, which is out)
void HighScoreScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuLink);
}

// 0x414db0
void HighScoreScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuLink);
}

// 0x415ab0, 0x402d40 (the victory screen's own: the same, and it places the Internet link above the menu link
// when the game is not the adventure; that link is out)
void HighScoreScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mMenuLink->mFont->StringWidth(mMenuLink->mLabel);
	(void) aWidth;
	mMenuLink->Resize(mWidth / 2 - 0xb9 + mX, 0x1b1, 0x172, 0x1e);
}

// the list: five records of [u16 length][name][i32 score][i32 check], the check being score ^ 0x42beef with every
// character times 0x11 xored in (0x4148e0 writes it, 0x402220 reads it)
// the names of the list that a new player finds (texts of the exe)
static const unsigned kDefaultNames[5] = {0x0048d218, 0x0048d210, 0x0048d208, 0x0048d200, 0x0048d1f8};
static const int kDefaultScores[5] = {1000, 900, 800, 700, 500};

// 0x402220
void HighScoreScreen::Load()
{
	for (int i = 0; i < 5; i++)
	{
		mNames[i] = GT(kDefaultNames[i]);
		mScores[i] = kDefaultScores[i];
	}
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/hiscores.dat", &aData))
		return;
	size_t aPos = 0;
	for (int i = 0; i < 5; i++)
	{
		// (a short file leaves the rest as it was; the original's fread calls would fail and use what is on its stack)
		if (aPos + 2 > aData.size())
			return;
		unsigned short aLen;
		memcpy(&aLen, aData.data() + aPos, 2);
		aPos += 2;
		if (aPos + aLen + 8 > aData.size())
			return;
		const char* aName = aData.data() + aPos;
		aPos += aLen;
		int aScore, aCheck;
		memcpy(&aScore, aData.data() + aPos, 4);
		memcpy(&aCheck, aData.data() + aPos + 4, 4);
		aPos += 8;
		int aExpect = aScore ^ 0x42beef;
		for (int j = 0; j < aLen; j++)
			aExpect ^= (signed char) aName[j] * 0x11;
		// the name is what comes before the first NUL (the original's strlen); an entry that fails the check keeps its name and
		// its score becomes the stored one modulo 256 (signed)
		const void* aEnd = memchr(aName, 0, aLen);
		mNames[i] = std::string(aName, aEnd != NULL ? (const char*) aEnd - aName : aLen);
		mScores[i] = aExpect == aCheck ? aScore : aScore % 256;
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

// 0x414d50
int HighScoreScreen::RankOf(int theScore)
{
	for (int i = 0; i < 5; i++)
		if (theScore > mScores[i])
			return i;
	return -1;
}

// ---------------------------------------------------------------------------------------------------------
// GameOverScroll

// 0x414290
GameOverScroll::GameOverScroll(MummyApp* theApp) :
	ScrollScreen(theApp),
	mHold(0)
{
}

GameOverScroll::~GameOverScroll()
{
}

// 0x414330: it rests 50 updates once it has come down, goes up, and leaves
void GameOverScroll::Update()
{
	ScrollScreen::Update();
	if (mOffset == 0)
	{
		if (mHold < 0x32)
			mHold++;
		else
			mScrollState = 5;
	}
	if (mScrollState == 6)
		mApp->RemoveGameOverScroll();
}

// 0x4142f0
void GameOverScroll::Draw(Graphics* g)
{
	ScrollScreen::Draw(g);
	g->DrawImage(mApp->mImages[IMAGE_GAMEOVERTEXT], 100, mOffset - 0x71 + mY);
}

// 0x415b40: the name for the score that the dialog asked for: the score takes its place and the others move down
void HighScoreScreen::SetName(const SexyString& theName)
{
	if (theName.empty())
		return;
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
}

// 0x402850: the ordinal's ending (its four texts are st, nd, rd and th)
static SexyString OrdinalSuffix(int theNumber)
{
	if ((theNumber % 100) / 10 != 1)
	{
		switch (theNumber % 10)
		{
		case 1:
			return GT(0x0048d33c);
		case 2:
			return GT(0x0048d340);
		case 3:
			return GT(0x0048d344);
		}
	}
	return GT(0x0048d348);
}

// 0x414ac0 (and 0x4025c0 of the victory screen): a score that makes the list asks for the name
void HighScoreScreen::AskName()
{
	int aRank = RankOf(mApp->mScore) + 1;
	if (aRank > 0)
	{
		char aBuf[16];
		sprintf(aBuf, "%d", aRank);
		mApp->ShowRecordScore(GT(0x0048d32c) + SexyString(aBuf) + OrdinalSuffix(aRank) + GT(0x0048d2c8));
	}
}

// 0x415a00
void HighScoreScreen::Update()
{
	Widget::Update();
	if (mCounter > 0 && --mCounter == 0)
	{
		// (the Internet link of the original is not here)
	}
	if (!mNameAsked && mFade >= 175.0)
	{
		AskName();
		mNameAsked = true;
	}
	if (mFade < 255.0)
	{
		mFade += 1.0;
		MarkDirty();
	}
}

// 0x415250: the score, its rank and the list, fading in; the list stands in a frame of the dialog's picture (0x414de0)
void HighScoreScreen::Draw(Graphics* g)
{
	int anAlpha = (int) mFade;
	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], -0x96, 0);
	{
		Graphics gc(*g);
		gc.ClipRect(0, 0, 0x1e7, 0x1dd);
		gc.DrawImage(mApp->mImages[IMAGE_NEXTLEVEL], -5, 5);
	}
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color(0xff, 0xbf, 0x22, anAlpha));
	DrawCentered(g, GT(0x004921b8), mWidth, 0x27);
	g->SetColor(Color(0xff, 0xff, 0, anAlpha));
	char aBuf[64];
	sprintf(aBuf, GTC(0x004921ac), mApp->mScore);
	DrawCentered(g, aBuf, mWidth, 0x49);
	g->SetColor(Color(0xff, 0xbf, 0x22, anAlpha));
	DrawCentered(g, GT(0x00492194), mWidth, 0x6d);
	int aRank = 0;
	while (aRank < 22 && kRankScore[aRank] < mApp->mScore)
		aRank++;
	g->SetColor(Color(0xff, 0xff, 0, anAlpha));
	DrawCentered(g, GT(kRankText[aRank]), mWidth, 0x8f);
	g->SetColorizeImages(false);
	g->Translate((mWidth - 0x19f) / 2, 0xa0);
	DrawNinePatch(g, mApp->mImages[IMAGE_DIALOG], 0x19f, 0xf0);
	g->Translate(-((mWidth - 0x19f) / 2), -0xa0);
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
		g->DrawString(aScore, (mWidth - mApp->mHeaderFont->StringWidth(aScore)) - 0x50, aY);
		aY += 0x1b;
	}
}

// 0x402dc0: the link back to the main menu (id 0 of the original is the Internet score list: not here)
void HighScoreScreen::ButtonDepress(int theId)
{
	if (theId == 1)
		mApp->ShowMainMenu(0, true);
}

void HighScoreScreen::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}

// ---------------------------------------------------------------------------------------------------------
// VictoryScreen

// 0x401ca0
VictoryScreen::VictoryScreen(MummyApp* theApp) :
	HighScoreScreen(theApp)
{
}

VictoryScreen::~VictoryScreen()
{
}

// 0x402a00: nothing yet; the link comes with the name (Update)
void VictoryScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
}

// 0x402c60: the name for the score is asked for once the picture is in, and the link to the menu comes then
void VictoryScreen::Update()
{
	Widget::Update();
	if (!mNameAsked && mFade >= 175.0)
	{
		AskName();
		mWidgetManager->AddWidget(mMenuLink);
		mNameAsked = true;
	}
	if (mFade < 255.0)
	{
		mFade += 1.0;
		MarkDirty();
	}
}

// 0x402a10
void VictoryScreen::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, (int) mFade));
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], -0x96, 0);
	{
		Graphics gc(*g);
		gc.ClipRect(0, 0, 0x1e7, 0x1dd);
		gc.DrawImage(mApp->mImages[IMAGE_NEXTLEVEL], -5, 5);
	}
	if (mApp->mGameMode == MODE_ADVENTURE_DONE)
	{
		g->DrawImage(mApp->mImages[IMAGE_BEACH], (mWidth - 0x17b) / 2, (mHeight - 0x180) / 2);
		g->SetFont(mApp->mHeaderFont);
		g->SetColor(Color(0, 0xff, 0, (int) mFade));
		DrawCentered(g, GT(0x0048d34c), mWidth, 0x1e);
	}
	else
		g->DrawImage(mApp->mImages[IMAGE_BEACH], (mWidth - 0x17b) / 2, 0xe);
}
