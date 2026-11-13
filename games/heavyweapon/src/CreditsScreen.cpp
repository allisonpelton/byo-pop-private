#include "CreditsScreen.h"
#include "FlickerLight.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "MainMenu.h"
#include "OptionsDialog.h"
#include "Compat2004.h"
#include "DDImage.h"
#include "Graphics.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "XMLParser.h"
#include <cmath>

using namespace Sexy;

namespace
{
	const double PI_TIMES_2 = 6.283185307179586;
	const double PHASE_SPEED = 0.1;
	const double SCROLL_SPEED = 0.25;
	const double SCROLL_START = 510.0;
	const double WAVE_STEP = 0.8;
	const int RIBBON_LEFT = 0x58;			// the ribbon's left edge, a point at each end
	const int RIBBON_TOP = 0x2c;
	const int RIBBON_BOTTOM = 0x94;
	const int WAVE_FIRST_X = 0x6e;
	const int WAVE_X_STEP = 0x16;
	const int WAVE_END_X = 0x108;
	const double WAVE_TOP_BASE = 44.0;
	const double WAVE_BOTTOM_BASE = 148.0;
	const int SCREEN_HEIGHT = 480;
	const int ALPHA_PEAK = 0x1ef;			// a line's alpha: this less twice its distance from the middle of the screen
	const double SCREEN_MIDDLE = 240.0;
	const int ROLE_X = 0xf;
	const int NAME_X = 0xb9;
	const int HEADING_SHIFT = 0x55;
}

// 0x41ec80: the lines of the credits file; the ribbon starts as its two ends
CreditsScreen::CreditsScreen(HeavyWeaponApp* theApp)
{
	mApp = theApp;
	for (int i = 0; i < NUM_RIBBON_POINTS; i++)
		mRibbon[i] = Point(0, 0);
	mRibbon[0] = Point(RIBBON_LEFT, RIBBON_TOP);
	mRibbon[NUM_RIBBON_POINTS - 1] = Point(RIBBON_LEFT, RIBBON_BOTTOM);
	mRibbonPhase = 0.0;
	mScroll = SCROLL_START;
	LoadLines();
	Resize(0, 0, 0x280, 0x1e0);
	mMouseVisible = false;
}

// 0x41e940 (body 0x41e7b0): the main menu returns (with a new lamp), and an options dialog open under the credits is made
// again
CreditsScreen::~CreditsScreen()
{
	if (mApp->mMainMenu != NULL)
	{
		mApp->mWidgetManager->AddWidget(mApp->mMainMenu);
		mApp->mMainMenu->mFlickerLight = new FlickerLight(mApp, mApp->mSprites[IMAGE_MAINGLOW]);
		if (mApp->GetDialog(HeavyWeaponApp::DIALOG_OPTIONS) != NULL)
		{
			mApp->KillDialog(HeavyWeaponApp::DIALOG_OPTIONS);
			mApp->AddDialog(HeavyWeaponApp::DIALOG_OPTIONS, new OptionsDialog(mApp));
		}
	}
}

// 0x41e970: data\credits.xml, a line for each text under a tag (h, t, l) and an empty one for each b
void CreditsScreen::LoadLines()
{
	XMLParser* aParser = new XMLParser();
	aParser->OpenFile("data\\credits.xml");
	XMLElement anElement;
	int aType = CREDIT_HEADING;
	while (!aParser->HasFailed() && aParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			if (anElement.mValue == "h")
				aType = CREDIT_HEADING;
			else if (anElement.mValue == "t")
				aType = CREDIT_ROLE;
			else if (anElement.mValue == "l")
				aType = CREDIT_NAME;
			else if (anElement.mValue == "b")
			{
				aType = CREDIT_BLANK;
				CreditLine aBlank;
				aBlank.mType = aType;
				mLines.push_back(aBlank);
			}
			if (!aParser->NextElement(&anElement))
				break;
			if (anElement.mType == XMLElement::TYPE_ELEMENT)
			{
				CreditLine aLine;
				aLine.mText = anElement.mValue;
				aLine.mType = aType;
				mLines.push_back(aLine);
			}
		}
	}
	delete aParser;
}

// 0x41ddd0 slot 16: the picture, the black ribbon, then each line fading with its distance from the middle of the screen
void CreditsScreen::Draw(Graphics* g)
{
	mApp->mSprites[IMAGE_CREDITS]->Draw(g, 0, 0, false, 0, 0, false);
	g->SetColor(Color(0, 0, 0));
	g->PolyFill(mRibbon, NUM_RIBBON_POINTS, false);

	int aOffset = 0;
	for (int i = 0; i < (int) mLines.size(); i++)
	{
		double aPos = (double) aOffset;
		int aDistance = (int) (SCREEN_MIDDLE - (aPos + mScroll));
		int anAlpha = ALPHA_PEAK - ((aDistance < 0) ? -aDistance : aDistance) * 2;
		if (anAlpha > 0xff)
			anAlpha = 0xff;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		int aType = mLines[i].mType;
		bool isOnScreen = (0.0 < aPos + mScroll) && (aPos + mScroll < (double) SCREEN_HEIGHT);
		if (aType == CREDIT_HEADING)
		{
			if (isOnScreen)
			{
				g->Translate(-HEADING_SHIFT, 0);
				g->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET20]);
				WriteCenteredLine(g, (int) (aPos + mScroll), mLines[i].mText);
				g->Translate(HEADING_SHIFT, 0);
			}
			aOffset += mApp->mFonts[FONT_RUBBERSTAMPLET20]->GetHeight() * 2;
		}
		else if (aType == CREDIT_ROLE)
		{
			if (isOnScreen)
			{
				g->SetColor(Color(0xff, 0xff, 0, anAlpha));
				g->SetFont(mApp->mFonts[FONT_NORMAL]);
				WriteString2004(g, mLines[i].mText, ROLE_X, (int) (aPos + mScroll), -1, -1, true, 0, -1);
			}
		}
		else if (aType == CREDIT_NAME)
		{
			if (isOnScreen)
			{
				g->SetFont(mApp->mFonts[FONT_NORMAL]);
				WriteString2004(g, mLines[i].mText, NAME_X, (int) (aPos + mScroll), -1, -1, true, 0, -1);
			}
			aOffset += mApp->mFonts[FONT_NORMAL]->GetHeight();
		}
		else if (aType == CREDIT_BLANK)
			aOffset += mApp->mFonts[FONT_NORMAL]->GetHeight();
	}
	// the text has scrolled off the top: it starts again
	if ((double) aOffset + mScroll < 0.0)
		mScroll = SCROLL_START;
}

// 0x41dce0 slot 17: the waves move, the text scrolls, and the ribbon's edges follow the waves (each x has the same wave on
// the top edge and the bottom edge, 104 apart)
void CreditsScreen::Update()
{
	Widget::Update();
	mRibbonPhase = mRibbonPhase - PHASE_SPEED;
	if (mRibbonPhase < 0.0)
		mRibbonPhase = mRibbonPhase + PI_TIMES_2;

	int aStep = 1;
	int aAmplitude = 2;
	int aTopIndex = 1;
	int aBottomIndex = NUM_RIBBON_POINTS - 2;
	for (int x = WAVE_FIRST_X; x < WAVE_END_X; x += WAVE_X_STEP)
	{
		double aSine = sin((double) aStep * WAVE_STEP + mRibbonPhase);
		mRibbon[aTopIndex] = Point(x, (int) ((aSine + 1.0) * (double) aAmplitude + WAVE_TOP_BASE));
		mRibbon[aBottomIndex] = Point(x, (int) ((aSine + 1.0) * (double) aAmplitude + WAVE_BOTTOM_BASE));
		aStep++;
		aAmplitude += 2;
		aTopIndex++;
		aBottomIndex--;
	}
	mScroll = mScroll - SCROLL_SPEED;
	MarkDirty();
}

// 0x42ea90 slot 19: the credits take the mouse and the focus once
void CreditsScreen::GotFocus()
{
	if (!mIsOver || !mMouseVisible)
	{
		mHasFocus = true;
		mMouseVisible = true;
	}
}

// 0x41dcb0 slot 28: a click once the credits have the focus closes them
void CreditsScreen::MouseDown(int x, int y, int theClickCount)
{
	if (mHasFocus)
	{
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
	}
	mHasFocus = true;
}
