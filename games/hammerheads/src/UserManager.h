#ifndef __HAMMERHEADS_USERMANAGER_H__
#define __HAMMERHEADS_USERMANAGER_H__

// The player roster and what each player has earned (0x471380-0x473710): the trophies, the high scores, the unlocked modes.

#include <string>
#include <vector>

namespace Sexy
{

// The trophies, in the order of the exe's own list (0x42d0a0 awards them)
enum Trophy
{
	TROPHY_GNOME_CRUSHER = 0,			// 100 gnomes smashed
	TROPHY_ULTIMATE_GNOME_CRUSHER,		// 1000
	TROPHY_BLAST_MASTER,
	TROPHY_SUPREME_BLAST_MASTER,
	TROPHY_ACCURACY_AWARD,
	TROPHY_MASTER_MARKSMAN,
	TROPHY_NO_GNOME_ESCAPED,
	TROPHY_SUPPLY_CHALLENGE,
	TROPHY_GOLDEN_HEART,
	TROPHY_VOW_OF_POVERTY,
	TROPHY_HEART_HOG,
	TROPHY_SILVER_MARATHON_MEDAL,
	TROPHY_GOLD_MARATHON_MEDAL,
	TROPHY_CLASSIC_SCORE_RIBBON,
	TROPHY_MARATHON_SCORE_RIBBON,
	TROPHY_REGICIST,
	TROPHY_BONUS_BANKER,
	TROPHY_PULVERIZER,					// 2500 gnomes smashed
	TROPHY_MOGUL,						// 2500 coins
	TROPHY_POWER_PLAYER,
	TROPHY_TROPHY_MASTER,
	NUM_TROPHIES
};

class User
{
public:
	bool			HasTrophy(int theTrophy) const;
};

class UserManager
{
public:
	User*			GetCurrentUser();
};

}

#endif
