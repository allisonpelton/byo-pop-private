#include "BossWarning.h"
#include "Board.h"
#include "Compat2004.h"
#include "GameText.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Font.h"
#include "MusicInterface.h"
#include "SexyMatrix.h"
#include "Sprite.h"
#include "XMLParser.h"

#include <math.h>

using namespace Sexy;

const BossInfo Sexy::BOSS_INFOS[HeavyWeaponApp::BOSS_FINAL + 1] =
{
	{ 0, 110, 0.0, "Helicopter" },
	{ 220, 320, 1.5707963267948966, "Battleship" },
	{ 220, 180, 1.5707963267948966, "Rainer" },
	{ 220, 320, 1.5707963267948966, "Wrecker" },
	{ 220, 300, 1.5707963267948966, "Ape" },
	{ 0, 110, 0.0, "Eye" },
	{ -220, 180, 4.71238898038469, "Head" },
	{ -200, 380, 3.141592653589793, "Worm" },
	{ 0, 110, 0.0, "Robot" },
	{ 0, 110, 0.0, "Final" }
};

// 0x40e730: turns each point by theAngle about the origin and moves it to (theX, theY), in the framework's float
// matrices, truncating the result
static void TurnPoints(Point* thePoints, int theCount, double theX, double theY, double theAngle)
{
	int i = 0;
	if (theCount > 0)
	{
		do
		{
			SexyTransform2D aTransform;
			aTransform.LoadIdentity();
			aTransform.Translate((float)thePoints[i].mX, (float)thePoints[i].mY);
			aTransform.RotateRad((float)theAngle);
			aTransform.Translate((float)theX, (float)theY);
			thePoints[i].mX = (int)aTransform.m02;
			thePoints[i].mY = (int)aTransform.m12;
			i++;
		}
		while (i < theCount);
	}
}

// 0x40ed50
BossWarning::BossWarning(Board* theBoard)
{
	mBoard = theBoard;
	mApp = theBoard->mApp;
	mFlashes = 0;
	mTextShown = 0;
	mFlash = 254;

	// Six bars, each wider and lower than the last, making an arrow
	int aBoss = mApp->GetBossIndex(mApp->mLevel);
	int aY = -54;
	int aRight = 10;
	int aLeft = -10;
	Point* anArrow = mArrows[0];
	do
	{
		anArrow[3].mX = aLeft;
		anArrow[0].mX = aLeft + 8;
		anArrow[0].mY = aY - 16;
		anArrow[1].mX = aRight - 8;
		anArrow[1].mY = aY - 16;
		anArrow[2].mX = aRight;
		anArrow[2].mY = aY;
		anArrow[3].mY = aY;
		TurnPoints(anArrow, 4, (double)BOSS_INFOS[aBoss].mWarningX, (double)BOSS_INFOS[aBoss].mWarningY,
			BOSS_INFOS[aBoss].mWarningAngle);
		aLeft -= 10;
		aRight += 10;
		aY += 20;
		anArrow += 4;
	}
	while (aLeft > -70);

	LoadText();
}

// 0x40eb30: the first boss's description is in the code; the others' are their elements' "info" in data\bosses.xml
void BossWarning::LoadText()
{
	if (mApp->mLevel == 0)
	{
		mText = GT(0x0052b058);
		return;
	}

	XMLParser aParser;
	aParser.OpenFile("data\\bosses.xml");
	XMLElement anElement;
	int aBoss = mApp->GetBossIndex(mApp->mLevel);
	while (aParser.NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START && anElement.mValue == BOSS_INFOS[aBoss].mName)
			mText = anElement.mAttributes["info"];
	}
}

// 0x40e800
void BossWarning::Update()
{
	if (mFlash == 254)
		mApp->PlaySample(SOUND_ALERT, 0);
	if (mFlash == 128 && mFlashes == 0)
		mApp->PlaySample(SOUND_V_DANGER, 0);
	if (mFlash != 0)
		mFlash -= 2;
	if (mFlash == 0)
	{
		mFlashes++;
		if (mFlashes == NUM_FLASHES)
		{
			mBoard->mBossWarning = NULL;
			// The boss tune, from its order in AtomicTank.mo3
			mBoard->mApp->mMusicInterface->PlayMusic(SONG_ATOMIC_TANK_42, 0x2a, false);
			delete this;
			return;
		}
		mFlash = 254;
	}

	// Typing from the first flash's middle, a letter every other update
	if (mTextShown < (int)mText.size() && (mFlashes > 0 || mFlash < 128))
	{
		if (mApp->mUpdateCount % 2 == 0)
		{
			mTextShown++;
			mApp->PlaySample(SOUND_STATBEEP, 0);
		}
	}
}

// 0x40e920
void BossWarning::Draw(Graphics* g)
{
	if (mFlash == 0)
		return;

	g->Translate(-320, 0);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(mFlash, 0, 0, 255));
	g->FillRect(0, 0, 640, 480);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->Translate(320, 0);

	// The arrows pulse from red to yellow, each a little after the one before
	for (int i = 0; i < NUM_ARROWS; i++)
	{
		int aPhase = (mApp->mUpdateCount + i * 16) % 100;
		g->SetColor(Color(255, (int)(sin(aPhase * 0.03141) * 255.0), 0, 128));
		g->PolyFill(mArrows[i], 4, false);
	}

	int aBoss = mApp->GetBossIndex(mApp->mLevel);
	if (mApp->mUpdateCount % 60 < 30)
		mApp->mSprites[IMAGE_DANGER]->Draw(g, BOSS_INFOS[aBoss].mWarningX, BOSS_INFOS[aBoss].mWarningY, true, 0, 0, false);

	g->SetFont(mApp->mFonts[FONT_KEYPUNCH16]);
	g->SetColor(Color(255, 255, 255));
	WriteWordWrapped2004(g, Rect(-190, 180, 380, 300), mText.substr(0, mTextShown),
		mApp->mFonts[FONT_KEYPUNCH16]->GetHeight(), -1);
}
