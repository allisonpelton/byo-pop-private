#include "HelpScreen.h"
#include "PixelusApp.h"
#include "UserDirectory.h"
#include "Grid.h"
#include "Board.h"
#include "TileMaster.h"
#include "Res.h"
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

// 0x40e1a0: the left board (0x40ddd0) is a 12 by 12 grid with places for two tiles in a row (and a third out of the way),
// a block at the end of the row, the Tile Master at the left of it, and no wave of light; the right board (0x40df90) is the
// puzzle of levels/helpscreen.txt with its best solution played by the grid
// 0x40da40
HelpScreen::HelpScreen(PixelusApp* theApp) :
	mApp(theApp)
{
	mLeft = new Board(mApp, 12, 12, 100, -40);
	Grid* aLeftGrid = mLeft->mGrid;
	aLeftGrid->mCells[6][4].mWanted = true;		// 0x424410 with 4: a place for a tile
	aLeftGrid->mCells[6][6].mWanted = true;
	aLeftGrid->mCells[6][7].mFixed = true;		// with 1: a block
	aLeftGrid->mCells[6][7].mObject = aLeftGrid->NewObject(6, 7);
	aLeftGrid->mCells[6][7].mObject->mFixed = true;
	aLeftGrid->mCells[0][0].mWanted = true;
	aLeftGrid->SetCursor(Grid::SIDE_RIGHT, 6);	// 0x424590 (6, 1)
	aLeftGrid->Recount();
	aLeftGrid->mSupply = aLeftGrid->mEmptyWanted;	// 0x421b20
	mLeft->mTileMaster = new TileMaster(mApp, 100, -40, 12, 12);
	mLeft->mTileMaster->SetTarget(Grid::SIDE_RIGHT, 6, true);
	mLeftMode = MODE_RUN;
	mLeftDark = 200.0;
	mPointerFrom = mPointerTo = 220.0;
	mPointerWalk = 0.0;
	mPointerStep = 0.015;
	mGlow = 0.0;
	mLeftFrame = 0;

	mRight = new Board(mApp, 16, 16, 5, -43);
	Buffer aBuffer;
	if (mApp->ReadBufferFromFile("levels/helpscreen.txt", &aBuffer))
		mPuzzleText.assign((const char*) aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	mRight->LoadDemoPuzzle(mPuzzleText);
	mRight->StartReplay(25, 100);					// 0x426560 (the first move comes after 25 updates), 0x422460
	mRightMode = MODE_IDLE;
	mRightDark = 200.0;
	mPictureAlpha = 0;
	mPicturePhase = 0;
	mHold = 0;

	mDontShowBox = new Checkbox(mApp->mImages[IMAGE_CHECKBOX_UNCHECKED], mApp->mImages[IMAGE_CHECKBOX_CHECKED], 0, this);
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	mDontShowBox->SetChecked(aUser == NULL || !aUser->mShowHelp, false);

	mOKButton = new SoundButton(mApp, mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], ID_OK, this);
	mOKButton->mLabel = GT(0x50b558);
	StyleButton(mApp, mOKButton, 3);
}

// port: the destructor (HelpScreen::~HelpScreen)
HelpScreen::~HelpScreen()
{
	delete mLeft;
	delete mRight;
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
		aUser->mShowHelp = !mDontShowBox->mChecked;
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

// 0x40d2d0: the pointer goes from one column of the grid to another (the grid starts at 100 and a cell is 20 wide)
void HelpScreen::MovePointer(int theFromCol, int theToCol)
{
	mPointerFrom = (theFromCol * 5 + 25) * 4;
	mPointerTo = (theToCol * 5 + 25) * 4;
	mPointerWalk = 0.0;
	mPointerStep = 0.015;
}

// 0x40d320: the left window's turn: the script of 950 updates
void HelpScreen::UpdateLeft()
{
	if (mLeftMode != MODE_RUN)
	{
		// under the dark layer
		if (mLeftDark != 200.0)
			mLeftDark = std::min(200.0, mLeftDark + 1.5);
		return;
	}

	mLeftDark = std::max(0.0, mLeftDark - 1.5);
	mLeftFrame++;
	mPointerWalk += mPointerStep;
	if (mPointerWalk >= 1.0 || mPointerWalk <= 0.0)
		mPointerStep = 0.0;
	mGlow = std::max(0.0, mGlow - 0.03);

	switch (mLeftFrame % 950)
	{
	case 1:
	case 650: mLeft->SetHover(6, 6, true); break;
	case 50:
	case 150:
	case 250: mLeft->DoThrow(); mGlow = 1.0; break;
	case 75: MovePointer(6, 5); break;
	case 100:
	case 550: mLeft->SetHover(5, 6, true); break;
	case 175: MovePointer(5, 4); break;
	case 200: mLeft->SetHover(4, 6, true); break;
	case 500:
	case 600:
	case 700: mLeft->DoPull(); mGlow = 1.0; break;
	case 790: mLeft->mTileMaster->IdleSoon(); break;	// 0x40d507
	case 525: MovePointer(4, 5); break;
	case 625: MovePointer(5, 6); break;
	case 949: mLeftMode = MODE_DONE; break;
	}
	mLeft->UpdateDemo();
}

// 0x40d840: the board plays its solution again
void HelpScreen::RestartRight()
{
	mRight->StartReplay(25, 100);
	mPictureAlpha = 0;
	mPicturePhase = 0;
}

// 0x40d8d0: the right window's turn: the solution is played, the finished picture stays 100 updates, then a picture covers
// the board, the puzzle is loaded again and the picture goes
void HelpScreen::UpdateRight()
{
	if (mRightMode != MODE_RUN)
	{
		if (mRightDark != 200.0)
			mRightDark = std::min(200.0, mRightDark + 1.5);
		return;
	}

	mRightDark = std::max(0.0, mRightDark - 1.0);
	if (mRightDark != 0.0)
		return;

	mRight->UpdateDemo();
	if (mHold > 0 && --mHold < 1)
	{
		mPicturePhase = 2;
		mPictureAlpha = 0;
	}
	if (mPicturePhase == 2)
	{
		mPictureAlpha += 2;
		if (mPictureAlpha > 254)
		{
			mRight->LoadDemoPuzzle(mPuzzleText);
			mPicturePhase = 1;
			mPictureAlpha = 255;
		}
	}
	else if (mPicturePhase == 1 && (mPictureAlpha -= 2) < 1)
	{
		mRightMode = MODE_DONE;
		RestartRight();
	}
	if (mRight->mGrid->mDone && !mRight->mGrid->mTileMoving && mPicturePhase != 2 && mPictureAlpha != 255 && mHold < 1)
	{
		mHold = 100;
		mPicturePhase = 0;
		mPictureAlpha = 0;
	}
}

// 0x40dd50: the two windows take turns (each window's own update runs first)
void HelpScreen::Update()
{
	Widget::Update();
	UpdateLeft();
	UpdateRight();
	if (mLeftMode == MODE_DONE)
	{
		mLeftMode = MODE_IDLE;
		mRightMode = MODE_RUN;
	}
	if (mRightMode == MODE_DONE)
	{
		mLeftMode = MODE_RUN;
		mRightMode = MODE_IDLE;
	}
	MarkDirty();
}

// port: a helper of the port (HelpScreen::DrawDark)
void HelpScreen::DrawDark(Graphics* g, int theX, int theWidth, double theDark)
{
	if (theDark <= 0.0)
		return;
	g->SetColorizeImages(true);
	g->SetColor(Color(0, 0, 0, theDark <= 200.0 ? (int) theDark : 200));
	g->FillRect(theX, WINDOW_Y, theWidth, WINDOW_H);
	g->SetColorizeImages(false);
}

// 0x40e360: the picture behind the windows, the boards in them, the dark layers, the frame, and the box's text
// 0x40daa0
void HelpScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_HELPBACK], LEFT_X, WINDOW_Y);

	// the left window (0x40d740): the board, the Tile Master and the pointer, which flashes when a tile is thrown or pulled
	g->SetClipRect(0, WIDGET_Y, LEFT_W, WINDOW_H);
	g->Translate(0, WIDGET_Y);
	mLeft->DrawDemo(g);
	float aPointerX = (float) ((mPointerTo - mPointerFrom) * mPointerWalk + mPointerFrom - 14.0);
	g->DrawImageF(mApp->mImages[IMAGE_CURSOR_POINTER], aPointerX, 70.0f);
	if (mGlow > 0.0)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) (mGlow * 255.0)));
		g->DrawImageF(mApp->mImages[IMAGE_CURSOR_POINTER], aPointerX, 70.0f);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}
	g->Translate(0, -WIDGET_Y);
	g->ClearClipRect();

	// the right window (0x40e0b0): the board, and the picture over it while the puzzle is loaded again
	if (mPictureAlpha != 255)
	{
		g->SetClipRect(RIGHT_X, WIDGET_Y, RIGHT_W, WINDOW_H);
		g->Translate(RIGHT_X, WIDGET_Y);
		mRight->DrawDemo(g);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, std::max(0, mPictureAlpha)));
		g->DrawImageF(mApp->mImages[IMAGE_HELPBACK], -7.0f, -4.0f, Rect(0x130, 0, 0x128, 0x10c));
		g->SetColorizeImages(false);
		g->Translate(-RIGHT_X, -WIDGET_Y);
		g->ClearClipRect();
	}

	DrawDark(g, LEFT_X, LEFT_W, mLeftDark);
	DrawDark(g, RIGHT_X, RIGHT_W, mRightDark);

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
