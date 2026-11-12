#ifndef __HAMMERHEADS_SHOPITEM_H__
#define __HAMMERHEADS_SHOPITEM_H__

// What the shop sells (0x4411d0-0x442cd0): each item is a creature, kinds 28 to 38, that stands on a pedestal in a cubby of
// the shop screen and is bought by hitting it. ShopItem is the plain one (a heart, a hammer, a power-up), ShopItemFlame
// the flame that burns (it picks its cel from a counter), ShopItemGift the gift that plays a roll of prizes before it gives one.

#include <string>

#include "Creature.h"

namespace Sexy
{

class Image;

// The reasons the shop's keeper gives for a refusal (the codes of ShopScreen's mood, 0x45df70)
enum ShopRefusal
{
	REFUSE_NO_COINS = 4,
	REFUSE_HAVE_HAMMER = 5,				// that hammer is the one the player has
	REFUSE_HAVE_BETTER_HAMMER = 6,		// the player has a better one
	REFUSE_HAVE_FLAME = 7,
	REFUSE_HEARTS_AT_MAXIMUM = 8,		// the capacity is seven hearts already
	REFUSE_HAVE_ICE_CREAM = 9,
	REFUSE_HAVE_CLOVER = 10,
	REFUSE_HAVE_STRAWBERRY = 11,
	REFUSE_HEALTH_FULL = 12
};

// 0x441250: whether the shop would sell the item to the player now; if not and theAnnounce is set, the shop's keeper says
// why (the reasons above). The hammers are checked first, then the money, then what each kind needs.
bool CanBuy(int theKind, int thePrice, bool theAnnounce);

// 0x42f270: where the picture of an item stands, from the top left of its cell (the anchor of a prize drawn by the gift)
void GetShopItemAnchor(double theAnchor[2], int theKind);

// 0x442570: the picture that goes with a kind
Image* GetShopItemImage(int theKind);

class ShopItem : public Creature
{
public:
	double			mPosX;				// +0xc0: where Update last put the item (the same as mX)
	double			mPosY;				// +0xc8
	double			mUnusedD0;			// +0xd0: 1.0; only saved and loaded
	int				mPrice;				// +0xd8: in coins
	double			mHeartAmount;		// +0xe0: the health a heart gives (a half and two and a half), 0 for the rest
	int				mOffsetY;			// +0xe8: how far the item stands above its cubby's line (the pedestal's height scaled by how low the cubby is)
	std::string		mDescription;		// +0xec: the keeper's line when the hammer is over the item
	std::string		mThanks;			// +0x108: the keeper's line after the item is bought
	double			mPurchaseTimer;		// +0x128: after a purchase, 0.01 less every update; at 0 the item is finished

public:
	ShopItem(int theKind, double theCubbyY);			// 0x441b40
	virtual ~ShopItem();								// 0x4415c0

	virtual void	GetSheetCell(double theCell[2]) const override;				// 0x441580
	virtual void	Update(const double thePos[2]) override;					// 0x4411d0
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime) override;	// 0x441640
	virtual bool	Hit() override;												// 0x442350
	virtual std::string SaveExtra() override;									// 0x4419c0
	virtual void	LoadExtra(const std::string& theData) override;				// 0x441a70
	virtual void	Remove() override;											// 0x441500

	// 0x441dc0: the purchase's effects: the health, the coins, the picture flying to where it goes, the keeper's thanks
	void			Purchase(double theHealth, int thePrice, int theKind);
};

// The flame (kind 31) is drawn from its sheet by a counter that goes up by a random step every update
class ShopItemFlame : public ShopItem
{
public:
	double			mFlameCounter;		// +0x130

public:
	ShopItemFlame(int theKind, double theCubbyY);		// 0x4423e0
	virtual ~ShopItemFlame();							// 0x442410

	virtual void	GetSheetCell(double theCell[2]) const override;				// 0x442460
	virtual void	Update(const double thePos[2]) override;					// 0x442420
};

// The gift (kind 38): a hit starts a roll of prizes, and the one that it stops on is bought as if it were the item itself
class ShopItemGift : public ShopItem
{
public:
	int				mOpenCount;			// +0x130: how many times it has been opened
	double			mOpenTimer;			// +0x138: after it is opened, 0.001 less every update from 1; at 0 the gift is spent
	double			mRollProgress;		// +0x140: 0 when the roll starts, 0.001 more every update to 1, where the prize is given
	int				mPrize;				// +0x148: the kind the roll stops on, -1 before a roll
	int				mNextPrize;			// +0x14c: the kind that the roll shows
	double			mLowestProgress;	// +0x150: the least progress at which the roll's picture was last the prize
	double			mGlowTurn;			// +0x158: a counter that goes up by a random step every update (the flame prize's cel)

public:
	ShopItemGift(int theKind, double theCubbyY);		// 0x4424c0
	virtual ~ShopItemGift();							// 0x442530

	virtual void	Update(const double thePos[2]) override;					// 0x442940
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime) override;	// 0x4426a0
	virtual std::string SaveExtra() override;									// 0x442bf0
	virtual void	Remove() override;											// 0x442540

	void			StartRoll(int thePrize);							// 0x442bb0: the roll begins; the prize is the one it stops on
};

}

#endif
