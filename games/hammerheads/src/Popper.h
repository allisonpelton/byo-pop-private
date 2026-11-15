#ifndef __HAMMERHEADS_POPPER_H__
#define __HAMMERHEADS_POPPER_H__

// A burst of particles at a point (0x440830-0x441100): stars when a gnome is hit, hearts, sparkles, ice, dust and the
// explosion of a bomb. It plays one effect of the particle library, or the bomb's picture cels, and is gone when the effect
// is.

#include "Object.h"

namespace Sexy
{

class ParticleEffect;
struct EmitterDef;

enum
{
	POPPER_STARS = 0,
	POPPER_STARS_FAST = 1,
	POPPER_SPARKLES = 2,
	POPPER_STICKER = 3,
	POPPER_FREEZE = 4,
	POPPER_HEARTS = 5,
	POPPER_EXPLOSION = 6,		// the cels of IMAGE_GNOME_EXPLOSION, no particles
	POPPER_DUST = 7,
	POPPER_DUST_KING = 8
};

class Popper : public Object
{
public:
	int				mType;			// +0x80
	EmitterDef*		mEmitter;		// +0x84: this popper's own copy of the emitter's definition (the direction is set in it)
	ParticleEffect*	mEffect;		// +0x88
	double			mX;				// +0x90
	double			mY;				// +0x98

public:
	// 0x440830: theDirection (x and y) sends the stars, sparkles and hearts the way it points; the other types ignore it
	Popper(double theX, double theY, int theType, const double* theDirection);
	virtual ~Popper();											// 0x441060

	OBJECT_TYPE(Popper)

	virtual void	FixedUpdate() override;						// 0x440d90
	virtual void	Draw(Graphics* g) override;					// 0x440e30
};

// 0x441100: makes a popper and gives it a blank name
Popper* MakePopper(double theX, double theY, int theType, const double* theDirection);

}

#endif
