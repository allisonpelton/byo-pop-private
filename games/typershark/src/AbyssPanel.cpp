#include "AbyssPanel.h"
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

// 0x450d40
AbyssPanel::AbyssPanel(WinWSApp* theApp) :
	TabPanel(theApp)
{
	mUser = NULL;
	mTitle = GT(0x005307f4);
}

// 0x444730 slot 16: nothing is drawn while the page is hidden. The heading uses the graphics' current colour (the
// original sets none before it); then the statistics, a black label and a gold value each, one line under the other
// from y = 0x9a (a font height each); the highest score is written with colour codes; then the boss picture and the
// four bosses' kill counts beside it. The deepest depth is the profile's, the highest score is the app's current user's
void AbyssPanel::Draw(Graphics* g)
{
	if (mShown)
	{
		ImageFont* aBodyFont = mApp->mFontMyriadCondensed14Bold;
		g->SetFont(mApp->mFontRockwell13Bold);
		g->DrawString(GT(0x0052f824), 0x3c, 0x50);

		g->SetFont(aBodyFont);
		g->SetColor(Color(0, 0, 0));
		g->DrawString(GT(0x0052f814), 0x23, 0x9a);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		SexyString aValue = StrFormat(GTC(0x0052b19c), mUser->mBestDepth);
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f814)) + 0x23, 0x9a);

		g->SetColor(Color(0, 0, 0));
		int aY = aBodyFont->GetHeight() + 0x9a;
		g->DrawString(GT(0x0052f7f8), 0x23, aY);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		aValue = StrFormat(GTC(0x0052b19c), mUser->mAbyssSharksKilled);
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f7f8)) + 0x23, aY);

		g->SetColor(Color(0, 0, 0));
		aY = aY + aBodyFont->GetHeight();
		g->DrawString(GT(0x0052f7dc), 0x23, aY);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		aValue = StrFormat(GTC(0x0052b19c), mUser->mAbyssPiranhasKilled);
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f7dc)) + 0x23, aY);

		aY = aY + aBodyFont->GetHeight();
		g->SetColor(Color(0, 0, 0));
		g->DrawString(GT(0x0052f7c0), 0x23, aY);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		int aBosses = mUser->GetKillCount(0x10);
		aBosses = aBosses + mUser->GetKillCount(0xf);
		aBosses = aBosses + mUser->GetKillCount(0xe);
		aBosses = aBosses + mUser->GetKillCount(0xc);
		aValue = StrFormat(GTC(0x0052b19c), aBosses);
		g->DrawString(aValue, aBodyFont->StringWidth(GT(0x0052f7c0)) + 0x23, aY);

		aY = aY + aBodyFont->GetHeight();
		g->SetColor(Color(0, 0, 0));
		WriteString2004(g, StrFormat(GTC(0x0052f794), mApp->mUser->mGroups[5].mBestScore), 0x23, aY, -1, -1, true, 0, -1);

		g->DrawImage(mApp->mImages[IMAGE_ABYSS_BOSS], 0x144, 0x18);

		g->SetFont(mApp->mFontMyriadCondensed14Bold);
		g->SetColor(Color(0xa6, 0x88, 0x56));
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->GetKillCount(0xc)), 0x1ac, 0xac);
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->GetKillCount(0xe)), 0x236, 0xac);
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->GetKillCount(0x10)), 0x236, 0x14e);
		g->DrawString(StrFormat(GTC(0x0052e144), mUser->GetKillCount(0xf)), 0x1ac, 0x14e);
	}
}
