#ifndef __ASTROPOP_BOARD_H__
#define __ASTROPOP_BOARD_H__

// Board: the playfield screen (original constructor 0x480020, vtable 0x566a30 with 47 slots, listener part at +0x58 with
// the table 0x566980, sizeof 0x1c0c). It is the one big object of the game: the field of bricks, the falling pieces, the
// power-ups, the stats, the replay recording. Its members keep their original offsets; until the code that uses a
// member says what it is, the member is called m<offset> (docs/board_fields.tsv, tools/boardfields.py).

#include "Text2004.h"
#include "Widget.h"
#include "Buffer.h"
#include "Color.h"
#include "EventBus.h"
#include "FrameAnim.h"
#include "SmallObjects.h"
#include <list>
#include <set>
#include <vector>
#include <string.h>

namespace Sexy
{

class Brick;
class XcfFile;
class MemoryImage;
class AstroPopApp;
class Board;
class Image;
class FuelCell;
class SpecialBrick;
class Particle;
class AstroButton;
class Checkbox;
class SoundInstance;
class Graphics;
class StoryScreen;
struct StoryEntry;

// 0x4b83f0 and 0x4b8430: the game's own DrawImageCel (BoardDraw.cpp)
void DrawCel(Graphics* g, Image* theImage, int theX, int theY, int theCel);
void DrawCelStretched(Graphics* g, Image* theImage, const Rect& theDest, int theCel);

// a cell of the field (the original keeps vectors of these: 8 bytes each)
struct CellRef
{
	int		mCol;
	int		mRow;
};

// a cell and a number (12 bytes each)
struct CellRef3
{
	int		mCol;
	int		mRow;
	Brick*	mBrick;
};

// A mark (12 bytes, the vector at +0x200): a kind of brick and how many pieces of it are to drop; the mark with the flag is
// the supercharged kind that the second game type announces
struct MarkRec
{
	int		mType;      // +0x00
	int		mCount;     // +0x04
	bool	mActive;    // +0x08
};

// A row of the table that sets the speed of the game by the time played (24 bytes, the vector at +0x280)
struct RampRecord
{
	int		mTime;      // updates played when the row ends
	int		mV1;        // -> +0x19cc
	int		mV2;        // -> +0x1ac4
	int		mV3;        // -> +0x1c04
	float	mF4;        // -> +0x1bec
	float	mF5;        // -> +0x1be8
};

// A layer of the scenery that scrolls down the screen (0x4333c0 steps it). The Board keeps one at +0x77c; the planets
// (0x3c bytes each, a ScrollLayer and one more number) are a vector at +0x160.
struct ScrollLayer
{
	Image*	mImage;             // +0x00
	float	mCur[4];            // +0x04 the picture now on the screen (mCur[1] counts down to its end)
	float	mNext[4];           // +0x14 the picture that follows (mNext[1] the place to start from)
	float	mF24;               // +0x24
	float	mF28;               // +0x28
	int		mPeriod;            // +0x2c the layer steps every mPeriod updates
	int		mI30;               // +0x30
	float	mSpeed;             // +0x34

	ScrollLayer()               // 0x436100
	{
		mImage = NULL;
		for (int i = 0; i < 4; i++)
		{
			mCur[i] = 0;
			mNext[i] = 0;
		}
		mF24 = -1234.0f;
		mF28 = -1234.0f;
		mSpeed = -1234.0f;
		mPeriod = 500000000;
		mI30 = 500000000;
	}
};

// A planet: a scroll layer and a number (0x3c bytes)
struct PlanetRec : public ScrollLayer
{
	float		mRangeStart;        // +0x38

	PlanetRec() : mRangeStart(0) { }
};

// One of the 100 streaks of the backdrop (0x30 bytes, the vector at +0x170)
struct StreakRec
{
	Color			mColor;             // +0x00 set at the end of a game by the kind
	float			mX;                 // +0x10
	float			mY;                 // +0x14
	float			mSpeed;             // +0x18
	float			mMinSpeed;          // +0x1c
	int				mMargin;            // +0x20
	int				mKind;              // +0x24 0, 1 or 2: the dark, middle or bright colour
	bool			mFlag28;            // +0x28
	Image*			mImage;             // +0x2c the picture of the kind (the application's 0x674, 0x678, 0x67c)

	StreakRec()                         // 0x432750 (the original leaves the speeds to the constructor's loop)
		: mColor(0, 0, 0), mX(-1.0f), mY(-1.0f), mSpeed(0), mMinSpeed(0), mMargin(1), mKind(0), mFlag28(true), mImage(NULL) { }
};

// A line of help for the pop-up that explains a brick (24 bytes, the vector at +0x1d0)
struct TipEntry
{
	Image*						mImage;     // +0x00 the picture of the brick
	int							mValue;     // +0x04 10 from the freeze bomb's entry on (the seconds it stops the rows), else 0
	std::vector<std::string>	mLines;     // +0x08 the kind, the name and the text

	TipEntry() : mImage(NULL), mValue(0) { }
};

// One of the stars (0x6c bytes, the vector at +0x190): a picture of the backdrop that drifts down the screen
struct StarRec
{
	Image*			mImage;             // +0x00
	unsigned char	m04[0x4c];
	float			mX;                 // +0x50
	float			mY;                 // +0x54 counts down the screen
	unsigned char	m58[0x08];
	int				m60;                // +0x60
	float			mSpeed;             // +0x64
	int				mImageId;           // +0x68 the resource id of the picture (IMAGE_BG_OBJECT1 .. 7), -1 for none

	StarRec()                           // 0x480020: the vector's fill element (the base part 0x40b1b0 zeroes it, the id is -1)
	{
		memset(this, 0, sizeof(*this));
		mImageId = -1;
	}
};

// A panel of the hint and intro screens (an object of 0x12c bytes, constructor 0x43fab0): a title and a text in two colours,
// the picture that goes with them, where they are shown and how they fade in. The Board keeps them in the vectors at
// +0x210 and +0x220 and chains the panels of one screen through mNext.
struct StageBg
{
	Color			mColor1;            // +0x00 the colour of the stage (the warp tints the theme buttons with it)
	Color			mColor2;            // +0x10
	std::string		mTitle;             // +0x20
	std::string		mText;              // +0x3c
	Image*			mPic;               // +0x58
	Image*			mOverPic; // +0x5c the picture over the stage (a hint's picture), faded in and out against mPic
	int				mX;                 // +0x60
	int				mY;                 // +0x64
	int				mX2;                // +0x68
	int				mY2;                // +0x6c
	FrameAnim		mPicAnim; // +0x70 the frame animation of mPic
	FrameAnim		mOverPicAnim; // +0xb0 the frame animation of mOverPic
	bool			mFadedOut; // +0xf0 set once the panel's fade-out has run to nothing (UpdateStageBackground); the panel is then neither updated nor drawn
	int				mTitleWidth; // +0xf4 the width of the rectangle the title is written in (constructed 150)
	int				mTitleX; // +0xf8 the left edge of the title's rectangle (constructed 120)
	int				mTextX; // +0xfc the left edge of the text's rectangle (constructed 400, the tutorial's panels set 300)
	int				mPicAlpha; // +0x100 the alpha of mPic, 0 to 255, moved by mPicAlphaStep
	int				mOverPicAlpha; // +0x104 the alpha of mOverPic, 0 to 255, moved by mOverPicAlphaStep
	int				mCrossfadeTimer; // +0x108 counts down 200 updates once both pictures have settled, then starts the pictures fading the other way round
	int				mPicAlphaStep; // +0x10c the change of mPicAlpha each update (+8 or -8, 0 when it rests)
	int				mOverPicAlphaStep; // +0x110 the change of mOverPicAlpha each update (+8 or -8, 0 when it rests)
	int				mPanelAlpha; // +0x114 the alpha of the whole panel (strip, title, text), 0 to 255
	int				mPanelAlphaStep; // +0x118 the change of mPanelAlpha each update; -8 fades the panel out and sets mFadedOut at 0
	int				m11c;               // +0x11c
	int				mTitleHeight; // +0x120 the height of the title as written by SetupHints
	int				mTextHeight; // +0x124 the height of the text as written by SetupHints
	StageBg*		mNext;              // +0x128 the next panel of the screen

	StageBg()                           // 0x43fab0
		: mPic(NULL), mOverPic(NULL), mX(0), mY(0), mX2(0), mY2(0), mFadedOut(false), mTitleWidth(0x96), mTitleX(0x78), mTextX(400),
		  mPicAlpha(0xff), mOverPicAlpha(0), mCrossfadeTimer(0), mPicAlphaStep(0), mOverPicAlphaStep(0), mPanelAlpha(0xff), mPanelAlphaStep(0), m11c(0), mTitleHeight(0), mTextHeight(0), mNext(NULL) { }
};

// A tutorial hint on the screen (0x30 bytes, constructor 0x44f500; the Board keeps them in a vector at +0x230): the
// picture of the hint, an arrow and a ring around the brick it is about, and the four rectangles around the ring that the
// screen is darkened with.
struct HintSpot
{
	StageBg*		mBg;                // +0x00 the hint's picture
	int				mArrowX;            // +0x04
	int				mArrowY;            // +0x08
	int				mArrowTop;          // +0x0c
	int				mRingX;             // +0x10
	int				mRingY;             // +0x14
	int				mId;                // +0x18 the hint (its flag in the profile is mId + 1)
	bool			mActive; // +0x1c the hint is on the screen; UpdateStageBackground then takes the panel from mBg
	std::vector<Rect>	mHole;          // +0x20 the darkened rectangles

	HintSpot() : mBg(NULL), mArrowX(-1), mArrowY(-1), mArrowTop(-1), mRingX(-1), mRingY(-1), mId(-1), mActive(false) {}
};

// A snowflake of the freeze effect (0x50 bytes, the vector at +0x240): it drifts along the top of the screen.
struct FreezeFlake
{
	float		mX;                 // +0x00
	float		mY;                 // +0x04
	float		mVelX;              // +0x08
	float		mVelY;              // +0x0c (not used)
	FrameAnim	mAnim;              // +0x10
};

// A warp star (0xa8 bytes, the vector at +0x150): a streak of light that falls over the warp screen (UpdateWarp).
// The original's object has only the vtable of its empty base class.
struct WarpStar
{
	float		mX;                 // +0x4c
	float		mY;                 // +0x50
	float		mField5C;           // +0x5c (set to 0, never read)
	float		mSpeed;             // +0x60 pixels per update
	FrameAnim	mAnim;              // +0x64
	Image*		mImage;             // +0xa4 one of the three sizes of IMAGE_WARP_FX
};

// A beam of the mode 2 bonus (12 bytes, the vector at +0x290)
struct LaserRec
{
	float		mAngle;             // +0x00 degrees: 0, 30, 60, ... 180
	int			mLife;              // +0x04 starts at 255, counts down by mDecay
	int			mDecay;             // +0x08 4 to 6 per update
};

// An explosion of the bonus of mode 1 (0x4c bytes, the vector at +0x1b0): a frame counter with a place
struct ExplosionAnim : public FrameAnim
{
	float		mX;                 // +0x40
	float		mY;                 // +0x44
	float		mStep;              // +0x48 6.283 divided by a number from 0 to 49 (the angle of the spin)
};

// A rate of one kind of brick for each of the three phases of a game (8 bytes)
struct RatePair
{
	float	mRate;              // +0x00 the share of a row's draws that make this kind
	float	m4;                 // +0x04 how much the share changes by each level

	RatePair() : mRate(0), m4(0) { }
};

// The rules for one kind of brick (0x18 bytes): how far apart two of them must be, and how likely each phase makes one.
// The rules of a field are kept in the order 7, 9, plain colour (killall), 6, 10, 0xb, 0xc, 0xd.
struct SpawnRule
{
	int					mNearSame;  // +0x00 how many columns apart two of the kind must be in a row
	int					mNearRow;   // +0x04 how many rows apart
	std::vector<RatePair>	mRates; // +0x08 by phase

	SpawnRule() : mNearSame(0x10000), mNearRow(0x10000), mRates(3) { }     // 0x44f490
};

struct SpawnRules
{
	SpawnRule	mRule[8];           // 0xc0 bytes
};

// The rows of a scripted game (the demo, the tutorial): ten brick codes a row, 100 rows (at +0x984; 0: none, 1-4 plain,
// 5 stone, 6-9 killall of a colour, 10 bomb)
struct ScriptRows
{
	int		mCode[100][10];

	ScriptRows() { memset(mCode, 0, sizeof(mCode)); }
};

// Two values that swing between 0 and 255 in turns (0x433850; the Board keeps one at +0x808)
struct PulsePair
{
	int		mA;                 // +0x00
	int		mB;                 // +0x04
	int		mDeltaA;            // +0x08
	int		mDeltaB;            // +0x0c
	int		mWait;              // +0x10 updates to wait before the next swing
	int		mWaitReset;         // +0x14

	void	Update(int theSpeed);                       // 0x433850
};

// The widget over the field (constructor 0x4327c0, vtable 0x563e28): it fades the field to a colour, shows the PAUSED
// text and the screens' darkening. The Board keeps it at +0x660 and points it back at itself (+0x7c).
class BoardOverlay : public Text2004<Widget>
{
public:
	Color			mColor58;           // +0x58 (white, clear)
	Color			mFill;              // +0x68 the colour the overlay fills the field with
	bool			mFlag78;            // +0x78
	Board*			mBoard;             // +0x7c
	bool			mFlag80;            // +0x80

	BoardOverlay();                                     // 0x4327c0
	void			SetFill(int theRed, int theGreen, int theBlue, int theAlpha);   // 0x432990
	void			SetFlag78(bool theFlag) { mFlag78 = theFlag; }                  // 0x4329c0

	// (the original's widget class also has two overrides that the port leaves to Widget: a mouse-move one and an
	// is-point-visible one; the widget is not mouse-visible, so neither is ever called)
	void			Update() override;                                              // 0x432930
	void			Draw(Graphics* g) override;                                     // 0x43fc10 (BoardOverlay.cpp)

private:
	void			DrawHint(Graphics* g);
	void			DrawWeaponHint(Graphics* g);
	void			DrawEmergency(Graphics* g);
};

extern bool gFlag5A5100;    // 0x5a5100

extern float gTestChargeAtReset;   // a test's weapon charge for a new board and ResetState (BoardState.cpp), below 0: none

class Board : public Text2004<Widget>, public EventListener
{
public:
	// BEGIN FIELDS (generated by tools/boardfields.py from docs/board_fields.tsv)
	std::list<Brick*>	mHeldBricks;// +0x70 the bricks the ship holds (the piece in hand, 1 to 6, first picked first); FirePiece shoots them up the column as one group
	std::list<std::list<Brick*> >	mFiredGroups;// +0x7c groups of bricks in flight to their places: the ones the ship fired and the falling groups OnBrickGone starts; OnBrickArrived puts them in the field
	std::list<Brick*>	mGoneBricks;// +0x88 bricks whose death is over (OnBrickGone fires them event 4); updated and drawn until each fires event 8 and moves to mFadedBricks
	std::list<std::list<Brick*> >	mFallingGroups;// +0x94 groups of bricks that lost their support (DropUnsupported), each with its target height; OnBrickGone starts them moving by handing them to mFiredGroups
	std::list<Brick*>	mTakenBricks;// +0xa0 bricks taken out of the field this update and not yet gone (RemoveBrick, RemoveCells), in step with mTakenCells; OnBrickGone hands them to mGoneBricks
	std::vector<CellRef>	mTakenCells;// +0xa8 the cell each taken brick was in, in step with mTakenBricks; DropUnsupported moves the bricks below those cells up the column, then the list is emptied
	std::vector<CellRef>	mConnectedCells;// +0xb8 the cells CollectConnected has gathered: joined cells holding a brick of one kind (the set a group makes); the blasts skip cells that are in it
	std::vector<CellRef>	mTurbotMarkedCells;// +0xc8 the cells of the bricks the mode 2 bonus's turbot wave will take (marked in BeginPlay, lit by SweepTurbots, taken as the wave reaches them)
	std::vector<BlinkRec>	mBeamSparks;// +0xd8 the 40 blinking sparks that drift along the beam under the pointer (DrawBeam): an x in the beam and a counter from 400 down to -20 that gives height and alpha
	std::vector<SunParticle*>	mSunSparks;// +0xe8 the 40 sparks of the sun that drift out over the end of a game, one every nine degrees (DrawEndSparks, stepped by UpdateGameOver)
	std::vector<int>	mKillAllTypes;// +0xf8 the brick kinds of the kill-all bricks that are done (OnKillAllDone); when the field is quiet every brick of those kinds is taken out, then the list is emptied
	std::vector<Brick*>	mFadedBricks;// +0x108 bricks that have faded out (event 8) and a column nuke fired in place; still updated and drawn over the ship until ClearField or the next level deletes them
	std::vector<Brick*>	mTargeted;// +0x118 the bricks that flash because the piece in hand would hit them (HighlightTargets)
	std::set<Brick*>	mDoomedBricks;// +0x128 the bricks to delete: DeleteDoomedBricks frees them and empties the set (ClearField, a new level and a bonus round put the old bricks in it)
	std::set<int>	mPlayedBonusStages;// +0x134 the numbers of the bonus stages (bonus files) already played; StartBonusRound picks one not in it, and the set is emptied at 30 entries and by GameOver
	std::vector<FloatText*>	mFloatTexts;// +0x140 the texts that float up the screen (points and announcements); drawn over the ship and freed when they are done
	std::vector<WarpStar*>	mWarpStars;// +0x150 the 15 warp stars that fall over the warp screen (Fn46a810)
	std::vector<PlanetRec>	mPlanets;// +0x160 the 5 planet layers of the backdrop (the second and third drawn only with 3D, the last two add light); StepScrollLayer scrolls them
	std::vector<StreakRec>	mStreaks;// +0x170 the 100 streaks of the backdrop (a pixel with a tail of the picture behind it); 25 are drawn without 3D, 100 with
	std::vector<SpawnRules>	mSpawnRules;// +0x180 the rules that decide which kinds of brick spawn: rates of each kind by phase (a one-element vector, only the first is read)
	std::vector<StarRec>	mBackdropStars;// +0x190 the 3 stars: pictures (IMAGE_BG_OBJECT1 to 7) that drift down the backdrop and wrap to above the highest one
	std::vector<CellRef3>	mAtomicMarkedBricks;// +0x1a0 the bricks marked for the mode 1 bonus (cell and brick, thinned out at random by the power level); they blow up when the meter runs out
	std::vector<ExplosionAnim*>	mAtomicExplosions;// +0x1b0 the up to 8 atomic explosion animations of the mode 1 bonus: seven over the marked bricks, the last the big blast (UpdateExplosion frees them)
	std::vector<Widget*>	mFieldButtons;// +0x1c0 the buttons of the field: one, the main button (id 0xfa1)
	std::vector<TipEntry>	mBrickTips;// +0x1d0 the pop-ups that describe the bricks (SetupHints): the picture and three lines each, pushed once for each place that shows them; nothing reads it
	std::vector<FadeRec>	mEndBlastFades;// +0x1e0 the 3 fading big-blast pictures (IMAGE_BIG_BLAST1 to 3) of the end of a game: alpha rises by a step to 255, falls by 4, one panel after the other
	std::vector<int>	mSpecialCooldowns;// +0x1f0 5 counters that count down from 2000 after a special brick is made (kill-all, bomb, row nuke, freeze, column nuke); that kind is not drawn while it is above 0
	std::vector<MarkRec>	mSuperchargeMarks;// +0x200 the marks of the supercharged colours (at most two): the brick kind, how many of it are to drop, and whether the mark is the lit one
	std::vector<StageBg*>	mHintPanels;// +0x210 the 12 hint panels: the first is the grab hint with the release and match hints chained after it, the others explain the brick kinds (ShowHint)
	std::vector<StageBg*>	mStageHintPanels;// +0x220 the hint panel of each of the first two stages (both the grab hint, whose chain goes on to the release and match hints); DrawStageBg shows the stage's
	std::vector<HintSpot*>	mHintSpots;// +0x230 the tutorial hints on the screen (ShowHint): each a panel, an arrow, a ring round its brick and the four rectangles that darken the rest
	std::vector<FreezeFlake>	mFreezeFlakes;// +0x240 the snowflakes of the freeze effect, at most 60 (a quarter of them drawn without 3D); they drift along the top of the screen
	std::vector<Image*>	mPhasePictures;// +0x250 the 14 pictures under the field in the mode's set (ids from 0x196, 0x1a4, 0x1b2 or 0x1c0), one for each phase of the game; 6 and 7 are the weapon's
	std::vector<DrawnText>	mBonusBannerTexts;// +0x260 the banner text, repeated to fill the width, that scrolls along the bottom while the bonus runs (UpdateSpecialMode)
	std::vector<DrawnText>	mSpecialBannerTexts;// +0x270 the banner text, repeated to fill the width, that scrolls along the bottom after a special brick is used (OnBlast names the bomb, nuke, freeze or booster)
	std::vector<RampRecord>	mSpeedRamp;// +0x280 the speed table (UpdateRamp): rows of an end time and values; the time played picks the two rows that the game's speeds are mixed from
	std::vector<LaserRec>	mTurbotBeams;// +0x290 the lightning beams of the mode 2 bonus, at most 6 (angle, life counting down from 255 by 4 to 6 an update), drawn at the ship (IMAGE_TURBOT_LIGHTNING)
	std::vector<Particle*>	mEnergySparks;// +0x2a0 the falling white sparks of the energy mode: UpdateSparks starts a row every 10 updates while the spark timer runs; freed when done
	std::vector<HintBox*>	mHintBoxes;// +0x2b0 the 3 hint boxes: the SUPA weapon, the meter, and a third that the pause fills
	std::vector<StoryEntry*>	mPilotStories;// +0x2c0 the stories of the pilot (BuildStoryText): two pages each
	std::vector<DemoStep>	mDemoSteps;// +0x2d0 the demo's steps (LoadDemo): a wait of a number of updates, a click (right or left) or a move of the pointer to one of ten columns
	Widget*	mDemoStartLink = NULL;// +0x2e0 the hyperlink (id 0xfa7) that the demo shows to invite the player to start a game; CloseOverlay removes it
	Widget*	mContinueButton = NULL;// +0x2e4 the continue button of the end page (id 0xfa4, made by CreateEndButtons)
	Widget*	mMenuButton = NULL;// +0x2e8 the menu button of the end page (id 0xfa5, made by CreateEndButtons)
	Widget*	mHelpButton = NULL;// +0x2ec the help button of the field (id 0xfa6, bottom left); disabled and hidden while the end buttons show
	Graphics*	mScreenGraphics = NULL;// +0x2f0 draws into mScreenImage (RedrawAfterDialog draws the widgets there)
	Image*	mScreenImage = NULL;// +0x2f4 the 640 x 480 picture of the screen that RedrawAfterDialog draws the widgets into and that the wipe at the end of a game takes away
	Image*	mWarpLightningImage = NULL;// +0x2f8 the lightning picture now flashing over the warp screens (one of three picked at random in UpdateScenery, drawn additively with 3D)
	MemoryImage*	mSirenImage = NULL;// +0x2fc a mirrored copy of IMAGE_SIREN, drawn with the cel of mSirenAnim
	MemoryImage*	mSirenFxImage = NULL;// +0x300 a mirrored copy of IMAGE_SIREN_FX, drawn with the cel of mSirenAnim
	MemoryImage*	mBannerTextImage = NULL;// +0x304 the banner's text drawn in white in its font (BuildBanner)
	MemoryImage*	mBannerWaveImage = NULL;// +0x308 the banner picture that is drawn: the text with each column moved down by a wave, 48 pixels taller than mBannerTextImage (BuildBanner)
	MemoryImage*	mRankImage = NULL;// +0x30c the picture of the player's rank (MakeRankImage, height 0x50), drawn in the HUD and made again when the rank changes
	ScreenWipe*	mScreenWipe = NULL;// +0x310 the wipe that ends a game (one of 4 kinds, picked at random); it takes mScreenImage away and the board closes when it is done
	std::vector<std::vector<Brick*> >	mFieldBricks;// +0x314 the field: 20 rows of 10 cells, row 0 on top (the stack scans run over rows 1 to 19); a brick, or NULL for an empty cell
	std::vector<CellRef>	mHoverCells;// +0x324 the cells lit around the pointer (HoverCell: the one under it and, by the power level, the cells round it); the first is the one under the pointer
	std::vector<std::vector<int> >	mDemoLayout;// +0x334 the layout of the demo's field (LoadDemo): 20 rows of 10 brick kinds, -1 for none; BuildField makes the field from it
	std::vector<std::vector<int> >	mDemoAddedRows;// +0x344 the rows added during the demo (the file's added rows, and the kinds of row 0 each time a game is recorded): one kind for each column
	Brick*	mHandKindBrick = NULL;// +0x354 the brick that stands for the kind in hand (MakeHand: a copy of the first brick taken, at the bottom of the field); MatchesHand compares with its kind
	AstroPopApp*	mApp = NULL;// +0x358 the application (the bricks play their sounds through it)
	PointerTrail	mPointerTrail;// +0x35c the pointer's recent steps (the original's draw feeds it); the port neither writes nor reads it
	Color	mBackdropTint;// +0x3c0 the tint of the backdrop (FadeBackdrop moves it 8 steps at a time towards the colour of the first held brick; grey 100, 100, 100 when none)
	Color	mOverlayFillColor;// +0x3d0 the colour the overlay is filled with: UpdateEffects counts its alpha down by 2 an update and hides the overlay at 0, the warp raises it to white
	FrameAnim	mMeterFullAnim;// +0x3e0 restarted when the weapon meter fills (SOUND_POWERUP), runs while it stays full and stops when the bonus ends; nothing draws it
	int	mAlertLevel = 0;// +0x404 1 while the stack is low or the game is over, else the stack height minus 12 up to 5 (UpdateAlertFlash); nothing reads it
	FrameAnim	mLaserAnim;// +0x420 the ship's laser (IMAGE_PLAYER1_LASER, mode 0 only), drawn while the bonus runs; a shot restarts it
	FrameAnim	mWarpGateAnim;// +0x460 the warp gates' animation (IMAGE_SLOW_WARP_GATE, IMAGE_FAST_WARP_GATE), stepped by Fn46a810 while a warp runs
	FrameAnim	mSirenAnim;// +0x4a0 the siren (IMAGE_SIREN)
	FrameAnim	mStageMeterAnim;// +0x4e0 reset when the stage's brick count reaches a quarter, a half or three quarters and when a booster adds to it; nothing updates or reads it
	FrameAnim	mTurbotPowerupAnim;// +0x520 the turbot power-up animation (IMAGE_TURBOT_POWERUP, mode 2 only), drawn at the ship while the bonus runs
	FrameAnim	mBoosterAnim;// +0x560 restarted when a booster brick adds to the brickometer (OnHyperBrick: 31 frames, ping-pong, 20 loops) and updated every update; nothing draws it
	FrameAnim	mResultPulseAnim;// +0x5a0 pulse of the end of a game (CreateEndButtons restarts it: 31 frames, ping-pong); its cel sets the alpha of the bar lit behind the player's result in the end page
	FrameAnim	mNewRankAnim;// +0x5e0 flash of a new rank: stopped by ResetGame, started when the rank beats the profile's best (Fn46a810); DrawHud flashes the rank's name red by its frame
	FrameAnim	mSuperchargeBlastAnim;// +0x620 the fuel cell blast (IMAGE_FUELCELL_DEATH) drawn at the top right: UpdateSupercharge starts it when called with its check set (7 updates a cel, holds the last cel)
	BoardOverlay	mOverlay;// +0x660 the widget over the field (BoardOverlay): fades the field to its fill colour, draws the pause box, the hint panels and the weapon hint; added over the Board's widgets
	SoundInstance*	mPauseMutedSound = NULL;// +0x6e4 a sound that Pause silences (volume 0) and the end of the pause restores (1.0); the destructor releases it; the port never assigns it, so it stays NULL
	int	mPointsTextX = 0;// +0x6e8 x of the brick that went first in the current removal, where its points text floats (OnBrickGone sets it, -1 when none; PayComboBonus also uses it)
	int	mPointsTextY = 0;// +0x6ec y of the brick that went first in the current removal, where its points text floats (OnBrickGone sets it, -1 when none; PayComboBonus uses it plus 25)
	int	mShipShimmerX = 0;// +0x6f0 left edge of the white band that sweeps over the ship while a bonus runs (DrawShip); always 0, set when the weapon starts
	int	mShipShimmerY = 0;// +0x6f4 top of the shimmer band in the ship's picture: 0 while the band grows, then 1 and up by 2 an update to 52, when the band starts again (UpdateSpecialMode)
	int	mShipShimmerWidth = 0;// +0x6f8 width of the shimmer band: 53 (set when the weapon starts, never changes)
	int	mShipShimmerHeight = 0;// +0x6fc height of the shimmer band: grows by 2 an update with the top at 0, then is 52 minus the top as the top moves down; the band is drawn while above 0
	int	mBrickometerRiseStep = 0;// +0x700 step counter of the brickometer's rise (set to 0 by OnHyperBrick); nothing reads it
	int	mBrickometerRiseTop = 0;// +0x704 y of the brickometer's top after OnHyperBrick filled it (444 minus its height); nothing reads it
	int	mBrickometerRiseSteps = 0;// +0x708 number of steps (23) of the brickometer's rise that OnHyperBrick sets up; nothing reads it
	int	mBrickometerRiseDistance = 0;// +0x70c how far the brickometer's top rose in pixels, old top minus new top (OnHyperBrick); nothing reads it
	std::string	mWeaponHintText;// +0x710 the text of the weapon prompt (a game text set when the Board is made); nothing reads it
	std::string	mStoryResourceGroup;// +0x72c name of the resource group of the story pages being shown (the pilot's Page<n> group that OpenStoryScreen loads)
	XcfFile*	mBonusRoundFile = NULL;// +0x748 the file of the bonus round (data\bonus<n>.xcf) that StartBonusRound reads; the Board owns it and deletes it
	AstroButton*	mHintOkButton = NULL;// +0x74c the OK button of a hint panel (id 0xfa2), coloured like the stage (RecolorThemeButtons); shown with the hint, hidden when DismissHint ends it
	Checkbox*	mHintOffCheckbox = NULL;// +0x750 the check box of a hint panel (id 0xfa3) that switches the hints off (DismissHint reads it); coloured like the stage
	StoryScreen*	mStoryScreen = NULL;// +0x754 the story screen over the Board (OpenStoryScreen makes it; the end of the story removes it); NULL when none
	Buffer	mDemoRecordBuffer;// +0x758 the demo's recording: RecordStep appends bytes for the changes of the input state and the update counts between them; nothing reads it
	LevelUpText*	mLevelUpText = NULL;// +0x778 the text that swooshes in over the field: the level-complete picture, a text at a quarter, half or three quarters of a stage, a minute of a survival game
	ScrollLayer	mSceneryLayer;// +0x77c the scenery behind the field: one of three pictures chosen by stage, scrolled down by StepScrollLayer, loaded again after each warp
	float	mSceneryX = 0.0f;// +0x7b4 x at which the scenery picture is drawn (DrawBackdrop); nothing writes it, so it is always 0
	int	mBonusRoundPoints = 0;// +0x7b8 points scored in a bonus round (AddScore while one runs; a forced add always counts); saved in the profile as setting 0x62; part of the total the panel shows
	int	mPlayPoints = 0;// +0x7bc points scored outside bonus rounds (AddScore), saved in the profile as setting 0x5c; first line of the level-up panel
	int	mBricksGoneCount = 0;// +0x7c0 bricks gone since the game began (OnBrickGone counts each; ResetGame clears it); nothing reads it
	int	mBestChain = 0;// +0x7c4 the longest chain of sets in this game (AdvanceChain); also kept in the profile as setting 0x5d; second line of the level-up panel
	int	mFieldsCleared = 0;// +0x7c8 fields cleared in this game (ClearBonus counts each); added to the profile's total of setting 0x2f when the game ends
	std::vector<std::string>	mLevelUpLines;// +0x7cc the lines the level-up panel types out: points, best chain, time, the total score when a bonus round follows, the points the next rank needs
	int	mLevelUpTypeInterval = 0;// +0x7dc updates between two letters typed in the level-up panel (5, set when the panel is made)
	int	mLevelUpLineIndex = 0;// +0x7e0 index of the level-up panel's line being typed; the lines before it are done, and past the last line it stays at the line count
	int	mLevelUpLetterIndex = 0;// +0x7e4 index of the last typed letter of the line being typed (DrawHud shows that many plus one letters of it)
	int	mLevelUpPanelY = 0;// +0x7e8 y of the level-up panel (IMAGE_HUD_UI): the negative of its height while hidden above the screen, 0 when down; slides 2 an update (Fn46a810)
	int	mLevelUpPanelHeight = 0;// +0x7ec height of the level-up panel: 206, so it slides in from that far above the screen (the brick generator's panel moves out as it comes in)
	int	mLevelUpScanY = 0;// +0x7f0 y of the green bar that scans down the level-up panel (3D only, DrawHud): held above the panel until it is down, then from 0 down to 187
	int	mLevelUpScanHeight = 0;// +0x7f4 height of the scan bar: grows to mLevelUpScanMaxHeight at the top, then the bar moves down and shrinks away at the bottom (0 when off)
	int	mLevelUpScanMaxHeight = 0;// +0x7f8 the greatest height of the scan bar: 10
	int	mLevelUpScanInterval = 0;// +0x7fc updates between steps of the scan bar: 2
	int	mLevelUpCursorInterval = 0;// +0x800 updates between blinks of the cursor once every line of the level-up panel is typed: 25
	bool	mLevelUpCursorShown = false;// +0x804 the cursor after the typed letters of the level-up panel is drawn (blinks every mLevelUpCursorInterval updates once the lines are all typed)
	PulsePair	mWeaponHintPulse;// +0x808 the two alphas (0 to 255, swinging in turns, PulsePair::Update) of the two pictures of the pilot's weapon in the weapon hint (DrawWeaponHint)
	int	mLeftArmX = 0;// +0x820 x of the left arm of the upgrade screen: slides in from minus its width to its stop and out again (Fn46a810)
	int	mLeftArmY = 0;// +0x824 y of the left arm and its head on the upgrade screen: 416
	int	mLeftArmStopX = 0;// +0x828 x where the left arm stops sliding in: 130 (SetupUpgradeAnims)
	int	mLeftArmStopY = 0;// +0x82c y that SetupUpgradeAnims sets for the left arm's stop: 416; nothing reads it
	bool	mLeftArmImagesOwned = false;// +0x830 the flipped copies of the arm and head (the left arm's pictures) belong to the Board and the destructor deletes them; set by SetupUpgradeAnims
	MemoryImage*	mLeftArmImage = NULL;// +0x834 flipped copy of the upgrade arm picture (made when the Board is made): the arm that slides in from the left
	MemoryImage*	mLeftHeadImage = NULL;// +0x838 flipped copy of the upgrade head picture: the head on the left arm, drawn with the cel of mLeftHeadAnim
	FrameAnim	mLeftHeadAnim;// +0x83c animation of the left arm's head: cels 0 to 5, backwards, 6 updates a cel; runs once the arm has stopped (Fn46a810)
	int	mRightArmX = 0;// +0x87c x of the right arm of the upgrade screen: starts at 660, past the right edge, slides in to its stop and out again (Fn46a810)
	int	mRightArmY = 0;// +0x880 y of the right arm and its head on the upgrade screen: 416
	int	mRightArmStopX = 0;// +0x884 x where the right arm stops sliding in: 463 (SetupUpgradeAnims)
	int	mRightArmStopY = 0;// +0x888 y that SetupUpgradeAnims sets for the right arm's stop: 416; nothing reads it
	bool	mRightArmImagesOwned = false;// +0x88c the destructor deletes the right arm's pictures when it is set; nothing sets it (they are the resource's pictures)
	Image*	mRightArmImage = NULL;// +0x890 the upgrade arm picture (IMAGE_UPGRADE_ARM, the resource's own) of the arm that slides in from the right
	Image*	mRightHeadImage = NULL;// +0x894 the upgrade head picture (IMAGE_UPGRADE_HEAD, the resource's own): the head of the right arm, drawn with the cel of mRightHeadAnim
	FrameAnim	mRightHeadAnim;// +0x898 animation of the right arm's head: cels 16 to 21, 6 updates a cel; runs once the arm has stopped (Fn46a810)
	bool	mDemoEnding = false;// +0x8d8 the demo is ending: CloseOverlay sets it, the white overlay fades in, then the demo stops and a new game starts (Update)
	bool	mFadingFromWhite = false;// +0x8d9 after the demo ends the white overlay fades out again (its alpha falls by 6 an update); cleared when it is clear
	bool	mBonusFieldLoaded = false;// +0x8da a bonus round's field has been loaded (StartBonusRound sets it, LoadLevel clears it); DrawBricks then leaves out row 0
	bool	mRecordingDemo = false;// +0x8db the game is being recorded as a demo (a red box flashes, RecordStep writes each update's input); the port never sets it
	bool	mDemoPlaying = false;// +0x8dc the demo plays: the title's game plays by itself from mDemoSteps and real input is ignored; the end fade clears it
	bool	mDemoInputActive = false;// +0x8dd set while a demo step sends its own click or pointer move to the input handlers, which ignore input while the demo plays unless it is set
	bool	mLevelUpTextShown = false;// +0x8de the level-up text is on the screen: it is updated and drawn while this is set; set when a new one starts, cleared when its animation is done
	bool	mStartWeaponAfterPause = false;// +0x8df set when a click on the weapon prompt ends the pause (MouseDown); at the end of that pause Pause calls BeginPlay once
	bool	mWelcomePlayed = false;// +0x8e0 the welcome voice has played in this game (Update plays it once no demo runs, and starts the engine loop); ResetState clears it
	bool	mFirstHelpPending = false;// +0x8e1 the full-screen help is still to show once (copied from the application's help flag when the Board is made); Update shows it and clears this
	bool	mUnusedClearedInCtor = false;// +0x8e2 set to false by the constructor only; nothing else writes or reads it
	bool	mTopRowsAllowed = false;// +0x8e3 gate of SpawnControl: ResetGame sets it and nothing clears it; a new top row comes only while it is set (and no bounce, bonus, warp or end runs)
	bool	mNewRowBouncing = false;// +0x8e4 AddTopRow sets it when the new top row starts to bounce, OnBrickBounced clears it once the row is still; meanwhile no new row, start or save
	bool	mPickUpButtonDown = false;// +0x8e5 the pick-up button (down key or left mouse button) is held; PickUp refuses without it; MouseUp and clicks that are no pick-up clear it
	bool	mBackdropTintChanging = false;// +0x8e6 the backdrop's tint is still moving to the colour of the first held brick: set when the hand changes (PickUpDone, nukes), cleared by FadeBackdrop on arrival
	bool	mGameWasReset = false;// +0x8e7 ResetGame sets it; nothing clears or reads it
	bool	mGameOver = false;// +0x8e8 the game is over: GameOver sets it and ResetGame clears it; Update runs UpdateGameOver meanwhile, input stops and SaveFinalScore stores the result
	bool	mLevelUpPhase = false;// +0x8e9 the level is complete and the level-up screen with the warp follows: LevelComplete sets it (not for a bonus round), ResetGame clears it; input and new rows wait
	bool	mStreaksFadingOut = false;// +0x8ea the warp streaks are fading out after a stage is loaded (LoadLevel sets it, UpdateEffects clears it when all 100 have shrunk away); input and new rows wait
	bool	mWarpActive = false;// +0x8eb the warp to the next stage is under way, from the end of its charge-up until the next stage is loaded (UpdateWarp); the hyperspace loop plays
	bool	mWarpCharging = false;// +0x8ec the charge-up before the warp: StartLevelUp sets it, the screen shakes and the white rises until UpdateWarp starts the warp
	bool	mWarpFlight = false;// +0x8ed the flight through the warp tunnel: the field has flown away, stars fall, the screen darkens and shakes until the gate comes out (UpdateWarp)
	bool	mFuelCellDue = false;// +0x8ee a fuel cell is to go into the next new top row: set after a long chain (OnBrickArrived, OnBrickGone), cleared by FillTopRow when it picks the column
	bool	mAtWarpGate = false;// +0x8ef the warp has come out at its gate screen: the gates light up under the pointer and a click on one picks the next stage (UpdateWarp, MouseMove)
	bool	mShipInWarp = false;// +0x8f0 the ship is drawn flying with the warp at the warp's own height instead of at the pointer; UpdateWarp sets it when the white-out ends
	bool	mPointerOverEasyGate = false;// +0x8f1 the pointer is over the easy gate of the warp screen and lights it; MouseMove only ever clears it, so it stays false
	bool	mPointerOverSlowGate = false;// +0x8f2 the pointer is over the slow gate of the warp screen (MouseMove sets it, UpdateWarp raises that gate's light while it is set)
	bool	mPointerOverFastGate = false;// +0x8f3 the pointer is over the fast gate of the warp screen, which exists only on later stages (MouseMove sets it, UpdateWarp raises that gate's light)
	bool	mWarpGatePicked = false;// +0x8f4 a gate of the warp screen has been clicked (MouseDown): the screen fades to white and UpdateWarp then loads the next stage
	bool	mHundredLeftSoundPlayed = false;// +0x8f5 the sound for the last hundred bricks of the stage (SOUND_LEVEL_UP1) has played; OnBrickGone sets it, ResetGame clears it
	bool	mFiftyLeftSoundPlayed = false;// +0x8f6 the sound for the last fifty bricks of the stage (SOUND_LEVEL_UP2) has played; OnBrickGone sets it, ResetGame clears it
	bool	mPauseTextWanted = false;// +0x8f7 Pause's second argument: true for a pause that losing the window's focus makes, false for one that a box makes; the overlay writes its pause texts only while it is set
	bool	mTimeBonusRunning = false;// +0x8f8 the time bonus window is open: AdvanceChain opens it with the stage's first set, PayTimeBonus pays for 3 to 6 quick sets, it closes after 700 updates
	bool	mAtomicBlastOver = false;// +0x8f9 the atomic blast animations are through (UpdateExplosion sets it), so the marked bricks of the mode 1 bonus go; UpdateSpecialMode clears it
	bool	mWarpEngaged = false;// +0x8fa set when the warp's white-out ends (SOUND_WARP_ENGAGED plays, the field is cleared); ResetGame clears it; nothing reads it
	bool	mFieldCleared = false;// +0x8fb nothing is left in rows 1 to 19 once everything has settled (ClearBonus is paid); FillTopRow and ResetGame clear it; the next row waits for it in a bonus
	bool	mPlayStarted = false;// +0x8fc the game has really begun: the first pick-up, shot, set or weapon start sets it (event 0x2a fires once); rows, countdowns and the freeze wait for it
	bool	mUnusedClearedInReset = false;// +0x8fd ResetGame clears it; nothing else writes or reads it
	bool	mSetMadeSinceTopRow = false;// +0x8fe a set of enough bricks was made (OnBrickArrived plays its sound); SpawnControl clears it when the next top row is due; nothing reads it
	bool	mSlowMotion = false;// +0x8ff when set only every eighth Update does the game's work (the others only record the demo step); nothing ever sets it
	bool	mLevelCompleteBlocked = false;// +0x900 LevelComplete returns at once while it is set; the constructor clears it and nothing sets it
	bool	mScreenWiping = false;// +0x901 the end-of-game screen wipe runs: Update waits for the wipe to end and then closes the board, Draw only draws the wipe; RedrawAfterDialog sets it
	bool	mWipeImageCaptured = false;// +0x902 cleared by the constructor and when RedrawAfterDialog starts to draw the screen into the wipe's picture; nothing reads it
	bool	mWipeImageUsed = false;// +0x903 cleared by the constructor and once RedrawAfterDialog has drawn the screen into the wipe's picture; nothing reads it
	bool	mWipeCreated = false;// +0x904 the constructor sets it once the screen picture and the wipe are made; nothing reads it
	bool	mGameOverAnimDone = false;// +0x905 the end-of-game blasts and fades are through (UpdateGameOver sets it, then makes the end buttons); UpdateGameOver returns at once while it is set
	bool	mMeterPaused = false;// +0x906 while set the weapon meter does not run down in UpdateSpecialMode; the constructor clears it and nothing sets it
	bool	mClearWaitStarted = false;// +0x907 mode 2: the wait for the field to be cleared has begun (BeginClearWait); set by ResetGame, cleared when the weapon starts so that the next quiet field begins it
	bool	mInHallOfFame = false;// +0x908 whether the game's result got into the hall of fame (AddHallOfFame's answer in SaveFinalScore and GameOver); nothing reads it
	bool	mStackWarningActive = false;// +0x909 the warning of a too high stack at the ship's column is on (CheckShipColumn sets it, 400 updates); SpawnControl holds the next top row back meanwhile
	bool	mTutorialStageForced = false;// +0x90a the first stage's tutorial hints apply whatever the stage number is; the constructor clears it and nothing sets it
	bool	mClickRepeating = false;// +0x90b set while Update repeats the held left button's click (classic game, weapon on); the pick-up sounds stay silent then; ResetState clears it
	bool	mUnusedFlagFromCtor = false;// +0x90c the constructor clears it; nothing else writes or reads it
	bool	mUnusedFlagFromReset = false;// +0x90d ResetGame clears it; nothing else writes or reads it
	bool	mBoosterDue = false;// +0x90e a booster brick is to go into the next new top row: set by AdvanceChain (long chain) and PayTimeBonus, cleared by FillTopRow when it picks the column
	bool	mClicksBlocked = false;// +0x90f while set MouseDown ignores the buttons and MouseUp keeps the pick-up flag: set when a bonus round starts, a box closes or the screen is redrawn after a box
	bool	mBonusRoundOn = false;// +0x910 a bonus round is on (from LevelComplete's bonus branch or a restored bonus game until its tally ends): no meter, no stage counting, bricks from a bonus file
	bool	mStartBannerDoneInBonus = false;// +0x911 set by UpdateStartBanner when the start banner has run through while a bonus round is on; StartBonusRound and ResetGame clear it; it holds back the round's no-move end
	bool	mBricksRevealing = false;// +0x912 the bonus round's bricks are still being shown one by one (StartBonusRound sets it, RevealBricks clears it); the first click and the end checks wait
	bool	mBonusRoundStarted = false;// +0x913 the player's first click has started the bonus round after its bricks appeared (MouseDown, event 0x28 fired); StartBonusRound clears it; the time runs from it
	bool	mBonusRoundLoaded = false;// +0x914 StartBonusRound sets it once the round's file is read; LevelComplete and the restore clear it; nothing reads it
	bool	mBonusFadingOut = false;// +0x915 while set UpdateIntroAndTally lowers the bonus fade value by 2 an update to 0; nothing ever sets it
	bool	mClearCountRunning = false;// +0x916 the count-down that pays the bricks left over runs (StartClearCount sets it, the tally's end clears it); the held pieces stand over the stack and clicks do nothing
	bool	mBonusWarpIn = false;// +0x917 the warp into a bonus round is running (LevelComplete sets it; UpdateIntroAndTally steps it through three parts before StartBonusRound); the screen is white
	bool	mBonusRevealDone = false;// +0x918 set once the bonus round's bricks have all appeared (RevealBricks, which also sets the banner's count to 300); a new bonus round clears it; only RevealBricks reads it
	bool	mWeaponHintShown = false;// +0x919 the pilot's weapon hint is the panel the overlay shows instead of a stage hint (DrawWeaponHint); DismissHint clears it; nothing sets it
	bool	mUpgradeDialogDone = false;// +0x91a the upgrade dialog has been closed (TakeFocus; a restored game starts with it set); UpdateWarp waits for it before the gate screen goes on and DrawWarpScreen draws
	bool	mPsychicUsed = false;// +0x91b mode 3: the psychic has taken bricks in this weapon run (PopPsychic sets it, UpdateSpecialMode clears it); meter and the time wait for it, a dark strip shows until then
	bool	mLaserUsed = false;// +0x91c mode 0: the laser has taken a brick in this weapon run (PickUp sets it, UpdateSpecialMode clears it); the meter runs down and the time counts only then
	bool	mEndPageShown = false;// +0x91d the end page is on the screen: CreateEndButtons sets it, Draw then calls DrawEndPage (the best results and the two buttons)
	bool	mHighScoreSoundDone = false;// +0x91e set once the delay after the game's end (a counter of 200 updates) is over and the new-high-score sound has played if the result was the best (Update)
	bool	mNewRankDelayOver = false;// +0x91f the wait before the new rank's flash is over: UpdateWarp clears it when the level beats the best one and starts the wait, Update sets it when the wait ends
	bool	mSupaButtonLit = false;// +0x920 the weapon button's alert lights are on: toggled every 50 updates while the blink counter runs (UpdateAlertFlash), DrawFrame chooses the lit or the dark picture
	bool	mUpgradeDialogSeen = false;// +0x921 an upgrade dialog has been closed in this game (TakeFocus sets it, ResetGame clears it); UpdateWarp then makes the new rank's flash wait 1 update instead of 100
	bool	mStartedFromCheckpoint = false;// +0x922 the game began at a level checkpoint (the constructor's flag, from StartClassic); the first play action clears it; DrawGoalBanners writes the stage text only while clear
	bool	mStageTextEnabled = false;// +0x923 the stage's brick-count text may be drawn over the field: false for a new game at level 1 and for a restored game, true once a level is done or the game starts higher
	bool	mComboTextVisible = false;// +0x924 the combo counter's "x<n>" text blinks: UpdateStates toggles it every 20 updates while the combo display timer runs; a combo below 6 is always drawn
	float	mBrickometerFill = 0.0f;// +0x928 how full the brickometer is, 0 to 1: bricks gone over the stage's goal (OnBrickGone); a warp pick starts it at 0.98 and it drains; 0 in a timed game
	float	mBrickometerDrain = 0.0f;// +0x92c the level the brickometer drains from after a warp pick (0.98, falling 0.0076 an update in UpdateEffects; mBrickometerFill follows it); 0 when idle
	float	mStackDanger = 0.0f;// +0x930 the danger of the stack: mean column height plus half the distance of the highest column from it (CheckStackHeight); scales the wait for the next top row
	float	m934 = 0.0f;// +0x934 ?
	float	m938 = 0.0f;// +0x938
	float	m93c = 0.0f;// +0x93c
	float	m940 = 0.0f;// +0x940
	float	m944 = 0.0f;// +0x944 the banner time (adds 0.07 per update)
	unsigned char	m948 = 0;// +0x948
	unsigned char	m949 = 0;// +0x949
	int	m94c = 0;// +0x94c ?
	int	m950 = 0;// +0x950 ?
	int	m954 = 0;// +0x954 ?
	int	m958 = 0;// +0x958 ?
	int	m95c = 0;// +0x95c ?
	int	m960 = 0;// +0x960 ?
	int	m964 = 0;// +0x964 ?
	int	m968 = 0;// +0x968 ?
	int	m96c = 0;// +0x96c ?
	int	m970 = 0;// +0x970 the highest stack seen last
	int	m974 = 0;// +0x974 ?
	int	m978 = 0;// +0x978 ?
	int	m97c = 0;// +0x97c ?
	int	m980 = 0;// +0x980 ?
	ScriptRows	m984;// +0x984 the rows of a scripted game
	int	made = 0;// +0xade ?
	int	m1924 = 0;// +0x1924 ?
	int	m1928 = 0;// +0x1928 ?
	int	m192c = 0;// +0x192c ?
	int	m1930 = 0;// +0x1930 the row of m984 to play next (-1 none)
	int	m1934 = 0;// +0x1934 ?
	int	m1938 = 0;// +0x1938 ?
	int	mField193C = 0;// +0x193c the stage: 1 + the time played / 6000
	int	m1940 = 0;// +0x1940 ?
	int	m1944 = 0;// +0x1944 ?
	int	m1948 = 0;// +0x1948 the white flash of the end: alpha, counts down by 2
	int	m194c = 0;// +0x194c ?
	int	m1950 = 0;// +0x1950 ?
	int	m1954 = 0;// +0x1954 ?
	int	m1958 = 0;// +0x1958 the pointer's x
	int	m195c = 0;// +0x195c the pointer's y
	int	m1960 = 0;// +0x1960 ?
	int	m1964 = 0;// +0x1964 ?
	int	m1968 = 0;// +0x1968 ?
	int	m196c = 0;// +0x196c ?
	int	m1970 = 0;// +0x1970 ?
	int	mScore = 0;// +0x1974 the score (profile setting 0x51)
	int	m1978 = 0;// +0x1978 ?
	int	m197c = 0;// +0x197c ?
	int	m1980 = 0;// +0x1980 ?
	int	m1984 = 0;// +0x1984 ?
	int	m1988 = 0;// +0x1988 ?
	int	m198c = 0;// +0x198c ?
	int	m1990 = 0;// +0x1990 ?
	int	m1994 = 0;// +0x1994 ?
	int	m1998 = 0;// +0x1998 ?
	int	m199c = 0;// +0x199c ?
	int	m19a0 = 0;// +0x19a0 ?
	int	m19a4 = 0;// +0x19a4 ?
	int	m19a8 = 0;// +0x19a8 ?
	int	m19ac = 0;// +0x19ac ?
	int	m19b0 = 0;// +0x19b0 ?
	int	m19b4 = 0;// +0x19b4 ?
	int	m19b8 = 0;// +0x19b8 ?
	int	m19bc = 0;// +0x19bc ?
	int	m19c0 = 0;// +0x19c0 ?
	int	m19c4 = 0;// +0x19c4 ?
	int	m19c8 = 0;// +0x19c8 ?
	int	m19cc = 0;// +0x19cc ?
	int	m19d0 = 0;// +0x19d0 ?
	int	m19d4 = 0;// +0x19d4 ?
	int	m19d8 = 0;// +0x19d8 ?
	int	m19dc = 0;// +0x19dc ?
	int	m19e0 = 0;// +0x19e0 ?
	int	m19e4 = 0;// +0x19e4 ?
	int	m19e8 = 0;// +0x19e8 ?
	int	m19ec = 0;// +0x19ec ?
	int	m19f0 = 0;// +0x19f0 ?
	int	m19f4 = 0;// +0x19f4 ?
	int	m19f8 = 0;// +0x19f8 ?
	int	m19fc = 0;// +0x19fc ?
	int	m1a00 = 0;// +0x1a00 ?
	int	m1a04 = 0;// +0x1a04 ?
	int	m1a08 = 0;// +0x1a08 ?
	int	m1a0c = 0;// +0x1a0c ?
	int	m1a10 = 0;// +0x1a10 ?
	int	m1a14 = 0;// +0x1a14 ?
	int	m1a18 = 0;// +0x1a18 ?
	int	m1a1c = 0;// +0x1a1c ?
	int	m1a20 = 0;// +0x1a20 ?
	int	m1a24 = 0;// +0x1a24 ?
	int	m1a28 = 0;// +0x1a28 ?
	int	m1a2c = 0;// +0x1a2c ?
	int	m1a30 = 0;// +0x1a30 ?
	int	m1a34 = 0;// +0x1a34 ?
	int	mMode = 0;// +0x1a38 the game mode (profile setting 0x53); also the key of the best score
	int	m1a3c = 0;// +0x1a3c ?
	int	m1a40 = 0;// +0x1a40 ?
	int	m1a44 = 0;// +0x1a44 ?
	int	m1a48 = 0;// +0x1a48 ?
	int	m1a4c = 0;// +0x1a4c ?
	int	m1a50 = 0;// +0x1a50 ?
	int	m1a54 = 0;// +0x1a54 the size of a group of same-coloured bricks that is broken up in a new row
	int	m1a58 = 0;// +0x1a58 ?
	int	m1a5c = 0;// +0x1a5c ?
	int	m1a60 = 0;// +0x1a60 ?
	int	m1a68 = 0;// +0x1a68 ?
	int	m1a70 = 0;// +0x1a70 ?
	int	m1a74 = 0;// +0x1a74 ?
	int	m1a78 = 0;// +0x1a78 ?
	int	m1a7c = 0;// +0x1a7c ?
	int	m1a80 = 0;// +0x1a80 ?
	int	m1a84 = 0;// +0x1a84
	int	m1a88 = 0;// +0x1a88 ?
	int	m1a8c = 0;// +0x1a8c ?
	int	m1a90 = 0;// +0x1a90 ?
	int	m1a94 = 0;// +0x1a94 ?
	int	m1a98 = 0;// +0x1a98 a time (in updates); -500000000: none
	int	m1a9c = 0;// +0x1a9c the same
	int	m1aa0 = 0;// +0x1aa0
	int	m1aa4 = 0;// +0x1aa4 ?
	int	m1aa8 = 0;// +0x1aa8 ?
	int	m1aac = 0;// +0x1aac ?
	int	m1ab0 = 0;// +0x1ab0 ?
	int	m1ab4 = 0;// +0x1ab4 ?
	int	m1ab8 = 0;// +0x1ab8 ?
	int	m1abc = 0;// +0x1abc ?
	int	m1ac0 = 0;// +0x1ac0 ?
	int	m1ac4 = 0;// +0x1ac4 ?
	int	m1ac8 = 0;// +0x1ac8 ?
	int	m1acc = 0;// +0x1acc ?
	int	m1ad0 = 0;// +0x1ad0
	int	m1ad4 = 0;// +0x1ad4
	int	m1ad8 = 0;// +0x1ad8
	int	m1adc = 0;// +0x1adc
	int	m1ae0 = 0;// +0x1ae0
	int	m1ae4 = 0;// +0x1ae4 ?
	int	m1ae8 = 0;// +0x1ae8 ?
	int	m1aec = 0;// +0x1aec
	int	m1af0 = 0;// +0x1af0 ?
	int	m1af4 = 0;// +0x1af4 ?
	int	m1af8 = 0;// +0x1af8 ?
	int	m1afc = 0;// +0x1afc ?
	int	m1b00 = 0;// +0x1b00 ?
	int	m1b04 = 0;// +0x1b04 ?
	int	m1b08 = 0;// +0x1b08 ?
	int	m1b0c = 0;// +0x1b0c ?
	int	m1b10 = 0;// +0x1b10 ?
	int	m1b14 = 0;// +0x1b14 ?
	int	m1b18 = 0;// +0x1b18 ?
	int	m1b1c = 0;// +0x1b1c ?
	int	m1b20 = 0;// +0x1b20 ?
	int	m1b24 = 0;// +0x1b24 ?
	int	m1b28 = 0;// +0x1b28 ?
	int	m1b2c = 0;// +0x1b2c ?
	int	m1b30 = 0;// +0x1b30
	int	m1b34 = 0;// +0x1b34 counts up by 4 (mod 256) while the game ends
	int	m1b38 = 0;// +0x1b38 ?
	int	m1b3c = 0;// +0x1b3c ?
	int	m1b40 = 0;// +0x1b40
	int	m1b44 = 0;// +0x1b44 ?
	int	m1b48 = 0;// +0x1b48 ?
	int	m1b4c = 0;// +0x1b4c
	int	m1b50 = 0;// +0x1b50
	int	m1b54 = 0;// +0x1b54 ?
	int	m1b58 = 0;// +0x1b58
	int	m1b5c = 0;// +0x1b5c ?
	int	m1b60 = 0;// +0x1b60 ?
	int	m1b64 = 0;// +0x1b64 ?
	int	m1b68 = 0;// +0x1b68 ?
	int	m1b6c = 0;// +0x1b6c the best level of the mode rounded down to a multiple of four, -1 none
	int	m1b70 = 0;// +0x1b70 ?
	int	m1b74 = 0;// +0x1b74 ?
	int	m1b78 = 0;// +0x1b78
	int	m1b7c = 0;// +0x1b7c ?
	int	m1b80 = 0;// +0x1b80 ?
	int	m1b84 = 0;// +0x1b84 ?
	int	m1b88 = 0;// +0x1b88 ?
	int	m1b8c = 0;// +0x1b8c ?
	int	m1b90 = 0;// +0x1b90 ?
	int	m1b94 = 0;// +0x1b94 ?
	int	m1b98 = 0;// +0x1b98 ?
	int	m1b9c = 0;// +0x1b9c ?
	int	m1ba0 = 0;// +0x1ba0 ?
	int	m1ba4 = 0;// +0x1ba4 ?
	int	m1ba8 = 0;// +0x1ba8 ?
	int	m1bac = 0;// +0x1bac ?
	int	m1bb0 = 0;// +0x1bb0 ?
	int	m1bb4 = 0;// +0x1bb4 ?
	int	m1bb8 = 0;// +0x1bb8 ?
	int	m1bbc = 0;// +0x1bbc ?
	int	m1bc0 = 0;// +0x1bc0 ?
	int	m1bc4 = 0;// +0x1bc4 ?
	int	m1bc8 = 0;// +0x1bc8 ?
	int	m1bcc = 0;// +0x1bcc ?
	int	m1bd0 = 0;// +0x1bd0 ?
	int	m1bd4 = 0;// +0x1bd4 ?
	float	m1bd8 = 0.0f;// +0x1bd8
	float	m1bdc = 0.0f;// +0x1bdc
	float	m1be0 = 0.0f;// +0x1be0
	float	m1be4 = 0.0f;// +0x1be4
	float	m1be8 = 0.0f;// +0x1be8
	float	m1bec = 0.0f;// +0x1bec
	float	m1bf0 = 0.0f;// +0x1bf0 how much the bonus chance changes by each level
	float	m1bf4 = 0.0f;// +0x1bf4 how much the penalty chance changes by each level
	float	m1bf8 = 0.0f;// +0x1bf8
	float	m1bfc = 0.0f;// +0x1bfc
	float	m1c00 = 0.0f;// +0x1c00
	int	mDropTicks = 0;// +0x1c04 updates a mutated brick waits before it drops
	bool	m1c08 = false;// +0x1c08 profile setting 0x2a + mode is 1
	bool	m1c09 = false;// +0x1c09
	// END FIELDS

	Board(AstroPopApp* theApp, EventBus* theBus, int theGameType, unsigned int theFlags, int theLevel, bool theFlag,
		  bool theNewGame, bool theDemo);                               // 0x480020
	virtual ~Board();                                                   // 0x47a910

	virtual void	OnFiring(int theEvent) override;                    // 0x4328d0: the hook of the Board's own events

	virtual void	Update() override;                              // 0x479480 slot 17

	// ---- the start (BoardCtor.cpp, BoardSetup.cpp)
	void			SetupHints();                                   // 0x475f50
	void			MarkHintsSeen(int theLevel);                    // 0x454410
	void			SetupClassicRules();                            // 0x44f670
	void			SetupLevel1Rules();                             // 0x47f3d0
	void			AdvanceLevel(bool theSave);                     // 0x441980
	void			LoadDemo();                                     // 0x470130
	void			BuildStoryText();                               // 0x47a940 (BoardStory.cpp)

	// ---- the small helpers (BoardCore.cpp)
	void			UpdateAnimCounters();                           // 0x433500
	void			SetOverlayHidden(bool theHide);                 // 0x433c30
	void			RecordStep(bool theForce);                      // 0x434570
	int				PickSpawnDelay();                               // 0x432a40
	void			FadeBackdrop();                                 // 0x436b80
	void			UpdateRamp();                                   // 0x437a50
	int				FindBottomRow(int theColumn, bool theIsX);      // 0x437590
	bool			NoRoomLeft();                                   // 0x438b60
	int				CountMovingBricks(bool theStrict);              // 0x439b70
	static bool		HasCell(int theCol, int theRow, const std::vector<CellRef>& theCells);       // 0x438d60
	static bool		HasCell3(int theCol, int theRow, const std::vector<CellRef3>& theCells);     // 0x438db0
	void			StepScrollLayer(ScrollLayer* theLayer);         // 0x4333c0
	void			UpdateScenery();                                // 0x43a1f0
	void			UpdateStageBackground();                        // 0x43a680
	void			StoreProgress();                                // 0x432f40
	void			UpdateKlaxon();                                 // 0x43b9c0
	bool			IsSettled(bool theAssumeIdle);                  // 0x43d2f0
	void			DeleteDoomedBricks();                           // 0x43d8b0
	Brick*			CreateBrick(int theType);                       // 0x4332b0
	Brick*			CreateMutator();                                // 0x433320
	Brick*			CreateFuelCell(int theColumn, int theRow);      // 0x433340
	SpecialBrick*	CreateSpecialBrick(int theType);                // 0x4393d0
	Brick*			CreatePowerBrick(int theType);                  // 0x439480

	// ---- the field (BoardField.cpp)
	static bool		TypesMatch(Brick* theBrick, int theType);       // 0x433030
	bool			FirstPieceIsType(int theType);                  // 0x436b30
	bool			IsTargeted(Brick* theBrick);                    // 0x439510
	void			ClearField();                                   // 0x44e010
	void			BuildField();                                   // 0x44e350

	// ---- the state (BoardState.cpp)
	void			ResetState();                                   // 0x4346c0
	void			UpdateEffects();                                // 0x439c20
	void			SetNextLevelScore();                            // 0x4344d0
	void			AddScore(int thePoints, bool theForce);         // 0x433b90
	void			SaveGameState();                                // 0x43a9a0
	void			ResetGame();                                    // 0x450250

	// ---- not translated yet (BoardStubs.cpp): each logs once and does nothing; named by address until they are read
	void			UpdateStates();                                 // 0x475040
	void			UpdateAlertFlash();                            // 0x433920
	void			UpdateStartBanner();                           // 0x433610
	void			PayTimeBonus();                                // 0x452a80
	void			UpdateFreeze();                                // 0x474de0
	void			UpdateSpecialMode();                                    // 0x46f010 (4339 bytes)
	void			UpdateIntroAndTally();                                    // 0x466e80 (3168 bytes)
	void			LoadLevel(bool theFlag);                       // 0x4663f0 (2647 bytes)
	void			LevelComplete();                                // 0x465290
	void			PayComboBonus();                                // 0x452d30
	void			StartLevelUp();                                 // 0x45dd70
	void			SetupUpgradeAnims();                           // 0x434200
	void			ApplyWeaponLevels();                            // 0x432b50
	void			SetupWarpLayers();                              // 0x43b1b0
	static std::string	FormatNumber(int theNumber);            // 0x43e220 with a comma after each three digits
	bool			UpdateWarp();                                   // 0x46a810 (3545 bytes): the warp between two stages and the level-up panel: a long state machine; true ends the update
	void			StartBonusRound(int theStage);                  // 0x465840: the field of a bonus round (-1 picks a stage)
	int				UpgradeScreenKind(int theLevel);                // 0x433130: the upgrade screen kind of a level (0, 1, 2 or 4)
	int				GetUpgradeSetting();                            // 0x4330a0: the profile setting that this level's upgrade counts (-1 with no table)
	std::string		GetUpgradeText();                               // 0x441850: the words that name this level's upgrade
	void			BeginClearWait();                               // 0x4335e0: the field waits to be cleared: the wait counter is set to 1000 and every brick holds its mutation (event 0x28)
	bool			OpenStoryScreen();                              // 0x468e70: the story screen of the level (true when one opens)
	void			LoadStoryPages();                               // 0x45ef90: the pictures of the stories of the level
	void			CloseStory();                                   // 0x469030: the story screen is done, the level starts
	void			UpdateStreaks();                                // 0x439650
	void			AddTopRow();                                    // 0x46e420
	bool			HasMarkOfType(int theType);                     // 0x438a90
	int				GetMarkCount(int theType);                      // 0x438af0
	Brick*			LowestFallingBrick94(int theColumn);            // 0x437780
	Brick*			LowestFallingBrick7c(int theColumn);            // 0x4376d0
	bool			ColumnReachesShip(int theX);                    // 0x438c60
	void			FillTopRow(bool theSpecials, bool theTutorial, bool theFromTop, bool thePowers, int theRow);   // 0x451720
	bool			RowAllows(int theKind, int theRow, int theRefRow, int theCol, bool theAnyDistance);   // 0x439040
	bool			CanPlaceKind(int theKind, int theRow, int theCol);  // 0x43d4c0
	void			CollectConnected(int theKind, int theCol, int theRow, unsigned int theLimit, bool theIncludeTop, const std::vector<CellRef>* theSkip, int thePrevCol, int thePrevRow);   // 0x450a30
	int				ColumnDepth(int theCol);                        // 0x439560
	int				CheckStackHeight();                             // 0x43d630
	void			CheckShipColumn();                              // 0x43d400
	Brick*			CreateKillAll(int theColor);                    // 0x439350
	void			ShowHint(int theKind, int theCol, int theRow, bool theFlag);   // 0x44fd60
	void			GameOver();                                     // 0x450f80
	void			UpdateBricks();                                 // 0x46e9b0
	void			RemoveBrick(Brick* theBrick, int theCol, int theRow, bool theCrack);   // 0x450d70
	bool			HitByNeighbour(int theCol, int theRow, int theKind);   // 0x4515f0
	void			DamageNeighbour(int theCol, int theRow, std::vector<CellRef>& theCracked);   // part of 0x4582d0
	void			DamageNeighbours(const std::vector<CellRef>& theKilled);   // 0x4582d0
	void			BuildBanner(const std::string& theText, int theTime, Font* theFont);   // 0x433df0
	void			ClearBonus();                                   // 0x4545b0
	void			StartClearCount();                              // 0x45a750
	void			DropUnsupported(std::vector<CellRef>& theCells);   // 0x46cc20
	bool			IsInFallingGroup94(Brick* theBrick);            // 0x438a30
	static bool		IsInGroups(Brick* theBrick, const std::vector<std::list<Brick*> >& theGroups);   // 0x4389c0
	bool			AnyMoveLeft(bool* theFlag);                     // 0x459bc0 (BoardMoves.cpp)
	// port: for the test commands (BoardBot.cpp)
	std::string		DescribeField();                                // the field as a JSON array of 19 strings of 10 characters
	int				BotBusyReason();                                // 0, or which of BotBusy's conditions holds (test-only)
	bool			BotBusy();                                      // the game takes no input now (bricks fly, a screen or a box is up)
	std::string		BotMove();                                      // the move the bot makes next as JSON, or "null" (BoardBot.cpp)
	void			UpdateGameOver();                               // 0x45e680
	void			CreateEndButtons();                             // 0x4593b0
	int				WeaponLevelSum();                               // 0x432e90
	void			SaveFinalScore();                               // 0x43e050

	// ---- the events of the bricks (BoardEvents.cpp, BoardArrive.cpp; the Board's handlers of the bus)
	virtual void	OnEvent(int theEvent, EventListener* theSource, int theArg) override;   // the table of the Board's listener part
	void			OnBrickArrived(Brick* theBrick);                // 0x473b20 event 5: a rising brick (or a group) reaches its place
	void			OnBrickGone(Brick* theBrick);                   // 0x46d1e0 event 6: a brick's death is over
	void			OnBrickLanded(Brick* theBrick);                 // 0x45cae0 event 7: a brick has fallen to the floor
	void			OnBrickFaded(Brick* theBrick);                  // 0x450f10 event 8
	void			OnBrickBounced(Brick* theBrick);                // 0x458770 event 0xb: the bounce of a brick is over
	void			OnBrickFinished(Brick* theBrick);               // 0x44bac0 event 0xc: the brick is to be deleted
	void			OnColumnNuke(Brick* theBrick);                  // 0x453250 event 0xd
	void			OnPowerBrickDone(Brick* theBrick);              // 0x45e920 event 0xe
	void			OnMutatorSpread(Brick* theBrick);               // 0x45d910 event 0xf
	void			OnHyperBrick(Brick* theBrick);                  // 0x452e70 event 0x12
	void			OnBoosterEnd(Brick* theBrick);                  // 0x44bba0 event 0x13
	void			OnBrickPopped(Brick* theBrick);                 // 0x45d7a0 event 0x1a
	void			OnBrickSettled(Brick* theBrick);                // 0x438e10 event 0x1b
	void			OnFreezeBomb(Brick* theBrick);                  // 0x43b910 event 0x1c
	void			OnKillAllDone(Brick* theBrick, int theType);    // 0x451380 event 0x1f
	void			OnBlast(Brick* theBrick, int theArg);           // 0x45ce40 event 0x26
	void			OnColourBrickDie(Brick* theBrick, int theType); // 0x45d6f0 event 0x27
	void			OnPieceBrickHit(Brick* theBrick);               // 0x451430 event 0x29
	void			AdvanceChain();                                 // 0x434320 one more set in a row
	void			CheckSparseField();                             // 0x437830
	void			SortFallingGroups();                            // 0x46cfd0 the groups in mFallingGroups by the height of their first brick
	void			UpdateSupercharge(bool theCheck, int theColour); // 0x45ac80 the colours that are charged
	void			AnnounceText(const std::string& theText);       // 0x454730 a text in the middle of the field that floats up
	std::vector<CellRef>	BlastNeighbours(int theCol, int theRow, Brick* theBrick);   // 0x4588d0
	void			RemoveCells(const std::vector<CellRef>& theCells, Image* theImage);   // (inside 0x45ce40) take the bricks of the cells out together
	void			HideCellHint(Brick* theBrick);                  // (inlined in the handlers) the cell hint goes when its brick does
	static int		CellColumn(float theX) { return ((int) theX - 0xa6) / 0x2c; }
	static int		CellRow(float theY) { int aY = (int) theY; return aY < 0x22 ? 0 : (aY - 0x22) / 0x19 + 1; }

	// ---- the input (BoardInput.cpp)
	virtual void	AddedToManager(WidgetManager* theManager) override;         // 0x459820 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;     // 0x43a5d0 slot 2
	virtual void	KeyChar(SexyChar theChar) override;                         // 0x45c850 slot 20
	virtual void	KeyDown(KeyCode theKey) override;                           // 0x473870 slot 21
	virtual void	MouseMove(int x, int y) override;                           // 0x461010 slot 25
	virtual void	MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x458170 slot 26 (the middle button)
	virtual void	MouseDown(int x, int y, int theClickCount) override;        // 0x478cd0 slot 27
	virtual void	MouseUp(int x, int y, int theClickCount) override;          // 0x432dc0 slot 29
	virtual void	MouseDrag(int x, int y) override;                           // 0x432b20 slot 31
	using Widget::MouseDown;
	using Widget::MouseUp;

	void			PickUp();                                       // 0x472360 the down key, the left button: take bricks from the stack, or use a tool
	void			FirePiece();                                    // 0x4732d0 the up key, the right button: shoot the bricks in hand up the column
	bool			CatchFrom7c(bool* theOk);                       // 0x44d8b0 catch the bricks of the fired groups that fly past the pointer
	bool			CatchFrom94(bool* theOk);                       // 0x44dc30 the same for the groups that fall in the column
	CellRef			LowestEmptiedCell(int theCol);                  // 0x437640 the lowest cell of the column emptied this update, (-1, -1) for none
	bool			MatchesHand(Brick* theBrick);                   // 0x4361c0 does the brick match the brick in hand
	void			MakeHand(int theType, float theX);              // (inlined three times) the brick in hand is born
	void			StartByHand();                                  // (inlined three times) the player's first move: the Return key, the start button, the middle button
	void			PickUpDone(bool theFail);                       // (the end of 0x472360) the sound of the pick up, and the checks that follow it
	void			PopPsychic(bool theHandEmpty);                  // (inside 0x472360, twice: with the hand empty, and with a brick in it) the psychic mode takes the lit bricks out

	// ---- the drawing (BoardDraw.cpp and the files named Board Draw...)
	virtual void Draw(Graphics* g) override;                              // 0x461e50 slot 16
	void			DrawBackdrop(Graphics* g);                      // 0x437bd0 the scenery, planets, streaks and stars
	void			DrawBeam(Graphics* g);                          // 0x45ea50 the beam under the pointer
	void			DrawBricks(Graphics* g, bool theGlow);          // 0x438e80
	void			DrawBonusIntro(Graphics* g);                    // 0x443fd0
	void			DrawStageBg(Graphics* g);                       // 0x43b5c0
	void			AnimateBanner();                                // 0x433fa0
	void			DrawShip(Graphics* g);                          // 0x438210
	void			DrawHud(Graphics* g);                           // 0x440f70
	void			DrawFrame(Graphics* g);                         // 0x444520
	void			DrawBonusStats(Graphics* g);                    // 0x443480
	void			DrawWarpScreen(Graphics* g);                    // 0x442c70
	// the parts of Draw (BoardDrawLayers.cpp, BoardDrawPage.cpp), in its order
	void			DrawDangerZone(Graphics* g);
	void			DrawModeTexts(Graphics* g);
	void			DrawGoalBanners(Graphics* g, int theCount);
	void			DrawCrosshair(Graphics* g);
	void			DrawExplosions(Graphics* g);
	void			DrawEndSparks(Graphics* g);
	void			DrawEndPage(Graphics* g);
	void			DrawSnow(Graphics* g);
	void			DrawUpgradeArm(Graphics* g);
	void			DrawSurvivalIntro(Graphics* g);
	void			DrawLogo(Graphics* g);
	void			DrawSupaBrackets(Graphics* g);
	void			DrawFreezeFx(Graphics* g);
	void			DrawSirens(Graphics* g);
	void			DrawSurvivalBonus(Graphics* g);
	void			DrawTestTexts(Graphics* g);

	// ---- the saved game (BoardRestore.cpp)
	void			RestoreGame(int theScore, int theSetting54, int theSetting55, float theSetting52, float theSetting50, int theSavedLevel);   // 0x467af0

	// ---- pause and play (BoardPlay.cpp)
	void			Pause(bool theStart, bool theFlag);             // 0x459040 a dialog opens (true) or closes (false)
	bool			HasWarpState() const;                           // 0x408ad0 a warp or a level change is going on (the engine loop is off then)
	void			RecolorThemeButtons();                          // 0x437910
	void			ShowThemeButtons();                             // 0x45ab90
	void			BeginPlay();                                    // 0x457930 the player's first move: the mode's play starts
	void			DismissHint();                                  // 0x45a930 the OK of a hint box
	void			CloseOverlay();                                 // 0x4347c0 the button 0xfa7
	void			RedrawAfterDialog();                            // 0x43a890 the field is drawn again after a menu or box
	void			TakeFocus();                                    // 0x4342f0 the Board takes the keyboard back
	static void		SortCells(std::vector<CellRef>& theCells);     // 0x450c00

	// ---- the update (BoardUpdate.cpp)
	void			RevealBricks();                                 // part of 0x479480: the bricks of a new field appear one by one
	void			SweepSprockets();                               // part of 0x479480
	void			SweepTurbots();                                 // part of 0x479480
	bool			SpawnControl();                                 // part of 0x479480: the pieces that drop and the end of the game (true: the update ends)
	void			UpdateExplosion();                              // part of 0x479480
	void			UpdateSparks();                                 // part of 0x479480

	// ---- the cursor's targets (BoardTarget.cpp)
	void			FlashOnce(Brick* theBrick);
	void			HighlightTargets();                             // 0x452750
	void			HoverCell(int theCol, unsigned int theRow, bool theForce);   // 0x453630
};

}

#endif
