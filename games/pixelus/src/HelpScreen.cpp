#include "HelpScreen.h"
#include "PixelusApp.h"
#include "UserDirectory.h"
#include "Grid.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "Buffer.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "WidgetManager.h"
#include <algorithm>

using namespace Sexy;

// 0x40e1a0: the check box starts as the opposite of the profile's flag; the button is the screen's way out
HelpScreen::HelpScreen(PixelusApp* theApp) :
	mApp(theApp)
{
	// the original shows a small grid beside the slides (0x40ddd0: a grid of 12 by 12 with one tile in it); the port draws
	// its picture from the slides' own grid, but the grid and the tile take two draws of the game's random numbers, which
	// the draws after them depend on
	mApp->RandNext();
	mApp->RandNext();
	mGrid = new Grid(16, 16);
	Buffer aBuffer;
	if (mApp->ReadBufferFromFile("levels/helpscreen.txt", &aBuffer))
	{
		std::string aText((const char*) aBuffer.GetDataPtr(), aBuffer.GetDataLen());
		mGrid->LoadFromText(aText);
	}
	for (int r = 0; r < mGrid->mRows; r++)
		for (int c = 0; c < mGrid->mCols; c++)
			if (mGrid->mCells[r][c].mWanted)
				mOrder.push_back(r * Grid::MAX_SIZE + c);
	int aLast = FIRST_SLIDE + SLIDE_GAP * (int) mOrder.size() + SLIDE_UPDATES;
	mCycle = aLast + HOLD_UPDATES + FADE_UPDATES;
	mTimer = 0;
	mDemoFrame = 0;

	mDontShowBox = new Checkbox(mApp->mImages[IMAGE_CHECKBOX_UNCHECKED], mApp->mImages[IMAGE_CHECKBOX_CHECKED], 0, this);
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	mDontShowBox->SetChecked(aUser == NULL || !aUser->mFlag2D, false);

	mOKButton = new SoundButton(mApp, mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], ID_OK, this);
	mOKButton->mLabel = GT(0x50b558);
	StyleButton(mApp, mOKButton, 3);
}

HelpScreen::~HelpScreen()
{
	delete mGrid;
	delete mOKButton;
	delete mDontShowBox;
}

// 0x40dc60 / 0x40dce0: the profile keeps the opposite of the box when the screen goes
void HelpScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mOKButton);
	theWidgetManager->AddWidget(mDontShowBox);
}

void HelpScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mOKButton);
	theWidgetManager->RemoveWidget(mDontShowBox);
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	if (aUser != NULL)
		aUser->mFlag2D = !mDontShowBox->mChecked;
}

// 0x40db80
void HelpScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aButtonHeight = mOKButton->mHeight;
	mOKButton->Resize(mX + mWidth - 0xa5, mY + mHeight - aButtonHeight - 5, 0x96, aButtonHeight);
	Image* aBox = mApp->mImages[IMAGE_CHECKBOX_CHECKED];
	mDontShowBox->Resize(mX + 0x14, mOKButton->mY + 7, aBox->GetWidth(), aBox->GetHeight());
}

void HelpScreen::Update()
{
	Widget::Update();
	if (++mTimer >= mCycle)
		mTimer = 0;
	// the demo throws a tile (a draw) at 50, 150 and 250 of its 950 updates (0x40d483 in 0x40d320)
	mDemoFrame = (mDemoFrame + 1) % 950;
	if (mDemoFrame == 50 || mDemoFrame == 150 || mDemoFrame == 250)
		mApp->RandNext();
	// the demo's tile moves play the move sample; a thrown tile lands 11 updates later (0x40d320, 0x424750)
	if (mDemoFrame == 50 || mDemoFrame == 150 || mDemoFrame == 250 || mDemoFrame == 500 || mDemoFrame == 600 || mDemoFrame == 700)
		mApp->PlaySample(SOUND_TILE_MOVING_02D);
	if (mDemoFrame == 61 || mDemoFrame == 161 || mDemoFrame == 261)
		mApp->PlaySample(SOUND_TILE_TARGET_02C);
	MarkDirty();
}

void HelpScreen::DrawDark(Graphics* g, int theX, int theWidth, int theAlpha)
{
	if (theAlpha <= 0)
		return;
	g->SetColor(Color(0, 0, 0, std::min(theAlpha, 200)));
	g->FillRect(theX, WINDOW_Y, theWidth, WINDOW_H);
}

// 0x40e360: the picture behind the frame, the fades, the frame, and the box's text
void HelpScreen::Draw(Graphics* g)
{
	// the cells of the puzzle (its walls and the places for the tiles), and how big they are in the two windows
	int aMinRow = 99, aMaxRow = -1, aMinCol = 99, aMaxCol = -1;
	for (int r = 0; r < mGrid->mRows; r++)
		for (int c = 0; c < mGrid->mCols; c++)
			if (mGrid->mCells[r][c].mWanted || mGrid->mCells[r][c].mFixed)
			{
				aMinRow = std::min(aMinRow, r);
				aMaxRow = std::max(aMaxRow, r);
				aMinCol = std::min(aMinCol, c);
				aMaxCol = std::max(aMaxCol, c);
			}
	int aRows = aMaxRow - aMinRow + 1, aCols = aMaxCol - aMinCol + 1;
	const int aSmall = 13;
	int aLeftX = BOARD_X + (LEFT_X + LEFT_W - BOARD_X - aCols * aSmall) / 2;
	int aLeftY = WINDOW_Y + (LEFT_VISIBLE_H - aRows * aSmall) / 2;
	const int aBig = Grid::CELL_SIZE;	// the right window shows the cells at their own size (a stretched tile loses its colour)
	int aRightX = RIGHT_X + (RIGHT_W - aCols * aBig) / 2;
	int aRightY = WINDOW_Y + (WINDOW_H - aRows * aBig) / 2;

	g->DrawImage(mApp->mImages[IMAGE_HELPBACK], LEFT_X, WINDOW_Y);
	g->SetClipRect(BOARD_X, WINDOW_Y, LEFT_X + LEFT_W - BOARD_X, LEFT_VISIBLE_H);
	for (int r = aMinRow; r <= aMaxRow; r++)
		for (int c = aMinCol; c <= aMaxCol; c++)
		{
			const Grid::Cell& aCell = mGrid->mCells[r][c];
			int aLeft = aLeftX + (c - aMinCol) * aSmall, aTop = aLeftY + (r - aMinRow) * aSmall;
			int aRight = aRightX + (c - aMinCol) * aBig, aBottom = aRightY + (r - aMinRow) * aBig;
			if (aCell.mWanted)
			{
				g->DrawImage(mApp->mImages[IMAGE_TILEBACK_WANTTILE_20X20], aLeft, aTop, aSmall, aSmall);
				g->ClearClipRect();
				g->DrawImage(mApp->mImages[IMAGE_TILEBACK_WANTTILE_20X20], aRight, aBottom);
				g->SetClipRect(BOARD_X, WINDOW_Y, LEFT_X + LEFT_W - BOARD_X, LEFT_VISIBLE_H);
			}
			else if (aCell.mFixed)
			{
				g->DrawImage(mApp->mImages[IMAGE_REDX_20X20], aLeft, aTop, aSmall, aSmall);
				g->ClearClipRect();
				g->DrawImage(mApp->mImages[IMAGE_REDX_20X20], aRight, aBottom);
				g->SetClipRect(BOARD_X, WINDOW_Y, LEFT_X + LEFT_W - BOARD_X, LEFT_VISIBLE_H);
			}
		}

	// the tiles come one after the other from the rack on the left, and each that lands is on the picture too
	for (size_t i = 0; i < mOrder.size(); i++)
	{
		int aRow = mOrder[i] / Grid::MAX_SIZE - aMinRow, aCol = mOrder[i] % Grid::MAX_SIZE - aMinCol;
		double aProgress = std::max(0.0, std::min(1.0, (mTimer - (FIRST_SLIDE + SLIDE_GAP * (int) i)) / (double) SLIDE_UPDATES));
		if (aProgress <= 0)
			continue;
		double anEase = 1 - (1 - aProgress) * (1 - aProgress);
		int aTargetX = aLeftX + aCol * aSmall;
		int aX = (int) (BOARD_X - aSmall + (aTargetX - (BOARD_X - aSmall)) * anEase);
		Image* aTile = mApp->mImages[IMAGE_TILEBLUE01 + (int) (i % 5)];
		g->DrawImage(aTile, aX, aLeftY + aRow * aSmall, aSmall, aSmall);

		if (aProgress >= 1)
		{
			g->ClearClipRect();
			g->DrawImage(aTile, aRightX + aCol * aBig, aRightY + aRow * aBig);
			g->SetClipRect(BOARD_X, WINDOW_Y, LEFT_X + LEFT_W - BOARD_X, LEFT_VISIBLE_H);
		}
	}
	g->ClearClipRect();

	// the fades: in at the start, out at the end
	int aFadeOutStart = mCycle - FADE_UPDATES;
	int anAlpha = 0;
	if (mTimer < FADE_UPDATES)
		anAlpha = 200 - (int) (mTimer * 1.5);
	else if (mTimer >= aFadeOutStart)
		anAlpha = (int) ((mTimer - aFadeOutStart) * 1.5);
	g->SetColorizeImages(true);
	DrawDark(g, LEFT_X, LEFT_W, anAlpha);
	DrawDark(g, RIGHT_X, RIGHT_W, anAlpha);
	g->SetColorizeImages(false);

	g->DrawImage(mApp->mImages[IMAGE_HELPFRONT], 0, 0);
	g->SetFont(mApp->mFonts[FONT_GARAMOND14]);
	g->SetColor(Color(255, 255, 255, 255));
	g->DrawString(GT(0x50d118), mDontShowBox->mX - mX + 0x20, mDontShowBox->mY - mY + 0x14);
}

// 0x40ddc0
void HelpScreen::ButtonDepress(int theId)
{
	if (theId == ID_OK)
		mApp->CloseHelp();
}
