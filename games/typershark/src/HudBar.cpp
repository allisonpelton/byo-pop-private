#include "HudBar.h"
#include "Board.h"
#include "Player.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Common.h"

using namespace Sexy;

// 0x441730: the flash has not started (step 0, rising, alpha 0); the font is the app's Rockwell 13
HudBar::HudBar(Player* thePlayer, WinWSApp* theApp)
{
	mPlayer = thePlayer;
	mApp = theApp;
	mField18 = 0;
	mFont = theApp->mFontRockwell13;
	mFlashStep = 0;
	mFadingUp = true;
	mAlpha = 0;
}

// 0x441760: while the player's extra-life flag is set the alpha of "EXTRA LIFE!" rises by 2 a tick to 255 and falls by 2
// to 0; every time it is back at 0 the step grows, and at the 4th the flag is cleared and the flash starts over. The board
// is marked dirty on every tick of the flash
void HudBar::Update()
{
	if (mPlayer->mFlag13d)
	{
		if (mFadingUp)
		{
			mAlpha += 2;
			if (mAlpha >= 0xff)
			{
				mAlpha = 0xff;
				mFadingUp = false;
			}
		}
		else
		{
			mAlpha -= 2;
			if (mAlpha <= 0)
			{
				mAlpha = 0;
				mFadingUp = true;
				mFlashStep++;
				if (mFlashStep >= 4)
				{
					mPlayer->mFlag13d = false;
					mFlashStep = 0;
					mAlpha = 0;
					mFadingUp = true;
				}
			}
		}
		mApp->mBoard->MarkDirty();
	}
}

// 0x449910: the band (black, alpha 128) over the top 25 rows, the lives (or "Game Over!") at the right in grey, the level
// (the depth in an abyss game, where the difficulty is -1) and the points at the left; the "^FFFFFF^" and "^C8C8C8^"
// parts of the format are the colours that Widget::WriteString reads. Then the green "EXTRA LIFE!" to the right of the
// level text while the flag is set (the width of the plain "LEVEL: %d     %d Points" text decides the place)
void HudBar::Draw(Graphics* g)
{
	std::string aText;

	g->SetColor(Color(0, 0, 0, 0x80));
	g->FillRect(0, 0, mApp->mWidth, 0x19);
	g->SetFont(mFont);
	g->SetColor(Color(0xc8, 0xc8, 0xc8));

	if (mPlayer->mField110 < 0)
		aText = GT(0x0052fda8);			// "Game Over!"
	else
		aText = StrFormat(GTC(0x0052fd98), mPlayer->mField110);			// "Lives left: %d"

	Board* aBoard = mApp->mBoard;
	aBoard->WriteString(g, aText, -5, mFont->GetAscent() + 2, mApp->mWidth, 1, true, 0, -1);

	g->SetColor(Color(0xff, 0xff, 0xff));

	std::string aLevelText;
	if (mApp->mDifficulty != -1)
		aLevelText = StrFormat(GTC(0x0052fd60), mPlayer->mField114 + 1, mPlayer->mScore);			// "^FFFFFF^LEVEL: ^C8C8C8^%d     ^FFFFFF^%d ^C8C8C8^Points"
	else
		aLevelText = StrFormat(GTC(0x0052fcf8), mApp->mBoard->mScore, mPlayer->mScore);			// "^FFFFFF^LEVEL: ^C8C8C8^Abyss     ^C8C8C8^Depth: ^FFFFFF^%d ^C8C8C8^feet     ^FFFFFF^%d ^C8C8C8^Points"
	aText = aLevelText;

	aBoard = mApp->mBoard;
	aBoard->WriteString(g, aText, 2, mFont->GetAscent() + 2, -1, -1, true, 0, -1);

	if (mPlayer->mFlag13d)
	{
		std::string aPlainText = StrFormat(GTC(0x0052fcdc), mPlayer->mField114 + 1, mPlayer->mScore);			// "LEVEL: %d     %d Points"
		g->SetColor(Color(0, 0xff, 0, mAlpha));
		std::string anExtraLife = GT(0x0052fcd0);			// "EXTRA LIFE!"
		int aY = mFont->GetAscent() + 2;
		g->DrawString(anExtraLife, mFont->StringWidth(aPlainText) + 0x32, aY);
	}
}
