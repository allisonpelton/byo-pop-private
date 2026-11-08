// StarField.cpp: the stars (STARFIELD) and the dots (NETWORK) that the two cheat words put behind the board
// (Sexy::StarField, 0x5d8ffc-0x5d9...). The order of the random draws is the original's: Sexy::Rand for places and the
// flash, the C runtime's rand for the speeds, colours and the dots' texts.

#include "StarField.h"
#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "Compat2004.h"
#include "Common.h"
#include "Graphics.h"
#include "Color.h"
#include "Font.h"
#include "Image.h"
#include "DDImage.h"
#include <string.h>

using namespace Sexy;

// the texts of the dots (the table at 0x66bb84)
static const char* const gDotTexts[8] =
{
	"101", "0010", "0101", "1101", "01101", "110010", "01001101", "11100101011"
};

// 0x5d8ffc
StarField::StarField()
{
	mTarget = 0;
	mImage = NULL;
	mNetwork = false;
}

StarField::~StarField()
{
}

// 0x5d9096: theWavy puts the board's wave over the nebula
void StarField::Draw(Graphics* g, bool theWavy)
{
	WinDMApp* aApp = gApp;
	if (!mNetwork)
	{
		if (mImage == NULL)
		{
			g->SetColor(Color(0));
			g->FillRect(0, 0, 640, 480);
		}
		else
		{
			int aWidth = mImage->mWidth;
			int aX = (aWidth - (aApp->mBoard->mUpdateCnt / 2) % aWidth) - 1;
			g->DrawImage(mImage, aX - aWidth, 0);
			g->DrawImage(mImage, aX, 0);
			g->DrawImage(mImage, aWidth + aX, 0);
			if (theWavy)
			{
				aApp->mBoard->DrawWavyImage(g, mImage, aX - mImage->mWidth, 0, 8.0f);
				aApp->mBoard->DrawWavyImage(g, mImage, aX, 0, 8.0f);
				aApp->mBoard->DrawWavyImage(g, mImage, mImage->mWidth + aX, 0, 8.0f);
			}
		}

		for (std::list<Star>::iterator anItr = mStars.begin(); anItr != mStars.end(); ++anItr)
		{
			g->SetColor(Color(anItr->mColor));
			g->FillRect((int)anItr->mX, (int)anItr->mY, 1, 1);
		}
	}
	else
	{
		g->SetColor(Color(0));
		int aHeight = aApp->ScaleNum(0x300, 0);
		int aWidth = aApp->ScaleNum(0x400, 0);
		g->FillRect(0, 0, aWidth, aHeight);

		for (std::list<Star>::iterator anItr = mStars.begin(); anItr != mStars.end(); ++anItr)
		{
			Color aColor(anItr->mColor);
			if (0.0f < anItr->mFlash && anItr->mFlash <= 0.5f)
				aColor.mAlpha = 0x80;

			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetFont(FONT_CONTINUUM_BOLD_14_NUM);
			const char* aText = gDotTexts[anItr->mText];
			int aY = (int)anItr->mY;
			while (*aText != '\0')
			{
				g->SetColor(aColor);
				g->DrawString(std::string(1, *aText), (int)anItr->mX, aY);
				aY += aApp->ScaleNum(0x14, 0);
				aText++;
			}
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
}

// 0x5d93ee: the speed and the colour of a new star come from the C runtime's rand
void StarField::AddStar(int theX, int theY)
{
	WinDMApp* aApp = gApp;
	mStars.push_back(Star());
	Star& aStar = mStars.back();
	memset(&aStar, 0, sizeof(aStar));

	aStar.mY = (float)theY;
	aStar.mX = (float)theX;
	aStar.mFlash = 0.0f;
	if (!mNetwork)
	{
		aStar.mVY = 0.0f;
		switch (CrtRand() % 3)
		{
		case 0:
			aStar.mVX = aApp->ScaleNum(-0.2f, 0.0f);
			aStar.mColor = 0x404040;
			break;
		case 1:
			aStar.mVX = aApp->ScaleNum(-0.6f, 0.0f);
			aStar.mColor = 0x909090;
			break;
		case 2:
			aStar.mVX = aApp->ScaleNum(-0.9f, 0.0f);
			aStar.mColor = 0xFFFFFF;
			break;
		}
	}
	else
	{
		aStar.mVX = 0.0f;
		aStar.mText = CrtRand() % 8;
		switch (CrtRand() % 5)
		{
		case 0:
			aStar.mVY = aApp->ScaleNum(1.0f, 0.0f);
			aStar.mColor = 0x404040;
			break;
		case 1:
			aStar.mVY = aApp->ScaleNum(1.2f, 0.0f);
			aStar.mColor = 0x606060;
			break;
		case 2:
			aStar.mVY = aApp->ScaleNum(1.5f, 0.0f);
			aStar.mColor = 0x909090;
			break;
		case 3:
			aStar.mVY = aApp->ScaleNum(1.8f, 0.0f);
			aStar.mColor = 0xC0C0C0;
			break;
		case 4:
			aStar.mVY = aApp->ScaleNum(2.2f, 0.0f);
			aStar.mColor = 0xFFFFFF;
			break;
		}
	}
}

// 0x5d95c7
void StarField::Init(int theCount, bool theNetwork)
{
	WinDMApp* aApp = gApp;
	mNetwork = theNetwork;
	mTarget = theCount;
	if (mImage == NULL && !theNetwork)
		mImage = aApp->GetImage("images/nebula1", true);

	mStars.clear();
	for (int aCount = theCount; aCount > 0; aCount--)
	{
		int aHeight = aApp->ScaleNum(0x300, 0);
		int aY = Rand() % aHeight;
		int aWidth = aApp->ScaleNum(0x400, 0);
		int aX = Rand() % aWidth;
		AddStar(aX, aY);
	}
}

// 0x5d968a: the stars drift left; the dots fall and flash. Those that leave are taken out and new ones come in at the
// edge they come from, up to the number that Init was given.
void StarField::Update()
{
	WinDMApp* aApp = gApp;
	std::list<Star>::iterator anItr = mStars.begin();
	while (anItr != mStars.end())
	{
		Star& aStar = *anItr;
		aStar.mX = aStar.mVX + aStar.mX;
		aStar.mY = aStar.mVY + aStar.mY;

		bool aErase;
		if (mNetwork)
		{
			if (aStar.mFlash == 0.0f && Rand() % 1000 == 0)
				aStar.mFlash = 1e-05f;

			if (aStar.mFlash != 0.0f)
			{
				aStar.mFlash = (float)((double)aStar.mFlash + (double)0.075f);
				if (aStar.mFlash >= 1.0f)
					aStar.mFlash = (Rand() % 100 < 0x50) ? 1e-05f : 0.0f;
			}
			aErase = 768.0f < aStar.mY;
		}
		else
		{
			aErase = !(0.0f < aStar.mX);
		}

		if (aErase)
			anItr = mStars.erase(anItr);
		else
			++anItr;
	}

	while ((int)mStars.size() < mTarget)
	{
		int aX, aY;
		if (!mNetwork)
		{
			int aHeight = aApp->ScaleNum(0x300, 0);
			aY = Rand() % aHeight;
			aX = aApp->ScaleNum(0x400, 0);
		}
		else
		{
			aY = aApp->ScaleNum(-200, 0);
			int aWidth = aApp->ScaleNum(0x400, 0);
			aX = Rand() % aWidth;
		}
		AddStar(aX, aY);
	}
}
