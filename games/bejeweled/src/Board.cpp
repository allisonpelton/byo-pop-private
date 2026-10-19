// Board.cpp, part 2 (0x4cb5bc..0x4ce44f): construction and destruction, drawing, the savegame, scoring
// and the per-tick update with the falling / swap / vortex states.
// The Board methods below the constructor's address are in BoardRules.cpp.
// Behaviour spec: games/bejeweled/docs/GAME_NOTES.md (section 3).

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Res.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "DDImage.h"
#include "ImageFont.h"
#include "SoundInstance.h"
#include "Buffer.h"
#include "ReplayRecorder.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// port: the original prefixes GetAppDataFolder(), which is "" before Vista; the framework maps
// "userdata\\" to the user dir and creates it when writing. Gameplay mods get their own save.
std::string Board::GetSaveGameFileName()
{
	return GameConfig::ScopedFileName("userdata\\savegame.dat");
}

static bool ReadSaveBool(const Buffer& theBuffer)
{
	uchar aByte = 0;
	theBuffer.ReadBytes(&aByte, 1);
	return aByte != 0;
}

static void DrawSparkle(Graphics* g, Image* theSparkleImage, int theFrame, int theX, int theY)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->DrawImage(theSparkleImage, theX, theY, Rect(theFrame * 40, 0, 40, 40));
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x4cb5bc
Board::Board(WinDMApp* theApp, bool timedMode)
{
	// port: the original leaves these uninitialised; the cells must be NULL before the first FillBoard
	memset(mBoard, 0, sizeof(mBoard));
	InitVariant(); // port(variant): rules snapshot, layout and variant flags (BoardVariant.cpp)
	mState = STATE_FALLING;
	mSwapAngle = 0;
	mSwapCenterX = 0;
	mSwapCenterY = 0;
	mMouseX = -1;
	mMouseY = -1;
	mMouseDownX = 0;
	mMouseDownY = 0;
	mTextPhase = 0.0;
	mTextDuration = 0;
	mTextTimeLeft = 0;
	mWipeY = 0;
	mBlackHoleFrame = 0;
	mLevelTextShown = false;

	mMoveScoreAlpha = 0.0;
	mHintGlowAlpha = 0.0;
	mTimedMode = timedMode;
	mTimeDrain = 0.0;
	mTimeDrainRate = 0.0;
	mIsNewGame = true;
	mApp = theApp;
	mCursorFrame = 0;
	mSelectedPiece = NULL;
	mLastLandSoundTick = 0;
	mGameOverCountdown = 0;
	mNewGamePending = false;
	mShowTutorial = false;
	mGameDuration = 0;
	mPlaySeconds = 0;
	mGemsCleared = 0;
	mLongestCascade = 0;
	mBestMoveScore = 0;
	mScore = 0;
	mDisplayScore = 0;
	mBarColor = BAR_COLOR_NORMAL;
	mLastWarningTick = 0;
	mMoveScore = 0;
	mUnused218 = 0;
	mMoveScoreTimer = 0;
	mHintGlowTimer = 0;
	mHintGlowDuration = 0;
	mInputDelay = 0;
	mMouseDown = false;
	mPlayStartTick = 0;
	mPauseCount = 0;
	mUserPauseCount = 0;
	mStatsDisabled = false;
	mLevelStartScore = 0;
	mUnused264 = 0;
	mSwapPiece1 = NULL;
	mSwapPiece2 = NULL;
	mPointsFont = mApp->mAlphaDigitsFont;
	mScoreFont = mApp->mScoreFont;
	mLevel = 0;
	mDisplayLevel = 1;

	mBackgroundImage = new DDImage(mApp->mDDInterface);
	mBackgroundImage->SetImageMode(false, false);
	mBackgroundImage->Create(640, 480);
	mNextBackgroundImage = new DDImage(mApp->mDDInterface);
	mNextBackgroundImage->SetImageMode(false, false);
	mNextBackgroundImage->Create(640, 480);
	SetupLevel();
	mLevelTargetScore = mLevelPoints;

	Graphics g(mBackgroundImage);
	g.DrawImage(mNextBackgroundImage, 0, 0);

	mTutorialCol = -1;
	mBlackHoleLevel = 0.0;
	mTutorialRow = -1;
	mTutorialCol2 = -1;
	mBackgroundChanged = false;
	mBarWidth = 0;
	mTimeDrainDelay = gGameConfig.mStartDrainDelay;
	mTextImage = NULL;
	mTextColorImage = NULL;
	mShowingText = false;
	mElectOffset = 0;
	mWhirlpoolSound = NULL;
	mMoveMatchCount = 0;
	mCascadeCount = 0;
	mTutorialBob = 0;
	mTutorialBobDir = 1;

	// Boards are generated until one has a horizontal-run move to the right in rows 4..6, columns
	// 1..6, for the first-game tutorial arrows; the rejected boards still consume Rand()
	do
	{
		FillBoard();
		int aMove[4];
		for (int i = 0; FindMove(aMove, i, true, false); i++)
		{
			if ((aMove[2] == aMove[0] + 1) && (aMove[1] >= 4) && (aMove[1] <= 6) && (aMove[0] >= 1) && (aMove[0] < 7))
			{
				mTutorialCol = aMove[0];
				mTutorialRow = aMove[1];
				mTutorialCol2 = aMove[2];
				break;
			}
		}
	}
	while (mTutorialCol == -1);
}

// 0x4cb931
void Board::UpdateBlackHole()
{
	mBlackHoleLevel += 0.02;
	if (mBlackHoleLevel > 1.0)
		mBlackHoleLevel = 1.0;
	if (mWhirlpoolSound != NULL)
		mWhirlpoolSound->SetVolume(mBlackHoleLevel);

	int aCenterX;
	int aCenterY;
	GetBoardCenter(&aCenterX, &aCenterY);
	int aWidth;
	int aHeight;
	GetBoardSize(&aWidth, &aHeight);
	// The spin speeds up by (aMaxRadius - radius) / 110000 per tick, so gems whirl faster near the
	// centre. sqrtf (0x4c9ab7) computes in double and rounds to float; aMaxRadius = 574 / 2.2 = 260.
	float aDiagonalSq = (float)((double)aWidth * aWidth + (double)(aHeight * aHeight));
	int aMaxRadius = (int)((int)(float)sqrt((double)aDiagonalSq) / 2.2);

	double aMaxRadiusBefore = 0.0;
	double aMaxRadiusAfter = 0.0;
	bool aDone = true;
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;

			aPiece->mX = cos(aPiece->mHoleAngle) * aPiece->mHoleRadius + aCenterX - 24.0;
			aPiece->mY = sin(aPiece->mHoleAngle) * aPiece->mHoleRadius + aCenterY - 24.0;
			if (aPiece->mHoleRadius >= aMaxRadiusBefore)
				aMaxRadiusBefore = aPiece->mHoleRadius;

			aPiece->mHoleRadius -= 1.0;
			// A radius that lands exactly on 0 keeps the vortex running for one more tick
			if (aPiece->mHoleRadius >= 0.0)
				aDone = false;
			else
				aPiece->mHoleRadius = 0.0;
			if (aPiece->mHoleRadius >= aMaxRadiusAfter)
				aMaxRadiusAfter = aPiece->mHoleRadius;

			aPiece->mHoleAngVel += (aMaxRadius - aPiece->mHoleRadius) / 110000.0;
			aPiece->mHoleAngle += aPiece->mHoleAngVel;

			if (mUpdateCnt % 8 == 0)
			{
				// Shrink frame from the radius: (int) truncates toward zero, so the gem starts dying inside
				// 96 px; frame 5 at the centre, reached one step per 8 ticks
				int aFrame = (int)((6.0 - aPiece->mHoleRadius * 6.0 / 96.0) - 1.0);
				if (aFrame >= 0)
				{
					if (!aPiece->mDying)
					{
						aPiece->mDyingFrame = 0;
						aPiece->mDying = true;
					}
					else if (aFrame > aPiece->mDyingFrame)
						aPiece->mDyingFrame++;
				}
			}
		}
	}

	if ((aMaxRadiusBefore >= 80.0) && (aMaxRadiusAfter < 80.0))
		mApp->PlaySample(SOUND_LEVELUP);
	if ((aMaxRadiusBefore >= 64.0) && (aMaxRadiusAfter < 64.0))
	{
		mLevelTextShown = true;
		ShowText(StrFormat(mApp->GetString("LEVEL").c_str(), mLevel + 2), 400);
	}

	if (aDone)
	{
		mLevel++;
		SetupLevel();
		for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
		{
			for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
			{
				delete mBoard[aRow][aCol];
				mBoard[aRow][aCol] = NULL;
			}
		}
		mLevelTargetScore = mLevelPoints + mScore;
		mLevelStartScore = mScore;
		mApp->PlaySample(SOUND_FIRECRACKLE);
		mWipeY = 0;
		mState = STATE_LEVEL_WIPE;
	}

	MarkDirty();
}

// 0x4cbc70
void Board::Draw(Graphics* g)
{
	int aCenterX;
	int aCenterY;
	GetBoardCenter(&aCenterX, &aCenterY);

	g->DrawImage(mBackgroundImage, 0, 0);
	if (mBlackHoleLevel > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mBlackHoleLevel * 255.0)));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[IMAGE_BLACKHOLE], aCenterX - 128, aCenterY - 128, Rect(mBlackHoleFrame * 256, 0, 256, 256));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	if (mUserPauseCount == 0)
	{
		Graphics aBoardG(*g);
		aBoardG.ClipRect(194, 12, 428, 416);

		for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
		{
			for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece == NULL)
					continue;

				int aSrcX = aPiece->mSpinFrame * 52 + 2;
				int aSrcY = aPiece->mColor * 104;
				if ((aPiece->mGlowing) && (aPiece->mSpinFrame == 0) && (!aPiece->mDying))
				{
					Image* aGemSheet = mApp->mImages[IMAGE_GEMSHEET];
					DDImage* aGlowImage = mApp->CreateCrossfadeImage(aGemSheet, Rect(aSrcX, aSrcY, GEM_SIZE, GEM_SIZE),
						aGemSheet, Rect(aPiece->mGlowFrame * 52 + aSrcX, aSrcY + 52, GEM_SIZE, GEM_SIZE), aPiece->mGlowAlpha);
					aBoardG.DrawImage(aGlowImage, (int)aPiece->mX, (int)aPiece->mY);
					delete aGlowImage;
				}
				else if (aPiece->mDying)
					aBoardG.DrawImage(mApp->mGemShrinkImages[aPiece->mColor][aPiece->mDyingFrame], (int)aPiece->mX, (int)aPiece->mY);
				else
					aBoardG.DrawImage(mApp->mImages[IMAGE_GEMSHEET], (int)aPiece->mX, (int)aPiece->mY, Rect(aSrcX, aSrcY, GEM_SIZE, GEM_SIZE));
			}
		}

		// Hint sparkles (unclipped): three sparkles start 8 frames apart and stop in the same order as
		// the count runs out (count >= 16, >= 8, > 0). port: with mods in use a fourth one starts another
		// 8 frames later on the right and stops with the third (ModIndicators.h)
		Image* aSparkleImage = mApp->mImages[IMAGE_SPARKLE];
		for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
		{
			for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if ((aPiece == NULL) || (aPiece->mSparkleCount <= 0))
					continue;

				if (aPiece->mSparkleCount >= 16)
					DrawSparkle(g, aSparkleImage, aPiece->mSparkleFrame % 14, (int)(aPiece->mX + 13.0), (int)(aPiece->mY - 7.0));
				if (aPiece->mSparkleCount >= 8)
				{
					int aFrame = (aPiece->mSparkleFrame - 8) % 14;
					if (aFrame >= 0)
						DrawSparkle(g, aSparkleImage, aFrame, (int)(aPiece->mX - 10.0), (int)(aPiece->mY + 3.0));
				}
				int aFrame = (aPiece->mSparkleFrame - 16) % 14;
				if (aFrame >= 0)
					DrawSparkle(g, aSparkleImage, aFrame, (int)(aPiece->mX + 5.0), (int)(aPiece->mY + 19.0));
				if (gGameConfig.mAnyModEnabled)
				{
					int aModFrame = (aPiece->mSparkleFrame - 24) % 14;
					if (aModFrame >= 0)
						DrawSparkle(g, aSparkleImage, aModFrame, (int)(aPiece->mX + 24.0), (int)(aPiece->mY + 6.0));
				}
			}
		}

		Piece* aCursorPieces[3] = {mSelectedPiece, mSwapPiece1, mSwapPiece2};
		for (int i = 0; i < 3; i++)
		{
			if (aCursorPieces[i] != NULL)
				g->DrawImage(mApp->mImages[IMAGE_TARGET], (int)(aCursorPieces[i]->mX - 2.0), (int)(aCursorPieces[i]->mY - 2.0), Rect((mCursorFrame % 6) * 52, 0, 52, 52));
		}

		g->SetColor(Color::White);
		for (std::list<ScorePopup>::iterator anItr = mScorePopups.begin(); anItr != mScorePopups.end(); ++anItr)
		{
			g->SetFont(mApp->mAlphaDigitsFont);
			g->SetColor(Color(255, 255, 255, std::min(255, anItr->mLife * 255 / 18)));
			SexyString aPoints = StrFormat("%d", anItr->mPoints);
			g->DrawString(aPoints, anItr->mX - mPointsFont->StringWidth(aPoints) / 2, mPointsFont->GetAscent() / 2 + anItr->mY);
		}

		if (mShowTutorial)
		{
			g->DrawImage(mApp->mImages[IMAGE_SWAPPER1], GetColX(mTutorialCol) + 3, GetRowY(mTutorialRow) + 38);
			// The hand moves to the second gem once the first one is selected
			int aHandCol = mTutorialCol;
			if ((mSelectedPiece != NULL) && (mSelectedPiece->mCol == mTutorialCol) && (mSelectedPiece->mRow == mTutorialRow))
				aHandCol = mTutorialCol2;
			g->DrawImage(mApp->mImages[IMAGE_SWAPPER2], GetColX(aHandCol) - 5, GetRowY(mTutorialRow) - 42 + mTutorialBob);
		}

		if (mState == STATE_LEVEL_WIPE)
		{
			// The elect strip runs along the wipe line; the frame is redrawn over it inside the clip
			Graphics aWipeG(*g);
			aWipeG.ClipRect(0, mWipeY - 8, mWidth, 16);
			aWipeG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			for (int i = 0; i < (mWidth + mElectOffset + 127) / 128; i++)
				aWipeG.DrawImage(mApp->mImages[IMAGE_ELECT], i * 128 - mElectOffset, mWipeY - 8);
			aWipeG.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			aWipeG.DrawImage(mApp->mImages[IMAGE_ALPHABACK], 0, 0);
		}

		if (mShowingText)
		{
			ColorizeText();
			g->DrawImage(mTextColorImage, aCenterX - mTextColorImage->GetWidth() / 2, aCenterY - mTextColorImage->GetHeight() / 2 - mHeight / 6);
		}

		if (mBlackHoleLevel > 0.0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int)(mBlackHoleLevel * 255.0)));
			Image* aHoleMask = mApp->mImages[IMAGE_HOLEMASK];
			g->DrawImage(aHoleMask, aCenterX - aHoleMask->GetWidth() / 2, aCenterY - aHoleMask->GetHeight() / 2);
			g->SetColorizeImages(false);
		}
	}
	else if ((mHasFocus) || (!mApp->mHasFocus))
	{
		// Paused: the gems are hidden
		g->SetFont(mApp->mHeaderFont);
		g->SetColor(Color::White);
		SexyString aText = mApp->GetString("GAME_PAUSED");
		g->DrawString(aText, aCenterX - g->GetFont()->StringWidth(aText) / 2, aCenterY - g->GetFont()->GetHeight() / 2);
		aText = mApp->GetString("CLICK_HERE_TO_CONTINUE");
		g->DrawString(aText, aCenterX - g->GetFont()->StringWidth(aText) / 2, aCenterY + g->GetFont()->GetHeight());
	}

	g->SetFont(mApp->mScoreFont);
	SexyString aScore = StrFormat("%d", mDisplayScore);
	g->SetColor(Color(173, 231, 255));
	g->DrawString(aScore, 93 - mScoreFont->StringWidth(aScore) / 2, 106);

	g->DrawImage(mApp->mTimerBarImages[mBarColor], 237, 444, Rect(0, 0, mBarWidth, 25));

	SexyString aLevel = StrFormat("%d", mDisplayLevel);
	g->SetFont(mApp->mHeaderFont);
	g->SetColor(Color::Black);
	g->DrawString(aLevel, 606 - g->GetFont()->StringWidth(aLevel) / 2, 465);

	if (mMoveScoreAlpha > 0.0)
	{
		// Centred at x 91, shifted left by a third of the '+' so the digits sit nearer the centre
		SexyString aMoveScore = StrFormat("+%d", mMoveScore);
		g->SetFont(mApp->mHeaderFont);
		g->SetColor(Color(255, 255, 255, (int)(mMoveScoreAlpha * 255.0)));
		g->DrawString(aMoveScore, 91 - g->GetFont()->StringWidth(aMoveScore) / 2 - g->GetFont()->CharWidth('+') / 3, 350);
	}

	if (mHintGlowAlpha > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mHintGlowAlpha * 255.0)));
		g->DrawImage(mApp->mImages[IMAGE_OVER_HINT], 64, 303);
		g->SetColorizeImages(false);
	}
}

// 0x4cca37
bool Board::SaveGameExists()
{
	return mApp->FileExists(GetSaveGameFileName());
}

// 0x4cca98
bool Board::LoadGame()
{
	Buffer aBuffer;
	if (!ReadSaveFile(&aBuffer)) // port(variant): was mApp->ReadBufferFromFile(GetSaveGameFileName(), &aBuffer, false); playback reads mPlaybackLoad
		return false;

	int aVersion = 0;
	aBuffer.ReadBytes((uchar*)&aVersion, 4);
	if (aVersion != SAVEGAME_VERSION)
		return false;

	mApp->SetTimedMode(ReadSaveBool(aBuffer), false);
	aBuffer.ReadBytes((uchar*)&mLevel, 4);
	aBuffer.ReadBytes((uchar*)&mDisplayLevel, 4);
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			delete mBoard[aRow][aCol];
			uchar aPresent = 0;
			aBuffer.ReadBytes(&aPresent, 1);
			if (aPresent == 0)
				mBoard[aRow][aCol] = NULL;
			else
			{
				// The whole struct is stored raw; only the position is snapped back to the grid
				Piece* aPiece = new Piece();
				aBuffer.ReadBytes((uchar*)aPiece, Piece::SAVE_SIZE); // port(variant): was sizeof(Piece)
				aPiece->mX = GetColX(aPiece->mCol);
				aPiece->mY = GetRowY(aPiece->mRow);
				mBoard[aRow][aCol] = aPiece;
			}
		}
	}

	aBuffer.ReadBytes((uchar*)&mState, 4);
	if ((mState == STATE_SWAPPING) || (mState == STATE_SWAPPING_BACK))
		mState = STATE_PLAYING;
	mTimedMode = ReadSaveBool(aBuffer);
	mIsNewGame = ReadSaveBool(aBuffer);
	aBuffer.ReadBytes((uchar*)&mBarWidth, 4);
	aBuffer.ReadBytes((uchar*)&mPlaySeconds, 4);
	aBuffer.ReadBytes((uchar*)&mIdleTicks, 4);
	aBuffer.ReadBytes((uchar*)&mScore, 4);
	aBuffer.ReadBytes((uchar*)&mPointMultiplier, 8);
	aBuffer.ReadBytes((uchar*)&mDisplayScore, 4);
	aBuffer.ReadBytes((uchar*)&mGemsCleared, 4);
	aBuffer.ReadBytes((uchar*)&mMoveMatchCount, 4);
	aBuffer.ReadBytes((uchar*)&mCascadeCount, 4);
	aBuffer.ReadBytes((uchar*)&mLongestCascade, 4);
	aBuffer.ReadBytes((uchar*)&mBestMoveScore, 4);
	mStatsDisabled = ReadSaveBool(aBuffer);
	aBuffer.ReadBytes((uchar*)&mUnused264, 4);
	aBuffer.ReadBytes((uchar*)&mLevelStartScore, 4);
	aBuffer.ReadBytes((uchar*)&mLevelTargetScore, 4);
	aBuffer.ReadBytes((uchar*)&mLevelPoints, 4);
	aBuffer.ReadBytes((uchar*)&mTimeDrain, 8);
	aBuffer.ReadBytes((uchar*)&mTimeDrainRate, 8);
	aBuffer.ReadBytes((uchar*)&mTimeDrainAccel, 8);
	aBuffer.ReadBytes((uchar*)&mBarColor, 4);
	aBuffer.ReadBytes((uchar*)&mWipeY, 4);
	aBuffer.ReadBytes((uchar*)&mBlackHoleFrame, 4);
	aBuffer.ReadBytes((uchar*)&mBlackHoleLevel, 8);
	mLevelTextShown = ReadSaveBool(aBuffer);

	mApp->mShowInstructions = false;
	GameLoadedVariant(aBuffer); // port(variant): the RNG the sidecar kept, and the LOAD event (BoardVariant.cpp)
	return true;
}

// 0x4ccdfe
bool Board::SaveGame()
{
	if (mRecorder != NULL) mRecorder->BeginSave(); // port(variant): the Pause and Unpause below are part of the SAVE event
	// Commits the play seconds so far
	Pause(false);
	Unpause(false);

	Buffer aBuffer;
	WriteSaveGame(&aBuffer, mPlaySeconds, mState);

	// A swap in progress is saved with its state, then the live board drops to STATE_PLAYING (the
	// original drops it right after writing mState; nothing written later depends on it)
	if ((mState == STATE_SWAPPING) || (mState == STATE_SWAPPING_BACK))
		mState = STATE_PLAYING;

	return WriteSaveFile(aBuffer); // port(variant): was mApp->WriteBufferToFile(GetSaveGameFileName(), &aBuffer); also SAVE and the sidecar
}

// port: the file a shutdown would leave now, but the game goes on untouched (WinDMApp::GetShutdownSaves).
// A shutdown saves twice (WinDMApp::Shutdown, then the deferred ~Board), so the file holds a swap in
// progress as STATE_PLAYING, which the first save dropped it to. The play seconds Pause() would commit
// are counted without committing them (committing drops the fraction of a second), and a swap in
// progress keeps animating.
void Board::SaveGameSnapshot(Buffer* theBuffer)
{
	int aPlaySeconds = mPlaySeconds;
	if (mPauseCount == 0)
		aPlaySeconds += (int)((uint)(mUpdateCnt - mPlayStartTick) / 100);

	int aState = mState;
	if ((aState == STATE_SWAPPING) || (aState == STATE_SWAPPING_BACK))
		aState = STATE_PLAYING;

	WriteSaveGame(theBuffer, aPlaySeconds, aState);
}

// Body of 0x4ccdfe: raw little-endian fields, no padding; a present cell is followed by the 0x78
// Piece bytes
void Board::WriteSaveGame(Buffer* theBuffer, int thePlaySeconds, int theState)
{
	Buffer& aBuffer = *theBuffer;
	int aVersion = SAVEGAME_VERSION;
	aBuffer.WriteBytes((const uchar*)&aVersion, 4);
	aBuffer.WriteBytes((const uchar*)&mTimedMode, 1);
	aBuffer.WriteBytes((const uchar*)&mLevel, 4);
	aBuffer.WriteBytes((const uchar*)&mDisplayLevel, 4);
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			uchar aPresent = (aPiece != NULL) ? 1 : 0;
			aBuffer.WriteBytes(&aPresent, 1);
			if (aPresent != 0)
				aBuffer.WriteBytes((const uchar*)aPiece, Piece::SAVE_SIZE); // port(variant): was sizeof(Piece)
		}
	}

	aBuffer.WriteBytes((const uchar*)&theState, 4);
	aBuffer.WriteBytes((const uchar*)&mTimedMode, 1);
	aBuffer.WriteBytes((const uchar*)&mIsNewGame, 1);
	aBuffer.WriteBytes((const uchar*)&mBarWidth, 4);
	aBuffer.WriteBytes((const uchar*)&thePlaySeconds, 4);
	aBuffer.WriteBytes((const uchar*)&mIdleTicks, 4);
	aBuffer.WriteBytes((const uchar*)&mScore, 4);
	aBuffer.WriteBytes((const uchar*)&mPointMultiplier, 8);
	aBuffer.WriteBytes((const uchar*)&mDisplayScore, 4);
	aBuffer.WriteBytes((const uchar*)&mGemsCleared, 4);
	aBuffer.WriteBytes((const uchar*)&mMoveMatchCount, 4);
	aBuffer.WriteBytes((const uchar*)&mCascadeCount, 4);
	aBuffer.WriteBytes((const uchar*)&mLongestCascade, 4);
	aBuffer.WriteBytes((const uchar*)&mBestMoveScore, 4);
	aBuffer.WriteBytes((const uchar*)&mStatsDisabled, 1);
	aBuffer.WriteBytes((const uchar*)&mUnused264, 4);
	aBuffer.WriteBytes((const uchar*)&mLevelStartScore, 4);
	aBuffer.WriteBytes((const uchar*)&mLevelTargetScore, 4);
	aBuffer.WriteBytes((const uchar*)&mLevelPoints, 4);
	aBuffer.WriteBytes((const uchar*)&mTimeDrain, 8);
	aBuffer.WriteBytes((const uchar*)&mTimeDrainRate, 8);
	aBuffer.WriteBytes((const uchar*)&mTimeDrainAccel, 8);
	aBuffer.WriteBytes((const uchar*)&mBarColor, 4);
	aBuffer.WriteBytes((const uchar*)&mWipeY, 4);
	aBuffer.WriteBytes((const uchar*)&mBlackHoleFrame, 4);
	aBuffer.WriteBytes((const uchar*)&mBlackHoleLevel, 8);
	aBuffer.WriteBytes((const uchar*)&mLevelTextShown, 1);
}

// 0x4cd130
bool Board::DeleteSaveGame()
{
	DeleteSaveSidecar(); // port(variant): the sidecar goes with the save (BoardVariant.cpp)
	return mApp->EraseFile(GetSaveGameFileName());
}

// 0x4cd1ba
Board::~Board()
{
	if (IsGameInProgress())
		SaveGame();

	delete mBackgroundImage;
	delete mNextBackgroundImage;
	delete mTextImage;
	delete mTextColorImage;
	if (mWhirlpoolSound != NULL)
		mWhirlpoolSound->Release();
	mApp->KillDialog(DIALOG_INSTRUCTIONS);
	// An abandoned game counts as played
	if (IsGameInProgress())
		mApp->mTimesPlayed++;
	if (mRecorder != NULL) mRecorder->OnBoardDestroyed(); // port(variant): the game's replay ends (finished, or kept on in the sidecar)

	// port: the original leaks all pieces
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
			delete mBoard[aRow][aCol];
	}
	for (size_t i = 0; i < mOrphanedPieces.size(); i++)
		delete mOrphanedPieces[i];
	delete mOwnRng; // port(variant): the board's own RNG
	delete mRecorder; // port(variant)
}

// 0x4cd287
bool Board::ProcessMatches()
{
	int aNumMatches = 0;
	int anAvgXSum = 0;
	int aMaxLength = 0;

	// Pass 0 scans each column top to bottom (vertical runs), pass 1 each row left to right. A gem in
	// a vertical and a horizontal run scores in both.
	for (int aPass = 0; aPass < 2; aPass++)
	{
		int aLines = (aPass == 0) ? mCols : mRows; // port(variant): was 8
		int aLength = (aPass == 0) ? mRows : mCols; // port(variant): was 8
		for (int i = 0; i < aLines; i++) // port(variant): was 8
		{
			int aColor = -1;
			int aCount = 0;
			int aStartCol = 0;
			int aStartRow = 0;
			for (int j = 0; j < aLength; j++) // port(variant): was 8
			{
				int aCol = (aPass == 0) ? i : j;
				int aRow = (aPass == 0) ? j : i;
				Piece* aPiece = mBoard[aRow][aCol];
				if ((aPiece != NULL) && (aPiece->mColor == aColor))
				{
					aCount++;
					if (j != aLength - 1) // port(variant): was 7
						continue;
				}

				if (aCount >= 3)
				{
					aNumMatches++;
					int aSumX = 0;
					int aSumY = 0;
					for (int aRunRow = aStartRow; aRunRow <= aRow; aRunRow++)
					{
						for (int aRunCol = aStartCol; aRunCol <= aCol; aRunCol++)
						{
							Piece* aRunPiece = mBoard[aRunRow][aRunCol];
							if ((aRunPiece != NULL) && (aRunPiece->mColor == aColor))
							{
								aSumX = (int)(aSumX + aRunPiece->mX);
								aSumY = (int)(aSumY + aRunPiece->mY);
								aRunPiece->mDyingFrame = 0;
								aRunPiece->mDying = true;
							}
						}
					}
					int anAvgX = aSumX / aCount;
					int anAvgY = aSumY / aCount;
					anAvgXSum += anAvgX;

					// The base value grows with each run of the move, cascades included
					mMoveMatchCount++;
					int aBasePoints = gGameConfig.SetValue(mMoveMatchCount);
					if (aCount > 3)
						aBasePoints += (aCount - 3) * gGameConfig.mLengthBonus;
					if (aCount > 5)
						aBasePoints += (aCount - 5) * gGameConfig.mLongLengthBonus;
					int aPoints = (int)(aBasePoints * mPointMultiplier);

					ScorePopup aPopup = {anAvgX + 24, anAvgY + 16, 110, -1, aPoints};
					mScorePopups.push_back(aPopup);
					mGemsCleared += aCount;
					mScore += aPoints;
					if (mMoveMatchCount == 1)
					{
						mHintGlowAlpha = 0.0;
						mHintGlowTimer = 0;
						mMoveScore = 0;
						mUnused218 = 0;
					}

					int anExcellentScore = (int)(mPointMultiplier * gGameConfig.mExcellentFactor);
					if ((mMoveScore < anExcellentScore) && (mMoveScore + aPoints >= anExcellentScore))
					{
						ShowText(mApp->GetString("EXCELLENT"), 200);
						mApp->PlaySample(SOUND_EXCELLENT);
					}
					mMoveScore += aPoints;
					if (mMoveScoreTimer != -1)
					{
						// The "+N" text stays 3 ticks per unmultiplied point, 3 to 12 seconds
						double aTicks = mMoveScore / mPointMultiplier * 3.0;
						if (aTicks < 300.0)
							aTicks = 300.0;
						else if (aTicks >= 1200.0)
							aTicks = 1200.0;
						mMoveScoreTimer = (int)aTicks;
					}

					if (aCount > aMaxLength)
						aMaxLength = aCount;
				}

				aColor = (aPiece != NULL) ? aPiece->mColor : -1;
				aCount = 1;
				aStartCol = aCol;
				aStartRow = aRow;
			}
		}
	}

	if (aNumMatches > 0)
	{
		int aPan = GetPanForX(anAvgXSum / aNumMatches + 24);
		mIdleTicks = 0;
		mApp->PlaySample((aMaxLength == 3) ? SOUND_GOTSET : SOUND_GOTSETBIG, aPan);
		int aCombo = mCascadeCount - 1;
		if (aCombo >= 5)
			aCombo = 4;
		mApp->PlaySample(SOUND_COMBO2 + aCombo, aPan);
		if (aNumMatches > 1)
			mApp->PlaySample(SOUND_MULTISHOT, aPan);
	}

	return aNumMatches > 0;
}

// 0x4cd687
void Board::UpdateFalling()
{
	int aLandedCount = 0;
	int aLandedXSum = 0;
	bool aSettled = true;
	// The board is always full in this state
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		double aRestY = GetRowY(aRow);
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece->mY == aRestY)
				aPiece->mVelY = 0.0;
			else
			{
				aSettled = false;
				aPiece->mY += aPiece->mVelY;
				if (aPiece->mY >= aRestY)
				{
					aLandedCount++;
					aLandedXSum = (int)(aPiece->mX + 24.0 + aLandedXSum);
					aPiece->mY = aRestY;
				}
				// A gem that just landed keeps this velocity for one tick
				aPiece->mVelY += 0.18;
			}
		}
	}

	if ((aLandedCount > 0) && (std::abs(mLastLandSoundTick - mUpdateCnt) >= 10))
	{
		mLastLandSoundTick = mUpdateCnt;
		mApp->PlaySample(SOUND_GEMONGEM, GetPanForX(aLandedXSum / aLandedCount));
	}

	if (aSettled)
	{
		if (ProcessMatches())
			mState = STATE_CLEARING;
		else
		{
			if (!mStatsDisabled)
			{
				if (mCascadeCount > mLongestCascade)
					mLongestCascade = mCascadeCount;
				if (mMoveScore > mBestMoveScore)
					mBestMoveScore = mMoveScore;
			}

			if (FindMove(NULL, 0, true, true))
				mState = STATE_PLAYING;
			else if (mTimedMode)
				mState = STATE_FALL_OUT;
			else
			{
				ShowText(mApp->GetString("NO_MOVES"), 400);
				GameOver(200);
			}
		}
	}

	MarkDirty();
}

// 0x4cd84f
void Board::UpdateSwap()
{
	MarkDirty();
	mSwapAngle += 5;
	if (mSwapAngle == 180)
	{
		Piece* aPiece1 = mSwapPiece1;
		Piece* aPiece2 = mSwapPiece2;
		int aCol = aPiece1->mCol;
		int aRow = aPiece1->mRow;
		aPiece1->mCol = aPiece2->mCol;
		aPiece1->mRow = aPiece2->mRow;
		aPiece1->mX = GetColX(aPiece1->mCol);
		aPiece1->mY = GetRowY(aPiece1->mRow);
		mBoard[aPiece1->mRow][aPiece1->mCol] = aPiece1;
		aPiece2->mCol = aCol;
		aPiece2->mRow = aRow;
		aPiece2->mX = GetColX(aPiece2->mCol);
		aPiece2->mY = GetRowY(aPiece2->mRow);
		Piece* aDisplacedPiece = mBoard[aRow][aCol];
		if (aDisplacedPiece != aPiece1)
		{
			// port: aPiece1 is a stale selection that FillBoard dropped; the original puts it back into
			// the grid and leaks the piece it overwrites. Keep each piece owned exactly once.
			mOrphanedPieces.erase(std::remove(mOrphanedPieces.begin(), mOrphanedPieces.end(), aPiece1), mOrphanedPieces.end());
			if (aDisplacedPiece != NULL)
				mOrphanedPieces.push_back(aDisplacedPiece);
		}
		mBoard[aRow][aCol] = aPiece2;

		mCascadeCount = 1;
		mMoveMatchCount = 0;
		if (ProcessMatches())
		{
			mSwapPiece1 = NULL;
			mSwapPiece2 = NULL;
			mState = STATE_CLEARING;
			if (mShowTutorial)
			{
				mShowTutorial = false;
				mApp->KillDialog(DIALOG_INSTRUCTIONS);
				mApp->mShowInstructions = false;
			}
			mIsNewGame = false;
			UsePendingSeed(); // port(variant): the game has started: it uses up a seed the player chose (BoardVariant.cpp)
		}
		else if (mState == STATE_SWAPPING)
		{
			// Swap back along the same circle. The second gem mirrors the first one's offsets, which
			// is off by 1 px when the centre was rounded.
			aPiece1->mSwapOffX = -aPiece1->mSwapOffX;
			aPiece1->mSwapOffY = -aPiece1->mSwapOffY;
			aPiece2->mSwapOffX = -aPiece1->mSwapOffX;
			aPiece2->mSwapOffY = -aPiece1->mSwapOffY;
			mSwapAngle = 0;
			mApp->PlaySample(SOUND_BAD);
			mState = STATE_SWAPPING_BACK;
		}
		else
		{
			mSwapPiece1 = NULL;
			mSwapPiece2 = NULL;
			mState = STATE_PLAYING;
		}
	}
	else
	{
		// Both gems slide through the centre along the swap axis: offset * cos(angle) goes from
		// +offset to -offset
		double aCos = cos(mSwapAngle * 3.14159 / 180.0);
		mSwapPiece1->mX = (int)(mSwapPiece1->mSwapOffX * aCos + mSwapCenterX);
		mSwapPiece1->mY = (int)(mSwapPiece1->mSwapOffY * aCos + mSwapCenterY);
		mSwapPiece2->mX = (int)(mSwapPiece2->mSwapOffX * aCos + mSwapCenterX);
		mSwapPiece2->mY = (int)(mSwapPiece2->mSwapOffY * aCos + mSwapCenterY);
	}
}

// 0x4cdabc
void Board::Update()
{
	if (mRecorder != NULL) mRecorder->SyncExternalRand(); // port(variant): draws from the global stream since the last update (XRAND)
	if (mPauseCount > 0)
		return;
	if (mUserPauseCount > 0)
		return;

	if (mInputDelay > 0)
		mInputDelay--;

	Widget::Update();

	if (mShowingText)
	{
		mTextPhase += 0.07;
		MarkDirty();
		if (--mTextTimeLeft == 0)
			mShowingText = false;
	}

	if (mTimeDrainDelay > 0)
		mTimeDrainDelay--;

	if ((mTimedMode) && (mTimeDrainDelay == 0) && ((mState == STATE_PLAYING) || (mState == STATE_SWAPPING)))
	{
		mTimeDrain += mTimeDrainRate / 100.0;
		mTimeDrainRate += mTimeDrainAccel / 100.0;
	}

	if (mHintGlowTimer > 0)
	{
		// Three triangle pulses over the glow's duration
		mHintGlowTimer--;
		int aPeriod = mHintGlowDuration / 3;
		int aDistance = std::abs(mHintGlowTimer % aPeriod - aPeriod / 2);
		mHintGlowAlpha = 1.0 - (double)aDistance / ((double)aPeriod * 0.5);
		if (mHintGlowTimer == 0)
			mHintGlowAlpha = 0.0;
		MarkDirty();
	}

	if ((mBlackHoleLevel > 0.0) && (mUpdateCnt % 2 == 0))
	{
		if (--mBlackHoleFrame < 0)
			mBlackHoleFrame = 8;
		MarkDirty();
	}

	if ((mShowTutorial) && (mUpdateCnt % 4 == 0))
	{
		mTutorialBob += mTutorialBobDir;
		if (std::abs(mTutorialBob) >= 3)
			mTutorialBobDir = -mTutorialBobDir;
		MarkDirty();
	}

	if (mMoveScoreTimer > 0)
	{
		mMoveScoreTimer--;
		if (mMoveScoreTimer < 50)
		{
			mMoveScoreAlpha = mMoveScoreTimer / 50.0;
			MarkDirty();
		}
		else if (mMoveScoreAlpha < 1.0)
		{
			mMoveScoreAlpha += 0.05;
			if (mMoveScoreAlpha > 1.0)
				mMoveScoreAlpha = 1.0;
			MarkDirty();
		}
	}

	// The bar shows the level progress; in timed mode it starts half full and the drain empties it
	int aProgress = (mDisplayScore - (int)mTimeDrain - mLevelStartScore) * BAR_WIDTH;
	int aRange = mLevelTargetScore - mLevelStartScore;
	int aTargetWidth;
	if (mTimedMode)
		aTargetWidth = aProgress / 2 / aRange + BAR_WIDTH / 2;
	else
		aTargetWidth = aProgress / aRange;
	if (aTargetWidth > BAR_WIDTH)
		aTargetWidth = BAR_WIDTH;
	if (aTargetWidth < 0)
		aTargetWidth = 0;

	bool aBarGrew = false;
	for (int i = 0; i < 2; i++)
	{
		if (aTargetWidth > mBarWidth)
		{
			aBarGrew = true;
			mBarWidth++;
			MarkDirty();
		}
		else if (aTargetWidth < mBarWidth)
		{
			mBarWidth--;
			MarkDirty();
		}
	}

	int aBarColor;
	if (!mTimedMode)
		aBarColor = BAR_COLOR_NORMAL;
	else
	{
		double aFraction = mBarWidth / 340.0;
		aBarColor = (mBarWidth * NUM_BAR_COLORS - 1) / BAR_WIDTH;
		if ((!aBarGrew) && (mState != STATE_GAME_OVER) && (mState != STATE_GAME_OVER_DONE))
		{
			// Below the warning fraction (240 / 1600 = 0.15) warning1 repeats every 80..240 ticks,
			// faster as the bar empties, and the bar flashes white
			int aWarningInterval = std::max(80, (int)(aFraction * 1600.0));
			if ((aWarningInterval <= (int)(gGameConfig.mWarningFraction * 1600.0)) &&
				((unsigned int)(mUpdateCnt - mLastWarningTick) >= (unsigned int)aWarningInterval))
			{
				mApp->PlaySample(SOUND_WARNING);
				mLastWarningTick = mUpdateCnt;
			}
			if ((aFraction < gGameConfig.mWarningFraction) && ((mUpdateCnt / 8) % 2 == 0))
				aBarColor = BAR_COLOR_FLASH;
		}
	}
	if ((mState == STATE_BLACK_HOLE) && ((mUpdateCnt / 8) % 2 == 0))
		aBarColor = BAR_COLOR_FLASH;
	if (aBarColor != mBarColor)
	{
		mBarColor = aBarColor;
		MarkDirty();
	}

	// Tick 149: register nag for unregistered copies (not ported)

	if (mUpdateCnt == 150)
	{
		mApp->PlaySample(SOUND_GO);
		if ((!mApp->mShowInstructions) || (mApp->mTimesPlayed >= 5))
			ShowText(mApp->GetString("GO"), 250);
		if ((mApp->mShowInstructions) && (mApp->mTimesPlayed < 5))
		{
			mApp->ShowInstructionsDialog();
			mShowTutorial = true;
		}
	}

	if ((++mIdleTicks > 1500) && (mState == STATE_PLAYING))
	{
		mIdleTicks = 0;
		mHintGlowDuration = 400;
		mHintGlowTimer = 400;
	}

	// The displayed score rolls toward the real one, faster the further behind it is (3, 2 or 1
	// times the multiplier per tick; the last step only every other tick when close). The second
	// pass rolls it down after a hint penalty.
	int aScoreDiff = mScore - mDisplayScore;
	bool aRolled = true;
	if (aScoreDiff >= (int)(mPointMultiplier * 150.0))
		mDisplayScore -= (int)(mPointMultiplier * -3.0);
	else if (aScoreDiff >= (int)(mPointMultiplier * 100.0))
		mDisplayScore -= (int)(mPointMultiplier * -2.0);
	else if ((aScoreDiff >= (int)(mPointMultiplier * 50.0)) || ((mDisplayScore < mScore) && (mUpdateCnt % 2 == 0)))
	{
		mDisplayScore += (int)mPointMultiplier;
		if (mDisplayScore > mScore)
			mDisplayScore = mScore;
	}
	else
		aRolled = false;
	if (aRolled)
		MarkDirty();

	aScoreDiff = mScore - mDisplayScore;
	aRolled = true;
	if (aScoreDiff <= (int)(mPointMultiplier * -150.0))
		mDisplayScore -= (int)(mPointMultiplier * 3.0);
	else if (aScoreDiff <= (int)(mPointMultiplier * -100.0))
		mDisplayScore -= (int)(mPointMultiplier + mPointMultiplier);
	else if ((aScoreDiff <= (int)(mPointMultiplier * -50.0)) || ((mDisplayScore > mScore) && (mUpdateCnt % 2 == 0)))
	{
		mDisplayScore -= (int)mPointMultiplier;
		if (mDisplayScore < mScore)
			mDisplayScore = mScore;
	}
	else
		aRolled = false;
	if (aRolled)
		MarkDirty();

	if ((mSelectedPiece != NULL) && (mUpdateCnt % 8 == 0))
	{
		MarkDirty();
		mCursorFrame++;
	}

	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;

			if ((mUpdateCnt % 5 == 0) && (aPiece->mSparkleCount > 0))
			{
				aPiece->mSparkleFrame++;
				aPiece->mSparkleCount--;
				MarkDirty();
			}

			// Every third tick a spinning or selected gem advances its spin and skips the hover glow
			if ((mUpdateCnt % 3 == 0) && ((aPiece->mSpinFrame != 0) || (aPiece == mSelectedPiece)))
			{
				aPiece->mSpinFrame = (aPiece->mSpinFrame + 1) % 15;
				MarkDirty();
				continue;
			}

			// The glow always decays and is topped up while hovered, so it fades in at a net +0.033
			if (aPiece->mGlowing)
			{
				aPiece->mGlowAlpha -= 0.012;
				if (aPiece->mGlowAlpha < 0.0)
				{
					aPiece->mGlowing = false;
					MarkDirty();
				}
			}

			double aMouseX = mMouseX;
			double aMouseY = mMouseY;
			if ((mState == STATE_PLAYING) && (aPiece != mSelectedPiece) &&
				(aPiece->mX <= aMouseX) && (aPiece->mY <= aMouseY) &&
				(aMouseX < aPiece->mX + GEM_SIZE) && (aMouseY < aPiece->mY + GEM_SIZE))
			{
				aPiece->mGlowAlpha += 0.045;
				if (aPiece->mGlowAlpha > 1.0)
					aPiece->mGlowAlpha = 1.0;
				if (!aPiece->mGlowing)
				{
					aPiece->mGlowFrame = 0;
					aPiece->mGlowing = true;
				}
			}

			if ((mUpdateCnt % 3 == 0) && (aPiece->mGlowing))
			{
				aPiece->mGlowFrame = (aPiece->mGlowFrame + 1) % 15;
				MarkDirty();
			}
		}
	}

	std::list<ScorePopup>::iterator anItr = mScorePopups.begin();
	while (anItr != mScorePopups.end())
	{
		if (mUpdateCnt % 3 == 0)
		{
			MarkDirty();
			if (anItr->mY > mPointsFont->GetAscent() / 2 + 2)
				anItr->mY--;
		}
		MarkDirty();
		if (--anItr->mLife <= 0)
		{
			anItr = mScorePopups.erase(anItr);
			MarkDirty();
		}
		else
			++anItr;
	}

	if (mState == STATE_PLAYING)
	{
		if (mDisplayScore - (int)mTimeDrain - mLevelStartScore >= mLevelTargetScore - mLevelStartScore)
			StartBlackHole();
		// Not limited to timed mode: in normal mode hint penalties can push the score a whole level
		// below the level start
		if ((mState == STATE_PLAYING) && (mDisplayScore - (int)mTimeDrain - mLevelStartScore <= mLevelStartScore - mLevelTargetScore))
		{
			ShowText(mApp->GetString("TIME_UP"), 400);
			GameOver(200);
		}
	}

	switch (mState)
	{
	case STATE_FALLING:
		UpdateFalling();
		break;
	case STATE_SWAPPING:
	case STATE_SWAPPING_BACK:
		UpdateSwap();
		break;
	case STATE_CLEARING:
		UpdateClearing();
		break;
	case STATE_GAME_OVER:
		UpdateGameOver();
		break;
	case STATE_BLACK_HOLE:
		UpdateBlackHole();
		break;
	case STATE_LEVEL_WIPE:
		UpdateLevelWipe();
		break;
	case STATE_FALL_OUT:
		UpdateFallOut();
		break;
	}

	if (mRecorder != NULL) mRecorder->AfterUpdate(); // port(variant): CHECK at a settle, END at the game-over screen
}
