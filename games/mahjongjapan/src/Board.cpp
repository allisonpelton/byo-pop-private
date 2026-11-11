#include "Board.h"

#include "Font.h"
#include "GameDialogs.h"
#include "Globals.h"
#include "Layer.h"
#include "Manager.h"
#include "PlayLayer.h"
#include "Res.h"
#include "Stream.h"
#include "User.h"

namespace mahjongjapan
{


// 0x40a3d0
void SavedGame::Serialize(jb::Stream& theStream)
{
	mMagic = 0x4d53;
	mVersion = 1;
	theStream.Short(mMagic);
	theStream.Char(mVersion);
	theStream.Char(mMode);
	theStream.Short(mLevel);
	theStream.Short(mProgress);
	theStream.Int(mScore);
	theStream.Word(mTime);
	theStream.Short(mMove);
	theStream.Serialize(mLayout);
	theStream.Checksum();
}

// 0x40a500
PointsText::PointsText(int thePoints, int theX, int theY) : mX(theX), mY(theY)
{
	mText = jb::Format(thePoints < 1 ? "%d" : "+%d", thePoints);
}

// 0x40a5b0
Board::Board(int theMode) : mMode(theMode), mModeByte((bool)(theMode & 0xff))
{
}

// 0x40a710 (the original frees the list's nodes but not the texts still on it: the port deletes them)
Board::~Board()
{
	for (PointsText* aText : mTexts)
		delete aText;
	mTexts.clear();
}

// 0x40a7d0
void Board::Clear()
{
	ClearFlights();
	if (mTiles != nullptr)
	{
		delete mTiles;
		mTiles = nullptr;  // the original leaves the pointer; nothing draws after Clear
	}
}

// 0x40a7f0
void Board::SetStyle(int theStyle)
{
	int aFaces;
	int aBack;
	switch (theStyle)
	{
	case 0:
		aFaces = 0x65;
		mSelImage = IMAGE_FACES_IVORY_SELECTED;
		aBack = 0;
		break;
	default:
		aFaces = 0xd7;
		mSelImage = IMAGE_FACES_GREEN_SELECTED;
		aBack = 0;
		break;
	case 2:
		aFaces = 0x139;
		mSelImage = IMAGE_FACES_RED_SELECTED;
		aBack = 6;
		break;
	case 3:
		aFaces = 0x13b;
		mSelImage = IMAGE_FACES_BLACK_SELECTED;
		aBack = 7;
		break;
	case 4:
		aFaces = 0x13d;
		mSelImage = IMAGE_FACES_PAINTED_SELECTED;
		aBack = 0;
		break;
	case 5:
		aFaces = 0x13f;
		mSelImage = IMAGE_FACES_NUMBERS_SELECTED;
		aBack = 0;
		break;
	}
	mManager->PreloadDllImage(mSelImage);
	if (mTiles == nullptr)
		mTiles = jb::CreateImage(0x120, 0x1d0);
	mTiles->Fill(0xff00ff);
	jb::Image* aBacks = jb::LoadDllImageKeyed(IMAGE_SPECIAL_FACES, 0xff00ff);
	jb::Image* aFaceImage = jb::LoadDllImage(aFaces);
	int aColumn = 0;
	int x = 0;
	do
	{
		int y = 0;
		int aKind = aColumn;
		do
		{
			// The blank tile (0) and the kinds draw on the set's back; the key tile (0x2b) and the specials on their
			// own backs
			int aCell = aBack;
			if (KIND_LAST_FLOWER < aKind)
			{
				if (aKind == KIND_GOLD)
					aCell = 1;
				else
				{
					aCell = 0;
					if (KIND_GOLD < aKind)
						aCell = aKind - KIND_LAST_FLOWER;
				}
			}
			jb::Rect aSrc;
			aSrc.left = aCell * 0x30;
			aSrc.right = aSrc.left + 0x30;
			aSrc.top = 0;
			aSrc.bottom = 0x3a;
			aBacks->DrawTo(mTiles, x, y, aSrc, 100, 100, 0xff);
			if (0 < aKind && aKind < KIND_GOLD)
			{
				aSrc.left = ((aKind - 1) % 6) * 0x26;
				aSrc.top = ((aKind - 1) / 6) * 0x30;
				aSrc.bottom = aSrc.top + 0x30;
				aSrc.right = aSrc.left + 0x26;
				aFaceImage->DrawTo(mTiles, x + 7, y + 3, aSrc, 100, 100, 0xff);
			}
			y += 0x3a;
			aKind += 6;
		} while (y < 0x1d0);
		x += 0x30;
		aColumn++;
	} while (x < 0x120);
	delete aBacks;
	delete aFaceImage;
}

// 0x40aa40
void Board::SetLayout(int theLayoutId, int theQuadPercent, uint32_t theSeed)
{
	mLayoutId = theLayoutId;
	mQuadPercent = theQuadPercent;
	if (theSeed == 0xffffffff)
		theSeed = jb::SeedTick();
	mSeed = theSeed;
}

// 0x40aa70
void Board::Init()
{
	mManager->PreloadDllImage(IMAGE_SPECIAL_FACES);
	mManager->PreloadDllImage(FONT_POINTS);
	mManager->PreloadDllImage(IMAGE_KEY_FRAMES);
	mManager->PreloadDllImage(IMAGE_KEY);
	mManager->PreloadDllImage(IMAGE_LOCK_FRAMES);
	mManager->PreloadDllImage(IMAGE_GOLD_FRAMES);
	mManager->PreloadDllImage(IMAGE_JOKER_FRAMES);
	mManager->PreloadDllImage(IMAGE_MAGNET_FRAMES);
	jb::LoadSound(SOUND_SELECT, false);
	jb::LoadSound(SOUND_NOT_FREE, false);
	jb::LoadSound(SOUND_MATCH, false);
	jb::LoadSound(SOUND_BONUS, false);
	jb::LoadSound(SOUND_JOKER_LOOP, true);
	jb::LoadSound(SOUND_MAGNET_LOOP, true);
	jb::LoadSound(SOUND_KEY_LOOP, true);
	jb::LoadSound(SOUND_SHUFFLE, false);
	jb::LoadSound(SOUND_UNDO, false);
	jb::LoadSound(SOUND_GOLD_MATCH, false);
	jb::LoadSound(SOUND_FLY_OFF_LOOP, true);
	jb::LoadSound(SOUND_JOIN, false);
	jb::LoadSound(SOUND_DEAL, false);
	mFont = jb::GetDllFont(FONT_POINTS, FONT_POINTS_MAP);
	RegUpdate(true);
	RegClick(true);
	RegLayerCmd(true);
	RegChar(true);
}

// 0x40abd0
void Board::ClearFlights()
{
	int i = 0;
	if (0 < mFlightCount)
	{
		do
		{
			delete mFlights[i];
			i++;
		} while (i < mFlightCount);
	}
	mFlightCount = 0;
}

// 0x40ac20
void Board::ClearTexts()
{
	for (PointsText* aText : mTexts)
		delete aText;
	mTexts.clear();
}

// 0x40acb0
void Board::LoadLayout()
{
	if (!mLayout.Load(mLayoutId))
	{
		ErrorBox* aBox = new ErrorBox();
		jb::gManager->Push(aBox, true, false, true);
		aBox->SetTitle(STRING_ERROR_TITLE);
		aBox->AddText(STRING_NO_LAYOUT_TEXT, 0, 0, 0x5a, false);
		return;
	}
	Measure();
}

// 0x40ada0
void Board::ClearHints()
{
	int i = 0;
	if (0 < mHintCount)
	{
		do
		{
			mHints[i]->mBobsAsHint = false;
			mHints[i]->mLiftStart = 0;
			i++;
		} while (i < mHintCount);
	}
	mHintCount = 0;
}

// 0x40ade0
void Board::Reset()
{
	mOver = false;
	mSelected = nullptr;
	mSelAnim = 0;
	ClearFlights();
	ClearHints();
	ClearTexts();
	mTray[0] = 0;
	mTray[1] = 0;
	mMove = 0;
	mDealt = false;
	mClockIdle = true;
}

// 0x40ae30
void Board::Resume(int theMove)
{
	mMove = theMove;
	UpdateTray();
	mDealt = true;
	mClockIdle = false;
}

// 0x40ae60
void Board::Start(bool theDeal, bool theKeep, bool theNewSeed)
{
	mStarted = true;
	mIdleSince = 0;
	if (theNewSeed)
		mSeed = jb::SeedTick();
	if (!theKeep)
		mLayout.Reset(theDeal || mDealt);
	SetFlyIn(gFlyIn);
	mPhaseTime = 0x2ee;
	mPhaseLeft = 0x2ee;
	gFlyIn++;
	if (6 < gFlyIn)
		gFlyIn = 1;
	mLayout.UpdateFree();
	if (theDeal || mDealt)
	{
		jb::CrtSrand(mSeed);
		mDeal.Start(&mLayout, mQuadPercent, mMove, false);
	}
	mMusicMove = -1;
	UpdateMoves();
	jb::gManager->StopSoundsExcept(-1);
	jb::PlaySound(SOUND_DEAL);
}

// 0x40af70
void Board::Shuffle()
{
	SetFlyOut();
	mPhaseTime = 500;
	mPhaseLeft = 500;
	mPhase = 1;
	jb::PlaySound(SOUND_SHUFFLE);
	ClearSelection();
	std::string aText = jb::LoadDllString(STRING_BONUS_SECONDS);
	mTexts.push_back(new PointsText(aText.c_str(), 0x244, 0x1bf));
	GetPlayLayer()->AddSeconds(0x14);
}

// 0x40b0a0
void Board::Reshuffle()
{
	mDealt = true;
	ClearFlights();
	ClearHints();
	jb::CrtSrand(jb::SeedTick());
	mDeal.Start(&mLayout, mQuadPercent, mMove, true);
	UpdateMoves();
	SwapFlyOut();
	mPhase = 2;
	mPhaseTime = 500;
	mPhaseLeft = 500;
}

// 0x40b130
void Board::Draw()
{
	if (mStarted && (IsActive() || mTipShown))
	{
		mShade = gUser.mShadeCovered;
		for (Tile* aTile = mLayout.First(); aTile != nullptr; aTile = mLayout.Next())
		{
			if (aTile->mTaken == 0)
				DrawTile(aTile);
		}
	}
	SetTrackDirty(false);
	if (0 < mTray[0])
	{
		DrawFace(0x163, 0x1d6, mTray[0], false, 6);
		if (mFlightKind == 4)
		{
			jb::Rect aSrc = {0x118, 0, 0x140, 0x32};
			DrawImage(0x164, 0x1d7, IMAGE_KEY_FRAMES, &aSrc, true, 100, 100, 0xff);
		}
	}
	if (0 < mTray[1])
		DrawFace(0x195, 0x1d6, mTray[1], false, 6);
	if (mStarted && (IsActive() || mTipShown))
	{
		int i = 0;
		if (0 < mFlightCount)
		{
			do
			{
				DrawFlight(mFlights[i]);
				i++;
			} while (i < mFlightCount);
		}
		if (mEscaping)
		{
			DrawFace(0x15c, 0x172, KIND_GOLD, false, 0);
			DrawFace(0x18e, 0x172, KIND_GOLD, false, 0);
		}
		for (PointsText* aText : mTexts)
		{
			int aRisen = 800 - aText->mLeft;
			int anAlpha;
			if (aRisen < 0x191)
				anAlpha = 0xff;
			else
			{
				anAlpha = (aText->mLeft * 0xff) / 400;
				if (anAlpha <= 0)
					continue;
			}
			int aWidth = mFont->GetWidth(aText->mText.c_str(), 100);
			int x = (-aWidth + 0x28) / 2 + 7 + aText->mX;
			if (x < 4)
				x = 4;
			if (0x31c < x + aWidth)
				x = -aWidth + 0x31c;
			mFont->DrawLeft(this, x, (aText->mY - (aRisen * 0x78) / 800) + 0x1e, aText->mText.c_str(), -1, 100, 100,
				anAlpha);
		}
	}
	SetTrackDirty(true);
}

// 0x40b3d0
void Board::DrawFlight(Flight* theFlight)
{
	int aTime = theFlight->mTime;
	int aGone = aTime - theFlight->mLeft;
	int x = ((theFlight->mToX - theFlight->mFromX) * aGone) / aTime + theFlight->mFromX;
	int y = ((theFlight->mToY - theFlight->mFromY) * aGone) / aTime + theFlight->mFromY;
	if (mFlightKind != 3)
	{
		if (0 < theFlight->mKind)
			DrawFace(x, y, theFlight->mKind, false, 0);
		return;
	}
	if (theFlight->mKind == KIND_KEY)
	{
		jb::Rect aSrc = {0x60, 0, 0x90, 0x3a};
		DrawImageKeyed(x, y, IMAGE_SPECIAL_FACES, 0xff00ff, &aSrc, true, 100, 100, 0xff);
		aSrc = {0x118, 0, 0x140, 0x32};
		DrawImage(x + 7, y + 1, IMAGE_KEY_FRAMES, &aSrc, true, 100, 100, 0xff);
		return;
	}
	DrawImage(x + 7, y + 1, IMAGE_KEY, nullptr, true, 100, 100, 0xff);
}

// 0x40b500
bool Board::OnCommand(int theCmd)
{
	mIdleSince = GetTime();
	if ((theCmd != 0x88 && theCmd != 0x90 && theCmd != 0x94 && theCmd != 0x10d && theCmd != 0x108 &&
			theCmd != 0x10c) ||
		(!mOver && mPhase == 0 && mPhaseLeft < 1))
	{
		StartClock();
		switch (theCmd)
		{
		case 0x88:
		case 0x10d:
			Undo();
			break;
		default:
			return false;
		case 0x90:
		case 0x108:
			ShowHint(true);
			return true;
		case 0x94:
		case 0x10c:
			// not in the last pair's flight: the shuffle's redeal clears that flight, whose end is what
			// closes the round, and the board would sit empty for good (a fix of the original, DEVIATIONS.md)
			if (mLeft == 0)
				return true;
			Shuffle();
			return true;
		}
	}
	return true;
}

// 0x40b660
void Board::OnChar(int theChar, int theRepeat, int theScan)
{
	if ((theChar == 0xd || theChar == 0x20) && mOver && mEscaping)
		SkipFlyOff();
}

// 0x40b690
void Board::Measure()
{
	Tile* aTile = mLayout.First();
	int aBottom = 0;
	int aLeft = 800;
	int aRight = 0;
	int aTop = 0x1c4;
	while (aTile != nullptr)
	{
		int x = aTile->mX * 0x15 + -6 + aTile->mZ * 6;
		aTile->mRect.left = x;
		int y = aTile->mY * 0x1a + aTile->mZ * -6;
		aTile->mRect.right = x + 0x30;
		aTile->mRect.top = y + 6;
		aTile->mRect.bottom = y + 0x40;
		if (x < aLeft)
			aLeft = x;
		if (aRight < aTile->mRect.right)
			aRight = aTile->mRect.right;
		if (y + 6 < aTop)
			aTop = y + 6;
		if (aBottom < aTile->mRect.bottom)
			aBottom = aTile->mRect.bottom;
		aTile = mLayout.Next();
	}
	mOffX = ((aLeft - aRight) + 800) / 2 - aLeft;
	mOffY = (((aTop - aBottom) + 0x1c4) / 2 - aTop) + -5;
}

// 0x40b780
void Board::SetFlyIn(int theKind)
{
	Tile* aTile = mLayout.First();
	switch (theKind)
	{
	case 1:
		if (aTile != nullptr)
		{
			do
			{
				aTile->mFromX = 0x178 - mOffX;
				aTile->mFromY = 0xc5 - mOffY;
				aTile = mLayout.Next();
			} while (aTile != nullptr);
		}
		break;
	case 2:
		if (aTile != nullptr)
		{
			do
			{
				aTile->mFromX = 10 - mOffX;
				aTile->mFromY = 10 - mOffY;
				aTile = mLayout.Next();
			} while (aTile != nullptr);
		}
		break;
	case 3:
		if (aTile != nullptr)
		{
			do
			{
				aTile->mFromX = aTile->mRect.bottom;  // as the original: the bottom, not the left
				aTile->mFromY = 0x150 - mOffY;
				aTile = mLayout.Next();
			} while (aTile != nullptr);
		}
		break;
	case 4:
		if (aTile != nullptr)
		{
			do
			{
				aTile->mFromX = 0x178 - mOffX;
				aTile->mFromY = 0x150 - mOffY;
				aTile = mLayout.Next();
			} while (aTile != nullptr);
		}
		break;
	case 5:
		if (aTile != nullptr)
		{
			do
			{
				aTile->mFromX = 0x2e6 - mOffX;
				aTile->mFromY = 10 - mOffY;
				aTile = mLayout.Next();
			} while (aTile != nullptr);
		}
		break;
	case 6:
		while (aTile != nullptr)
		{
			aTile->mFromY = 0x150 - mOffY;
			aTile->mFromX = aTile->mRect.left;
			aTile = mLayout.Next();
		}
		break;
	}
}

// 0x40b900: the place on the board becomes the start, and a corner off it the end
void Board::SetFlyOut()
{
	int anOffX = mOffX;
	int anOffY = mOffY;
	Tile* aTile = mLayout.First();
	while (aTile != nullptr)
	{
		if (aTile->mTaken == 0)
		{
			aTile->mFromX = aTile->mRect.left;
			aTile->mFromY = aTile->mRect.top;
			if (aTile->mX % 4 < 2)
				aTile->mRect.left = 0x2e6 - anOffX;
			else
				aTile->mRect.left = 10 - anOffX;
			if (aTile->mY % 4 < 2)
				aTile->mRect.top = 0x180 - anOffY;
			else
				aTile->mRect.top = 10 - anOffY;
		}
		aTile = mLayout.Next();
	}
}

// 0x40b9a0
void Board::SwapFlyOut()
{
	Tile* aTile = mLayout.First();
	while (aTile != nullptr)
	{
		if (aTile->mTaken == 0)
		{
			int x = aTile->mFromX;
			int y = aTile->mFromY;
			aTile->mFromX = aTile->mRect.left;
			aTile->mRect.left = x;
			aTile->mFromY = aTile->mRect.top;
			aTile->mRect.top = y;
		}
		aTile = mLayout.Next();
	}
}

// 0x40b9e0
void Board::DrawTile(Tile* theTile)
{
	int x;
	int y;
	int aDY;
	if (mPhaseLeft < 1)
	{
		y = mOffY;
		aDY = theTile->mRect.top;
		x = mOffX + theTile->mRect.left;
	}
	else
	{
		int aTime = mPhaseTime;
		int aGone = aTime - mPhaseLeft;
		x = ((theTile->mRect.left - theTile->mFromX) * aGone) / aTime + mOffX + theTile->mFromX;
		aDY = theTile->mFromY;
		y = ((theTile->mRect.top - theTile->mFromY) * aGone) / aTime + mOffY;
	}
	y += aDY;
	if (theTile->mLiftStart != 0)
	{
		if (!theTile->mBobsAsHint)
		{
			// A tile put back by an undo settles
			int aLift = 0xd - (int)((uint32_t)(GetTime() - theTile->mLiftStart) / 0x1e);
			if (aLift < 1)
				theTile->mLiftStart = 0;
			else
			{
				x += aLift;
				y -= aLift;
				mAnimating = true;
			}
		}
		else
		{
			// A hint's tiles bob three times
			uint32_t aStep = (uint32_t)(GetTime() - theTile->mLiftStart) / 0x26;
			if (aStep < 0x24)
			{
				aStep %= 0xc;
				if (6 < aStep)
					aStep = 0xc - aStep;
				x += aStep;
				y -= aStep;
				mAnimating = true;
			}
			else
				ClearHints();
		}
	}
	if (mHighlight && (theTile->mKind == mHighlightKind || theTile->mKind == mHighlightOther))
	{
		uint32_t aStep = ((uint32_t)ManagerTime() / 0x26) % 0xc;
		if (6 < aStep)
			aStep = 0xc - aStep;
		x += aStep;
		y -= aStep;
	}
	DrawFace(x, y, theTile->mKind, mSelected == theTile, 0);
	if (mShade && !theTile->mFree)
	{
		jb::Rect aRect;
		aRect.left = x + 6;
		aRect.right = x + 0x30;
		aRect.bottom = y + 0x34;
		aRect.top = y;
		FillRectAlpha(aRect, 0x50, 0, true);
	}
	if (mFlightKind == 4 && theTile == mFlights[0]->mTile)
	{
		// The halves joining
		Flight* aFlight = mFlights[0];
		int aFrame = ((aFlight->mTime - aFlight->mLeft) * 0x12) / aFlight->mTime;
		if (0x11 < aFrame)
			aFrame = 0x11;
		jb::Rect aSrc;
		aSrc.left = aFrame * 0x28;
		aSrc.right = aSrc.left + 0x28;
		aSrc.top = 0;
		aSrc.bottom = 0x32;
		DrawImage(x + 7, y + 1, IMAGE_LOCK_FRAMES, &aSrc, true, 100, 100, 0xff);
	}
}

// 0x40bc10
void Board::DrawFace(int theX, int theY, int theKind, bool theSelected, int theOffset)
{
	jb::Rect aSrc;
	aSrc.left = (theKind % 6) * 0x30 + theOffset;
	aSrc.top = (theKind / 6) * 0x3a;
	aSrc.right = (aSrc.left - theOffset) + 0x30;
	aSrc.bottom = (aSrc.top - theOffset) + 0x3a;
	DrawImagePtr(theX, theY, mTiles, &aSrc, true, 100, 100, 0xff);
	if (theSelected && theKind != 0)
	{
		int anImage = mSelAnim;
		uint32_t aCell;
		if (anImage == 0)
		{
			if (KIND_LAST_FLOWER < theKind)
				return;
			aCell = (theKind + -1) % 6;
			anImage = mSelImage;
			aSrc.top = ((theKind + -1) / 6) * 0x32;
			aSrc.bottom = aSrc.top + 0x32;
		}
		else
		{
			aCell = (uint32_t)mSelFrame % (uint32_t)mSelFrames;
			aSrc.top = 0;
			aSrc.bottom = 0x32;
		}
		aSrc.left = aCell * 0x28;
		aSrc.right = aSrc.left + 0x28;
		DrawImage((theX - theOffset) + 7, theY + 1, anImage, &aSrc, true, 100, 100, 0xff);
	}
}

// 0x40bd70
bool Board::Update()
{
	bool aChanged = mAnimating;
	mAnimating = false;
	if (0 < mPhaseLeft)
	{
		mPhaseLeft -= GetDelta();
		if (mPhaseLeft < 1)
		{
			int aPhase = mPhase;
			mPhaseLeft = 0;
			if (aPhase == 0)
				GetPlayLayer()->OnBoardShown();
			else if (aPhase == 1)
				Reshuffle();
			else if (aPhase == 2)
				EndShuffle();
		}
		aChanged = true;
	}
	if (0 < mFlightCount)
	{
		int i = 0;
		if (0 < mFlightCount)
		{
			do
			{
				Flight* aFlight = mFlights[i];
				i++;
				aFlight->mLeft -= GetDelta();
			} while (i < mFlightCount);
		}
		if (mFlights[0]->mLeft < 0)
		{
			switch (mFlightKind)
			{
			case 0:
				EndTake();
				CheckStuck();
				break;
			case 1:
				FlyMagnet();
				break;
			case 2:
				EndMagnet();
				CheckStuck();
				break;
			case 3:
				JoinKeyAndLock();
				break;
			case 4:
				FlyKeyAndLock();
				break;
			case 5:
				EndKeyAndLock();
				CheckStuck();
				break;
			case 6:
				EndUndo();
				break;
			case 7:
				UndoMagnet();
				break;
			case 8:
				EndUndoMagnet();
				break;
			case 9:
				EndGold();
				break;
			case 10:
				FlyOff();
				break;
			case 0xb:
				EndGoldInTray();
				break;
			}
			if (mLeft == 0)
			{
				mOver = true;
				AddTimer(0x65, 100, 1, 1);
			}
		}
		aChanged = true;
	}
	if (mSelected != nullptr && mSelAnim != 0 && 99 < (uint32_t)(GetTime() - mSelFrameTime))
	{
		mSelFrame++;
		mSelFrameTime = GetTime();
		aChanged = true;
	}
	if (!mTexts.empty())
	{
		aChanged = true;
		auto anIt = mTexts.begin();
		do
		{
			PointsText* aText = *anIt;
			int aDelta = GetDelta();
			int aLeft = aText->mLeft;
			aText->mLeft = aLeft - aDelta;
			if (aLeft - aDelta < 0)
			{
				delete aText;
				anIt = mTexts.erase(anIt);
			}
			else
				++anIt;
		} while (anIt != mTexts.end());
	}
	if (mIdleSince != 0)
	{
		if (!mOver && 30000 < (uint32_t)(GetTime() - mIdleSince))
		{
			mIdleSince = GetTime();
			if (gUser.mAutoHint)
				ShowHint(false);
		}
		return aChanged;
	}
	mIdleSince = GetTime() + -15000;
	return aChanged;
}

// 0x40c030
bool Board::OnLButtonDown(int theFlags, int theX, int theY)
{
	if (!mOver)
	{
		mIdleSince = GetTime();
		if ((mFlightCount < 1 || mFlightKind == 0 || mFlightKind == 6) && mPhase == 0 && mPhaseLeft < 1)
		{
			int aZ = -1;
			int aTileX = 0;
			int aTileY = 0;
			Tile* aTile = mLayout.HitTest(theX - mOffX, theY - mOffY, &aZ, &aTileX, &aTileY);
			if (CanSelect(aTile) && CanPair(aTile))
			{
				ClearHints();
				mSelAnim = 0;
				if (mSelSound != -1)
				{
					jb::StopSound(mSelSound);
					mSelSound = -1;
				}
				StartClock();
				Tile* aSelected = mSelected;
				if (aSelected == aTile)
					mSelected = nullptr;
				else
				{
					if (aSelected != nullptr && TilesMatch(aSelected->mKind, aTile->mKind))
					{
						Take(mSelected, aTile);
						mSelected = nullptr;
						jb::Invalidate();
						return true;
					}
					mSelected = aTile;
					StartSelAnim();
				}
				jb::PlaySound(SOUND_SELECT);
				jb::Invalidate();
				return true;
			}
			jb::PlaySound(SOUND_NOT_FREE);
		}
	}
	return true;
}

// 0x40c1b0: the tip of a special tile, beside it
bool Board::OnRButtonDown(int theFlags, int theX, int theY)
{
	if (!mLayout.mHasCounts)
		return false;
	int aZ = -1;
	int aTileX = 0;
	int aTileY = 0;
	Tile* aTile = mLayout.HitTest(theX - mOffX, theY - mOffY, &aZ, &aTileX, &aTileY);
	if (aTile == nullptr)
		return false;
	mTip = nullptr;
	switch (aTile->mKind)
	{
	case KIND_KEY:
	case KIND_LOCK:
		OnTimer(2, -1);
		break;
	case KIND_JOKER:
		OnTimer(4, -1);
		break;
	case KIND_MAGNET:
		OnTimer(3, -1);
		break;
	}
	if (mTip == nullptr)
		return false;
	int aPos[2];
	GetPos(aPos);
	int aSize[2];
	((Dialog*)mTip)->GetSize(aSize);
	int x = aTile->mRect.left + mOffX;
	int y;
	if (aSize[0] < x)
	{
		x -= aSize[0];
		y = (aTile->mRect.top - aSize[1] / 2) + aPos[1] + 0x1d + mOffY;
	}
	else
	{
		x = mOffX + aTile->mRect.right;
		if (x < 800 - aSize[0])
			y = (aTile->mRect.top - aSize[1] / 2) + aPos[1] + 0x1d + mOffY;
		else
		{
			x = 0x14;
			y = aPos[1] + 0x14;
		}
	}
	if (y < aPos[1] + 5)
		y = aPos[1] + 5;
	else if (0x1fe - aSize[1] < y)
		y = 0x1fe - aSize[1];
	if (x < 0)
		x = 0;
	else if (800 - aSize[1] < x)  // as the original: the height
		x = 800 - aSize[1];
	mTip->SetPos(x, y);
	return false;
}

// 0x40c330
void Board::StartClock()
{
	if (mClockIdle)
	{
		mClockIdle = false;
		GetPlayLayer()->SetClockRunning(true);
	}
}

// 0x40c350
bool Board::CanSelect(Tile* theTile)
{
	if (theTile == nullptr)
		return false;
	if (mLayout.mHasCounts)
	{
		Tile* aSelected = mSelected;
		if (theTile->mKind == KIND_LOCK)
			return aSelected != nullptr && aSelected->mKind == KIND_KEY;
		if (aSelected != nullptr && (aSelected->mKind == KIND_MAGNET || aSelected->mKind == KIND_JOKER) && theTile->mKind < KIND_GOLD)
			return true;
	}
	return theTile->mFree;
}

// 0x40c3b0
bool Board::CanPair(Tile* theTile)
{
	if (!mLayout.mHasCounts)
		return true;
	if (mSelected == nullptr)
		return true;
	int aKind = mSelected->mKind;
	if (aKind != KIND_KEY && aKind != KIND_JOKER && aKind != KIND_MAGNET)
		return true;
	return mLayout.IsVisible(theTile);
}

// 0x40c400
void Board::StartSelAnim()
{
	switch (mSelected->mKind)
	{
	case KIND_GOLD:
		mSelFrames = 7;
		mSelAnim = IMAGE_GOLD_FRAMES;
		break;
	case KIND_KEY:
		mSelFrames = 7;
		mSelAnim = IMAGE_KEY_FRAMES;
		mSelSound = SOUND_KEY_LOOP;
		break;
	case KIND_JOKER:
		mSelFrames = 7;
		mSelAnim = IMAGE_JOKER_FRAMES;
		mSelSound = SOUND_JOKER_LOOP;
		break;
	case KIND_MAGNET:
		mSelFrames = 7;
		mSelAnim = IMAGE_MAGNET_FRAMES;
		mSelSound = SOUND_MAGNET_LOOP;
		break;
	}
	if (mSelSound != -1)
		jb::PlaySound(mSelSound);
	mSelFrame = 0;
}

// 0x40c4d0
void Board::Take(Tile* theTile, Tile* theOther)
{
	mMove++;
	FinishFlights();
	if (KIND_LAST_FLOWER < theTile->mKind)
	{
		switch (theTile->mKind)
		{
		case KIND_GOLD:
			TakeGold(theTile, theOther);
			return;
		case KIND_KEY:
			TakeKeyAndLock(theTile, theOther);
			jb::PlaySound(SOUND_MATCH);
			return;
		case KIND_JOKER:
			TakeJoker(theTile, theOther);
			jb::PlaySound(SOUND_MATCH);
			return;
		case KIND_MAGNET:
			TakeMagnet(theTile, theOther);
			jb::PlaySound(SOUND_MATCH);
			return;
		}
	}
	theTile->mTaken = mMove;
	theOther->mTaken = mMove;
	mLayout.UpdateNeighbours(theTile, theOther, nullptr, 0, true);
	UpdateMoves();
	Flight* aFlight = new Flight(theTile, mOffX + theTile->mRect.left, mOffY + theTile->mRect.top, 800);
	Flight* anOther = new Flight(theOther, mOffX + theOther->mRect.left, mOffY + theOther->mRect.top, 800);
	if (aFlight->mFromX < anOther->mFromX)
	{
		mFlights[0] = aFlight;
		mFlights[1] = anOther;
	}
	else
	{
		mFlights[0] = anOther;
		mFlights[1] = aFlight;
	}
	mFlightCount = 2;
	mFlightKind = 0;
	mFlights[0]->SetTo(0x15c, 0x1d6);
	mFlights[1]->SetTo(0x18e, 0x1d6);
	AddPoints(theOther, 2, true);
	jb::PlaySound(SOUND_MATCH);
}

// 0x40c6a0
void Board::AddPoints(Tile* theTile, int theCount, bool theBonus)
{
	int aPoints = KindPoints(theTile->mKind) * theCount;
	if (0 < aPoints)
	{
		mTexts.push_back(new PointsText(aPoints, mOffX + theTile->mRect.left, mOffY + theTile->mRect.top));
		if (theBonus && KindsMatch(theTile->mKind, mTray[0]))
		{
			std::string aText = jb::LoadDllString(STRING_BONUS_POINTS);
			mTexts.push_back(new PointsText(aText.c_str(), mOffX + 0x32 + theTile->mRect.left,
				mOffY + 0x1a + theTile->mRect.top));
			jb::PlaySound(SOUND_BONUS);
			aPoints += 500;
		}
		GetPlayLayer()->AddScore(aPoints);
	}
}

// 0x40c8b0
void Board::AddEscapePoints(Tile* theTile)
{
	int aPoints = theTile->mKind < KIND_KEY ? 200 : 500;
	mTexts.push_back(new PointsText(aPoints, 0x174, 0x168));
	GetPlayLayer()->AddScore(aPoints);
}

// 0x40c990
void Board::EndTake()
{
	mTray[0] = mFlights[0]->mKind;
	mTray[1] = mFlights[1]->mKind;
	ClearFlights();
}

// 0x40c9c0: the half flies onto the other and they join where it lies
void Board::TakeKeyAndLock(Tile* theTile, Tile* theOther)
{
	theTile->mTaken = mMove;
	mFlights[0] = new Flight(theTile, mOffX + theTile->mRect.left, mOffY + theTile->mRect.top, 400);
	mFlights[0]->SetTo(0x15c, 0x1d6);
	mFlights[1] = new Flight(theOther, mOffX + theTile->mRect.left, mOffY + theTile->mRect.top, 400);
	mFlights[1]->SetTo(mOffX + theOther->mRect.left, mOffY + theOther->mRect.top);
	mLayout.UpdateNeighbours(theTile, nullptr, nullptr, 0, true);
	mFlightCount = 2;
	mFlightKind = 3;
}

// 0x40cab0
void Board::JoinKeyAndLock()
{
	Tile* aTile = mFlights[1]->mTile;
	mTray[0] = mFlights[0]->mKind;
	mTray[1] = 0;
	ClearFlights();
	mFlights[0] = new Flight(aTile, 0, mOffX + aTile->mRect.left, mOffY + aTile->mRect.top, 1000);
	mFlights[0]->SetTo(mOffX + aTile->mRect.left, mOffY + aTile->mRect.top);
	mFlightCount = 1;
	mFlightKind = 4;
	jb::PlaySound(SOUND_JOIN);
}

// 0x40cb60
void Board::FlyKeyAndLock()
{
	Tile* aTile = mFlights[0]->mTile;
	aTile->mTaken = mMove;
	mLayout.UpdateNeighbours(aTile, nullptr, nullptr, 0, true);
	UpdateMoves();
	ClearFlights();
	mFlights[0] = new Flight(aTile, mOffX + aTile->mRect.left, mOffY + aTile->mRect.top, 500);
	mFlights[0]->SetTo(0x18e, 0x1d6);
	mFlightCount = 1;
	mFlightKind = 5;
}

// 0x40cc00
void Board::EndKeyAndLock()
{
	mTray[1] = KIND_LOCK;
	ClearFlights();
}

// 0x40cc10
void Board::TakeJoker(Tile* theTile, Tile* theOther)
{
	Tile* aTiles[41];
	theTile->mTaken = mMove;
	mFlights[0] = new Flight(theTile, mOffX + theTile->mRect.left, mOffY + theTile->mRect.top, 800);
	mFlights[0]->SetTo(0x15c, 0x1d6);
	int aCount = mLayout.GetMatching(theOther->mKind, aTiles, 0x29);
	if (0 < aCount)
	{
		Tile** aTile = aTiles;
		Flight** aFlight = &mFlights[1];
		int aLeft = aCount;
		do
		{
			(*aTile)->mTaken = mMove;
			*aFlight = new Flight(*aTile, (*aTile)->mRect.left + mOffX, (*aTile)->mRect.top + mOffY, 800);
			(*aFlight)->SetTo(0x18e, 0x1d6);
			aTile++;
			aFlight++;
			aLeft--;
		} while (aLeft != 0);
	}
	AddPoints(theOther, aCount, false);
	mLayout.UpdateNeighbours(theTile, nullptr, nullptr, 0, true);
	mLayout.UpdateNeighbours(aTiles, aCount, nullptr, 0, true);
	UpdateMoves();
	mFlightKind = 0;
	mFlightCount = aCount + 1;
}

// 0x40cd80: the other flies onto the linked tile, which takes its kind and flies to the tray as the wild tile
void Board::TakeMagnet(Tile* theTile, Tile* theOther)
{
	theOther->mMagnetLink = theTile;
	theOther->mTaken = mMove;
	mLayout.UpdateNeighbours(theOther, nullptr, nullptr, 0, true);
	mFlights[0] = new Flight(theOther, mOffX + theOther->mRect.left, mOffY + theOther->mRect.top, 500);
	mFlights[0]->SetTo(mOffX + theTile->mRect.left, mOffY + theTile->mRect.top);
	mFlightCount = 1;
	mFlightKind = 1;
}

// 0x40ce30
void Board::FlyMagnet()
{
	Tile* aTile = mFlights[0]->mTile;
	Tile* aLinked = aTile->mMagnetLink;
	aLinked->mKind = aTile->mKind;
	aLinked->mPreset = aTile->mPreset;
	UpdateMoves();
	ClearFlights();
	mFlights[0] = new Flight(aTile, 0x2f, mOffX + aLinked->mRect.left, mOffY + aLinked->mRect.top, 500);
	mFlights[0]->SetTo(0x15c, 0x1d6);
	mFlightCount = 1;
	mFlightKind = 2;
}

// 0x40ced0
void Board::EndMagnet()
{
	int aKind = mFlights[0]->mKind;
	mTray[1] = 0;
	mTray[0] = aKind;
	ClearFlights();
}

// 0x40cef0
void Board::FinishFlights()
{
	if (0 < mFlightCount)
	{
		if (mFlightKind == 0)
		{
			EndTake();
			return;
		}
		if (mFlightKind == 6)
		{
			EndUndo();
			return;
		}
	}
}

// 0x40cf20
void Board::Undo()
{
	Tile* aTiles[41];
	if (mMove == 0)
		return;
	if (0 < mFlightCount && mFlightKind != 0 && mFlightKind != 6)
		return;
	FinishFlights();
	ClearHints();
	ClearSelection();
	int aKind = -1;
	int aCount = mLayout.GetTaken(mMove, aTiles, 0x29);
	if (aCount == 1)
	{
		// The linked wild tile flies back, and the tile it took flies out of it
		aTiles[0]->mFree = true;
		mFlights[0] = new Flight(aTiles[0], 0x2f, 0x15c, 0x1d6, 300);
		mFlights[0]->SetTo(aTiles[0]->mMagnetLink->mRect.left + mOffX, aTiles[0]->mMagnetLink->mRect.top + mOffY);
		mFlightCount = 1;
		mFlightKind = 7;
	}
	else if (2 < aCount)
	{
		// The wild tile and the tiles it took
		if (0 < aCount)
		{
			Tile** aTile = aTiles;
			Flight** aFlight = &mFlights[1];
			int aLeft = aCount;
			do
			{
				if ((*aTile)->mKind == KIND_JOKER)
				{
					mFlights[0] = new Flight(*aTile, 0x15c, 0x1d6, 400);
					mFlights[0]->SetTo((*aTile)->mRect.left + mOffX, (*aTile)->mRect.top + mOffY);
				}
				else
				{
					*aFlight = new Flight(*aTile, 0x18e, 0x1d6, 400);
					(*aFlight)->SetTo((*aTile)->mRect.left + mOffX, (*aTile)->mRect.top + mOffY);
					aFlight++;
					GetPlayLayer()->AddScore(-KindPoints((*aTile)->mKind));
				}
				aTile++;
				aLeft--;
			} while (aLeft != 0);
		}
		mFlightCount = aCount;
		mFlightKind = 6;
	}
	else if (aCount == 2 && (aTiles[0]->mKind == KIND_KEY || aTiles[1]->mKind == KIND_KEY))
	{
		// The halves
		if (aTiles[0]->mKind != KIND_KEY)
			UndoPair(aTiles[1], aTiles[0]);
		else
			UndoPair(aTiles[0], aTiles[1]);
	}
	else
	{
		// A pair (the original reads the list without a count of 2 too; a move always took tiles)
		Tile* aFirst = aTiles[1];
		Tile* aSecond = aTiles[0];
		if (aTiles[0]->mFromX < aTiles[1]->mFromX)
		{
			aFirst = aTiles[0];
			aSecond = aTiles[1];
		}
		UndoPair(aFirst, aSecond);
		aKind = aTiles[0]->mKind;
		GetPlayLayer()->AddScore(KindPoints(aKind) * -2);
	}
	mMove--;
	UpdateTray();
	if (mTray[0] < KIND_GOLD && KindsMatch(mTray[0], aKind))
		GetPlayLayer()->AddScore(-500);
	jb::PlaySound(SOUND_UNDO);
}

// 0x40d1f0
void Board::UpdateTray()
{
	Tile* aTiles[41];
	if (mMove == 0)
	{
		mTray[0] = 0;
		mTray[1] = 0;
		return;
	}
	int aCount = mLayout.GetTaken(mMove, aTiles, 0x29);
	if (aCount == 1)
	{
		mTray[1] = 0;
		mTray[0] = KIND_MAGNET;
		return;
	}
	if (aCount < 3)
	{
		if (aCount == 2 && (aTiles[0]->mKind == KIND_KEY || aTiles[1]->mKind == KIND_KEY))
		{
			mTray[0] = KIND_KEY;
			mTray[1] = KIND_LOCK;
			return;
		}
		mTray[0] = aTiles[0]->mKind;
		mTray[1] = aTiles[1]->mKind;
	}
	else
	{
		int i = 0;
		mTray[0] = KIND_JOKER;
		if (0 < aCount)
		{
			Tile** aTile = aTiles;
			do
			{
				if ((*aTile)->mKind != KIND_JOKER)
				{
					mTray[1] = aTiles[i]->mKind;
					return;
				}
				i++;
				aTile++;
			} while (i < aCount);
		}
	}
}

// 0x40d2f0
void Board::UndoPair(Tile* theTile, Tile* theOther)
{
	mFlights[0] = new Flight(theTile, 0x15c, 0x1d6, 400);
	mFlights[0]->SetTo(theTile->mRect.left + 0xe + mOffX, theTile->mRect.top + -0xe + mOffY);
	mFlights[1] = new Flight(theOther, 0x18e, 0x1d6, 400);
	mFlights[1]->SetTo(theOther->mRect.left + 0xe + mOffX, theOther->mRect.top + -0xe + mOffY);
	mFlightCount = 2;
	mFlightKind = 6;
}

// 0x40d3c0
void Board::EndUndo()
{
	Tile* aTiles[41];
	int i = 0;
	int aCount = mFlightCount;
	if (0 < aCount)
	{
		Tile** aTile = aTiles;
		Flight** aFlight = mFlights;
		do
		{
			Tile* aBack = (*aFlight)->mTile;
			*aTile = aBack;
			if (aCount < 3)
				aBack->mFree = true;
			else
				mLayout.UpdateFree(aBack);
			(*aTile)->mTaken = 0;
			(*aTile)->mLiftStart = GetTime();
			i++;
			aFlight++;
			aTile++;
			aCount = mFlightCount;
		} while (i < aCount);
	}
	mLayout.UpdateNeighbours(aTiles, mFlightCount, nullptr, 0, false);
	UpdateMoves();
	ClearFlights();
}

// 0x40d460
void Board::UndoMagnet()
{
	Tile* aTile = mFlights[0]->mTile;
	ClearFlights();
	aTile->mKind = aTile->mMagnetLink->mKind;
	mFlights[0] = new Flight(aTile, mOffX + aTile->mMagnetLink->mRect.left, mOffY + aTile->mMagnetLink->mRect.top, 300);
	mFlights[0]->SetTo(mOffX + 0xe + aTile->mRect.left, mOffY + -0xe + aTile->mRect.top);
	mFlightKind = 8;
	mFlightCount = 1;
	aTile->mMagnetLink->mKind = KIND_MAGNET;
	aTile->mMagnetLink->mPreset = true;
	aTile->mMagnetLink = nullptr;
}

// 0x40d510
void Board::EndUndoMagnet()
{
	Tile* aTile = mFlights[0]->mTile;
	aTile->mTaken = 0;
	aTile->mLiftStart = GetTime();
	mLayout.UpdateNeighbours(aTile, nullptr, nullptr, 0, false);
	mLayout.UpdateFree(aTile);
	UpdateMoves();
	ClearFlights();
}

// 0x40d560
void Board::UpdateMoves()
{
	mLayout.CountMoves(&mPairs, &mLeft, &mKindsFree);
	int i = 0;
	mHintIndex = 0;
	ClearHints();
	int k = 0;
	if (0 < mKindsFree)
	{
		do
		{
			mHintOrder[k] = k;
			k++;
		} while (k < mKindsFree);
	}
	int aCount = mKindsFree;
	if (0 < aCount)
	{
		int* anOrder = mHintOrder;
		do
		{
			int j = jb::Random(aCount);
			int aSwap = *anOrder;
			*anOrder = mHintOrder[j];
			mHintOrder[j] = aSwap;
			aCount = mKindsFree;
			i++;
			anOrder++;
		} while (i < aCount);
	}
	if (mMode == 0)
	{
		int aKeys = mLayout.CountFree2b();
		if (aKeys == 2 && mTipGold)
		{
			AddTimer(1, 1000, 1, 1);
			mTipGold = false;
		}
		if (mMusicMove == -1 && 0 < aKeys)
		{
			mMusicMove = mMove;
			jb::gManager->SetMusicLoop(4, 4);
		}
		else if (mMove + 4 <= mMusicMove)
		{
			mMusicMove = -1;
			jb::gManager->SetMusicLoop(4, GetPlayLayer()->mLevel % 2 != 0 ? 0 : 10);
		}
		if (mTipFlowers && mLayout.HasFreeFlowers())
		{
			AddTimer(7, 1000, 1, 1);
			mTipFlowers = false;
		}
	}
	GetPlayLayer()->SetMoves(mPairs, mLeft);
}

// 0x40d6e0
void Board::ShowHint(bool theAsked)
{
	ClearHints();
	ClearSelection();
	if (mPairs < 1)
	{
		if (!mLayout.mHasCounts)
			return;
		mHintCount = mLayout.GetFreeSpecials(mHints, 400);
		if (mHintCount == 0)
			return;
	}
	else
	{
		if (theAsked)
			mHintIndex++;
		if (mKindsFree <= mHintIndex)
			mHintIndex = 0;
		mHintCount = mLayout.GetPairsOf(mHintOrder[mHintIndex], mHints, 400);
	}
	StartHints();
	jb::Invalidate();
	if (theAsked && GetPlayLayer()->mScore != 0)
	{
		mTexts.push_back(new PointsText(-100, 0x1e5, 0x1bf));
		GetPlayLayer()->AddScore(-100);
	}
}

// 0x40d870
void Board::StartHints()
{
	int i = 0;
	if (0 < mHintCount)
	{
		do
		{
			mHints[i]->mBobsAsHint = true;
			mHints[i]->mLiftStart = GetTime();
			i++;
		} while (i < mHintCount);
	}
}

// 0x40d8b0
void Board::TakeGold(Tile* theTile, Tile* theOther)
{
	mOver = true;
	PlayLayer* aLayer = GetPlayLayer();
	aLayer->EnableButtons(false);
	aLayer->SetClockRunning(false);
	aLayer->SetMoves(0, mLeft);
	aLayer->CheckBestTime();
	aLayer->Advance();
	theTile->mTaken = mMove;
	theOther->mTaken = mMove;
	Flight* aFlight = new Flight(theTile, mOffX + theTile->mRect.left, mOffY + theTile->mRect.top, 800);
	Flight* anOther = new Flight(theOther, mOffX + theOther->mRect.left, mOffY + theOther->mRect.top, 800);
	if (aFlight->mFromX < anOther->mFromX)
	{
		mFlights[0] = aFlight;
		mFlights[1] = anOther;
	}
	else
	{
		mFlights[0] = anOther;
		mFlights[1] = aFlight;
	}
	mFlightCount = 2;
	mFlightKind = 9;
	mFlights[0]->SetTo(0x15c, 0x172);
	mFlights[1]->SetTo(0x18e, 0x172);
	AddPoints(theOther, 2, false);
	jb::PlaySound(SOUND_GOLD_MATCH);
	jb::gManager->SetMusicLoop(4, 8);
}

// 0x40da30
void Board::EndGold()
{
	if (2 < mLeft)
	{
		FlyOff();
		mEscaping = true;
		mFlightKind = 10;
		jb::PlaySound(SOUND_FLY_OFF_LOOP);
		return;
	}
	ClearFlights();
	EndRound();
}

// 0x40da70: the lowest tiles fly off, two at a time
void Board::FlyOff()
{
	if (0 < mFlightCount && mEscaping)
		AddEscapePoints(mFlights[0]->mTile);
	GetPlayLayer()->SetMoves(0, mLeft);
	ClearFlights();
	Tile* aTile;
	Tile* anOther;
	mLayout.GetLowest(&aTile, &anOther);
	if (aTile == nullptr)
	{
		EndRound();
		return;
	}
	aTile->mTaken = 1;
	Flight* aFlight = new Flight(aTile, mOffX + aTile->mRect.left, mOffY + aTile->mRect.top, 0x15e);
	Flight* aSecond = nullptr;
	if (anOther != nullptr)
	{
		anOther->mTaken = 1;
		aSecond = new Flight(anOther, mOffX + anOther->mRect.left, mOffY + anOther->mRect.top, 0x15e);
		if (aSecond != nullptr && aSecond->mFromX <= aFlight->mFromX)
		{
			mFlights[0] = aSecond;
			mFlights[1] = aFlight;
			goto set;
		}
	}
	mFlights[0] = aFlight;
	mFlights[1] = aSecond;
set:
	mFlights[0]->SetTo(0x15c, 0x172);
	mFlightCount = 1;
	mLeft--;
	if (mFlights[1] != nullptr)
	{
		mFlights[1]->SetTo(0x18e, 0x172);
		mFlightCount = 2;
		mLeft--;
	}
}

// 0x40dc00: the key tiles drop into the tray
void Board::EndRound()
{
	mEscaping = false;
	mFlights[0] = new Flight(nullptr, 0x2b, 0x15c, 0x172, 400);
	mFlights[1] = new Flight(nullptr, 0x2b, 0x18e, 0x172, 400);
	mFlights[0]->SetTo(0x15c, 0x1d6);
	mFlights[1]->SetTo(0x18e, 0x1d6);
	mFlightCount = 2;
	mFlightKind = 0xb;
	GetPlayLayer()->CheckBestScore();
}

// 0x40dcc0: the end of a round (100, 0x65) and the tips (0-8)
bool Board::OnTimer(int theId, int theIndex)
{
	if (theId == 100)
	{
		GetPlayLayer()->ShowLevelDone();
		return true;
	}
	if (theId == 0x65)
	{
		GetPlayLayer()->ShowResult();
		return true;
	}
	if (theId < 0 || 8 < theId)
		return true;
	if (GetPlayLayer()->TipsOff())
		return true;
	bool aPauseSounds = false;
	mTipShown = true;
	switch (theId)
	{
	case 1:
		mHighlight = true;
		mHighlightKind = KIND_GOLD;
		mHighlightOther = -1;
		aPauseSounds = true;
		break;
	case 2:
		mHighlight = true;
		aPauseSounds = theIndex == -1;
		mHighlightKind = KIND_KEY;
		mHighlightOther = 0x2d;
		break;
	case 3:
		mHighlight = true;
		aPauseSounds = theIndex == -1;
		mHighlightKind = KIND_MAGNET;
		mHighlightOther = -1;
		break;
	case 4:
		mHighlight = true;
		aPauseSounds = theIndex == -1;
		mHighlightKind = KIND_JOKER;
		mHighlightOther = -1;
		break;
	case 7:
		mHighlight = true;
		mHighlightKind = KIND_FIRST_FLOWER;
		mHighlightOther = 0x28;
		aPauseSounds = true;
		break;
	}
	mTip = new TipBox(theId, this);
	if (!aPauseSounds)
		mLayer->SetPauseSounds(false);
	jb::gManager->Push(mTip, true, false, true);
	mLayer->SetPauseSounds(true);
	return true;
}

// 0x40deb0
void Board::EndGoldInTray()
{
	mStarted = false;
	ClearFlights();
	mTray[0] = KIND_GOLD;
	mTray[1] = KIND_GOLD;
	GetPlayLayer()->SetMoves(0, 0);
	ClearTexts();
	AddTimer(100, 100, 1, 1);
}

// 0x40df00
void Board::SkipFlyOff()
{
	PlayLayer* aLayer = GetPlayLayer();
	if (mFlightCount == 2)
		aLayer->AddScore(200);
	else if (mFlightCount == 1)
		aLayer->AddScore(500);
	ClearFlights();
	aLayer->SetMoves(0, 2);
	aLayer->AddScore((mLayout.TakeKinds() / 2) * 200);
	aLayer->AddScore(mLayout.TakeSpecials() * 500);
	EndRound();
}

// 0x40dfa0
Layout* Board::GetSaveLayout()
{
	if (mStarted && !mOver && mMove != 0)
		return &mLayout;
	return nullptr;
}

// 0x40dfd0
void Board::CheckStuck()
{
	if (mPairs < 1 && mLeft != 0)
	{
		if (mLayout.GetFreeSpecials(nullptr, 0) < 1)
			jb::gManager->Push(new NoMovesBox(), true, false, true);
		else if (!mStuckShown)
		{
			AddTimer(5, 0x32, 1, 1);
			mStuckShown = true;
		}
	}
}

// 0x40e090
void Board::ClearSelection()
{
	mSelected = nullptr;
	mSelAnim = 0;
	if (mSelSound != -1)
	{
		jb::StopSound(mSelSound);
		mSelSound = -1;
	}
}

// 0x40e0e0
void Board::OnEnabledChanged()
{
	if (IsEnabled())
		mTipShown = false;
}

} // namespace mahjongjapan
