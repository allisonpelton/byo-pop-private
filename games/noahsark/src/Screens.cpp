// The full-screen widgets of Noah's Ark Deluxe (see Screens.h).

#include "Screens.h"
#include "NoahApp.h"
#include "Button2003.h"
#include "Board.h"
#include "GameText.h"
#include "ModFeatures.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// TitleScreen

// 0x41f830
TitleScreen::TitleScreen(NoahApp* theApp) :
	mApp(theApp),
	mProgress(0.0),
	mFade(0.0),
	mLinkShown(false),
	mTip(0),
	mClicked(false)
{
	// port: the shareware branches (plays remaining, the register link, the trial counter) are not ported; the
	// screen always takes the registered path (docs/GAME_NOTES.md section 3)
	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mFonts[FONT_SMALL]);
	mPlayLink->mColor = Color(0xf5, 0xcd, 0x0a);
	mPlayLink->mOverColor = Color(0xf5, 0xcd, 0x0a);
	mPlayLink->mLabel = GT(0x004961d0);
	mPlayLink->mUnderlineSize = 0;
	mPlayLink->SetVisible(false);
	mHasTransparencies = false;
}

TitleScreen::~TitleScreen()
{
	delete mPlayLink;
}

// 0x420460 / 0x420490
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4204c0
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth) / 2, 0x1bb, aWidth, 0x20);
}

// 0x4202b0
void TitleScreen::Update()
{
	Widget::Update();
	if (mUpdateCnt % 3 == 0)
	{
		mProgress = mApp->GetLoadingThreadProgress();
		if ((mProgress == 1.0) && mApp->mLoaded && !mLinkShown)
		{
			mPlayLink->SetVisible(true);
			mLinkShown = true;
		}
		MarkDirty();
	}
	if (mUpdateCnt % 300 == 0)
	{
		mTip++;
		MarkDirty();
	}
}

// 0x41fc70
void TitleScreen::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	g->DrawImage(anImages[IMAGE_TITLE], 0, 0);
	Image* aBar = anImages[IMAGE_TITLEBAR];
	g->DrawImage(aBar, 0x99, 0x174, Rect(0, 0, (int) (mProgress * aBar->mWidth), aBar->mHeight));
	g->DrawImage(anImages[IMAGE_POPCAPLOGO], 0x20b, 0x16a);
	g->DrawImage(anImages[IMAGE_POPCAPURL], 0x20b, 0x1cb);

	// the line in the middle (registered: the thanks), black outline then yellow
	SexyString aLine = GT(0x0049620c);
	g->SetFont(mApp->mFonts[FONT_SMALL]);
	g->SetColor(Color(0, 0, 0));
	for (int dx = -1; dx < 2; dx++)
		for (int dy = -1; dy < 2; dy++)
		{
			int aWidth = g->GetFont()->StringWidth(aLine);
			g->DrawString(aLine, (mWidth - aWidth) / 2 + dx, dy + 0x170);
		}
	g->SetColor(Color(0xff, 0xff, 0));
	g->DrawString(aLine, (mWidth - g->GetFont()->StringWidth(aLine)) / 2, 0x170);

	g->SetFont(mApp->mFonts[FONT_SMALL]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(GT(0x00496200) + mApp->mProductVersion, 0x16, 0xc2);

	if (mPlayLink->mVisible)
	{
		g->SetColor(Color(0, 0, 0));
		g->SetFont(mApp->mFonts[FONT_SMALL]);
		int aWidth = g->GetFont()->StringWidth(mPlayLink->mLabel);
		for (int dx = -2; dx < 1; dx++)
			for (int dy = -2; dy < 1; dy++)
				g->DrawString(mPlayLink->mLabel, (mWidth - aWidth) / 2 + dx, dy + 0x1d8);
	}
}

// 0x420550: a press on the link clicks, through the app
void TitleScreen::ButtonPress(int theId)
{
	(void) theId;
	mApp->ButtonPress(0);
}

// 0x420560: the play link starts the game's menus
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == 0)
		mClicked = true;
	else
		mApp->TitleScreenDone();
}

// ---------------------------------------------------------------------------------------------------------
// IntroWidget

// 0x413ea0
IntroWidget::IntroWidget(NoahApp* theApp) :
	mApp(theApp),
	mContinueButton(NULL),
	mHover(0),
	mAlpha(0xff)
{
	mHasTransparencies = true;
	Image* aBack = mApp->mImages[IMAGE_BUTTONBACK];
	Font* aFont = mApp->mFonts[FONT_MEDIUM];
	mActionButton = mApp->AddDialogButton(BUTTON_ACTION, Rect(100, 0xa5, 0xc4, 0x2f), aBack);
	mActionButton->SetFont(aFont);
	mActionButton->mLabel = GT(0x00494a48);
	mActionButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mStrategyButton = mApp->AddDialogButton(BUTTON_STRATEGY, Rect(100, 0xdb, 0xc4, 0x2f), aBack);
	mStrategyButton->SetFont(aFont);
	mStrategyButton->mLabel = GT(0x00494a3c);
	mStrategyButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mPuzzleButton = mApp->AddDialogButton(BUTTON_PUZZLE, Rect(100, 0x111, 0xc4, 0x2f), aBack);
	mPuzzleButton->SetFont(aFont);
	mPuzzleButton->mLabel = GT(0x00494a34);
	mPuzzleButton->SetColor(0, Color(0xff, 0xff, 0xff));
	if (mApp->HasSavedGame())
	{
		mContinueButton = mApp->AddDialogButton(BUTTON_CONTINUE, Rect(100, 0x147, 0xc4, 0x2f), aBack);
		mContinueButton->SetFont(aFont);
		mContinueButton->mLabel = GT(0x00494a28);
		mContinueButton->SetColor(0, Color(0xff, 0xff, 0xff));
	}
	mQuitButton = mApp->AddDialogButton(BUTTON_QUIT, Rect(100, 0x17d, 0xc4, 0x2f), aBack);
	mQuitButton->SetFont(aFont);
	mQuitButton->mLabel = GT(0x00494a20);
	mQuitButton->SetColor(0, Color(0xff, 0xff, 0xff));
}

// 0x414360: the buttons belong to the app's list (NoahApp::RemoveIntro) and go with the widget
IntroWidget::~IntroWidget()
{
}

// 0x414410
void IntroWidget::Update()
{
	if (mAlpha < 0xff)
	{
		mAlpha += 10;
		MarkDirty();
	}
	bool aOver = false;
	DialogButton* aButtons[5] = {mActionButton, mStrategyButton, mPuzzleButton, mContinueButton, mQuitButton};
	for (int i = 0; i < 5; i++)
	{
		if ((aButtons[i] != NULL) && aButtons[i]->mIsOver)
		{
			aOver = true;
			if (mHover != i + 1)
			{
				mHover = i + 1;
				MarkDirty();
				mAlpha = 0;
			}
		}
	}
	if (!aOver && (mHover != 0))
	{
		mHover = 0;
		MarkDirty();
		mAlpha = 0;
	}
}

// 0x414530
void IntroWidget::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_GAMESELECTOR], 0, 0);
	if (mContinueButton == NULL)
	{
		// no saved game: the Continue button is drawn dimmed, as a picture
		DrawImageBox2003(g, Rect(100, 0x147, 0xc4, 0x2f), mApp->mImages[IMAGE_BUTTONBACK]);
		g->SetFont(mApp->mFonts[FONT_COLORLESS]);
		g->SetColor(Color(0x78, 0x78, 0x78));
		SexyString aText = GT(0x00494a28);
		g->DrawString(aText, (0xc4 - g->GetFont()->StringWidth(aText)) / 2 + 100, 0x16d);
	}

	// the lines about the button the mouse is on (or the general invitation)
	static const unsigned int aTables[6][6] =
	{
		{0x0049c510, 0x004949f0, 0x0049c510, 0x004949e0, 0x0049c510, 0x0049c510},
		{0x004949d0, 0x004949c0, 0x004949b0, 0x004949a0, 0x00494994, 0x00494984},
		{0x00494978, 0x0049496c, 0x0049495c, 0x00494950, 0x00494944, 0x00494938},
		{0x00494928, 0x00494918, 0x00494908, 0x004948f8, 0x004948ec, 0x004948e0},
		{0x004948d0, 0x004948c0, 0x004948b0, 0x004948a4, 0x00494894, 0x00494888},
		{0x0049c510, 0x00494878, 0x0049c510, 0x0049c510, 0x0049c510, 0x0049c510}
	};
	bool aFirstEmpty = GT(aTables[0][mHover]).empty();
	bool aLastEmpty = GT(aTables[5][mHover]).empty();
	int aY = 0xcd;
	if (!aFirstEmpty && aLastEmpty)
		aY = 0xe1;
	g->SetFont(mApp->mFonts[FONT_GARAMOND_MED]);
	g->SetColor(Color(0, 0, 0, mAlpha));
	for (int i = 0; i < 6; i++)
	{
		const SexyString& aText = GT(aTables[i][mHover]);
		if (aTables[i][mHover] == 0x0049c510)
			continue;
		g->DrawString(aText, (0x117 - g->GetFont()->StringWidth(aText)) / 2 + 0x150, aY + i * 0x28);
	}
}

// ---------------------------------------------------------------------------------------------------------
// NextLevelWidget

static const int gRankThresholds[24] =
{
	1250, 2500, 5000, 7500, 10000, 12500, 15000, 17500, 20000, 25000, 30000, 35000, 40000, 45000, 50000, 75000,
	100000, 150000, 200000, 500000, 1000000, 5000000, 10000000, 999999999
};

// 0x4944c0: the ranks of the game-over screen, the same names in capitals
static const unsigned int gRankNamesCaps[23] =
{
	0x0049466c, 0x0049465c, 0x00494650, 0x00494644, 0x00494638, 0x00494624, 0x00494618, 0x00494608, 0x004945f8,
	0x004945e8, 0x004945d8, 0x004945c8, 0x004945b8, 0x004945ac, 0x004945a0, 0x00494594, 0x00494588, 0x00494574,
	0x00494564, 0x00494558, 0x00494544, 0x00494538, 0x00494528
};

// 0x494ab4: the names of the ranks from the lowest
static const unsigned int gRankNames[23] =
{
	0x00494e68, 0x00494e58, 0x00494e4c, 0x00494e40, 0x00494e34, 0x00494e20, 0x00494e14, 0x00494e04, 0x00494df4,
	0x00494de4, 0x00494dd4, 0x00494dc4, 0x00494db4, 0x00494da8, 0x00494d9c, 0x00494d90, 0x00494d84, 0x00494d70,
	0x00494d60, 0x00494d54, 0x00494d40, 0x00494d34, 0x00494d24
};

// 0x494b14 and 0x494b24: the two lines at the bottom, four of each
static const unsigned int gTipLines[4] = {0x00494ce4, 0x00494ca0, 0x00494c68, 0x00494c30};
static const unsigned int gTipLines2[4] = {0x00494bf0, 0x00494bac, 0x00494b70, 0x00494b34};

// 0x414f10
NextLevelWidget::NextLevelWidget(NoahApp* theApp) :
	mApp(theApp),
	mBackground(NULL),
	mBonus(false),
	mBonusScore(0)
{
	mHasTransparencies = true;
	if (mApp->mBonusRound)
	{
		mBonus = true;
		mBonusScore = mApp->mGameMode;
	}
	mSaved = mApp->mSavedThisLevel;
	mApp->mSavedThisLevel = 0;
	mApp->mGameScore += mSaved * 5;

	mNextLink = new HyperlinkWidget(0, this);
	mNextLink->mLabel = GT(0x00494e9c);
	mNextLink->mColor = Color(0xff, 0xff, 0);
	mNextLink->SetFont(mApp->mFonts[FONT_SMALL]);
	mNextLink->mUnderlineSize = 0;
	mSeconds = (mApp->mBoard == NULL) ? 0 : (int) ((GameTickCount() - mApp->mBoard->mStartTick) / 1000);
	if ((mApp->mBoard != NULL) && ModFixOn("steadyClock"))
		mSeconds = (mApp->mUpdateCount - mApp->mBoard->mStartUpdate) / 100;     // fix steadyClock: 100 updates a second
	int aKinds = (mApp->mGameMode == GAMEMODE_STRATEGY) ? 3 : 4;
	mTip = CrtRand() % aKinds;
	if (mApp->mGameMode == GAMEMODE_STRATEGY)
		mTip++;
	mApp->PlaySample(SOUND_LEVELUPVOICE);
}

NextLevelWidget::~NextLevelWidget()
{
	delete mNextLink;
	delete mBackground;
}

void NextLevelWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNextLink);
}

void NextLevelWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNextLink);
}

// 0x4165f0
void NextLevelWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mNextLink->mFont->StringWidth(mNextLink->mLabel);
	mNextLink->Resize((mWidth - aWidth) / 2 + mX, mHeight - 0x37, aWidth + 8, 0x1d);   // the original's y has no mY added
}

// 0x420550: a press on the link clicks, through the app
void NextLevelWidget::ButtonPress(int theId)
{
	(void) theId;
	mApp->ButtonPress(0);
}

// 0x416650: the link ends the bonus round and starts the next level
void NextLevelWidget::ButtonDepress(int theId)
{
	if (theId == 0)
	{
		if (mApp->mBonusRound)
		{
			mApp->mBonusRound = false;
			mApp->mGameMode = mApp->mSavedGameMode;
		}
		mApp->mBoard->NewLevel();
	}
	mApp->RemoveNextLevel();
}

static void DrawCentered(Graphics* g, Font* theFont, const SexyString& theText, int theWidth, int theY, const Color& theColor)
{
	g->SetFont(theFont);
	g->SetColor(theColor);
	g->DrawString(theText, (theWidth - theFont->StringWidth(theText)) / 2, theY);
}

// 0x4151f0: the panel is drawn once into an image
void NextLevelWidget::Draw(Graphics* g)
{
	if (mBackground != NULL)
	{
		g->DrawImage(mBackground, 0, 0);
		return;
	}
	mBackground = new MemoryImage(mApp);
	mBackground->Create(mWidth, mHeight);
	Graphics ig(mBackground);
	Image** anImages = mApp->mImages;
	ig.DrawImage(anImages[IMAGE_FRAME], 0, 0);
	DrawImageBox2003(&ig, Rect(0, 0, 0x1d8, 0x1d7), anImages[IMAGE_DIALOG]);

	SexyString aTitle = GT(0x00494f34);
	ig.SetFont(mApp->mFonts[FONT_BIG]);
	ig.SetColor(Color(0x32, 0x32, 0x32, 200));
	for (int dx = 0; dx < 2; dx++)
		for (int aPass = 0; aPass < 2; aPass++)
			ig.DrawString(aTitle, (mWidth - ig.GetFont()->StringWidth(aTitle)) / 2 + dx, 0x3c);
	ig.SetColor(Color(0xff, 0xff, 0xff));
	ig.DrawString(aTitle, (mWidth - ig.GetFont()->StringWidth(aTitle)) / 2, 0x3c);

	Image* aPicture = anImages[IMAGE_HAPPYENDING];
	int aPicX = (mWidth - aPicture->mWidth) / 2;
	ig.DrawImage(aPicture, aPicX, 0x50);

	Font* aTiny = mApp->mFonts[FONT_GARAMOND_TINY];
	ig.SetFont(aTiny);
	ig.SetColor(Color(0x75, 0x5d, 0x43));
	SexyString aTime = FormatText2(GT((mSeconds % 60 < 10) ? 0x00494f24 : 0x00494f18), mSeconds / 60, mSeconds % 60);
	ig.DrawString(aTime, aPicX + 10, 100);

	SexyString aLeft, aRight;
	if (!mBonus)
	{
		aLeft = FormatText(GT(0x00494ee8), mApp->mComboCount);
		aRight = FormatText(GT(0x00494ed8), mApp->mBestCombo);
	}
	else
	{
		aLeft = FormatText(GT(0x00494f04), mApp->mAnimalsSaved);
		aRight = FormatText(GT(0x00494ef8), mSaved * 5);
	}
	int aMiddleStart = aPicX + 10 + aTiny->StringWidth(aTime);
	int aRightX = aPicX + aPicture->mWidth - 10 - aTiny->StringWidth(aRight);
	ig.DrawString(aLeft, (aRightX - aMiddleStart - aTiny->StringWidth(aLeft)) / 2 + aMiddleStart, 100);
	ig.DrawString(aRight, aRightX, 100);

	// the rank
	DrawCentered(&ig, aTiny, GT(0x00494ecc), mWidth, 0x104, Color(0x75, 0x5d, 0x43));
	int aRank = 0;
	while ((aRank < 23) && (mApp->mGameScore >= gRankThresholds[aRank]))
		aRank++;
	if (aRank > 22)
		aRank = 22;
	DrawCentered(&ig, mApp->mFonts[FONT_MEDIUM], GT(gRankNames[aRank]), mWidth, 0x128, Color(0xff, 0xff, 0xff));
	DrawCentered(&ig, aTiny, FormatText(GT(0x00494eb4), gRankThresholds[aRank]), mWidth, 0x144,
		Color(0x75, 0x5d, 0x43));

	DrawCentered(&ig, aTiny, GT(gTipLines[mTip]), mWidth, 0x169, Color(0xc2, 0x9f, 0x83));
	DrawCentered(&ig, aTiny, GT(gTipLines2[mTip]), mWidth, 0x182, Color(0xc2, 0x9f, 0x83));
	g->DrawImage(mBackground, 0, 0);
}

// ---------------------------------------------------------------------------------------------------------
// HighScoreWidget

std::string Sexy::Ordinal(int theNumber)
{
	// 0x4124d0
	char aBuffer[32];
	snprintf(aBuffer, sizeof(aBuffer), "%d", theNumber);
	const char* aSuffix;
	if ((theNumber % 100) / 10 != 1)
	{
		switch (theNumber % 10)
		{
		case 1: aSuffix = GTC(0x004947a4); break;
		case 2: aSuffix = GTC(0x004947a8); break;
		case 3: aSuffix = GTC(0x004947ac); break;
		default: aSuffix = GTC(0x004947b0); break;
		}
	}
	else
		aSuffix = GTC(0x004947b0);
	return std::string(aBuffer) + aSuffix;
}

// 0x411a70
HighScoreWidget::HighScoreWidget(NoahApp* theApp) :
	mApp(theApp),
	mChecked(false),
	mFade(0.0)
{
	mHasTransparencies = true;
	mMenuLink = new HyperlinkWidget(1, this);
	mMenuLink->mLabel = GT(0x004946c8);
	mMenuLink->SetFont(mApp->mFonts[FONT_MEDIUM]);
	mMenuLink->mColor = Color(0xf8, 0x7a, 0x02);
	mMenuLink->mOverColor = Color(0xff, 0xff, 0xff);

	// the defaults: five names with 1000, 900, 800, 700 and 500 points
	static const unsigned int aNames[5] = {0x004946bc, 0x004946b4, 0x004946ac, 0x004946a4, 0x0049469c};
	static const int aScores[5] = {1000, 900, 800, 700, 500};
	for (int i = 0; i < 5; i++)
	{
		mNames[i] = GT(aNames[i]);
		mScores[i] = aScores[i];
	}
	Load();
}

HighScoreWidget::~HighScoreWidget()
{
	delete mMenuLink;
}

void HighScoreWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuLink);
}

void HighScoreWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuLink);
}

// 0x4134d0
void HighScoreWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mMenuLink->Resize(mWidth / 2 - 0xd7 + mX, 0x1a2, 0x1ae, 0x22);
}

// 0x411ea0: hiscores.dat is five times [u16 length][name][i32 score][i32 check]. Each record is stored as it is
// read: the name up to its first NUL, and the score, or when the check does not match the score's remainder
// mod 256 (C's %, so a negative score stays negative); the next record is read all the same. A file that ends early
// (the original's fread just fails there) keeps the records read so far and leaves the later ones as they were.
void HighScoreWidget::Load()
{
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/hiscores" + ScoreKeySuffix() + ".dat", &aData))
		return;
	size_t aPos = 0;
	for (int i = 0; i < 5; i++)
	{
		if (aPos + 2 > aData.size())
			return;
		unsigned short aLen;
		memcpy(&aLen, aData.data() + aPos, 2);
		aPos += 2;
		if (aPos + aLen + 8 > aData.size())
			return;
		std::string aName(aData.data() + aPos, aLen);
		aPos += aLen;
		int aScore, aCheck;
		memcpy(&aScore, aData.data() + aPos, 4);
		memcpy(&aCheck, aData.data() + aPos + 4, 4);
		aPos += 8;
		unsigned int aWant = (unsigned int) aScore ^ 0x42beef;
		for (int j = 0; j < (int) aLen; j++)
			aWant ^= (unsigned int) (aName[j] * 0x11);
		mNames[i] = aName.c_str();
		mScores[i] = ((unsigned int) aCheck == aWant) ? aScore : aScore % 256;
	}
}

// 0x412060
void HighScoreWidget::Save()
{
	std::string aData;
	for (int i = 0; i < 5; i++)
	{
		unsigned short aLen = (unsigned short) mNames[i].size();
		aData.append((const char*) &aLen, 2);
		aData.append(mNames[i]);
		int aScore = mScores[i];
		aData.append((const char*) &aScore, 4);
		unsigned int aCheck = (unsigned int) aScore ^ 0x42beef;
		for (int j = 0; j < (int) aLen; j++)
			aCheck ^= (unsigned int) (mNames[i][j] * 0x11);
		aData.append((const char*) &aCheck, 4);
	}
	Platform::WriteUserFileAtomic("hiscores" + ScoreKeySuffix() + ".dat", aData);
}

// 0x412680
int HighScoreWidget::RankOf(int theScore)
{
	int aRank = 0;
	for (int i = 4; i >= 0; i--)
		if (mScores[i] < theScore)
			aRank = i + 1;
	return aRank;
}

// 0x413550: put the game's score in at its place with the name; the lower ones move down
void HighScoreWidget::AddScore(const std::string& theName)
{
	if (theName.empty())
		return;
	int aScore = mApp->mGameScore;
	int aRank = RankOf(aScore);
	if (aRank == 0)
		return;
	for (int i = 4; i >= aRank; i--)
	{
		mNames[i] = mNames[i - 1];
		mScores[i] = mScores[i - 1];
	}
	mNames[aRank - 1] = theName;
	mScores[aRank - 1] = aScore;
	Save();
}

// 0x412240: the score earns a place: the player is asked for a name
void HighScoreWidget::CheckRank()
{
	int aRank = RankOf(mApp->mGameScore);
	if (aRank > 0)
		mApp->ShowRecordScore(Ordinal(aRank));
}

// 0x413460
void HighScoreWidget::Update()
{
	mUpdateCnt++;
	if (!mChecked && (mFade >= 175.0))
	{
		CheckRank();
		mChecked = true;
	}
	if (mFade < 255.0)
	{
		mFade += ModFixOn("quickScores") ? 3.0 : 1.0;      // fix quickScores: the fade-in of the game-over screen, and the wait for the name, takes a third of the time
		if (mFade > 255.0)
			mFade = 255.0;
		MarkDirty();
	}
}

// 0x412b40
void HighScoreWidget::Draw(Graphics* g)
{
	int aAlpha = (int) mFade;
	g->SetColorizeImages(true);
	g->SetColor(Color(0x59, 0x72, 0xb3));
	g->FillRect(0, 0, mWidth, mHeight);
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (mApp->mBoard != NULL)
	{
		mApp->mBoard->DrawFish(g);
		mApp->mBoard->DrawBubbles(g);
	}
	g->SetFont(mApp->mFonts[FONT_MEDIUM]);
	int aScore = mApp->mGameScore;
	Font* aFont = mApp->mFonts[FONT_MEDIUM];
	g->SetColor(Color(0xff, 0xbf, 0x22, aAlpha));
	g->DrawString(GT(0x004947d8), (mWidth - aFont->StringWidth(GT(0x004947d8))) / 2, 0x27);
	g->SetColor(Color(0xff, 0xff, 0, aAlpha));
	SexyString aPoints = FormatText(GT(0x004947cc), aScore);
	g->DrawString(aPoints, (mWidth - aFont->StringWidth(aPoints)) / 2, 0x48);
	g->SetColor(Color(0xff, 0xbf, 0x22, aAlpha));
	g->DrawString(GT(0x004947b4), (mWidth - aFont->StringWidth(GT(0x004947b4))) / 2, 0x72);
	int aRank = 0;
	while ((aRank < 23) && (aScore >= gRankThresholds[aRank]))
		aRank++;
	if (aRank > 22)
		aRank = 22;
	g->SetColor(Color(0xff, 0xff, 0, aAlpha));
	g->DrawString(GT(gRankNamesCaps[aRank]), (mWidth - aFont->StringWidth(GT(gRankNamesCaps[aRank]))) / 2, 0x95);

	g->SetColor(Color(0xff, 0xff, 0xff, aAlpha));
	DrawImageBox2003(g, Rect((mWidth - 0x19f) / 2, 0xa0, 0x19f, 0xf0), mApp->mImages[IMAGE_DIALOG]);
	g->SetColorizeImages(false);
	g->SetFont(aFont);
	g->SetColor(Color(0xff, 0xbf, 0x22, aAlpha));
	g->DrawString(GT(0x0049444c), (mWidth - aFont->StringWidth(GT(0x0049444c))) / 2, 0xd1);
	int aY = 0xf7;
	for (int i = 0; i < 5; i++)
	{
		g->SetFont(mApp->mFonts[FONT_GARAMOND_MED]);
		g->SetColor(Color(0, 0, 0, aAlpha));
		g->DrawString(mNames[i], 0x96, aY);
		g->SetFont(aFont);
		g->SetColor(Color(0xff, 0xff, 0, aAlpha));
		SexyString aScoreText = StrFormat("%d", mScores[i]);
		g->DrawString(aScoreText, (mWidth - aFont->StringWidth(aScoreText)) - 0x96, aY);
		aY += 0x1e;
	}
}

// 0x420550: a press on the link clicks, through the app
void HighScoreWidget::ButtonPress(int theId)
{
	(void) theId;
	mApp->ButtonPress(0);
}

// 0x413520
void HighScoreWidget::ButtonDepress(int theId)
{
	if (theId == 1)
		mApp->ShowIntro(false);
}
