#include "LevelIntroWidget.h"

#include "Application.h"
#include "Buffer.h"
#include "Common.h"
#include "Creature.h"
#include "DrawUtil.h"
#include "Font.h"
#include "GameScreen.h"
#include "GnomeBouncing.h"
#include "GnomeFlying.h"
#include "Graphics.h"
#include "HowToPlayWidget.h"
#include "Image.h"
#include "IntroCreature.h"
#include "NcButton.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

enum
{
	BUTTON_CONTINUE = 0
};

// Where the button stands under the page's top
const int BUTTON_Y = 400;

// How the light turns: its angle is the time times this, and it is drawn this far above the creature's feet (less the cel's half height)
const double LIGHT_TURN_RATE = 0.05;
const int LIGHT_RAISE = 0x32;

// How fast the aviator and the jester change their cels, and how many cels each has
const double FLYING_CEL_RATE = 30.0;
const int FLYING_CELS = 8;
const double BOUNCING_CEL_RATE = 20.0;
const int BOUNCING_CELS = 10;

// How much the time and the creature move on in an update
const double TIME_STEP = 0.01;

// The heading and the lines of text: where the heading is, where the first line is and how far apart the lines are
const int HEADING_Y = 0x140;
const int FIRST_LINE_Y = 0x15e;
const int LINE_SPACING = 0x14;

}

// 0x457f80
LevelIntroWidget::LevelIntroWidget(int theLevel) :
	NcMovingWidget(gApp->mHeight)
{
	mTime = 0.0;
	mIntroIndex = -1;
	mButton = MakeNcButton(BUTTON_CONTINUE, this, "", IMAGE_INTROSCREEN_BUTTON, 3);
	ReadFile(theLevel);
	mCreature = MakeIntroCreature(mIntroIndex);
}

// 0x457cd0: the gnome is deleted without being counted as one that got away
LevelIntroWidget::~LevelIntroWidget()
{
	delete mButton;
	if (mCreature != nullptr)
	{
		mCreature->mCubbyReleased = true;
		mCreature->mHealth = 0;
		delete mCreature;
	}
}

// 0x457dc0: the first line of the file is the number that picks the gnome, the others are the page's heading and lines
void LevelIntroWidget::ReadFile(int theLevel)
{
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(GetIntroFileName(theLevel), &aBuffer, false))
		return;

	StringToInt(aBuffer.ReadLine(), &mIntroIndex);
	while (!aBuffer.AtEnd())
		mLines.push_back(aBuffer.ReadLine());
}

// 0x457810
void LevelIntroWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mButton);
}

// 0x457840
void LevelIntroWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mButton);
}

// 0x4577b0: the button is in the middle, under the page's top
void LevelIntroWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mButton->Move((theWidth - IMAGE_INTROSCREEN_BUTTON->GetCelWidth()) / 2 + theX, theY + BUTTON_Y);
}

// 0x457950: the gnome moves, and the light turns, unless the how-to-play movie or the game-won box is up
void LevelIntroWidget::Update()
{
	Widget::Update();
	if (!HowToPlayWidget::IsActive() && gApp->GetDialog(DIALOG_GAME_WON) == nullptr)
	{
		double aPosition[2] = { 0.0, 0.0 };
		mCreature->Update(aPosition);
		mTime += TIME_STEP;
	}
}

// 0x4579c0: the light turning behind the gnome, the gnome (the aviator and the jester are drawn by functions of their own, with a
// cel that runs on with the time), the heading and the lines
void LevelIntroWidget::Draw(Graphics* g)
{
	Graphics aG(*g);

	int aCreatureY;
	int aMarkOffsetY;
	GetIntroPlacement(mCreature, aCreatureY, aMarkOffsetY);

	int aHalfHeight = IMAGE_DIVINELIGHT_SMALL->mHeight / 2;
	int aHalfWidth = IMAGE_DIVINELIGHT_SMALL->mWidth / 2;
	aG.DrawImageRotated(IMAGE_DIVINELIGHT_SMALL, mWidth / 2 - aHalfWidth, aCreatureY - aHalfHeight + aMarkOffsetY - LIGHT_RAISE,
		mTime * LIGHT_TURN_RATE, aHalfWidth, aHalfHeight, nullptr);

	if (mCreature->mKind == KIND_FLYING)
		DrawFlyingIntro(g, (double) (mWidth / 2), (double) aCreatureY, ToInt(mTime * FLYING_CEL_RATE) % FLYING_CELS);
	else if (mCreature->mKind == KIND_BOUNCING)
		DrawBouncingIntro(g, (double) (mWidth / 2), (double) aCreatureY, ToInt(mTime * BOUNCING_CEL_RATE) % BOUNCING_CELS);
	else
		mCreature->Draw(g, (double) (mWidth / 2), (double) aCreatureY, 0, 0.0);

	aG.SetColor(Color(255, 255, 255, 255));
	aG.SetFont(FONT_PTBANANASPLIT27);
	WriteCenteredLine(&aG, HEADING_Y, mLines[0]);

	aG.SetColor(Color(255, 255, 255, 255));
	aG.SetFont(FONT_EUROFURENCE14BOLD);
	int aLineY = FIRST_LINE_Y;
	for (int aLine = 1; aLine < (int) mLines.size(); aLine++)
	{
		WriteCenteredLine(&aG, aLineY, mLines[aLine]);
		aLineY += LINE_SPACING;
	}
}

// 0x457910: it has gone out of sight: the page is taken down and the game goes on
void LevelIntroWidget::Closed()
{
	if (mCloseResult == 0)
	{
		gApp->mWidgetManager->RemoveWidget(this);
		gApp->SafeDeleteWidget(this);
		gApp->LevelIntroDone();
	}
}

// 0x4578f0: the button sends it down
void LevelIntroWidget::ButtonDepress(int theId)
{
	if (theId == BUTTON_CONTINUE)
	{
		mCloseResult = 0;
		mLeaving = true;
	}
}

// 0x4580b0
LevelIntroWidget* Sexy::MakeLevelIntro(GameScreen* theGameScreen)
{
	int aLevel = theGameScreen->mLevel - 1;
	if (theGameScreen->mMode != GAME_MODE_MARATHON && theGameScreen->mMode != GAME_MODE_TOUGH_COOKIE && HasIntroFile(aLevel))
		return new LevelIntroWidget(aLevel);
	return nullptr;
}
