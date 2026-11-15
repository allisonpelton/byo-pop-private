#ifndef __STARFIELD_H__
#define __STARFIELD_H__

// StarField.h: the field of stars or of linked dots that two of the cheat words put behind the board (Sexy::StarField,
// 0x5d8ffc-0x5d9...; the object is 0x1c bytes). Board::mCheatStars (+0x165bc) holds it; Board::Update updates it and
// Board::Draw draws it. The offsets in the comments are the original's, from the start of the object.

#include <list>

namespace Sexy
{

class Graphics;
class Image;

class StarField
{
public:
	// one star or dot (the original's list node holds these 0x1c bytes after its two links)
	struct Star
	{
		float		mX;                 // +0x0
		float		mY;                 // +0x4
		float		mVX;                // +0x8
		float		mVY;                // +0xc
		int			mColor;             // +0x10 0xRRGGBB, grey
		int			mText;              // +0x14 the row of 0 and 1 that the dot draws (network)
		float		mFlash;             // +0x18 0 or how far the flash has got (network)
	};

	std::list<Star>	mStars;             // +0x4
	int				mTarget;            // +0x10 the stars that Update keeps up
	Image*			mImage;             // +0x14 the nebula behind the stars (images/nebula1), none for the dots
	bool			mNetwork;           // +0x18 the second kind: dots that are linked by lines (NETWORK), not stars (STARFIELD)

public:
	StarField();                        // 0x5d8ffc
	virtual ~StarField();

	void			Init(int theCount, bool theNetwork);    // 0x5d95c7
	void			AddStar(int theX, int theY);            // 0x5d93ee
	void			Update();                               // 0x5d968a
	void			Draw(Graphics* g, bool theWavy);        // 0x5d9096
};

}

#endif
