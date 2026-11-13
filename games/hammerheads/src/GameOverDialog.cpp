#include "GameOverDialog.h"

#include <cmath>

#include "Application.h"
#include "Buffer.h"
#include "Common.h"
#include "Creature.h"
#include "DialogButton.h"
#include "Font.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "Image.h"
#include "Insets.h"
#include "IntroCreature.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

namespace
{

// The box is this tall (and 0x258 wide at least) with a gnome on it, or this tall without
const int HEIGHT_WITH_CREATURE = 0x1c2;
const int HEIGHT_WITHOUT_CREATURE = 0xfa;
const int MIN_WIDTH = 0x258;

// The room around the text: left, top, right and bottom
const Insets CONTENT_INSETS(0x3c, 5, 0x3c, 0x1e);

// The turn of the light behind the gnome a time step, and how far below the creature's mark its feet are drawn
const double LIGHT_TURN = 0.0075;
const int CREATURE_DROP = 0x1e;

// How far below the creature the lines about the next level are, in the colour of the hint for the early levels
const int NEXT_LEVEL_LINE_Y = 0x23;
const int HINT_LINE_1_Y = 0x3c;
const int HINT_LINE_2_Y = 0x50;
const int HINT_LAST_LEVEL = 6;

// The heights of the lines about the game that was played, and of the question the box asks of the modes with no gnome
const int QUESTION_Y = 0x91;
const int REACHED_LINE_Y = 0x64;
const int SCORE_LINE_Y = 0x78;

// How the question mark moves: it pulses a time step at this rate, and sways and bobs at the rates its caller gives
const double MARK_PULSE_SPEED = 0.025;
const double MARK_MOTION_SCALE = 0.3;
const double MARK_BOB_SIZE = 10.0;
const double MARK_SWAY_SIZE = -5.0;
const int MARK_RISE = 0x14;
const double MARK_SWAY_SPEED = 0.05;
const double MARK_BOB_SPEED = 0.11;

// Whether the box has a gnome of the next level on it
bool ShowsCreature()
{
	return HasIntroFile(gApp->mGameScreen->mLevel) && gApp->mGameMode == GAME_MODE_CLASSIC;
}

// 0x46a2b0: how long a game of marathon lasted, in words
std::string DescribeTime(int theHours, int theMinutes, int theSeconds)
{
	const char* anHourWord = theHours == 1 ? "hour" : "hours";
	const char* aMinuteWord = theMinutes == 1 ? "minute" : "minutes";
	const char* aSecondWord = theSeconds == 1 ? "second" : "seconds";
	if (0 < theHours)
		return StrFormat("%d %s and %d %s", theHours, anHourWord, theMinutes, aMinuteWord);
	if (0 < theMinutes)
		return StrFormat("%d %s and %d %s", theMinutes, aMinuteWord, theSeconds, aSecondWord);
	return StrFormat("%d %s", theSeconds, aSecondWord);
}

// A line in the middle of the box
void DrawCentered(Graphics& g, const std::string& theText, int theCenterX, int theY)
{
	g.DrawString(theText, theCenterX - g.GetFont()->StringWidth(theText) / 2, theY);
}

}

// 0x44ff70
GameOverDialog::GameOverDialog() :
	NcMovingDialog(ShowsCreature() ? HEIGHT_WITH_CREATURE : HEIGHT_WITHOUT_CREATURE, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON,
		DIALOG_GAME_OVER, true, "", "", "Too bad...", Dialog::BUTTONS_YES_NO)
{
	mContentInsets = CONTENT_INSETS;
	SetButtonFont(FONT_ARIAL10);
	mYesButton->mLabel = "Try Again";
	mNoButton->mLabel = "Main Menu";

	// the gnome is the one the level's file names, dark
	mCreature = nullptr;
	int aLevel = gApp->mGameScreen->mLevel;
	if (HasIntroFile(aLevel))
	{
		Buffer aBuffer;
		if (gApp->ReadBufferFromFile(GetIntroFileName(aLevel), &aBuffer, false))
		{
			int aIntroIndex = 0;
			StringToInt(aBuffer.ReadLine(), &aIntroIndex);
			mCreature = MakeIntroCreature(aIntroIndex);
			if (mCreature != nullptr)
				mCreature->mDarkened = true;
		}
	}
}

// 0x44fc70: the gnome is deleted without being counted as one that got away
GameOverDialog::~GameOverDialog()
{
	if (mCreature != nullptr)
	{
		mCreature->mHealth = 0;
		mCreature->mCubbyReleased = true;
		delete mCreature;
		mCreature = nullptr;
	}
}

// 0x44fe10
void GameOverDialog::Update()
{
	WidgetContainer::Update();
	if (mCreature != nullptr)
	{
		double aPosition[2] = { 0.0, 0.0 };
		mCreature->Update(aPosition);
	}
}

// 0x44fd00: a question mark over the gnome that grows and fades in and out while it sways and bobs
void GameOverDialog::DrawQuestionMark(Graphics* g, int theTime, int theOffsetY, double theSwaySpeed, double theBobSpeed)
{
	Image* aMark = IMAGE_QUESTIONMARK;
	double aPulse = (cos((double) theTime * MARK_PULSE_SPEED) + 1.0) * 0.5;
	double aScale = aPulse * 0.5 + 1.0;
	int aWidth = (int) ((double) aMark->mWidth * aScale);
	int aHeight = (int) ((double) aMark->mHeight * aScale);

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int) (aPulse * 255.0)));
	int aBob = (int) (cos((double) theTime * theBobSpeed * MARK_MOTION_SCALE) * MARK_BOB_SIZE);
	int aY = aBob - aMark->mHeight - MARK_RISE + theOffsetY;
	int aSway = (int) (sin((double) theTime * theSwaySpeed * MARK_MOTION_SCALE) * MARK_SWAY_SIZE);
	g->DrawImage(aMark, mWidth / 2 - aMark->mWidth / 2 - aSway, aY, aWidth, aHeight);
	g->SetColorizeImages(false);
}

// 0x4502e0: the gnome of the next level under its light and mark (a classic game), then how the game went
void GameOverDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	Graphics aG(*g);
	int aCenterX = mWidth / 2;
	aG.SetFont(FONT_EUROFURENCE14BOLD);

	if (mCreature != nullptr && gApp->mGameMode == GAME_MODE_CLASSIC)
	{
		int aCreatureY;
		int aMarkOffsetY;
		GetIntroPlacement(mCreature, aCreatureY, aMarkOffsetY);
		double aLightAngle = (double) mUpdateCnt * LIGHT_TURN;
		aCreatureY += CREATURE_DROP;

		Image* aLight = IMAGE_DIVINELIGHT_SMALL;
		int aCelHeight = mCreature->mImage->GetCelHeight();
		aG.DrawImageRotated(aLight, mWidth / 2 - aLight->mWidth / 2, aCreatureY - aCelHeight / 2 - aLight->mHeight / 2, aLightAngle,
			aLight->mWidth / 2, aLight->mHeight / 2, nullptr);
		mCreature->Draw(&aG, (double) (mWidth / 2), (double) aCreatureY, 0, 0.0);
		DrawQuestionMark(&aG, mUpdateCnt, aMarkOffsetY, MARK_SWAY_SPEED, MARK_BOB_SPEED);

		aG.SetColor(Color(0, 0, 0, 255));
		DrawCentered(aG, "Try again and find out what is on the next level!", aCenterX, aCreatureY + NEXT_LEVEL_LINE_Y);
		if (gApp->mGameScreen->mLevel < HINT_LAST_LEVEL)
		{
			aG.SetColor(Color(255, 255, 200));
			DrawCentered(aG, "If you make it to the end of level 5,", aCenterX, aCreatureY + HINT_LINE_1_Y);
			DrawCentered(aG, "you will not have to start on level 1 again!", aCenterX, aCreatureY + HINT_LINE_2_Y);
			aG.SetColor(Color(0, 0, 0, 255));
		}
	}
	else if (gApp->mGameMode == GAME_MODE_MARATHON || gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
	{
		aG.SetColor(Color(0, 0, 0, 255));
		DrawCentered(aG, "Would you like to try again?", aCenterX, QUESTION_Y);
	}

	if (gApp->mGameMode == GAME_MODE_CLASSIC || gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
	{
		int aGnomesSmashed = GetStats()->GetInt(STAT_GNOMES_SMASHED);
		int aLevel = gApp->mGameScreen->mLevel;
		std::string aText;
		if (aGnomesSmashed == 0)
			aText = StrFormat("You reached level %d, failing to hit a single gnome", aLevel);
		else if (aGnomesSmashed == 1)
			aText = StrFormat("You reached level %d, bashing one measly gnome", aLevel);
		else
			aText = StrFormat("You reached level %d, bashing %d gnomes", aLevel, aGnomesSmashed);
		DrawCentered(aG, aText, aCenterX, REACHED_LINE_Y);

		aText = StrFormat("and scoring %d points!", GetStats()->GetInt(STAT_SCORE));
		DrawCentered(aG, aText, aCenterX, SCORE_LINE_Y);
	}
	else
	{
		int aSeconds = (int) GetStats()->Get(STAT_MARATHON_TIME);
		std::string aTime = DescribeTime(aSeconds / 3600, (aSeconds % 3600) / 60, aSeconds % 60);
		DrawCentered(aG, StrFormat("You survived for %s,", aTime.c_str()), aCenterX, REACHED_LINE_Y);

		int aGnomesSmashed = GetStats()->GetInt(STAT_GNOMES_SMASHED);
		int aScore = GetStats()->GetInt(STAT_SCORE);
		std::string aText;
		if (aGnomesSmashed == 0)
			aText = StrFormat("failing to hit a single gnome and scoring %d points!", aScore);
		else if (aGnomesSmashed == 1)
			aText = StrFormat("bashing one measly gnome and scoring %d points!", aScore);
		else
			aText = StrFormat("bashing %d gnomes and scoring %d points!", aGnomesSmashed, aScore);
		DrawCentered(aG, aText, aCenterX, SCORE_LINE_Y);
	}
}

// 0x44ff10
int GameOverDialog::GetPreferredHeight(int theWidth)
{
	return ShowsCreature() ? HEIGHT_WITH_CREATURE : HEIGHT_WITHOUT_CREATURE;
}

// 0x44fee0
void GameOverDialog::Closed()
{
	if (mCloseResult == Dialog::ID_YES)
		gApp->RetryFromGameOver();
	else if (mCloseResult == Dialog::ID_NO)
		gApp->GameOverDone();
}

// 0x450aa0
GameOverDialog* Sexy::MakeGameOverDialog()
{
	GameOverDialog* aDialog = new GameOverDialog();
	LayoutDialog(aDialog, MIN_WIDTH);
	int aWidth = aDialog->mWidth;
	int aHeight = aDialog->mHeight;
	aDialog->Resize((gApp->mWidth - aWidth) / 2, -aHeight, aWidth, aHeight);
	gApp->AddDialog(DIALOG_GAME_OVER, aDialog);
	return aDialog;
}
