// The board at rest: the next move, the tips for new kinds of gem and the turn's end
#include "Board.h"

#include "CustomBassMusicInterface.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Piece.h"
#include "Profile.h"
#include "SoundInstance.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "WidgetManager.h"

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
			if (mUnknown2584 != nullptr) {
				// port: the dialog's 0x750b8c (its +0x21c set when its +0x16c is 5) comes with the dialog
				NotPorted("Board::SettleTurn: the +0x2584 dialog");
			}
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
	// port: 0x6bbc58 puts up the first waiting tip ("flamegem" and the rest) pointing at its gem, or with -autoplay
	// marks it seen (0x7338b6); it comes with the tips' dialog
	NotPorted("Board::ShowNextTip (0x6bbc58)");
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
	if (mUnknown2664 == mUnknown2660 || (mUnknown2660 == 0 && mUnknown2664 != -1)) {
		// 0x6b35b1: the fruit's count starts again; a fruit dance that has begun (3) moves on to its end
		mUnknown2660 = 0;
		mUnknown2664 = 0;
		if (mUnknown265C == 3) {
			mUnknown265C = 5;
			mGoodTube->mUnknown5F0.SetCurve(GTS(0x7dc558));
		}
	}
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
			// port: 0x6bb7e3 comes with the tutorial's gems (+0x1c79)
			NotPorted("Board::EndTurn: 0x6bb7e3");
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
	if (mUnknown158 != 0 && !mUnknown37C4 && !mUnknown2629) {
		// port: 0x6bcd96 keeps the game's best move, cascade and score with the game saved at the level's start (for
		// their replays) in the app and the profile; it comes with the replays
		NotPorted("Board::UpdateRecords (0x6bcd96)");
	}
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

void Board::CheckCombos()
{
	// port: 0x6bbbc2 rates the turn (0x6a5179) and shows its praise (0x6ba73a, "Good", "Excellent" and the rest);
	// it comes with the praise
	mUnknown2228 = 0;
	NotPorted("Board::CheckCombos (0x6bbbc2)");
}
