// The high score entry box (see Dialogs.h).

#include "Dialogs.h"
#include "AtomicaApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HighScores.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cstdio>

#ifdef __EMSCRIPTEN__
#include "WebProfiles.h"
#endif

using namespace Sexy;

// 0x41cc00
HighScoreDialog::HighScoreDialog(AtomicaApp* theApp, int theScore) :
	GameDialog(theApp, false),
	mSpin(1.0f),
	mScore(theScore),
	mTick(0)
{
	mBoxX = 0x6e;
	// the monitor says one of ten things
	static const unsigned int kPhrases[10] = {0x4b01b8, 0x4b01a4, 0x4b0190, 0x4b0180, 0x4b0168, 0x4b0158, 0x4b0144, 0x4b0134,
		0x4b011c, 0x4b0104};
	mPhrase = GTC(kPhrases[mApp->mRand.Get(10, false)]);
	mEdit = new EditWidget(0, this);
	mOthers.Add(mEdit);
	mEdit->SetFont(mApp->mMonitorFontMedium);
	// 0x41cc00: no field or outline of its own (the box draws the picture of one), light text, a white highlight
	static int kEditColors[5][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}};
	mEdit->SetColors(kEditColors, 5);
	mEdit->mMaxChars = 15;
	mEdit->mMaxPixels = 140;
#ifdef __EMSCRIPTEN__
	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with the site's player
	Byopop::SiteType("atomica", mEdit, false);
#endif
	mOk = AddButton(2);
	mSkip = AddButton(3);
}

// 0x41ca20
HighScoreDialog::~HighScoreDialog()
{
	mApp->mScoreDialog = NULL;
}

// 0x41caf0: the box is placed by the monitor's picture; the name line and the buttons by the x and y it was asked for
void HighScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	mEdit->Resize(0xcc, theY + 0x117, 0xe4, 0x20);
	mOk->Resize(theX + 0x78, theY + 0x145, 0x50, 0x1b);
	mSkip->Resize(theX + 0xd4, theY + 0x145, 0x50, 0x1b);
	mManager->SetFocus(mEdit);
}

void HighScoreDialog::Update()
{
	GameDialog::Update();
	// 0x41ca50
	mSpin += 0.05f;
	if (mSpin >= 256.0f)
		mSpin -= 256.0f;
	mTick += 4;
	if (mTick > 300)
		mTick = 0;
}

// 0x41cf00: "okay" puts the name in the table (and the voice cheers), "skip" closes the box
void HighScoreDialog::ButtonDepress(int theId)
{
	if (mClosing)
		return;
	if (theId == 2)
	{
		Sound* aVoices[6] = {mApp->mSndCool, mApp->mSndExcellent, mApp->mSndGreat, mApp->mSndNifty, mApp->mSndTerrific,
			mApp->mSndWicked};
		aVoices[mApp->mRand.Get(6, false)]->Play();
		mApp->mScoreTables[mApp->mMode == 0 ? 0 : 1].Add(mScore, mEdit->mString.c_str());
		mApp->WriteHighScores();
		mClosing = true;
	}
	else if (theId == 3)
		mClosing = true;
}

void HighScoreDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void) theId;
	(void) theString;
	ButtonDepress(2);
}

// 0x41cf90: the colours of the page's scores run through the title (six copies climbing, faint to full), the score
// between two arrows that come and go, what the monitor says and the prompt; the name goes on a picture of a field and the
// buttons are plain green fields
void HighScoreDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	Color aColors[256];
	BuildScoreColors(aColors, 255);
	// each use steps on to the next colour, round the 256 by 255 as the original does
	int aIndex = (int) mSpin;
	g->SetFont(mApp->mMonitorFont);
	const std::string& aTitle = GT(0x4b0258);
	int aTitleX = mWidth / 2 - mApp->mMonitorFont->StringWidth(aTitle) / 2;
	int aY = 0xdc;
	for (int anAlpha = 0; anAlpha < 0x40; anAlpha += 12)
	{
		g->SetColor(Color(aColors[aIndex].mRed, aColors[aIndex].mGreen, aColors[aIndex].mBlue, anAlpha));
		g->DrawString(aTitle, aTitleX, aY);
		aY -= 3;
		aIndex = (aIndex + 1 > 0xff) ? aIndex - 0xff : aIndex + 1;
	}
	g->SetColor(Color(aColors[aIndex].mRed, aColors[aIndex].mGreen, aColors[aIndex].mBlue, 255));
	g->DrawString(aTitle, aTitleX, 0xca);

	aIndex = (aIndex + 1 > 0xff) ? aIndex - 0xff : aIndex + 1;
	aIndex = 0xff - aIndex;
	int aRed = aColors[aIndex].mRed;
	int aGreen = aColors[aIndex].mGreen;
	if (aGreen < 0xb0)
		aGreen = 0xaf;
	if (aRed < 0xb0)
		aRed = 0xaf;
	g->SetFont(mApp->mScoreFont);
	g->SetColor(Color(aRed, aGreen, aColors[aIndex].mBlue, 255));
	char aScore[16];
	snprintf(aScore, sizeof(aScore), "%d", mScore);
	std::string aScoreText = aScore;
	int aScoreWidth = mApp->mScoreFont->StringWidth(aScoreText);
	g->DrawString(aScoreText, mWidth / 2 - aScoreWidth / 2, 0xb4);
	int anArrowGap = (mTick < 0x32) ? 0x10 : 0x16;
	if ((mTick < 0x32) || (mTick >= 0x97))
	{
		g->SetColorizeImages(true);
		mApp->mScoreArrows[0]->DrawCentered(g, mWidth / 2 - aScoreWidth / 2 - anArrowGap, 0xab);
		mApp->mScoreArrows[1]->DrawCentered(g, aScoreWidth / 2 + anArrowGap + mWidth / 2, 0xab);
		g->SetColorizeImages(false);
	}

	aIndex = (aIndex + 1 > 0xff) ? aIndex - 0xff : aIndex + 1;
	g->SetFont(mApp->mMonitorFontMedium);
	g->SetColor(Color(aColors[aIndex].mRed, aColors[aIndex].mGreen, aColors[aIndex].mBlue, (mTick < 0x65) ? 0x80 : 0xff));
	std::string aPhrase = mPhrase;
	g->DrawString(aPhrase, mWidth / 2 - mApp->mMonitorFontMedium->StringWidth(aPhrase) / 2, 0xeb);
	g->SetFont(mApp->mMonitorFontSmall);
	g->SetColor(Color(aColors[aIndex].mRed, aColors[aIndex].mGreen, aColors[aIndex].mBlue, 255));
	const std::string& aPrompt = GT(0x4b0244);
	g->DrawString(aPrompt, mWidth / 2 - mApp->mMonitorFontSmall->StringWidth(aPrompt) / 2, 0x10e);

	mApp->mTextEdit->Draw(g, mEdit->mX - mX, mEdit->mY - mY);
	g->SetFont(mApp->mMonitorFont);
	ButtonWidget* aButtons[2] = {mOk, mSkip};
	const std::string aCaptions[2] = {GT(0x4b01d8), GT(0x4b01d0)};
	for (int i = 0; i < 2; i++)
	{
		ButtonWidget* aButton = aButtons[i];
		g->SetColor(Color(0, 255, 0, 0x80));
		if (aButton->mIsDown)
			g->SetColor(Color(0, 255, 0, 0xff));
		if (aButton->mIsOver)
			g->SetColor(Color(0, 255, 0, 0xaf));
		g->FillRect(aButton->mX - mX, aButton->mY - mY, aButton->mWidth, aButton->mHeight);
		g->SetColor(Color(0, 0x32, 0, 255));
		g->DrawString(aCaptions[i], aButton->mX - mX + aButton->mWidth / 2 - mApp->mMonitorFont->StringWidth(aCaptions[i]) / 2,
			aButton->mY - mY + 0x14);
	}
}
