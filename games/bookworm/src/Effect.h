#ifndef __BOOKWORM_EFFECT_H__
#define __BOOKWORM_EFFECT_H__

// Effect.cpp: the board's and the overlay's short-lived effects - Effect (base), Sparkle (the gem tiles'
// sparkles), FlyingPieceList (the tiles of a submitted word flying into Lex's mouth) and their owner
// EffectMgr - and the particle system: Ember and its pooled owner EmberListMgr (sparks, embers, smoke).
// The managers own an MTRand each (new MTRand(Sexy::Rand()) in the original: SeedMTRand2004 here);
// EffectMgr::mRand is drawn only by FlyingPieceList::Update, EmberListMgr::mRand never (it is only saved).
// Both managers are saved inside the game save (classic.dat / action.dat), byte for byte as the original: their
// random generator's state and a count of effects, which is always 0, since Board::SaveGame clears the effects,
// the embers and the overlay's effects before it writes. The effects themselves have no save code (the original's
// Effect, Sparkle, FlyingPieceList and Ember Save and Load were never reached; GAME_NOTES, "Code taken out").
// Offsets in the comments are absolute (the original's); nothing depends on them.
// Behaviour spec: re/bookworm/analysis/board_draw.md section 3; names: re/bookworm/analysis/NAMES.md.

#include <cstdio>
#include <list>
#include <vector>

namespace Sexy
{

class BookWormApp;
class Graphics;
class MTRand;
class Piece;
class EffectMgr;
class EmberListMgr;

// Effect::GetType, which EffectMgr::HasEffectOfType and GetEffectsOfType ask for
enum EffectType
{
	EFFECT_SPARKLE = 0,
	EFFECT_FLYING_PIECES = 1
};

// Ember::mType: image and cel layout in Ember::Draw (board_draw.md 3.5)
enum EmberType
{
	EMBER_SPARK_SM = 0,             // fx_spark_sm, vertical cels, additive
	EMBER_SPARK_MED = 1,            // fx_spark_med
	EMBER_SPARK_LRG = 2,            // fx_spark_lrg
	EMBER_EMBER_SM = 3,             // fx_ember_sm, horizontal cels, additive
	EMBER_EMBER_MED = 4,            // fx_ember_med additive, then fx_ember_smoke_med normal at alpha 176
	EMBER_EMBER_LRG = 5,            // fx_ember_lrg
	EMBER_SMOKE_SM = 6,             // fx_ember_smoke_sm, additive
	EMBER_SMOKE_MED = 7,            // fx_ember_smoke_med
	EMBER_SMOKE_LRG = 8,            // fx_ember_smoke_lrg
	EMBER_FLAME_LRG = 9,            // fx_flame_lrg, additive
	EMBER_SMOKE_PUFF = 10           // fx_smoke_sm, normal, alpha mAlpha * 255, at (x - 15, y - 15)
};

// Original: class Sexy::Effect, sizeof 0x10, vftable 0x4fc450 (8 slots; the port has no slots 5 and 6, Save and Load).
// Never allocated on its own.
class Effect
{
public:
	// +0x00 vfptr
	EffectMgr*		mEffectMgr;             // +0x04 owner (set by OverlayWidget::AddEffect)
	int				mUpdateCnt;             // +0x08 ++ per Update
	bool			mDead;                  // +0x0c EffectMgr::Update deletes the effect when set
	// +0x0d..+0x0f padding

public:
	Effect(EffectMgr* theEffectMgr);                                // 0x41e880
	virtual void	DeleteEffect(Effect* theEffect);                // 0x41e950 slot 0: deletes theEffect, not this
	virtual void	Update();                                       // 0x41e8c0 slot 1
	virtual void	Draw(Graphics* g) {}                            // 0x401570 slot 2 (empty)
	virtual int		GetType() = 0;                                  // 0x41f260 slot 3 (the base's returns -1, never called)
	virtual bool	IsDead() { return mDead; }                      // 0x451af0 slot 4
	virtual ~Effect();                                              // 0x41f270 body, 0x41f280 slot 7
};

// Original: class Sexy::Sparkle : public Effect, sizeof 0x48, vftable 0x4fc474.
// One sparkle_large animation (14 cels, one every 8 updates) centred in a 57x57 box.
class Sparkle : public Effect
{
public:
	double			mX;                     // +0x10 top-left of the 57x57 box the sparkle is centred in
	double			mY;                     // +0x18
	double			mVX;                    // +0x20 added to mX per update
	double			mVY;                    // +0x28 added to mY per update
	double			mGravity;               // +0x30 added to mVY per update
	int				mDelay;                 // +0x38 updates to wait before moving/animating (drawn meanwhile)
	// +0x3c a flag only the (unreached) Sparkle::Save and Load used, +0x3d..+0x3f padding
	int				mImage;                 // +0x40 app image id, ctor IMAGE_SPARKLE_LARGE (0x8d)
	int				mFrame;                 // +0x44 0..13; dead when it wraps to 0

public:
	Sparkle(EffectMgr* theEffectMgr);                               // 0x41f2b0
	virtual void	Update() override;                              // 0x41e970 slot 1
	virtual void	Draw(Graphics* g) override;                     // 0x41e9e0 slot 2
	virtual int		GetType() override { return EFFECT_SPARKLE; }            // 0x4ba8e0 slot 3 (shared "return 0" stub)
	virtual ~Sparkle();                                             // 0x41f8a0 slot 7
};

// Original: class Sexy::FlyingPieceList : public Effect, sizeof 0x20, vftable 0x4fc4a0.
// The tiles of a submitted word flying into Lex's mouth (the overlay's EffectMgr); owns its pieces.
class FlyingPieceList : public Effect
{
public:
	std::list<Piece*>	mPieces;            // +0x10 (head +0x14, size +0x18) the pieces still in flight
	int				mLastChompCnt;          // +0x1c mUpdateCnt when the chomp sound last played

public:
	FlyingPieceList(EffectMgr* theEffectMgr, std::list<Piece*>* thePieces); // 0x420920 (thePieces may be NULL)
	virtual void	Update() override;                              // 0x421150 slot 1
	virtual void	Draw(Graphics* g) override;                     // 0x41fba0 slot 2
	virtual int		GetType() override { return EFFECT_FLYING_PIECES; }      // 0x4201b0 slot 3
	virtual ~FlyingPieceList();                                     // 0x4200f0 body, 0x4202a0 slot 7
};

// Original: class Sexy::EffectMgr, sizeof 0x1c, vftable 0x4fc498 (1 slot: deleting dtor).
// Board+0x70 (sparkles) and OverlayWidget+0x5c (flying tiles).
class EffectMgr
{
public:
	// +0x00 vfptr
	BookWormApp*		mApp;               // +0x04
	MTRand*				mRand;              // +0x08 drawn only by FlyingPieceList::Update
	int					mUpdateCnt;         // +0x0c ++ per Update
	std::list<Effect*>	mEffects;           // +0x10 (head +0x14, size +0x18) update and draw order

public:
	EffectMgr(BookWormApp* theApp);                                 // 0x4201c0 (one Sexy::Rand() draw)
	virtual ~EffectMgr();                                           // 0x420060 body, 0x420270 slot 0

	void			Clear();                                        // 0x41f930 deletes every effect
	bool			Update();                                       // 0x41f990 true if it had effects before updating
	void			Draw(Graphics* g);                              // 0x41fa10
	void			AddEffect(Effect* theEffect);                   // 0x4207f0 push_back, takes ownership
	bool			HasEffectOfType(int theType);                   // 0x41fa50
	std::list<Effect*>* GetEffectsOfType(int theType);              // 0x420830 new list or NULL (caller deletes)
	void			Save(FILE* theFile);                            // 0x41faa0 (writes the count 0: see the file header)
	void			Load(FILE* theFile);                            // 0x420b70 callers Clear() first
};

// Original: struct Ember (no vtable, no RTTI name), sizeof 0x60. Lives in EmberListMgr.
struct Ember
{
	EmberListMgr*	mMgr;                   // +0x00 owner (images and screen size through mMgr->mApp)
	int				mUpdateCnt;             // +0x04 never incremented (only saved), so mLoop advances every update
	bool			mDead;                  // +0x08
	// +0x09..+0x0f padding
	double			mX;                     // +0x10 centre
	double			mY;                     // +0x18
	double			mVX;                    // +0x20
	double			mVY;                    // +0x28
	double			mGravity;               // +0x30 added to mVY per update (Reset 0.01)
	double			mAlpha;                 // +0x38 life 1..0: frame, death, type 10 alpha
	double			mAlphaDecay;            // +0x40 subtracted from mAlpha per update
	int				mDelay;                 // +0x48 updates to wait
	bool			mLoop;                  // +0x4c cycle the frames instead of following mAlpha
	// +0x4d..+0x4f padding
	int				mFrame;                 // +0x50
	int				mNumFrames;             // +0x54 0 until the first Update
	int				mType;                  // +0x58 EmberType
	// +0x5c..+0x5f padding

	Ember(EmberListMgr* theMgr);                                    // 0x41f300 (Reset)
	~Ember();                                                       // 0x41ec40
	void			Reset(EmberListMgr* theMgr);                    // 0x41ec50 every field to its default
	void			Update();                                       // 0x41ecb0
	void			Draw(Graphics* g);                              // 0x41f320
};

// Original: class Sexy::EmberListMgr, sizeof 0x2c, vftable 0x4fc4c4 (1 slot: deleting dtor).
// Board+0x74, created with a pool of 1000 embers.
class EmberListMgr
{
public:
	// +0x00 vfptr
	BookWormApp*		mApp;               // +0x04
	MTRand*				mRand;              // +0x08 never drawn from (only saved and restored)
	std::vector<Ember*>	mEmbers;            // +0x0c active, in draw order
	std::vector<Ember*>	mFreeEmbers;        // +0x1c the pool

public:
	EmberListMgr(BookWormApp* theApp, int thePoolSize);             // 0x421070 (one Sexy::Rand() draw)
	virtual ~EmberListMgr();                                        // 0x4203a0 body, 0x420520 slot 0

	void			Clear();                                        // 0x4202d0 deletes the active and the pooled embers
	Ember*			NewEmber();                                     // 0x420d60 the pool's last ember, or a new one
	void			AddEmber(int theType, double theX, double theY, double theVX, double theVY, bool theLoop,
						double theAlpha, double theAlphaDecay);     // 0x420e00
	bool			Update();                                       // 0x420fd0 true if it had embers before updating
	void			Draw(Graphics* g);                              // 0x41f8d0
	void			Save(FILE* theFile);                            // 0x41ff80 (writes the count 0: see the file header)
	void			Load(FILE* theFile);                            // 0x420e70 callers Clear() first
};

}

#endif
