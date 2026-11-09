#include "AdventurePanel.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "Common.h"
#include "compat2004/Compat2004.h"
#include "Res.h"

using namespace Sexy;

// 0x450df0
AdventurePanel::AdventurePanel(WinWSApp* theApp) :
	TabPanel(theApp)
{
	mUser = NULL;
	mTitle = GT(0x005308cc);
}

// 0x444fa0 slot 16: nothing is drawn while the page is hidden. The heading uses the graphics' current colour; the
// statistics are a black label and a gold value each, one line under the other from y = 0x5f (a font height each), and
// the expeditions line (colour codes) six pixels lower. The medal is stretched to 0x42 x 0x5a at (0x150, 0x10e): bronze
// for 1 or 2 finished games, silver for 3 or 4, gold for more (none for 0). On the right the treasure picture, the six
// gems' counts in two columns, and the four secret items in two columns: the item and its count, or a question mark
// when the count is 0
void AdventurePanel::Draw(Graphics* g)
{
	if (mShown)
	{
		ImageFont* aBodyFont = mApp->mFontMyriadCondensed14Bold;
		g->SetFont(mApp->mFontRockwell13Bold);
		g->DrawString(GT(0x0052f8c0), 0x3c, 0x23);

		g->SetFont(aBodyFont);
		g->SetColor(Color(0, 0, 0));
		g->DrawString(GT(0x0052f7f8), 0x23, 0x5f);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		SexyString aValue = StrFormat(GTC(0x0052b19c), mUser->mSharksKilled);
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f7f8)) + 0x23, 0x5f);

		g->SetColor(Color(0, 0, 0));
		int aY = aBodyFont->GetHeight() + 0x5f;
		g->DrawString(GT(0x0052f7dc), 0x23, aY);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		aValue = StrFormat(GTC(0x0052b19c), mUser->mPiranhasKilled);
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f7dc)) + 0x23, aY);

		g->SetColor(Color(0, 0, 0));
		aY = aY + aBodyFont->GetHeight();
		g->DrawString(GT(0x0052f7c0), 0x23, aY);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		aValue = StrFormat(GTC(0x0052b19c), mUser->mBossesKilled);
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f7c0)) + 0x23, aY);

		g->SetColor(Color(0, 0, 0));
		aY = aY + aBodyFont->GetHeight();
		g->DrawString(GT(0x0052f89c), 0x23, aY);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		aValue = StrFormat(GTC(0x0052b19c), mUser->mBestTreasureWords);
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f89c)) + 0x23, aY);

		g->SetColor(Color(0, 0, 0));
		aY = aY + aBodyFont->GetHeight();
		g->DrawString(GT(0x0052f88c), 0x23, aY);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		aValue = StrFormat(GTC(0x0052b19c), mUser->GetBestScore());
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f88c)) + 0x23, aY);

		int aExpeditionsY = aY + aBodyFont->GetHeight() + 6;
		g->SetColor(Color(0, 0, 0));
		WriteString2004(g, StrFormat(GTC(0x0052f858), mApp->mUser->mGamesCompleted), 0x23, aExpeditionsY, -1, -1, true, 0, -1);

		int aGames = mApp->mUser->mGamesCompleted;
		if (aGames != 0)
		{
			Image* aMedal;
			if (aGames <= 2)
				aMedal = mApp->mImages[IMAGE_MEDAL_BRONZE];
			else if (aGames <= 4)
				aMedal = mApp->mImages[IMAGE_MEDAL_SILVER];
			else
				aMedal = mApp->mImages[IMAGE_MEDAL_GOLD];
			if (aMedal != NULL)
			{
				Rect aSrcRect(0, 0, aMedal->mWidth, aMedal->mHeight);
				Rect aDestRect(0x150, 0x10e, 0x42, 0x5a);
				g->DrawImage(aMedal, aDestRect, aSrcRect);
			}
		}

		g->DrawImage(mApp->mImages[IMAGE_TAB_PANEL_TREASURE], 0x198, 0x17);

		g->SetColor(Color(0, 0, 0));
		WriteString2004(g, GT(0x0052f84c), 0x1a3, 0x2a, 0xc8, 0, true, 0, -1);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->mGemsFound[0]), 0x1db, 0x5a);
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->mGemsFound[1]), 0x23e, 0x5a);
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->mGemsFound[2]), 0x1db, 0x8e);
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->mGemsFound[3]), 0x23e, 0x8e);
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->mGemsFound[4]), 0x1db, 0xc3);
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->mGemsFound[5]), 0x23e, 0xc3);

		g->SetColor(Color(0, 0, 0));
		WriteString2004(g, GT(0x0052f838), 0x1a3, 0xf6, 0xc8, 0, true, 0, -1);
		g->SetColor(Color(0xa6, 0x88, 0x56));

		if (mUser->mSecretsFound[0] == 0)
			g->DrawImage(mApp->mImages[IMAGE_TAB_PANEL_QUESTION], 0x1a9, 0x10b);
		else
		{
			g->DrawImage(mApp->mImages[IMAGE_SECRET_CROWN], 0x1a9, 0x10b);
			g->DrawString(StrFormat(GTC(0x0052e144), mUser->mSecretsFound[0]), 0x1dc, 0x124);
		}

		if (mUser->mSecretsFound[1] == 0)
			g->DrawImage(mApp->mImages[IMAGE_TAB_PANEL_QUESTION], 0x1a9, 0x13d);
		else
		{
			g->DrawImage(mApp->mImages[IMAGE_SECRET_FIGURINE], 0x1a9, 0x13d);
			g->DrawString(StrFormat(GTC(0x0052e144), mUser->mSecretsFound[1]), 0x1dc, 0x156);
		}

		if (mUser->mSecretsFound[2] == 0)
			g->DrawImage(mApp->mImages[IMAGE_TAB_PANEL_QUESTION], 0x20c, 0x10b);
		else
		{
			g->DrawImage(mApp->mImages[IMAGE_SECRET_NECKLACE], 0x20c, 0x10b);
			g->DrawString(StrFormat(GTC(0x0052e144), mUser->mSecretsFound[2]), 0x23f, 0x124);
		}

		if (mUser->mSecretsFound[3] == 0)
			g->DrawImage(mApp->mImages[IMAGE_TAB_PANEL_QUESTION], 0x20c, 0x13d);
		else
		{
			g->DrawImage(mApp->mImages[IMAGE_SECRET_SCEPTER], 0x20c, 0x13d);
			g->DrawString(StrFormat(GTC(0x0052e144), mUser->mSecretsFound[3]), 0x23f, 0x156);
		}
	}
}
