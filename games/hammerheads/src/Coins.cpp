#include "Coins.h"

#include <cstdio>
#include <string>

#include "Application.h"
#include "Color.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Hearts.h"
#include "Image.h"
#include "Rect.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

static ObjectRef gCoinsRef;		// 0x597fa0

// 0x444390
Coins::Coins() :
	Object(0x80)
{
}

// 0x444010
Coins::~Coins()
{
}

// 0x443e80
Coins* Sexy::GetCoins()
{
	return dynamic_cast<Coins*>(gCoinsRef.Get());
}

// 0x444390
Coins* Sexy::MakeCoins()
{
	Coins* aCoins = new Coins();
	aCoins->SetName("");
	gCoinsRef = ObjectRef(aCoins);
	return aCoins;
}

// 0x443eb0
int Sexy::GetCoinsX()
{
	if (gApp->IsShopOpen())
		return 0x276;

	if (GetStats()->Get(STAT_HEALTH_CAPACITY) == 7.0 && GetStats()->GetInt(STAT_COIN_BALANCE) > 9999)
		return IMAGE_INGAMEUI_HEARTS->GetCelRect(0).mWidth / 2 + 0x2a6;
	return 0x2a9;
}

// 0x444040
void Sexy::GetCoinsPos(double* thePos)
{
	thePos[0] = (double)(GetCoinsX() - IMAGE_INGAMEUI_COIN->mWidth);
	thePos[1] = 29.0;
}

// 0x443a60: the same as the challenge meter's (the time of each flight runs down, and one that has run out is taken away; the
// entry that moves up into its place is left until the following step, as in the original)
void Coins::Update(float theFrac)
{
	Object::Update(theFrac);

	for (int i = 0; i < (int)mFlights.size(); i++)
	{
		if (mFlights[i].mTime > 0.0)
		{
			mFlights[i].mTime = mFlights[i].mTime - (double)theFrac * 0.01;
			if (mFlights[i].mTime <= 0.0)
			{
				mFlights[i].mTime = 0.0;
				mFlights.erase(mFlights.begin() + i);
			}
		}
	}
}

// 0x444150
void Coins::AddCoins(int theAmount)
{
	CoinFlight aFlight = { GetStats()->GetInt(STAT_COIN_BALANCE), 0.75 };
	mFlights.push_back(aFlight);
	GetStats()->AddInt(STAT_COIN_BALANCE, theAmount);
}

// 0x4441a0
void Coins::Draw(Graphics* g)
{
	Graphics aG(*g);
	if (mDead)
		return;

	aG.SetColor(Color(255, 255, 255, 255));
	aG.SetFont(FONT_PTBANANASPLIT27INGAMEUI);

	int aShownCoins = GetStats()->GetInt(STAT_COIN_BALANCE);
	if (!mFlights.empty())
		aShownCoins = mFlights[0].mCoins;

	char aBuffer[32];
	snprintf(aBuffer, sizeof(aBuffer), "%d", aShownCoins);
	std::string aText = aBuffer;

	int aBoxWidth = aG.GetFont()->StringWidth(aText) + 0x14;
	if (aBoxWidth < 0x3a)
		aBoxWidth = 0x3a;
	aG.DrawImageBox(Rect(GetCoinsX() + (10 - aBoxWidth), 8, aBoxWidth, IMAGE_SCOREBACK->mHeight), IMAGE_SCOREBACK);
	aG.DrawImage(IMAGE_INGAMEUI_COIN, GetCoinsX() - IMAGE_INGAMEUI_COIN->mWidth, 0x1d);
	aG.DrawString(aText, GetCoinsX() - aG.GetFont()->StringWidth(aText), 0x23);
}
