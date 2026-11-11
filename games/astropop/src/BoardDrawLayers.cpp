// Board: Draw (0x461e50). It puts the layers of the screen on in this order: the backdrop, the danger zone, the texts of the
// modes, the beam, the bricks, the banners of the goal, the crosshair, the explosions, the end-of-game page, the snow, the
// panels of the stages and the upgrades, the logo or the combo, the SUPA weapon, the ship, the sprites on top and the
// hints. The original works out its positions on the x87 stack at 24 bits of precision (UpdateFrames sets it): every sum is a
// float before it is cut to a whole pixel, as _ftol does.

#include "Board.h"
#include "GameText.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "Font.h"
#include "FrameAnim.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Particle.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "SmallObjects.h"
#include "UserProfile.h"

using namespace Sexy;

// 0x461e50 slot 16
void Board::Draw(Graphics* g)
{
	// (the original asks the application if it has 3D here and does nothing with the answer)
	int aCount = (m1924 < 1) ? mUpdateCnt : m1af0;

	if (m901)
	{
		mScreenWipe->Draw(g, 0, 0);
		return;
	}

	DrawBackdrop(g);
	DrawDangerZone(g);
	DrawModeTexts(g);
	DrawBeam(g);

	if (mMode == 0 && m1a34 == 1 && !mLaserAnim.mStopped)
		DrawCel(g, IMAGE_PLAYER1_LASER, m198c, 0, mLaserAnim.GetCel());

	DrawBricks(g, false);
	if (!m8ed)
		DrawBricks(g, true);

	DrawGoalBanners(g, aCount);
	DrawCrosshair(g);
	DrawBonusIntro(g);
	DrawExplosions(g);
	if (mFlag8E8)
		DrawEndSparks(g);
	if (m91d)
		DrawEndPage(g);
	DrawSnow(g);

	if ((m1a60 == 1 || m90a) &&
		((!m8eb && (m8ed || (!m8ef && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0))) || m8ed))
		DrawStageBg(g);

	DrawUpgradeArm(g);
	DrawSurvivalIntro(g);
	DrawFrame(g);
	if (m8de)
		m778->Draw(g);

	DrawLogo(g);
	DrawSupaBrackets(g);
	DrawFreezeFx(g);
	DrawSirens(g);

	if (!m917 || m1bc8 == 0 || 0 < m1bc8)
		DrawShip(g);
	if (m1a60 == 1)
		DrawHud(g);

	for (unsigned int i = 0; i < mFadedBricks.size(); i++)
		mFadedBricks[i]->Draw(g, false);
	for (unsigned int i = 0; i < mFloatTexts.size(); i++)
		mFloatTexts[i]->Draw(g);

	if (m1b64 != 0)
	{
		if (m1b60 < 0xff)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, m1b60));
		}
		g->DrawImage(IMAGE_PERFECT, 0x182 - IMAGE_PERFECT->mWidth / 2, 0x96);
		g->SetColorizeImages(false);
	}

	if (m1a60 == 2)
	{
		if (0 < m1b40 && m1a34 != 1)
			DrawSurvivalBonus(g);
		for (unsigned int i = 0; i < mEnergySparks.size(); i++)
		{
			bool is3D = mApp->Is3DAccelerated();
			if (is3D)
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			mEnergySparks[i]->Draw(g);
			if (mApp->Is3DAccelerated())
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	if (mMode == 2 && -IMAGE_TURBOT_WAVE->mHeight < m1a48)
		g->DrawImage(IMAGE_TURBOT_WAVE, 0x8a, m1a48);

	if (m910 && 0 < m1b7c)
	{
		AnimateBanner();
		g->DrawImage(mBannerWaveImage, 0x182 - mBannerWaveImage->GetWidth() / 2, 0xf0 - mBannerWaveImage->GetHeight() / 2);
	}

	if (m916)
		DrawBonusStats(g);
	if (m8ef && m1a60 == 1)
		DrawWarpScreen(g);

	if (mApp->mFlag6f9 && !m8eb && !m8ed && !m8ef && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0 && !m916 &&
		(!m910 || m913))
	{
		for (unsigned int i = 0; i < mHintBoxes.size(); i++)
			mHintBoxes[i]->Draw(g);
	}

	DrawTestTexts(g);
}

//////////////////////////////////////////////////////////////////////////
// The parts of Draw, in its order

// the glow at the bottom of the screen that warns of the rows closing in
// port: a layer of Board::Draw (0x461e50)
void Board::DrawDangerZone(Graphics* g)
{
	if (!(!m910 && (13 < m970 || 0 < m1aac) && !m8eb && !m8ed && !m8ef && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0))
		return;

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, m1aac));
	g->DrawImage(IMAGE_UI_DANGERZONE, 0, mHeight - IMAGE_UI_DANGERZONE->mHeight);
	int aSecond = (m1aac < m1aa8) ? m1aac : m1aa8;
	if (0 < aSecond && mApp->Is3DAccelerated())
	{
		g->SetColor(Color(0xff, 0xff, 0xff, aSecond));
		g->DrawImage(IMAGE_UI_DANGERZONE, 0, mHeight - IMAGE_UI_DANGERZONE->mHeight);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
}

// the texts that the modes put on the field: the lines of the bonus of the first game type (+0x260) in a colour that
// runs round the hues, or the texts of the other game types (+0x270) in another
// port: a layer of Board::Draw (0x461e50)
void Board::DrawModeTexts(Graphics* g)
{
	if (mFlag8E8)
		return;

	if (m1a34 == 1)
	{
		Color aColor((int) (mApp->HSLToRGB(m1ad0, 0xff, 0x80) | 0xff000000));
		g->SetColor(aColor);
		for (unsigned int i = 0; i < mBonusBannerTexts.size(); i++)
		{
			g->SetFont(mBonusBannerTexts[i].mFont);
			g->SetColor(aColor);
			g->DrawString(mBonusBannerTexts[i].mText, mBonusBannerTexts[i].mX, mBonusBannerTexts[i].mY);
		}
	}
	else if (!m916 && m1990 == 0 && !mSpecialBannerTexts.empty())
	{
		Color aColor((int) (mApp->HSLToRGB(m1b04, 0xff, 0x80) | 0xff000000));
		g->SetColor(aColor);
		for (unsigned int i = 0; i < mSpecialBannerTexts.size(); i++)
		{
			g->SetFont(mSpecialBannerTexts[i].mFont);
			g->SetColor(aColor);
			g->DrawString(mSpecialBannerTexts[i].mText, mSpecialBannerTexts[i].mX, mSpecialBannerTexts[i].mY);
		}
	}
}

// the bars over the field that say what to do (the first game type) and, in the classic game, how many bricks to pop and
// whether the level is a checkpoint
// port: a layer of Board::Draw (0x461e50)
void Board::DrawGoalBanners(Graphics* g, int theCount)
{
	// the pulse of the text: up for a hundred updates, down for a hundred
	int aPulse = theCount % 200;
	if (99 < aPulse)
		aPulse = 200 - aPulse;

	if (m1a34 == 1 && !m91b && !m91c && !mFlag8E8)
	{
		g->SetColor(Color(0, 0, 0, 0x80));
		g->FillRect(Rect(0, 0xe6, mWidth, 0x14));
		g->SetColor(Color(0xff, 0xff, 0xff, aPulse + 0x96));
		Font* aFont = GetFontById(FONT_DIALOG_BOX_BTN_ID);
		g->SetFont(aFont);
		std::string aText;
		if (mMode == 2 || mMode == 1)
			aText = GT(0x00565dd8);
		else if (mMode == 3)
			aText = GT(0x00565e28);
		else if (mMode == 0)
			aText = GT(0x00565e00);
		if (mMode == 2 && mTurbotMarkedCells.empty())
			aText = GT(0x00565dbc);
		g->DrawString(aText, 0x182 - aFont->StringWidth(aText) / 2, 0xf5);
	}

	// the pop-up of a hint is open
	bool aHintOpen = !mHintSpots.empty() && mHintSpots[0]->m1c && !(-m7ec < m7e8);
	if (!m922 || aHintOpen)
	{
		if (-1 < m1960 && m923 && (!m8eb || m8ec) && !m8ed && !m8ef && !m8f4 && !m8ec && !aHintOpen && m1a60 == 1 && 1 < m1940)
		{
			int aTextAlpha;
			int aFillAlpha;
			if (m1960 == 0)
			{
				aTextAlpha = 0xff;
				aFillAlpha = 0x80;
			}
			else
			{
				aTextAlpha = m1960;
				aFillAlpha = (0x80 < m1960) ? 0x80 : m1960;
			}
			g->SetColor(Color(0, 0, 0, aFillAlpha));
			g->FillRect(Rect(0, 0x14a, mWidth, 0x14));
			int aPulseAlpha = (aTextAlpha <= aPulse + 0x96) ? aTextAlpha : aPulse + 0x96;
			g->SetColor(Color(0xff, 0xff, 0xff, aPulseAlpha));
			Font* aSmall = GetFontById(FONT_DIALOG_BOX_BTN_ID);
			g->SetFont(aSmall);
			std::string aBricks = StrFormat(GTC(0x00565d84), m1a18);
			g->DrawString(aBricks, 0x182 - aSmall->StringWidth(aBricks) / 2, 0x159);

			// a level that makes a new starting point: every fourth, once it is further than the best one so far
			int aBest = 0;
			mApp->mUser->GetInt(mMode + 8, &aBest);
			if (m1940 % 4 == 0 && aBest < m1940)
			{
				Font* aBig = GetFontById(FONT_HIGH_SCORE_ID);
				g->SetColor(Color(0xff, 0xff, 0, aTextAlpha));
				g->SetFont(aBig);
				WriteString(g, GT(0x00565d70), 0xa6, g->GetFont()->GetAscent() + 0xf5, 0x1b8, 0, true, 0, -1);
				g->SetColor(Color(0xff, 0xff, 0xff, aTextAlpha));
				g->SetFont(aSmall);
				WriteString(g, GT(0x00565d50), 0xa6, aBig->GetHeight() + aBig->GetAscent() + 0xf5, 0x1b8, 0,
					true, 0, -1);
				WriteString(g, GT(0x00565d3c), 0xa6, aBig->GetHeight() + aBig->GetAscent() + aSmall->GetHeight() + 0xf5,
					0x1b8, 0, true, 0, -1);
			}
		}
	}
	else
	{
		// a game that goes on from its last checkpoint
		g->SetColor(Color(0, 0, 0, 0x80));
		g->FillRect(Rect(0, 0x14a, mWidth, 0x14));
		g->SetColor(Color(0xff, 0xff, 0xff, aPulse + 0x96));
		Font* aFont = GetFontById(FONT_DIALOG_BOX_BTN_ID);
		g->SetFont(aFont);
		std::string aText = GT(0x00565d9c);
		g->DrawString(aText, 0x182 - aFont->StringWidth(aText) / 2, 0x159);
		if (m1a60 == 1)
		{
			std::string aBricks = StrFormat(GTC(0x00565d84), m1a18);
			g->SetColor(Color(0, 0, 0, 0x80));
			g->FillRect(Rect(0, 0x15e, mWidth, 0x14));
			g->SetColor(Color(0xff, 0xff, 0xff, aPulse + 0x96));
			g->DrawString(aBricks, 0x182 - aFont->StringWidth(aBricks) / 2, 0x16d);
		}
	}
}

// the mark that follows the pointer in the mode that picks single bricks, or the one on the brick the pointer is over
// port: a layer of Board::Draw (0x461e50)
void Board::DrawCrosshair(Graphics* g)
{
	if (mFlag8E8)
		return;

	int aCol;
	int aRow;
	int aCel;
	if (!mHoverCells.empty())
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		aCol = mHoverCells[0].mCol;
		aRow = mHoverCells[0].mRow;
		aCel = 2;
	}
	else if (mMode == 3 && m1a34 == 1)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		aRow = (m96c < 0x22) ? 1 : (m96c - 0x22) / 0x19 + 1;
		if (aRow == 0)
			aRow = 1;
		aCol = (m968 - 0xa6) / 0x2c;
		aCel = 0;
	}
	else
		return;

	int aY = (aRow < 1) ? 4 : aRow * 0x19 + 9;
	DrawCel(g, IMAGE_VIXX_CROSSHAIR, aCol * 0x2c + 0x98, aY - 0xd, aCel);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// the atomic explosion of the bonus (the eighth of the animations of the explosions), twice the size of its picture, and the
// symbol that fades in over it
// port: a layer of Board::Draw (0x461e50)
void Board::DrawExplosions(Graphics* g)
{
	for (unsigned int i = 0; i < mAtomicExplosions.size(); i++)
	{
		ExplosionAnim* anExplosion = mAtomicExplosions[i];
		if (anExplosion->mDone)
			continue;
		if (!(anExplosion->mMaxLoops < 1 || anExplosion->mLoopCount < anExplosion->mMaxLoops))
			continue;
		if (i != 7)
			continue;

		int aWidth = (IMAGE_ATOMIC_EXPLOSION->mWidth / IMAGE_ATOMIC_EXPLOSION->mNumCols) * 2;
		int aX = 0x182 - aWidth / 2;
		Rect aDest(aX, (int) anExplosion->mY, aWidth, IMAGE_ATOMIC_EXPLOSION->mHeight * 2);
		DrawCelStretched(g, IMAGE_ATOMIC_EXPLOSION, aDest, anExplosion->GetCel());
		if (0 < m1a40)
		{
			int anAlpha = m1a40 * 2;
			if (0xff < anAlpha)
				anAlpha = 0xff;
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
			g->SetColorizeImages(true);
			g->DrawImage(IMAGE_ATOMIC_SYMBOL, aX, (int) anExplosion->mY);
			g->SetColorizeImages(false);
		}
	}
}

// the end of a game: the panels fade in over the field one after the other, and the sparks of the sun drift over them
// port: a layer of Board::Draw (0x461e50)
void Board::DrawEndSparks(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	for (unsigned int i = 0; i < mEndBlastFades.size(); i++)
	{
		if (0 < mEndBlastFades[i].mValue)
		{
			g->SetColor(Color(0xff, 0xff, 0xff, mEndBlastFades[i].mValue));
			g->DrawImage(GetImageById(IMAGE_BIG_BLAST1_ID + i), 0, 0);
		}
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
	for (unsigned int i = 0; i < mSunSparks.size(); i++)
	{
		SunParticle* aSpark = mSunSparks[i];
		if (!aSpark->mAnim.mStopped)
			DrawCel(g, IMAGE_SUN_PARTICLE, (int) aSpark->mX, (int) aSpark->mY, aSpark->mAnim.GetCel());
	}
}

// the snowflakes of the freeze (only a quarter of them without 3D); each is brightest in the middle of the screen
// port: a layer of Board::Draw (0x461e50)
void Board::DrawSnow(Graphics* g)
{
	if (!(0 < m19fc && m1ad4 == 0 && (m1a34 == 0 || mMode == 1 || mMode == 2)))
		return;

	int aMaxAlpha = (m19fc < 0x3d) ? m19fc * 4 : 0xff;
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	int aFlakes = (int) mFreezeFlakes.size();
	if (!mApp->Is3DAccelerated())
		aFlakes = aFlakes >> 2;
	for (int i = 0; i < aFlakes; i++)
	{
		FreezeFlake& aFlake = mFreezeFlakes[i];
		int anAlpha;
		if (aFlake.mVelX > 0.0f)
			anAlpha = (int) (520.0f - aFlake.mX);
		else
			anAlpha = (int) (aFlake.mX - 270.0f);
		if (0xff < anAlpha)
			anAlpha = 0xff;
		if (aMaxAlpha < anAlpha)
			anAlpha = aMaxAlpha;
		if (0 < anAlpha)
		{
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
			DrawCel(g, IMAGE_PARTICLE_SNOWFLAKE, (int) aFlake.mX, (int) aFlake.mY, aFlake.mAnim.GetCel());
		}
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
}

// the arm and the head of the upgrade screen
// port: a layer of Board::Draw (0x461e50)
void Board::DrawUpgradeArm(Graphics* g)
{
	if (!((m1b2c == 2 || m1b2c == 4) && (m8ef || m8ed)))
		return;

	g->DrawImage(m834, m820, m824);
	g->DrawImage(m890, m87c, m880);
	int anArmWidth = IMAGE_UPGRADE_ARM->mWidth;
	int aHeadWidth = IMAGE_UPGRADE_HEAD->GetCelWidth();
	DrawCel(g, m838, m820 + anArmWidth, m824, mAnim83c.GetCel());
	DrawCel(g, m894, m87c - aHeadWidth, m880, mAnim898.GetCel());
}

// the introduction of the second game type
// port: a layer of Board::Draw (0x461e50)
void Board::DrawSurvivalIntro(Graphics* g)
{
	if (!(m1a60 == 2 && !m8fc))
		return;

	int anAlpha = m1960;
	int aFillAlpha;
	if (anAlpha == 0)
	{
		anAlpha = 0xff;
		aFillAlpha = 0x80;
	}
	else
	{
		if (anAlpha < 1)
			return;
		aFillAlpha = (0x80 < anAlpha) ? 0x80 : anAlpha;
	}

	g->SetColor(Color(0, 0, 0, aFillAlpha));
	g->FillRect(Rect(0, 0xbe, mWidth, 0xb9));
	g->SetColor(Color(0xff, 0xff, 0, anAlpha));
	g->SetFont(GetFontById(FONT_HIGH_SCORE_ID));
	WriteString(g, GT(0x00565ba4), 0xa6, 0xdd, 0x1b8, 0, true, 0, -1);
	g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
	g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
	WriteString(g, GT(0x00565b80), 0xa6, 0xf1, 0x1b8, 0, true, 0, -1);
	WriteString(g, GT(0x00565b6c), 0xa6, 0x105, 0x1b8, 0, true, 0, -1);
	g->SetColor(Color(0xff, 0xff, 0, anAlpha));
	g->SetFont(GetFontById(FONT_HIGH_SCORE_ID));

	ProfileRecord aBest;
	for (int i = 0; i < 6; i++)
		aBest.mField[i] = -1;
	if (!mApp->mUser->GetRecord(2, 0, &aBest))
	{
		WriteString(g, GT(0x00565b58), 0xa6, 0x144, 0x1b8, 0, true, 0, -1);
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
		WriteString(g, GT(0x00565b40), 0xa6, 0x15b, 0x1b8, 0, true, 0, -1);
	}
	else
	{
		WriteString(g, GT(0x00565b30), 0xa6, 0x144, 0x1b8, 0, true, 0, -1);
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
		int aSeconds = aBest.mField[4] % 60;
		g->SetFont(GetFontById(FONT_HIGH_SCORE_ID));
		std::string aTime = StrFormat((aSeconds < 10) ? "%d:0%d" : "%d:%d", aBest.mField[4] / 60, aSeconds);
		WriteString(g, aTime, 0xa6, 0x15b, 0x1b8, 0, true, 0, -1);
	}
}

// the logo at the top (the classic game's or the second game type's), or while a combo runs the picture of the combo
// port: a layer of Board::Draw (0x461e50)
void Board::DrawLogo(Graphics* g)
{
	if (m1afc < 1 || mFlag8E8 || m910)
	{
		Image* aLogo = (m1a60 != 2) ? IMAGE_UI_LOGO_CLASSIC : IMAGE_UI_LOGO_SURVIVAL;
		g->DrawImage(aLogo, m1a80, 0x1d);
		return;
	}

	int aRow = m1b00 - 2;
	if (IMAGE_COMBO_LIST->mNumRows <= aRow)
		aRow = IMAGE_COMBO_LIST->mNumRows - 1;
	int aCelHeight = IMAGE_COMBO_LIST->GetCelHeight();
	Rect aSrc(0, aCelHeight * aRow, IMAGE_COMBO_LIST->mWidth, aCelHeight);
	Font* aFont = (0x13 < m1b00) ? FONT_LARGE_COMBO_BLUE : FONT_COMBO_BLUE;
	if (m1af8 == 1)
	{
		g->DrawImage(IMAGE_UI_SCREEN_RED, 0, 0);
		aFont = (m1b00 < 0x14) ? FONT_COMBO_RED : FONT_LARGE_COMBO_RED;
	}
	else if (m1af8 == 2)
	{
		g->DrawImage(IMAGE_UI_SCREEN_YELLOW, 0, 0);
		aFont = (0x13 < m1b00) ? FONT_LARGE_COMBO_YELLOW : FONT_COMBO_YELLOW;
	}
	if (m1b00 < 6 || m924)
	{
		g->DrawImage(IMAGE_COMBO_LIST, 0xd, 0x10, aSrc);
		std::string aText = StrFormat("x%d", m1b00);
		int aTextWidth = aFont->StringWidth(aText);
		g->SetFont(aFont);
		g->SetColor(Color(0xff, 0xff, 0xff));
		Rect aRect = (m1b00 < 0x14) ? Rect(8, 0x39, 0x85, 0x2f) : Rect(8, 0x39, aTextWidth, 0x2f);
		WriteWordWrapped(g, aRect, aText, -1, 0);
	}
}

// the width of the part of the SUPA weapon's title that is shown for each step of its charge, and of the bonus round's
// (0x563c4c and 0x563c78)
static const int kSupaTextWidths[11] = {45, 89, 126, 173, 189, 233, 268, 303, 338, 372, 412};
static const int kBonusTextWidths[11] = {40, 77, 117, 154, 194, 216, 256, 293, 331, 370, 412};

// the brackets that open over the top of the field when the SUPA weapon (or a bonus round) is ready, with its title and
// the name of the weapon
// port: a layer of Board::Draw (0x461e50)
void Board::DrawSupaBrackets(Graphics* g)
{
	if (m1ad4 == 0 || mFlag8E8)
		return;

	int aShift = -m1adc;
	int aLeftX = aShift + 0x167;
	int aRightX = IMAGE_SUPA_BRACKET_LEFT->mWidth + 0x167 + m1adc;
	if (0x24c < aRightX)
		aRightX = 0x24d;
	int aGreen = m1ae0;
	int anAlpha = (0x7f < aGreen) ? aGreen : 0x80;
	g->SetColor(Color(0xff, aGreen, aGreen, anAlpha));
	if (0 < m1adc)
		g->FillRect(aShift + 0x179, m1ad8 + 5, aRightX + (6 - aLeftX), 0x50);
	g->DrawImage(IMAGE_SUPA_BRACKET_LEFT, aLeftX, m1ad8);
	g->DrawImage(IMAGE_SUPA_BRACKET_RIGHT, aRightX, m1ad8);

	if (2 < m1ad4)
	{
		Image* aTitle;
		int aWidth;
		if (!m910)
		{
			aWidth = kSupaTextWidths[m1acc];
			aTitle = IMAGE_SUPA_WEAPON_TEXT;
		}
		else
		{
			aWidth = kBonusTextWidths[m1acc];
			aTitle = IMAGE_BONUSROUND_TEXT;
		}
		g->DrawImage(aTitle, 0xb4, m1ad8 + 0xc, Rect(0, 0, aWidth, aTitle->mHeight));
	}

	if (3 < m1ad4)
	{
		g->SetColor(Color(0xff, 0xff, 0, m1ae4));
		std::string aName = GT(0x00565b1c);
		if (mMode == 1)
			aName = GT(0x00565b0c);
		else if (mMode == 2)
			aName = GT(0x00565afc);
		else if (mMode == 3)
			aName = GT(0x00565aec);
		if (m910)
			aName = GT(0x00565ae0);
		Font* aFont = GetFontById(FONT_NEURO30_ID);
		g->SetFont(aFont);
		g->DrawString(aName, ((IMAGE_SUPA_BRACKET_RIGHT->mWidth + 0x1cd) / 2 - aFont->StringWidth(aName) / 2) + 0x80,
			m1ad8 + 0x3f);
	}
}

// the frost over the screen while time is frozen
// port: a layer of Board::Draw (0x461e50)
void Board::DrawFreezeFx(Graphics* g)
{
	if (m19fc <= 0)
		return;

	if (m19fc < 0x3d)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, m19fc << 2));
	}
	if ((m1a34 == 0 || mMode == 1 || mMode == 2) && ((mMode != 2 && mMode != 1) || m1ad4 == 0))
		g->DrawImage(IMAGE_UI_FREEZE_OVERLAY, 0x95, 0);
	g->DrawImage(IMAGE_UI_FREEZE_FX, m1a80 + 0x3c, 0x6c);
	g->DrawImage(IMAGE_UI_FREEZE_FX, m1a80 + 0x52, 0x9e);
	g->DrawImage(IMAGE_UI_FREEZE_FX, m1a80, 0xfe);
	if (m19fc < 0x3d)
		g->SetColorizeImages(false);
}

// the two sirens at the top of the screen, each in the picture and in its light (3D only)
// port: a layer of Board::Draw (0x461e50)
void Board::DrawSirens(Graphics* g)
{
	if (mSirenAnim.mStopped || mFlag8E8)
		return;

	int aCel = mSirenAnim.GetCel();
	DrawCel(g, IMAGE_SIREN, m1a80, -0xe, aCel);
	DrawCel(g, mSirenImage, m1a80 + 0x244, -0xe, mSirenAnim.GetCel());
	if (mApp->Is3DAccelerated())
	{
		if (mApp->Is3DAccelerated())
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		DrawCel(g, IMAGE_SIREN_FX, m1a80, -0x31, mSirenAnim.GetCel());
		DrawCel(g, mSirenFxImage, m1a80 + 0x1bd, -0x31, mSirenAnim.GetCel());
		if (mApp->Is3DAccelerated())
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// the bar across the top that tells which colour the second game type has supercharged
// port: a layer of Board::Draw (0x461e50)
void Board::DrawSurvivalBonus(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, 0x80));
	g->FillRect(0xa9, 3, 0x1b8, 0x18);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));

	// the first mark that is lit, the first of all when none is
	unsigned int aKind = 0;
	for (unsigned int i = 0; i < mSuperchargeMarks.size(); i++)
	{
		if (mSuperchargeMarks[i].mActive)
		{
			aKind = (unsigned int) mSuperchargeMarks[i].mType;
			break;
		}
	}
	std::string aColor;
	switch (aKind)
	{
	case 0:
		aColor = GT(0x00565adc);
		break;
	case 1:
		aColor = GT(0x00565ad4);
		break;
	case 2:
		aColor = GT(0x00565acc);
		break;
	case 3:
		aColor = GT(0x00565ac4);
		break;
	case 4:
		aColor = GT(0x00565abc);
		break;
	default:
		break;
	}
	WriteString(g, StrFormat(GTC(0x00565a9c), aColor.c_str()), 0xa6, 0x14, 0x1b8, 0, true, 0, -1);
}

// the red box of a recording, and the flashing name of the demo
// port: a layer of Board::Draw (0x461e50)
void Board::DrawTestTexts(Graphics* g)
{
	if (m8db && mUpdateCnt % 0x28 < 0x14)
	{
		std::string aText = GT(0x00565a90);
		g->SetFont(GetFontById(FONT_DIALOG_BOX_ID));
		g->SetColor(Color(0, 0, 0));
		g->FillRect(500, 400, g->GetFont()->StringWidth(aText) + 4, g->GetFont()->GetHeight() + 4);
		g->SetColor(Color(0xff, 0, 0));
		g->DrawString(aText, 0x1f6, 0x19f);
	}

	if (m8dc)
	{
		int aPhase = mUpdateCnt % 0x80;
		if (0x3f < aPhase)
			aPhase = 0x7f - aPhase;
		g->SetColor(Color(0xff, 0xff, 0, aPhase * 4));
		g->SetFont(GetFontById(FONT_HIGH_SCORE_ID));
		std::string aText = GT(0x00565a84);
		g->DrawString(aText, 0x182 - g->GetFont()->StringWidth(aText) / 2, g->GetFont()->GetHeight() + 0x123);
	}
}
