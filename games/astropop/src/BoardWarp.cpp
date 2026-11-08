// Board: the warp between two stages and the level-up panel (Fn46a810, 0x46a810: a long state machine that runs
// instead of the rest of the update) and the streaks of the warp charge-up (0x439650).

#include "Board.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "Image.h"
#include "Particle.h"
#include "SoundInstance.h"
#include "ResourceManager.h"
#include "WidgetManager.h"
#include "Res.h"

using namespace Sexy;

// 0x439650: the streaks of the warp charge-up grow and speed up (the 100 are stepped in groups of ten)
void Board::UpdateStreaks()
{
	unsigned int aCount = (unsigned int) m170.size();
	if (aCount > 100)
		aCount = 100;
	for (unsigned int i = 0; i < aCount; i++)
	{
		StreakRec* aStreak = &m170[i];
		aStreak->mMargin += 3;
		aStreak->mY += 3.0f;
		if (aStreak->mMargin > 0x1e0)
			aStreak->mMargin = 0x1e0;
		aStreak->mSpeed += m917 ? 0.06f : 0.03f;
	}
}

// 0x46a810: true ends the update
bool Board::Fn46a810()
{
	mAnim460.Update();
	mAnim5e0.Update();
	if (mUpdateCnt % 5 == 0)
	{
		m1b14++;
		if (m1b14 >= IMAGE_CP_WARP_OVERLAY->mNumCols)
			m1b14 = 0;
	}

	// the level-up panel types its lines and the arms of the upgrade screen slide in
	if ((m8ed || m8ef) && ((m1b2c != 2 && m1b2c != 4) || m91a))
	{
		if (!m8f4 && m1a60 == 1 && m7e8 < 0)
		{
			m7e8 += 2;
			if (m7e8 >= 0)
			{
				m7e8 = 2;
				int aBest = 0;
				mApp->mUser->GetInt(0x71, &aBest);
				if (aBest < m1bd4)
				{
					m91f = false;
					m1b44 = m921 ? 1 : 100;
					mAnim5e0.SetStopped(false);
					mApp->mUser->SetInt(0x71, m1bd4);
					mApp->mUser->Save();
				}
			}
		}
		if (m7e8 >= 0 && m1a60 == 1)
		{
			m7e8 = 0;
			m90f = false;
			if (m7f0 < 0)
				m7f0 = 0;
			int aCnt = mUpdateCnt;
			if (aCnt % m800 == 0 && (unsigned int) m7e0 >= (unsigned int) m7cc.size())
				m804 = !m804;
			if (aCnt % m7dc == 0 && (unsigned int) m7e0 < (unsigned int) m7cc.size())
			{
				mApp->PlaySample(SOUND_COMPUTER_TEXT);
				m7e4++;
				if ((unsigned int) m7e4 >= (unsigned int) m7cc[m7e0].size())
				{
					m7e4 = 0;
					m7e0++;
				}
			}
			if (mUpdateCnt % m7fc == 0)
			{
				int aPulse = m7f4;
				if (aPulse < m7f8 && m7f0 == 0)
					m7f4 = aPulse + 1;
				else
				{
					m7f0++;
					if (m7f0 + aPulse >= 0xbb)
					{
						m7f4 = aPulse - 1;
						if (m7f4 <= 0)
						{
							m7f4 = 0;
							m7f0 = 0;
						}
					}
				}
			}
		}
		if (m1b2c == 2 || m1b2c == 4)
		{
			m820 -= 4;
			if (m820 < -IMAGE_UPGRADE_ARM->mWidth)
				m820 = -IMAGE_UPGRADE_ARM->mWidth;
			m87c += 4;
			if (m87c > IMAGE_UPGRADE_HEAD->GetCelWidth() + 0x280)
				m87c = IMAGE_UPGRADE_HEAD->GetCelWidth() + 0x280;
		}
	}

	// the panel slides away
	if (m8f4 || (m7e8 < 0 && m7e8 > -m7ec && !m8ed && !m8ef))
	{
		m7e8 -= 2;
		if (m7e8 <= -m7ec)
		{
			m7e8 = -m7ec;
			ShowThemeButtons();
		}
	}

	// the warp stars fall
	for (unsigned int i = 0; i < m150.size(); i++)
	{
		WarpStar* aStar = m150[i];
		aStar->mAnim.Update();
		aStar->mY += aStar->mSpeed;
		if ((float) (mHeight + 0x32) < aStar->mY)
			aStar->mY = (float) (((int) AstroRand() % 500000000) % 0x32 - 100);
	}

	if (m8ec)
	{
		// the charge-up: the screen shakes, then the warp starts
		m199c = ((int) AstroRand() % 500000000) % 0xb - 5;
		m19a4 += 2;
		if (m19a4 > 0xff)
		{
			m19a4 = 0xff;
			m8eb = true;
			m8ed = false;
			m8ec = false;
			if (mApp->mHyperspaceLoop != NULL)
				mApp->mHyperspaceLoop->Play(true, false);
			if (mApp->mEngineLoop != NULL)
				mApp->mEngineLoop->Stop();
			Fire(9);
		}
		if (m19a4 == 0x96)
			mApp->PlaySample(SOUND_WARP);
		UpdateStreaks();
		return false;
	}

	if (!m8eb)
		return false;

	if (!m8ed)
	{
		if (!m8ef && !m8f4)
		{
			// the screen fades in white while the field flies away
			if (mUpdateCnt % m1998 == 0 && m3d0.mAlpha >= 10 && m1954 < 7)
			{
				m1954++;
				if (m1954 >= 5)
				{
					mOverlay.SetFill(255, 255, 255, 255);
					for (unsigned int i = 0; i < m140.size(); i++)
						delete m140[i];
					m140.clear();
				}
			}
			m3d0.mAlpha++;
			if (m3d0.mAlpha > 0x28)
			{
				m3d0.mAlpha = 0xff;
				m8de = false;
				mOverlay.SetFill(m3d0.mRed, m3d0.mGreen, m3d0.mBlue, m3d0.mAlpha);
				m8ed = true;
				m8f0 = true;
				m1954 = 2;
				for (unsigned int i = 0; i < 100 && i < m170.size(); i++)
					m170[i].mFlag28 = false;
				m8fa = true;
				mApp->PlaySample(SOUND_WARP_ENGAGED);
				ClearField();
				m938 = -313.0f;
				for (int i = 0; i < 15; i++)
				{
					WarpStar* aStar = new WarpStar();
					aStar->mX = 0;
					aStar->mY = 0;
					aStar->mField5C = 0;
					aStar->mSpeed = 0;
					aStar->mImage = NULL;
					if (i < 5)
						aStar->mImage = IMAGE_WARP_FX_LARGE;
					else if (i < 10)
						aStar->mImage = IMAGE_WARP_FX_MED;
					else
						aStar->mImage = IMAGE_WARP_FX_SMALL;
					aStar->mAnim.SetStopped(false);
					aStar->mAnim.SetDelay(7);
					aStar->mAnim.SetNumFrames(aStar->mImage->mNumCols);
					int aDraw = (int) AstroRand() % 500000000;
					if (aDraw % 2 == 0)
						aStar->mX = (float) (((int) AstroRand() % 500000000) % (mWidth - 0x226) + 0x226);
					else
						aStar->mX = (float) (((int) AstroRand() % 500000000) % 0xdc);
					aStar->mY = (float) (((int) AstroRand() % 500000000) % 0x32 - 100);
					aStar->mField5C = 0;
					aStar->mSpeed = (float) ((double) (((int) AstroRand() % 500000000) % 0x51) * (double) 0.01f + (double) 0.4f);
					m150.push_back(aStar);
				}
				if (m1a0c < 4)
					m198c = IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2 - 0x22 + m1a70;
				else
					m198c = 0x159;
				if (m1b2c == 2 || m1b2c == 4)
				{
					m938 = 423.0f;
					mApp->Fn412730();
					mApp->mWidgetManager->BringToFront(&mOverlay);
				}
			}
			m19a8 += m19ac;
			if (m19a8 > 0xff)
				m19a8 = 0xff;
			return false;
		}
		else if (!m8ef)
		{
			if (m8f4)
			{
				// the screen fades to white and the next stage begins
				m3d0.mRed = 0xff;
				m3d0.mGreen = 0xff;
				m3d0.mBlue = 0xff;
				m3d0.mAlpha += 6;
				if (m3d0.mAlpha > 0xff)
				{
					m3d0.mAlpha = 0xff;
					m3d0.mGreen = 0xff;
					m3d0.mBlue = 0xff;
					m3d0.mRed = 0xff;
					m8eb = false;
					m8f4 = false;
					if (mApp->mHyperspaceLoop != NULL)
						mApp->mHyperspaceLoop->Stop();
					if (mApp->mEngineLoop != NULL)
						mApp->mEngineLoop->Play(true, false);
					AstroRand();
					int aSceneryId = mField193C % 3 + 0x18b;
					std::string aName = GetStringIdById(m77c.mI30);
					mApp->mResourceManager->DeleteImage(aName);
					m77c.mImage = LoadImageById(mApp->mResourceManager, aSceneryId);
					m77c.mI30 = aSceneryId;
					m77c.mF24 = 0;
					m77c.mF28 = -1.0f;
					m77c.mCur[0] = 0;
					m77c.mCur[1] = (float) (m77c.mImage->mHeight - 0x1e0);
					m77c.mCur[2] = 640.0f;
					m77c.mCur[3] = 480.0f;
					m77c.mNext[0] = 0;
					m77c.mNext[1] = 0;
					m77c.mNext[2] = 640.0f;
					m77c.mNext[3] = 0;
					if (Fn468e70())
						return true;
					mApp->PlaySample(SOUND_GET_READY);
					LoadLevel(true);
					m7e8 = -m7ec;
				}
				mOverlay.SetFill(m3d0.mRed, m3d0.mGreen, m3d0.mBlue, m3d0.mAlpha);
			}
		}
		else
		{
			// the warp has come out: the gate lights up
			m199c = ((int) AstroRand() % 500000000) % 4 - 2;
			m19e0 += 2;
			if (m19e0 > 0xff)
				m19e0 = 0xff;
			if (!m8f1)
			{
				m19e4 -= 8;
				if (m19e4 < 0)
					m19e4 = 0;
			}
			else
			{
				m19e4 += 8;
				if (m19e4 > 0xff)
					m19e4 = 0xff;
			}
			if (!m8f2)
			{
				m19e8 -= 8;
				if (m19e8 < 0)
					m19e8 = 0;
			}
			else
			{
				m19e8 += 8;
				if (m19e8 > 0xff)
					m19e8 = 0xff;
			}
			if (!m8f3)
			{
				m19ec -= 8;
				if (m19ec < 0)
					m19ec = 0;
			}
			else
			{
				m19ec += 8;
				if (m19ec > 0xff)
					m19ec = 0xff;
			}
			if (m1b2c == 2 || m1b2c == 4)
			{
				if (!m91a && !m8f4)
				{
					if (m820 < m828)
					{
						m820 += 4;
						if (m820 >= m828)
							m820 = m828;
					}
					else
						mAnim83c.Update();
					if (m87c <= m884)
					{
						mAnim898.Update();
						return false;
					}
					m87c -= 4;
					if (m87c <= m884)
					{
						m87c = m884;
						return false;
					}
				}
			}
		}
	}
	else
	{
		// the flight through the warp: the screen darkens and shakes until the end of the tunnel
		// (x87, 53 bits: the step is not rounded to a float before it is used)
		double aStep = (m938 < 0.0f) ? 0.0 : (double) m938 * (double) 0.001f;
		double aNew = (1.5 - aStep) + (double) m938;
		m938 = (float) aNew;
		if (aNew > 423.0)
			m938 = 423.0f;
		m199c = ((int) AstroRand() % 500000000) % 4 - 2;
		if (m3d0.mAlpha > 0)
			m3d0.mAlpha--;
		if (m3d0.mBlue > 0)
			m3d0.mBlue--;
		if (m3d0.mGreen > 0)
			m3d0.mGreen--;
		mOverlay.SetFill(m3d0.mRed, m3d0.mGreen, m3d0.mBlue, m3d0.mAlpha);
		if (m938 >= 423.0f && m3d0.mAlpha == 0 && m3d0.mGreen == 0 && m3d0.mBlue == 0)
		{
			m938 = 423.0f;
			m8ef = true;
			m8ed = false;
			m19e0 = 0;
			m19ec = 0;
			m19e8 = 0;
			m19e4 = 0;
			return false;
		}
	}
	return false;
}
