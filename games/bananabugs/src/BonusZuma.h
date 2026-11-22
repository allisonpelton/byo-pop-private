// BonusZuma: the frog (0x412f20..0x413f90 in the original, 0x304 bytes; the name is from the original's sound and picture
// names). The monkey holds it ("ZUMA"), turns it with the second button and puts it down with the first: the frog lands
// where it was put and, after a pause, turns (a little more each frame) toward the nearest bug and, when that bug is far
// enough off (between 64 and 216 pixels) and alive, snaps to face it and spits a ball: the ball flies along the way the
// frog faces (200 frames) and kills the first bug it passes within 40 pixels (or leaves the screen); the ball is then
// taken back, and after 64 frames the frog turns to the nearest bug again. The frog is put away (fades out) when the
// bonus ends, or when the land it stands on is cut away.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class Anim;

// What the frog is doing (BonusZuma::mMode)
enum ZumaMode
{
	ZUMA_HELD = 0,           // the monkey holds it (or it has not started)
	ZUMA_RELOADING = 1,      // the ball is on its way back: the frog turns toward the nearest bug, waiting out the countdown
	ZUMA_AIMING = 2,         // turning toward the nearest bug until one is in range, then it shoots
	ZUMA_SHOT = 3            // the ball is flying
};

class BonusZuma : public Bonus
{
public:
	int mMode;               // +0x2d8: ZumaMode
	ObjBase* mBall;          // +0x2dc: the ball, a child of the bonus (so it flies along the way the frog faces); hidden when not flying
	ObjBase* mFrog;          // +0x2e0: the frog's picture, a child of the bonus (the bonus's own object shows nothing)
	AnimKey* mFlightKey;     // +0x2e4: the ball's flight: its length is set by each shot
	int mHeldTrack;          // +0x2e8: the frog appears and then sways a little for ever
	int mDropTrack;          // +0x2ec: it flies a short way along its angle and lands (event 5: it starts to hunt)
	int mEndTrack;           // +0x2f0: it fades out (event 8: the bonus is put away)
	int mCountdown;          // +0x2f4: frames the frog waits before it shoots again (120 from Setup, 64 after a shot)
	float mAngle;            // +0x2f8: the way the frog faces (radians): it is turned toward the nearest bug
	float mUnused2fc;        // +0x2fc: Start sets it to 0; nothing reads it
	int mUnused300;          // +0x300: Start sets it to 0; nothing reads it
	bool mEnding;            // port: End has started the fade (the original starts it again each frame the land is cut)

	static Anim* sZumaAnim;     // 0x60e1e4: "images/bonus_zuma"
	static Anim* sZumaBallAnim; // 0x60e1e8: "images/bonus_zuma_ball"

	BonusZuma();                              // 0x412f20 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x412f70
	bool Update() override;                   // 0x412fa0
	void Notify(ObjBase* theObj) override;    // 0x4132b0
	void Reset() override;                    // 0x413340
	void End() override;                      // 0x413370
	bool EndWhenBugsGone() override;          // 0x412570 (the base's EndOnLastBug)
	void Start() override;                    // 0x413380
	bool Place() override;                    // 0x413460: put down

private:
	void Shoot();                             // 0x413570 (ESI the bonus): the ball is sent along the way the frog faces
	void ResetBall();                         // 0x4135b0 (EDX the bonus): the ball goes back to the frog and is hidden
	void BuildBall();                         // 0x4135f0
	void BuildFrog();                         // 0x413840 (EDI the bonus)
	void BuildTracks();                       // 0x4138d0
};

} // namespace Sexy
