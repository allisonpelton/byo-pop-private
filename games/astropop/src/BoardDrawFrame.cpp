// Board: the frame around the field (0x444520): the panels at the sides, the brickometer, the pictures of the game's phase
// under the field, the gauge and the button of the supa weapon, and the score.

#include "Board.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"

using namespace Sexy;

// 0x444520
void Board::DrawFrame(Graphics* g)
{
	int aTick = (m1924 < 1) ? mUpdateCnt : m1af0;
	bool aDim = m1b3c > 0;                  // the screen fades out: the frame goes dark with it

	if (mFlag8E8)
	{
		g->DrawImage(IMAGE_UI_MANGLED_L, m1a80, 0);
		g->DrawImage(IMAGE_UI_MANGLED_R, m1a80 + 600, 0);
		g->DrawImage(IMAGE_UI_MANGLED_B, m1a80 + 0xa9, 0x1cf);
	}
	else
	{
		if (m7e8 != 0)
			g->DrawImage(IMAGE_BRICK_GENERATOR_UI, m1a80 + 0x98, -(m7ec + m7e8));

		if (aDim)
		{
			g->SetColorizeImages(true);
			int aShade = 0xff - m1b3c;
			g->SetColor(Color(aShade, aShade, aShade));
		}
		if (m8dc)
		{
			// the two dark bars behind the recording texts
			g->SetColor(Color(0, 0, 0, 200));
			Font* aBarFont = GetFontById(FONT_HIGH_SCORE_ID);
			g->FillRect(0, 0x125, 0x280, aBarFont->mHeight + 4);
			aBarFont = GetFontById(FONT_HIGH_SCORE_ID);
			g->FillRect(0, 0x14e, 0x280, aBarFont->mHeight + 4);
		}
		g->DrawImage(IMAGE_UI_INGAME_L, m1a80, 0);
		if (aDim)
			g->SetColorizeImages(false);

		// the brickometer: how full it is (the progress of the level, or the time of a survival game), and its colour
		float aFill;
		if (m910)
			aFill = 1.0f;
		else if (m1a60 == 1)
			aFill = m928;
		else if (m1a60 == 2)
			aFill = (float) (m1a14 % 300) * 0.0033333334f;
		else
			aFill = 1.0f;
		int aFillHeight = (int) (aFill * 444.0f);

		Color aGaugeColor;
		if (aFill >= 0.5f)
		{
			int aGreen;
			if (aFill >= 0.75f)
			{
				aGreen = 0x80 - (int) ((aFill - 0.75f) * -512.0f);
				if (aGreen > 0xff)
					aGreen = 0xff;
			}
			else
				aGreen = (int) ((aFill - 0.5f) * 512.0f);
			aGaugeColor = Color(0xff, aGreen, 0);
		}
		else
		{
			int aBlue = 0x80 - (int) (aFill * 255.0f);
			if (aBlue < 0)
				aBlue = 0;
			aGaugeColor = Color(0xff, 0, aBlue);
		}

		if ((m8eb && !m8ec) || m8ed || m8ef || m8f4 || m8ec || m910 || m917 || m916)
			aGaugeColor = Color(0x7f, 0x7f, 0x7f);
		if ((m8ec || (m917 && m1bc0 < 3)) && mUpdateCnt % 0x1e < 0xf && (m1b2c != 1 || m917))
			aGaugeColor = Color(0xff, 0xff, 0xff);

		g->SetColor(Color(0, 0, 0));
		g->FillRect(m1a80 + 0x267, 7, 0x17, 0x1bc);
		g->SetColor(aGaugeColor);
		g->FillRect(m1a80 + 0x267, 0x1c3 - aFillHeight, 0x17, aFillHeight);
		g->DrawImage(IMAGE_UI_INGAME_R, m1a80 + 600, 0);
		g->DrawImage(IMAGE_UI_INGAME_T, m1a80 + 0xa9, 0);
		g->DrawImage(IMAGE_UI_INGAME_B, m1a80 + 0xa9, 0x1cf);
		g->DrawImage(IMAGE_UI_BRICKOMETER_OVERLAY, m1a80 + 0x26b, 0x9a);

		if (m1a60 == 1)
		{
			g->SetFont(GetFontById(FONT_DIALOG_BOX_ID));
			int aShade = aDim ? 0xff - m1b3c : 0xff;
			g->SetColor(Color(aShade, aShade, aShade));
			g->DrawString(StrFormat("Level %d", m1940), m1a80 + 0x36, 0xbb);
		}
		if (m1a60 == 2)
		{
			if (aDim)
			{
				g->SetColorizeImages(true);
				int aShade = 0xff - m1b3c;
				g->SetColor(Color(aShade, aShade, aShade));
			}
			g->DrawImage(IMAGE_UI_SURVIVAL_TIME, 0x84, 0x7d);
			if (aDim)
				g->SetColorizeImages(false);
		}

		// the picture under the field: the weapon of the supa game type, or the phase's own
		bool aSkipPhase = false;
		if (m1a34 == 1)
		{
			if (mMode == 0 || mMode == 3)
			{
				g->DrawImage(m250[6], m1a80, 0x131);
				if (m1ab8 == 7 && m1ac0 > 0)
				{
					g->SetColorizeImages(true);
					g->SetColor(Color(0xff, 0xff, 0xff, m1ac0 << 2));
					g->DrawImage(m250[7], m1a80, 0x131);
					g->SetColorizeImages(false);
				}
				if (m1ad4 == 0)
				{
					g->SetColor(Color(0, 0, 0, 0x80));
					g->FillRect(0xa9, 3, 0x1b4, 0x18);
					g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
					g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
					int aSeconds = (int) (m1bd8 / m1be0 * 0.01f + 1.0f);
					WriteString(g, StrFormat("^FFFF00^SUPA Weapon time remaining: ^FFFFFF^%d", aSeconds), 0xa6, 0x14, 0x1b8, 0,
						true, 0, -1);
				}
			}
			else if (mMode == 1 || mMode == 2)
			{
				g->DrawImage(m250[6], m1a80, 0x131);
				int aFade = 0;
				if (m1ab8 == 7 && m1ac0 > 0)
				{
					g->SetColorizeImages(true);
					aFade = m1ac0;
				}
				else if (m1ab8 == 6 && m1ac0 > 0)
				{
					g->SetColorizeImages(true);
					aFade = PickSpawnDelay() - m1ac0;
				}
				else
					aSkipPhase = true;
				if (!aSkipPhase)
				{
					g->SetColor(Color(0xff, 0xff, 0xff, aFade << 4));
					g->DrawImage(m250[7], m1a80, 0x131);
					g->SetColorizeImages(false);
				}
			}
		}
		else
		{
			g->DrawImage(m250[m1ab8], m1a80, 0x131);
			if (mApp->Is3DAccelerated() && ((m1ab8 > 1 && m1ab8 < 6) || (m1ab8 > 7 && m1ab8 < 10)))
			{
				// the picture glows over itself
				if (mApp->Is3DAccelerated())
				{
					g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					g->SetColorizeImages(true);
				}
				Image* aGlow;
				if (m1ab8 < 2 || m1ab8 > 5)
				{
					int aPhase = aTick % 0x7e;
					if (aPhase > 0x3f)
						aPhase = 0x7e - aPhase;
					if (aPhase < 0)
						aPhase = 0;
					g->SetColor(Color(0xff, 0xff, 0xff, aPhase * 4));
					aGlow = m250[1];
				}
				else
				{
					int aPhase = aTick % 0xfe;
					if (aPhase > 0x7f)
						aPhase = 0xfe - aPhase;
					if (aPhase < 0)
						aPhase = 0;
					g->SetColor(Color(0xff, 0xff, 0xff, aPhase * 2));
					aGlow = m250[0];
				}
				g->DrawImage(aGlow, m1a80, 0x131);
				if (mApp->Is3DAccelerated())
				{
					g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
					g->SetColorizeImages(false);
				}
			}
		}

		// the time left of a bonus round, across the top of the field
		if (m910 && m19fc < 0x3d && (m1bc0 == 0 || m1bc0 > 1))
		{
			g->SetColor(Color(0, 0, 0, 0xff));
			g->FillRect(0xa7, 4, 0x1b7, 0x17);
			int aAlpha = 0xff;
			if (m19fc > 0 && m19fc <= 0x3c)
			{
				int aFadeOut = m19fc * 3;
				if (aFadeOut > 0xff)
					aFadeOut = 0xff;
				aAlpha = 0xff - aFadeOut;
			}
			g->SetColor(Color(0xff, 0xff, 0, aAlpha));
			Font* aFont = GetFontById(FONT_DIALOG_BOX_BTN_ID);
			g->SetFont(aFont);
			std::string aLabel = "Bonus round time remaining: ";
			g->DrawString(aLabel, 0xf0, 0x14);
			g->SetColor(Color(0xff, 0xff, 0xff));
			int aSeconds = (m1b70 < 0) ? 0x3c : m1b70 / 100;
			g->DrawString(StrFormat("%d", aSeconds), aFont->StringWidth(aLabel) + 0xf0, 0x14);
		}

		if ((m8eb && !m8ec) || m8ed || m8ef || m8f4 || m8ec)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			DrawCel(g, IMAGE_CP_WARP_OVERLAY, m1a80, 0x131, m1b14);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}

		// the gauge of the supa weapon (its width is how far it has charged) and the button that fires it
		Image* aGauge = IMAGE_POWERUP_GAUGE;
		double aCharge = (double) m1bd8 / (double) m1bdc;
		if (aCharge > 1.0)
			aCharge = 1.0;
		int aGaugeWidth = (int) ((double) aGauge->mWidth * aCharge);
		Image* aLights = IMAGE_SUPA_BUTTON_LIGHTS_OFF;
		Image* aButton = NULL;
		int aGlowAlpha = 0;
		bool aPanelOut = !m230.empty() && m230[0]->m1c && m7e8 <= -m7ec;    // a hint panel is up over the field
		if (m1b48 > -IMAGE_SUPA_BUTTON_ALERT->mWidth)
		{
			int aX = m1958;
			int aY = m195c;
			bool aOverStart = aX >= 18 && aX <= 126 && aY >= 225 && aY <= 253 && m1a34 == 0;
			bool aOverPause = mOverlay.mFlag78 && aX >= -4 && aX <= -4 + 152 && aY >= 194 && aY <= 194 + 102;
			if (aOverStart || aOverPause)
			{
				bool aBusy = m8ec || m8eb || m8ed || m8ef || m8f4 || aPanelOut || m910 || m917 || m916;
				if (mApp->IsLeftButtonDown() && !aBusy)
					aButton = IMAGE_SUPA_BUTTON_DOWN;
				else if (aBusy)
					aButton = IMAGE_SUPA_BUTTON_UP;
				else
					aButton = IMAGE_SUPA_BUTTON_OVER;

				aGlowAlpha = mUpdateCnt % 0xff;
				if (aGlowAlpha > 0x7f)
					aGlowAlpha = 0xff - aGlowAlpha;
				if (aDim)
				{
					aGlowAlpha = 0;
					aLights = m920 ? IMAGE_SUPA_BUTTON_LIGHTS_ON : IMAGE_SUPA_BUTTON_LIGHTS_OFF;
				}
			}
			else if (aDim && m1a34 == 0)
			{
				if (m920)
				{
					aLights = IMAGE_SUPA_BUTTON_LIGHTS_ON;
					aButton = IMAGE_SUPA_BUTTON_ALERT;
				}
				else
					aButton = IMAGE_SUPA_BUTTON_UP;
			}
			else
			{
				aGlowAlpha = mUpdateCnt % 0xff;
				if (aGlowAlpha > 0x7f)
					aGlowAlpha = 0xff - aGlowAlpha;
				aButton = IMAGE_SUPA_BUTTON_UP;
			}
		}

		bool aLocked = m910 || m917 || m8ec || m8eb || m8ed || m8ef || m8f4;     // the gauge and the button are dimmed
		if (!aLocked)
		{
			g->DrawImage(aGauge, m1a80 + 3, 0xde, Rect(0, 0, aGaugeWidth, aGauge->mHeight));
			if (m1a34 == 0 && m1bd8 < m1bdc)
				g->DrawImage(IMAGE_POWERUP_CHARGING, m1a80 + 3, 0xde);
			else if (m1a34 == 1)
				g->DrawImage(IMAGE_POWERUP_ACTIVE, m1a80 + 3, 0xde);
			if (aButton != NULL)
			{
				g->DrawImage(aButton, m1b48 + m1a80, 0xd7);
				g->DrawImage(aLights, m1b48 + m1a80, 0xd7);
				if (aGlowAlpha > 0)
				{
					g->SetColorizeImages(true);
					g->SetColor(Color(0xff, 0xff, 0xff, aGlowAlpha * 2));
					g->DrawImage(IMAGE_SUPA_BUTTON_LIGHTS_ON, m1b48 + m1a80, 0xd7);
					g->SetColorizeImages(false);
				}
			}
		}
		else
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, 0x80));
			g->DrawImage(aGauge, m1a80 + 3, 0xde, Rect(0, 0, aGaugeWidth, aGauge->mHeight));
			g->SetColorizeImages(false);
			if (m1a34 == 0 && m1bd8 < m1bdc)
				g->DrawImage(IMAGE_POWERUP_CHARGING, m1a80 + 3, 0xde);
			else if (m1a34 == 1)
				g->DrawImage(IMAGE_POWERUP_ACTIVE, m1a80 + 3, 0xde);
			if (aButton != NULL)
			{
				g->SetColorizeImages(true);
				g->SetColor(Color(0x80, 0x80, 0x80));
				g->DrawImage(aButton, m1b48 + m1a80, 0xd7);
				g->DrawImage(aLights, m1b48 + m1a80, 0xd7);
				g->SetColorizeImages(false);
			}
		}

		if (m910 || m917)
			g->SetColorizeImages(false);

		if (!m648)
			DrawCel(g, IMAGE_FUELCELL_DEATH, 0x217, 0x30, (mAnim620.mNumFrames == 1) ? 0 : mAnim620.mFrame);

		if (aDim)
		{
			g->SetColorizeImages(true);
			int aShade = 0xff - m1b3c;
			g->SetColor(Color(aShade, aShade, aShade));
			g->SetColorizeImages(false);
		}
	}

	// the score (the time of a survival game)
	g->SetFont(FONT_PLAYER_SCORE);
	g->SetColor(Color(0xff, 0xff, 0, aDim ? 0xff - m1b3c : 0xff));
	if (m1a60 == 2)
	{
		int aMinutes = m19b4 / 6000;
		int aSeconds = (m19b4 / 100) % 60;
		std::string aTime;
		if (aSeconds < 10)
			aTime = StrFormat("%d:0%d", aMinutes, aSeconds);
		else
			aTime = StrFormat("%d:%d", aMinutes, aSeconds);
		WriteString(g, aTime, m1a80 + 10, 0x9a, 0x7d, 1, true, 0, -1);
	}
	else
		WriteString(g, FormatNumber(m1970), m1a80 + 10, 0x9a, 0x78, 1, true, 0, -1);
}
