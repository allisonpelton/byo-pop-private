// Board: the timers and effects that step every update (UpdateStates): the pace of the incoming pieces, the fade between
// the demo and the game, the alert flashes, the start banner, the minute banners, the freeze effect, the scrolling texts
// and the countdown of the marks on bricks.

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "SmallObjects.h"
#include "Res.h"

using namespace Sexy;

// 0x433920: the alert flashes. While the stack is high (+0x970 from 14) the danger picture pulses (+0x1aa8 between 0 and
// 255 by +0x1aa4) and its cover fades in (+0x1aac); the warning at the top (+0x1b3c) comes up when nothing else is going
// on. The "game over" danger picture slides in from above (+0x1a98, +0x1a9c); when the timer of the klaxon (+0x1a7c)
// ends the siren animation stops, the song goes back and the pace of the pieces is chosen again.
void Board::UpdateAlertFlash()
{
	if (m970 < 0xe || m910)
	{
		m404 = 1;
		if (m1aac > 0 && !m910)
		{
			m1aac -= 6;
			if (m1aac < 0)
				m1aac = 0;
		}
		if (m1b3c > 0)
		{
			m1b3c -= 2;
			if (m1b3c < 0)
				m1b3c = 0;
		}
	}
	else
	{
		m404 = m970 - 0xc;
		if (m404 > 5)
			m404 = 5;

		m1aa8 += m1aa4;
		if (m1aa8 >= 0x100)
		{
			m1aa8 = 0xff;
			m1aa4 = -m1aa4;
		}
		else if (m1aa8 < 0)
		{
			m1aa8 = 0;
			m1aa4 = -m1aa4;
		}

		if (m1aac < 0xff)
		{
			m1aac += 4;
			if (m1aac > 0xff)
				m1aac = 0xff;
		}

		if (m1a34 == 1 || m1bd8 < m1bdc || m8eb || m8ed || m8ef || m8f4 || m8ec || mFlag8E9 || mFlag8E8 || m1924 != 0 ||
			m1b3c > 0x7f)
		{
			if (m1b3c > 0)
			{
				m1b3c -= 2;
				if (m1b3c < 0)
					m1b3c = 0;
			}
		}
		else
			m1b3c++;

		if (m1b3c > 0 && mUpdateCnt % 0x32 == 0)
			m920 = !m920;
	}

	if (m1a98 > -500000000)
	{
		if (m1a98 > -0x14 - IMAGE_GAMEOVER_DANGER->mHeight)
		{
			int aOther = m1a9c;
			if (aOther < 1)
			{
				int aBefore = m1a98;
				m1a98 = aBefore - 0xc;
				if (aBefore - 0xc < 0x15 && aOther == -500000000)
					m1a9c = 1;
			}
			else
			{
				m1a9c = aOther + 1;
				if (aOther + 1 > 0x95)
					m1a9c = -250000000;
			}
		}
		else
		{
			m1a98 = -500000000;
			m1a9c = -500000000;
		}
	}

	if (m1a7c > 0)
	{
		m1a7c--;
		if (m1a7c < 1)
		{
			m1a7c = -1;
			mSirenAnim.SetStopped(true);
			mApp->PlaySong(9);
			if (m1ab8 < 10 || m1ab8 > 0xd)
			{
				m1ab8 = 4;
				m1abc = 0;
			}
			m1ac0 = PickSpawnDelay();
		}
	}
}

// 0x433610: the start banner (+0x1ad4 is its phase 0 to 6): the brackets slide in, the lines fade in one by one, hold,
// and the lot slides out. Phase 6 of a game of the second kind lets the next part (+0x911) begin. The screen shake of a
// strong hit (+0x1aec updates, +0x1a80 the shift) runs beside it.
void Board::UpdateStartBanner()
{
	if (m1aec > 0 && m1a34 == 1)
	{
		m1a80 = ((int) AstroRand() % 500000000) % 5 - 2;
		m1aec--;
		if (m1aec < 1)
			m1a80 = 0;
	}

	if (m1ad4 == 1)
	{
		m1ad8 += 4;
		if (m1ad8 >= 0)
		{
			m1ad8 = 0;
			m1adc = 0;
			m1ad4 = 2;
		}
	}
	else if (m1ad4 == 2)
	{
		m1adc += 0xc;
		if (m1adc > 0xe6)
		{
			m1ae8 = 0;
			m1acc = 0;
			m1adc = 0xe7;
			m1ad4 = 3;
		}
	}
	else if (m1ad4 == 3)
	{
		m1ae0 -= 3;
		if (m1ae0 < 1)
			m1ae0 = 0;
		m1ae8++;
		if (m1ae8 > 3)
		{
			m1ae8 = 0;
			m1acc++;
			if (m1acc > 10)
			{
				m1ae4 = 0;
				m1acc = 10;
				m1ad4 = 4;
			}
		}
	}
	else if (m1ad4 == 4)
	{
		m1ae0 -= 3;
		if (m1ae0 < 1)
			m1ae0 = 0;
		m1ae4 += 10;
		if (m1ae4 > 0xfe)
		{
			m1ae8 = 0;
			m1ae4 = 0xff;
			m1ad4 = 5;
		}
	}
	else if (m1ad4 == 5)
	{
		m1ae8++;
		if (m1ae8 > 0x95)
		{
			m1ae0 -= 3;
			if (m1ae0 < 1)
				m1ae0 = 0;
			m1ae8 = 0;
			m1ad4 = 6;
			if (m910)
				m911 = true;
		}
	}
	else if (m1ad4 == 6)
	{
		m1ae0 -= 3;
		if (m1ae0 < 1)
			m1ae0 = 0;
		m1ad8 -= 4;
		if (m1ad8 <= -IMAGE_SUPA_BRACKET_LEFT->mHeight)
			m1ad4 = 0;
	}
}

// 0x452a80: a time bonus. While the game is young (+0x1a2c counts the updates since it began to run, up to 700) the bonus is
// paid once for each step of the time (200, 300, 400, 500, 600, 700 updates) when the player has done well enough in the
// time (+0x1a50 at least 3 .. 8): a thousand points for each step times the fraction of the stage left (+0x940), at most
// 10000, 15000, 20000, 22000, 25000, 30000, rounded down to 50.
void Board::PayTimeBonus()
{
	int aBonus = 0;
	if (m910)
		return;

	int aTime = m1a2c;
	if (aTime <= 0xc8 && m1a30 == 0 && m1a50 >= 3)
	{
		m1a30 = 1;
		aBonus = (int) ((double) m940 * (double) 1000.0f);
		if (aBonus > 10000)
			aBonus = 10000;
	}
	else if (aTime <= 0x12c && m1a30 <= 1 && m1a50 >= 4)
	{
		m1a30 = 2;
		aBonus = (int) ((double) m940 * (double) 2000.0f);
		if (aBonus > 15000)
			aBonus = 15000;
	}
	else if (aTime <= 0x190 && m1a30 <= 2 && m1a50 >= 5)
	{
		m1a30 = 3;
		aBonus = (int) ((double) m940 * (double) 3000.0f);
		if (aBonus > 20000)
			aBonus = 20000;
	}
	else if (aTime <= 0x1f4 && m1a30 <= 3 && m1a50 >= 6)
	{
		m1a30 = 4;
		aBonus = (int) ((double) m940 * (double) 4000.0f);
		if (aBonus > 22000)
			aBonus = 22000;
	}
	else if (aTime <= 0x258 && m1a30 <= 4 && m1a50 >= 7)
	{
		m1a30 = 5;
		aBonus = (int) ((double) m940 * (double) 5000.0f);
		if (aBonus > 25000)
			aBonus = 25000;
	}
	else if (aTime <= 0x2bc && m1a30 <= 5 && m1a50 >= 8)
	{
		m1a30 = 6;
		aBonus = (int) ((double) m940 * (double) 6000.0f);
		if (aBonus > 30000)
			aBonus = 30000;
	}

	aBonus = aBonus - aBonus % 50;
	if (aBonus > 0)
	{
		FloatText* aText = new FloatText(StrFormat(GTC(0x5652b0), aBonus), FONT_POINTS, 300.0f, (float) (m1a30 * 40 + 0xe6), false);
		m140.push_back(aText);
		AddScore(aBonus, true);
		aBonus = 0;
	}

	if (m1a2c > 0x2bc)
	{
		m8f8 = false;
		m1a30 = 0;
		m1a2c = 0;
		m1a50 = 0;
	}
}

// 0x474de0: the freeze effect. While the freeze lasts (+0x19fc updates, counting down once the game runs) a snowflake
// is added every 25 updates (up to 60) at one edge of the top of the screen, drifting across at one pixel an update; the
// effect ends with event 0x28 (0).
void Board::UpdateFreeze()
{
	if (m19fc <= 0)
		return;

	if (m8fc)
		m19fc--;

	if (mUpdateCnt % 0x19 == 0)
	{
		if (m240.size() < 0x3c)
		{
			FreezeFlake aFlake;
			aFlake.mAnim.SetNumFrames(IMAGE_PARTICLE_SNOWFLAKE->mNumCols);
			aFlake.mAnim.SetDelay(((int) AstroRand() % 500000000) % 5 + 7);
			aFlake.mAnim.SetStopped(false);
			aFlake.mVelY = 0.0f;
			if ((m240.size() & 1) == 0)
			{
				aFlake.mVelX = -1.0f;
				aFlake.mX = 640.0f;
				aFlake.mY = (float) (((int) AstroRand() % 500000000) % 5 + 7);
			}
			else
			{
				aFlake.mVelX = 1.0f;
				aFlake.mX = 105.0f;
				aFlake.mY = (float) (((int) AstroRand() % 500000000) % 5 + 7);
			}
			m240.push_back(aFlake);
		}
	}

	for (unsigned int i = 0; i < m240.size(); i++)
	{
		m240[i].mX = m240[i].mVelX + m240[i].mX;
		if (m240[i].mX > 640.0f)
			m240[i].mX = 105.0f;
		else if (m240[i].mX < 105.0f)
			m240[i].mX = 640.0f;
		m240[i].mAnim.Update();
	}

	if (m19fc == 0)
		Fire(0x28, 0);
}

// 0x475040: the per-update timers of the game (see the file's header). While the warp cover is not up (+0x1924 < 1) the
// pace timers, the level picture, the fade between the demo and the game, the choice of the next piece pace and the
// side panels step; then the alerts, the start banner and the freeze run, and while the game is running the minute
// banner, the stage, the clear count-down and the other state machines.
void Board::UpdateStates()
{
	if (m1924 < 1)
	{
		if (m1abc > 0)
			m1abc--;
		if (m1ac0 > 0)
			m1ac0--;

		if (m8de)
		{
			m778->Update();
			if (m778->m08 == 0)
				m8de = false;
		}

		mAnim560.Update();

		if (!m8eb && !m8ed && !m8ef && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0 && m19fc < 1)
		{
			if (m1b5c > 0)
			{
				m1b5c--;
				if (m1b5c < 1)
					m1b5c = 0;
			}
		}

		if (m1960 > 0)
		{
			m1960 -= 4;
			if (m1960 < 2)
				m1960 = -1;
		}

		if (!m8d8 || !m8dc)
		{
			if (m8d9)
			{
				m978 -= 6;
				if (m978 < 1)
				{
					m978 = 0;
					m8d9 = false;
				}
				mOverlay.SetFill(0xff, 0xff, 0xff, m978);
			}
		}
		else
		{
			m978 += 6;
			if (m978 > 0xfe)
			{
				for (unsigned int i = 0; i < m140.size(); i++)
					delete m140[i];
				m140.clear();
				m978 = 0xff;
				m8dc = false;
				m8d8 = false;
				ClearField();
				ResetState();
				ResetGame();
				LoadLevel(true);
				m8d9 = true;
			}
			mOverlay.SetFill(0xff, 0xff, 0xff, m978);
		}

		if (m1a34 == 1)
		{
			if (m1ac0 < 1 && m1ab8 == 7 && (mMode == 0 || mMode == 3))
			{
				m1ac0 = 0;
				m1ab8 = 6;
			}
			else if (m1a34 == 1 && m1ac0 < 1 && (mMode == 1 || mMode == 2))
			{
				m1ab8++;
				if (m1ab8 > 7)
					m1ab8 = 6;
				m1ac0 = PickSpawnDelay();
			}
		}
		else if (m1ac0 < 1)
		{
			int aPace = m1ab8;
			if (aPace < 2 || aPace > 5)
			{
				if (aPace >= 8 && aPace <= 9)
					m1ab8 = (aPace == 8) + 8;
				else
				{
					if (aPace >= 10 && aPace <= 0xd)
					{
						aPace++;
						if (aPace > 0xd)
							aPace = 10;
						if (m1abc < 1)
						{
							m1abc = 0;
							aPace = m4c8 ? 4 : 8;
						}
					}
					m1ab8 = aPace;
				}
			}
			else
			{
				int aNext;
				do
				{
					int aDraw = ((int) AstroRand() % 500000000) % 5;
					if (aDraw < 2)
						aNext = 2;
					else if (aDraw == 2)
						aNext = 3;
					else
						aNext = (aDraw != 3) + 4;
				} while (aNext == m1ab8);
				m1ab8 = aNext;
			}
			m1ac0 = PickSpawnDelay();
		}

		if (m1bd8 == m1bdc && m1a34 == 0 && m1b48 < 0)
		{
			m1b48 += 2;
			if (m1b48 >= 0)
				m1b48 = 0;
		}
		else if (m1a34 == 1 && m1b48 > -IMAGE_SUPA_BUTTON_ALERT->mWidth)
		{
			m1b48 -= 3;
			if (m1b48 <= -IMAGE_SUPA_BUTTON_ALERT->mWidth)
				m1b48 = -IMAGE_SUPA_BUTTON_ALERT->mWidth;
		}

		if (m1afc > 0)
		{
			m1afc--;
			if (m1afc <= 0)
			{
				m1b00 = 0;
				m1afc = 0;
			}
			else if (m1afc % 0x14 == 0)
				m924 = !m924;
		}

		if (m1b18 > 0)
		{
			m1b18--;
			if (m1b18 <= 0)
			{
				m1b18 = 0;
				PayComboBonus();
			}
		}
	}

	UpdateAlertFlash();

	bool aCalm = !m8eb && !m8ed && !m8ef && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0;
	if (aCalm)
		UpdateFreeze();

	if (m8eb || m8ed || m8ef || m8f4 || m8ec || (!mFlag8E9 && !mFlag8E8 && m1924 == 0))
		UpdateStartBanner();

	if (m8eb || m8ed || m8ef || m8f4 || m8ec || mFlag8E9 || mFlag8E8 || m1924 != 0 || !m8fc)
		goto Finish;

	if (!m910 && !m917)
	{
		if (m1a60 == 1 || (mMode != 3 && mMode != 0) || m1a34 != 1 || m91b || m91c)
		{
			m19b4++;
			if (m19b4 > 0 && m19b4 % 6000 == 0 && m1a60 == 2)
			{
				m8de = true;
				delete m778;

				int aMinutes = m19b4 / 6000;
				std::string aText;
				if (aMinutes > 1)
					aText = StrFormat(GTC(0x56640c), aMinutes);
				else
					aText = GT(0x566400);

				MemoryImage* anImage = new MemoryImage(mApp);
				Font* aFont = GetFontById(0xb2);
				anImage->SetImageMode(true, true);
				anImage->Create(aFont->StringWidth(aText), aFont->GetHeight());

				Graphics g(anImage);
				g.SetColor(Color(0xff, 0xff, 0xff));
				g.SetFont(aFont);
				g.DrawString(aText, 0, aFont->GetAscent());

				m778 = new LevelUpText(anImage, 0);
				m778->m04 = true;
				mApp->PlaySample(SOUND_WORDSWOOSH);
			}
		}
	}

	m19b8++;
	UpdateStageBackground();
	UpdateSpecialMode();
	UpdateIntroAndTally();

	if (m1b64 == 1)
	{
		int aBefore = m1b60;
		m1b60 = aBefore + 8;
		if (aBefore + 8 > 0xfe)
		{
			m1b64 = 2;
			m1b60 = 0xff;
		}
	}
	else if (m1b64 == 2 && (m1b68--, m1b68 < 1))
	{
		m1b68 = 0;
		m1b64 = 3;
	}
	else if (m1b64 == 3)
	{
		int aBefore = m1b60;
		m1b60 = aBefore - 3;
		if (aBefore - 3 < 1)
		{
			m1b64 = 0;
			m1b60 = 0;
		}
	}

	if (m1a34 == 0 && m1bbc <= mField193C && m8fc && m19fc < 1 && (m1b08--, m1b08 < 1))
	{
		int aHalf = m1b0c / 2;
		m1b0c = aHalf;
		if (aHalf < 300)
			m1b0c = 300;
		m1b08 = m1b0c;
		m1934++;
		if (m1ab4 < 1)
			m1ab4 = 100;
	}

	if (!m270.empty())
	{
		m1b04 = (m1b04 + 4) & 0xff;
		for (unsigned int i = 0; i < m270.size(); i++)
		{
			m270[i].mX++;
			if (m270[i].mX >= m270[i].mFont->StringWidth(m270[i].mText) + 0x29e)
			{
				int aMin = 500000000;
				for (unsigned int j = 0; j < m270.size(); j++)
				{
					if (m270[j].mX < aMin)
						aMin = m270[j].mX;
				}
				m270[i].mX = aMin - m270[i].mFont->StringWidth(m270[i].mText) - 0x1e;
			}
		}
	}

	for (unsigned int i = 0; i < m1f0.size(); i++)
	{
		if (m1f0[i] > 0)
			m1f0[i]--;
	}

	if (mApp->mFlag658)
	{
		if (m1a5c > 0)
			m1a58++;
		if (m1a58 == 1000)
			mApp->MakeDialog(mMode + 2, false, 0x6c, -1, -1, 3, GT(0x56633c), GT(0x566348), GT(0x562024), std::string(), true);
	}

Finish:
	if (m1928 > 0)
	{
		MarkDirty();
		m1928 -= 0x28;
		if (m1928 < 0)
			m1928 = 0;
	}

	if (m8fc)
	{
		for (unsigned int i = 0; i < m200.size(); )
		{
			if (m200[i].mCount < 1)
			{
				i++;
				continue;
			}
			m200[i].mCount--;
			if (m200[i].mCount > 0)
			{
				if (m200[i].mCount < 0xff)
					Fire(0x25, m200[i].mCount);
				i++;
			}
			else
			{
				int aKind = -m200[i].mType;
				if (aKind == 0)
					aKind = -500000000;
				Fire(0x20, aKind);
				m200.erase(m200.begin() + i);
			}
		}
	}

	if (m8f8)
	{
		m1a2c++;
		PayTimeBonus();
	}
}
