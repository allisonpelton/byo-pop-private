#include "Score.h"

#include <cstdio>
#include <string>

#include "Application.h"
#include "Color.h"
#include "Floater.h"
#include "Font.h"
#include "Gameplay.h"
#include "Graphics.h"
#include "Image.h"
#include "GameScreen.h"
#include "Rect.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

static ObjectRef gScoreRef;		// 0x598040

// 0x44b070
Score::Score() :
	Object(0x80)
{
}

Score::~Score()
{
}

// 0x44b170
void Score::SetScore(int theScore)
{
	GetStats()->SetInt(STAT_SCORE, theScore);
}

// 0x44b190
int Score::GetScore() const
{
	return GetStats()->GetInt(STAT_SCORE);
}

// 0x44b0f0
Score* Sexy::GetScore()
{
	return dynamic_cast<Score*>(gScoreRef.Get());
}

// 0x44b8c0
Score* Sexy::MakeScore()
{
	Score* aScore = new Score();
	aScore->SetName("");
	gScoreRef.Set(aScore);
	return aScore;
}

// 0x44b1c0
ScoreDisplayPos Sexy::GetScoreDisplayPos()
{
	ScoreDisplayPos aPos = { 225.0, 35.0 };
	return aPos;
}

// 0x44b1a0
ScoreDisplayPos Sexy::GetBonusTrayPos()
{
	ScoreDisplayPos aPos = { 193.0, -13.0 };
	return aPos;
}

// 0x44b090
bool Sexy::IsNoFloaterCarrying(Image* theImage)
{
	ObjectManager* aManager = ObjectManager::Get();
	for (size_t i = 0; i < aManager->mObjects.size(); i++)
	{
		FloaterShrinking* aFloater = dynamic_cast<FloaterShrinking*>(aManager->mObjects[i]);
		if (aFloater != nullptr && aFloater->mImage == theImage)
			return false;
	}
	return true;
}

// 0x44b120
void Score::AddQuiet(int thePoints)
{
	Stats* aStats = GetStats();
	aStats->SetInt(STAT_SCORE, aStats->GetInt(STAT_SCORE) + thePoints);
	aStats->SetInt(STAT_LEVEL_SCORE, aStats->GetInt(STAT_LEVEL_SCORE) + thePoints);
}

// 0x44b9b0
void Score::AddPoints(int thePoints, int theX, int theY, int theType)
{
	char aBuffer[32];
	snprintf(aBuffer, sizeof(aBuffer), "%d", thePoints);
	std::string aText = aBuffer;

	if (GetGameplay()->mFiveTimesTimer > 0.0)
		thePoints = thePoints * 5;

	if (theType != 1)
		AddQuiet(thePoints);

	if (theType != -1)
	{
		Floater* aFloater = MakeFloater(theType, (double)theX, (double)theY, aText.c_str(), 0, "");
		if (theType == 1)
			aFloater->SetPendingPoints(thePoints);

		if (GetGameplay()->mFiveTimesTimer > 0.0)
			MakeFloater(theType, (double)theX, (double)(theY + 0x23), "x5", 0, "");
	}
}

// One of the signs under the score: the picture at half size, at its place
static void DrawSign(Graphics* g, Image* theImage, double theX, double theY)
{
	g->DrawImage(theImage, (int)theX, (int)theY, (int)((double)theImage->mWidth * 0.5), (int)((double)theImage->mHeight * 0.5));
}

// Whether the sign of a power that has theTimeLeft is hidden in this blink: in its last seconds it blinks, when the level
// allows that
static bool IsBlinkedOut(double theTimeLeft, double theBlinkTime, double theBlinkRate, bool theBlinkAllowed)
{
	if (theTimeLeft <= theBlinkTime)
	{
		int aBlink = (int)(theTimeLeft * theBlinkRate);
		if ((aBlink % 2) != 0 && theBlinkAllowed)
			return true;
	}
	return false;
}

// 0x44b200
void Score::Draw(Graphics* g)
{
	if (mDead)
		return;

	Stats* aStats = GetStats();

	if (gApp->mShopScreen != nullptr)
	{
		// in the shop: the power-ups the player carries into the level, drawn next to the sign of their shelf
		if (aStats->GetInt(STAT_HAS_ICE_CREAM) != 0 && IsNoFloaterCarrying(IMAGE_GNOME_BONUS_ICECREAM))
		{
			ScoreDisplayPos aPos = { 298.0, 30.0 };
			DrawSign(g, IMAGE_GNOME_BONUS_ICECREAM, aPos.mX + 125.0, aPos.mY - 30.0);
		}
		if (aStats->GetInt(STAT_HAS_CLOVER) != 0 && IsNoFloaterCarrying(IMAGE_LUCKY_CLOVER))
		{
			ScoreDisplayPos aPos = { 378.0, 45.0 };
			DrawSign(g, IMAGE_LUCKY_CLOVER, aPos.mX + 95.0, aPos.mY - 30.0);
		}
		if (aStats->GetInt(STAT_HAS_STRAWBERRY) != 0 && IsNoFloaterCarrying(IMAGE_GNOME_BONUS_STRAWBERRY))
		{
			ScoreDisplayPos aPos = { 338.0, 45.0 };
			DrawSign(g, IMAGE_GNOME_BONUS_STRAWBERRY, aPos.mX + 110.0, aPos.mY - 30.0);
		}
		return;
	}

	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_PTBANANASPLIT27INGAMEUI);
	char aBuffer[32];
	snprintf(aBuffer, sizeof(aBuffer), "%d", aStats->GetInt(STAT_SCORE));
	std::string aText = aBuffer;
	int aTextWidth = g->GetFont()->StringWidth(aText);

	int aBoxWidth = aTextWidth + 0x14;
	if (aBoxWidth < 0x3a)
		aBoxWidth = 0x3a;
	g->DrawImageBox(Rect(0xeb - aBoxWidth, 8, aBoxWidth, IMAGE_SCOREBACK->mHeight), IMAGE_SCOREBACK);
	g->DrawString(aText, 0xe1 - aTextWidth, 0x23);
	g->DrawImage(IMAGE_INGAMEUI_SCORE, 0xe1 - IMAGE_INGAMEUI_SCORE->mWidth, 0x23);

	bool aBlinkAllowed = gApp->mGameScreen->IsLevelRunning();
	Gameplay* aGameplay = GetGameplay();

	// the x5 sign: it blinks in the last four seconds
	if (aGameplay->mFiveTimesDelay <= 0.0 && aGameplay->mFiveTimesTimer > 0.0)
	{
		if (!IsBlinkedOut(GetGameplay()->mFiveTimesTimer, 4.0, 5.0, aBlinkAllowed))
		{
			Rect aDest(0xc1, -13, (int)((double)IMAGE_MOLE_X5->GetCelWidth() * 0.5), (int)((double)IMAGE_MOLE_X5->GetCelHeight() * 0.5));
			g->DrawImage(IMAGE_MOLE_X5, aDest, IMAGE_MOLE_X5->GetCelRect(0, 2));
		}
	}

	double aFreezeTime = aStats->Get(STAT_FREEZE_TIMER);
	if (aFreezeTime > 0.0)
	{
		if (!IsBlinkedOut(aFreezeTime, 2.0, 10.0, aBlinkAllowed) && IsNoFloaterCarrying(IMAGE_GNOME_BONUS_ICECREAM))
			DrawSign(g, IMAGE_GNOME_BONUS_ICECREAM, 298.0, 30.0);
	}

	aGameplay = GetGameplay();
	if (aGameplay->mCloverDelay <= 0.0 && aGameplay->mCloverTimer > 0.0)
	{
		if (!IsBlinkedOut(GetGameplay()->mCloverTimer, 2.0, 10.0, aBlinkAllowed))
			DrawSign(g, IMAGE_LUCKY_CLOVER, 378.0, 45.0);
	}

	aGameplay = GetGameplay();
	if (aGameplay->mDoubleHitDelay <= 0.0 && aGameplay->mDoubleHitTimer > 0.0)
	{
		if (!IsBlinkedOut(GetGameplay()->mDoubleHitTimer, 2.0, 10.0, aBlinkAllowed))
			DrawSign(g, IMAGE_GNOME_POWERUP, 415.0, 20.0);
	}

	aGameplay = GetGameplay();
	if (aGameplay->mStrawberryDelay <= 0.0 && aGameplay->mStrawberryTimer > 0.0)
	{
		if (!IsBlinkedOut(GetGameplay()->mStrawberryTimer, 2.0, 10.0, aBlinkAllowed))
			DrawSign(g, IMAGE_GNOME_BONUS_STRAWBERRY, 338.0, 45.0);
	}
}
