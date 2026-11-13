#ifndef __HAMMERHEADS_CUBBY_H__
#define __HAMMERHEADS_CUBBY_H__

// A hole in the ground that a creature comes up out of (0x431c70-0x434360). The boards place them; each one waits, rises
// with a creature on it (SetState 1), holds it up for a time that depends on the level (2) and lets it sink (3). A strike
// of the hammer that lands on its creature is handled here (Strike).

#include <string>

#include "Object.h"

namespace Sexy
{

class Creature;
class ImageBlast;
class MemoryImage;
class Image;

// What a saved game keeps of one cubby (the original's struct at 0x434360 / 0x433810)
struct CubbyState
{
	std::string		mName;
	bool			mActive;
	int				mState;
	int				mVariant;
	double			mRise;
	double			mTimer;
	int				mCreatureCount;
	bool			mHasCreature;
	// the creature, when there is one
	int				mCreatureState;
	bool			mFinished;
	bool			mSpent;
	bool			mUntouchable;
	int				mKind;
	double			mHeight;
	double			mHeightVelocity;
	double			mGravity;
	int				mHealth;
	int				mFrozenTimer;
	bool			mShatters;
	std::string		mExtra;
};

class Cubby : public SimpleObject
{
public:
	enum
	{
		STATE_EMPTY = 0,
		STATE_RISING = 1,
		STATE_UP = 2,
		STATE_SINKING = 3
	};

	int				mState;					// +0xa0
	int				mVariant;				// +0xa4: Rand() % 6 for a cubby the editor made; only saved and restored
	double			mRise;					// +0xa8: how far up, 0 to 1
	double			mTimer;					// +0xb0: empty: the wait before the next creature; up: the time left
	Creature*		mCreature;				// +0xb8
	MemoryImage*	mCover;					// +0xbc: the backdrop under the cubby as a mask (see the constructor), drawn over the creature
	Image*			mBackImage;				// +0xc0: the hole
	Image*			mPlatformImage;			// +0xc4
	int				mCreatureCount;			// +0xc8: how many creatures have come up
	double			mRiseSpeed;				// +0xd0: of mRise a second (0.008 until the first rise)
	ImageBlast*		mPieces;				// +0xd8: the pieces of the creature just smashed

public:
	Cubby(double theX, double theY, int theVariant, Image* theBackdrop);	// 0x433080
	virtual ~Cubby();							// 0x431f90
	OBJECT_TYPE(Cubby)

	virtual void	FixedUpdate() override;		// 0x4334d0
	virtual void	Update(float theFrac) override;	// 0x4336a0
	virtual void	Draw(Graphics* g) override;	// 0x432b60
	virtual bool	MouseOver(int theX, int theY) override;	// 0x432120: the creature has a solid pixel there
	virtual bool	Hit(int theX, int theY, const std::vector<Point>& theMask, int theWidth, int theHeight) override;	// 0x4321b0
	virtual void	Strike(double theX, double theY) override;	// 0x433a00

	void			DrawCover(Graphics* g);		// 0x431c70
	void			SetState(int theState, int theKind);	// 0x432d60: theKind picks the creature for state 1 (-1 asks the level)
	void			Spawn(int theKind);			// 0x432290: puts a new creature of the kind on the cubby
	void			ReleaseCreature();			// 0x431cb0: deletes the creature without a trace
	void			UpdateHitRect();			// 0x432040: the part of the creature that can be hit, in mBounds

	void			GetCreaturePosition(double thePos[2]) const;	// 0x431be0: where the creature stands
	void			GetPlatformPosition(double thePos[2]) const;	// 0x431b30: where the platform is drawn
	void			GetPlatformCenter(double thePos[2]) const;		// 0x431b80
	void			GetStrikePoint(double thePos[2]) const;			// 0x431cf0: where the hammer lands on the creature
	void			GetFloaterAnchor(double thePos[2]) const;		// 0x431d90: where a floating message starts
	Rect			GetEffectRect() const;							// 0x432ae0: what the cubby may draw outside its own area
	int				GetCreatureCount() const;						// 0x431ea0
	bool			IsSinking() const;								// 0x431eb0
	bool			IsRising() const;								// 0x431ec0

	void			SaveState(CubbyState& theState) const;			// 0x434360
	void			LoadState(const CubbyState& theState);			// 0x433810

	static void		FreezeOrdinary(Creature* theCreature);			// 0x431e20: freezes a creature of an ordinary kind
};

// Over every cubby of the level (the manager's list, last first)
int				CountHeartCreatures();							// 0x432870: hearts up on the level
bool			IsGiftReady();									// 0x4328d0: the shop's gift box can be opened
int				CountCreaturesOfKind(int theKind);				// 0x432950: up and alive on an active cubby
void			FreezeAll();									// 0x4329c0
void			ExplodeAll(Creature* theExcept);				// 0x432a00: breaks every creature up (a bomb)
void			ConvertToHearts(Creature* theExcept);			// 0x4344d0: a creature that is up and ordinary becomes a heart

}

#endif
