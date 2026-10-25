#include "SidePanel.h"
#include "Board.h"
#include "BookWormApp.h"
#include "Definitionator.h"
#include "Lex.h"
#include "ModFeatures.h"
#include "ModWords.h"
#include "Piece.h"
#include "Res.h"
#include "Solver.h"
#include "Undo.h"
#include "Buffer.h"
#include "Font.h"
#include "ImageFont.h"
#include "Graphics.h"
#include "Image.h"
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// The web page (games/bookworm/web/shell.html) shapes its stage to the game's screen; SDL sizes the canvas to it
EM_JS(void, bookworm_notify_screen, (int theWidth, int theHeight), {
	if (typeof Module['onGameScreen'] === 'function')
		Module['onGameScreen'](theWidth, theHeight);
});
#endif

using namespace Sexy;

// The board backdrop's frame spans x 225..640; the panel is its left and right 107 pixels side by side, so it
// has both corners and the bottom ornaments
static const int FRAME_SRC_X = 225;
static const int FRAME_SRC_W = 415;
static const int PANEL_HALF_W = SIDE_PANEL_WIDTH / 2;
static const int CONTENT_X = 16;
static const int CONTENT_RIGHT = SIDE_PANEL_WIDTH - 16;
static const int CONTENT_TOP = 18;
static const int CONTENT_BOTTOM = 436;  // above the frame's bottom ornaments
static const int ROW_H = 16;
static const int DIVIDER_H = 15;
static const int HEADER_H = 24;
static const int MIN_WORD_ROWS = 6;     // the words keep at least this many rows under a long solver list
static const int HINTS_TOP = CONTENT_TOP + HEADER_H;
static const int HINT_ROWS = (CONTENT_BOTTOM - HINTS_TOP - 6 - HEADER_H - MIN_WORD_ROWS * ROW_H) / ROW_H;

SidePanel::SidePanel(BookWormApp* theApp)
{
	mApp = theApp;
	mLogBoard = NULL;
	mLogBits = -1;
	mHoverHint = -1;
	mMouseX = -1;
	mMouseY = -1;
	mHoverDelay = 0;
	mDefBoard = 0;
	mMouseVisible = true;
	mHasAlpha = true;
}

bool SidePanel::Enabled()
{
	return ModFeatureEnabled("widescreen");
}

void SidePanel::TellPage(int theWidth, int theHeight)
{
#ifdef __EMSCRIPTEN__
	bookworm_notify_screen(theWidth, theHeight);
#else
	(void)theWidth;
	(void)theHeight;
#endif
}

Rect SidePanel::HintRect(int theIdx)
{
	return Rect(CONTENT_X - 4, HINTS_TOP + theIdx * ROW_H, CONTENT_RIGHT - CONTENT_X + 8, ROW_H);
}

int SidePanel::HintAt(int x, int y) const
{
	Board* aBoard = CurrentBoard();
	if (!HintsLive(aBoard))
		return -1;
	int aCount = std::min((int)aBoard->mSolver->mHints.size(), HINT_ROWS);
	for (int i = 0; i < aCount; i++)
	{
		if (HintRect(i).Contains(x, y))
			return i;
	}
	return -1;
}

Board* SidePanel::CurrentBoard() const
{
	Board* aBoard = mApp->mBoard;
	if ((aBoard == NULL) || (aBoard->mWidgetManager == NULL))
		return NULL;
	return aBoard;
}

// The solver's list stands for the board as it is: tiles at rest, in play, nothing over the board
bool SidePanel::HintsLive(Board* theBoard) const
{
	return (theBoard != NULL) && (theBoard->mSolver != NULL) && (Solver::BoardAtRest(theBoard)) &&
		(theBoard->mSolver->IsCurrent()) && (mApp->GetDialogCount() == 0);
}

// The words of the board's game log (Board::Submit: code 0xd the points, the length and the letters; 0xc the
// tile types of a word with special tiles; 0xb the bonus word's points; 0xe a level; 0 game over and the score)
void SidePanel::ReadLog(Board* theBoard)
{
	if ((theBoard == mLogBoard) && (theBoard->mGameLog.GetDataLenBits() == mLogBits))
		return;
	mLogBoard = theBoard;
	mLogBits = theBoard->mGameLog.GetDataLenBits();
	mWords.clear();

	Buffer aLog = theBoard->mGameLog;   // a copy: reading moves the read position
	aLog.SeekFront();
	aLog.ReadByte();                    // the log's version (3), the mode, the seed and the time
	aLog.ReadByte();
	aLog.ReadLong();
	aLog.ReadLong();
	int aLevel = 0;
	while (aLog.mReadBitPos + 4 <= aLog.mDataBitSize)
	{
		int aCode = aLog.ReadNumBits(4, false);
		if (aCode == 0xe)
			aLevel = aLog.ReadShort();
		else if (aCode == 0xd)
		{
			PlayedWord aWord;
			aWord.mScore = (int)aLog.ReadLong();
			int aLength = aLog.ReadNumBits(4, false);
			for (int i = 0; i < aLength; i++)
				aWord.mWord += (char)('A' + aLog.ReadNumBits(5, false));
			aWord.mBonus = 0;
			aWord.mLevel = aLevel;
			aWord.mTileType = TILE_NORMAL;
			mWords.push_back(aWord);
		}
		else if ((aCode == 0xc) && (!mWords.empty()))
		{
			// one type per tile: a Q tile spells "QU"
			PlayedWord& aWord = mWords.back();
			int aTiles = (int)aWord.mWord.size() - (int)std::count(aWord.mWord.begin(), aWord.mWord.end(), 'Q');
			static const int aRank[NUM_TILE_TYPES] = { 0, 2, 3, 1, 4, 5 };  // normal, green, gold, fire, sapphire, diamond
			for (int i = 0; i < aTiles; i++)
			{
				int aType = aLog.ReadNumBits(3, false);
				if ((aType < NUM_TILE_TYPES) && (aRank[aType] > aRank[aWord.mTileType]))
					aWord.mTileType = aType;
			}
		}
		else if ((aCode == 0xb) && (!mWords.empty()))
			mWords.back().mBonus = (int)aLog.ReadLong();
		else if (aCode == 0)
			aLog.ReadLong();
		else
			break;
	}
}

void SidePanel::Update()
{
	Widget::Update();
	Board* aBoard = CurrentBoard();
	if (aBoard != NULL)
	{
		ReadLog(aBoard);
		if ((aBoard->mSolver != NULL) && (Solver::BoardAtRest(aBoard)))
			aBoard->mSolver->Refresh();
	}
	if ((mHoverHint >= 0) && (!HintsLive(aBoard)))
		mHoverHint = -1;
	UpdateHoverDefinition(aBoard);
	MarkDirty();
}

SexyString SidePanel::WordAt(int x, int y) const
{
	int anIdx = HintAt(x, y);
	if (anIdx >= 0)
		return CurrentBoard()->mSolver->mHints[anIdx].mWord;
	int aWord = WordIndexAt(x, y);
	return (aWord >= 0) ? mWords[aWord].mWord : SexyString();
}

int SidePanel::WordIndexAt(int x, int y) const
{
	for (const std::pair<Rect, int>& aRow : mWordRows)
	{
		if ((aRow.first.Contains(x, y)) && (aRow.second < (int)mWords.size()))
			return aRow.second;
	}
	return -1;
}

// The word under the mouse, in play: after a moment its definition (or a mod's note on it) in Lex's balloon,
// tried again while another message is up
void SidePanel::UpdateHoverDefinition(Board* theBoard)
{
	SexyString aWord;
	if ((mMouseX >= 0) && (theBoard != NULL) && (theBoard->mLex != NULL) && (theBoard->mState == BOARD_PLAYING) &&
		(theBoard->mPauseCount <= 0) && (!theBoard->mSummaryShown) && (mApp->mDefinitionator != NULL) &&
		(mApp->GetDialogCount() == 0))
		aWord = WordAt(mMouseX, mMouseY);
	if (aWord != mHoverWord)
	{
		EndHoverDefinition(theBoard);
		mHoverWord = aWord;
		mHoverDelay = 15;
		return;
	}
	if ((aWord.empty()) || (mDefBoard != 0) || (mHoverDelay <= 0) || (--mHoverDelay > 0))
		return;

	SexyString aDefinition = mApp->mDefinitionator->GetDefinition(aWord);
	if (aDefinition.empty())
		aDefinition = ModWordNote(aWord);
	if (aDefinition.empty())
		return;
	if (theBoard->mLex->PreviewDefinition(aWord, aDefinition))
		mDefBoard = theBoard->mBoardSerial;
	else
		mHoverDelay = 10;
}

// The hovered word's definition goes; the selection's own (Definitions While Selecting) comes back after its delay
void SidePanel::EndHoverDefinition(Board* theBoard)
{
	if (mDefBoard == 0)
		return;
	if ((theBoard != NULL) && (theBoard->mBoardSerial == mDefBoard) && (theBoard->mLex != NULL))
	{
		theBoard->mLex->EndPreview(false);
		theBoard->mSelectDefWord.clear();
	}
	mDefBoard = 0;
}

static Color TileColor(int theType)
{
	switch (theType)
	{
	case TILE_GREEN:	return Color(120, 225, 110);
	case TILE_GOLD:		return Color(255, 205, 70);
	case TILE_FIRE:		return Color(255, 130, 70);
	case TILE_SAPPHIRE:	return Color(120, 170, 255);
	case TILE_DIAMOND:	return Color(215, 245, 255);
	default:			return Color(236, 226, 204);
	}
}

static void DrawHeader(Graphics* g, BookWormApp* theApp, const SexyString& theText, int theY)
{
	Font* aFont = theApp->mFonts[FONT_SCRIPPSCOLLEGEOS15BOLD];
	g->SetFont(aFont);
	g->SetColor(Color(255, 214, 110));
	g->DrawString(theText, (SIDE_PANEL_WIDTH - aFont->StringWidth(theText)) / 2, theY + aFont->GetAscent());
	g->SetColor(Color(255, 214, 110, 90));
	g->FillRect(CONTENT_X, theY + HEADER_H - 5, CONTENT_RIGHT - CONTENT_X, 1);
}

// A row: theLeft at the left, theRight right-aligned, in theFont; theLabel (small, before theLeft) when given
static void DrawRow(Graphics* g, Font* theFont, const SexyString& theLeft, const SexyString& theRight,
	const Color& theLeftColor, const Color& theRightColor, int theY, int theIndent)
{
	g->SetFont(theFont);
	int aBaseline = theY + (ROW_H + theFont->GetAscent()) / 2 - 1;
	int aRightWidth = theFont->StringWidth(theRight);
	SexyString aLeft = theLeft;
	int aRoom = CONTENT_RIGHT - aRightWidth - 6 - (CONTENT_X + theIndent);
	while ((aLeft.size() > 1) && (theFont->StringWidth(aLeft) > aRoom))
		aLeft.resize(aLeft.size() - 1);
	g->SetColor(theLeftColor);
	g->DrawString(aLeft, CONTENT_X + theIndent, aBaseline);
	g->SetColor(theRightColor);
	g->DrawString(theRight, CONTENT_RIGHT - aRightWidth, aBaseline);
}

// The solver's list from theY; the y below it
int SidePanel::DrawHints(Graphics* g, Board* theBoard, int theY)
{
	DrawHeader(g, mApp, _S("Solver"), theY);
	theY += HEADER_H;

	Solver* aSolver = theBoard->mSolver;
	bool isLive = HintsLive(theBoard);
	if ((!isLive) || (aSolver->mHints.empty()))
	{
		Font* aFont = mApp->mFonts[FONT_MYRIADWEB10];
		g->SetFont(aFont);
		g->SetColor(Color(200, 185, 160));
		SexyString aText = (!isLive) ? _S("(when the tiles rest)") : _S("No words on the board");
		g->DrawString(aText, (SIDE_PANEL_WIDTH - aFont->StringWidth(aText)) / 2, theY + 12);
		return theY + ROW_H + 6;
	}

	Font* aLabelFont = mApp->mFonts[FONT_MYRIADWEB10];
	Font* aFont = mApp->mFonts[FONT_MYRIADWEB12BOLD];
	const SolverWord* aShown = aSolver->ShownHint();
	int aCount = std::min((int)aSolver->mHints.size(), HINT_ROWS);
	for (int i = 0; i < aCount; i++)
	{
		const SolverWord& aHint = aSolver->mHints[i];
		Rect aRect = HintRect(i);
		if (aShown == &aHint)
		{
			g->SetColor(Color(255, 214, 110, 70));
			g->FillRect(aRect);
		}
		else if (i == mHoverHint)
		{
			g->SetColor(Color(255, 255, 255, 40));
			g->FillRect(aRect);
		}

		const char* aKind = "BEST";
		Color aKindColor(200, 185, 160);
		if (aHint.mKind == SOLVER_HINT_FIRE)
		{
			aKind = "FIRE";
			aKindColor = Color(255, 130, 70);
		}
		else if (aHint.mKind == SOLVER_HINT_BONUS)
		{
			aKind = "BONUS";
			aKindColor = Color(255, 214, 110);
		}
		else if (aHint.mKind == SOLVER_HINT_LONGEST)
		{
			aKind = "LONG";
			aKindColor = Color(120, 170, 255);
		}
		else if (aHint.mKind == SOLVER_HINT_CASHIN)
		{
			aKind = "CASH";
			aKindColor = Color(120, 225, 110);
		}
		g->SetFont(aLabelFont);
		if ((i < 10) && (SolverHotkeys()))
		{
			g->SetColor(Color(150, 140, 120));   // the number key that plays it
			g->DrawString(StrFormat(_S("%d"), (i + 1) % 10), 5, theY + (ROW_H + aLabelFont->GetAscent()) / 2 - 1);
		}
		g->SetColor(aKindColor);
		g->DrawString(aKind, CONTENT_X, theY + (ROW_H + aLabelFont->GetAscent()) / 2 - 1);

		// the word in the colour of its best bonus tile (orange and "!" when it is not safe)
		static const int aRank[NUM_TILE_TYPES] = { 0, 1, 2, 0, 3, 4 };  // normal, green, gold, fire, sapphire, diamond
		int aBestType = TILE_NORMAL;
		for (Piece* aPiece : aHint.mPath)
		{
			if ((aPiece->mType < NUM_TILE_TYPES) && (aRank[aPiece->mType] > aRank[aBestType]))
				aBestType = aPiece->mType;
		}
		bool isSafe = (aHint.mSafety == SOLVER_SAFE);
		SexyString aWord = aHint.mWord;
		if (!isSafe)
			aWord += _S(" !");
		DrawRow(g, aFont, aWord, StrFormat(_S("%d"), aHint.mScore), isSafe ? ((aBestType == TILE_NORMAL) ?
			Color(255, 255, 255) : TileColor(aBestType)) : Color(255, 140, 100),
			Color(255, 214, 110), theY, 40);
		theY += ROW_H;
	}
	return theY + 6;
}

// The words played, newest first, with a line for each level
void SidePanel::DrawWords(Graphics* g, int theY)
{
	DrawHeader(g, mApp, _S("Words Played"), theY);
	theY += HEADER_H;
	if (mWords.empty())
		return;

	Font* aFont = mApp->mFonts[FONT_MYRIADWEB12BOLD];
	Font* aSmallFont = mApp->mFonts[FONT_MYRIADWEB10];
	int aLevel = -1;
	for (int i = (int)mWords.size() - 1; i >= 0; i--)
	{
		const PlayedWord& aWord = mWords[i];
		if (aWord.mLevel != aLevel)
		{
			if (theY + DIVIDER_H + ROW_H > CONTENT_BOTTOM)
				break;
			aLevel = aWord.mLevel;
			g->SetFont(aSmallFont);
			g->SetColor(Color(200, 185, 160));
			SexyString aText = StrFormat(_S("Level %d"), aLevel + 1);
			int aTextW = aSmallFont->StringWidth(aText);
			int aMidY = theY + DIVIDER_H / 2;
			g->DrawString(aText, (SIDE_PANEL_WIDTH - aTextW) / 2, aMidY + aSmallFont->GetAscent() / 2);
			g->SetColor(Color(200, 185, 160, 80));
			g->FillRect(CONTENT_X, aMidY, (SIDE_PANEL_WIDTH - aTextW) / 2 - 6 - CONTENT_X, 1);
			g->FillRect((SIDE_PANEL_WIDTH + aTextW) / 2 + 6, aMidY, CONTENT_RIGHT - (SIDE_PANEL_WIDTH + aTextW) / 2 - 6, 1);
			theY += DIVIDER_H;
		}
		if (theY + ROW_H > CONTENT_BOTTOM)
			break;
		Rect aRect(CONTENT_X - 4, theY, CONTENT_RIGHT - CONTENT_X + 8, ROW_H);
		mWordRows.push_back(std::make_pair(aRect, i));
		if ((mMouseX >= 0) && (aRect.Contains(mMouseX, mMouseY)))
		{
			g->SetColor(Color(255, 255, 255, 40));
			g->FillRect(aRect);
		}
		SexyString aPoints = (aWord.mBonus > 0) ? StrFormat(_S("%d +%d"), aWord.mScore, aWord.mBonus) :
			StrFormat(_S("%d"), aWord.mScore);
		DrawRow(g, aFont, aWord.mWord, aPoints, TileColor(aWord.mTileType), Color(255, 214, 110), theY, 0);
		theY += ROW_H;
	}
}

void SidePanel::Draw(Graphics* g)
{
	Image* aBackdrop = mApp->mImages[IMAGE_BACKDROP_BOARD];
	if (aBackdrop != NULL)
	{
		g->DrawImage(aBackdrop, 0, 0, Rect(FRAME_SRC_X, 0, PANEL_HALF_W, mHeight));
		g->DrawImage(aBackdrop, PANEL_HALF_W, 0, Rect(FRAME_SRC_X + FRAME_SRC_W - PANEL_HALF_W, 0, PANEL_HALF_W, mHeight));
	}
	else
	{
		g->SetColor(Color(36, 30, 25));
		g->FillRect(0, 0, mWidth, mHeight);
	}

	mWordRows.clear();
	Board* aBoard = CurrentBoard();
	if (aBoard == NULL)
		return;
	int aY = CONTENT_TOP;
	if (aBoard->mSolver != NULL)
		aY = DrawHints(g, aBoard, aY);
	DrawWords(g, aY);
}

void SidePanel::MouseMove(int x, int y)
{
	mHoverHint = HintAt(x, y);
	mMouseX = x;
	mMouseY = y;
}

void SidePanel::MouseLeave()
{
	mHoverHint = -1;
	mMouseX = -1;
	mMouseY = -1;
}

// A click on a word of the solver's list selects it, as its hint would (Tab then goes on from it); a click on the
// one selected plays it, as SUBMIT does
void SidePanel::MouseDown(int x, int y, int theClickCount)
{
	Board* aBoard = CurrentBoard();
	if ((aBoard == NULL) || (aBoard->IsWatched()))
		return;     // a watched replay plays only its recording (Replay.h)
	// a double click on a word played: back to just after it, the later moves undone (mod "undo")
	int aWord = (theClickCount == 2) ? WordIndexAt(x, y) : -1;
	if ((aWord >= 0) && (aBoard->mUndo != NULL))
	{
		aBoard->mUndo->RequestUndoTo(aWord + 1);
		return;
	}
	int anIdx = (theClickCount > 0) ? HintAt(x, y) : -1;
	if (anIdx < 0)
		return;
	Solver* aSolver = aBoard->mSolver;
	if (aSolver->ShownHint() == &aSolver->mHints[anIdx])
		aBoard->ButtonDepress(BOARD_BUTTON_SUBMIT);
	else
		aSolver->SelectListed(anIdx);
}
