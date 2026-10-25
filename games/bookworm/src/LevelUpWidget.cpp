#include "LevelUpWidget.h"
#include "BookWormApp.h"
#include "Board.h"
#include "StatKeeper.h"
#include "Lex.h"
#include "ModWords.h"
#include "OverlayWidget.h"
#include "WordBalloon.h"
#include "GreedyBots.h"
#include "ColorizedButton.h"
#include "Res.h"
#include "Compat2004.h"

#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Font.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cmath>

using namespace Sexy;

namespace Sexy
{

// 0x52f368
const GameTextId gRankNames[NUM_RANK_NAMES] =
{
	GT_RANK_BOOK_BURNER,
	GT_RANK_ENCYCLOPEDIA_SALESMAN,
	GT_RANK_TRAINEE,
	GT_RANK_CLERK,
	GT_RANK_SCRIBE,
	GT_RANK_BOOKKEEPER,
	GT_RANK_BOOKBINDER,
	GT_RANK_PROOFREADER,
	GT_RANK_ASSISTANT_LIBRARIAN,
	GT_RANK_LIBRARIAN,
	GT_RANK_SENIOR_LIBRARIAN,
	GT_RANK_ARCHIVIST,
	GT_RANK_GRAND_ARCHIVIST,
	GT_RANK_EDITOR,
	GT_RANK_EDITOR_IN_CHIEF,
	GT_RANK_PUBLISHER,
	GT_RANK_PUBLISHER_EMERITUS,
	GT_RANK_GLOSSOGRAPHER,
	GT_RANK_WORDSTALKER,
	GT_RANK_WORDHOARDER,
	GT_RANK_WORDMASTER,
	GT_RANK_VOCABULIST,
	GT_RANK_THESAURIAN,
	GT_RANK_THESAURUS_REX,
	GT_RANK_LEXOGRAPHER,
	GT_RANK_LEXOGRAPHER_1ST_CLASS,
	GT_RANK_DICTIONARIAN,
	GT_RANK_SUPER_DICTIONARIAN,
	GT_RANK_BOOKWORM,
	GT_RANK_MEGA_BOOKWORM,
	GT_RANK_ULTRA_BOOKWORM,
	GT_RANK_BOOKWORM_SUPREME
};

}

// Where Lex walks back to when the summary goes away (Board puts him there after creating him)
static const int LEX_HOME_X = 60;
static const int LEX_HOME_Y = 210;

// The panel's corners bounce on this y; gravity per update, bounce factor, rest threshold
static const double LEVELUP_FLOOR_Y = 75.0;
static const double LEVELUP_GRAVITY = 0.15;
static const double LEVELUP_BOUNCE = -0.4;
static const double LEVELUP_REST_VEL = -0.5;

// 0x42bd50
LevelUpWidget::LevelUpWidget(BookWormApp* theApp)
{
	mApp = theApp;
	mHasAlpha = true;
	mFalling = true;
	mContinueButton = NULL;
	mPanelImage = NULL;
	mPanelGraphics = NULL;
	mPanelDirty = false;
	mPanelAngle = 0.0;
	mLeftY = -150.0;
	mLeftVel = 0.0;
	mRightY = -174.0;
	mRightVel = 0.0;
	mPanelY = -150.0;
	mUnusedA8 = 189.0;
	mFrozenB0 = false;
	mSlide = 0.0;
	mSlidePhase = 0.0;
	mHeaderY = 0;
	mFooterY = 0;
	mLexArrived = false;
	mUnused78 = 0.03;
	mBotsBalloon = NULL;
}

// 0x42be40 (scalar deleting dtor 0x42beb0)
LevelUpWidget::~LevelUpWidget()
{
	DeletePanel();

	if (mContinueButton != NULL)
	{
		delete mContinueButton;
		mContinueButton = NULL;
	}
	delete mBotsBalloon;    // port(mod)
	mBotsBalloon = NULL;
}

// 0x42b980
void LevelUpWidget::DeletePanel()
{
	if (mPanelGraphics != NULL)
	{
		delete mPanelGraphics;
		mPanelGraphics = NULL;
	}

	if (mPanelImage != NULL)
	{
		delete mPanelImage;
		mPanelImage = NULL;
	}
}

// 0x42b9c0. A transparent off-screen image the size of dialog_levelup; DrawPanel paints it at the next Draw.
void LevelUpWidget::CreatePanel()
{
	DeletePanel();

	Image* aPanelBackground = mApp->mImages[IMAGE_DIALOG_LEVELUP];
	mPanelImage = new MemoryImage(mApp);
	mPanelImage->Create(aPanelBackground->GetWidth(), aPanelBackground->GetHeight());
	mPanelImage->SetImageMode(true, true);
	mPanelGraphics = new Graphics(mPanelImage);
	mPanelDirty = true;
}

// 0x42baa0
void LevelUpWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	CreatePanel();
}

// 0x42bad0. No NULL check: the app calls it only after the widget was added (AddedToManager made the button).
void LevelUpWidget::SetDisabled(bool isDisabled)
{
	Widget::SetDisabled(isDisabled);
	mContinueButton->SetDisabled(isDisabled);
}

// 0x42baf0. No NULL check, as SetDisabled.
void LevelUpWidget::SetVisible(bool isVisible)
{
	Widget::SetVisible(isVisible);
	mContinueButton->SetVisible(isVisible);
	if (mBotsBalloon != NULL)
		mBotsBalloon->SetVisible(isVisible);    // port(mod): hidden with the page (the help screen)
}

// 0x42bb10. Creates the Continue button the first time, parked at the bottom-right corner (off the screen)
// until the footer slides in, and moves Lex's walk target 26 px left and 6 px up, out of the panel's way.
void LevelUpWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	if (mContinueButton == NULL)
	{
		ImageFont* aFont = mApp->mFonts[FONT_VERDANA12BOLD];
		Image* aButtonImage = mApp->mImages[IMAGE_BAR_FOOTER_CONTINUE];
		mContinueButton = new ColorizedButton(LEVELUP_ID_CONTINUE, this);
		mContinueButton->Resize(mWidth, mHeight, aButtonImage->GetWidth(), aButtonImage->GetHeight());
		mContinueButton->mButtonImage = aButtonImage;
		mContinueButton->SetFont(aFont);
		mContinueButton->mDoFinger = true;
	}
	theWidgetManager->AddWidget(mContinueButton);

	mApp->mBoard->mLex->mTargetY -= 6;
	mApp->mBoard->mLex->mTargetX -= 26;
}

// 0x42bc20
void LevelUpWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);

	if (mContinueButton != NULL)
		theWidgetManager->RemoveWidget(mContinueButton);
	if (mBotsBalloon != NULL)
		theWidgetManager->RemoveWidget(mBotsBalloon);   // port(mod)

	if ((mApp->mBoard != NULL) && (mApp->mBoard->mLex != NULL))
	{
		mApp->mBoard->mLex->mTargetX = LEX_HOME_X;
		mApp->mBoard->mLex->mTargetY = LEX_HOME_Y;
	}
}

// 0x42bc80
void LevelUpWidget::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x42bc90. Continue: after a level the board goes on (BOARD_NEXT_LEVEL; Board removes this widget); after
// game over the saved game is deleted and the Hall of Fame opens.
void LevelUpWidget::ButtonDepress(int theId)
{
	if ((mContinueButton == NULL) || (theId != mContinueButton->mId))
		return;

	SetVisible(false);
	SetDisabled(true);
	mWidgetManager->SetFocus(mApp->mBoard);
	mApp->mBoard->mLex->mTargetX = LEX_HOME_X;
	mApp->mBoard->mLex->mTargetY = LEX_HOME_Y;

	Board* aBoard = mApp->mBoard;
	if (aBoard->mState == BOARD_GAMEOVER)
	{
		aBoard->DeleteSavedGame(mApp->mGameMode);
		mApp->ShowHallOfFame();
		return;
	}

	aBoard->mState = BOARD_NEXT_LEVEL;
}

// 0x42bee0 (100 updates a second). Nothing happens while disabled (the help screen is up).
// Phase 1: the panel's two top corners fall under gravity and bounce on y 75 independently, the panel
// tilting by their difference; from the constructor's values the left corner bounces at updates 55, 99
// and 116, the right one at 58, 104, 122 and 129, so the fall ends at update 129 with mPanelY = 75.
// Phase 2 (from the same update): the header and footer slide in along a quarter sine (53 updates), the
// Continue button riding on the footer; then Lex's word balloon is hidden.
void LevelUpWidget::Update()
{
	Widget::Update();

	if (mDisabled)
		return;

	// port: a test script's "quit" runs Shutdown in the middle of an update; ShutdownWidgets has then removed
	// the board (deleted later, which removes this widget) but WidgetManager::UpdateFrame still updates this
	// widget in the same update. The original never updates after Shutdown.
	if (mApp->mBoard == NULL)
		return;

	Lex* aLex = mApp->mBoard->mLex;

	if (mFalling)
	{
		Image* aPanelBackground = mApp->mImages[IMAGE_DIALOG_LEVELUP];

		// The velocity multiplied by the bounce is the one just incremented; a corner whose bounce is
		// slower than 0.5 rests (later updates re-add gravity, overshoot the floor and zero it again)
		mLeftVel += LEVELUP_GRAVITY;
		mLeftY += mLeftVel;
		if (mLeftY > LEVELUP_FLOOR_Y)
		{
			mLeftY = LEVELUP_FLOOR_Y;
			mLeftVel *= LEVELUP_BOUNCE;
			if (mLeftVel > LEVELUP_REST_VEL)
				mLeftVel = 0.0;
		}

		mRightVel += LEVELUP_GRAVITY;
		mRightY += mRightVel;
		if (mRightY > LEVELUP_FLOOR_Y)
		{
			mRightY = LEVELUP_FLOOR_Y;
			mRightVel *= LEVELUP_BOUNCE;
			if (mRightVel > LEVELUP_REST_VEL)
				mRightVel = 0.0;
		}

		mPanelY = (mRightY + mLeftY) * 0.5;
		mPanelAngle = atan2(mRightY - mLeftY, (double)aPanelBackground->GetWidth());

		if ((mLeftVel == 0.0) && (mRightVel == 0.0))
		{
			mFalling = false;
			mPanelAngle = 0.0;
		}

		MarkDirty();
	}

	if ((!mFalling) && (!mFrozenB0))
	{
		// Board bookkeeping: once Lex has stopped in BOARD_LEVEL_COMPLETE, the board's "new high score" flag
		// is cleared as soon as Lex is idle (nothing else reads it)
		if (!mLexArrived)
		{
			if ((mApp->mBoard->mState == BOARD_LEVEL_COMPLETE) && (!aLex->IsBusy()))
				mLexArrived = true;
		}
		else
		{
			Board* aBoard = mApp->mBoard;
			if ((aBoard->mNewHighScore) && (!aLex->IsBusy()))
				aBoard->mNewHighScore = false;
		}

		if (mSlide != 1.0)
		{
			mSlidePhase += 0.03;
			if (mSlidePhase < 1.570795)
				mSlide = sin(mSlidePhase);
			else
				mSlide = 1.0;

			// The level-up header's height is used at game over too (both bars are 106 high). Header from
			// about -103 to 0, footer from the bottom up to the panel's bottom minus the footer's height.
			int aHeaderHeight = mApp->mImages[IMAGE_BAR_LEVELUP_HEADER]->GetHeight();
			mHeaderY = (int)(aHeaderHeight * mSlide) - aHeaderHeight;

			int aFooterHeight = mApp->mImages[IMAGE_BAR_FOOTER_SCROLL]->GetHeight();
			mFooterY = mHeight - (int)((mHeight - (mPanelImage->GetHeight() + mPanelY - aFooterHeight)) * mSlide);

			int aPanelWidth = mPanelImage->GetWidth();
			mContinueButton->Move((mWidth - aPanelWidth - 5) + aPanelWidth / 2 - mContinueButton->mWidth / 2,
				mFooterY + 27);

			MarkDirty();
			return;
		}

		WordBalloon* aBalloon = mApp->mBoard->mOverlay->mBalloon;
		if (aBalloon->mVisible)
			aBalloon->Show(0);
		UpdateBotsBalloon();    // port(mod)
	}
}

// port(mod): a greedy bot's line in the balloon: its score, then how its game went
static SexyString BotLine(const char* theName, const GreedyBotResult& theResult)
{
	SexyString aLine = StrFormat("%s: %s\n", theName, CommaSeperate(theResult.mScore).c_str());
	if (theResult.mEnd == GREEDY_PLAYING)
		return aLine + StrFormat("level %d, playing...", theResult.mLevel + 1);
	if (theResult.mEnd == GREEDY_CAPPED)
		return aLine + StrFormat("level %d, still going", theResult.mLevel + 1);
	return aLine + StrFormat("%s at level %d", (theResult.mEnd == GREEDY_STUCK) ? "stuck" : "burned", theResult.mLevel + 1);
}

// port(mod): at game over, once the page has slid in, the scores of the greedy bots (mod "greedy-bots") from
// the game's first board in a balloon above Lex, in the left column; it follows their games until both have
// ended, then says which of them the player beat. A click hides it.
void LevelUpWidget::UpdateBotsBalloon()
{
	static int aColors[4][4] = { {255, 255, 255, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255} };   // Lex::Say's
	Board* aBoard = mApp->mBoard;
	GreedyBots* aBots = aBoard->mBots;
	if ((aBots == NULL) || (!aBots->mHasResults) || (aBoard->mState != BOARD_GAMEOVER) || (mWidgetManager == NULL))
		return;

	const GreedyBotResult& aBest = aBots->mResults[GREEDY_BEST_WORD];
	const GreedyBotResult& aFire = aBots->mResults[GREEDY_FIRE_FIRST];
	SexyString aText = BotLine("Best word", aBest) + "\n\n" + BotLine("Fire first", aFire);
	SexyString aFooter;
	if (aBots->Finished())
	{
		int aScore = aBoard->mGameStats->mScore;
		bool beatBest = aScore > aBest.mScore;
		bool beatFire = aScore > aFire.mScore;
		if ((beatBest) && (beatFire))
			aFooter = "You beat both bots!";
		else if (beatBest)
			aFooter = "You beat the best-word bot!";
		else if (beatFire)
			aFooter = "You beat the fire-first bot!";
		else
			aFooter = "The bots win this time.";
	}

	Lex* aLex = aBoard->mLex;
	if (mBotsBalloon == NULL)
	{
		mBotsBalloon = new WordBalloon(mApp, mApp->mImages[IMAGE_BACKDROP_WB_TEXT], mApp->mImages[IMAGE_WB_TEXT_TALK_TAIL],
			Point(0, -2));
		mBotsBalloon->SetColors(aColors, 4);
		mBotsBalloon->mTextInsets = Insets(10, 0, 10, 0);
		mWidgetManager->AddWidget(mBotsBalloon);
		mBotsBalloon->Show(-1);
	}
	else if (aText + aFooter == mBotsText)
	{
		mBotsBalloon->Move(mBotsBalloon->mX, aLex->mY - mBotsBalloon->mHeight + 30);
		return;
	}
	mBotsText = aText + aFooter;

	// as wide as the column left of the page; the tail over Lex's head
	int aWidth = (mWidth - mPanelImage->GetWidth() - 5) - 4;
	mBotsBalloon->SetBoxImages(mApp->mImages[IMAGE_BACKDROP_WB_TEXT], mApp->mImages[IMAGE_WB_TEXT_TALK_TAIL],
		Point((int)(aWidth * 0.6), -2));
	mBotsBalloon->SetHeader("Greedy bots", mApp->mFonts[FONT_MYRIADWEB12BOLD], 0);
	mBotsBalloon->SetText(aText, mApp->mFonts[FONT_MYRIADWEB10], -1);
	if (aFooter.length() != 0)
		mBotsBalloon->SetFooter(aFooter, mApp->mFonts[FONT_MYRIADWEB12BOLD], 0);
	else
		mBotsBalloon->ClearFooter();
	int aHeight = mBotsBalloon->GetPreferredHeight(aWidth);
	mBotsBalloon->Resize(2, aLex->mY - aHeight + 30, aWidth, aHeight);
}

// 0x42cdd0. The panel sits 5 px from the right edge. Falling, it is drawn rotated about its integer centre;
// then the header, the level number, the panel and the footer, in that order (the header under the panel,
// the footer over it).
void LevelUpWidget::Draw(Graphics* g)
{
	if ((mPanelImage != NULL) && (mPanelGraphics != NULL) && (mPanelDirty))
		DrawPanel(mPanelGraphics);

	int aPanelX = mWidth - mPanelImage->GetWidth() - 5;

	if (mFalling)
	{
		g->DrawImageRotated(mPanelImage, aPanelX, (int)mPanelY, mPanelAngle);
	}
	else if (!mFrozenB0)
	{
		int aCenterX = aPanelX + mPanelImage->GetWidth() / 2;

		Image* aHeader;
		if (mApp->mBoard->mState == BOARD_GAMEOVER)
			aHeader = mApp->mImages[IMAGE_BAR_GAMEOVER_HEADER];
		else
			aHeader = mApp->mImages[IMAGE_BAR_LEVELUP_HEADER];
		g->DrawImage(aHeader, aCenterX - aHeader->GetWidth() / 2 + 4, mHeaderY);

		// The level number, centred over the header's width: one px right for one digit, one left otherwise
		SexyString aLevelText = StrFormat("%d", mApp->mBoard->mLevel + 1);
		g->SetFont(mApp->mFonts[FONT_VERDANA18BOLDCOLOR]);
		int aTextX = aCenterX - aHeader->GetWidth() / 2;
		if (aLevelText.size() == 1)
			aTextX += 1;
		else
			aTextX -= 1;
		int aTextY = mHeaderY + ((mApp->mBoard->mState == BOARD_GAMEOVER) ? 55 : 52);
		g->SetColor(Color(255, 255, 255, 255));
		WriteString2004(g, aLevelText, aTextX, aTextY, aHeader->GetWidth(), 0, true, 0, -1);

		g->DrawImage(mPanelImage, aPanelX, (int)mPanelY);

		// x from the header's width, not the footer's (both 468)
		g->DrawImage(mApp->mImages[IMAGE_BAR_FOOTER_SCROLL], aCenterX - aHeader->GetWidth() / 2 + 4, mFooterY);
	}

	Widget::Draw(g);
}

// 0x42c210. Paints the 490x390 panel once (panel coordinates; y are baselines; the WriteString2004 calls
// centre their text in [x, x + w]). Left column: the best and the longest word of the level (of the game
// at game over), the level time and the total time; right column: the rank name, its picture in a frame
// and the score of the next rank. Heights at the original fonts: Scripps 13/15/18 = 20/23/28.
void LevelUpWidget::DrawPanel(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_DIALOG_LEVELUP], 0, 0);
	Widget::Draw(g);

	Board* aBoard = mApp->mBoard;
	bool isLevelSummary = aBoard->mState != BOARD_GAMEOVER;
	StatKeeper* aStats = isLevelSummary ? aBoard->mLevelStats : aBoard->mGameStats;

	SexyString aBestLabel = GameText(GT_SUMMARY_BEST_WORD);
	SexyString aBestWord = aStats->mBestWord;
	SexyString aBestPoints = StrFormat(GameTextC(GT_SUMMARY_POINTS_FORMAT), aStats->mBestWordScore);
	SexyString aLongestLabel = GameText(GT_SUMMARY_LONGEST_WORD);
	SexyString aLongestWord = aStats->mLongestWord;
	SexyString aLongestLetters = StrFormat(GameTextC(GT_SUMMARY_LETTERS_FORMAT), (int)aStats->mLongestWord.size());
	SexyString aTimeText = mApp->GetLevelTimeString(GameText(GT_SUMMARY_TIME));
	SexyString aTotalText = mApp->GetTotalTimeString(GameText(GT_SUMMARY_TOTAL));

	// Left column: x 34, 206 wide, black except the words (blue)
	g->SetColor(Color(0, 0, 0));
	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD]);
	int aY = g->GetFont()->GetHeight() * 2 + 18;
	WriteString2004(g, aBestLabel, 34, aY, 206, 0, true, 0, -1);

	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS18BOLD]);
	aY += g->GetFont()->GetHeight();
	g->SetColor(Color(0, 0, 255));
	WriteString2004(g, aBestWord, 34, aY, 206, 0, true, 0, -1);
	g->SetColor(Color(0, 0, 0));

	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS13BOLD]);
	aY += g->GetFont()->GetHeight();
	WriteString2004(g, aBestPoints, 34, aY, 206, 0, true, 0, -1);

	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD]);
	aY += g->GetFont()->GetHeight() * 2;
	WriteString2004(g, aLongestLabel, 34, aY, 206, 0, true, 0, -1);

	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS18BOLD]);
	aY += g->GetFont()->GetHeight();
	g->SetColor(Color(0, 0, 255));
	WriteString2004(g, aLongestWord, 34, aY, 206, 0, true, 0, -1);
	g->SetColor(Color(0, 0, 0));

	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS13BOLD]);
	aY += g->GetFont()->GetHeight();
	WriteString2004(g, aLongestLetters, 34, aY, 206, 0, true, 0, -1);

	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD]);
	aY += g->GetFont()->GetHeight() * 2;
	g->DrawString(aTimeText, 74, aY);
	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD]);
	g->DrawString(aTotalText, 74, aY + g->GetFont()->GetHeight());

	// port(mod): the game's and the player's all-time count of a mod's words (rude-words' "stats")
	SexyString aModLabel = ModWordStatsLabel();
	if (aModLabel.length() != 0)
	{
		int aModY = aY + g->GetFont()->GetHeight() * 2;
		g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS13BOLD]);
		g->DrawString(StrFormat("%s: %d, all time %d", aModLabel.c_str(), aBoard->mModWordsPlayed,
			aBoard->GetModWordsAllTime()), 74, aModY);
	}

	// The rank: the level reached (the next level after a level, the current one at game over)
	unsigned int aRank = isLevelSummary ? aBoard->mLevel + 1 : aBoard->mLevel;
	int aNextRankScore = Board::LevelThreshold(aRank);
	SexyString aRankLabel = GameText(GT_SUMMARY_YOUR_RANK);
	SexyString aNextLabel = GameText(GT_SUMMARY_NEXT_RANK);
	SexyString aNextScore = StrFormat(GameTextC(GT_SUMMARY_NEXT_RANK_FORMAT), aNextRankScore);
	unsigned int aNameIdx = aRank;
	if (aNameIdx > (unsigned int)(NUM_RANK_NAMES - 1))
		aNameIdx = NUM_RANK_NAMES - 1;
	SexyString aRankName = GameText(gRankNames[aNameIdx]);

	// Right column: x 256, 204 wide
	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD]);
	int aRankLabelY = g->GetFont()->GetHeight() + 18;
	WriteString2004(g, aRankLabel, 256, aRankLabelY, 204, 0, true, 0, -1);

	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD]);
	int aRankNameY = aRankLabelY + g->GetFont()->GetHeight();
	g->SetColor(Color(0, 0, 255));
	WriteString2004(g, aRankName, 256, aRankNameY, 204, 0, true, 0, -1);
	g->SetColor(Color(0, 0, 0));

	int aFrameY = aRankNameY + (int)(g->GetFont()->GetHeight() * 0.5);
	Image* aFrame = mApp->mImages[IMAGE_RANKFRAME];
	g->DrawImage(aFrame, (204 - aFrame->GetWidth()) / 2 + 256, aFrameY - 2);

	// The rank picture: the strip holds square cells (its width) one per rank; a rank past the last cell
	// shows the last one with a star holding the number of ranks beyond it
	MemoryImage* aStrip = mApp->mRankStripImage;
	int aCellSize = aStrip->GetWidth();
	int aCell = (int)aRank;
	if (aStrip->GetHeight() / aStrip->GetWidth() - 1 <= (int)aRank)
		aCell = aStrip->GetHeight() / aStrip->GetWidth() - 1;
	int aPictureX = (204 - aCellSize) / 2 + 256;
	int aPictureY = aFrameY + 10;
	g->DrawImage(aStrip, aPictureX, aPictureY, Rect(0, aCell * aCellSize, aCellSize, aCellSize));

	if (aCell < (int)aRank)
	{
		Image* aStar = mApp->mImages[IMAGE_LEVEL_UP_STAR];
		int aStarX = aPictureX - aStar->GetWidth() / 2 - 7 + aStrip->GetWidth();
		int aStarY = aPictureY - aStar->GetHeight() / 2 - 7 + aStrip->GetWidth();
		g->DrawImage(aStar, aStarX, aStarY);

		g->SetFont(mApp->mFonts[FONT_MYRIADWEB10]);
		g->SetColor(Color(0, 0, 0));
		SexyString anExtraText = StrFormat("%d", (int)aRank - aCell);
		int anExtraX = (aStar->GetWidth() - g->GetFont()->StringWidth(anExtraText)) / 2 + 1 + aStarX;
		int anExtraY = (aStar->GetHeight() + g->GetFont()->GetHeight()) / 2 + aStarY;
		g->DrawString(anExtraText, anExtraX, anExtraY);
	}

	// The next rank's label and score below the picture: (int)(h * -1.5) is truncated toward zero
	g->SetColor(Color(0, 0, 0));
	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD]);
	int aNextY = aPictureY + (aCellSize - (int)(g->GetFont()->GetHeight() * -1.5));
	WriteString2004(g, aNextLabel, 256, aNextY, 204, 0, true, 0, -1);
	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD]);
	WriteString2004(g, aNextScore, 256, aNextY + g->GetFont()->GetHeight(), 204, 0, true, 0, -1);

	mPanelDirty = false;
}
