// FullScreenHelp: the picture of how to play that a game opens with (see FullScreenHelp.h).

#include "FullScreenHelp.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Board.h"
#include "Checkbox.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The places of the pictures (the original keeps them in globals that its start sets, 0x59dbb8 to 0x59dbfc)
const int kLeftX = 0xa5;            // where the left scene's bricks start
const int kLeftY = 0x6b;
const int kRightX = 0x1d8;          // and the right scene's
const int kRightY = 0x6b;
const int kLeftMouseX = 0xab;       // the mouse picture over the left scene
const int kLeftBeamY = 0x6a;        // the beam and the ship of the left scene
const int kLeftShipBase = 0x6c;
const int kRightMouseX = 0x1de;
const int kRightBeamY = 0x6a;
const int kRightShipBase = 0x6c;
const int kSupaX = 0xab;            // the supa weapon's button
const int kSupaY = 0x111;
const int kSupaWidth = 0x81;
const int kSupaMouseBase = 0x6c;
const int kLevelUpX = 0x1d5;        // the picture of the level-up under the scenes
const int kLevelUpY = 0x111;

const int kLeftColumns = 7;
const int kRightColumns = 7;

// The kind of each brick of the right scene (0x563d40, four rows of seven; -1: none), and, of the bricks that break up
// (0x563db0, 1 for those), and of the left scene (0x563d04, two rows of seven)
const int kLeftKinds[2 * 7] = {2, 0, 0, 3, 1, 2, 1, 3, 1, 1, 3, 3, 0, 2};
const int kRightKinds[4 * 7] = {2, 1, 3, 0, 3, 2, 3, 2, 0, 0, 1, -1, 2, 1, 3, 1, 2, 2, -1, 0, 2, 0, 1, -1, -1, -1, -1, 1};
const int kRightBreaks[4 * 7] = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0};

// The original's cel drawing of an image of a row of cels goes through the shared DrawCel
}

// 0x470a60
FullScreenHelp::FullScreenHelp(AstroPopApp* theApp) :
	mLeftBeamColor(0xff, 0xff, 0),
	mLeftShipX(0),
	mLeftCount(0),
	mLeftMouse(0),
	mLeftBlink(false),
	mRightBeamColor(0xff, 0xff, 0xff),
	mRightShipX(0),
	mRightColumn(0),
	mRightMouse(0),
	mRightBlink(false),
	mContinueButton(NULL),
	mApp(theApp),
	mRightCount(0),
	mSupaCount(0),
	mSupaCharge(0),
	mSupaBlink(false),
	mRightPointerX(0),
	mPointerY(0),
	mSupaPointerX(0),
	mSupaPointerY(0),
	mNoMoreCheck(NULL)
{
	mLeftShipX = kLeftX + IMAGE_HELP_BRICK_BLUE->mWidth * 3;
	mLeftMouse = 1;
	mLeftBlink = true;

	mLeft.resize(2);
	int aIndex = 0;
	for (int aRow = 0; aRow < 2; aRow++)
	{
		for (int aCol = 0; aCol < kLeftColumns; aCol++)
		{
			Cell* aCell = new Cell();
			aCell->mX = IMAGE_HELP_BRICK_BLUE->mWidth * aCol + kLeftX;
			aCell->mY = IMAGE_HELP_BRICK_BLUE->mHeight * aRow + kLeftY;
			aCell->mTargetY = aCell->mY;
			aCell->mSpeed = 0;
			aCell->mImage = GetImageById(kLeftKinds[aIndex] + 0x1d1);
			aCell->mDeathImage = NULL;
			mLeft[aRow].push_back(aCell);
			aIndex++;
		}
	}

	mRight.resize(4);
	FillRight();

	mLeftBeamColor = Color(0xff, 0xff, 0xff, 0xff);
	mRightBeamColor = Color(0xff, 0xff, 0xff);
	mRightShipX = kRightX + IMAGE_HELP_BRICK_BLUE->mWidth * 2;
	mRightMouse = 1;
	mRightColumn = 2;
	mRightBlink = true;
	mRightPointerX = mRightShipX;
	mPointerY = kRightBeamY - 0x14 + IMAGE_HELP_BRICK_BLUE->mHeight * 4;

	mNoMoreCheck = new Checkbox(IMAGE_HELP_UNCHECKED, IMAGE_HELP_CHECKED, 0, NULL);
	mNoMoreCheck->mChecked = false;

	mContinueButton = new AstroButton(0xfb4, mApp);
	mContinueButton->mOverImage = IMAGE_HELP_CONTINUE_OVER;
	mContinueButton->mDownImage = IMAGE_HELP_CONTINUE_DOWN;
	mContinueButton->mButtonImage = NULL;

	mState.assign(4, 0);
	mRightCount = 0;
	mLeftCount = 0;
	mSupaCount = 0;
	mSupaCharge = 0;
	mSupaPointerX = kSupaWidth / 2 + 0x28 + kSupaX;
	mSupaPointerY = 0x145;
	mSupaBlink = true;
}

// The bricks of the right scene, four rows of seven, each with the picture of its kind (the ones that break up have the
// picture of that too and a frame counter that waits). The constructor and the start of the scene's round make them alike.
void FullScreenHelp::FillRight()
{
	int aIndex = 0;
	for (int aRow = 0; aRow < 4; aRow++)
	{
		for (int aCol = 0; aCol < kRightColumns; aCol++)
		{
			Cell* aCell = new Cell();
			aCell->mX = IMAGE_HELP_BRICK_BLUE->mWidth * aCol + kRightX;
			aCell->mY = kRightY + IMAGE_HELP_BRICK_BLUE->mHeight * aRow;
			aCell->mTargetY = aCell->mY;
			aCell->mSpeed = 0;
			aCell->mImage = GetImageById(kRightKinds[aIndex] + 0x1d1);
			aCell->mDeathImage = (kRightBreaks[aIndex] == 1) ? IMAGE_HELP_BRICK_BLUE_DEATH : NULL;
			aCell->mAnim.SetNumFrames(IMAGE_HELP_BRICK_BLUE_DEATH->mNumCols);
			aCell->mAnim.SetDelay(10);
			aCell->mAnim.mHoldLast = true;
			aCell->mAnim.SetStopped(true);
			mRight[aRow].push_back(aCell);
			aIndex++;
		}
	}
}

void FullScreenHelp::ClearRight()
{
	for (size_t aRow = 0; aRow < mRight.size(); aRow++)
	{
		for (size_t aCol = 0; aCol < mRight[aRow].size(); aCol++)
			delete mRight[aRow][aCol];
		mRight[aRow].clear();
	}
}

// 0x46c040: the tick box and the button go with it (the application takes the box off the manager first)
FullScreenHelp::~FullScreenHelp()
{
	delete mNoMoreCheck;
	delete mContinueButton;
	for (size_t aRow = 0; aRow < mLeft.size(); aRow++)
	{
		for (size_t aCol = 0; aCol < mLeft[aRow].size(); aCol++)
			delete mLeft[aRow][aCol];
	}
	ClearRight();
}

// 0x435aa0: the tick box and the button are put in their places
void FullScreenHelp::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	mNoMoreCheck->Resize(0xcf, 0x1cc, IMAGE_HELP_UNCHECKED->mWidth, IMAGE_HELP_UNCHECKED->mHeight);
	mContinueButton->Resize(0x92, 0x1a3, IMAGE_HELP_CONTINUE_OVER->mWidth, IMAGE_HELP_CONTINUE_OVER->mHeight);
	theManager->AddWidget(mNoMoreCheck);
	theManager->AddWidget(mContinueButton);
}

// 0x435b20
void FullScreenHelp::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mNoMoreCheck);
	theManager->RemoveWidget(mContinueButton);
}

// 0x43c770: the black ground, the three scenes, the level-up and the frame of the picture over them
void FullScreenHelp::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0));
	g->FillRect(0, 0, mWidth, mHeight);

	// the scene on the left: the beam and the ship, the bricks, the mouse and the pointer
	g->SetColorizeImages(true);
	g->SetColor(mLeftBeamColor);
	g->DrawImage(IMAGE_HELP_BEAM, mLeftShipX, kLeftBeamY);
	g->SetColorizeImages(false);
	g->DrawImage(IMAGE_HELP_SHIP, mLeftShipX - 2, (kLeftShipBase - IMAGE_HELP_SHIP->mHeight) + kLeftBeamY);
	for (size_t aRow = 0; aRow < mLeft.size(); aRow++)
	{
		for (size_t aCol = 0; aCol < mLeft[aRow].size(); aCol++)
			g->DrawImage(mLeft[aRow][aCol]->mImage, mLeft[aRow][aCol]->mX, mLeft[aRow][aCol]->mY);
	}

	Image* aMouse = IMAGE_HELP_MOUSE_LEFT;
	int aCel;
	int aMouseY = (kLeftShipBase - IMAGE_HELP_MOUSE_LEFT->mHeight) - 4 + kLeftBeamY;
	if (mLeftMouse == 0)
		aCel = 1;
	else if (mLeftMouse == 1)
		aCel = mLeftBlink ? 0 : 1;
	else
	{
		aCel = mLeftBlink ? 0 : 1;
		aMouse = IMAGE_HELP_MOUSE_RIGHT;
	}
	DrawCel(g, aMouse, kLeftMouseX + 4, aMouseY, aCel);
	g->DrawImage(IMAGE_CUSTOM_POINTER, mLeftShipX - 0x14, mPointerY);

	// the scene on the right
	g->SetColorizeImages(true);
	g->SetColor(mRightBeamColor);
	g->DrawImage(IMAGE_HELP_BEAM, mRightShipX, kRightBeamY);
	g->SetColorizeImages(false);
	g->DrawImage(IMAGE_HELP_SHIP, mRightShipX - 2, (kRightShipBase - IMAGE_HELP_SHIP->mHeight) + kRightBeamY);
	for (size_t aRow = 0; aRow < mRight.size(); aRow++)
	{
		for (size_t aCol = 0; aCol < mRight[aRow].size(); aCol++)
		{
			if (kRightKinds[aRow * kRightColumns + aCol] == -1)
				continue;
			Cell* aCell = mRight[aRow][aCol];
			// once the group breaks up the bricks that do show their breaking up, until it is done
			if (mState[1] > 5 && aCell->mDeathImage != NULL)
			{
				FrameAnim& anAnim = aCell->mAnim;
				if (anAnim.mStopped || anAnim.mDone || (anAnim.mMaxLoops > 0 && anAnim.mMaxLoops <= anAnim.mLoopCount))
					continue;
				DrawCel(g, aCell->mDeathImage, aCell->mX, aCell->mY, anAnim.GetCel());
				continue;
			}
			g->DrawImage(aCell->mImage, aCell->mX, aCell->mY);
		}
	}

	aMouse = IMAGE_HELP_MOUSE_LEFT;
	aMouseY = (kRightShipBase - IMAGE_HELP_MOUSE_LEFT->mHeight) - 4 + kRightBeamY;
	if (mRightMouse == 0)
		aCel = 1;
	else if (mRightMouse == 1)
		aCel = mRightBlink ? 0 : 1;
	else
	{
		aCel = mRightBlink ? 0 : 1;
		aMouse = IMAGE_HELP_MOUSE_RIGHT;
	}
	DrawCel(g, aMouse, kRightMouseX + 4, aMouseY, aCel);
	g->DrawImage(IMAGE_CUSTOM_POINTER, mRightPointerX - 0x14, mPointerY);

	// the supa weapon: the strip that charges, the button, the mouse and a target that comes to it
	g->DrawImage(IMAGE_HELP_SUPA_CHARGING, kSupaX, kSupaY);
	int aSupaState = mState[2];
	bool aDrawMouse = true;
	if (aSupaState == 0)
		g->DrawImage(IMAGE_HELP_SUPA_CHARGED, kSupaX, kSupaY, Rect(0, 0, mSupaCharge, IMAGE_HELP_SUPA_CHARGED->mHeight));
	else if (aSupaState <= 2)
		g->DrawImage(IMAGE_HELP_SUPA_BUTTON_OFF, kSupaX, kSupaY);
	else
	{
		aDrawMouse = false;
		if (aSupaState == 3)
		{
			g->DrawImage(IMAGE_HELP_SUPA_BUTTON_ON, kSupaX, kSupaY);
			DrawCel(g, IMAGE_HELP_MOUSE_LEFT, kSupaX + 4, (kSupaMouseBase - IMAGE_HELP_MOUSE_LEFT->mHeight) - 4 + kSupaY,
					mSupaBlink ? 0 : 1);

			// the target, in white: eight strokes around its middle
			int aX = mSupaPointerX + 6;
			int aY = mSupaPointerY + 0x1b;
			g->SetColor(Color(0xff, 0xff, 0xff));
			g->DrawLine(aX - 3, aY - 3, aX - 8, aY - 8);
			g->DrawLine(aX + 3, aY + 3, aX + 8, aY + 8);
			g->DrawLine(aX - 3, aY + 3, aX - 8, aY + 8);
			g->DrawLine(aX + 3, aY - 3, aX + 8, aY - 8);
			g->DrawLine(aX - 3, aY, aX - 8, aY);
			g->DrawLine(aX + 3, aY, aX + 8, aY);
			g->DrawLine(aX, aY + 3, aX, aY + 8);
			g->DrawLine(aX, aY - 3, aX, aY - 8);
		}
	}
	if (aDrawMouse)
		DrawCel(g, IMAGE_HELP_MOUSE_LEFT, kSupaX + 4, (kSupaMouseBase - IMAGE_HELP_MOUSE_LEFT->mHeight) - 4 + kSupaY, 1);
	g->DrawImage(IMAGE_CUSTOM_POINTER, mSupaPointerX - 0x14, mSupaPointerY);

	// the level-up of the game that is on, the frame, and in the other game its words
	Image* aLevelUp = IMAGE_HELP_LEVELUP_CLASSIC;
	if (mApp->mBoard->m1a60 != 1)
		aLevelUp = IMAGE_HELP_LEVELUP_SURVIVAL;
	g->DrawImage(aLevelUp, kLevelUpX, kLevelUpY);
	g->DrawImage(IMAGE_HELP_BG, 0, 0);
	if (mApp->mBoard->m1a60 == 2)
	{
		g->DrawImage(IMAGE_HELP_LEVELUP_SURVIVAL_TEXT, 0x154, 0x109);
		g->DrawImage(IMAGE_HELP_SURVIVAL, 0x1a0, 0xf6);
	}
}

// 0x471060: the three scenes go on. The left one: the ship takes the column of bricks and shoots it up again. The right
// one: the ship takes two bricks from a column, goes to the next and takes one from it, goes on to the fourth and shoots
// them, a group of four breaks up and the brick over it comes down. Under them the supa weapon charges, its button is
// pressed and the target goes up to it. Each scene waits 200 updates (of a step) before the next.
void FullScreenHelp::Update()
{
	Widget::Update();

	std::vector<int>& aState = mState;

	// ---- the left scene
	if (aState[0] == 0)
	{
		if (mUpdateCnt % 0x19 == 0)
			mLeftBlink = !mLeftBlink;
		mLeftCount++;
		if (mLeftCount >= 200)
		{
			mLeftCount = 0;
			aState[0]++;
			mLeft[1][3]->mSpeed = 2;
			mLeft[1][3]->mTargetY = ((kLeftShipBase - IMAGE_HELP_BRICK_RED->mHeight) - IMAGE_HELP_SHIP->mHeight) + kLeftBeamY;
			mLeft[0][3]->mSpeed = 2;
			mLeft[0][3]->mTargetY = mLeft[1][3]->mTargetY - IMAGE_HELP_BRICK_RED->mHeight;
			mLeftBeamColor = Color(0xff, 0xff, 0);
		}
	}
	else if (aState[0] == 1)
	{
		if (mUpdateCnt % 0x19 == 0)
			mLeftBlink = !mLeftBlink;
		Cell* aLow = mLeft[1][3];
		aLow->mY += aLow->mSpeed;
		int aReached = (aLow->mTargetY <= aLow->mY) ? 1 : 0;
		if (aReached)
			aLow->mY = aLow->mTargetY;
		Cell* aHigh = mLeft[0][3];
		aHigh->mY += aHigh->mSpeed;
		if (aHigh->mTargetY <= aHigh->mY)
		{
			aReached++;
			aHigh->mY = aHigh->mTargetY;
		}
		if (aReached == 2)
		{
			mLeftMouse = 2;
			mLeftCount++;
			if (mLeftCount >= 200)
			{
				mLeftCount = 0;
				aState[0]++;
				aLow->mSpeed = -2;
				aLow->mTargetY = IMAGE_HELP_BRICK_BLUE->mHeight + kLeftY;
				aHigh->mSpeed = -2;
				aHigh->mTargetY = kLeftY;
				mLeftBeamColor = Color(0xff, 0xff, 0xff, 0xff);
			}
		}
	}
	else
	{
		if (mUpdateCnt % 0x19 == 0)
			mLeftBlink = !mLeftBlink;
		Cell* aLow = mLeft[1][3];
		aLow->mY += aLow->mSpeed;
		if (aLow->mY <= aLow->mTargetY)
			aLow->mY = aLow->mTargetY;
		Cell* aHigh = mLeft[0][3];
		aHigh->mY += aHigh->mSpeed;
		if (aHigh->mY <= aHigh->mTargetY)
			aHigh->mY = aHigh->mTargetY;
		mLeftMouse = 0;
		mLeftCount++;
		if (mLeftCount >= 200)
		{
			aState[0] = 0;
			mLeftCount = 0;
			mLeftMouse = 1;
			aLow->mSpeed = 0;
			aLow->mTargetY = aLow->mY;
			aHigh->mSpeed = 0;
			aHigh->mTargetY = aHigh->mY;
		}
	}

	// ---- the right scene
	if (aState[1] == 0)
	{
		if (mUpdateCnt % 0x19 == 0)
			mRightBlink = !mRightBlink;
		mRightCount++;
		if (mRightCount >= 200)
		{
			aState[1]++;
			mRightBeamColor = Color(0, 0, 0xff);
			mRightCount = 0;
			mRight[2][2]->mSpeed = 2;
			mRight[2][2]->mTargetY = ((kRightShipBase - IMAGE_HELP_BRICK_RED->mHeight) - IMAGE_HELP_SHIP->mHeight) + kRightBeamY;
		}
	}
	else if (aState[1] == 1 || aState[1] == 3)
	{
		// the ship picks up a brick (of the third and then of the fourth column): it comes down to the ship
		if (mUpdateCnt % 0x19 == 0)
			mRightBlink = !mRightBlink;
		Cell* aBrick = mRight[2][aState[1] == 1 ? 2 : 3];
		aBrick->mY += aBrick->mSpeed;
		if (aBrick->mTargetY <= aBrick->mY)
		{
			aBrick->mY = aBrick->mTargetY;
			mRightMouse = 0;
			mRightCount++;
			if (mRightCount >= 200)
			{
				mRightCount = 0;
				aState[1]++;
			}
		}
	}
	else if (aState[1] == 2)
	{
		// the ship goes to the next column and takes the bricks of it too
		if (mUpdateCnt % 0x19 == 0)
			mRightBlink = !mRightBlink;
		mRightPointerX++;
		int aColumn = (mRightPointerX - kRightX) / IMAGE_HELP_BRICK_BLUE->mWidth;
		if (aColumn != mRightColumn)
		{
			mRightColumn = aColumn;
			mRightShipX = IMAGE_HELP_BRICK_BLUE->mWidth * aColumn + kRightX;
			mRight[2][2]->mX = mRightShipX;
		}
		int aLimit = kRightX + IMAGE_HELP_BRICK_BLUE->mWidth * 3;
		if (aLimit <= mRightPointerX)
		{
			mRightPointerX = aLimit;
			mRightMouse = 1;
			mRightCount++;
			if (mRightCount >= 200)
			{
				aState[1]++;
				mRightBeamColor = Color(0, 0, 0xff);
				mRightCount = 0;
				mRight[2][3]->mSpeed = 2;
				mRight[2][3]->mTargetY = ((kRightShipBase + IMAGE_HELP_BRICK_RED->mHeight * -2) - IMAGE_HELP_SHIP->mHeight) + kRightBeamY;
			}
		}
	}
	else if (aState[1] == 4)
	{
		// the ship, with its three bricks, goes to the sixth column
		if (mUpdateCnt % 0x19 == 0)
			mRightBlink = !mRightBlink;
		mRightPointerX++;
		int aColumn = (mRightPointerX - kRightX) / IMAGE_HELP_BRICK_BLUE->mWidth;
		if (aColumn != mRightColumn)
		{
			mRightColumn = aColumn;
			mRightShipX = IMAGE_HELP_BRICK_BLUE->mWidth * aColumn + kRightX;
			mRight[2][2]->mX = mRightShipX;
			mRight[2][3]->mX = mRightShipX;
		}
		int aLimit = kRightX + IMAGE_HELP_BRICK_BLUE->mWidth * 4;
		if (aLimit <= mRightPointerX)
		{
			mRightPointerX = aLimit;
			mRightMouse = 2;
			mRightCount++;
			if (mRightCount >= 200)
			{
				mRight[2][3]->mSpeed = -2;
				mRight[2][3]->mTargetY = mRight[0][4]->mY + IMAGE_HELP_BRICK_BLUE->mHeight;
				mRight[2][2]->mSpeed = -2;
				mRight[2][2]->mTargetY = mRight[2][3]->mTargetY + IMAGE_HELP_BRICK_BLUE->mHeight;
				mRightCount = 0;
				mRightBeamColor = Color(0xff, 0xff, 0xff);
				aState[1]++;
			}
		}
	}
	else if (aState[1] == 5)
	{
		// the bricks are shot up the column
		if (mUpdateCnt % 0x19 == 0)
			mRightBlink = !mRightBlink;
		Cell* aSecond = mRight[2][3];
		aSecond->mY += aSecond->mSpeed;
		int aReached = (aSecond->mY <= aSecond->mTargetY) ? 1 : 0;
		if (aReached)
			aSecond->mY = aSecond->mTargetY;
		Cell* aFirst = mRight[2][2];
		aFirst->mY += aFirst->mSpeed;
		if (aFirst->mY <= aFirst->mTargetY)
		{
			aReached++;
			aFirst->mY = aFirst->mTargetY;
		}
		if (aReached == 2)
		{
			mRightMouse = 0;
			aState[1]++;
			mRightCount = 0;
			for (size_t aRow = 0; aRow < mRight.size(); aRow++)
			{
				for (size_t aCol = 0; aCol < mRight[aRow].size(); aCol++)
				{
					if (mRight[aRow][aCol]->mDeathImage != NULL)
						mRight[aRow][aCol]->mAnim.SetStopped(false);
				}
			}
		}
	}
	else if (aState[1] == 6)
	{
		// the group breaks up: when the four bricks of it are done the brick over it comes down
		int aDone = 0;
		if (mUpdateCnt % 0x19 == 0)
			mRightBlink = !mRightBlink;
		for (size_t aRow = 0; aRow < mRight.size(); aRow++)
		{
			for (size_t aCol = 0; aCol < mRight[aRow].size(); aCol++)
			{
				Cell* aCell = mRight[aRow][aCol];
				if (aCell->mDeathImage != NULL)
				{
					aCell->mAnim.Update();
					FrameAnim& anAnim = aCell->mAnim;
					if (anAnim.mDone || (anAnim.mMaxLoops > 0 && anAnim.mMaxLoops <= anAnim.mLoopCount) || anAnim.mStopped)
						aDone++;
				}
			}
		}
		if (aDone == 4)
		{
			mRight[2][5]->mSpeed = -2;
			mRight[2][5]->mTargetY = kRightY;
			mRightCount = 0;
			aState[1]++;
		}
	}
	else if (aState[1] == 7)
	{
		Cell* aBrick = mRight[2][5];
		aBrick->mY += aBrick->mSpeed;
		if (aBrick->mY <= aBrick->mTargetY)
		{
			aBrick->mY = aBrick->mTargetY;
			mRightCount++;
			if (mRightCount >= 200)
			{
				// the round starts again with all the bricks
				aState[1] = 0;
				mRightCount = 0;
				mRightBeamColor = Color(0xff, 0xff, 0xff);
				mRightShipX = kRightX + IMAGE_HELP_BRICK_BLUE->mWidth * 2;
				mRightMouse = 1;
				mRightColumn = 2;
				mRightBlink = true;
				mRightPointerX = mRightShipX;
				mPointerY = kRightBeamY - 0x14 + IMAGE_HELP_BRICK_BLUE->mHeight * 4;
				ClearRight();
				mRight.clear();
				mRight.resize(4);
				FillRight();
			}
		}
	}

	// ---- the supa weapon
	if (aState[2] == 0)
	{
		mSupaCharge++;
		if (IMAGE_HELP_SUPA_CHARGED->mWidth <= mSupaCharge)
		{
			mSupaCharge = IMAGE_HELP_SUPA_CHARGED->mWidth;
			aState[2]++;
		}
	}
	else if (aState[2] == 1)
	{
		mSupaCount++;
		if (mSupaCount >= 200)
		{
			mSupaCount = 0;
			aState[2]++;
		}
	}
	else if (aState[2] == 2)
	{
		mSupaPointerY--;
		if (mSupaPointerY <= 0x124)
		{
			mSupaPointerY = 0x124;
			mSupaCount++;
			if (mSupaCount >= 0x4b)
			{
				mSupaCount = 0;
				aState[2]++;
			}
		}
	}
	else
	{
		if (mUpdateCnt % 0x19 == 0)
			mSupaBlink = !mSupaBlink;
		mSupaCount++;
		if (mSupaCount >= 200)
		{
			mSupaCount = 0;
			aState[2] = 0;
			mSupaCharge = 0;
			mSupaPointerX = kSupaWidth / 2 + 0x28 + kSupaX;
			mSupaPointerY = 0x145;
			mSupaBlink = true;
		}
	}

	MarkDirty();
}
