// Sexy::Effect (bejeweled3, vtable 0x865f3c, constructor 0x729460), its two kinds Sexy::ParticleEffect (0x865f4c, 0x72b180; type 0x16: a
// copy of a PIEffect resource) and Sexy::PopAnimEffect (0x865f5c, 0x72af30; type 0x17: a copy of a PopAnim), and the widget that
// holds them, Sexy::EffectsManager (0x865df4, constructor 0x72a510). The board has two of these (+0x12bc under the gems, +0x12c0 over
// them); an effect follows a piece by its id (+0x28) and goes when its animation has run out and no piece uses it (+0x1b4).
// port: the exe's Effect has 30 other kinds (types 0 to 0x15: the sparks, the rings, the lens rings, the lightning) of which only the
// spark of the time gems (type 5, SparkEffect) and the time gems' bolts (type 0x15, TimeBonusEffect.h) are made so far; the field list
// and the manager's switch over the types are kept for the kinds made (a kind's case of the switch is its KindUpdate)
#pragma once

#include "Color.h"

#include <vector>

namespace Sexy {

class Board;
class Graphics;
class Image;
class Piece;
class PIEffect;
class PopAnim;

class EffectsManager;

class Effect {
public:
	enum { TYPE_SPARK = 5, TYPE_TIME_BONUS = 0x15, TYPE_PARTICLE = 0x16, TYPE_POPANIM = 0x17 };

	int mType;                        // +8
	bool mOverlay = false;            // +0xc: drawn in the manager's overlay pass (DrawOverlay, slot 44, 0x72e3c0) instead of its draw
	float mX = 0.0f;                  // +0x10: in 1200ths of the art height, from the window's corner
	float mY = 0.0f;                  // +0x14
	int mPieceId = -1;                // +0x28: the piece it follows; -1 for none (a piece that is gone sets 0, see Piece::ReleaseEffects)
	float mVelocityX = 0.0f;          // +0x1c: the place moves by this over the step (mStep) each update
	float mVelocityY = 0.0f;          // +0x20
	float mDampingX = 1.0f;           // +0x2c: the velocity is multiplied by these (the vertical one after the gravity is added) in the updates the step divides
	float mDampingY = 1.0f;           // +0x30
	float mGravity = 0.0f;            // +0x38: added to the vertical velocity before its damping
	float mAlpha = 1.0f;              // +0x4c
	float mAlphaChange = -0.01f;      // +0x50 (0 for both kinds: their constructors)
	float mAlphaMax = 1.0f;           // +0x54
	float mScale = 1.0f;              // +0x58
	float mScaleChange = 0.0f;        // +0x5c
	float mScaleMin = 0.0f;           // +0x68
	float mScaleMax = 10000.0f;       // +0x6c
	Color mColor = Color(255, 255, 255, 255);   // +0x70: the colour a spark is drawn in
	float mAngle = 0.0f;              // +0x90
	float mAngleChange = 0.0f;        // +0x94: added to the angle in each update of the manager (the spark's case of its switch)
	Image* mImage = nullptr;          // +0x180: what a spark draws
	bool mUpdateTwice = false;        // +0x184: slot 1 runs twice in each update of the manager
	int mStep = 1;                    // +0x188: the alpha and scale changes are made in each update whose count it divides
	int mFlags = 0;                   // +0x1ac: bit 8 marks the effects the level change's flame gems have (0x749130), bit 4 a hold
	bool mDone = false;               // +0x1b0: the animation has run out
	int mUsers = 0;                   // +0x1b4: how many pieces hold it (it is kept while one does)
	EffectsManager* mManager = nullptr;   // +0x1b8: the layer it is in

	explicit Effect(int theType) : mType(theType) {}
	virtual ~Effect() {}
	// slot 1: one step (0x732880 for a ParticleEffect, 0x7325d0 for a PopAnimEffect); slot 2: the drawing
	virtual void Update() = 0;
	virtual void Draw(Graphics* g) = 0;

	// the manager's switch over the type (0x7303f0), after the alpha and scale changes: what the kind adds to the common step
	virtual void KindUpdate() {}

	// 0x728c00 (bejeweled3): the animation is let run out: it stops looping and goes to its last frame (a ParticleEffect)
	virtual void Release() {}
	// the part of 0x732880 and 0x7325d0 that follows the piece: puts mX and mY on its middle, or lets the effect go when it is gone
	void FollowPiece();
};

// 0x729c20 (bejeweled3) with type 5, whose constructor 0x729460 gives it: the speck of light the time gems throw off (drawn by the
// manager's case at 0x72d9e5: the picture additive, in the colour, at the scale and turn). It follows its piece: where it is, is
// from the piece's corner, and it is not drawn while the piece is gone.
class SparkEffect : public Effect {
public:
	// 0x729460 (type 5): 3 draws of the app's Rand() (a green and a blue amount of its colour, which its maker replaces, and the turn,
	// by 0x712da0 times pi); the scale 0.75 and its change 0.005 and the turn's change 0.01 are the maker's to replace or keep
	SparkEffect();
	void Update() override {}   // the base Effect's slot 1 (0x417820) steps two curves that are constants here
	void Draw(Graphics* g) override;   // the manager's case 5 of 0x72ce60
	void KindUpdate() override;   // the manager's case 5 of 0x7303f0: the scale and the turn go on by their changes
};

// 0x72b180 (bejeweled3): a copy of a PIEffect resource (+0x1c0) that is drawn with a matrix put together by its update
class ParticleEffect : public Effect {
public:
	PIEffect* mEffect;                // +0x1c0
	bool mFirstUpdate = true;         // +0x1c4: the first update is ResetAnim, the later ones PIEffect::Update (which draws an app Rand() for the seed)
	bool mPieceMatrix = false;        // +0x1c5: the matrix goes to the effect's draw transform, with the place in it (else to the place's transform)

	explicit ParticleEffect(const PIEffect* theResource);   // 0x72b180 (bejeweled3), with the Effect's constructor 0x729460 (type 0x16)
	~ParticleEffect() override;
	void Update() override;   // slot 1, 0x732880 (bejeweled3)
	void Draw(Graphics* g) override;   // slot 2, 0x72c460 (bejeweled3)
	void Release() override;   // 0x728c00 (bejeweled3)
};

// 0x72af30 (bejeweled3): a copy of a PopAnim resource (+0x1c0), drawn about its middle at the effect's place at the scale mScale
class PopAnimEffect : public Effect {
public:
	PopAnim* mAnim;                   // +0x1c0
	bool mUpdateAnimTwice = true;     // +0x1c4: 1 from the constructor: the animation is updated twice in each update

	explicit PopAnimEffect(const PopAnim* theResource);     // 0x72af30 (bejeweled3), with 0x729460 (type 0x17)
	~PopAnimEffect() override;
	void Update() override;   // slot 1, 0x7325d0 (bejeweled3)
	void Draw(Graphics* g) override;   // slot 2, 0x728a30 (bejeweled3)
	void PlayLabel(const char* theLabel);   // slot 3, 0x732b90 (bejeweled3): starts the animation at a label ("" for its first frame)
};

// 0x72a510 (bejeweled3): the list of effects of one layer of the board (+0xbc is the board, +0x110 the layer's alpha, 1, and +0x114 its
// update count). The exe makes it a widget; the board's +0x12bc is drawn by the board's own gem passes (0x7568a0) and updated by
// 0x767090, the +0x12c0 one is the board's child
class EffectsManager {
public:
	Board* mBoard;                    // +0xbc
	float mAlpha = 1.0f;              // +0x110
	int mUpdateCount = 0;             // +0x114
	std::vector<Effect*> mEffects;    // the list at +0xc0, in the order the effects were added

	explicit EffectsManager(Board* theBoard) : mBoard(theBoard) {}
	~EffectsManager();
	// 0x417f10 (bejeweled3): puts an effect at the end of the list (0x72a370 for the ones that set their layer after)
	void Add(Effect* theEffect);
	// Draws the effect: the ones that follow a piece (not types 1, 0x15, 0x16 and 0x17) are drawn from the piece's corner and
	// are left out while it is gone (0x72ce60)
	void DrawEffect(Effect* theEffect, Graphics* g);
	// slot 22, 0x7303f0 (bejeweled3): the effects update one by one; the ones that are done and held by no piece go
	void Update();
	// slot 26 (0x7303c0, with 0x72ce60 and its third argument 0) and slot 44 (0x72e3c0, third argument 1): the effects of the
	// manager's pass
	void Draw(Graphics* g, bool theOverlay);
	// 0x729ca0 (bejeweled3): all the effects go
	void Clear();
};

}  // namespace Sexy
