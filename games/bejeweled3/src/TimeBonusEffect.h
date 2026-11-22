// Sexy::TimeBonusEffect (bejeweled3, vtable 0x865f84, type 0x15, 0x2c0 bytes, constructor 0x72a3b0): the glow and the bolts of lightning
// that play about a Lightning time gem, and the sparks it throws off. 0x741130 makes one for each time gem the fill makes
// (LightningBoard::OnFillAccepted); it follows the gem by its id and goes when the gem has gone and its last bolt has.
#pragma once

#include "Effects.h"

#include <vector>

namespace Sexy {

// One bolt of lightning (the exe's 0x5c-byte record in the vector at +0x1cc): it runs from an angle on the gem's edge to another (or to
// the edge of a neighbouring gem), through up to 4 points that jump about along its length
struct TimeBonusBolt {
	float mStartAngle;        // +0: where on the gem's edge it starts, in radians from the right; it turns by mStartTurn in each update
	float mStartTurn;         // +4
	float mEndAngle;          // +8: where it ends: on the gem's own edge or, for a targeted bolt, on the target's
	float mEndTurn;           // +0xc
	bool mCircling;           // +0x10: the bolt that runs round the edge (its ends are put together and drift apart); drawn in the second colour
	bool mTargeted;           // +0x11: the bolt that jumps to a neighbouring gem
	int mTargetPieceId;       // +0x14: the id of that gem
	float mHeight[8];         // +0x18: how far each point is off the line (8 places, up to 4 used)
	float mHeightChange[8];   // +0x38: how fast it goes
	int mPointCount;          // +0x58
};

class TimeBonusEffect : public Effect {
public:
	int mColor = 0;                         // +0x1d8: the gem's colour (0 to 6)
	int mSeconds = 0;                       // +0x1dc: the seconds the gem is worth, which is how many bolts and sparks it makes
	std::vector<TimeBonusBolt> mBolts;      // +0x1cc (begin) to +0x1d0 (end)

	// 0x72a3b0 (bejeweled3), with 0x729460 (type 0x15); the colour and seconds are the piece's
	explicit TimeBonusEffect(const Piece* thePiece);
	void Update() override;   // slot 1, 0x730de0
	void Draw(Graphics* g) override;   // slot 2, 0x72ec70

	// 0x741130 (bejeweled3): makes the effect for a time gem: its place is 64 and 64 (the gem's middle from its corner) and it is
	// added to the board's layer over the gems (+0x12c0)
	static void MakeFor(Board* theBoard, const Piece* thePiece);

private:
	void MakeSpark(int theFrame);
	void MakeBolt(const Piece* thePiece);
	// returns true when the bolt has run out and goes
	bool UpdateBolt(TimeBonusBolt& theBolt, int theFrame);
};

}  // namespace Sexy
