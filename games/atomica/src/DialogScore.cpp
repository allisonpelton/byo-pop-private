// The high score entry box (see Dialogs.h).

#include "Dialogs.h"
#include "AtomicaApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HighScores.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <cstdio>

using namespace Sexy;

// 0x41cc00
HighScoreDialog::HighScoreDialog(AtomicaApp* theApp, int theScore) :
	GameDialog(theApp, false),
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
	mEdit->mMaxChars = 20;
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
	mTick++;
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

// 0x41cf90: port: the title, the score, what the monitor says, the name line and the two buttons (the original also
// cycles colours and slides a head across the monitor)
void HighScoreDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	Font* aMonitor = mApp->mMonitorFont;
	int aPulse = 160 + (int) (95.0f * (mTick % 100) / 100.0f);
	g->SetFont(aMonitor);
	g->SetColor(Color(0, aPulse, 0, 255));
	const std::string& aTitle = GT(0x4b0258);
	g->DrawString(aTitle, mWidth / 2 - aMonitor->StringWidth(aTitle) / 2, 0xa0);
	char aScore[16];
	snprintf(aScore, sizeof(aScore), "%d", mScore);
	g->SetFont(mApp->mScoreFont);
	g->SetColor(Color(0xb0, 255, 0, 255));
	std::string aScoreText = aScore;
	g->DrawString(aScoreText, mWidth / 2 - mApp->mScoreFont->StringWidth(aScoreText) / 2, 0xc8);
	g->SetFont(mApp->mMonitorFontMedium);
	g->SetColor(Color(0, 255, 100, 255));
	std::string aPhrase = mPhrase;
	g->DrawString(aPhrase, mWidth / 2 - mApp->mMonitorFontMedium->StringWidth(aPhrase) / 2, 0xeb);
	g->SetFont(mApp->mMonitorFontSmall);
	g->SetColor(Color(0, 255, 0, 255));
	const std::string& aPrompt = GT(0x4b0244);
	g->DrawString(aPrompt, mWidth / 2 - mApp->mMonitorFontSmall->StringWidth(aPrompt) / 2, 0x10e);
	DrawButton(g, mOk, GT(0x4b01c8));
	DrawButton(g, mSkip, GT(0x4b01d0));
}
