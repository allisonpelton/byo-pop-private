#include "IntroCreature.h"

#include "Application.h"
#include "Common.h"
#include "Creature.h"
#include "GnomeBomb.h"
#include "GnomeBonus.h"
#include "GnomeBubbleHead.h"
#include "GnomeChallenge.h"
#include "GnomeChest.h"
#include "GnomeClover.h"
#include "GnomeCupido.h"
#include "GnomeForkyChap.h"
#include "GnomeHide.h"
#include "GnomeKing.h"
#include "GnomeMultiplier.h"
#include "GnomeNapoleon.h"
#include "GnomePowerUp.h"
#include "GnomeRabbit.h"
#include "GnomeSave.h"
#include "GnomeShop.h"
#include "GnomeSilver.h"
#include "GnomeStrawberry.h"
#include "GnomeStrong.h"
#include "GnomeSwitcher.h"
#include "GnomeWoman.h"

using namespace Sexy;

namespace
{

// Where the creatures stand on the page: the forky chap is taller than the others
const int FEET_Y = 0xfe;
const int FEET_Y_FORKY_CHAP = 0x11d;

// How far the question mark of the game-over box is moved down for the creatures that are not as tall as the gnomes
const int MARK_OFFSET_SHORT = 0x12;
const int MARK_OFFSET_SHORTER = 0x1e;
const int MARK_OFFSET_KING = -15;

// How the chest of the page stands: open, with the coins flying
const double CHEST_TIMER = 1.0;

}

// 0x457c30
std::string Sexy::GetIntroFileName(int theLevel)
{
	return StrFormat("gnomedata\\%d.txt", theLevel);
}

// 0x457c60
bool Sexy::HasIntroFile(int theLevel)
{
	return gApp->FileExists(GetIntroFileName(theLevel));
}

// 0x457100
Creature* Sexy::MakeIntroCreature(int theIntroIndex)
{
	switch (theIntroIndex)
	{
	case 0: return new Creature(KIND_GNOME);
	case 1: return new GnomeWoman();
	case 2: return new GnomeShop();
	case 3: return new GnomeStrong();
	case 4: return new GnomeForkyChap();
	case 5: return new GnomeSilver();
	case 6: return new GnomeBomb();
	case 7: return new GnomeBubbleHead();
	case 8: return new GnomeHide();
	case 9: return new GnomeChallenge();
	case 10: return new GnomeMultiplier();
	case 11: return new GnomeCupido();
	case 12:
	{
		GnomeChest* aChest = new GnomeChest();
		aChest->mStage = GnomeChest::STAGE_THROWING;
		aChest->mTimer = CHEST_TIMER;
		return aChest;
	}
	case 13: return new GnomeClover();
	case 14: return new GnomeKing(true);
	case 15: return new GnomeRabbit();
	case 16: return new Creature(KIND_BOUNCING);
	case 17: return new GnomeBonus(KIND_BONUS_DUCK);
	case 18: return new GnomeBonus(KIND_BONUS_ICE_CREAM);
	case 19: return new GnomeStrawberry();
	case 20: return new GnomeSave();
	case 21: return new Creature(KIND_FLYING);
	case 22: return new GnomeNapoleon();
	case 23: return new GnomePowerUp();
	case 24: return new GnomeSwitcher();
	default: return nullptr;
	}
}

// 0x457870
void Sexy::GetIntroPlacement(const Creature* theCreature, int& theY, int& theMarkOffsetY)
{
	int aKind = theCreature->mKind;
	if (aKind == KIND_FORKY_CHAP)
		theY = FEET_Y_FORKY_CHAP;
	else
	{
		theY = FEET_Y;
		if (aKind == KIND_MULTIPLIER || aKind == KIND_CHEST || aKind == KIND_CLOVER)
		{
			theMarkOffsetY = MARK_OFFSET_SHORT;
			return;
		}
		if (aKind == KIND_BONUS_DUCK || aKind == KIND_STRAWBERRY || aKind == KIND_BONUS_ICE_CREAM || aKind == KIND_SAVE)
		{
			theMarkOffsetY = MARK_OFFSET_SHORTER;
			return;
		}
		if (aKind == KIND_KING)
		{
			theMarkOffsetY = MARK_OFFSET_KING;
			return;
		}
	}
	theMarkOffsetY = 0;
}
