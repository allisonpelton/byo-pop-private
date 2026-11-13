#ifndef __HAMMERHEADS_CREATURE_H__
#define __HAMMERHEADS_CREATURE_H__

// What comes up out of a cubby: a gnome, a heart or coin, the shop and challenge signs, the shop's items. A Creature is
// not an Object; the cubby that holds it moves it and draws it. The kinds and their classes are in the table of
// Creature.cpp; the gnome classes (GnomeBomb and the rest) change what a kind does.

#include <vector>
#include <string>

#include "Rect.h"
#include "Point.h"

namespace Sexy
{

class Graphics;
class Image;
class Cubby;
class ImageBlast;

enum CreatureKind
{
	KIND_GNOME = 0,
	KIND_WOMAN = 1,
	KIND_STRONG = 2,
	KIND_BOMB = 3,
	KIND_BUBBLEHEAD = 4,
	KIND_HIDE = 5,
	KIND_FORKY_CHAP = 6,
	KIND_SILVER = 7,
	KIND_MULTIPLIER = 8,
	KIND_CUPIDO = 9,
	KIND_BOUNCING = 10,
	KIND_FLYING = 11,
	KIND_HEART = 12,
	KIND_COIN = 13,
	KIND_BIG_COIN = 14,
	KIND_SHOP = 15,
	KIND_CHALLENGE = 16,
	KIND_CHEST = 17,
	KIND_CLOVER = 18,
	KIND_BONUS_ICE_CREAM = 19,
	KIND_BONUS_DUCK = 20,
	KIND_STRAWBERRY = 21,
	KIND_RABBIT = 22,
	KIND_KING = 23,
	KIND_SAVE = 24,
	KIND_NAPOLEON = 25,
	KIND_POWER_UP = 26,
	KIND_SWITCHER = 27,
	KIND_SHOP_HEART = 28,
	KIND_SHOP_BIG_HEART = 29,
	KIND_SHOP_EXTRA_HEART = 30,
	KIND_SHOP_FLAME = 31,
	KIND_SHOP_HAMMER_2 = 32,
	KIND_SHOP_HAMMER_3 = 33,
	KIND_SHOP_HAMMER_4 = 34,
	KIND_SHOP_ICE_CREAM = 35,
	KIND_SHOP_CLOVER = 36,
	KIND_SHOP_STRAWBERRY = 37,
	KIND_SHOP_GIFT = 38,
	NUM_KINDS = 39,
	ITEM_NONE = 39					// in a creature's list of carried items: nothing
};

class Creature
{
public:
	int				mState;				// 0 waiting; SetState(1) gives a creature standing on the ground a hop
	int				mKind;
	Image*			mImage;				// the sheet the creature is drawn from
	Image*			mGlowImage;			// the same sheet as white silhouettes (GetGlowImage)
	Image*			mShadowImage;
	double			mAnchorX;			// where the sheet's cell is held relative to the creature's position
	double			mAnchorY;
	double			mHeadX;				// where a hammer strike lands, in the cell
	double			mHeadY;
	double			mHeight;			// above the platform
	double			mHeightVelocity;
	double			mGravity;
	int				mHealth;			// hits it takes
	int				mScore;
	bool			mCountsTowardLimit;	// counts against the level's limit of creatures up at once (Gameplay's pops)
	std::vector<int> mCarriedItems;		// the kinds it may drop; one is chosen the first time it is asked (GetCarriedItem)
	int				mAge;
	int				mFrozenTimer;		// above 0 the creature is frozen; set to 600
	bool			mDarkened;			// drawn with black over it
	bool			mShielded;			// a hit that takes no health off shows a different message
	bool			mShatters;			// SpawnPieces breaks it into pieces even when it is not frozen
	int				mEscapePenalty;		// the health the player loses when it gets away alive
	int				mVariant;			// 3 for the bomb and the switcher, 1 for the stronger gnomes (read by the gnome classes)
	bool			mIsGnome;			// a gnome counts for the smashed total
	bool			mUntouchable;		// set by Remove: the hammer and the hover tests skip it
	bool			mFinished;			// set by Remove together with mUntouchable
	bool			mSpent;				// set by the chest when it opens (with mFinished)
	double			mX;					// where Update last put it
	double			mY;
	double			mTopY;				// the highest it has been
	double			mExposed;			// how far it has risen above its top
	double			mGlow;				// 0 to 1, how much its glow shows while the hammer is over it
	Cubby*			mCubby;
	int				mCarriedItem;		// -1 until chosen
	bool			mCubbyReleased;		// set by the cubby just before it deletes the creature

public:
	explicit Creature(int theKind);								// 0x430340
	virtual ~Creature();										// 0x42f8c0

	virtual void	GetSheetCell(double theCell[2]) const;		// 0x42f730: row, column of the sheet's cell to draw
	virtual void	Update(const double thePos[2]);				// 0x42f410
	virtual void	UpdateWithoutCubby(const double thePos[2]);	// 0x42f400: Update for a creature that is in no cubby (the level introduction's and the game-over box's gnome); ages it
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime);	// 0x4316d0
	virtual bool	PixelHit(const double thePos[2], int theX, int theY);	// 0x42f750
	virtual bool	MaskHit(const double thePos[2], int theX, int theY, const std::vector<Point>& theMask, int theRectX, int theRectY, int theRectW, int theRectH);	// 0x4300a0
	virtual bool	Hit();										// 0x42f6f0: the hammer's damage; true when the health is gone
	virtual void	Unused20();									// 0x408d10
	virtual std::string SaveExtra();							// 0x431ac0: what the creature adds to a saved game (a ValueList's text); nothing here
	virtual void	LoadExtra(const std::string& theData);		// 0x4314b0: reads it back
	virtual int		GetScore();									// 0x42fa80
	virtual void	Remove();									// 0x42fa90
	virtual ImageBlast* SpawnPieces(bool theFlag);				// 0x4301f0: the pieces it breaks into (the cubby draws them)
	virtual int		GetHealth();								// 0x42faa0
	virtual void	SetFrozen(bool theFrozen);					// 0x42f350

	void			SetState(int theState);						// 0x42f6b0
	void			SetCubby(Cubby* theCubby);					// 0x42f8a0
	void			ClearEscapePenalty();						// 0x42f8b0
	bool			IsFrozen() const;							// 0x42f340
	int				GetCarriedItem();							// 0x4318b0: the kind it drops, chosen on the first call
	int				FlashAlpha(double theTime) const;			// 0x42f620
	Rect			GetScreenRect(const double thePos[2]) const;	// 0x430040: where the creature's cell is drawn when it stands at thePos
	void			DrawShadow(Graphics* g, double theX, double theY);	// 0x42f550
	void			DrawBody(Graphics* g, double theX, double theY, int theCol, int theRow, int theFlash, double theAlpha);	// 0x42fab0
	void			DrawHealth(Graphics* g, double theX, double theY, int theHealth);	// 0x4314d0
	void			GetPositionInCell(double thePos[2], double theX, double theY) const;	// 0x42f600
};

// The creatures that exist (0x597f30)
extern std::vector<Creature*> gCreatures;

// 0x432290: a creature of the class that goes with its kind (the factory the cubby calls); theCubbyY is the height of the
// cubby on the screen, which only the shop's items use
Creature* MakeCreature(int theKind, double theCubbyY);

// 0x41c4c0: the sheet of white silhouettes that goes with a creature's sheet, made on first use and kept
Image* GetGlowImage(Image* theImage);

// 0x41d040: a ramp that rises from 0 at theStart, runs at one speed and settles at 1 by theEnd
double Ease(double theValue, double theStart, double theEnd);

}

#endif
