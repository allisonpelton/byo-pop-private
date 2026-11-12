#ifndef __HAMMERHEADS_STICKER_H__
#define __HAMMERHEADS_STICKER_H__

// A picture on the board that the hammer can strike (0x442f60-0x443430): the snails, shells and mushrooms lying about. Each
// is worth 500 points once, and puffs a cloud of leaves. Also the banners of the speed bonuses (0x442da0).

#include "Object.h"

namespace Sexy
{

class Image;

enum
{
	NUM_STICKER_KINDS = 28
};

class Sticker : public SimpleObject
{
public:
	int				mKind;				// +0xa0: which picture (a place in the exe's list of the sticker images, as the boards give it)
	bool			mHit;				// +0xa4: it has been struck and is gone
	double			mCenterX;			// +0xa8
	double			mCenterY;			// +0xb0

public:
	Sticker(double theX, double theY, int theKind);								// 0x443160
	virtual ~Sticker();															// 0x442f60

	OBJECT_TYPE(Sticker)

	virtual void	FixedUpdate() override;										// 0x442f70
	virtual void	Draw(Graphics* g) override;									// 0x442fa0
	virtual bool	Hit(int theX, int theY, const std::vector<Point>& theMask, int theWidth, int theHeight) override;	// 0x4432e0
	virtual void	Strike(double theX, double theY) override;					// 0x443050

	void			SetHit(bool theHit);										// 0x443120: a saved level brings it back as it was
	void			GetStrikePoint(double* thePoint) const;						// 0x443130: where a strike on it is aimed
};

// 0x426700: the picture of a kind
Image* GetStickerImage(int theKind);

// 0x442da0: the banner of a speed bonus (0 to 4, hasty to giga) at the point, worth 250, 500, 750, 1000 or 2000 points
void MakeSpeedSticker(int theTier, double theX, double theY);

}

#endif
