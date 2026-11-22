// BonusFlyPaper: the fly paper (0x40c700..0x40d090 in the original, 0x2e8 bytes). The monkey holds it ("CLICK TO DROP
// FLYPAPER"), turns it with the second button and drops it with the first: it is thrown a little way in the way it was
// turned and lies on the board, and every bug that walks over it is held (the zap area, 10 frames more each frame).
// When the bonus ends the paper fades out and is put away.
#pragma once

#include "BonusDropper.h"

namespace Sexy
{

class Anim;

class BonusFlyPaper : public BonusFadingDropper
{
public:
	int mUnused2e4;          // +0x2e4: nothing sets or reads it

	static Anim* sPaperAnim; // 0x60e1d4: "images/bonus_paper"

	BonusFlyPaper();                          // 0x40c700 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x40c740
	bool Update() override;                   // 0x40c760 (the same as BonusBomb's)
	void Notify(ObjBase* theObj) override;    // 0x40c790
	void Reset() override;                    // 0x40c840
	bool EndWhenBugsGone() override;          // 0x412570 (the base's EndOnLastBug)
	void Start() override;                    // 0x40c870

private:
	void BuildTracks();                       // 0x40c930
};

} // namespace Sexy
