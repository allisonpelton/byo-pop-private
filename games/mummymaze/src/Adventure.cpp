// The adventure mode's world map (WinMM.exe 0x417010 and its methods, 0x427050's pyramid buttons) and the saved
// adventures (data\saves\adv<n>.sav, here adv<n>.sav in the user folder: 0x4195d0 writes one, the map's constructor reads
// it). The slot chooser, the new-game and delete dialogs are in Dialogs.cpp and the app's methods that tie them together
// at the end of this file.
//
// A saved adventure: the player's name (int length, characters), the format number 2, the name again (the app's +0x104cc),
// the progress of the fifteen pyramids (a Buffer's "web string": one byte each, 0xff for a pyramid that is still shut, else
// 0 to 15 chambers done), the pyramid the marker stands on (a byte), the time played (a double) and the number of times
// the explorer was caught (an int).

#include "Screens.h"
#include "MummyApp.h"
#include "Board.h"
#include "Dialogs.h"
#include "GameText.h"
#include "Res.h"
#include "Buffer.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

#include "Tables.inc"

// ---------------------------------------------------------------------------------------------------------
// the files

static std::string AdventureFileName(int theSlot)
{
	char aBuf[32];
	sprintf(aBuf, "adv%d.sav", theSlot);
	return aBuf;
}

bool MummyApp::AdventureExists(int theSlot)
{
	std::string aData;
	return Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + AdventureFileName(theSlot), &aData);
}

namespace
{

struct AdventureReader
{
	const std::string&	mData;
	size_t				mPos;
	bool				mOk;

	AdventureReader(const std::string& theData) : mData(theData), mPos(0), mOk(true) {}

	int Int()
	{
		int aValue = 0;
		if (mPos + 4 > mData.size())
		{
			mOk = false;
			return 0;
		}
		memcpy(&aValue, mData.data() + mPos, 4);
		mPos += 4;
		return aValue;
	}
	int Byte()
	{
		if (mPos + 1 > mData.size())
		{
			mOk = false;
			return 0;
		}
		return (unsigned char) mData[mPos++];
	}
	double Double()
	{
		double aValue = 0;
		if (mPos + 8 > mData.size())
		{
			mOk = false;
			return 0;
		}
		memcpy(&aValue, mData.data() + mPos, 8);
		mPos += 8;
		return aValue;
	}
	std::string String()
	{
		int aLen = Int();
		if (!mOk || aLen < 0 || mPos + (size_t) aLen > mData.size())
		{
			mOk = false;
			return "";
		}
		std::string aString = mData.substr(mPos, aLen);
		mPos += aLen;
		return aString;
	}
};

void PutInt(std::string& theData, int theValue)
{
	theData.append((const char*) &theValue, 4);
}

void PutString(std::string& theData, const std::string& theString)
{
	PutInt(theData, (int) theString.size());
	theData += theString;
}

}

// the name that a saved adventure carries ("" when there is none)
bool MummyApp::AdventureName(int theSlot, SexyString& theName)
{
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + AdventureFileName(theSlot), &aData))
		return false;
	AdventureReader aReader(aData);
	theName = aReader.String();
	return true;
}

// 0x41bb10's friend 0x41bc80: the file goes
void MummyApp::DeleteAdventure(int theSlot)
{
	Platform::RemoveUserFile(AdventureFileName(theSlot));
	ShowAdventureSlots();
}

// a new adventure (0x4201e0 case 0x7db): a saved game with nothing done yet
void MummyApp::CreateAdventure(int theSlot, const SexyString& theName)
{
	std::string aData;
	PutString(aData, theName);
	PutInt(aData, 2);
	PutString(aData, theName);
	Buffer aBuffer;
	for (int i = 0; i < 15; i++)
		aBuffer.WriteByte(0xff);
	PutString(aData, aBuffer.ToWebString());
	aData += (char) 0;
	double aTime = 0;
	aData.append((const char*) &aTime, 8);
	PutInt(aData, 0);
	mAdvName = theName;
	mTotalTime = 0;
	mDeaths = 0;
	Platform::WriteUserFileAtomic(AdventureFileName(theSlot), aData);
}

// ---------------------------------------------------------------------------------------------------------
// PyramidButton

// 0x427050: the pictures are the ones the map's constructor gives it (the over and the down picture are the same)
PyramidButton::PyramidButton(int theId, ButtonListener* theListener, MummyApp* theApp) :
	ButtonWidget(theId, theListener),
	mApp(theApp),
	mHover(false),
	mEnabled(true)
{
}

// 0x4271c0 (the game's own copy of the stock button's drawing: a picture for each state, no label)
void PyramidButton::Draw(Graphics* g)
{
	bool aPressed = mIsDown && mIsOver && !mDisabled;
	if (mInverted)
		aPressed = !aPressed;
	if (!aPressed)
	{
		Image* anImage = NULL;
		if (mHover && !mEnabled && mDisabledImage != NULL)
			anImage = mDisabledImage;
		else if ((mIsOver || mIsDown) && mOverImage != NULL)
			anImage = mOverImage;
		else
			anImage = mButtonImage;
		if (anImage != NULL)
			g->DrawImage(anImage, 0, 0);
	}
	else if (mDownImage != NULL)
		g->DrawImage(mDownImage, 0, 0);
	else if (mButtonImage != NULL)
		g->DrawImage(mButtonImage, 1, 1);
}

// 0x427d50, 0x427d70
void PyramidButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	mHover = true;
}

void PyramidButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	mHover = false;
}

// 0x4276d0: a click on a pyramid that is shut says why
void PyramidButton::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	ButtonWidget::MouseDown(x, y, theBtnNum, theClickCount);
	if (mDisabled || mEnabled)
		return;
	WorldMap* aMap = mApp->mWorldMap;
	if (aMap == NULL)
		return;
	if (mId == 15 && aMap->mProgress[14] < 0)
		mApp->DoMessage(DIALOG_MAP_MESSAGE, "", GT(0x00495348), 3, 0xa2, 0x52, 0x13c);
	else if (aMap->mProgress[mId - 1] < 0)
		mApp->DoMessage(DIALOG_MAP_MESSAGE, "", GT(0x004952b4), 3, 0xa2, 0x52, 0x13c);
	else if (aMap->mProgress[mId - 1] == 0xf)
		mApp->DoMessage(DIALOG_MAP_MESSAGE, "", GT(0x00495270), 3, 0xa2, 0x52, 0x13c);
	MarkDirty();
}

// 0x427d00: only a pyramid that is open answers
void PyramidButton::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	Widget::MouseUp(x, y, theBtnNum, theClickCount);
	MarkDirty();
	if (mIsOver && !mDisabled && mEnabled && mWidgetManager->mHasFocus)
		mButtonListener->ButtonDepress(mId);
}

MapHotspot::MapHotspot(int theId, ButtonListener* theListener) :
	ButtonWidget(theId, theListener)
{
}

void MapHotspot::Draw(Graphics* g)
{
	(void) g;
}

// ---------------------------------------------------------------------------------------------------------
// WorldMap

// 0x417010
WorldMap::WorldMap(MummyApp* theApp, int theSlot, bool theCheckEntry) :
	mApp(theApp),
	mCurrent(0),
	mTarget(0),
	mNext(0),
	mStep(-1),
	mHoverShown(-1),
	mHoverAlpha(0),
	mHoverFading(false),
	mOpened(false),
	mCheckEntry(theCheckEntry),
	mInvalid(false)
{
	mDoFinger = true;
	mChamberDone = mApp->mChamberDone;
	mApp->mChamberDone = false;
	for (int i = 0; i < 15; i++)
		mProgress[i] = -1;
	mProgress[0] = 0;

	// the saved adventure
	std::string aData;
	bool aRead = Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + AdventureFileName(theSlot), &aData);
	AdventureReader aReader(aData);
	std::string aName = aReader.String();
	int aVersion = aReader.Int();
	if (!aRead || !aReader.mOk || aVersion != 2)
		mInvalid = true;
	else
	{
		mApp->mAdvName = aReader.String();
		std::string aProgress = aReader.String();
		int aCurrent = aReader.Byte();
		double aTime = aReader.Double();
		int aDeaths = aReader.Int();
		if (!aReader.mOk)
			mInvalid = true;
		else
		{
			Buffer aBuffer;
			aBuffer.FromWebString(aProgress);
			for (int i = 0; i < 15; i++)
			{
				int aValue = aBuffer.ReadByte();
				if (aValue < 0x10)
					mProgress[i] = aValue;
			}
			mProgress[0] = mProgress[0] < 0 ? 0 : mProgress[0];
			mCurrent = mTarget = (aCurrent >= 0 && aCurrent < 15) ? aCurrent : 0;
			mApp->mTotalTime = aTime;
			mApp->mDeaths = aDeaths;
		}
	}
	if (mInvalid)
	{
		mApp->ShowMainMenu(2, true);
		mApp->DoMessage(DIALOG_BAD_SAVE, GT(0x004944bc), GT(0x004944d0), 3, 0xa2, 0x52, 0x13c);
	}
	else if (mChamberDone)
	{
		// the chamber that has just been won
		mCurrent = mTarget = mApp->mAdvPyramid;
		mProgress[mCurrent]++;
		Unlock();
		Save();
	}

	mEnterButton = new ButtonWidget(99, this);
	mEnterButton->mDoFinger = true;
	mEnterButton->mButtonImage = mApp->mImages[IMAGE_ADVBUTTONS];
	mEnterButton->mMouseInsets = Insets(-1, -1, -1, -1);
	if (mProgress[mCurrent] == 0xf)
		mEnterButton->SetVisible(false);
	mBackButton = new ButtonWidget(0x62, this);
	mBackButton->mDoFinger = true;
	mBackButton->mButtonImage = mApp->mImages[IMAGE_ADVSAVEANDQUIT];
	mBackButton->mMouseInsets = Insets(-1, -1, -1, -1);
	for (int i = 0; i < 15; i++)
	{
		PyramidButton* aButton = new PyramidButton(i + 1, this, mApp);
		aButton->mDoFinger = true;
		aButton->mDisabledImage = mApp->mImages[IMAGE_PYRAMIDDOWN];
		aButton->mOverImage = mApp->mImages[IMAGE_PYRAMIDOVER];
		aButton->mDownImage = mApp->mImages[IMAGE_PYRAMIDDOWN];
		aButton->mMouseInsets = Insets(-1, -1, -1, -1);
		if (mProgress[i] < 0 || mProgress[i] > 14)
		{
			aButton->mEnabled = false;
			aButton->mDoFinger = false;
		}
		mPyramids[i] = aButton;
	}
	mStatsButton = new MapHotspot(0x32, this);
	mStatsButton->mDoFinger = true;
	mPlayerButton = new MapHotspot(0x33, this);
	mPlayerButton->mDoFinger = true;
	mChestButton = new MapHotspot(0x34, this);
	mChestButton->mDoFinger = true;

	mMarkerX = kMapPos[mCurrent * 2] - 0x1a;
	mMarkerY = kMapPos[mCurrent * 2 + 1] - 0x1a;
	for (int i = 0; i < 18; i++)
	{
		mSparkFrame[i] = CrtRand() % 0xf;
		mSparkX[i] = CrtRand() % 0x1c;
		mSparkY[i] = CrtRand() % 0x19;
	}
	for (int i = 0; i < 15; i++)
		mDistance[i] = 99;
	mApp->SetMusicOrder(0x43, true);
}

WorldMap::~WorldMap()
{
	delete mEnterButton;
	delete mBackButton;
	delete mStatsButton;
	delete mPlayerButton;
	delete mChestButton;
	for (int i = 0; i < 15; i++)
		delete mPyramids[i];
}

// 0x4187d0
void WorldMap::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mEnterButton);
	theWidgetManager->AddWidget(mBackButton);
	theWidgetManager->AddWidget(mStatsButton);
	theWidgetManager->AddWidget(mPlayerButton);
	theWidgetManager->AddWidget(mChestButton);
	for (int i = 0; i < 15; i++)
		theWidgetManager->AddWidget(mPyramids[i]);
}

// 0x418840
void WorldMap::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mEnterButton);
	theWidgetManager->RemoveWidget(mBackButton);
	theWidgetManager->RemoveWidget(mStatsButton);
	theWidgetManager->RemoveWidget(mPlayerButton);
	theWidgetManager->RemoveWidget(mChestButton);
	for (int i = 0; i < 15; i++)
		theWidgetManager->RemoveWidget(mPyramids[i]);
}

// 0x4188b0
void WorldMap::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mBackButton->Resize(0, 0x18d, 0xb5, 0x2d);
	mEnterButton->Resize(-5, 0x1af, 0x118, 0x2d);
	mStatsButton->Resize(0x19f, 0, 0x3c, 0x42);
	mPlayerButton->Resize(0x13f, 0x1ab, 0x8d, 0x35);
	mChestButton->Resize(0x208, 0, 0x78, 0x1e0);
	for (int i = 0; i < 15; i++)
		mPyramids[i]->Resize(kMapPos[i * 2] - 0x1a, kMapPos[i * 2 + 1] - 0x1d, 0x34, 0x34);
}

// 0x418970: a pyramid that was shut is open now (nothing done in it yet)
void WorldMap::Open(int thePyramid)
{
	if (mProgress[thePyramid] < 0)
		mProgress[thePyramid] = 0;
}

// 0x419940: what a solved pyramid opens: its neighbours on the map, and the last pyramid once the others are solved
void WorldMap::Unlock()
{
	static const int kOpens[14][4] =
	{
		{1, 6, -1, -1}, {2, -1, -1, -1}, {1, 3, -1, -1}, {2, 4, -1, -1}, {3, 5, -1, -1}, {4, 8, 9, -1}, {7, -1, -1, -1},
		{6, 8, 10, -1}, {5, 7, 9, 10}, {5, 8, 12, -1}, {7, 8, 11, -1}, {10, 12, -1, -1}, {9, 11, 13, -1}, {12, -1, -1, -1}
	};
	for (int i = 0; i < 15; i++)
	{
		if (mProgress[i] == 0xf && i < 14)
		{
			for (int j = 0; j < 4 && kOpens[i][j] >= 0; j++)
				Open(kOpens[i][j]);
		}
		bool aAllDone = true;
		for (int j = 0; j < 14; j++)
		{
			if (mProgress[j] < 0xf)
				aAllDone = false;
		}
		if (aAllDone)
			Open(14);
	}
}

// 0x418a30: how many paths away each pyramid is from one (a search; 99 for the ones not reached)
void WorldMap::Measure(int thePyramid, int theDistance)
{
	mDistance[thePyramid] = theDistance;
	for (int i = 0; i < 15; i++)
	{
		if (kMapLinks[thePyramid * 15 + i] == 1 && mProgress[i] > -1 && theDistance + 1 < mDistance[i])
			Measure(i, theDistance + 1);
	}
}

// 0x4189b0: the next pyramid on the way from one to another (the nearest of the neighbours of theFrom to theTo)
int WorldMap::NextHop(int theFrom, int theTo)
{
	if (theFrom == theTo)
		return theFrom;
	for (int i = 0; i < 15; i++)
		mDistance[i] = 99;
	Measure(theTo, 0);
	int aBest = 99;
	int aNext = 99;
	for (int i = 0; i < 15; i++)
	{
		if (kMapLinks[theFrom * 15 + i] == 1 && mProgress[i] > -1 && mDistance[i] < aBest)
		{
			aBest = mDistance[i];
			aNext = i;
		}
	}
	return aNext;
}

// the pyramids that can be entered (open and not solved) answer clicks again
void WorldMap::EnableOpenPyramids()
{
	for (int i = 0; i < 15; i++)
	{
		if (mProgress[i] > -1 && mProgress[i] < 0xf)
		{
			mPyramids[i]->mEnabled = true;
			mPyramids[i]->mDoFinger = true;
		}
	}
}

// 0x4195d0
void WorldMap::Save()
{
	if (mInvalid)
		return;
	std::string aOld;
	std::string aName;
	if (Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + AdventureFileName(mApp->mCurrentSlot), &aOld))
	{
		AdventureReader aReader(aOld);
		aName = aReader.String();
		if (!aReader.mOk)
			aName = mApp->mAdvName;
	}
	else
		aName = mApp->mAdvName;
	std::string aData;
	PutString(aData, aName);
	PutInt(aData, 2);
	PutString(aData, mApp->mAdvName);
	Buffer aBuffer;
	for (int i = 0; i < 15; i++)
		aBuffer.WriteByte((uchar) mProgress[i]);
	PutString(aData, aBuffer.ToWebString());
	aData += (char) mCurrent;
	double aTime = mApp->mTotalTime;
	aData.append((const char*) &aTime, 8);
	PutInt(aData, mApp->mDeaths);
	Platform::WriteUserFileAtomic(AdventureFileName(mApp->mCurrentSlot), aData);
}

// 0x4183c0
void WorldMap::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_ADVENTUREMAP], 0, 0);
	g->SetColorizeImages(true);
	g->SetColor(Color(0, 0, 0, mHoverAlpha));
	if (mHoverShown >= 0)
		g->DrawImage(mApp->mImages[IMAGE_BIGMAPSTRIP], 0xe1, 0, Rect(0, mHoverShown * 0x6c, 0x101, 0x6c));
	g->SetColorizeImages(false);
	for (int i = 0; i < 15; i++)
	{
		if (mProgress[i] > -1)
		{
			g->DrawImage(mApp->mImages[IMAGE_PYRAMIDDARK], kMapPos[i * 2] - 0xe, kMapPos[i * 2 + 1] - 0xe);
			g->DrawImage(mApp->mImages[IMAGE_PYRAMIDLIT], kMapPos[i * 2] - 0xe, kMapPos[i * 2 + 1] - 0xe,
				Rect(mProgress[i] * 0x1c, 0, 0x1c, 0x1c));
		}
	}
	for (int i = 0; i < 18; i++)
	{
		g->SetDrawMode(1);
		g->DrawImage(mApp->mImages[IMAGE_FOGPUFF], mSparkX[i] + 0xb3, mSparkY[i] + 0x77, Rect(mSparkFrame[i] * 0x1e, 0, 0x1e, 0x2a));
		g->SetDrawMode(0);
	}
	g->DrawImage(mApp->mImages[IMAGE_MAPHEAD], mMarkerX, mMarkerY);
	// the treasures found (a picture for each solved pyramid; the ones of 9 to 11 and the last come below)
	for (int i = 0; i < 15; i++)
	{
		if ((i < 9 || i > 11) && mProgress[i] == 0xf && i < 14)
		{
			int aCell = abs(i & 1);
			g->DrawImage(mApp->mImages[IMAGE_BIGTREASURES], aCell * 0x34 + 0x217, (i / 2) * 0x33 + 0x13,
				Rect(aCell * 0x34, (i / 2) * 0x33, 0x34, 0x33));
		}
	}
	if (mProgress[9] == 0xf)
		g->DrawImage(mApp->mImages[IMAGE_BIGTREASURES], 0x217, 0x112, Rect(0, 0xff, 0x34, 0x33));
	if (mProgress[10] == 0xf)
		g->DrawImage(mApp->mImages[IMAGE_BIGTREASURES], 0x24b, 0x112, Rect(0x34, 0xff, 0x34, 0x33));
	if (mProgress[11] == 0xf)
		g->DrawImage(mApp->mImages[IMAGE_BIGTREASURES], 0x24b, 0xdf, Rect(0x34, 0xcc, 0x34, 0x33));
	if (mProgress[14] == 0xf)
		g->DrawImage(mApp->mImages[IMAGE_BIGTREASURES], 0x226, 0x17b, Rect(0xf, 0x168, 0xf, 0xf));
}

// 0x417c10
void WorldMap::Update()
{
	Widget::Update();
	if (mApp->mGameMode == MODE_ADVENTURE_DONE)
		mApp->mGameMode = MODE_ADVENTURE;
	if (mCheckEntry)
	{
		if (mProgress[mCurrent] < 0xf)
		{
			// a chamber has been won and the pyramid goes on: straight into the next chamber
			mApp->mAdvPyramid = mCurrent;
			mApp->mAdvChamber = mProgress[mCurrent];
			mApp->StartBoardScreen();
			mApp->RemoveWorldMap();
			return;
		}
		if (mApp->mGameMode != MODE_ADVENTURE_DONE)
		{
			// the pyramid is solved: its treasure on the ending board
			Unlock();
			mApp->mGameMode = MODE_ADVENTURE_DONE;
			mApp->mAdvChamber = 0xe;
			mApp->StartBoardScreen();
			if (mProgress[14] == 0xf)
			{
				// the last pyramid: the game is won and the map starts again
				for (int i = 0; i < 15; i++)
				{
					mProgress[i] = 0;
					mPyramids[i]->mEnabled = true;
					mPyramids[i]->mDoFinger = true;
					mCurrent = 0;
					mTarget = 0;
				}
			}
			mApp->RemoveWorldMap();
			return;
		}
	}
	if (mApp->mIsRegistered && !mOpened)
	{
		// (the first time: what the solved pyramids have opened can be entered)
		mOpened = true;
		Unlock();
		EnableOpenPyramids();
	}

	// which pyramid's name is shown
	int aHover = -1;
	for (int i = 0; i < 15; i++)
	{
		if (mPyramids[i]->mHover)
		{
			mHoverFading = false;
			aHover = i;
		}
	}
	if (aHover != mHoverShown)
	{
		if (aHover == -1)
			mHoverFading = true;
		else
		{
			mHoverShown = aHover;
			mHoverAlpha = 0;
		}
		MarkDirty();
	}

	// the marker's walk
	if (mCurrent != mTarget && mStep < 0)
	{
		mNext = NextHop(mCurrent, mTarget);
		mStep = 0;
	}
	if (mStep >= 0)
	{
		int anEdge = kMapEdge[mCurrent * 15 + mNext];
		if (mNext < 0 || mNext > 14 || anEdge == 0)
		{
			mNext = mCurrent;
			mStep = -1;
			return;
		}
		int aX = mMarkerX;
		int aY = mMarkerY;
		if (mStep < 6)
		{
			int anIndex = (anEdge <= 0) ? (abs(anEdge) * 6 + 5 - mStep) : (mStep + abs(anEdge) * 6);
			aX = kMapWay[anIndex * 2] - 10;
			aY = kMapWay[anIndex * 2 + 1] - 10;
		}
		else if (mStep == 6)
		{
			aX = kMapPos[mNext * 2] - 0x1a;
			aY = kMapPos[mNext * 2 + 1] - 0x1a;
		}
		if (mMarkerX < aX)
			mMarkerX++;
		else if (aX < mMarkerX)
			mMarkerX--;
		if (mMarkerY < aY)
			mMarkerY++;
		else if (aY < mMarkerY)
			mMarkerY--;
		if (mMarkerX == aX && mMarkerY == aY)
			mStep++;
		if (mStep == 7 || (mStep == 6 && mNext != mTarget))
		{
			mStep = -1;
			mCurrent = mNext;
			if (mNext == mTarget)
			{
				EnableOpenPyramids();
				mEnterButton->SetDisabled(false);
				mEnterButton->SetVisible(mProgress[mCurrent] < 0xf);
			}
		}
		MarkDirty();
	}

	// the name's picture fades in and out
	if (mHoverAlpha < 0xb4 && !mHoverFading)
	{
		mHoverAlpha += 3;
		MarkDirty();
	}
	else if (mHoverAlpha > 0 && mHoverFading)
	{
		mHoverAlpha -= 3;
		MarkDirty();
	}

	// the twinkles drift across the sand
	if (mUpdateCnt % 0xe == 0)
	{
		for (int i = 0; i < 18; i++)
		{
			mSparkFrame[i]++;
			if (mSparkFrame[i] == 0xf)
				mSparkFrame[i] = CrtRand() % 0xf;
			int aSpeed = (i < 9) ? 1 : 2;
			if (i & 1)
				aSpeed = -aSpeed;
			mSparkX[i] += aSpeed;
			if (mSparkX[i] >= 0x1c)
			{
				mSparkX[i] = 0;
				mSparkY[i] = abs(CrtRand() % 0x19);
			}
			else if (mSparkX[i] < 0)
			{
				mSparkX[i] = 0x1b;
				mSparkY[i] = abs(CrtRand() % 0x19);
			}
		}
		MarkDirty();
	}
}

// 0x418a90
void WorldMap::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}

// 0x418aa0
void WorldMap::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0x32:
	{
		// the statistics: the time, the chambers and pyramids done, and how often the explorer was caught
		int aTotal = (int) mApp->mTotalTime;
		int aHours = aTotal / 3600;
		int aMinutes = (aTotal / 60) % 60;
		int aSeconds = aTotal % 60;
		int aChambers = 0;
		int aPyramids = 0;
		for (int i = 0; i < 15; i++)
		{
			if (mProgress[i] > 0)
				aChambers += mProgress[i];
			if (mProgress[i] == 0xf)
				aPyramids++;
		}
		char aBuf[200];
		SexyString aText;
		sprintf(aBuf, GTC(0x004923f8), aHours);
		aText += aBuf;
		if (aHours != 1)
			aText += "s";
		sprintf(aBuf, GTC(0x004923e8), aMinutes);
		aText += aBuf;
		if (aMinutes != 1)
			aText += "s";
		sprintf(aBuf, GTC(0x004923d8), aSeconds);
		aText += aBuf;
		if (aSeconds != 1)
			aText += "s";
		sprintf(aBuf, GTC(0x00492390), aChambers, aPyramids);
		aText += aBuf;
		if (aPyramids != 1)
			aText += "s";
		sprintf(aBuf, GTC(0x00492360), mApp->mDeaths);
		aText += aBuf;
		if (mApp->mDeaths != 1)
			aText += "s";
		aText += ".";
		mApp->DoMessage(DIALOG_ADV_INFO, GT(0x00492350), aText, 3, 0xa2, 0x52, 0x13c);
		return;
	}
	case 0x33:
	{
		SexyString aName;
		mApp->AdventureName(mApp->mCurrentSlot, aName);
		char aBuf[200];
		sprintf(aBuf, GTC(0x00492338), aName.c_str());
		mApp->DoMessage(DIALOG_ADV_INFO, GT(0x00492240), aBuf, 3, 0xa2, 0x52, 0x13c);
		return;
	}
	case 0x34:
		mApp->DoMessage(DIALOG_INTRO, GT(0x004922d4), GT(0x004922e0), 3, 0xa2, 0x52, 0x13c);
		return;
	case 99:
		// into the pyramid that the marker stands on
		if (mProgress[mCurrent] == mApp->mAdvChamber && mCurrent == mApp->mAdvPyramid && mApp->mBoard != NULL)
		{
			mApp->RemoveWorldMap();
			return;
		}
		mApp->mAdvPyramid = mCurrent;
		mApp->mAdvChamber = mProgress[mCurrent];
		mApp->StartBoardScreen();
		mApp->RemoveWorldMap();
		return;
	case 0x62:
		mApp->ShowMainMenu(2, true);
		mApp->SetMusicOrder(1, true);
		mApp->RemoveWorldMap();
		return;
	default:
		break;
	}
	if (theId < 1 || theId > 15)
		return;
	if (theId - 1 != mCurrent)
	{
		// walk there
		mTarget = theId - 1;
		for (int i = 0; i < 15; i++)
		{
			mPyramids[i]->mEnabled = false;
			mPyramids[i]->mDoFinger = false;
		}
		mEnterButton->SetDisabled(true);
		mEnterButton->SetVisible(false);
		return;
	}
	if (mStep >= 0)
		return;
	mApp->mAdvPyramid = mCurrent;
	mApp->mAdvChamber = mProgress[mCurrent];
	mApp->StartBoardScreen();
	mApp->RemoveWorldMap();
}

// ---------------------------------------------------------------------------------------------------------
// the app's side

// 0x41c300: the world map goes, and the adventure is saved
void MummyApp::RemoveWorldMap()
{
	if (mWorldMap != NULL)
	{
		mWorldMap->Save();
		mWidgetManager->RemoveWidget((Widget*) mWorldMap);
		SafeDeleteWidget((Widget*) mWorldMap);
		mWorldMap = NULL;
	}
}

// 0x41b800: the games that were saved, or a new one when there are none
void MummyApp::ShowAdventureSlots()
{
	bool aAny = false;
	for (int i = 0; i < 5; i++)
	{
		if (AdventureExists(i))
			aAny = true;
	}
	if (aAny)
	{
		AdventureSlotsDialog* aDialog = (AdventureSlotsDialog*) NewDialogOf(new AdventureSlotsDialog(this));
		aDialog->Resize((mWidth - 0x17c) / 2, (mHeight - aDialog->GetPreferredHeight(0x17c)) / 2, 0x17c,
			aDialog->GetPreferredHeight(0x17c));
		AddDialog(DIALOG_ADVENTURE_NEW, aDialog);
	}
	else
	{
		mCurrentSlot = 0;
		ShowNewAdventure();
	}
}

void MummyApp::ShowNewAdventure()
{
	NewAdventureDialog* aDialog = (NewAdventureDialog*) NewDialogOf(new NewAdventureDialog(this));
	aDialog->Resize((mWidth - 0x17c) / 2, (mHeight - aDialog->GetPreferredHeight(0x17c)) / 2, 0x17c,
		aDialog->GetPreferredHeight(0x17c));
	AddDialog(DIALOG_NEW_ADVENTURE_NAME, aDialog);
}

// 0x41bb10: the games to delete
void MummyApp::ShowDeleteAdventure()
{
	DeleteAdventureDialog* aDialog = (DeleteAdventureDialog*) NewDialogOf(new DeleteAdventureDialog(this));
	aDialog->Resize((mWidth - 0x17c) / 2, (mHeight - aDialog->GetPreferredHeight(0x17c)) / 2, 0x17c,
		aDialog->GetPreferredHeight(0x17c));
	AddDialog(DIALOG_DELETE_ADVENTURE, aDialog);
}

// 0x41bcc0: the saved adventure of a slot on the world map, or the box that asks for a name when there is none
void MummyApp::LoadAdventure(int theSlot, bool theFlag)
{
	RemoveWorldMap();
	mCurrentSlot = theSlot;
	if (!AdventureExists(theSlot))
	{
		ShowNewAdventure();
		return;
	}
	WorldMap* aMap = new WorldMap(this, theSlot, theFlag);
	if (aMap->mInvalid)
	{
		delete aMap;
		return;
	}
	mWorldMap = aMap;
	mWorldMap->mDoFinger = true;
	mWorldMap->Resize(0, 0, 0x280, 0x1e0);
	mWidgetManager->AddWidget(mWorldMap);
	if (mWorldMap->mProgress[0] == 0xf && mWorldMap->mCurrent == 0 && mGameMode == MODE_ADVENTURE_DONE)
		DoMessage(DIALOG_INTRO, GT(0x00493f54), GT(0x00493f68), 3, 0x6e, 0x55, 0x1a4);
}

// 0x406f10, 0x407cc0 and 0x4083e0: the explorer was caught in the adventure: the saved game is read again (the maze starts
// over from the map), the catch is counted and the game saved
void MummyApp::AdventureCaught()
{
	WorldMap* aMap = new WorldMap(this, mCurrentSlot, false);
	if (aMap->mInvalid)
	{
		delete aMap;
		return;
	}
	mWorldMap = aMap;
	mDeaths++;
	RemoveWorldMap();
}

// 0x413950 (cases 2 and 3) and 0x42a3d0 (case 13): the way out of a maze to the map: the time played in it is added
void MummyApp::AdventureLeave()
{
	WorldMap* aMap = new WorldMap(this, mCurrentSlot, false);
	if (aMap->mInvalid)
	{
		delete aMap;
		return;
	}
	mWorldMap = aMap;
	DWORD aNow = GetTickCount();
	mTotalTime += (double) (int) (mElapsedBase / 100 + (aNow - (mBoard != NULL ? mBoard->mStartTick : aNow)) / 1000);
	RemoveWorldMap();
}

// 0x42a3d0 case 13: the world map button of the side panel
void MummyApp::OnWorldMapButton()
{
	if (mBoard == NULL)
	{
		if (mVictory == NULL)
		{
			double aTime = mTotalTime;
			LoadAdventure(mCurrentSlot, false);
			mTotalTime = aTime;
		}
	}
	else if (mVictory == NULL)
	{
		DWORD aNow = GetTickCount();
		DWORD aStart = mBoard->mStartTick;
		int aBase = (int) mElapsedBase;
		RemoveWorldMap();
		RemoveBoard();
		LoadAdventure(mCurrentSlot, false);
		mTotalTime = (double) (int) (aBase / 100 + (aNow - aStart) / 1000) + mTotalTime;
	}
}
