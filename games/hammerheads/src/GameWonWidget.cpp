#include "GameWonWidget.h"

#include "Application.h"
#include "Color.h"
#include "Common.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "NcButton.h"
#include "Res.h"
#include "Stats.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// Where the text is: it is cut off outside the box of the picture, and centred in it
const int BOX_X = 0x1dc;
const int BOX_Y = 0x40;
const int BOX_WIDTH = 0x138;
const int BOX_HEIGHT = 0x11e;
const int TEXT_CENTER_X = 0x278;

// The text starts under the box (the box's bottom edge) and goes up by this every update until the last of it has gone by
const double SCROLL_START = 350.0;
const double SCROLL_END = -625.0;
const double SCROLL_SPEED = 0.15;

// The sound comes after this many hundredths of the delay have run out
const double SOUND_DELAY_START = 1.0;
const double SOUND_DELAY_SPEED = 0.01;

// The space a line takes, and the gaps between the paragraphs of the text
const int LINE_HEIGHT = 0x19;
const int PARAGRAPH_GAP = 0x32;
const int END_GAP = 0x96;

// The button: its width, and how far above the bottom of the page its bottom is
const int BUTTON_WIDTH = 0x14f;
const int BUTTON_RAISE = 10;

const Color TEXT_COLOR(0, 0, 0, 255);
const Color END_COLOR(255, 255, 255, 255);

// The page ends with the main menu (0x453280)
void WipeToMainMenuAfterWin()
{
	gApp->GameEndToMainMenu();
}

// 0x453460: a line in the middle of the box, and the next line's place
void DrawCentered(Graphics& g, const std::string& theText, int* theY)
{
	g.DrawString(theText, TEXT_CENTER_X - g.GetFont()->StringWidth(theText) / 2, *theY);
	*theY += LINE_HEIGHT;
}

// A line that says how many there were: its own words for one, and the format for any other number
void DrawCountLine(Graphics& g, int theCount, const char* theOneText, const char* theFormat, int* theY)
{
	if (theCount == 1)
		DrawCentered(g, theOneText, theY);
	else
		DrawCentered(g, StrFormat(theFormat, theCount), theY);
}

// The gnome's farewell to a Tough Cookie player: the lines of the game won, and the sniffs
void DrawToughFarewell(Graphics& g, int* theY)
{
	g.SetFont(FONT_EUROFURENCE14);
	DrawCentered(g, "You did it", theY);
	g.SetFont(FONT_EUROFURENCE14BOLD);
	DrawCentered(g, "AGAIN!?!", theY);
	g.SetFont(FONT_EUROFURENCE14);
	*theY += PARAGRAPH_GAP;
	DrawCentered(g, "I have to give it to you,", theY);
	DrawCentered(g, "you are one tough cookie!", theY);
	*theY += LINE_HEIGHT;
	DrawCentered(g, "I think I shall just retire now...", theY);
	DrawCentered(g, "Perhaps a kind soul will find", theY);
	DrawCentered(g, "a quiet spot in the shade for me...", theY);
	DrawCentered(g, "*sniff*", theY);
	*theY += LINE_HEIGHT;
	DrawCentered(g, "*sniff*", theY);
}

// The gnome's farewell after the classic game
void DrawClassicFarewell(Graphics& g, int* theY)
{
	g.SetFont(FONT_EUROFURENCE14BOLD);
	DrawCentered(g, "AAARGH! ", theY);
	g.SetFont(FONT_EUROFURENCE14);
	*theY += PARAGRAPH_GAP + LINE_HEIGHT;
	DrawCentered(g, "You may have smashed me,", theY);
	DrawCentered(g, "but now I am stronger than ever!", theY);
	*theY += LINE_HEIGHT;
	DrawCentered(g, "As long as there is superglue", theY);
	DrawCentered(g, "us garden gnomes will come back", theY);
	DrawCentered(g, "to ruin your lawn...", theY);
	*theY += LINE_HEIGHT;
	DrawCentered(g, "The power of garden gnomes", theY);
	DrawCentered(g, "is unstoppable!", theY);
}

// What the player did in the game
void DrawGameSummary(Graphics& g, int* theY)
{
	Stats* aStats = GetStats();

	g.SetColor(TEXT_COLOR);
	g.SetFont(FONT_EUROFURENCE14);
	DrawCentered(g, "During this game,", theY);
	DrawCentered(g, StrFormat("you have smashed %d gnomes.", aStats->GetInt(STAT_GNOMES_SMASHED)), theY);

	int aBombs = aStats->GetInt(STAT_BOMBS_HIT);
	if (aBombs == 0)
		DrawCentered(g, "You didn't hit any bombs.", theY);
	else if (aBombs == 1)
		DrawCentered(g, "You have hit 1 bomb.", theY);
	else
		DrawCentered(g, StrFormat("You have hit %d bombs.", aBombs), theY);

	int aMisses = aStats->GetInt(STAT_MISSES);
	if (aMisses == 0)
		DrawCentered(g, "You didn't miss at all!", theY);
	else if (aMisses == 1)
		DrawCentered(g, "You missed only once!", theY);
	else
		DrawCentered(g, StrFormat("You missed %d times.", aMisses), theY);

	DrawCentered(g, StrFormat("You collected %d hearts,", aStats->GetInt(STAT_HEARTS_COLLECTED)), theY);
	DrawCentered(g, StrFormat("%d coins,", aStats->GetInt(STAT_COINS_COLLECTED)), theY);
	DrawCountLine(g, aStats->GetInt(STAT_POWER_BOTTLES), "1 power bottle,", "%d power bottles,", theY);
	DrawCountLine(g, aStats->GetInt(STAT_CLOVERS), "and 1 lucky clover.", "and %d lucky clovers.", theY);
	DrawCountLine(g, aStats->GetInt(STAT_SUPPLIES_BOUGHT), "You bought 1 supply in the shop.",
		"You bought %d supplies in the shop.", theY);
	DrawCountLine(g, aStats->GetInt(STAT_HEARTS_MISSED), "You failed to pick up 1 heart.",
		"You failed to pick up %d hearts.", theY);
	DrawCountLine(g, aStats->GetInt(STAT_BONUS_HITS), "In the bonus screens, you had 1 hit",
		"In the bonus screens, you had %d hits", theY);
	DrawCountLine(g, aStats->GetInt(STAT_BONUS_MISSES), "and 1 miss.", "and %d misses.", theY);
	DrawCountLine(g, aStats->GetInt(STAT_PERFECT_BONUS_SCREENS), "You finished 1 bonus screen perfectly.",
		"You finished %d bonus screens perfectly.", theY);
}

// What the gnome lets out when the text has run out: how to find the jester gnome of the main menu
void DrawCheatHint(Graphics& g, int* theY)
{
	g.SetColor(TEXT_COLOR);
	g.SetFont(FONT_ARIAL10);
	DrawCentered(g, "Pssst... I'll tell you a CHEAT!", theY);
	DrawCentered(g, "Wait for the jester gnome to appear", theY);
	DrawCentered(g, "on the main menu screen,", theY);
	DrawCentered(g, "then quickly click him for a bonus", theY);
	DrawCentered(g, "when you start a new game!", theY);
}

}

// 0x4534c0: winning opens Tough Cookie to the player
GameWonWidget::GameWonWidget()
{
	mScrollY = SCROLL_START;
	mSoundDelay = SOUND_DELAY_START;
	mButton = MakeNcButton(0, this, "Main Menu", nullptr, 3);
	gApp->FadeOutMusic();
	gApp->mUserManager->GetCurrentUser()->mToughUnlocked = true;
}

// 0x453290
GameWonWidget::~GameWonWidget()
{
	delete mButton;
}

// 0x4533a0
void GameWonWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mButton);
}

// 0x416010
void GameWonWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mButton);
}

// 0x453300: the text goes up and stops when it has all gone by; the sound of the win comes when the delay is over
void GameWonWidget::Update()
{
	Widget::Update();

	double aScrollY = mScrollY - SCROLL_SPEED;
	if (aScrollY < SCROLL_END)
		aScrollY = SCROLL_END;
	mScrollY = aScrollY;
	MarkDirty();

	if (0.0 < mSoundDelay)
	{
		double aSoundDelay = mSoundDelay - SOUND_DELAY_SPEED;
		if (aSoundDelay < 0.0)
			aSoundDelay = 0.0;
		mSoundDelay = aSoundDelay;
		if (aSoundDelay == 0.0)
			gApp->PlaySample(SOUND_STING_GAME_WON);
	}
}

// 0x4533d0: the button is at the bottom, in the middle
void GameWonWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	int aButtonHeight = mButton->mFont->GetHeight();
	mButton->Resize((theWidth - BUTTON_WIDTH) / 2 + theX, theY - aButtonHeight - BUTTON_RAISE + theHeight, BUTTON_WIDTH, aButtonHeight);
}

// 0x453660: the picture, and in its box the farewell, what the player did and "The End" going up; once the end has gone as
// far as it goes, how to find the jester gnome
void GameWonWidget::Draw(Graphics* g)
{
	bool aToughCookie = gApp->mGameMode == GAME_MODE_TOUGH_COOKIE;

	g->DrawImage(IMAGE_GAMEWON_BACK, 0, 0);
	if (aToughCookie)
		g->DrawImage(IMAGE_GAMEWON_BACK_TOUGH_COOKIE, mWidth - IMAGE_GAMEWON_BACK_TOUGH_COOKIE->mWidth, 0);

	Graphics aG(*g);
	aG.ClipRect(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT);
	int aY = ToInt(mScrollY);

	aG.SetColor(TEXT_COLOR);
	if (aToughCookie)
		DrawToughFarewell(aG, &aY);
	else
		DrawClassicFarewell(aG, &aY);

	aY += PARAGRAPH_GAP;
	DrawGameSummary(aG, &aY);

	aY += END_GAP;
	aG.SetFont(FONT_PTBANANASPLIT27);
	aG.SetColor(END_COLOR);
	DrawCentered(aG, "The End", &aY);

	if (mScrollY == SCROLL_END)
	{
		aY += LINE_HEIGHT;
		DrawCheatHint(aG, &aY);
	}

	if (aToughCookie)
		g->DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->mWidth, mHeight - IMAGE_TOUGH_COOKIE_STAMP->mHeight);
}

// 0x453440: the way out is a wipe to the main menu, whichever button it is (there is the one)
void GameWonWidget::ButtonDepress(int theId)
{
	gApp->DoZoomWipe(WipeToMainMenuAfterWin, WIPE_HAMMER);
}

// 0x4535c0
GameWonWidget* Sexy::MakeGameWon()
{
	GameWonWidget* aWidget = new GameWonWidget();
	aWidget->Resize(0, 0, gApp->mWidth, gApp->mHeight);
	gApp->mWidgetManager->AddWidget(aWidget);
	gApp->mWidgetManager->SetFocus(aWidget);
	return aWidget;
}
