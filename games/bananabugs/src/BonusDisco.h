// BonusDisco: the disco ball (0x40b0d0..0x40b7c0 in the original, 0x2f0 bytes). When it is won ("DISCO") the backdrop
// darkens, the disco song plays and a mirror ball lowers in front of the ground (200 frames); every bug is frozen for
// 1500 frames and every kill counts as made by a pea (so each drops bananas). The bonus lasts 600 frames, or until the
// last bug is gone once the ball has landed; the bugs are then released and the ball rises and goes.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class Anim;

class BonusDisco : public Bonus
{
public:
	bool mEnded;             // +0x2d8: End has been called
	int mCountdown;          // +0x2dc: frames until the bonus ends (600 from Start); -99 for none
	int mLowerTrack;         // +0x2e0: the ball lowers from 300 above to 20 above its place; its last key's event 4 says it has landed
	int mSpinTrack;          // +0x2e4: the ball turns where it is, for ever
	int mRiseTrack;          // +0x2e8: the ball rises again; its last key's event 5 puts the bonus away
	bool mDarkened;          // +0x2ec: the backdrop has been darkened by this bonus
	bool mLanded;            // +0x2ed: the ball has landed (event 4); only then may the last bug's death end the bonus
	bool mMusicOn;           // +0x2ee: the disco song is playing (sound 90, looped)
	bool mEndRequested;      // +0x2ef: the last bug was gone before the ball landed: End as soon as an event comes in

	static Anim* sDiscoBallAnim; // 0x60e1c4: "images/bonus_discoball"

	BonusDisco();                             // 0x40b0d0
	void Setup(int theKind) override;         // 0x40b0f0
	bool Update() override;                   // 0x40b140
	void Notify(ObjBase* theObj) override;    // 0x40b190
	void Reset() override;                    // 0x40b200
	void End() override;                      // 0x40b280
	bool EndWhenBugsGone() override;          // 0x40b310
	void Start() override;                    // 0x40b330

private:
	void BuildTracks();                       // 0x40b450
};

} // namespace Sexy
