#include "Help.h"
#include "SevenSeasApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Res.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x40a3f0: a cell's first pixel
static int Cell(int theCell)
{
	return theCell * 0x2a;
}

// the whirlpools in the four corners (0x493550)
static const int kWhirls[4][2] = {{0, 0}, {0, 10}, {10, 0}, {10, 10}};

// the words: their place (in the draw's translated frame) and text
struct HelpLine
{
	int					mX;
	int					mY;
	unsigned int		mText;
};

static const HelpLine kLines[] =
{
	{0x87, 0x41, 0x004a355c}, {0x87, 0x52, 0x004a3540}, {0x87, 0x63, 0x004a3524}, {0x87, 0x74, 0x004a350c},
	{0x161, 0x37, 0x004a3500}, {0x161, 0x48, 0x004a34f0}, {0x161, 0x59, 0x004a34e0}, {0x161, 0x6a, 0x004a34d4},
	{0x161, 0x7b, 0x004a34c4}, {0x161, 0x8c, 0x004a34b4},
	{0x140, 0xdc, 0x004a34a4}, {0x140, 0xed, 0x004a3490}, {0x140, 0xfe, 0x004a347c},
	{0x13d, 0x14a, 0x004a3468}, {0x13d, 0x15b, 0x004a3450}, {0x13d, 0x16c, 0x004a343c}, {0x13d, 0x17d, 0x004a3424},
	{0x13d, 0x18e, 0x004a340c},
	{0xa8, 0x136, 0x004a33f8}, {0xa8, 0x147, 0x004a33e0}, {0xa8, 0x158, 0x004a33c8}, {0xa8, 0x169, 0x004a33b4},
	{0xa8, 0x17a, 0x004a33a0}, {0xa8, 0x18b, 0x004a3394},
	{0x12, 0x12c, 0x004a3384}, {0x12, 0x13d, 0x004a3370}, {0x12, 0x14e, 0x004a335c}, {0x12, 0x15f, 0x004a334c},
	{0x12, 0x170, 0x004a3338}, {0x12, 0x181, 0x004a3324},
	{0x12, 0x91, 0x004a3310}, {0x12, 0xa2, 0x004a32f8}, {0x12, 0xb3, 0x004a32e0}, {0x12, 0xc4, 0x004a32c8}
};

// 0x40d330
HelpWidget::HelpWidget(SevenSeasApp* theApp) :
	mApp(theApp),
	mScrollX(0.0),
	mScrollY(0.0),
	mSpeedX(0.3),
	mSpeedY(0.3)
{
	ButtonWidget** aButtons[2] = {&mTutorialButton, &mReturnButton};
	unsigned int aLabels[2] = {0x004a32a0, 0x004a3290};
	for (int i = 0; i < 2; i++)
	{
		ButtonWidget* aButton = new ButtonWidget(i, this);
		aButton->mDoFinger = true;
		aButton->SetFont(mApp->mFonts[FONT_TUT]);
		aButton->mLabel = GT(aLabels[i]);
		aButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xe4, 0x37));
		// only COLOR_LABEL is set; the hovered and pressed label keeps ButtonWidget's default, which is black (the pressed
		// button is the all-black DBUTTONBLACK in the original, the same as the plate here)
		aButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(0, 0, 0));
		// the button is a black plate (the original's picture for it is all black)
		aButton->SetColor(ButtonWidget::COLOR_DARK_OUTLINE, Color(0, 0, 0));
		aButton->SetColor(ButtonWidget::COLOR_LIGHT_OUTLINE, Color(0, 0, 0));
		aButton->SetColor(ButtonWidget::COLOR_MEDIUM_OUTLINE, Color(0, 0, 0));
		aButton->SetColor(ButtonWidget::COLOR_BKG, Color(0, 0, 0));
		aButton->mMouseInsets = Insets(-1, -1, -1, -1);
		*aButtons[i] = aButton;
	}
}

// 0x40d760
HelpWidget::~HelpWidget()
{
	delete mTutorialButton;
	delete mReturnButton;
}

// 0x40d7d0
void HelpWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mTutorialButton);
	theWidgetManager->AddWidget(mReturnButton);
}

// 0x40d800
void HelpWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mTutorialButton);
	theWidgetManager->RemoveWidget(mReturnButton);
}

// 0x40f6a0
void HelpWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mTutorialButton->Resize(mX + 0x37, mHeight - 0x3c, 0x96, 0x26);
	mReturnButton->Resize(mWidth - 0xd7 + mX, mHeight - 0x3c, 0x96, 0x26);
}

// 0x40d9d0 (and 0x40d840 for the water, which drifts as the demo's does)
void HelpWidget::Update()
{
	Widget::Update();
	if (mApp->mTalking)
	{
		TalkWidget* aTalk = mApp->mTalkWidget;
		if ((aTalk->mMode == 1) && (aTalk->mProgress == 1.0))
			aTalk->mMode = 0;
		if ((aTalk->mMode == -1) && (aTalk->mProgress == 0.0))
		{
			aTalk->mMode = 0;
			mApp->mTalking = false;
		}
	}

	int aRand = CrtRand();
	if (aRand % 3 == 0)
		mSpeedX = mSpeedX + 0.04;
	else if (aRand % 3 == 1)
		mSpeedX = mSpeedX - 0.04;
	aRand = CrtRand();
	if (aRand % 3 == 0)
		mSpeedY = mSpeedY + 0.01;
	else if (aRand % 3 == 1)
		mSpeedY = mSpeedY - 0.01;
	if (0.125 < mSpeedX)
		mSpeedX = 0.125;
	else if (mSpeedX < 0.1)
		mSpeedX = 0.1;
	if (0.125 < mSpeedY)
		mSpeedY = 0.125;
	else if (mSpeedY < 0.1)
		mSpeedY = 0.1;
	mScrollX = mScrollX + mSpeedX;
	if (mScrollX < 0.0)
		mScrollX = mScrollX + 500.0;
	if (500.0 <= mScrollX)
		mScrollX = mScrollX - 500.0;
	mScrollY = mScrollY + mSpeedY;
	if (mScrollY < 0.0)
		mScrollY = mScrollY + 500.0;
	if (500.0 <= mScrollY)
		mScrollY = mScrollY - 500.0;
	MarkDirty();
}

// 0x40f370: the arrow on a cell next to the ship's (5, 5); on the ship's own cell, a red cross three cells to either side
void HelpWidget::DrawCellArrow(Graphics* g, int theX, int theY)
{
	Graphics g2(*g);
	g2.ClipRect(Cell(theX), Cell(theY), 0x2a, 0x2a);
	int aDX = theX - 5;
	int aDY = theY - 5;
	if ((aDX == 0) && (aDY == 0))
	{
		for (int anOffset = -0x7e; anOffset < 0xfc; anOffset += 0xfc)
		{
			Graphics g3(*g);
			g3.SetColorizeImages(true);
			g3.SetColor(Color(0xff, 0x37, 0));
			g3.ClipRect(Cell(theX) + anOffset, Cell(theY), 0x2a, 0x2a);
			g3.DrawImage(mApp->mImages[IMAGE_CROSSHAIR], Cell(theX) + 8 + anOffset, Cell(theY) + 8);
			g3.SetColorizeImages(false);
		}
		return;
	}
	// the arrow's picture and its nudge, by the direction from the ship
	int anImage = -1;
	int aNudgeX = 0;
	int aNudgeY = 0;
	if (aDX == -1)
	{
		if (aDY == -1) { anImage = 0; aNudgeX = 4; aNudgeY = 4; }
		else if (aDY == 0) { anImage = 1; aNudgeX = 0; aNudgeY = 4; }
		else if (aDY == 1) { anImage = 2; aNudgeX = 4; aNudgeY = -4; }
	}
	else if (aDX == 0)
	{
		if (aDY == -1) { anImage = 3; aNudgeX = 4; aNudgeY = 0; }
		else if (aDY == 1) { anImage = 5; aNudgeX = 4; aNudgeY = -4; }
	}
	else if (aDX == 1)
	{
		if (aDY == -1) { anImage = 6; aNudgeX = 0; aNudgeY = 4; }
		else if (aDY == 0) { anImage = 7; aNudgeX = -4; aNudgeY = 4; }
		else if (aDY == 1) { anImage = 8; aNudgeX = 0; aNudgeY = -4; }
	}
	if (anImage >= 0)
		g2.DrawImage(mApp->mArrows[anImage], Cell(theX) + aNudgeX, Cell(theY) + aNudgeY);
}

// 0x40da40
void HelpWidget::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	int aScrollX = (int) mScrollX;
	int aScrollY = (int) mScrollY;
	g->DrawImage(anImages[IMAGE_WATER], aScrollX, aScrollY);
	g->DrawImage(anImages[IMAGE_WATER], aScrollX - 500, aScrollY);
	g->DrawImage(anImages[IMAGE_WATER], aScrollX, aScrollY - 500);
	g->DrawImage(anImages[IMAGE_WATER], aScrollX - 500, aScrollY - 500);
	g->DrawImage(anImages[IMAGE_BACKDROP], 0, 0, Rect(0xad, 0, 0x1d3, 0x1e0));
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawImage(anImages[IMAGE_GRID], 0, 0);
	g->Translate(0, 0xc);

	for (int i = 0; i < 4; i++)
	{
		Graphics gWhirl(*g);
		gWhirl.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		gWhirl.ClipRect(Cell(kWhirls[i][0]), Cell(kWhirls[i][1]), 0x28, 0x28);
		gWhirl.DrawImage(anImages[IMAGE_WHIRL], Cell(kWhirls[i][0]), Cell(kWhirls[i][1]) + ((mUpdateCnt + i * 2) / 4 % 0x10) * -0x28);
		gWhirl.SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	// the player's ship and the others, each on its heading (the frame of the sheet) and the first sail frame
	struct Piece { int mImage; int mCellX; int mCellY; int mFrame; };
	static const Piece kPieces[] =
	{
		{IMAGE_GOODSHIP, 5, 5, 8}, {IMAGE_PIRATE, 1, 2, 4}, {IMAGE_REDPIRATE, 1, 6, 4}, {IMAGE_MONSTER, 9, 4, 4}
	};
	for (int i = 0; i < 4; i++)
	{
		Graphics gShip(*g);
		gShip.ClipRect(Cell(kPieces[i].mCellX), Cell(kPieces[i].mCellY) - 5, 0x32, 0x32);
		gShip.DrawImage(anImages[kPieces[i].mImage], Cell(kPieces[i].mCellX) - kPieces[i].mFrame * 0x32, Cell(kPieces[i].mCellY) - 5);
	}
	Graphics gIsland(*g);
	gIsland.ClipRect(Cell(9) - 2, Cell(7) - 2, 0x2a, 0x20);
	gIsland.DrawImage(anImages[IMAGE_ISLANDS], Cell(9) - 2, Cell(7) - 0x2c);

	g->Translate(5, 0);
	for (int aDX = -1; aDX < 2; aDX++)
		for (int aDY = -1; aDY < 2; aDY++)
			DrawCellArrow(g, 5 + aDX, 5 + aDY);
	g->Translate(-5, 0);

	g->SetFont(mApp->mFonts[FONT_FONT1]);
	g->SetColor(Color(0xff, 0, 0));
	WriteCenteredLine(g, 0x23, GT(0x004a3578));
	g->SetFont(mApp->mFonts[FONT_TUT]);
	g->SetColor(Color(0xff, 0xff, 0));
	for (unsigned int i = 0; i < sizeof(kLines) / sizeof(kLines[0]); i++)
		g->DrawString(GT(kLines[i].mText), kLines[i].mX, kLines[i].mY);
}

// 0x40f710
void HelpWidget::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}

// 0x40f720
void HelpWidget::ButtonDepress(int theId)
{
	if (theId == 0)
	{
		mApp->mTutorial = true;
		mApp->EndTutorial();
	}
	else if (theId == 1)
		mApp->CloseHelp();
}
