// ScoreBoard: the level's board of figures (0x464320..0x4687e0 in the original). A panel at the foot of the design space
// slides up as the level begins and away as it ends. It shows the score, the percent filled and the level's number
// in the game's red font, the monkeys the second upgrade adds (MonkeyLives), a bar of the world's progress, 25 banana
// icons that pop in as bananas are earned, and the fire balls the tenth upgrade gives, which leave from it. Sparks
// glow about its counters while the monkey plays, and dirt falls from it as it settles.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;
class Emitter;
class FireBall;
class MonkeyLives;
class ObjString;

const int SCOREBOARD_FIREBALLS = 3;
const int SCOREBOARD_MONKEYS = 4;        // the second upgrade's value is 0, 1, 2 or 4
const int SCOREBOARD_HUD_EMITTERS = 8;
const int SCOREBOARD_ICONS = 25;         // the bananas the level counts
const int SCOREBOARD_ICON_SLOTS = 50;    // the original's array is twice as long; the rest is never used

class ScoreBoard : public ObjBase
{
public:
	ObjBase* mGems;                      // +0x27c: the picture of the gems, one cel for each fire ball the player has
	int mUnused280;                      // +0x280: nothing writes or reads it
	FireBall* mFireBalls[SCOREBOARD_FIREBALLS]; // +0x284: made as the board is, none past the tenth upgrade's value
	float mUnused290;                    // +0x290: set from the picture's size; nothing reads it
	float mUnused294;                    // +0x294
	float mPlaceX;                       // +0x298: where the board rests (further right on a wide screen)
	float mPlaceY;                       // +0x29c
	ObjString* mScoreText;               // +0x2a0
	ObjString* mPercentText;             // +0x2a4
	ObjString* mLevelText;               // +0x2a8
	Anim* mDirtAnim;                     // +0x2ac: images/dirt01
	Anim* mLeafAnim;                     // +0x2b0: images/tinyleaf
	Anim* mGemsAnim;                     // +0x2b4: images/scoreboard_gems
	Emitter* mDirtFall;                  // +0x2b8: dirt falling down off the board while it rises into place
	Emitter* mRightSpray;                // +0x2bc: dirt thrown up at the right of the board
	Emitter* mLeftSpray;                 // +0x2c0: and at the left
	Emitter* mHud[SCOREBOARD_HUD_EMITTERS]; // +0x2c4: the sparks about the counters (guess: names of the pairs unknown)
	int mEnterTrack;                     // +0x2e4: the track that slides the board up
	int mLeaveTrack;                     // +0x2e8: and away
	bool mAway;                          // +0x2ec: set from the start and when it has left; clear once it is in place
	MonkeyLives* mMonkeys[SCOREBOARD_MONKEYS]; // +0x2f0
	Anim* mBananaIconAnim;               // +0x300: images/bananabunch01
	ObjBase* mIcons[SCOREBOARD_ICON_SLOTS]; // +0x304: the first 25 are made
	int mIconCount;                      // +0x3cc: the icons shown
	ObjBase* mBarTopLeft;                // +0x3d0: marks of the world's progress bar, at (-146,-32) from the board
	ObjBase* mBarTopRight;               // +0x3d4: at (149,-32)
	ObjBase* mBarBottomLeft;             // +0x3d8: at (-146,-12)

	ScoreBoard();                        // 0x464320
	~ScoreBoard();                       // 0x464eb0 (the body 0x464ed0)
	void Notify(ObjBase* theObj) override; // 0x468660
	bool Update() override;              // 0x4685e0
	void Draw(Graphics* g) override;     // 0x464f50

	void Show();                         // 0x465080: slides up
	void Leave();                        // 0x468780: slides away and throws leaves (was ClearBananaIcons)
	bool IsAway() const { return mAway; } // 0x468770 (was IsDone)
	void ShowScore(int theScore);        // 0x4684a0
	void ShowPercent(int thePercent);    // 0x468500
	void ShowLevel(int theLevel);        // 0x468550
	void ShowBananaIcon();               // 0x465140
	void MarkLastIcon();                 // 0x4650f0
	void ClearBananaIcons();             // 0x4650a0: every icon on shown goes
	void LaunchFireBall(int theIndex);   // 0x4651a0
	void ActivateHud();                  // 0x4676e0
	void DeactivateHud();                // 0x467750
	void WakeMonkeys();                  // 0x468740
	void DismissMonkeys();               // 0x4687b0

private:
	void BuildBananaIcons();             // 0x4651e0
	void BuildEnterTrack();              // 0x465bf0
	void BuildLeaveTrack();              // 0x465db0
	void SpawnLeaf(int theDelay);        // 0x465fe0
	void BuildHud();                     // 0x466350
	void BuildDirtFall();                // 0x4677d0
	void BuildSprays();                  // 0x467b30
	void BuildBarMarks();                // 0x4682a0
	void BuildGems();                    // 0x468410
	void ShowBananaTotal(int theTotal);  // 0x4685a0
	Emitter* MakeHudEmitter(AnimKey* theFirst, AnimKey* theSecond, int theZ, int theCount, int theRate, int theX,
		int theY, int theJitterX, float theDepth); // the pattern 0x466350 repeats eight times
};

} // namespace Sexy
