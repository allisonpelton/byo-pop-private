// The beginner's demo of a first easy game (WinSS.exe 0x4132b0 constructor, 0x414830 update, 0x413850 draw, 0x416040
// the button). Behaviour spec: docs/GAME_NOTES.md section 4.

#include "Demo.h"
#include "SevenSeasApp.h"
#include "Anims.h"
#include "Res.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cstring>

using namespace Sexy;

// 0x40a3f0: a cell's first pixel
static int Cell(int theCell)
{
	return theCell * 0x2a;
}

// the cell offsets of the show's parts: where the player's ship sails (0x4937bc) and where the pirate that leads the
// others sails (0x4937e4); index by the part (1 to 5)
static const int kPlayerMove[6][2] = {{0, 0}, {1, 0}, {0, 0}, {1, 0}, {1, -1}, {0, 0}};
static const int kBossMove[6][2] = {{0, 0}, {0, -1}, {-1, -1}, {0, -1}, {0, -1}, {0, 0}};
// the whirlpools in the four corners (0x493814)
static const int kWhirls[4][2] = {{0, 0}, {0, 10}, {10, 0}, {10, 10}};

// the captions of the parts (0x4a39f0 and 0x4a3a10): the first line, the second line; part 4 has the second line only
static const unsigned kCaptionTop[9] = {0, 0x004a3b64, 0x004a3b48, 0x004a3b28, 0, 0x004a3b08, 0x004a3ae0, 0x004a3ae0, 0x004a3ae0};
static const unsigned kCaptionBottom[9] = {0, 0x004a3ac8, 0x004a3ab0, 0x004a3a90, 0x004a3a78, 0x004a3a58, 0x004a3a34, 0x004a3a34, 0x004a3a34};

// a ship that is new: on its cell, heading south, with a random start for the sail animation (the draw comes first)
static void InitShip(DemoShip& theShip, int theX, int theY)
{
	theShip.mAnim = CrtRand() % 8;
	theShip.mTimer = 0;
	theShip.mFrame = 8;
	theShip.mX = theShip.mTargetX = theX;
	theShip.mY = theShip.mTargetY = theY;
}

// the heading of a ship that is about to sail to a neighbour cell (the same lines for every ship at 0x414c00 and
// the others)
static void Face(DemoShip& theShip)
{
	int aDX = theShip.mTargetX - theShip.mX;
	if (aDX == -1)
	{
		if (theShip.mTargetY < theShip.mY)
			theShip.mFrame = 0xe;
		if (theShip.mTargetY == theShip.mY)
			theShip.mFrame = 0xc;
		if (theShip.mY < theShip.mTargetY)
			theShip.mFrame = 10;
	}
	else if (theShip.mTargetX == theShip.mX)
	{
		if (theShip.mTargetY < theShip.mY)
			theShip.mFrame = 0;
		if (theShip.mY < theShip.mTargetY)
			theShip.mFrame = 8;
	}
	else if (aDX == 1)
	{
		if (theShip.mTargetY < theShip.mY)
			theShip.mFrame = 2;
		if (theShip.mTargetY == theShip.mY)
			theShip.mFrame = 4;
		if (theShip.mY < theShip.mTargetY)
			theShip.mFrame = 6;
	}
}

static void Sink(DemoShip& theShip)
{
	theShip.mTargetX = 99;
	theShip.mX = 99;
	theShip.mTargetY = 99;
	theShip.mY = 99;
}

// 0x4132b0
DemoWidget::DemoWidget(SevenSeasApp* theApp) :
	mApp(theApp),
	mAct(0),
	mStep(0),
	mFade(0xff),
	mShotCnt(0),
	mWaitCnt(0),
	mTurnCnt(0),
	mScrollX(0.0),
	mScrollY(0.0),
	mSpeedX(0.0),
	mSpeedY(0.0)
{
	mButton = new DialogButton2003(mApp->mImages[IMAGE_DBUTTONWOOD], 1, this);
	InitShips();
	mCursorX = Cell(5);
	mCursorY = Cell(5);
	mButton->mLabel = GT(0x004a3b80);
	mButton->SetFont(mApp->mFonts[FONT_MT11]);
	mButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xe2, 0x3c));
}

DemoWidget::~DemoWidget()
{
	delete mButton;
	for (std::list<AnimInstance*>::iterator anItr = mAnims.begin(); anItr != mAnims.end(); ++anItr)
		delete *anItr;
}

// the ships on their first cells; each draws a random start of its sail animation (this order of the draws)
void DemoWidget::InitShips()
{
	InitShip(mPlayer, 5, 5);
	InitShip(mPirates[0], 1, 0);
	InitShip(mPirates[1], 1, 8);
	InitShip(mPirates[2], 8, 1);
	InitShip(mPirates[3], 6, 1);
	InitShip(mBoss, 10, 9);
}

// 0x4137c0
void DemoWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mButton);
}

// 0x4137e0
void DemoWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	mApp->mWidgetManager->RemoveWidget(mButton);
}

// 0x413810
void DemoWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mButton->Resize(0x100, 0x1a9, 300, 0x1e);
}

// 0x40f750: the mouse position goes to the side panel too
void DemoWidget::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mApp->FollowMouse(mX + x, y);
}

// 0x416030
void DemoWidget::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}

// 0x416040: the real game begins
void DemoWidget::ButtonDepress(int theId)
{
	(void) theId;
	mApp->mBoard = new Board(mApp, false);
	mApp->mBoard->Resize(0xad, 0, 0x1d3, 0x1e0);
	mApp->mWidgetManager->AddWidget(mApp->mBoard);
	mApp->mWidgetManager->BringToFront(mApp->mBoard);
	mApp->mWidgetManager->SetFocus(mApp->mBoard);
	mApp->mUndoButton->SetVisible(true);
	mApp->mQuitButton->SetVisible(false);
	mApp->RemoveDemo();
}

// 0x415ec0: the explosions and splashes step to their next frame and end
void DemoWidget::UpdateAnims()
{
	std::list<AnimInstance*>::iterator anItr = mAnims.begin();
	while (anItr != mAnims.end())
	{
		AnimInstance* anAnim = *anItr;
		anAnim->mDelay = anAnim->mDelay - 1;
		if (anAnim->mDelay < 1)
		{
			AnimDef* aDef = anAnim->mDef;
			int aNumFrames = (int) aDef->mFrames.size();
			anAnim->mFrameIdx = anAnim->mFrameIdx + 1;
			if (anAnim->mFrameIdx < aNumFrames)
				anAnim->mDelay = aDef->mFrames[anAnim->mFrameIdx].mDelay;
			else if (aDef->mLoop)
			{
				anAnim->mFrameIdx = 0;
				anAnim->mDelay = aDef->mFrames[0].mDelay;
			}
			else
			{
				delete anAnim;
				anItr = mAnims.erase(anItr);
				continue;
			}
		}
		++anItr;
	}
}

// 0x415f60
void DemoWidget::AddAnim(const char* theName, int theX, int theY)
{
	AnimDef* aDef = mApp->GetAnimDef(theName);
	if (aDef == NULL)
		return;
	AnimInstance* anAnim = new AnimInstance();
	anAnim->mDef = aDef;
	anAnim->mX = theX;
	anAnim->mY = theY;
	anAnim->mFrameIdx = 0;
	anAnim->mFlag = false;
	anAnim->mDelay = aDef->mFrames.empty() ? 0 : aDef->mFrames[0].mDelay;
	mAnims.push_back(anAnim);
}

// 0x414830
void DemoWidget::Update()
{
	Widget::Update();
	UpdateAnims();

	// the water drifts: the speeds wander between a tenth and an eighth of a pixel
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

	mPlayer.mAnim++;
	mBoss.mAnim++;
	for (int i = 0; i < 4; i++)
		mPirates[i].mAnim++;
	if (mUpdateCnt % 2 == 0)
		return;

	switch (mAct)
	{
	case 0:
		// the black cover goes away
		mFade = mFade - 10;
		if (mFade < 0)
			mFade = 0;
		if (mFade < 1)
			mAct = 1;
		break;
	case 6:
		// the player's ship sails to the north-east corner
		if (mPlayer.mTimer == 0)
		{
			if (mPlayer.mX < 10)
				mPlayer.mTargetX = mPlayer.mTargetX + 1;
			if (0 < mPlayer.mY)
				mPlayer.mTargetY = mPlayer.mTargetY - 1;
			Face(mPlayer);
			mApp->PlaySample(SOUND_MOVEGOOD);
		}
		mPlayer.mTimer = mPlayer.mTimer + 1;
		if (mPlayer.mTimer >= 0x28)
		{
			mPlayer.mX = mPlayer.mTargetX;
			mPlayer.mY = mPlayer.mTargetY;
			if (mPlayer.mY != 0)
				mPlayer.mTimer = 0;
			else
			{
				mApp->PlaySample(SOUND_WHIRLOUT);
				mTurnCnt = 0;
				mAct++;
			}
		}
		break;
	case 7:
		// the ship turns on the spot and then is thrown out of the whirlpool at the other corner
		mPlayer.mFrame = mPlayer.mFrame + 1;
		if (0xf < mPlayer.mFrame)
			mPlayer.mFrame = 0;
		mTurnCnt++;
		if (0x27 < mTurnCnt)
		{
			mTurnCnt = 0;
			mPlayer.mFrame = 4;
			mPlayer.mTargetX = 2;
			mPlayer.mX = 2;
			mPlayer.mTargetY = 7;
			mPlayer.mY = 7;
			mApp->PlaySample(SOUND_WHIRLIN);
			mAct++;
		}
		break;
	case 8:
		mPlayer.mFrame = mPlayer.mFrame - 1;
		if (mPlayer.mFrame < 0)
			mPlayer.mFrame = 0xf;
		mTurnCnt++;
		if (0x28 <= mTurnCnt)
		{
			mTurnCnt = 0;
			mAct++;
		}
		break;
	case 9:
		// the black cover comes back, then it all starts again
		mFade = mFade + 10;
		if (0xff < mFade)
			mFade = 0xff;
		if (mFade < 0xff)
			break;
		mAct = 0;
		InitShips();
		mStep = 0;
		break;
	default:
		// parts 1 to 5: the cursor goes to the cell, clicks, the player's ship sails (or fires), the pirates answer
		switch (mStep)
		{
		case 0:
		{
			int aTX = Cell(kPlayerMove[mAct][0] + mPlayer.mX);
			int aTY = Cell(kPlayerMove[mAct][1] + mPlayer.mY);
			if ((mCursorX != aTX) || (mCursorY != aTY))
			{
				if (mCursorX < aTX)
					mCursorX = mCursorX + 2;
				if (aTX < mCursorX)
					mCursorX = mCursorX - 2;
				if (mCursorY < aTY)
					mCursorY = mCursorY + 2;
				if (aTY < mCursorY)
					mCursorY = mCursorY - 2;
			}
			else
				mStep++;
			break;
		}
		case 1:
			// the click: the cursor goes down
			if (mCursorY < Cell(kPlayerMove[mAct][1] + mPlayer.mY) + 3)
			{
				mCursorY = mCursorY + 1;
				mCursorX = mCursorX + 1;
			}
			else
			{
				mApp->PlaySample(SOUND_CLICK);
				mStep++;
			}
			break;
		case 2:
			// and up
			if (Cell(kPlayerMove[mAct][1] + mPlayer.mY) < mCursorY)
			{
				mCursorY = mCursorY - 1;
				mCursorX = mCursorX - 1;
			}
			else
				mStep++;
			break;
		case 3:
			if (mPlayer.mTimer == 0)
			{
				mPlayer.mTargetX = kPlayerMove[mAct][0] + mPlayer.mX;
				mPlayer.mTargetY = kPlayerMove[mAct][1] + mPlayer.mY;
				if ((mPlayer.mTargetX == mPlayer.mX) && (mPlayer.mTargetY == mPlayer.mY))
				{
					// the click on the ship itself: the broadsides
					mPlayer.mTimer = 0;
					mShotCnt = 0;
					mApp->PlaySample(SOUND_BOOM);
					mStep = 4;
					break;
				}
				Face(mPlayer);
				mApp->PlaySample(SOUND_MOVEGOOD);
			}
			if (mPlayer.mTimer < 0x28)
				mPlayer.mTimer = mPlayer.mTimer + 1;
			else
			{
				mPlayer.mX = mPlayer.mTargetX;
				mPlayer.mY = mPlayer.mTargetY;
				mPlayer.mTimer = 0;
				mStep = 6;
			}
			break;
		case 4:
			mShotCnt = mShotCnt + 2;
			if ((mAct == 5) && (mShotCnt == 0x28))
			{
				mApp->PlaySample(SOUND_EXPLODE2);
				AddAnim("EXPLODE", Cell(9) + 0x14, Cell(5) + 0x14);
				mBoss.mY = 99;
				mBoss.mX = 99;
			}
			if (mShotCnt < 0x78)
				break;
			if (mAct == 2)
			{
				mApp->PlaySample(SOUND_EXPLODE2);
				AddAnim("EXPLODE", Cell(6) + 0x14, Cell(2) + 0x14);
				AddAnim("SPLASH", Cell(6) + 0x12, Cell(8) + 0x12);
			}
			else if (mAct == 5)
				AddAnim("SPLASH", Cell(5) + 0x12, Cell(1) + 0x12);
			mWaitCnt = 0;
			mStep++;
			if (mAct == 2)
				Sink(mPirates[3]);
			break;
		case 5:
			mWaitCnt++;
			if (0xe < mWaitCnt)
				mStep = 6;
			break;
		case 6:
			// the pirates sail: the leader its own way, the others towards the player's ship
			if (mBoss.mTimer == 0)
			{
				mBoss.mTargetX = kBossMove[mAct][0] + mBoss.mX;
				mBoss.mTargetY = kBossMove[mAct][1] + mBoss.mY;
				for (int i = 0; i < 4; i++)
				{
					DemoShip& aPirate = mPirates[i];
					if (aPirate.mX < mPlayer.mX)
						aPirate.mTargetX = aPirate.mTargetX + 1;
					if (mPlayer.mX < aPirate.mX)
						aPirate.mTargetX = aPirate.mTargetX - 1;
					if (aPirate.mY < mPlayer.mY)
						aPirate.mTargetY = aPirate.mTargetY + 1;
					if (mPlayer.mY < aPirate.mY)
						aPirate.mTargetY = aPirate.mTargetY - 1;
				}
				if ((mBoss.mTargetX != mBoss.mX) || (mBoss.mTargetY != mBoss.mY))
				{
					Face(mBoss);
					for (int i = 0; i < 4; i++)
						Face(mPirates[i]);
					mApp->PlaySample(SOUND_MOVEEVIL);
				}
			}
			if (mBoss.mTimer < 0x28)
			{
				mBoss.mTimer = mBoss.mTimer + 1;
				for (int i = 0; i < 4; i++)
					mPirates[i].mTimer = mPirates[i].mTimer + 1;
				break;
			}
			mBoss.mX = mBoss.mTargetX;
			mBoss.mY = mBoss.mTargetY;
			for (int i = 0; i < 4; i++)
			{
				mPirates[i].mX = mPirates[i].mTargetX;
				mPirates[i].mY = mPirates[i].mTargetY;
			}
			mBoss.mTimer = 0;
			for (int i = 0; i < 4; i++)
				mPirates[i].mTimer = 0;
			mWaitCnt = 0;
			if ((mAct == 4) || (mAct == 3))
				mApp->PlaySample(SOUND_EXPLODE);
			if (mAct == 3)
				AddAnim("EXPLODE", Cell(7) + 0x14, Cell(4) + 0x14);
			else if (mAct == 4)
				AddAnim("EXPLODE", Cell(5) + 0x14, Cell(4) + 0x14);
			mStep++;
			break;
		case 7:
			mWaitCnt++;
			if (mAct == 3)
				Sink(mPirates[2]);
			else if (mAct == 4)
			{
				Sink(mPirates[1]);
				Sink(mPirates[0]);
			}
			else
				mWaitCnt = 0xe;
			if (mWaitCnt < 0xe)
				break;
			mAct++;
			mStep = 0;
			break;
		}
		break;
	}
	MarkDirty();
}

// 0x415e10: the explosions and splashes, added to the picture
void DemoWidget::DrawAnims(Graphics* g)
{
	for (std::list<AnimInstance*>::iterator anItr = mAnims.begin(); anItr != mAnims.end(); ++anItr)
	{
		AnimInstance* anAnim = *anItr;
		AnimDef* aDef = anAnim->mDef;
		const AnimFrame& aFrame = aDef->mFrames[anAnim->mFrameIdx];
		int aY = anAnim->mY - aFrame.mH / 2;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		if (aDef->mFlagB == 1)
			aY = aY - anAnim->mFrameIdx;
		g->DrawImage(aDef->mImage, anAnim->mX - aFrame.mW / 2, aY, Rect(aFrame.mX, aFrame.mY, aFrame.mW, aFrame.mH));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// a pirate ship (or the leader) on its way between two cells, a sheet of 16 headings by 8 frames
static void DrawShip(Graphics* g, Image* theSheet, const DemoShip& theShip)
{
	Graphics g2(*g);
	int aSailX = Cell(theShip.mX) - 5 + (theShip.mTargetX - theShip.mX) * theShip.mTimer;
	int aSailY = Cell(theShip.mY) - 5 + (theShip.mTargetY - theShip.mY) * theShip.mTimer;
	g2.ClipRect(aSailX, aSailY, 0x32, 0x32);
	g2.DrawImage(theSheet, aSailX + theShip.mFrame * -0x32, aSailY + (theShip.mAnim / 0xe % 8) * -0x32);
}

// the cannon ball and its shadow of the broadsides (the first one flies up before the ships are drawn, the second
// one down after them), the offsets from the player's cell
static void DrawBall(Graphics* g, SevenSeasApp* theApp, const DemoShip& thePlayer, int theOffsetX, int theBallY, int theShadowY)
{
	g->DrawImage(theApp->mImages[IMAGE_CANNON], Cell(thePlayer.mX) + 0x10 + theOffsetX, Cell(thePlayer.mY) + 0x10 + theBallY);
	g->DrawImage(theApp->mImages[IMAGE_CANNONSHADOW], Cell(thePlayer.mX) + 0x10 + theOffsetX, Cell(thePlayer.mY) + 0x20 + theShadowY);
}

// 0x413850
void DemoWidget::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	int aScrollX = (int) mScrollX;
	int aScrollY = (int) mScrollY;
	g->DrawImage(anImages[IMAGE_WATER], aScrollX, aScrollY);
	g->DrawImage(anImages[IMAGE_WATER], aScrollX - 500, aScrollY);
	g->DrawImage(anImages[IMAGE_WATER], aScrollX, aScrollY - 500);
	g->DrawImage(anImages[IMAGE_WATER], aScrollX - 500, aScrollY - 500);
	g->DrawImage(anImages[IMAGE_BACKDROP], 0, 0, Rect(0xad, 0, 0x1d3, 0x1e0));
	g->Translate(0, 0xc);
	g->DrawImage(anImages[IMAGE_GRID], -6, -0xc);

	// the island on the way and the wrecks that appear as the show goes on
	Graphics gIsland(*g);
	gIsland.ClipRect(Cell(7) - 2, Cell(4) - 2, 0x2a, 0x2a);
	gIsland.DrawImage(anImages[IMAGE_ISLANDS], Cell(7) + (mUpdateCnt / 0xe % 4) * -0x2a - 2, Cell(4) - 2);
	if ((2 < mAct) || ((mAct == 2) && (5 < mStep)))
	{
		Graphics gRubble(*g);
		gRubble.ClipRect(Cell(6), Cell(2), 0x28, 0x28);
		gRubble.DrawImage(anImages[IMAGE_RUBBLE], Cell(6) + ((mUpdateCnt / 0x1e + 8) % 0xc) * -0x28, Cell(2));
	}
	if (4 < mAct)
	{
		Graphics gRubble(*g);
		gRubble.ClipRect(Cell(5), Cell(4), 0x28, 0x28);
		gRubble.DrawImage(anImages[IMAGE_RUBBLE], Cell(5) + ((mUpdateCnt / 0x1e + 9) % 0xc) * -0x28, Cell(4));
	}
	if ((5 < mAct) || ((mAct == 5) && ((4 < mStep) || ((mStep == 4) && (0x2b < mShotCnt)))))
	{
		Graphics gRubble(*g);
		gRubble.ClipRect(Cell(9), Cell(5), 0x28, 0x28);
		gRubble.DrawImage(anImages[IMAGE_RUBBLE], Cell(9) + ((mUpdateCnt / 0x1e + 0xe) % 0xc) * -0x28, Cell(5));
	}

	for (int i = 0; i < 4; i++)
		DrawShip(g, anImages[IMAGE_PIRATE], mPirates[i]);
	DrawShip(g, anImages[IMAGE_REDPIRATE], mBoss);

	// the broadsides' first cannon ball: it flies up (part 2) or up and to the left (part 5)
	if ((0 < mAct) && (mAct < 9) && (mStep == 4))
	{
		int aLift = 0;
		int aDir = 0;
		int aDX = 0;
		if (mAct == 2)
		{
			aLift = (mShotCnt < 0x6a) ? 5 : 0x6e - mShotCnt;
			aDir = -1;
		}
		else if (mAct == 5)
		{
			aLift = (mShotCnt < 0x6a) ? 5 : 0x6e - mShotCnt;
			aDX = -1;
			aDir = -1;
		}
		DrawBall(g, mApp, mPlayer, mShotCnt * aDX, mShotCnt * aDir - aLift, mShotCnt * aDir);
	}

	// the whirlpools in the corners
	for (int i = 0; i < 4; i++)
	{
		Graphics gWhirl(*g);
		gWhirl.ClipRect(Cell(kWhirls[i][0]), Cell(kWhirls[i][1]), 0x28, 0x28);
		gWhirl.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		gWhirl.DrawImage(anImages[IMAGE_WHIRL], Cell(kWhirls[i][0]), Cell(kWhirls[i][1]) + ((mUpdateCnt + i * 2) / 4 % 0x10) * -0x28);
		gWhirl.SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	// the player's ship
	DrawShip(g, anImages[IMAGE_GOODSHIP], mPlayer);

	// the second cannon ball: it flies down (part 2) or down and to the right (part 5)
	if ((0 < mAct) && (mAct < 9) && (mStep == 4))
	{
		int aLift = 0;
		int aDir = 0;
		int aDX = 0;
		bool aDraw = true;
		if (mAct == 2)
		{
			aLift = (mShotCnt < 0x6a) ? 5 : 0x6e - mShotCnt;
			aDir = 1;
		}
		else if (mAct == 5)
		{
			aLift = (mShotCnt < 0x1a) ? 5 : 0x1e - mShotCnt;
			aDX = 1;
			aDir = 1;
			if (0x28 < mShotCnt)
				aDraw = false;
		}
		if (aDraw)
			DrawBall(g, mApp, mPlayer, mShotCnt * aDX, mShotCnt * aDir - aLift, mShotCnt * aDir);
	}
	if (mAct < 6)
		g->DrawImage(anImages[IMAGE_CURSOR1], mCursorX - 5, mCursorY - 5);
	DrawAnims(g);
	g->Translate(0, -0xc);

	if ((mAct < 1) || (8 < mAct))
	{
		// the black cover of the fade in and out
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, mFade));
		g->FillRect(0, 0, mWidth, mHeight);
		g->SetColorizeImages(false);
		return;
	}
	g->SetColor(Color(0xff, 0xff, 0));
	g->SetFont(mApp->mFonts[FONT_MT11]);
	if (kCaptionTop[mAct] == 0)
		WriteCenteredLine(g, 0x2e, GT(kCaptionBottom[mAct]));
	else
	{
		WriteCenteredLine(g, 0x25, GT(kCaptionTop[mAct]));
		WriteCenteredLine(g, 0x37, GT(kCaptionBottom[mAct]));
	}
}
