#include "Tutorial.h"
#include "AngleTable.h"
#include "CellButton.h"
#include "Compat2004.h"
#include "DynomiteFossil.h"
#include "DynomitePanic.h"
#include "DynomiteStomped.h"
#include "DynomiteTimeTrial.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "Kroney.h"
#include "Launcher.h"
#include "SideBar.h"
#include "Sprite.h"
#include "Whirley.h"
#include "WidgetManager.h"
#include <cstring>

using namespace Sexy;

// port: split out of Step 0x43b2c0: the arrow's point (the original's two-int class)
static GridPoint* NewPoint(int theX, int theY)
{
	GridPoint* aPoint = new GridPoint();
	aPoint->mX = theX;
	aPoint->mY = theY;
	return aPoint;
}

// port: split out of Step 0x43b2c0: a cell's picture over its egg (bits 9 to 12)
static void SetKind(GridCell& theCell, int theKind)
{
	theCell.mBits = theKind << 9 | (theCell.mBits & 0xffffe1ff);
}

// 0x43ac40
Tutorial::Tutorial(FossilizedApp* theApp)
{
	mApp = theApp;
	mSerial = Dynomite::NextSerial();	// port(livesplit)
	mStep = -1;
	mShownStep = -1;
	// port: the original leaves these to the first Step
	mTimer = 0;
	mBoxX = 0;
	mBoxY = 0;
	mContinueY = 0;
	mNoContinue = false;

	mInchy = new Pic(theApp, "images\\TutorInchy", true);
	mContinuePic = new Pic(theApp, "images\\continuebuttonsheet", true);
	mContinueCell = new Cell(mContinuePic, 0x4d, 0x11, 0);
	mContinueOverCell = new Cell(mContinuePic, 0x4d, 0x11, 1);
	mContinueButton = new CellButton(100, this);
	mContinueButton->mBtnNoDraw = false;
	mContinueButton->mDoFinger = true;
	mContinueButton->SetVisible(true);
	mContinueButton->mButtonCell = mContinueCell;
	mContinueButton->mOverCell = mContinueOverCell;
	mContinueButton->mDownCell = mContinueOverCell;

	mLinks[0] = new HyperlinkWidget(1000, this);
	mLinks[0]->mLabel = mApp->GetString("ENDLESS_PUZZLE");
	mLinks[0]->SetFont(mApp->mWhiteFontSmall);
	mLinks[0]->mColor = Color(200, 0xff, 200);
	mLinks[1] = new HyperlinkWidget(1001, this);
	mLinks[1]->mLabel = mApp->GetString("STOMPED_PUZZLE");
	mLinks[1]->SetFont(mApp->mWhiteFontSmall);
	mLinks[1]->mColor = Color(200, 0xff, 200);
	mLinks[2] = new HyperlinkWidget(1002, this);
	mLinks[2]->mLabel = mApp->GetString("FOSSIL_CHALLENGE");
	mLinks[2]->SetFont(mApp->mWhiteFontSmall);
	mLinks[2]->mColor = Color(200, 0xff, 200);
	mLinks[3] = new HyperlinkWidget(1003, this);
	mLinks[3]->mLabel = mApp->GetString("TIME_TRIAL");
	mLinks[3]->SetFont(mApp->mWhiteFontSmall);
	mLinks[3]->mColor = Color(200, 0xff, 200);

	mDelay = 0;
	mArrow = NULL;
	mLinksShown = false;
	mHasTransparencies = true;
	mOldLockCursor = mApp->mLockCursor;
	mApp->mLockCursor = false;
}

// 0x43aad0 (the continue button is not deleted)
Tutorial::~Tutorial()
{
	mApp->mLockCursor = mOldLockCursor;
	if (mInchy != NULL)
	{
		delete mInchy;
		mInchy = NULL;
	}
	if (mContinuePic != NULL)
	{
		if (mContinueCell != NULL)
			delete mContinueCell;
		if (mContinueOverCell != NULL)
			delete mContinueOverCell;
		if (mContinuePic != NULL)
			delete mContinuePic;
		mContinuePic = NULL;
	}
	if (mArrow != NULL)
	{
		delete mArrow;
		mArrow = NULL;
	}
	if (mLinks[0] != NULL)
	{
		mApp->SafeDeleteWidget(mLinks[0]);
		mApp->SafeDeleteWidget(mLinks[1]);
		mApp->SafeDeleteWidget(mLinks[2]);
		mApp->SafeDeleteWidget(mLinks[3]);
		mLinks[0] = NULL;
		mLinks[1] = NULL;
		mLinks[2] = NULL;
		mLinks[3] = NULL;
	}
}

// 0x439ee0
void Tutorial::MarkDirty()
{
	Widget::MarkDirty();
	mApp->mSideBar->MarkDirty();
}

// 0x439f00: where the continue button goes under theWidth's text
int Tutorial::ContinueY(int theWidth)
{
	Graphics* g = new Graphics((Image*) NULL);
	g->SetFont(mApp->mWhiteFontSmall);
	int aHeight = GetWordWrappedHeight2004(g, mBoxX - 200 + theWidth, mText, 15);
	mContinueY = (mBoxY - aHeight / 2) + 0x35 + aHeight;
	if (g != NULL)
		delete g;
	return mContinueY;
}

// 0x439fc0
void Tutorial::SetupBoard(int theMode)
{
	Dynomite* aBoard = mApp->mBoard;
	if (aBoard != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(aBoard);
		mApp->SafeDeleteWidget(mApp->mBoard);
		mApp->mBoard = NULL;
	}
	switch (theMode)
	{
	case 1:
		strcpy(mApp->mPackFile, "DEFAULT.EGG");
		aBoard = new DynomiteStomped();
		aBoard->Resize(0x96, 0, 0x1ea, 0x1e0);
		aBoard->Init(mApp, 2);
		aBoard->mNextEggKind = 0;
		mApp->mWidgetManager->AddWidget(aBoard);
		mApp->mBoard = aBoard;
		break;
	case 2:
		aBoard = new DynomitePanic();
		aBoard->Resize(0x96, 0, 0x1ea, 0x1e0);
		aBoard->Init(mApp, 1);
		aBoard->mNextEggKind = 0;
		mApp->mWidgetManager->AddWidget(aBoard);
		mApp->mBoard = aBoard;
		((DynomitePanic*) aBoard)->mKroney->Start(100000.0f);
		break;
	case 3:
		aBoard = new DynomiteFossil();
		aBoard->Resize(0x96, 0, 0x1ea, 0x1e0);
		aBoard->Init(mApp, 2);
		mApp->mWidgetManager->AddWidget(aBoard);
		mApp->mBoard = aBoard;
		break;
	case 4:
		aBoard = new DynomiteTimeTrial();
		aBoard->Resize(0x96, 0, 0x1ea, 0x1e0);
		aBoard->Init(mApp, 2);
		mApp->mWidgetManager->AddWidget(aBoard);
		mApp->mBoard = aBoard;
		break;
	}
	mApp->mBoard->mGrid->mBabyEggChance = 5000000;
	mApp->mWidgetManager->RemoveWidget(this);
	mApp->mWidgetManager->AddWidget(this);
	mApp->mSideBar->GameStarted();
}

// 0x43a240
void Tutorial::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mContinueButton);
	mApp->mWidgetManager->BringToFront(this);
	mApp->mWidgetManager->BringToFront(mContinueButton);
}

// 0x43a280
void Tutorial::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mContinueButton);
	theWidgetManager->RemoveWidget(mLinks[0]);
	theWidgetManager->RemoveWidget(mLinks[1]);
	theWidgetManager->RemoveWidget(mLinks[2]);
	theWidgetManager->RemoveWidget(mLinks[3]);
}

// 0x43a2e0: the continue button at the box's right, under the text
void Tutorial::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	if (mNoContinue == true)
	{
		mContinueButton->Resize(-1000, -1000, 0x4d, 0x11);
		return;
	}
	mContinueButton->Resize(mBoxX + mWidth - 200 + theX, mContinueY + theY, 0x4d, 0x11);
}

// 0x43a360 (the y is the board's plus the widget's, an original slip the board's y of 0 hides)
void Tutorial::MouseDown(int x, int y, int theClickCount)
{
	Dynomite* aBoard = mApp->mBoard;
	aBoard->MouseDown((mX - aBoard->mX) + x, aBoard->mY + mY + y, theClickCount);
	mApp->mBoard->mIsOver = true;
}

// 0x43a3b0 (likewise)
void Tutorial::MouseMove(int x, int y)
{
	Dynomite* aBoard = mApp->mBoard;
	aBoard->MouseMove((mX - aBoard->mX) + x, aBoard->mY + mY + y);
	mApp->mBoard->mIsOver = true;
}

// 0x43a3f0
void Tutorial::ShowLinks(bool show)
{
	if (show == true)
	{
		if (mLinksShown == false)
		{
			mWidgetManager->AddWidget(mLinks[0]);
			mLinks[0]->Resize(mX + 10, mY + 0x78, 0x8c, 0x12);
			mWidgetManager->AddWidget(mLinks[1]);
			mLinks[1]->Resize(mX + 10, mY + 0x8b, 0x8c, 0x12);
			mWidgetManager->AddWidget(mLinks[2]);
			mLinks[2]->Resize(mX + 10, mY + 0x9e, 0x8c, 0x12);
			mWidgetManager->AddWidget(mLinks[3]);
			mLinks[3]->Resize(mX + 10, mY + 0xb1, 0x8c, 0x12);
			mLinksShown = true;
		}
	}
	else if ((show == false) && (mLinksShown == true))
	{
		mWidgetManager->RemoveWidget(mLinks[0]);
		mWidgetManager->RemoveWidget(mLinks[1]);
		mWidgetManager->RemoveWidget(mLinks[2]);
		mWidgetManager->RemoveWidget(mLinks[3]);
		mLinksShown = false;
	}
}

// 0x43a530
void Tutorial::Start()
{
	SetupBoard(2);
	mApp->mBoard->MouseMove(0x131, 300);
	mApp->mSideBar->SetPauseState(2);
	mApp->mBoard->mInputLocked = true;
	mStep = 0;
}

// 0x43a580: the head 25 pixels deep and 40 wide, the shaft 30 wide
// port: the x87 code keeps the products and sums unrounded (in 53 bits), so this works in double
void Tutorial::DrawArrow(Graphics* g, int* theFrom, int* theTo)
{
	double aDir[2];
	mApp->mAngles->Normalize(aDir, theFrom[0] - theTo[0], theFrom[1] - theTo[1]);
	double aBaseX = aDir[0] * 25.0 + (double) theTo[0];
	double aBaseY = aDir[1] * 25.0 + (double) theTo[1];
	Point aPoints[7];
	aPoints[0].mX = theTo[0];
	aPoints[0].mY = theTo[1];
	aPoints[1].mX = (int) (aBaseX + aDir[1] * 20.0);
	aPoints[1].mY = (int) (aBaseY - aDir[0] * 20.0);
	aPoints[2].mX = (int) (aBaseX + aDir[1] * 15.0);
	aPoints[2].mY = (int) (aBaseY - aDir[0] * 15.0);
	aPoints[3].mX = (int) ((double) theFrom[0] + aDir[1] * 15.0);
	aPoints[3].mY = (int) ((double) theFrom[1] - aDir[0] * 15.0);
	aPoints[4].mX = (int) ((double) theFrom[0] - aDir[1] * 15.0);
	aPoints[4].mY = (int) ((double) theFrom[1] + aDir[0] * 15.0);
	aPoints[5].mX = (int) (aBaseX - aDir[1] * 15.0);
	aPoints[5].mY = (int) (aBaseY + aDir[0] * 15.0);
	aPoints[6].mX = (int) (aBaseX - aDir[1] * 20.0);
	aPoints[6].mY = (int) (aBaseY + aDir[0] * 20.0);
	g->PolyFill(aPoints, 7);
}

// 0x43a720: the black box, the links' box, the arrow and its shadow, Inchy, and the text and its shadow
void Tutorial::Draw(Graphics* g)
{
	SetDrawGraphics(g);
	g->SetFont(mApp->mWhiteFontSmall);
	int aHeight = GetWordWrappedHeight2004(g, mWidth - 200, mText, 15);
	int aTop = mBoxY - aHeight / 2;
	Rect aTextRect(mBoxX + 100, aTop + 0x32, mWidth - 200, aHeight);
	Rect aShadowRect(mBoxX + 0x66, aTop + 0x34, mWidth - 200, aHeight);
	g->SetColor(Color(0, 0, 0, 0xff));
	g->FillRect(mBoxX, aTop + 0x32 - 5, mWidth - 100, aHeight + 10);
	if (mLinksShown == true)
		g->FillRect(5, 0x73, 0x96, 0x5f);
	if (mArrow != NULL)
	{
		int anAngle = mApp->mAngles->AngleTo(mArrow->mX, mArrow->mY, mX + 0x32 + mBoxX, mY + 0x32 + mBoxY);
		int aFrom[2];
		aFrom[0] = mBoxX + 0x32;
		aFrom[1] = mBoxY + 0x32;
		int aTo[2];
		aTo[0] = (int) ((double) aFrom[0] - (double) mApp->mAngles->mDX[anAngle] * 75.0);
		aTo[1] = (int) ((double) aFrom[1] - (double) mApp->mAngles->mDY[anAngle] * 75.0);
		int aShadowFrom[2];
		aShadowFrom[0] = mBoxX + 0x37;
		aShadowFrom[1] = mBoxY + 0x37;
		int aShadowTo[2];
		aShadowTo[0] = aTo[0] + 5;
		aShadowTo[1] = aTo[1] + 5;
		g->SetColor(Color(0, 0, 0, 200));
		DrawArrow(g, aShadowFrom, aShadowTo);
		g->SetColor(Color(0xff, 0, 0, 0xff));
		DrawArrow(g, aFrom, aTo);
	}
	mInchy->Draw((mBoxX - (mInchy->mWidth >> 1)) + 0x32, (mBoxY - (mInchy->mHeight >> 1)) + 0x32);
	g->SetColor(Color(0, 0, 0, 0xff));
	WriteWordWrapped2004(g, aShadowRect, mText, 15, -1);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	WriteWordWrapped2004(g, aTextRect, mText, 15, -1);
}

// 0x43a9e0: continue (100), a step that is done (0, no sound), or a mode's link
void Tutorial::ButtonPress(int theId)
{
	if (mArrow != NULL)
	{
		delete mArrow;
		mArrow = NULL;
	}
	if (theId != 0)
		mApp->mSndEgglands->Play();
	Resize(-1000, -1000, 0x1a4, 100);
	ShowLinks(false);
	if (theId < 1000)
	{
		mDelay = 0x19;
		if (mStep != 0x2c)
		{
			mStep = mStep + 1;
			return;
		}
		mStep = 0x2d;
		mApp->mBoard->mPauseCount = 0;
	}
	else
	{
		if (theId == 1000)
		{
			mStep = 0x32;
			return;
		}
		if (theId == 1001)
		{
			mStep = 0x42;
			return;
		}
		if (theId == 1002)
		{
			mStep = 0x4f;
			return;
		}
		if (theId == 1003)
		{
			mStep = 99;
			return;
		}
	}
}

// 0x43b2c0: Inchy's text and box for mStep, its board and its grid, once mDelay has run out. Each step's text is
// a string id from the exe (GT); the box is mBoxX/mBoxY into the widget, 200 less than its width wide.
void Tutorial::Step()
{
	if (0 < mDelay)
	{
		mDelay = mDelay - 1;
		return;
	}
	mShownStep = mStep;
	mBoxX = 0;
	mBoxY = 0;
	mNoContinue = false;
	mApp->mBoard->mChain = -1;
	switch (mStep)
	{
	case 0:
		mTimer = 0x32;
		break;
	case 1:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff578));
		ContinueY(0x1a4);
		Resize(0xe6, 100, 0x1a4, 100);
		break;
	case 2:
		mArrow = NewPoint(0x4b, 0xac);
		mBoxX = 0x32;
		mBoxY = 0;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff530));
		ContinueY(0x1a4);
		Resize(0x6e, 100, mBoxX + 0x1a4, 100);
		break;
	case 3:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff498));
		ContinueY(0x1a4);
		Resize(0xe6, 100, 0x1a4, 300);
		ShowLinks(true);
		break;
	case 4:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff448));
		ContinueY(0x1a4);
		Resize(0xe6, 100, mBoxX + 0x1a4, 100);
		break;
	case 5:
		mArrow = NewPoint(0x18d, 0x1e0);
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff3f8));
		ContinueY(0x1a4);
		Resize(0x10e, 0xfa, mBoxX + 0x1a4, 0x96);
		break;
	case 6:
		mArrow = NewPoint(0x113, 0x1c2);
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff3b0));
		ContinueY(0x1a4);
		Resize(0xfa, 0xdc, mBoxX + 0x1a4, 0x96);
		break;
	case 7:
		mArrow = NewPoint(0x239, 0x67);
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff378));
		ContinueY(0x14a);
		Resize(400, 0xaa, mBoxX + 0x14a, mBoxY + 100);
		break;
	case 8:
		mArrow = NewPoint(0x203, 0x1b2);
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff34c));
		ContinueY(0x15e);
		Resize(0x17c, 0x10e, mBoxX + 0x15e, mBoxY + 0x96);
		break;
	case 9:
		mArrow = NewPoint(0x50, 0x15a);
		mBoxX = 0x23;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff300));
		ContinueY(0x1a4);
		Resize(0x7e, 300, mBoxX + 0x1a4, mBoxY + 100);
		break;
	case 10:
		mArrow = NewPoint(0x172, 0x48);
		mBoxY = 0x23;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff2cc));
		ContinueY(0x1a4);
		Resize(200, 0x96, mBoxX + 0x1a4, mBoxY + 100);
		break;
	case 0xb:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff278));
		ContinueY(0x1a4);
		Resize(0xe6, 100, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mCeilingAtGoal = true;
		mApp->mBoard->mCeilingGoal = -35;
		break;
	case 0xc:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff228));
		ContinueY(0x1a4);
		Resize(0xe6, 0x50, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mGrid->Clear();
		break;
	case 0xd:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff180));
		ContinueY(0x1a4);
		Resize(0xe6, 0x50, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		break;
	case 0xe:
	{
		mArrow = NewPoint(0x198, 0x41);
		mBoxY = 0x19;
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->Place(4, 0, 2, 0);
		mApp->mBoard->mGrid->Place(4, 1, 2, 0);
		mApp->mBoard->mGrid->Place(4, 2, 1, 0);
		mApp->mBoard->mGrid->Place(4, 3, 1, 0);
		mApp->mBoard->mGrid->ResetCeiling();
		Egg* anEgg = mApp->mBoard->NextEgg();
		anEgg->mColor = 1;
		mApp->mBoard->mLauncher->Freeze(anEgg);
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff120));
		ContinueY(0x1a4);
		Resize(0xe6, 0x78, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	}
	case 0xf:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff0d0));
		ContinueY(0x1a4);
		Resize(0xe6, 0x78, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x10:
		mNoContinue = true;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff088));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		break;
	case 0x11:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4ff000));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x12:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fef90));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x13:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fef30));
		ContinueY(0x1a4);
		Resize(0xe6, 0xdc, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x14:
	{
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4feef8));
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->Place(4, 0, 2, 0);
		mApp->mBoard->mGrid->Place(4, 1, 2, 0);
		mApp->mBoard->mGrid->Place(5, 2, 2, 0);
		mApp->mBoard->mGrid->Place(5, 3, 2, 0);
		mApp->mBoard->mGrid->Place(6, 4, 1, 0);
		mApp->mBoard->mGrid->Place(6, 5, 1, 0);
		mApp->mBoard->mGrid->ResetCeiling();
		Egg* anEgg = mApp->mBoard->NextEgg();
		anEgg->mColor = 2;
		mApp->mBoard->mLauncher->Freeze(anEgg);
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	}
	case 0x15:
		mArrow = NewPoint(0x1d6, 0);
		mNoContinue = true;
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4feeb8));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		break;
	case 0x16:
		mNoContinue = false;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fee58));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x17:
		mArrow = NewPoint(0x192, 0x8a);
		mBoxY = 0x19;
		mNoContinue = false;
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->Place(4, 0, 2, 0);
		mApp->mBoard->mGrid->Place(4, 1, 3, 0);
		mApp->mBoard->mGrid->Place(4, 2, 4, 0);
		mApp->mBoard->mGrid->Place(4, 3, 5, 0);
		mApp->mBoard->mGrid->Place(4, 4, 0x12, 0);
		mApp->mBoard->mGrid->Place(4, 5, 0x13, 0);
		mApp->mBoard->mGrid->ResetCeiling();
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fede0));
		ContinueY(0x1a4);
		Resize(0xe6, 0xb4, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mCeilingAtGoal = true;
		mApp->mBoard->mCeilingGoal = -35;
		break;
	case 0x18:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fed70));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x19:
	{
		mArrow = NewPoint(0x19f, 0x55);
		mBoxY = 0x23;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fed18));
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->Place(3, 0, 3, 0);
		mApp->mBoard->mGrid->Place(4, 0, 3, 0);
		mApp->mBoard->mGrid->Place(5, 0, 3, 0);
		mApp->mBoard->mGrid->Place(3, 1, 3, 0);
		mApp->mBoard->mGrid->Place(4, 1, 3, 0);
		mApp->mBoard->mGrid->Place(5, 1, 3, 0);
		mApp->mBoard->mGrid->Place(3, 2, 3, 0);
		mApp->mBoard->mGrid->Place(4, 2, 3, 0);
		mApp->mBoard->mGrid->Place(5, 2, 3, 0);
		mApp->mBoard->mGrid->Place(3, 3, 3, 0);
		mApp->mBoard->mGrid->Place(4, 3, 3, 0);
		mApp->mBoard->mGrid->Place(5, 3, 3, 0);
		mApp->mBoard->mGrid->Place(3, 4, 3, 0);
		mApp->mBoard->mGrid->Place(4, 4, 3, 0);
		mApp->mBoard->mGrid->Place(5, 4, 3, 0);
		mApp->mBoard->mGrid->ResetCeiling();
		Egg* anEgg = mApp->mBoard->NextEgg();
		anEgg->mColor = 3;
		mApp->mBoard->mLauncher->Freeze(anEgg);
		BabyEgg* aBaby = new BabyEgg(mApp->mBoard->mGrid, 4, 3, 100000000);
		if (0 < aBaby->mTime)
			mApp->mBoard->mGrid->mBabyEggs.Add(aBaby);
		ContinueY(0x17c);
		Resize(0x14a, 0x82, mBoxX + 0x17c, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	}
	case 0x1a:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fec90));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x1b:
		mNoContinue = true;
		mApp->mBoard->mGrid->mMaxColor = 2;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fec50));
		ContinueY(0x1a4);
		Resize(0xe6, 0xfa, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		break;
	case 0x1c:
		mNoContinue = true;
		mArrow = NewPoint(0x113, 0x1c2);
		mApp->mBoard->mGrid->mMaxColor = 2;
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->AddRow(0);
		mApp->mBoard->mGrid->AddRow(1);
		mApp->mBoard->mGrid->AddRow(2);
		mApp->mBoard->mGrid->ResetCeiling();
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4febb8));
		ContinueY(0x1a4);
		Resize(0xfa, 0xdc, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		break;
	case 0x1d:
		mArrow = NewPoint(0x18b, 0x1cc);
		mApp->mBoard->mGrid->mMaxColor = 2;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4feb68));
		ContinueY(0x1a4);
		Resize(0xff, 0x131, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x1e:
		mApp->mBoard->mGrid->mMaxColor = 2;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4feb10));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x1f:
		mApp->mBoard->mGrid->mMaxColor = 2;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4feaa0));
		mApp->mBoard->mGrid->mMaxColor = 3;
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->AddRow(0);
		mApp->mBoard->mGrid->AddRow(1);
		mApp->mBoard->mGrid->AddRow(2);
		mApp->mBoard->mGrid->AddRow(3);
		mApp->mBoard->mGrid->AddRow(4);
		mApp->mBoard->mGrid->AddRow(5);
		mApp->mBoard->mGrid->AddRow(6);
		mApp->mBoard->mGrid->AddRow(7);
		mApp->mBoard->mGrid->AddRow(8);
		mApp->mBoard->mGrid->AddRow(9);
		mApp->mBoard->mGrid->ResetCeiling();
		mApp->mBoard->mGrid->ClearDynamite();
		// six bonus eggs (kinds 1, 2, 5, 6, 7 and 9) for the next steps' arrows
		SetKind(mApp->mBoard->mGrid->mCells[3][7], 1);
		SetKind(mApp->mBoard->mGrid->mCells[1][5], 2);
		SetKind(mApp->mBoard->mGrid->mCells[7][6], 5);
		SetKind(mApp->mBoard->mGrid->mCells[5][4], 6);
		SetKind(mApp->mBoard->mGrid->mCells[3][4], 7);
		SetKind(mApp->mBoard->mGrid->mCells[6][2], 9);
		ContinueY(0x1a4);
		Resize(200, 0x96, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x20:
		mArrow = NewPoint(0x17b, 0xe3);
		mApp->mBoard->mGrid->mMaxColor = 2;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fea28));
		ContinueY(0x1a4);
		Resize(300, 0x50, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x21:
		mArrow = NewPoint(0x13b, 0x9f);
		mApp->mBoard->mGrid->mMaxColor = 2;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe9b0));
		ContinueY(0x1a4);
		Resize(0xfa, 10, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x22:
		mArrow = NewPoint(0x1f1, 0xc1);
		mApp->mBoard->mGrid->mMaxColor = 2;
		mBoxY = 0x23;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe978));
		ContinueY(0x140);
		Resize(400, 0xbe, mBoxX + 0x140, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x23:
		mArrow = NewPoint(0x1ae, 0x7d);
		mApp->mBoard->mGrid->mMaxColor = 2;
		mBoxY = 0x23;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe930));
		ContinueY(0x1a4);
		Resize(0x140, 0x69, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x24:
		mArrow = NewPoint(0x16d, 0x7d);
		mApp->mBoard->mGrid->mMaxColor = 2;
		mBoxY = 0x23;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe904));
		ContinueY(0x1a4);
		Resize(0x140, 0x7d, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mCeilingAtGoal = true;
		mApp->mBoard->mCeilingGoal = -35;
		break;
	case 0x25:
		mArrow = NewPoint(0x1ce, 0x3b);
		mApp->mBoard->mGrid->mMaxColor = 2;
		mBoxY = 0x23;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe8a0));
		ContinueY(0x172);
		Resize(0x172, 0x3b, mBoxX + 0x172, mBoxY + 0x6e);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x26:
	{
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe818));
		Egg* anEgg = mApp->mBoard->NextEgg();
		anEgg->mColor = 2;
		anEgg->mKind = 1;
		mApp->mBoard->mLauncher->Freeze(anEgg);
		ContinueY(0x208);
		Resize(0xb4, 200, mBoxX + 0x208, mBoxY + 0x6e);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mCeilingAtGoal = true;
		mApp->mBoard->mCeilingGoal = -35;
		break;
	}
	case 0x27:
		mNoContinue = true;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe7ec));
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->Place(4, 0, 2, 0);
		mApp->mBoard->mGrid->Place(4, 1, 2, 0);
		mApp->mBoard->mGrid->Place(5, 2, 2, 0);
		mApp->mBoard->mGrid->Place(5, 3, 2, 0);
		mApp->mBoard->mGrid->Place(6, 4, 2, 0);
		mApp->mBoard->mGrid->Place(6, 5, 2, 0);
		mApp->mBoard->mGrid->ResetCeiling();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		break;
	case 0x28:
		mArrow = NewPoint(0x233, 0x82);
		mBoxY = 0x1e;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe7a0));
		ContinueY(0x140);
		Resize(400, 0x9b, mBoxX + 0x140, mBoxY + 0x6e);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x29:
		mBoxY = 0x1e;
		mApp->mBoard->mGrid->mMaxColor = 9;
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->AddRow(0);
		mApp->mBoard->mGrid->AddRow(1);
		mApp->mBoard->mGrid->AddRow(2);
		mApp->mBoard->mGrid->AddRow(3);
		mApp->mBoard->mGrid->AddRow(4);
		mApp->mBoard->mGrid->AddRow(5);
		mApp->mBoard->mGrid->AddRow(6);
		mApp->mBoard->mGrid->AddRow(7);
		mApp->mBoard->mGrid->AddRow(8);
		mApp->mBoard->mGrid->AddRow(9);
		mApp->mBoard->mGrid->ResetCeiling();
		mApp->mBoard->mGrid->mMaxColor = 4;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe760));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x6e);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x2a:
		mArrow = NewPoint(0x195, 0x184);
		mBoxY = 0x1e;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe6f8));
		ContinueY(0x1a4);
		Resize(300, 0xe1, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		break;
	case 0x2b:
		mNoContinue = true;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe690));
		ContinueY(0x1a4);
		Resize(0xe6, 0xb4, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		break;
	case 0x2c:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe620));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 100);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x2d:
		// off the screen while the egg sinks; Update waits for the game's end
		mNoContinue = true;
		Resize(-5000, 200, mBoxX + 0x1a4, mBoxY + 0x96);
		break;
	case 0x2e:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe618));
		ContinueY(300);
		Resize(300, 0xa0, mBoxX + 300, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x2f:
		mArrow = NewPoint(0x2d, 0x128);
		mBoxX = 0x32;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe5a8));
		ContinueY(0x1a4);
		Resize(0x1e, 0x118, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x30:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe550));
		ContinueY(0x1a4);
		Resize(0xe6, 0xb4, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x31:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe4b0));
		ContinueY(0x1a4);
		Resize(0xe6, 0xb4, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x32:
		// the Endless puzzle's part
		SetupBoard(2);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x33:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe408));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x34:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe3e4));
		ContinueY(0x1a4);
		Resize(0xe6, 0xb4, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x35:
		mArrow = NewPoint(0x18b, 0x28);
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe368));
		mBoxY = 0x19;
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x36:
		mNoContinue = true;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe338));
		ContinueY(0x1a4);
		Resize(0xe6, 0x28, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x37:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe2a0));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x96);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x38:
		mText = mApp->GetString(GT(0x4fe26c));
		mArrow = NewPoint(0x239, 0x67);
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		ContinueY(0x14a);
		Resize(400, 0xaa, mBoxX + 0x14a, mBoxY + 100);
		break;
	case 0x39:
		mNoContinue = true;
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe220));
		((DynomitePanic*) mApp->mBoard)->mKroney->Start(10.0f);
		ContinueY(0x14a);
		Resize(0x15e, 200, mBoxX + 0x14a, mBoxY + 100);
		break;
	case 0x3a:
	{
		// the arrow and the box follow the whirley Update has waited for
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe20c));
		Whirley* aWhirley = ((DynomitePanic*) mApp->mBoard)->mWhirleys.Get(0);
		mArrow = NewPoint((int) ((double) aWhirley->mX + 150.0), (int) aWhirley->mY);
		ContinueY(0x14a);
		Resize((int) aWhirley->mX, (int) ((double) aWhirley->mY - 110.0), mBoxX + 0x14a, mBoxY + 0x82);
		mApp->mBoard->mPauseCount = 1;
		break;
	}
	case 0x3b:
		mApp->mSndInchypop->Play();
		mText = mApp->GetString(GT(0x4fe1b0));
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x3c:
		mText = mApp->GetString(GT(0x4fe118));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0x96, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x3d:
		mText = mApp->GetString(GT(0x4fe0a8));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xa0, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x3e:
	{
		mNoContinue = true;
		mText = mApp->GetString(GT(0x4fe068));
		Whirley* aWhirley = ((DynomitePanic*) mApp->mBoard)->mWhirleys.Get(0);
		aWhirley->mVX = 0.0f;
		aWhirley->mVY = 0.0f;
		aWhirley->mClimb = 0.0f;
		aWhirley->mSpeed = 0.0f;
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xa0, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 0;
		break;
	}
	case 0x3f:
		mText = mApp->GetString(GT(0x4fdfe0));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xaa, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x40:
		mText = mApp->GetString(GT(0x4fdf90));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xaa, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x41:
		mText = mApp->GetString(GT(0x4fdf30));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xb4, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mPauseCount = 1;
		break;
	case 0x42:
		// the Stomped puzzle's part
		SetupBoard(1);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		mApp->mBoard->mGrid->ClearDynamite();
		break;
	case 0x43:
		mText = mApp->GetString(GT(0x4fdf00));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x44:
		mText = mApp->GetString(GT(0x4fdeb0));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xb4, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x45:
		mText = mApp->GetString(GT(0x4fde40));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x46:
		mText = mApp->GetString(GT(0x4fddf0));
		mArrow = NewPoint(0x21c, 0xdc);
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		ContinueY(0x14a);
		Resize(400, 300, mBoxX + 0x14a, mBoxY + 0x78);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x47:
		mText = mApp->GetString(GT(0x4fdd68));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x48:
	{
		mNoContinue = true;
		mText = mApp->GetString(GT(0x4fdd38));
		DynomiteStomped* aStomped = (DynomiteStomped*) mApp->mBoard;
		aStomped->mShotsPerDrop = 5;
		aStomped->mShotsLeft = 5;
		mArrow = NewPoint(0x21c, 0xdc);
		mApp->mSndInchypop->Play();
		ContinueY(0x140);
		Resize(0x17c, 0x32, mBoxX + 0x140, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 0;
		break;
	}
	case 0x49:
		mText = mApp->GetString(GT(0x4fdd20));
		mArrow = NewPoint(0x230, 0x19c);
		mApp->mSndInchypop->Play();
		ContinueY(0x140);
		Resize(0x1af, 0xe2, mBoxX + 0x140, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x4a:
		mNoContinue = true;
		mText = mApp->GetString(GT(0x4fdcfc));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0x32, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x4b:
		mText = mApp->GetString(GT(0x4fdcb0));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x4c:
		mText = mApp->GetString(GT(0x4fdc78));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xfa, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x4d:
		mText = mApp->GetString(GT(0x4fdc00));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xdc, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x4e:
		mText = mApp->GetString(GT(0x4fdbc4));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x4f:
		// the Fossil Challenge's part
		SetupBoard(3);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		((DynomiteFossil*) mApp->mBoard)->mKroney->Start(100000.0f);
		break;
	case 0x50:
		mText = mApp->GetString(GT(0x4fdb8c));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x51:
		mText = mApp->GetString(GT(0x4fdb30));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x52:
		mArrow = NewPoint(0x180, 0x71);
		mText = mApp->GetString(GT(0x4fdb1c));
		mBoxY = 0x1e;
		mApp->mSndInchypop->Play();
		ContinueY(0x140);
		Resize(0x17c, 0x71, mBoxX + 0x140, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x53:
		mText = mApp->GetString(GT(0x4fdac8));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x54:
		mText = mApp->GetString(GT(0x4fda80));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x55:
		mText = mApp->GetString(GT(0x4fda44));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd7, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x56:
		mText = mApp->GetString(GT(0x4fd9d8));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x57:
		mText = mApp->GetString(GT(0x4fd970));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x58:
		mText = mApp->GetString(GT(0x4fd958));
		mArrow = NewPoint(0x239, 0x67);
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		ContinueY(0x14a);
		Resize(400, 0xaa, mBoxX + 0x14a, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x59:
		mText = mApp->GetString(GT(0x4fd8b8));
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xaa, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x5a:
		mText = mApp->GetString(GT(0x4fd828));
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xaa, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x5b:
		mText = mApp->GetString(GT(0x4fd7c8));
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(200, 0xaa, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x5c:
		mText = mApp->GetString(GT(0x4fd750));
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xcd, 0xaa, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x5d:
		mArrow = NewPoint(0x6d, 0x192);
		mNoContinue = true;
		mText = mApp->GetString(GT(0x4fd710));
		mBoxX = 0x23;
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0x7c, 0x172, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x5e:
		mText = mApp->GetString(GT(0x4fd6e8));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x5f:
		mText = mApp->GetString(GT(0x4fd688));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xcd, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x60:
		mText = mApp->GetString(GT(0x4fd628));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x61:
		mText = mApp->GetString(GT(0x4fd5c0));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x62:
		// the end of the part, or (with the museum still open) a wait for it to close
		if (((DynomiteFossil*) mApp->mBoard)->mMuseum == NULL)
		{
			mText = mApp->GetString(GT(0x4fd54c));
		}
		else
		{
			mNoContinue = true;
			mText = mApp->GetString(GT(0x4fd570));
		}
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 99:
		// the Time Trial's part
		SetupBoard(4);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 100:
		mText = mApp->GetString(GT(0x4fd524));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x65:
		mText = mApp->GetString(GT(0x4fd4a8));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x66:
		mText = mApp->GetString(GT(0x4fd430));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x67:
		mNoContinue = true;
		mText = mApp->GetString(GT(0x4fd3c8));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 100, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = false;
		mApp->mBoard->mFireBlocked = false;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x68:
		mText = mApp->GetString(GT(0x4fd370));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x69:
		mArrow = NewPoint(0x253, 0x139);
		mText = mApp->GetString(GT(0x4fd360));
		mBoxY = 0x19;
		mApp->mSndInchypop->Play();
		ContinueY(0x104);
		Resize(0x1d6, 0x139, mBoxX + 0x104, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x6a:
		mText = mApp->GetString(GT(0x4fd318));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x6b:
		mText = mApp->GetString(GT(0x4fd2a0));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x6c:
		mText = mApp->GetString(GT(0x4fd220));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x6d:
		mText = mApp->GetString(GT(0x4fd1d0));
		mApp->mSndInchypop->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 200, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x6e:
		mText = mApp->GetString(GT(0x4fd194));
		mApp->mSndInchypop->Play();
		mApp->mSndChime->Play();
		ContinueY(0x1a4);
		Resize(0xe6, 0xd2, mBoxX + 0x1a4, mBoxY + 0x82);
		mApp->mBoard->mInputLocked = true;
		mApp->mBoard->mFireBlocked = true;
		mApp->mBoard->mPauseCount = 0;
		break;
	case 0x6f:
		// the end: back to the main menu, which deletes this
		mApp->MenuButton();
		break;
	}
}

// 0x4408b0: the step's wait for the player, and the grid kept still (or moved) under the text. ButtonPress(0)
// ends a step that waits.
void Tutorial::Update()
{
	if (mStep != mShownStep)
		Step();
	if (0 < mTimer)
		mTimer = mTimer - 1;
	switch (mStep)
	{
	case 0:
		if (mTimer == 0)
		{
			mStep = 1;
			return;
		}
		break;
	case 0xc:
	{
		EggGrid* aGrid = mApp->mBoard->mGrid;
		if (-0x28 < aGrid->mBottom)
		{
			aGrid->mCeiling = aGrid->mCeiling - 3.0f;
			return;
		}
		break;
	}
	case 0xd:
		mApp->mBoard->mGrid->Clear();
		mApp->mBoard->mGrid->mCeiling = -30.0f;
		return;
	case 0xe:
	case 0xf:
	case 0x1b:
	case 0x1c:
	{
		((DynomitePanic*) mApp->mBoard)->mCeilingSpeed->mSpeed = 0.0f;
		if (mStep == 0x1b)
		{
			if ((char) mApp->mBoard->mGrid->mCells[4][3].mBits == '\0')
				ButtonPress(0);
		}
		else if (mStep == 0x1c)
		{
			Egg* anEgg = mApp->mBoard->mLauncher->mLoaded;
			if ((anEgg != NULL) && (anEgg->mKind != 0))
				ButtonPress(0);
		}
		EggGrid* aGrid = mApp->mBoard->mGrid;
		aGrid->mCeiling = aGrid->mCeiling + 1.0f;
		aGrid = mApp->mBoard->mGrid;
		if (-11.0f < aGrid->mCeiling)
		{
			aGrid->mCeiling = -11.0f;
			return;
		}
		break;
	}
	case 0x10:
	case 0x15:
	case 0x14:
	case 0x17:
	case 0x19:
	case 0x1a:
	case 0x1d:
	{
		((DynomitePanic*) mApp->mBoard)->mCeilingSpeed->mSpeed = 0.0f;
		if ((mStep == 0x10) || (mStep == 0x15))
		{
			// the blast took the eggs and the rows have come down: the cell aimed at is empty
			if (8.0f < mApp->mBoard->mGrid->mRows)
				ButtonPress(0);
			char aColor;
			if (mStep == 0x10)
				aColor = (char) mApp->mBoard->mGrid->mCells[4][3].mBits;
			else
				aColor = (char) mApp->mBoard->mGrid->mCells[5][2].mBits;
			if (aColor == '\0')
				ButtonPress(0);
		}
		EggGrid* aGrid = mApp->mBoard->mGrid;
		aGrid->mCeiling = aGrid->mCeiling + 1.0f;
		aGrid = mApp->mBoard->mGrid;
		if (-11.0f < aGrid->mCeiling)
		{
			aGrid->mCeiling = -11.0f;
			return;
		}
		break;
	}
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x18:
	case 0x1e:
	{
		((DynomitePanic*) mApp->mBoard)->mCeilingSpeed->mSpeed = 0.0f;
		EggGrid* aGrid = mApp->mBoard->mGrid;
		if (-355.0f < aGrid->mCeiling)
		{
			aGrid->mCeiling = aGrid->mCeiling - 1.0f;
			return;
		}
		break;
	}
	case 0x16:
	{
		((DynomitePanic*) mApp->mBoard)->mCeilingSpeed->mSpeed = 0.0f;
		EggGrid* aGrid = mApp->mBoard->mGrid;
		if (-355.0f < aGrid->mCeiling)
		{
			aGrid->mCeiling = -355.0f;
			return;
		}
		break;
	}
	case 0x1f:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x27:
	case 0x28:
	case 0x29:
	case 0x2a:
	case 0x2b:
	{
		((DynomitePanic*) mApp->mBoard)->mCeilingSpeed->mSpeed = 0.0f;
		if (mStep == 0x27)
		{
			// the lightbulb is on
			if ((mApp->mBoard->mBonusBits & 1) != 0)
				ButtonPress(0);
		}
		else if (mStep == 0x2b)
		{
			// an egg has sat past the line for a second
			if (0x1e < mApp->mBoard->mGrid->mPastLineFrames)
				ButtonPress(0);
		}
		EggGrid* aGrid = mApp->mBoard->mGrid;
		aGrid->mCeiling = aGrid->mCeiling + 2.0f;
		aGrid = mApp->mBoard->mGrid;
		if (-11.0f < aGrid->mCeiling)
		{
			aGrid->mCeiling = -11.0f;
			return;
		}
		break;
	}
	case 0x26:
	{
		((DynomitePanic*) mApp->mBoard)->mCeilingSpeed->mSpeed = 0.0f;
		EggGrid* aGrid = mApp->mBoard->mGrid;
		if (-500.0f < aGrid->mCeiling)
		{
			aGrid->mCeiling = aGrid->mCeiling - 3.0f;
			return;
		}
		break;
	}
	case 0x2d:
		// the game is over and its "game over" has faded in
		((DynomitePanic*) mApp->mBoard)->mCeilingSpeed->mSpeed = 0.0f;
		if (mApp->mBoard->mGameOver != true)
			return;
		if (0x32 < mApp->mBoard->mGameOverAlpha)
			ButtonPress(0);
		break;
	case 0x32:
		if (mShownStep == 0x32)
		{
			ButtonPress(0);
			return;
		}
		mDelay = 0;
		return;
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x37:
	case 0x38:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3f:
		((DynomitePanic*) mApp->mBoard)->mCeilingSpeed->mSpeed = 0.0f;
		return;
	case 0x36:
	{
		// a few shots at the puzzle coming down, stopped short of the line
		Dynomite* aBoard = mApp->mBoard;
		((DynomitePanic*) aBoard)->mCeilingSpeed->mSpeed = 0.04f;
		if (0xfa < aBoard->mGrid->mBottom)
			((DynomitePanic*) aBoard)->mCeilingSpeed->mSpeed = 0.0f;
		if (3 < aBoard->mShotCount)
		{
			ButtonPress(0);
			return;
		}
		break;
	}
	case 0x39:
	{
		// the first whirley well up the board
		DynomitePanic* aBoard = (DynomitePanic*) mApp->mBoard;
		aBoard->mCeilingSpeed->mSpeed = 0.0f;
		if ((0 < aBoard->mWhirleys.mCount) && (aBoard->mWhirleys.Get(0)->mY < 380.0f))
		{
			ButtonPress(0);
			aBoard->mKroney->Start(100000.0f);
			return;
		}
		break;
	}
	case 0x3e:
	{
		// the whirley shot
		DynomitePanic* aBoard = (DynomitePanic*) mApp->mBoard;
		aBoard->mCeilingSpeed->mSpeed = 0.0f;
		EggGrid* aGrid = aBoard->mGrid;
		if (0x15e < aGrid->mBottom)
			aGrid->mCeiling = aGrid->mCeiling - 1.0f;
		if (aBoard->mWhirleys.mCount == 0)
		{
			ButtonPress(0);
			return;
		}
		break;
	}
	case 0x42:
		if (mShownStep != 0x42)
		{
			mDelay = 0;
			return;
		}
		ButtonPress(0);
		break;
	case 0x48:
		// Mama's meter a shot from her foot
		if (((DynomiteStomped*) mApp->mBoard)->mShotsLeft < 4)
		{
			ButtonPress(0);
			return;
		}
		break;
	case 0x4a:
		if (4 < ((DynomiteStomped*) mApp->mBoard)->mShotsLeft)
			ButtonPress(0);
		break;
	case 0x4f:
		if (mShownStep == 0x4f)
		{
			ButtonPress(0);
			return;
		}
		mDelay = 0;
		return;
	case 0x5d:
		// the museum opened
		if (((DynomiteFossil*) mApp->mBoard)->mMuseum != NULL)
		{
			ButtonPress(0);
			return;
		}
		break;
	case 0x62:
		// and closed
		if ((mNoContinue == true) && (((DynomiteFossil*) mApp->mBoard)->mMuseum == NULL))
		{
			ButtonPress(0);
			return;
		}
		break;
	case 99:
		mDelay = 0;
		if (mShownStep == 99)
		{
			ButtonPress(0);
			return;
		}
		break;
	case 0x67:
		if (3 < mApp->mBoard->mShotCount)
			ButtonPress(0);
		break;
	}
}
