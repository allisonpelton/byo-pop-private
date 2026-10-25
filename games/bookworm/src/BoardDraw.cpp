// BoardDraw.cpp: the drawing half of Board.cpp - Board::Draw and the helpers only it reaches (level text,
// progress bar, score, last word, tiles and their glows, selection arrows). The Board class is declared
// in Board.h (board_logic). Every coordinate is Board-local (the Board covers the 640x480 screen).
// Behaviour spec: re/bookworm/analysis/board_draw.md section 2; names: re/bookworm/analysis/NAMES.md.
//
// Graphics state: the helpers draw on g directly; a colour or font one of them sets stays set for the
// next (none reads state it did not set), colorize and the draw mode are always put back to off / normal.

#include "Board.h"
#include "Solver.h"
#include "BookWormApp.h"
#include "Res.h"
#include "Piece.h"
#include "WordScore.h"
#include "Lex.h"
#include "Effect.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "ModFeatures.h"
#include "ModWords.h"
#include "TileSymbols.h"
#include "Font.h"
#include "ImageFont.h"
#include <cmath>
#include <cstdlib>
#include <list>

using namespace Sexy;

// port(mod): long-words (ModWords.h): the widest word the box under the score takes, centred on x 112
static const int LONG_WORD_WIDTH = 200;

// 0x40a980
// Background, Lex's shadow, level text, progress bar, score, last word, Lex's burp smoke, the tiles, then
// the board's effects (sparkles) and particles. Lex is a separate widget drawn above the Board.
void Board::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[mBurnedDown ? IMAGE_BACKDROP_BOARD_SCORCHED : IMAGE_BACKDROP_BOARD], 0, 0);
	Widget::Draw(g);

	if (mLex->mShowShadow)
	{
		// The shadow under Lex follows his x (kept in 0..208 - width) and fades out as he moves away from
		// his target position: invisible at 100 px. The squared distance is summed as an int.
		Image* aShadowImage = mApp->mImages[IMAGE_LEX_SHADOW];
		int aMaxX = 0xd0 - aShadowImage->GetWidth();
		int aDY = mLex->mTargetY - (int)mLex->mCurY;
		int aDX = mLex->mTargetX - (int)mLex->mCurX;
		double aDist = sqrt((double)(aDY*aDY + aDX*aDX));
		if (aDist >= 100.0)
			aDist = 100.0;
		double aFade = 1.0 - aDist * 0.01;

		int aX = (int)(mLex->mCurX - 25.0);
		if (aX < 0)
			aX = 0;
		if (aX >= aMaxX)
			aX = aMaxX;

		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, (int)(aFade * 255.0)));
		g->DrawImage(aShadowImage, aX, 0x17c);
		g->SetColorizeImages(false);
	}

	DrawLevelText(g);
	DrawProgressBar(g);
	DrawScore(g);
	DrawWord(g);
	if (mHintButton != NULL)
		DrawPlankDivider(g, 136, 100);  // port(mod)
	if (mUndoButton != NULL)
		DrawPlankDivider(g, 136, 181);  // port(mod)

	if (mBurping)
	{
		// Lex's burp: a smoke puff that rises 48 px from his mouth while mBurpAlpha falls from 1 to 0
		// (Board::Update, 0.005 per tick).
		Image* aSmokeImage = mApp->mImages[IMAGE_FX_SMOKE_SM];
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mBurpAlpha * 255.0)));
		g->DrawImage(aSmokeImage, mLex->mX + 100,
			mLex->mY - (int)((1.0 - mBurpAlpha) * 48.0) - aSmokeImage->GetHeight() / 2 + 0x32);
		g->SetColorizeImages(false);
	}

	DrawPieces(g);
	mEffectMgr->Draw(g);
	mEmberMgr->Draw(g);
	if (mSolver != NULL)
		mSolver->Draw(g);   // port(mod): the hint's tag

	// port(mod): a watched replay says so above the tiles, with its speed and keys (Replay.h)
	if (mPlayer != NULL)
	{
		Font* aFont = mApp->mFonts[FONT_MYRIADWEB10];
		SexyString aText = StrFormat(_S("REPLAY %dX   1-4: speed, space: pause, Esc: stop"), mApp->mWatchSpeed);
		int aWidth = aFont->StringWidth(aText) + 16;
		int aX = 233 + (640 - 233 - aWidth) / 2;   // over the tiles (x 233..640)
		g->SetColor(Color(0, 0, 0, 160));
		g->FillRect(aX, 0, aWidth, 17);
		g->SetFont(aFont);
		g->SetColor(Color(255, 214, 110));
		g->DrawString(aText, aX + 8, 13);
	}
}

// port(mod): a groove across a left-panel plank (30 px high from theY), dark with a light edge, where a
// mod's button takes the plank's right part
void Board::DrawPlankDivider(Graphics* g, int theX, int theY)
{
	g->SetColor(Color(40, 20, 6, 210));
	g->FillRect(theX - 1, theY + 4, 2, 22);
	g->SetColor(Color(235, 190, 120, 110));
	g->FillRect(theX + 1, theY + 4, 1, 22);
}

// 0x4056e0
// The level number (GT_BOARD_LEVEL_FORMAT) in the top-left panel; it fades out while a word is shown
// (mLastWordAlpha).
void Board::DrawLevelText(Graphics* g)
{
	if (mBurnedDown)
		return;

	int anAlpha = 255 - (int)(mLastWordAlpha * 255.0);
	if (anAlpha < 0)
		anAlpha = 0;
	if (anAlpha > 254)
		anAlpha = 255;

	Font* aFont = mApp->mFonts[FONT_LOCKERGNOME24];
	g->SetFont(aFont);
	SexyString aText = StrFormat(GameTextC(GT_BOARD_LEVEL_FORMAT), mLevel + 1);
	int aWidth = aFont->StringWidth(aText);

	g->SetColor(Color(0, 0, 0, anAlpha));
	g->DrawString(aText, 0x71 - aWidth / 2, 0x4b);
	g->SetColor(Color(0x81, 0x5d, 0x34, anAlpha));
	g->DrawString(aText, 0x70 - aWidth / 2, 0x4a);
}

// 0x403c00
// The level progress bar, sliding in from the left under a clip at x 16. After a level-up the shown
// progress drains in the previous level's colour (one image per level % 12, progress_yellow downwards).
void Board::DrawProgressBar(Graphics* g)
{
	if (mShownProgress > 0.0)
	{
		Graphics aBarG(*g);

		int aLevel = mLevel;
		if (mLevelProgress < mShownProgress)
			aLevel--;

		// Quirk: level index -1 (draining at level 0) gives image 139, progress_burnt.
		Image* aBarImage = mApp->mImages[IMAGE_PROGRESS_YELLOW - aLevel % 12];
		if (mBurnedDown)
			aBarImage = mApp->mImages[IMAGE_PROGRESS_BURNT];

		int aWidth = aBarImage->GetWidth();
		int aX = (int)((double)aWidth * mShownProgress + 16.0) - aWidth;
		if (aX > 16)
			aX = 16;

		aBarG.ClipRect(16, 0x90, aWidth, aBarImage->GetHeight());
		aBarG.DrawImage(aBarImage, aX, 0x90);
	}
}

// 0x405580
// The score, centred at x 108.
void Board::DrawScore(Graphics* g)
{
	SexyString aText = StrFormat("%d", mShownScore);
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(mApp->mFonts[FONT_GOLDSCORE]);
	int aX = 0x6c - g->GetFont()->StringWidth(aText) / 2;
	g->DrawString(aText, aX, 0x1f);

	if (mScoreGlow > 0.0)
	{
		// The heated overlay. Quirk: the glow (at most 0.5 here) is truncated to an int before the
		// multiply, so the alpha is always 0 and the overlay never shows. Kept (it draws nothing).
		g->SetFont(mApp->mFonts[FONT_GOLDSCOREHEATED]);
		double aGlow = (mScoreGlow < 0.5) ? mScoreGlow : 0.5;
		g->SetColor(Color(255, 255, 255, (int)aGlow * 255));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawString(aText, aX, 0x1f);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x407c60
// The last word above the board and its points, fading with mLastWordAlpha. The word's colour, stored in
// mLastWordColor, says what tiles it used: short, all plain, or the best special type in it.
void Board::DrawWord(Graphics* g)
{
	if (mBurnedDown)
		return;
	if (mLastWordAlpha <= 0.0)
		return;

	WordScore aWordScore(mLastWord);
	SexyString aWord = mLastWord.mWord;

	if (aWordScore.mWord.size() != 0)
	{
		if (aWord.size() < 3)
			mLastWordColor = Color(0xcc, 0xad, 0x6e, 0xff);
		else if (aWordScore.mTypeCount[TILE_NORMAL] == aWordScore.mNumPieces)
			mLastWordColor = Color(0xff, 0xdd, 0x7b, 0xff);
		else if (aWordScore.mTypeCount[TILE_DIAMOND] > 0)
			mLastWordColor = Color(0xff, 0xff, 0xff, 0xff);
		else if (aWordScore.mTypeCount[TILE_SAPPHIRE] > 0)
			mLastWordColor = Color(0x2d, 0xc3, 0xff, 0xff);
		else if (aWordScore.mTypeCount[TILE_GOLD] > 0)
			mLastWordColor = Color(0xff, 0xdc, 0x00, 0xff);
		else if (aWordScore.mTypeCount[TILE_GREEN] > 0)
			mLastWordColor = Color(0x00, 0xff, 0x00, 0xff);
		else if (aWordScore.mTypeCount[TILE_FIRE] > 0)
			mLastWordColor = Color(0xff, 0x64, 0x00, 0xff);
		// else the colour is left as it was
	}

	if (aWord.size() != 0)
	{
		SexyString aPointsText = aWordScore.mValid ? StrFormat(GameTextC(GT_BOARD_POINTS_FORMAT), aWordScore.mScore) :
			GameText(GT_BOARD_ZERO_POINTS);

		// The word reaches full alpha at half the fade; the points' shadow uses the plain fade.
		int aFadeAlpha = (int)(mLastWordAlpha * 255.0);
		mLastWordColor.mAlpha = (aFadeAlpha < 0x80) ? aFadeAlpha * 2 : 255;

		// The font is chosen on the full length; at most 12 letters are shown, 11 when the 12th is the
		// 'Q' of a "QU".
		Font* aFont = mApp->mFonts[FONT_VERDANA18BOLD];
		if (aWord.size() > 10)
			aFont = mApp->mFonts[FONT_VERDANA14BOLD];
		if ((aWord.size() > 12) && (ModLongWords()))
		{
			// port(mod): long-words (ModWords.h): the whole word, in the largest of the Verdanas that fits
			const int aFonts[] = { FONT_VERDANA14BOLD, FONT_VERDANA12BOLD, FONT_VERDANA10BOLD };
			for (int i = 0; i < 3; i++)
			{
				aFont = mApp->mFonts[aFonts[i]];
				if (aFont->StringWidth(aWord) <= LONG_WORD_WIDTH)
					break;
			}
		}
		else if (aWord.size() > 12)
		{
			aWord.resize(12);
			if (aWord[11] == 'Q')
				aWord.resize(11);
		}

		int aWidth = aFont->StringWidth(aWord);
		g->SetFont(aFont);
		g->SetColor(Color(0, 0, 0, mLastWordColor.mAlpha));
		g->DrawString(aWord, 0x71 - aWidth / 2, 0x41);
		g->SetColor(mLastWordColor);
		g->DrawString(aWord, 0x70 - aWidth / 2, 0x40);

		// The invalid word's text (GT_BOARD_ZERO_POINTS) is made but never drawn
		if (aWordScore.mValid)
		{
			aFont = mApp->mFonts[FONT_VERDANA14BOLD];
			int aPointsWidth = aFont->StringWidth(aPointsText);
			g->SetFont(aFont);
			g->SetColor(Color(0, 0, 0, aFadeAlpha));
			g->DrawString(aPointsText, 0x71 - aPointsWidth / 2, 0x55);
			g->SetColor(mLastWordColor);
			g->DrawString(aPointsText, 0x70 - aPointsWidth / 2, 0x54);
		}
	}
}

// 0x40a160
// The tiles bottom row first, then three glow passes over lists collected on the way (green, gold, fire
// with its flame animation), then the selection arrows. A piece can be in the fire list twice (a fire
// tile and, as the tile below another fire tile, its neighbour): it then glows twice.
void Board::DrawPieces(Graphics* g)
{
	PieceList aFireList;            // A: fire tiles, the tiles under them about to burn, pending fire tiles
	PieceList aGreenList;           // B
	PieceList aGoldList;            // C: gold, sapphire and diamond
	// (the original also builds a fourth list that is never filled)
	PieceList* anExtraList = NULL;

	// Action game paused: the tiles are drawn without letters.
	bool isBlank = true;
	if ((mApp->mGameMode != GAMEMODE_ACTION) || (mPauseFade <= 0.0) || mBurnedDown)
		isBlank = false;

	// A selection with a green tile makes the other selected tiles glow green; one with a gold or gem
	// tile makes them glow gold.
	for (PieceList::iterator anItr = mSelection.begin(); anItr != mSelection.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece != NULL)
		{
			int aType = aPiece->mType;
			if ((aType == TILE_GREEN) && (anExtraList == NULL))
				anExtraList = &aGreenList;
			if (((aType == TILE_GOLD) || (aType == TILE_SAPPHIRE) || (aType == TILE_DIAMOND)) &&
				((anExtraList == NULL) || (anExtraList == &aGreenList)))
				anExtraList = &aGoldList;
		}
	}

	for (int aRow = BOARD_ROWS - 1; aRow >= 0; aRow--)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;

			if (isBlank)
				DrawPieceBlank(g, aPiece);
			else
				DrawPiece(g, aPiece);

			if (mBurnedDown || aPiece->mBurning)
				continue;

			int aType = aPiece->mType;
			if (aType == TILE_FIRE)
			{
				aFireList.push_back(aPiece);
				Piece* aBelow = PieceBelow(aPiece);
				if ((aBelow != NULL) && (aBelow->mBurnCountdown >= 1))
					aFireList.push_back(aBelow);
			}
			else if (aPiece->mPendingFire)
				aFireList.push_back(aPiece);
			else if (aType == TILE_GREEN)
				aGreenList.push_back(aPiece);
			else if ((aType == TILE_GOLD) || (aType == TILE_SAPPHIRE) || (aType == TILE_DIAMOND))
				aGoldList.push_back(aPiece);
			else if ((anExtraList != NULL) && aPiece->mSelected)
				anExtraList->push_back(aPiece);
		}
	}

	// Green glow, pulsing 48..176 with a period of 128 updates
	for (PieceList::iterator anItr = aGreenList.begin(); anItr != aGreenList.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece == NULL)
			continue;

		int aLevel = 0xb0 - abs((mUpdateCnt * 2) % 256 - 0x80);
		g->SetColor(Color(aLevel, aLevel, aLevel));
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		Image* aGlowImage = mApp->mImages[IMAGE_TILE_GREEN_GLOW];
		int anOffset = (aGlowImage->GetWidth() - mApp->mImages[IMAGE_TILE_GREEN]->GetWidth()) / 2;
		g->DrawImage(aGlowImage, (int)aPiece->mX - anOffset, (int)aPiece->mY - anOffset);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	// Gold glow, 64..128, period 128
	for (PieceList::iterator anItr = aGoldList.begin(); anItr != aGoldList.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece == NULL)
			continue;

		int aLevel = 0x80 - abs(mUpdateCnt % 128 - 0x40);
		g->SetColor(Color(aLevel, aLevel, aLevel));
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		Image* aGlowImage = mApp->mImages[IMAGE_TILE_GOLD_GLOW];
		int anOffset = (aGlowImage->GetWidth() - mApp->mImages[IMAGE_TILE_GOLD]->GetWidth()) / 2;
		g->DrawImage(aGlowImage, (int)aPiece->mX - anOffset, (int)aPiece->mY - anOffset);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	// Fire glow, 0..255 (also the alpha), period 127.5 updates; fire tiles also get two flame strips,
	// tile_fire_fx1 drawn a second time with its halves swapped.
	for (PieceList::iterator anItr = aFireList.begin(); anItr != aFireList.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece == NULL)
			continue;

		int aLevel = 0xff - abs((mUpdateCnt * 4) % 0x1fe - 0xff);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		Image* aGlowImage = mApp->mImages[IMAGE_TILE_FIRE_GLOW];
		int anOffset = (aGlowImage->GetWidth() - mApp->mImages[IMAGE_TILE_FIRE]->GetWidth()) / 2;
		g->SetColorizeImages(true);
		g->SetColor(Color(aLevel, aLevel, aLevel, aLevel));
		g->DrawImage(aGlowImage, (int)aPiece->mX - anOffset, (int)aPiece->mY - anOffset);
		g->SetColorizeImages(false);

		if (aPiece->mType == TILE_FIRE)
		{
			Graphics aFlameG(*g);
			aFlameG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			Image* aFx1Image = mApp->mImages[IMAGE_TILE_FIRE_FX1];
			Image* aFx2Image = mApp->mImages[IMAGE_TILE_FIRE_FX2];
			int aNumCels = aFx1Image->GetHeight() / 57;     // also used for fx2
			int aCel1Y = ((mUpdateCnt / 6) % aNumCels) * 57;
			int aCel2Y = ((mUpdateCnt / 8) % aNumCels) * 57;
			int aX = (int)aPiece->mX;
			int aY = (int)aPiece->mY;
			aFlameG.ClipRect(aX, aY, 57, 57);
			aFlameG.DrawImage(aFx1Image, aX, aY, Rect(0, aCel1Y, 57, 57));
			aFlameG.DrawImage(aFx2Image, aX, aY, Rect(0, aCel2Y, 57, 57));
			aFlameG.DrawImage(aFx1Image, aX + 28, aY, Rect(0, aCel1Y, 28, 57));
			aFlameG.DrawImage(aFx1Image, aX, aY, Rect(28, aCel1Y, 28, 57));
			aFlameG.SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	if (!mBurnedDown && (mSelection.size() != 0))
		DrawSelection(g);
}

// 0x4044f0
// One tile from the pre-drawn letter strips (app tile sets), or, for sapphire and diamond, from the gem
// image, the letter glyph, the value digit and an additive highlight. A tile is lit when selected or when
// its letter is the held key (mHighlightKey). Letter flips (a tile's own, or the whole board in
// BOARD_SCRAMBLING) run as a diagonal wave: frames 1..7 show tile_spin_fx, then the new letter.
// port(mod): colour-blind mode's symbol over a special tile (not on the charred board)
static void DrawModSymbol(Board* theBoard, Graphics* g, Piece* thePiece)
{
	if ((theBoard->mTileSymbols) && (!theBoard->mBurnedDown))
		DrawTileSymbol(g, theBoard->mApp, (int)thePiece->mX, (int)thePiece->mY, thePiece->mType,
			ModFeatureBool("tileSymbols", "large", false));
}

void Board::DrawPiece(Graphics* g, Piece* thePiece)
{
	int aLetter = thePiece->mLetter;
	MemoryImage* aTileSet = mApp->mTileSetImages[TILESET_NORMAL];

	if (mBurnedDown)
	{
		aTileSet = mApp->mTileSetImages[TILESET_CHARRED];
	}
	else
	{
		bool isHeldKey = (aLetter == mHighlightKey);

		// Flip frame: t * 4.75 - (col + row) / 4, times 8 (t rises 0.01 per tick). A flipping fire tile
		// shows the normal set until frame 8; the board-wide scramble skips fire tiles.
		bool isFlipFrame = false;
		int aFrame = 0;
		if (thePiece->mFlipping)
		{
			aFrame = (int)((thePiece->mFlipProgress * 4.75 - (double)(thePiece->mCol + thePiece->mRow) * 0.25) * 8.0);
			if ((aFrame >= 8) && (thePiece->mType == TILE_FIRE))
				aTileSet = mApp->mTileSetImages[TILESET_FIRE];
			isFlipFrame = true;
		}
		else if ((mState == BOARD_SCRAMBLING) && (thePiece->mType != TILE_FIRE))
		{
			aFrame = (int)((mScrambleProgress * 4.75 - (double)(thePiece->mCol + thePiece->mRow) * 0.25) * 8.0);
			isFlipFrame = true;
		}

		if (isFlipFrame)
		{
			aLetter = (aFrame > 0) ? thePiece->mNextLetter : thePiece->mLetter;
			if ((aFrame > 0) && (aFrame < 8))
			{
				g->DrawImage(mApp->mImages[IMAGE_TILE_SPIN_FX], (int)thePiece->mX, (int)thePiece->mY,
					Rect(0, aFrame * 57 - 57, 57, 57));
				return;
			}
		}

		// A tile's own flip draws with the set chosen above (no highlight, no gem parts).
		if (!thePiece->mFlipping)
		{
			bool isLit = thePiece->mSelected || isHeldKey;
			switch (thePiece->mType)
			{
			case TILE_NORMAL:
				aTileSet = mApp->mTileSetImages[isLit ? TILESET_NORMAL_HILITE : TILESET_NORMAL];
				break;
			case TILE_GREEN:
				aTileSet = mApp->mTileSetImages[isLit ? TILESET_GREEN_HILITE : TILESET_GREEN];
				break;
			case TILE_GOLD:
				aTileSet = mApp->mTileSetImages[isLit ? TILESET_GOLD_HILITE : TILESET_GOLD];
				break;
			case TILE_FIRE:
				aTileSet = mApp->mTileSetImages[isLit ? TILESET_FIRE_HILITE : TILESET_FIRE];
				break;
			case TILE_SAPPHIRE:
			case TILE_DIAMOND:
			{
				Image* aGemImage;
				if (thePiece->mType == TILE_SAPPHIRE)
					aGemImage = mApp->mImages[isLit ? IMAGE_TILE_SAPPHIRE_HILITE : IMAGE_TILE_SAPPHIRE];
				else
					aGemImage = mApp->mImages[isLit ? IMAGE_TILE_DIAMOND_HILITE : IMAGE_TILE_DIAMOND];

				// The original sets this colour with colorize off: it has no effect.
				g->SetColor(Color(255, 255, 255, 0xb0));
				g->DrawImage(aGemImage, (int)thePiece->mX, (int)thePiece->mY);

				// The letter glyph (59x59, drawn unclipped at the tile's corner) in black.
				Image* aLettersImage = mApp->mImages[IMAGE_FONT_TILE_LETTERS];
				int aLetterHeight = aLettersImage->GetHeight() / 26;
				g->SetColor(Color(0, 0, 0, 255));
				g->SetColorizeImages(true);
				g->DrawImage(aLettersImage, (int)thePiece->mX, (int)thePiece->mY,
					Rect(0, (aLetter - 'A') * aLetterHeight, aLettersImage->GetWidth(), aLetterHeight));
				g->SetColorizeImages(false);

				// The letter's value in the bottom-right corner ('Q' is 10).
				Image* aDigitsImage = mApp->mImages[IMAGE_FONT_TILE_DIGITS];
				int aValue = (aLetter == 'Q') ? 10 : gLetterValues[aLetter - 'A'];
				int aDigitHeight = aDigitsImage->GetHeight() / 10;
				g->SetColor(Color(255, 255, 255, 255));
				g->SetColorizeImages(true);
				g->DrawImage(aDigitsImage, (int)thePiece->mX - aDigitsImage->GetWidth() + 0x36,
					(int)thePiece->mY - aDigitHeight + 0x35,
					Rect(0, (aValue - 1) * aDigitHeight, aDigitsImage->GetWidth(), aDigitHeight));
				g->SetColorizeImages(false);

				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->DrawImage(mApp->mImages[IMAGE_TILE_GEMHILITE], (int)thePiece->mX, (int)thePiece->mY);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				DrawModSymbol(this, g, thePiece);
				return;
			}
			default:
				// other types: the plain set chosen at the top
				break;
			}
		}
	}

	g->DrawImage(aTileSet, (int)thePiece->mX, (int)thePiece->mY, Rect((aLetter - 'A') * 57, 0, 57, 57));
	DrawModSymbol(this, g, thePiece);
}

// 0x404380
// A paused action game hides the letters: the plain tile image of the piece's type (lit or not), plus the
// gem highlight on sapphire and diamond tiles.
void Board::DrawPieceBlank(Graphics* g, Piece* thePiece)
{
	bool isLit = thePiece->mSelected || (thePiece->mLetter == mHighlightKey);

	int anImageId;
	switch (thePiece->mType)
	{
	case TILE_NORMAL:
		anImageId = isLit ? IMAGE_TILE_HILITE : IMAGE_TILE;
		break;
	case TILE_GREEN:
		anImageId = isLit ? IMAGE_TILE_GREEN_HILITE : IMAGE_TILE_GREEN;
		break;
	case TILE_GOLD:
		anImageId = isLit ? IMAGE_TILE_GOLD_HILITE : IMAGE_TILE_GOLD;
		break;
	case TILE_FIRE:
		anImageId = isLit ? IMAGE_TILE_FIRE_HILITE : IMAGE_TILE_FIRE;
		break;
	case TILE_SAPPHIRE:
		anImageId = isLit ? IMAGE_TILE_SAPPHIRE_HILITE : IMAGE_TILE_SAPPHIRE;
		break;
	case TILE_DIAMOND:
		anImageId = isLit ? IMAGE_TILE_DIAMOND_HILITE : IMAGE_TILE_DIAMOND;
		break;
	default:
		anImageId = IMAGE_TILE;
		break;
	}

	g->DrawImage(mApp->mImages[anImageId], (int)thePiece->mX, (int)thePiece->mY, Rect(0, 0, 57, 57));
	if ((thePiece->mType == TILE_SAPPHIRE) || (thePiece->mType == TILE_DIAMOND))
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[IMAGE_TILE_GEMHILITE], (int)thePiece->mX, (int)thePiece->mY);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x405340
// Along the selection: an arrow from each tile towards the next one (the arrows image is a 3x2 sheet laid
// around the tile centre, the cel in the neighbour's direction is shown; odd columns sit half a tile
// lower, so a sideways step points down from an even column and up from an odd one), and an "X" on the
// letters past the word length limit. Letters are counted with "QU" as 2.
void Board::DrawSelection(Graphics* g)
{
	int aLetterCount = 0;
	int aLimit = ModWordMaxLength();    // port(mod): 12, or the longest word with long-words (ModWords.h)

	for (PieceList::iterator anItr = mSelection.begin(); anItr != mSelection.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece == NULL)
			continue;

		aLetterCount += (aPiece->mLetter == 'Q') ? 2 : 1;

		PieceList::iterator aNextItr = anItr;
		++aNextItr;
		if ((aNextItr != mSelection.end()) && (aLetterCount < aLimit) && (*aNextItr != NULL) &&
			(((*aNextItr)->mLetter != 'Q') || (aLetterCount < aLimit - 1)))
		{
			Piece* aNextPiece = *aNextItr;
			Image* anArrowsImage = mApp->mImages[IMAGE_ARROWS];
			int aDX = aNextPiece->mCol - aPiece->mCol;
			int aDY = aNextPiece->mRow - aPiece->mRow;
			if (aDX != 0)
			{
				int aParity = aPiece->mCol % 2;
				if ((aParity == 0) && (aDY == 0))
					aDY = 1;
				else if ((aParity == 1) && (aDY == 0))
					aDY = -1;
			}

			Graphics anArrowG(*g);
			int aCelWidth = anArrowsImage->GetWidth() / 3;
			int aHeight = anArrowsImage->GetHeight();
			int aCelX = aDX + 1;
			int aCelY = (aDY >= 0) ? 1 : 0;
			int aTransY = (57 - anArrowsImage->GetHeight()) / 2 + (int)aPiece->mY;
			anArrowG.Translate((57 - anArrowsImage->GetWidth()) / 2 + (int)aPiece->mX, aTransY);
			anArrowG.ClipRect(aCelX * aCelWidth, aCelY * (aHeight / 2), aCelWidth, aHeight / 2);
			anArrowG.DrawImage(anArrowsImage, 0, 0);
		}

		// port: the registered build's limit (12 letters) is taken; the trial build's 8 (app
		// mIsRegistered false) is not ported.
		if (aLetterCount > aLimit)
			g->DrawImage(mApp->mImages[IMAGE_TILE_X], (int)aPiece->mX, (int)aPiece->mY);
	}
}
