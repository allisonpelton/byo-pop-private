#ifndef __BOOKWORM_BIGLEX_H__
#define __BOOKWORM_BIGLEX_H__

// BigLex.cpp: the big Lex mouth over the game-selector backdrop. BWGameSelector makes it in its ctor
// (Resize(70, 231, 133, 123)) and plays the "Welcome" lip-sync once per run (PlayAnim(0, false) with
// SOUND_LEX_WELCOME at the selector's update 100). The closed mouth is part of the backdrop: frame 0
// draws nothing.
// Original: class Sexy::BigLex : public Widget, sizeof 0x70 (new(0x70) in the BWGameSelector ctor
// 0x401600), vftable 0x4f9724 (47 slots; overrides 0 dtor, 16 Draw, 17 Update). Offsets are absolute
// (the 2004 Widget is 0x58 bytes).
// Behaviour spec: re/bookworm/analysis/lex.md section 3. The two Board helpers that sit in BigLex.cpp's
// address range (0x402f60, 0x402fd0) are Board::IsAdjacent / Board::SetActionSpeed (NAMES.md D14).

#include "Widget.h"
#include "Lex.h"

namespace Sexy
{

class BookWormApp;
class Graphics;

enum
{
	NUM_BIGLEX_ANIMS = 1            // row 0: "Welcome"
};

extern const LexAnimFrame gBigLexAnims[NUM_BIGLEX_ANIMS][LEX_ANIM_STEPS];  // 0x4f9590, defined in BigLex.cpp

class BigLex : public Widget
{
public:
	BookWormApp*	mApp;                   // +0x58 ctor arg
	int				mAnim;                  // +0x5c -1; row of gBigLexAnims, -1 none
	int				mAnimStep;              // +0x60 0
	int				mAnimTick;              // +0x64 0
	int				mFrame;                 // +0x68 -1; image id, drawn only when > 0
	bool			mAnimLoop;              // +0x6c false
	// +0x6d..+0x6f pad; sizeof 0x70

public:
	BigLex(BookWormApp* theApp);                                    // 0x402df0
	virtual ~BigLex();                                              // 0x402e30 (deleting dtor 0x402f30) slot 0

	virtual void	Draw(Graphics* g) override;                     // 0x402e80 slot 16
	virtual void	Update() override;                              // 0x402eb0 slot 17

	void			PlayAnim(int theAnim, bool theLoop);            // 0x402e40
};

}

#endif
