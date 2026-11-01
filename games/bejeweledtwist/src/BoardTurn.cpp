// The board at rest: the next move, the tips for new kinds of gem and the turn's end
#include "Board.h"

#include "CustomBassMusicInterface.h"
#include "FlyingText.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Piece.h"
#include "Profile.h"
#include "Res.h"
#include "SoundInstance.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "WidgetManager.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

void Board::SettleTurn()
{
	mUnknown20C4 = mUnknown20C0;
	if (IsBusy() || mUnknown149C.size() != 0 || mUnknown21D8)
		return;
	CheckTips();
	ShowNextTip();
	CheckLevelDone();
	mUnknown15C = 0;
	// The original writes "Move FOUND at" the move, or "NO MOVE FOUND", to the debugger
	FindMove(&mUnknown201C, &mUnknown2020, false);
	if (mGameMode == MODE_CHALLENGE) {
		if (mUnknown2630 != 0) {
			// port: 0x6b3432 checks the challenge's goal; it comes with the challenges
			NotPorted("Board::SettleTurn: the challenge's goal (0x6b3432)");
		}
	} else if (mBonusColors.size() == 0) {
		// The tube's text once a bonus challenge is over
		SexyString aText;
		if (!IsBusy())
			aText = gApp->GetString(GTS(0x7dd398), GT(0x7dd370));
		else
			aText = gApp->GetString("", _S(""));
		mGoodTube->mText = aText;
	}
	// The gems that cannot move, and the special ones among them
	mUnknown2018 = 0;
	mUnknown2180 = 0;
	mUnknown2184 = 0;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr)
				continue;
			if (!aPiece->IsFlagSet(Piece::FLAG_BOMB) && !aPiece->IsFlagSet(Piece::FLAG_DEVIL) &&
				!aPiece->IsFlagSet(Piece::FLAG_1)) {
				if (CanMove(aPiece))
					continue;
			} else {
				mUnknown2184++;
			}
			mUnknown2180++;
		}
	}
	if (mUnknown21D8 || mUnknown21DA || mUnknown21D9 || !mUnknown25A4 || mUnknown248 != 0.0f ||
		mUnknown149C.size() != 0)
		return;
	if (!mUnknown214) {
		if (mUnknown265C != 4) {
			bool aFirst = mUnknown1390;
			if (mUnknown2584 != nullptr)
				mUnknown2584->MoveDone();
			EndTurn();
			if (!aFirst && !mUnknown21A2) {
				mUnknown1FB8++;
				AddStat(0x21, 1);
				if (mGameMode == MODE_CHALLENGE && mUnknown2630 != 0 && !mUnknown21A2) {
					// port: the challenge's turn (0x6d86c9 on app +0xf5c) and its goal (0x6b3432) come with the
					// challenges
					NotPorted("Board::SettleTurn: the challenge's turn");
				}
			}
		}
	} else {
		mUnknown214 = false;
		// port: 0x6ba5b4 sends a traveller to a gem (Rand picks it) and the points it brings (0x70205f, 0x6a43e8)
		// come with the travellers
		NotPorted("Board::SettleTurn: a traveller (0x6ba5b4)");
	}
}

void Board::QueueTip(int theKind, Piece* thePiece)
{
	if (mUnknown37C4)
		return;
	// The first gem of each kind; a kind already waiting keeps its gem
	mUnknown256C.insert(std::make_pair(theKind, thePiece == nullptr ? 0 : thePiece->mId));
}

void Board::ShowNextTip()
{
	if (mUnknown265C == 4 || mUnknown248 != 0.0f || mUnknown256C.empty() || mUnknown23F5)
		return;
	// The first waiting tip, by its kind, and the gem it points at
	std::map<int, int>::iterator aTip = mUnknown256C.begin();
	int aKind = aTip->first;
	int anId = aTip->second;
	mUnknown256C.erase(aTip);
	if (0 <= mApp->mAutoPlayMode) {
		// -autoplay sees every tip at once
		mApp->mProfile->SetFlag1E4(aKind, true);
		return;
	}
	Piece* aPiece = GetPieceById(anId);

	// The script that teaches the kind (the names are the labels in properties/tutorial.txt)
	std::string aName;
	switch (aKind) {
	case 0: aName = "flamegem"; break;
	case 1: aName = "lightninggem"; break;
	case 3: aName = "skullintro"; break;
	case 4: aName = "lockintro"; break;
	case 5: aName = "devilintro"; break;
	case 6: aName = "fruitintro"; break;
	case 7: aName = "coal"; break;
	case 8: aName = "timefreeze"; break;
	case 9: aName = "bombgem"; break;
	case 0xb: aName = "wheel"; break;
	case 0xc: aName = "chains2x"; break;
	case 0xd: aName = "chains3x"; break;
	case 0xe: aName = "basics"; break;
	case 0x12: aName = "supernova"; break;
	default: break;
	}
	if (aName.empty() || (aPiece == nullptr && anId != 0))
		return;

	mUnknown34E8 = aKind;
	mUnknown23F5 = true;
	// (the original prints "[RUNNING SCRIPT" and the name with OutputDebugString)
	if (mUnknown254C.GetDataLen() == 0) {
		// The script's lines, a line each (blank lines skipped, and a last line without an end of line)
		Buffer aFile;
		if (gApp->ReadBufferFromFile("properties/tutorial.txt", &aFile, true)) {
			SexyString aLine;
			for (unsigned char aChar : aFile.mData) {
				if (aChar == '\n' || aChar == '\r') {
					aLine = Trim(aLine);  // 0x45d880
					if (!aLine.empty())
						mUnknown254C.WriteString(aLine);
					aLine.clear();
				} else {
					aLine += (SexyChar)aChar;
				}
			}
		}
	}
	mUnknown254C.SeekFront();
	mUnknown2584 = new Tutorial(this, aPiece, aName.c_str(), &mUnknown254C);
	mUnknown2584->Resize(0, 0, mWidth, mHeight);
	mUnknown2584->mKind = aKind;
	mUnknown23DC.push_back(mUnknown2584);
}

void Board::ShowTube(bool theShow, bool theInstant)
{
	if (theShow) {
		mGoodTube->UpdateImages();
		if (!mUnknown2041) {
			mUnknown25A6 = true;
			if (mUnknown21D9) {
				mUnknown21D9 = false;
				LoadGame(mSaveBuffer);
			}
		}
		int aSpace = gBoard->mWidth - GetBoardWidth();
		int aHeight = mGoodTube->GetTubeHeight();
		int aWidth = mGoodTube->GetTubeWidth();
		aSpace -= gBoard->mGoodTube->GetTubeWidth();
		mGoodTube->Resize(Ftol((double)(aSpace / 3) - (double)mUnknown20E0), 0, aWidth, aHeight);
	}
	mUnknown2041 = theShow;
	if (theShow) {
		mGoodTube->mVisible = true;
		if (theInstant)
			mGoodTube->mX = 0;
		else
			StartUnknown3880();
	} else {
		mGoodTube->mVisible = false;
		if (theInstant)
			mGoodTube->mX = mApp->mWidth / 2 - GetImageOffsetX(0x3d6);
	}
}

void Board::CloseTip()
{
	Tutorial* aTip = mUnknown2584;
	if (aTip != nullptr) {
		if (aTip->mMarkSeen)
			mApp->mProfile->SetFlag1E4(aTip->mKind, true);
		if (mUnknown2584->mKind == 0xe) {
			mUnknown3904 = false;
			if (gApp->mUnknownF85) {
				mApp->EraseFile(mApp->GetSaveName(mGameMode, mUnknown2674));
				mUnknown36B0 = true;
			} else {
				mGoodTube->SetShown(true);
				mGoodTube->SetUnknown14D(true);
				mUnknown14F8 = 0;
				mUnknown168 = 0;
				mUnknown818 = 0;
				mUnknown224 = 0;
				SetLevelSteps(0);
			}
			mUnknown3918 = mApp->mProfile->mFlags1E0;
			mUnknown3914 = mApp->mProfile->mFlags1E4;
			mApp->mProfile->mRank = 0;
			mApp->mProfile->Save();
		}
		// (the board's dialog queue deletes it)
		mUnknown2584 = nullptr;
	}
	mUnknown145C = 1.0f;
	mUnknown21A0 = false;
	mUnknown1C88 = 1.0f;
	mUnknown1C90 = false;
	mUnknown1C8C = 1.0f;
	mApp->mWidgetManager->SetFocus(this);
	if (mUnknown23F5 && ((mUnknown3914 >> 0xe) & 1) == 0) {
		mApp->mProfile->SetFlag1E4(0xe, true);
		// 0x75fc8a
		if (mApp->mBoard == nullptr || !mApp->mBoard->mUnknown37C4)
			std::memset(mApp->mUnknownC74, 0, sizeof(mApp->mUnknownC74));
		std::memset(mUnknown2234, 0, sizeof(mUnknown2234));  // 0x6a69f0
		ReadConfig(true);
		gApp->mProfile->Save();
	}
	mUnknown3914 = mApp->mProfile->mFlags1E4;
	mUnknown23F5 = false;
}

bool Board::CheckTips()
{
	if (mUnknown23F5)
		return true;
	if (IsBusy() || mGameMode == MODE_BLITZ)
		return false;
	Piece* aKind1 = nullptr;
	Piece* aDevil = nullptr;
	Piece* aLocked = nullptr;
	Piece* aCoal = nullptr;
	Piece* aBomb = nullptr;
	Piece* aFruit = nullptr;
	Piece* anElectro = nullptr;
	Piece* aPower = nullptr;
	Piece* anElectroPower = nullptr;
	bool aBombAtZero = false;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr || aPiece->mFlags == 0)
				continue;
			if (aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
				aBomb = aPiece;
				if (aPiece->mBombCount == 0)
					aBombAtZero = true;
			} else if (aPiece->IsFlagSet(Piece::FLAG_1)) {
				aKind1 = aPiece;
			} else if (aPiece->IsFlagSet(Piece::FLAG_LOCKED)) {
				if (aPiece->IsFlagSet(Piece::FLAG_12))
					aDevil = aPiece;
				else
					aLocked = aPiece;
			} else if (aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
				aDevil = aPiece;
			} else if (aPiece->IsFlagSet(Piece::FLAG_FRUIT)) {
				aFruit = aPiece;
			} else if (!aPiece->IsFlagSet(Piece::FLAG_ELECTRO)) {
				if (aPiece->IsFlagSet(Piece::FLAG_POWER))
					aPower = aPiece;
				else if (aPiece->IsFlagSet(Piece::FLAG_COAL))
					aCoal = aPiece;
			} else if (aPiece->IsFlagSet(Piece::FLAG_POWER)) {
				anElectroPower = aPiece;
			} else {
				anElectro = aPiece;
			}
		}
	}
	if (aBombAtZero)
		return false;
	// Each tip once, unless the profile has seen it (its bit in +0x3914)
	bool aShown = false;
	if (aBomb != nullptr && (mUnknown3914 >> 9 & 1) == 0) {
		QueueTip(9, aBomb);
		aShown = true;
	}
	if (aFruit != nullptr && (mUnknown3914 >> 6 & 1) == 0) {
		QueueTip(6, aFruit);
		aShown = true;
	}
	if (aKind1 != nullptr && (mUnknown3914 >> 3 & 1) == 0) {
		QueueTip(3, nullptr);
		aShown = true;
	}
	if (aDevil != nullptr && (mUnknown3914 >> 5 & 1) == 0)
		QueueTip(5, aDevil);
	if (aLocked != nullptr && (mUnknown3914 >> 4 & 1) == 0)
		QueueTip(4, aLocked);
	if (anElectro != nullptr && (mUnknown3914 >> 1 & 1) == 0) {
		QueueTip(1, anElectro);
		aShown = true;
	}
	if (aPower != nullptr && (mUnknown3914 & 1) == 0) {
		QueueTip(0, aPower);
		aShown = true;
	}
	if (aCoal != nullptr && (mUnknown3914 >> 7 & 1) == 0) {
		QueueTip(7, aCoal);
		aShown = true;
	}
	if (anElectroPower != nullptr && (mUnknown3914 >> 0x12 & 1) == 0) {
		QueueTip(0x12, anElectroPower);
		aShown = true;
	}
	return aShown;
}

bool Board::DropFlaggedGems()
{
	if (IsSparkBlocked())
		return false;
	int aCol = 0;
	bool aDropped = false;
	if (0 < mCols) {
		do {
			int aRow = mRows;
			while (aRow--, -1 < aRow) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr)
					continue;
				if (aPiece->IsFlagSet(Piece::FLAG_1) && !aPiece->mUnknown51C && !aPiece->mBreaking) {
					Piece* aBelow = GetPieceAtXY(Ftol(aPiece->mX), Ftol((double)gGemHeight + (double)aPiece->mY));
					if (aBelow != nullptr && aBelow->mPendingDX == 0.0f && aBelow->mPendingDY == 0.0f &&
						!aBelow->mBreaking) {
						// port: the gem drops onto the one below (a 0x34-byte object, 0x6f4985, into +0x21e4, updated
						// by 0x6f465f); it comes with those objects
						NotPorted("Board::DropFlaggedGems: the drop (0x6f4985)");
						aDropped = true;
					} else {
						if ((double)GetRowY(mRows - 1) > (double)aPiece->mY)
							break;
						// At the bottom row the game ends and the board blows up
						EndGame(true);
					}
					aPiece->mBlasted = true;
					aPiece->mUnknown116 = true;
					break;
				}
				aPiece->mUnknown51C = false;
			}
			aCol++;
		} while (aCol < mCols);
	}
	return aDropped;
}

void Board::EndTurn()
{
	if (mUnknown37C4 || mUnknown2629)
		mUnknown3920 = true;
	mUnknown3918 = mApp->mProfile->mFlags1E0;
	mUnknown3914 = mApp->mProfile->mFlags1E4;
	if (!mUnknown1C6D) {
		int aMoves = mUnknown1E8 - mUnknown1EC;
		if (aMoves < mUnknown234)
			aMoves = mUnknown234;
		mUnknown234 = aMoves;
		mUnknown2016 = false;
	}
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr) {
				int aLeft = aPiece->mUnknown4FC;
				if (1 < aLeft) {
					aPiece->mLockFlash = 1.0f;
					aPiece->mUnknown4FC = aLeft - 1;
				}
				aPiece->mUnknown538 = 0;
				aPiece->mUnknown537 = false;
			}
		}
	}
	if (mCoalMax != 0) {
		if (mUnknown2234[74] == 0) {
			int aCoal = mUnknown1F64;
			mUnknown1F68++;
			if ((aCoal + 2) * 50 < mUnknown1F68) {
				mUnknown1F68 = 0;
				if (0 < aCoal)
					mUnknown1F64 = aCoal - 1;
			}
		} else {
			int aCoal = mUnknown1F64;
			if (aCoal < mUnknown2234[74])
				aCoal = mUnknown2234[74];
			mUnknown1F68 = 0;
			mUnknown1F64 = aCoal;
		}
	}
	if (CountDownCounters())
		return;
	if (!CheckTips()) {
		if (mUnknown1FB8 != mUnknown1FC4)
			mUnknown1FC4 = mUnknown1FB8;
	}
	ResetFruitCount();
	if (mUnknown2668) {
		// port: 0x6b3432 checks the challenge's goal; it comes with the challenges
		NotPorted("Board::EndTurn: the challenge's goal (0x6b3432)");
		mUnknown2668 = false;
	}
	if (mUnknown160 != 1) {
		int aMoves = mUnknown1E8 - mUnknown1EC;
		UpdateRecords();
		if (20 < aMoves || 3 < mUnknown1508 || mUnknown262A)
			mUnknown3904 = true;
	}
	if (mUnknown2629) {
		if (mUnknown2584 == nullptr && !mUnknown37C4)
			mUnknown3904 = true;
		if (mUnknown2629) {
			mWidgetFlagsMod.mAddFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
			mUnknown2629 = false;
		}
	}
	if (mSparkBlock < 1) {
		if (mUnknown1C79 && mUnknown2190 != mUnknown1FB8) {
			mUnknown2190 = mUnknown1FB8;
			DropFlaggedGems();
		}
	} else {
		mSparkBlock--;
		if (mSparkBlock == 0) {
			for (int i = 0; i < 2; i++) {
				if (mUnknown21C4[i] != nullptr) {
					mUnknown21C4[i]->Stop();
					mUnknown21C4[i]->Release();
					mUnknown21C4[i] = nullptr;
				}
			}
		}
	}
	if (!IsGameOverPending()) {
		if (-1 < mUnknown1C64) {
			// port: 0x6b40b5 comes with the gems the level asks for (+0x1c64)
			NotPorted("Board::EndTurn: 0x6b40b5");
			mUnknown1C64 = -1;
		}
		CheckCombos();
		mUnknown1390 = false;
		mUnknown21A2 = false;
		if (mGameMode != MODE_CHALLENGE && (mGameMode == MODE_BLITZ || 1 < mUnknown158) && mBonusColors.size() == 0)
			StartBonusChallenge(-1);
	}
}

void Board::UpdateRecords()
{
	// 0x6bcd96: the session's (the app's) and the player's (the profile's) best gems in a move, move's points and
	// cascade, each with the game as the level started for its replay; a challenge's count only for the session
	if (mUnknown158 == 0 || mUnknown37C4 || mUnknown2629)
		return;
	bool aForProfile = mGameMode != MODE_CHALLENGE;
	Profile* aProfile = mApp->mProfile;
	int aGems = mUnknown1F8;
	if (mApp->mUnknownFA0[0].mUnknown28 < aGems) {
		mApp->mUnknownFA0[0].mBuffer = mSaveBuffer;
		mApp->mUnknownFA0[0].mUnknown20 = mUnknown23A4;
		mApp->mUnknownFA0[0].mUnknown24 = mUnknown23A8;
		mApp->mUnknownFA0[0].mUnknown28 = aGems;
		mUnknown390C = aGems;
	}
	if (aForProfile && aProfile->mSavedGames[0].mBest < aGems) {
		aProfile->mSavedGames[0].mData = mSaveBuffer;
		aProfile->mSavedGames[0].mUnknown20 = mUnknown23A4;
		aProfile->mSavedGames[0].mUnknown24 = mUnknown23A8;
		aProfile->mSavedGames[0].mBest = aGems;
	}
	// the move's points, with the geodes still in flight
	int aPoints = 0;
	if (!mUnknown14AC.empty()) {
		// port: each geode's +0xa8 times +0x21d0, +0x2580 and the mode's multiplier; the geodes come with the coal
		NotPorted("Board::UpdateRecords: the geodes' points");
	}
	aPoints = mUnknown168 - mUnknown16C + aPoints;
	if (mApp->mUnknownFA0[2].mUnknown28 < aPoints) {
		mUnknown3910 = aPoints;
		mApp->mUnknownFA0[2].mBuffer = mSaveBuffer;
		mApp->mUnknownFA0[2].mUnknown20 = mUnknown23A4;
		mApp->mUnknownFA0[2].mUnknown24 = mUnknown23A8;
		mApp->mUnknownFA0[2].mUnknown28 = aPoints;
	}
	if (aForProfile && aProfile->mSavedGames[2].mBest < aPoints) {
		aProfile->mSavedGames[2].mData = mSaveBuffer;
		aProfile->mSavedGames[2].mUnknown20 = mUnknown23A4;
		aProfile->mSavedGames[2].mUnknown24 = mUnknown23A8;
		aProfile->mSavedGames[2].mBest = aPoints;
	}
	if (mApp->mUnknownFA0[1].mUnknown28 < mUnknown1508) {
		mUnknown3908 = mUnknown1508;
		mApp->mUnknownFA0[1].mBuffer = mSaveBuffer;
		mApp->mUnknownFA0[1].mUnknown20 = mUnknown23A4;
		mApp->mUnknownFA0[1].mUnknown24 = mUnknown23A8;
		mApp->mUnknownFA0[1].mUnknown28 = mUnknown1508;
	}
	if (aForProfile && aProfile->mSavedGames[1].mBest < mUnknown1508) {
		mUnknown2234[(0x22e4 - 0x2234) / 4] = mUnknown1508;
		aProfile->mSavedGames[1].mData = mSaveBuffer;
		aProfile->mSavedGames[1].mUnknown20 = mUnknown23A4;
		aProfile->mSavedGames[1].mUnknown24 = mUnknown23A8;
		aProfile->mSavedGames[1].mBest = mUnknown1508;
	}
	if (mUnknown238 < mUnknown1508)
		mUnknown238 = mUnknown1508;
}

bool Board::IsGameOverPending()
{
	if (mUnknown36B0)
		return true;
	if (mGameMode < MODE_CHALLENGE || MODE_BLITZ < mGameMode) {
		// The music is in danger while +0x262c is below 6
		CustomBassMusicInterface* aMusic = mApp->mTwistMusic;
		if (aMusic != nullptr)
			aMusic->SetState(mUnknown262C < 6 ? 1 : 0);
	}
	return false;
}

namespace {

const int gPraiseRatings[6] = {3, 6, 12, 20, 30, 45};          // 0x84e960: the least rating for each praise
const int gPraiseSounds[6] = {951, 947, 940, 953, 957, 961};   // 0x7dbf00
const int gPraiseImages[6] = {721, 676, 580, 725, 759, 763};   // 0x7dbf18

}  // namespace

void Board::CheckCombos()
{
	bool aDemo = mUnknown37C4;
	mUnknown2228 = 0;
	bool aShow = !CheckLevelDone() && !aDemo;
	if (mGameMode == MODE_BLITZ && mUnknown3458 <= (1000 / mApp->mFrameTime) * 10)
		aShow = false;
	int aRating = GetTurnRating();
	int aLevel = 5;
	if (mUnknown1F80 < 5) {
		do {
			if (gPraiseRatings[aLevel] <= aRating) {
				if (0 < aLevel && !mUnknown37C4)
					mUnknown3904 = true;
				if (aShow)
					ShowPraise(aLevel);
				return;
			}
			aLevel--;
		} while (mUnknown1F80 < aLevel);
	}
}

int Board::GetTurnRating()
{
	mUnknown3804 = 0;
	int aPart = mUnknown2234[74] * 2 - 1;
	if (aPart < 0)
		aPart = 0;
	mUnknown3804 = aPart;
	float aCascades;
	if (0.0f <= (float)std::pow((double)(float)(mUnknown1508 - 1), (double)1.5f))  // 0x670a49
		aCascades = (float)std::pow((double)(float)(mUnknown1508 - 1), (double)1.5f);
	else
		aCascades = 0;
	mUnknown3804 += Ftol((double)aCascades);
	aPart = mUnknown2234[64] * 2 - 1;
	if (aPart < 0)
		aPart = 0;
	int aSum = mUnknown3804 + aPart;
	int aPart2 = mUnknown2234[65] * 2 - 1;
	mUnknown3804 = aSum;
	if (aPart2 < 0)
		aPart2 = 0;
	int aPart3 = mUnknown2234[20] * 2 - 1;
	mUnknown3804 = mUnknown2234[69] * mUnknown2234[69] + mUnknown2234[72] + mUnknown2234[71] + mUnknown2234[21] + aSum +
		aPart2;
	if (aPart3 < 0)
		aPart3 = 0;
	mUnknown3804 += aPart3;
	aPart = mUnknown2234[28] * 8 - 40;
	if (aPart < 0)
		aPart = 0;
	mUnknown3804 += aPart;
	mUnknown3804 += Ftol(std::pow((double)mUnknown1F8 / 15.0, 1.5));
	return mUnknown3804;
}

void Board::ShowPraise(int theLevel)
{
	if (mGameMode == MODE_CHALLENGE || mUnknown21A3 || IsBusy())
		return;
	if (mApp->mUnmanagedSound != nullptr) {
		// another voice is still speaking: UpdateGame shows it once that has faded
		mUnknown1F84 = theLevel;
		return;
	}
	int aSound = gPraiseSounds[theLevel];
	mUnknown222C = mUpdateCnt;
	mUnknown1F80 = theLevel;
	int anUp = Ftol((double)gApp->mScale * 250.0);
	int aY = GetRowY(0) + (mRows * gGemHeight / 2 - anUp);
	int aX = GetColX(0) + mCols * gGemWidth / 2;
	FlyingText* aText =
		new FlyingText(0, GetImageById(gPraiseImages[theLevel]), aX, aY, 1.0f, GetSoundById(aSound), 0.0f);
	aText->mQueued = false;
	mFlyingTexts.push_back(aText);
	// (the original also notes the update count for each praise, 0x85a9d0, which nothing reads)
}

void Board::ShowMenu()
{
	// 0x6a6983
	if (mUnknown138C != 0) {
		// port: the original deletes the object at +0x138c here (virtual slot 1) and clears it; nothing sets it in
		// the port
		mUnknown138C = 0;
	}
	mApp->ShowOptionsDialog();
}

void Board::AskQuit()
{
	// 0x6b38f3: the quit question, by the game's mode; the board hears the answer
	SexyString aHeader;
	SexyString aLines;
	if (mGameMode == MODE_CHALLENGE) {
		if (!IsTimed()) {
			aHeader = mApp->GetString(GTS(0x7dc6a0), GT(0x7dc680));
			aLines = mApp->GetString(GTS(0x7dc638), GT(0x7dc5a8));
		} else {
			aHeader = mApp->GetString(GTS(0x7dc794), GT(0x7dc788));
			aLines = mApp->GetString(GTS(0x7dc740), GT(0x7dc6b0));
		}
	} else if (mGameMode == MODE_BLITZ) {
		aHeader = mApp->GetString(GTS(0x7da148), GT(0x7da12c));
		aLines = mApp->GetString(GTS(0x7dc820), GT(0x7dc7a0));
	} else if (mGameMode == MODE_ZEN) {
		aHeader = mApp->GetString(GTS(0x7dc93c), GT(0x7dc920));
		aLines = mApp->GetString(GTS(0x7dc8e0), GT(0x7dc860));
	} else {
		aHeader = mApp->GetString(GTS(0x7da148), GT(0x7da12c));
		aLines = mApp->GetString(GTS(0x7dc9c8), GT(0x7dc950));
	}
	TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, 1, true, aHeader, aLines,
		GT(0x80d9a4), 3);
	aDialog->mTwistListener = this;
	double aMinWidth = (double)gGemWidth * 6.75;
	double aWidth = (double)aDialog->GetPreferredWidth() <= aMinWidth ? aMinWidth : (double)aDialog->GetPreferredWidth();
	aDialog->mWidth = Ftol(aWidth);
	aDialog->mHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
	aDialog->mZOrder = 200;
	mApp->AddDialog(aDialog);
	aDialog->mX = mWidth / 2 - aDialog->mWidth / 2;
	aDialog->mY = mHeight / 2 - aDialog->mHeight / 2;
	aDialog->mYesButton->mLabel = mApp->GetString(GTS(0x812e14), GT(0x7db674));
	aDialog->mNoButton->mLabel = mApp->GetString(GTS(0x812e10), GT(0x7db66c));
}

void Board::DialogButtonDepress(int theDialogId, int theButtonId)
{
	// 0x6c8632
	if (theDialogId == 1) {
		mApp->KillDialog(1);
		if (!mApp->mLoadingThreadCompleted) {
			if (mUnknown2584 != nullptr)  // (the original does not look)
				mUnknown2584->mState = 0x10;
		} else {
			Tutorial* aTip = mUnknown2584;
			if (aTip != nullptr) {
				if (mUnknown2208 == aTip)
					mUnknown2208 = nullptr;
				if (aTip->mKind == 0xe) {
					mApp->mProfile->MarkTutorialDone();
					mUnknown3918 = mApp->mProfile->mFlags1E0;
					mUnknown3914 = mApp->mProfile->mFlags1E4;
				}
				aTip->mWidgetManager->RemoveWidget(aTip);
				mUnknown830.SetInVal(0.0);
				mApp->SafeDeleteWidget(aTip);
				mUnknown2584 = nullptr;
			}
			if (theButtonId == 1000) {
				if (mApp->mOptionsDialog != nullptr)
					mApp->FinishOptionsDialog(false);
				if (mGameMode == MODE_ZEN)
					UpdateStats();
				mUnknown34EC = 1;
				if (mApp->mMusicInterface != nullptr) {
					// port: the original keeps BASS's position in the music playing (0x6ee282) here (+0x37dc); nothing
					// reads it back, so the port keeps 0 (as in RemovedFromManager)
				}
			}
		}
	} else if (theDialogId == 0x28) {
		if (theButtonId == 4) {
			ShowMenu();
		} else if (theButtonId == 0x3e9) {
			if (!mUnknown23F5) {
				mApp->mProfile->mUnknown1B1 = false;
				mApp->mProfile->Save();
			} else {
				CloseTip();
			}
		}
	}
}
