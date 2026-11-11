#include "LayoutSelect.h"

#include "EngineRes.h"
#include "Font.h"
#include "GameDialogs.h"
#include "Globals.h"
#include "Image.h"
#include "Layout.h"
#include "MainMenu.h"
#include "Manager.h"
#include "MapLayer.h"
#include "PlayLayer.h"
#include "Res.h"
#include "User.h"

#include <cstdio>
#include <cstdlib>

namespace mahjongjapan
{

// The layout ids of the list (0x51c4c4): the 125 of 437 on, then the 37 of 600 on
static int ListLayoutId(int theIndex)
{
	return theIndex < 125 ? 437 + theIndex : 600 + (theIndex - 125);
}

// 0x402f80
int GetLayoutId(int theIndex)
{
	if (-1 < theIndex && theIndex < gLayoutCount)
		return ListLayoutId(theIndex);
	return 0;
}

// 0x4019b0
int CountBackgrounds()
{
	return GetProgressStage() + 9;
}

// 0x401060
void BackgroundPicture::OnInit()
{
	SetUserFlag(true);
}

// 0x401070: the picture in its frame, ringed when chosen
void BackgroundPicture::DrawNormal()
{
	if (mBackground != -1)
	{
		jb::Rect aRect = {0, 0, 0xc3, 5};
		DrawImage(0, 0, IMAGE_BACKGROUND_LOCK, &aRect, true, 100, 100, 0xff);
		aRect = {0, 0x6f, 0xc3, 0x75};
		DrawImage(0, 0x6f, IMAGE_BACKGROUND_LOCK, &aRect, true, 100, 100, 0xff);
		aRect = {0, 5, 4, 0x6f};
		DrawImage(0, 5, IMAGE_BACKGROUND_LOCK, &aRect, true, 100, 100, 0xff);
		aRect = {0xbd, 5, 0xc3, 0x6f};
		DrawImage(0xbd, 5, IMAGE_BACKGROUND_LOCK, &aRect, true, 100, 100, 0xff);
		aRect.left = (mBackground % 3) * 0xba;
		aRect.right = aRect.left + 0xb9;
		aRect.top = (mBackground / 3) * 0x6b;
		aRect.bottom = aRect.top + 0x6a;
		DrawImage(4, 5, IMAGE_BACKGROUND_THUMBS, &aRect, true, 100, 100, 0xff);
		SetTrackDirty(false);
		if (IsChecked())
			DrawImage(-0xd, -0xc, IMAGE_BACKGROUND_GLOW, nullptr, true, 100, 100, 0xff);
		SetTrackDirty(true);
		return;
	}
	DrawImage(0, 0, IMAGE_BACKGROUND_LOCK, nullptr, true, 100, 100, 0xff);
}

// 0x401250
bool BackgroundPicture::OnLButtonDblClk(int theFlags, int theX, int theY)
{
	jb::PlaySound(jb::SOUND_CLICK);
	((BackgroundBox*)mLayer)->Apply();
	return true;
}

// 0x401340
void BackgroundBox::Create()
{
	int aLeft[] = {2, 1, 3, 3, 0};
	int aRight[] = {2, 3, 1, 3, 0};
	int aTop[] = {2, 3, 3, 3, 3, 2, 1, 0};
	int aBottom[] = {2, 3, 3, 3, 1, 3, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(STRING_BACKGROUND_TITLE);
	jb::Toggle* aCycle = new jb::Toggle(IMAGE_CHECK_BOX, &mGroup, 10);
	Add(aCycle, 0x2e, 0x32);
	jb::SetCheckLabel(aCycle, STRING_RANDOM_BACKGROUND, FONT_TEXT, FONT_TEXT_MAP, 0x20, 0x1f, 0xff);
	mPictures[0] = new BackgroundPicture(&mGroup, 0);
	Add(mPictures[0], 0x2e, 0x61);
	mPictures[1] = new BackgroundPicture(&mGroup, 1);
	Add(mPictures[1], 0xf6, 0x61);
	mPictures[2] = new BackgroundPicture(&mGroup, 2);
	Add(mPictures[2], 0x1be, 0x61);
	mUp = new jb::Button(IMAGE_SCROLL_UP, 4, false);
	Add(mUp, 0x286, 0x63);
	mPictures[3] = new BackgroundPicture(&mGroup, 3);
	Add(mPictures[3], 0x2e, 0xda);
	mPictures[4] = new BackgroundPicture(&mGroup, 4);
	Add(mPictures[4], 0xf6, 0xda);
	mPictures[5] = new BackgroundPicture(&mGroup, 5);
	Add(mPictures[5], 0x1be, 0xda);
	mDown = new jb::Button(IMAGE_SCROLL_DOWN, 4, false);
	Add(mDown, 0x286, 0x138);
	DialogButton* aButton = new DialogButton(0, 0x66);
	Add(aButton, 0xfb, 0x159);
	aButton->SetText(STRING_OK);
	aButton = new DialogButton(1, 0x66);
	Add(aButton, 0x173, 0x159);
	aButton->SetText(STRING_CANCEL);
	int aBackground = gUser.mBackground;
	if (aBackground == -1)
	{
		mGroup.Select(10);
		mCycle = true;
		mFirst = 0;
	}
	else
	{
		mCycle = false;
		mFirst = (aBackground / 3) * 3;
		if (0xf < mFirst)
			mFirst = 0xf;
		int aFirst = mFirst;
		if (CountBackgrounds() <= aFirst + 3)
			mFirst = aFirst - 3;
		mGroup.Select(aBackground - mFirst);
	}
	Refresh();
}

// 0x401810
void BackgroundBox::ShowPictures()
{
	int aCount = CountBackgrounds();
	int i = 0;
	do
	{
		int aBackground = mFirst + i;
		if (aCount <= aBackground)
			mPictures[i]->mBackground = -1;
		else
			mPictures[i]->mBackground = aBackground;
		mPictures[i]->SetEnabled(aCount > aBackground);
		i++;
	} while (i < 6);
}

// 0x401860
bool BackgroundBox::OnCommand(int theCmd)
{
	switch (theCmd)
	{
	case 0x126:
		if (mFirst < 0xf)
		{
			mFirst = mFirst + 3;
			Refresh();
		}
		return true;
	case 0:
		Apply();
		return true;
	case 1:
		jb::gManager->Pop(true, 1);
		return true;
	case 0x125:
		if (0 < mFirst)
		{
			mFirst = mFirst - 3;
			Refresh();
		}
		return true;
	case 0x12a:
		mCycle = !mCycle;
		if (!mCycle)
			mGroup.Select(0);
		return true;
	case 0x142:
		mCycle = false;
		return true;
	}
	return true;
}

// 0x401920
void BackgroundBox::Refresh()
{
	ShowPictures();
	mUp->SetEnabled(0 < mFirst);
	bool aDown = false;
	if (mFirst < 0xf && mFirst + 6 <= CountBackgrounds())
		aDown = true;
	mDown->SetEnabled(aDown);
	int aSelected = mGroup.GetSelected();
	if (aSelected != 10)
	{
		if (!mPictures[aSelected]->IsEnabled())
			mGroup.Select(CountBackgrounds() - mFirst - 1);
	}
	jb::Invalidate();
}

// 0x4019c0
void BackgroundBox::Apply()
{
	int aSelected = mGroup.GetSelected();
	if (aSelected == 10)
		gUser.mBackground = -1;
	else
		gUser.mBackground = mFirst + aSelected;
	PlayLayer* aPlay = (PlayLayer*)jb::gManager->Pop(true, 1);
	aPlay->NextBackground();
}

// 0x401a50
void LayoutBackdrop::Draw()
{
	DrawImage(0, 0, IMAGE_LAYOUTS_BACKGROUND, nullptr, true, 100, 100, 0xff);
}

// 0x401bb0
void LayoutHeader::Draw()
{
	mTitleFont->DrawCentred(this, 0, 0x10, mTitle.c_str(), jb::SCREEN_WIDTH, 100, 100, 0xff);
	mPageFont->DrawLeft(this, 0x67, 0x233, mPrevPage.c_str(), -1, 100, 100, 0xff);
	mPageFont->DrawRight(this, 0x2b5, 0x233, mNextPage.c_str(), 100, 100, 0xff);
	mPlayedFont->DrawCentred(this, 0, 0x2a, mPlayed.c_str(), jb::SCREEN_WIDTH, 100, 100, 0xff);
}

// 0x401c60
void LayoutHeader::Init()
{
	mTitle = jb::LoadDllString(STRING_LAYOUTS_TITLE);
	mTitleFont = jb::GetDllFont(FONT_LAYOUTS_TITLE, FONT_LAYOUTS_TITLE_MAP);
	mPageFont = jb::GetDllFont(FONT_CLOCK, FONT_CLOCK_MAP);
	mPlayedFont = jb::GetDllFont(FONT_CLOCK, FONT_CLOCK_MAP);
	int aLeft = mCount;
	int aPlayed = 0;
	if (0 < aLeft)
	{
		int k = 0;
		do
		{
			if (gUser.mBestTimes[k] != 36000)
				aPlayed++;
			k++;
			aLeft--;
		} while (aLeft != 0);
		if (0 < aPlayed)
		{
			mPlayed = jb::FormatDll(STRING_LAYOUTS_COMPLETED, aPlayed, mCount);
			return;
		}
	}
	mPlayed.clear();
}

// 0x401d10
bool LayoutHeader::Update()
{
	if (!((LayoutSelect*)mLayer)->BuildNext())
		RegUpdate(false);
	return true;
}

// 0x401d40
void LayoutHeader::SetPages(int thePrevFirst, int thePrevLast, int theNextFirst, int theNextLast)
{
	mPrevPage = jb::Format("%d-%d", thePrevFirst + 1, thePrevLast + 1);
	mNextPage = jb::Format("%d-%d", theNextFirst + 1, theNextLast + 1);
}

// 0x401eb0
void LayoutSlot::Draw()
{
	DrawImage(0, 0, mSelected ? 0xbc : 0xc3, nullptr, true, 100, 100, 0xff);
	if (mState == 1)
		DrawImageKeyed(0, 0, FILE_IMAGE_SAVED_BOARD, 0xff00ff, &mCell, true, 100, 100, 0xff);
	else if (mState == 2)
		DrawImagePtr(0, 0, mSheet, &mCell, true, 100, 100, 0xff);
	mFont->DrawCentred(this, 0, 0xb, mName.c_str(), 0xc1, 100, 100, 0xff);
	mFont->DrawCentred(this, 0, 0x8f, mInfo.c_str(), 0xc1, 100, 100, 0xff);
	if (mHasBest)
		DrawImage(0x7d, 0, IMAGE_WON_RIBBON, nullptr, true, 100, 100, 0xff);
}

// 0x401fc0
void LayoutSlot::Init()
{
	mCell.left = (mIndex % 3) * 0xc1;
	mCell.right = mCell.left + 0xc1;
	mCell.top = (mIndex / 3) * 0x93;
	mCell.bottom = mCell.top + 0x93;
	mFont = jb::GetDllFont(FONT_SMALL, FONT_SMALL_MAP);
	SetCursorKind(jb::CURSOR_KIND_HAND, false);
	RegClick(true);
	RegHover(true);
	SetUserFlag(true);
}

// 0x402060
bool LayoutSlot::OnLButtonDown(int theFlags, int theX, int theY)
{
	if (mState != 0)
	{
		((LayoutSelect*)mLayer)->Select(mIndex);
		mSelected = true;
		jb::PlaySound(jb::SOUND_CLICK);
	}
	return true;
}

// 0x4020a0
bool LayoutSlot::OnLButtonDblClk(int theFlags, int theX, int theY)
{
	if (mState != 0)
	{
		jb::PlaySound(jb::SOUND_CLICK);
		((LayoutSelect*)mLayer)->Choose(mIndex);
	}
	return true;
}

// 0x4020d0
void LayoutSlot::SetSaved(const char* theName, int theTiles, int theFree, int theLayout)
{
	mFree = theFree;
	mState = 1;
	mTiles = theTiles;
	mName = theName;
	SetInfo(theLayout);
}

// 0x402120
void LayoutSlot::Empty()
{
	mState = 0;
	mTiles = 0;
	mFree = 0;
	mName.clear();
	mInfo.clear();
}

// 0x402150
void LayoutSlot::SetInfo(int theLayout)
{
	mSaved = false;
	mHasBest = gUser.mBestTimes[theLayout] != 36000;
	std::string aPath;
	gUser.GetFreeSave(theLayout, aPath);
	if (!jb::FileExists(aPath.c_str()))
	{
		if (!mHasBest)
			mInfo = jb::FormatDll(STRING_LAYOUT_TILES, mTiles, mFree);
		else
		{
			std::string aTime = jb::FormatTime(gUser.mBestTimes[theLayout]);
			mInfo = jb::FormatDll(STRING_LAYOUT_WON, aTime.c_str());
		}
	}
	else
	{
		mInfo = jb::LoadDllString(STRING_LAYOUT_SAVED);
		mSaved = true;
	}
}

// 0x402270
void LayoutSlot::GetSaved(std::string& theName, int* theTiles, int* theFree)
{
	theName = mName;
	*theTiles = mTiles;
	*theFree = mFree;
}

// 0x4022a0
void LayoutSlot::SetLayout(jb::Image* theSheet, int theLayoutId, int theLayout)
{
	mSheet = theSheet;
	theSheet->BeginDrawing();
	jb::Image* aBack = jb::LoadDllImageKeyed(IMAGE_THUMB_TILE, 0xff00ff);
	jb::Image* aFaces = jb::LoadDllImage(IMAGE_THUMB_FACES);
	Layout aLayout;
	if (!aLayout.Load(theLayoutId))
	{
		ErrorBox* aBox = new ErrorBox();
		jb::gManager->Push(aBox, true, false, true);
		aBox->SetTitle(STRING_ERROR_TITLE);
		aBox->AddText(STRING_NO_LAYOUT_TEXT, 0, 0, 0x5a, false);
	}
	else
	{
		Measure(aLayout);
		LayoutSelect* aSelect = (LayoutSelect*)mLayer;
		mName = jb::FormatDll(STRING_LAYOUT_NUMBERED, aSelect->mPage + mIndex + 1, aLayout.mName.c_str());
		SetInfo(theLayout);
		jb::Rect aBackRect = {0, 0, 0xc, 0xf};
		Tile* aTile = aLayout.First();
		while (aTile != nullptr)
		{
			int aY = aTile->mRect.top + mOffY;
			int anX = aTile->mRect.left + mOffX;
			aBack->DrawTo(theSheet, anX, aY, aBackRect, 100, 100, 0xff);
			int aFace = aTile->mKind - 1;
			if (aFace < 0 || 0x29 < aFace)
				aFace = jb::Random(0x2a);
			jb::Rect aFaceRect;
			aFaceRect.left = (aFace % 6) * 9;
			aFaceRect.right = aFaceRect.left + 9;
			aFaceRect.top = (aFace / 6) * 0xc;
			aFaceRect.bottom = aFaceRect.top + 0xc;
			aFaces->DrawTo(theSheet, anX + 2, aY + 1, aFaceRect, 100, 100, 0xff);
			aTile = aLayout.Next();
		}
		mState = 2;
	}
	// The original deletes the two images only after a layout that loaded; the port deletes them always
	delete aBack;
	delete aFaces;
}

// 0x402540
void LayoutSlot::Measure(Layout& theLayout)
{
	Tile* aTile = theLayout.First();
	int aMinLeft = 0xc1;
	int aMaxRight = 0;
	int aMinTop = 0x93;
	int aMaxBottom = 0;
	mTiles = 0;
	mFree = 0;
	while (aTile != nullptr)
	{
		int aLeft = (aTile->mX * 10) / 2 - 1 + aTile->mZ;
		aTile->mRect.left = aLeft;
		int aTop = (aTile->mY * 0xd) / 2 - aTile->mZ;
		aTile->mRect.right = aLeft + 9;
		aTile->mRect.top = aTop + 1;
		aTile->mRect.bottom = aTop + 0xd;
		if (aLeft < aMinLeft)
			aMinLeft = aLeft;
		if (aMaxRight < aTile->mRect.right)
			aMaxRight = aTile->mRect.right;
		if (aTop + 1 < aMinTop)
			aMinTop = aTop + 1;
		if (aMaxBottom < aTile->mRect.bottom)
			aMaxBottom = aTile->mRect.bottom;
		theLayout.UpdateFree(aTile);
		if (aTile->mFree)
			mFree = mFree + 1;
		mTiles = mTiles + 1;
		aTile = theLayout.Next();
	}
	mOffX = ((aMinLeft - aMaxRight + 0xc1) / 2 + mCell.left) - aMinLeft;
	mOffY = ((aMinTop - aMaxBottom + 0x93) / 2 + mCell.top) - aMinTop;
}

// 0x402720
LayoutSelect::~LayoutSelect()
{
	delete mSheet;
}

// 0x402780
void LayoutSelect::Load()
{
	gLayoutCount = 0xa2;
	mHeader = new LayoutHeader(gLayoutCount);
	Add(new LayoutBackdrop(), 0, 0);
	if (0 < gLayoutCount)
	{
		int aPos[6] = {0x43, 0x130, 0x21c, 0x3a, 0xdd, 0x181};
		int aColumn = 0;
		do
		{
			int anX = aPos[aColumn];
			int aRows = 3;
			int anIndex = aColumn;
			int aRow = 0;
			do
			{
				LayoutSlot* aSlot = new LayoutSlot(anIndex);
				mSlots[anIndex] = aSlot;
				Add(aSlot, anX, aPos[3 + aRow]);
				anIndex = anIndex + 3;
				aRow++;
				aRows--;
			} while (aRows != 0);
			aColumn++;
		} while (aColumn < 3);
	}
	jb::Button* aButton = new jb::Button(IMAGE_LAYOUTS_PREV_BUTTON, 4, false);
	Add(aButton, 0x32, 0x21d);
	aButton->SetShowDisabled(false);
	aButton = new jb::Button(IMAGE_LAYOUTS_NEXT_BUTTON, 4, false);
	Add(aButton, 0x2ba, 0x21d);
	aButton->SetShowDisabled(false);
	aButton = new jb::Button(IMAGE_LAYOUTS_MENU_BUTTON, 3, false);
	Add(aButton, 0xd3, 0x22a);
	aButton->SetLabel(STRING_LAYOUTS_MAIN_MENU, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0x8c, 0x19, 2, 2, 0xff);
	if (0 < gLayoutCount)
	{
		aButton = new jb::Button(IMAGE_LAYOUTS_PLAY_BUTTON, 3, false);
		Add(aButton, 0x1b2, 0x22a);
		aButton->SetLabel(STRING_PLAY_LAYOUT, FONT_BUTTON, FONT_BUTTON_MAP, 0, 0xa4, 0x19, 2, 2, 0xff);
		if (gLayoutCount < gUser.mSelectedLayout)
			gUser.mSelectedLayout = 0;
		int aPage = gUser.mSelectedLayout / 9;
		mPage = aPage * 9;
		mSelected = gUser.mSelectedLayout - aPage * 9;
		mSlots[mSelected]->mSelected = true;
		Add(mHeader, 0, 0);
		ShowPage();
	}
	int aMusic = MUSIC_LAYOUTS_2;
	if (gMusicTurn == 1)
		aMusic = MUSIC_LAYOUTS_1;
	jb::gManager->LoadMusic(aMusic);
	jb::gManager->PlayMusic(aMusic, false, 0x44c, 0);
	gMusicTurn = gMusicTurn + 1;
	if (2 < gMusicTurn)
		gMusicTurn = 0;
}

// 0x402b40
bool LayoutSelect::OnCommand(int theCmd)
{
	switch (theCmd - 0xc1)
	{
	case 0:
		mPage = mPage - 9;
		if (mPage < 0)
			mPage = ((gLayoutCount - 1) / 9) * 9;
		break;
	case 1:
		mPage = mPage + 9;
		if (gLayoutCount <= mPage)
			mPage = 0;
		ShowPage();
		return true;
	default:
		return false;
	case 3:
		Choose(mSelected);
		return true;
	case 4:
		jb::gManager->SlideMusic(0, 0x44c);
		jb::gManager->SwitchTo(new MainMenu());
		return true;
	}
	ShowPage();
	return true;
}

// 0x402c90
void LayoutSelect::ShowPage()
{
	if (gLayoutCount <= mSelected % 9 + mPage)
	{
		mSlots[mSelected]->mSelected = false;
		mSelected = 0;
		mSlots[0]->mSelected = true;
	}
	gUser.mSelectedLayout = mSelected + mPage;
	int aPrevFirst = mPage - 9;
	if (aPrevFirst < 0)
		aPrevFirst = ((gLayoutCount - 1) / 9) * 9;
	int aPrevLast = aPrevFirst + 8;
	if (gLayoutCount <= aPrevLast)
		aPrevLast = gLayoutCount - 1;
	int aNextFirst = mPage + 9;
	if (gLayoutCount <= aNextFirst)
		aNextFirst = 0;
	int aNextLast = aNextFirst + 8;
	if (gLayoutCount <= aNextLast)
		aNextLast = gLayoutCount - 1;
	mHeader->SetPages(aPrevFirst, aPrevLast, aNextFirst, aNextLast);
	LoadSheet();
}

// 0x402d50
void LayoutSelect::Select(int theSlot)
{
	if (mSelected != theSlot)
	{
		mSlots[mSelected]->mSelected = false;
		gUser.mSelectedLayout = mPage + theSlot;
		mSelected = theSlot;
		jb::Invalidate();
	}
}

// 0x402d90
void LayoutSelect::Choose(int theSlot)
{
	mSelected = theSlot;
	if (mSlots[theSlot]->mSaved)
	{
		YesNoBox* aBox = new YesNoBox(this, 2, 1, STRING_CONTINUE, STRING_RESTART, false, 0x78, 0x78);
		jb::gManager->Push(aBox, true, true, true);
		aBox->SetTitle(STRING_SAVED_GAME_TITLE);
		aBox->AddText(STRING_SAVED_GAME_TEXT, 0, 0, 0x5a, false);
		return;
	}
	Start(false);
}

// 0x402e80
bool LayoutSelect::OnYes(int theId)
{
	Start(true);
	return false;
}

// 0x402e90
bool LayoutSelect::OnNo(int theId)
{
	Start(false);
	return false;
}

// 0x402ea0
void LayoutSelect::Start(bool theResume)
{
	int aLayout = mPage + mSelected;
	gUser.mSelectedLayout = aLayout;
	std::string aName;
	int aTiles;
	int aFree;
	mSlots[mSelected]->GetSaved(aName, &aTiles, &aFree);
	PlayLayer* aPlay = new PlayLayer(1);
	aPlay->SetLayout(aLayout, theResume, ListLayoutId(aLayout), aName.c_str());
	jb::gManager->SwitchTo(aPlay);
}

// 0x402fa0
void LayoutSelect::LoadSheet()
{
	jb::gManager->FreeImage(FILE_IMAGE_SAVED_BOARD);
	int i = 9;
	LayoutSlot** aSlot = mSlots;
	do
	{
		(*aSlot)->Empty();
		aSlot++;
		i--;
	} while (i != 0);
	std::string aBmp;
	std::string aTxt;
	GetSlotFiles(aBmp, aTxt, mPage);
	if (jb::gManager->LoadFileImage(FILE_IMAGE_SAVED_BOARD, aBmp.c_str()))
	{
		FILE* aFile = fopen(aTxt.c_str(), "rt");
		if (aFile != nullptr)
		{
			std::string aLine;
			std::string aName;
			char aBuf[1000];
			int k = 0;
			do
			{
				if (fgets(aBuf, 1000, aFile) == nullptr)
					break;
				aLine = aBuf;
				int aPos = 0;
				aName = jb::Tokenize(aLine, 9, aPos);
				int aTiles = strtol(jb::Tokenize(aLine, 9, aPos).c_str(), nullptr, 10);
				int aFree = strtol(jb::Tokenize(aLine, 9, aPos).c_str(), nullptr, 10);
				mSlots[k]->SetSaved(aName.c_str(), aTiles, aFree, k + mPage);
				k++;
			} while (k < 9);
			fclose(aFile);
			return;
		}
		jb::gManager->FreeImage(FILE_IMAGE_SAVED_BOARD);
	}
	if (mSheet == nullptr)
		mSheet = jb::CreateImage(0x243, 0x1b9);
	mSheet->Fill(0xff00ff);
	mBuilt = 0;
	mHeader->StartBuilding();
}

// 0x403270
bool LayoutSelect::BuildNext()
{
	int aBuilt = mBuilt;
	if (aBuilt < 9 && mPage + aBuilt < gLayoutCount)
	{
		int aLayout = mPage + aBuilt;
		mSlots[aBuilt]->SetLayout(mSheet, ListLayoutId(aLayout), aLayout);
		mBuilt = mBuilt + 1;
		if (mBuilt < 9 && mBuilt + mPage < gLayoutCount)
			return true;
		std::string aBmp;
		std::string aTxt;
		GetSlotFiles(aBmp, aTxt, mPage);
		if (mSheet->SaveBmp(aBmp.c_str()))
		{
			FILE* aFile = fopen(aTxt.c_str(), "wt");
			if (aFile != nullptr)
			{
				std::string aName;
				int k = 0;
				do
				{
					int aTiles;
					int aFree;
					mSlots[k]->GetSaved(aName, &aTiles, &aFree);
					if (aTiles == 0)
						break;
					fprintf(aFile, "%s\t%d\t%d\n", aName.c_str(), aTiles, aFree);
					k++;
				} while (k < 9);
				fclose(aFile);
			}
		}
	}
	return false;
}

} // namespace mahjongjapan
