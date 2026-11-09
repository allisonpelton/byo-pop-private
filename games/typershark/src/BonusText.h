#ifndef __TYPERSHARK_BONUSTEXT_H__
#define __TYPERSHARK_BONUSTEXT_H__

// BonusText.cpp: the text that floats up from a place on the board (the points of a word, a bonus...) and fades out:
// the colour goes from gold (255, 180, 60) through red to nothing, the alpha last. The board makes them with
// AddBonusText (the text, x, y and a font that defaults to the app's font at +0x8b8), which sets the
// position after the constructor, and removes one in state 1.
// Original: class Sexy::BonusText : GameObject (the only base, so the GameObject vptr is at +0 and the slots are direct),
// vftable 0x531a08 (deleting destructor 0x462260, Update 0x460fd0, Draw 0x4610c0, the rest GameObject's), sizeof 0x120;
// constructor 0x4621a0 (the text is a std::string passed by value, then the font), part destructor 0x462290. The field
// comments give the offsets in the original's object.
//
// States (GameObject::mState): 0 rising and fading, 1 finished (not drawn, not updated).

#include "GameObject.h"
#include <string>

namespace Sexy
{

class Font;
class Graphics;

class BonusText : public GameObject
{
public:
	int						mRed;					// +0xf0 255
	int						mGreen;					// +0xf4 180; fades to 0 after the blue
	int						mBlue;					// +0xf8 60; fades to 0 first
	int						mAlpha;					// +0xfc 255; fades last
	std::string				mText;					// +0x100 constructor argument 1
	Font*					mFont;					// +0x11c constructor argument 2

public:
	BonusText(const std::string& theText, Font* theFont);							// 0x4621a0
	virtual ~BonusText();															// deleting destructor 0x462260

	virtual void			Update() override;										// slot 1, 0x460fd0
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x4610c0
};

}

#endif
