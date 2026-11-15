#include "LevelHud.h"

#include <cmath>
#include <cstdio>
#include <string>

#include "Application.h"
#include "Color.h"
#include "DrawUtil.h"
#include "Floater.h"
#include "Font.h"
#include "GameScreen.h"
#include "Gameplay.h"
#include "Graphics.h"
#include "Image.h"
#include "Rect.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

static ObjectRef gLevelIndicatorRef;		// 0x598000
static ObjectRef gMarathonMeterRef;			// 0x598020

// 0x44a820
LevelIndicator::LevelIndicator() :
	Object(0x80)
{
}

LevelIndicator::~LevelIndicator()
{
}

// 0x44a820
LevelIndicator* Sexy::MakeLevelIndicator()
{
	LevelIndicator* anIndicator = new LevelIndicator();
	anIndicator->SetName("");
	gLevelIndicatorRef = ObjectRef(anIndicator);
	return anIndicator;
}

// 0x44a6b0
void LevelIndicator::Draw(Graphics* g)
{
	if ((gApp->mGameMode != GAME_MODE_CLASSIC && gApp->mGameMode != GAME_MODE_TOUGH_COOKIE) || mDead)
		return;

	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_PTBANANASPLIT27INGAMEUI);

	char aBuffer[32];
	snprintf(aBuffer, sizeof(aBuffer), "%d", gApp->mGameScreen->mLevel);
	std::string aText = aBuffer;

	int aBoxWidth = g->GetFont()->StringWidth(aText) + 0x14;
	if (aBoxWidth < 0x3a)
		aBoxWidth = 0x3a;
	g->DrawImageBox(Rect(0x4b - aBoxWidth, 8, aBoxWidth, IMAGE_SCOREBACK->mHeight), IMAGE_SCOREBACK);
	g->DrawString(aText, 0x41 - g->GetFont()->StringWidth(aText), 0x23);
	g->DrawImage(IMAGE_INGAMEUI_LEVEL, 0x41 - IMAGE_INGAMEUI_LEVEL->mWidth, 0x22);
}

// 0x44a910
MarathonMeter::MarathonMeter() :
	Object(0x80),
	mFlash(0.0)
{
}

// 0x44a980
MarathonMeter::~MarathonMeter()
{
}

// 0x44a9b0
MarathonMeter* Sexy::MakeMarathonMeter()
{
	MarathonMeter* aMeter = new MarathonMeter();
	aMeter->SetName("");
	gMarathonMeterRef = ObjectRef(aMeter);
	return aMeter;
}

// 0x44aa90
void MarathonMeter::Update(float theFrac)
{
	Object::Update(theFrac);

	mFlash = mFlash - (double)theFrac * 0.01;
	if (mFlash < 0.0)
		mFlash = 0.0;

	if (GetGameplay()->mPhase == Gameplay::PHASE_OVER)
		return;

	double aStep = (double)theFrac * 0.01;
	double aOldTime = GetStats()->Get(STAT_MARATHON_TIME);
	double aNewTime = aStep + aOldTime;
	GetStats()->Set(STAT_MARATHON_TIME, aNewTime);

	// a minute that ends is worth 500 points for each minute the game has lasted (this one included), shattering at the
	// bottom of the screen
	int aNewSeconds = ToInt(aNewTime);
	int aOldSeconds = ToInt(aOldTime);
	if (aNewSeconds % 60 < aOldSeconds % 60)
	{
		Floater* aBonus = MakeFloater(FLOATER_SHATTER_SIDE, 640.0, 560.0, "", (aNewSeconds / 60 + 1) * 500, "");
		aBonus->SetImage(IMAGE_MARATHON_TIMEBONUS);
		gApp->PlaySample(SOUND_UI_TIME_BONUS);
		mFlash = 1.5;
	}
}

// 0x44ac30: the clock is drawn at the bottom of the screen in the middle: a face with an hour hand that goes round in twelve
// minutes and a minute hand, and the time beside it
void MarathonMeter::Draw(Graphics* g)
{
	if (mDead || gApp->mGameMode != GAME_MODE_MARATHON)
		return;

	double aTime = GetStats()->Get(STAT_MARATHON_TIME);
	int aSeconds = ToInt(aTime);
	int aHours = aSeconds / 3600;
	int aMinutes = aSeconds % 3600 / 60;
	int aSecondsOfMinute = aSeconds % 3600 % 60;

	g->SetFont(FONT_EUROSTYLE36);
	g->SetColor(Color(255, 255, 255, 255));

	// the text, and the width of a time as wide as it gets for its form so the clock does not shift as the digits change
	char aBuffer[32];
	std::string aSample;
	if (aHours > 0)
	{
		snprintf(aBuffer, sizeof(aBuffer), "%d:%02d:%02d", aHours, aMinutes, aSecondsOfMinute);
		aSample = "1:01:01";
	}
	else
	{
		snprintf(aBuffer, sizeof(aBuffer), "%d:%02d", aMinutes, aSecondsOfMinute);
		aSample = "1:01";
	}
	std::string aText = aBuffer;

	int aTextWidth = g->GetFont()->StringWidth(aSample) + 0xc;
	int aClockWidth = IMAGE_MARATHON_CLOCK->mWidth;
	int aClockX = (gApp->mWidth - aClockWidth - aTextWidth) / 2;
	int aBoxWidth = aClockWidth + aTextWidth + 0x28;
	int aBoxHeight = IMAGE_MARATHON_CLOCK_BACK->mHeight;
	g->DrawImageBox(Rect(gApp->mWidth / 2 - aBoxWidth / 2, gApp->mHeight - aBoxHeight, aBoxWidth, aBoxHeight),
		IMAGE_MARATHON_CLOCK_BACK);

	double aMinuteFraction = (((double)(aSeconds % 60) + aTime) - (double)aSeconds) * 0.016666666666666666;
	g->DrawImage(IMAGE_MARATHON_CLOCK, aClockX, 0x221);
	double aHourAngle = (((double)(aSeconds % 720) + aTime) - (double)aSeconds) * 0.001388888888888889 * -6.283185307179586;
	g->DrawImageRotated(IMAGE_MARATHON_CLOCK_HOURHAND, aClockX, 0x221, aHourAngle, nullptr);
	double aMinuteAngle = (aMinuteFraction - 0.004166666666666667) * -6.283185307179586;
	g->DrawImageRotated(IMAGE_MARATHON_CLOCK_MINUTEHAND, aClockX, 0x221, aMinuteAngle, nullptr);
	g->DrawString(aText, aClockWidth + 0xc + aClockX, IMAGE_MARATHON_CLOCK->mHeight / 2 + 0x232);

	// after a bonus the whole clock is drawn once more, added to itself, as bright as the flash is
	if (mFlash != 0.0)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aBrightness = ToInt(mFlash * 0.6666666666666666 * 255.0);
		g->SetColor(Color(aBrightness, aBrightness, aBrightness));
		g->SetColorizeImages(true);
		g->DrawImage(IMAGE_MARATHON_CLOCK, aClockX, 0x221);
		g->DrawImageRotated(IMAGE_MARATHON_CLOCK_HOURHAND, aClockX, 0x221, aHourAngle, nullptr);
		g->DrawImageRotated(IMAGE_MARATHON_CLOCK_MINUTEHAND, aClockX, 0x221, aMinuteAngle, nullptr);
		g->DrawString(aText, aClockWidth + 0xc + aClockX, IMAGE_MARATHON_CLOCK->mHeight / 2 + 0x232);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}
}
