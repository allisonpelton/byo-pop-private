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
	int		mRowInterval; // +0x19cc the time between top rows (mRowInterval) from this row on
	int		mMaxMutators; // +0x1ac4 the most mutators in the field at once (mMaxMutators)
	int		mDropTicks; // +0x1c04 the updates a mutating brick waits before it drops (mDropTicks)
	float	mPenaltyChance; // +0x1bec the chance of a penalty brick in a new row (mPenaltyChance)
	float	mBonusChance; // +0x1be8 the chance of a power brick in a new row (mBonusChance)
};

// A layer of the scenery that scrolls down the screen (0x4333c0 steps it). The Board keeps one at +0x77c; the planets
// (0x3c bytes each, a ScrollLayer and one more number) are a vector at +0x160.
struct ScrollLayer
{
	Image*	mImage;             // +0x00
	float	mCur[4];            // +0x04 the picture now on the screen (mCur[1] counts down to its end)
	float	mNext[4];           // +0x14 the picture that follows (mNext[1] the place to start from)
	float	mCurY; // +0x24 the y at which the current picture (mCur) is drawn, 0 when it is at the top; the layer steps it down until it passes the screen's height
	float	mNextY; // +0x28 the y at which the picture that follows (mNext) is drawn; -1 while there is none
	int		mPeriod;            // +0x2c the layer steps every mPeriod updates
	int		mPictureId; // +0x30 the id of the picture the layer shows (0x18c at the start; the warp sets the scenery's)
	float	mSpeed;             // +0x34

	ScrollLayer()               // 0x436100
	{
		mImage = NULL;
		for (int i = 0; i < 4; i++)
		{
			mCur[i] = 0;
			mNext[i] = 0;
		}
		mCurY = -1234.0f;
		mNextY = -1234.0f;
		mSpeed = -1234.0f;
		mPeriod = 500000000;
		mPictureId = 500000000;
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
	Image*			mImage;             // +0x2c the picture of the kind (the application's 0x674, 0x678, 0x67c)

	StreakRec()                         // 0x432750 (the original leaves the speeds to the constructor's loop)
		: mColor(0, 0, 0), mX(-1.0f), mY(-1.0f), mSpeed(0), mMinSpeed(0), mMargin(1), mKind(0), mImage(NULL) { }
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
	float			mX;                 // +0x50
	float			mY;                 // +0x54 counts down the screen
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
	int				mTitleHeight; // +0x120 the height of the title as written by SetupHints
	int				mTextHeight; // +0x124 the height of the text as written by SetupHints
	StageBg*		mNext;              // +0x128 the next panel of the screen

	StageBg()                           // 0x43fab0
		: mPic(NULL), mOverPic(NULL), mX(0), mY(0), mX2(0), mY2(0), mFadedOut(false), mTitleWidth(0x96), mTitleX(0x78), mTextX(400),
		  mPicAlpha(0xff), mOverPicAlpha(0), mCrossfadeTimer(0), mPicAlphaStep(0), mOverPicAlphaStep(0), mPanelAlpha(0xff), mPanelAlphaStep(0), mTitleHeight(0), mTextHeight(0), mNext(NULL) { }
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
	bool			mEmergencyPause;            // +0x78 the game is paused because no room is left (the user's setting 0x2e is 1): the overlay draws the emergency panel and a click in the pause rectangle goes on
	Board*			mBoard;             // +0x7c
	bool			mShownWhileRunning;            // +0x80 set when the overlay is shown while no pause is on; the end of a pause hides the overlay unless this is set

	BoardOverlay();                                     // 0x4327c0
	void			SetFill(int theRed, int theGreen, int theBlue, int theAlpha);   // 0x432990
	void			SetEmergencyPause(bool theFlag) { mEmergencyPause = theFlag; }                  // 0x4329c0

	// (the original's widget class also has two overrides that the port leaves to Widget: a mouse-move one and an
	// is-point-visible one; the widget is not mouse-visible, so neither is ever called)
	void			Update() override;                                              // 0x432930
	void			Draw(Graphics* g) override;                                     // 0x43fc10 (BoardOverlay.cpp)

private:
	void			DrawHint(Graphics* g);
	void			DrawWeaponHint(Graphics* g);
	void			DrawEmergency(Graphics* g);
};

extern bool gStateSaved;    // the quiet state of the game was saved (SaveGameState sets it); the bricks' moves clear it, and the update saves again only when it is clear

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
	bool	mLeftArmImagesOwned = false;// +0x830 the flipped copies of the arm and head (the left arm's pictures) belong to the Board and the destructor deletes them; set by SetupUpgradeAnims
	MemoryImage*	mLeftArmImage = NULL;// +0x834 flipped copy of the upgrade arm picture (made when the Board is made): the arm that slides in from the left
	MemoryImage*	mLeftHeadImage = NULL;// +0x838 flipped copy of the upgrade head picture: the head on the left arm, drawn with the cel of mLeftHeadAnim
	FrameAnim	mLeftHeadAnim;// +0x83c animation of the left arm's head: cels 0 to 5, backwards, 6 updates a cel; runs once the arm has stopped (Fn46a810)
	int	mRightArmX = 0;// +0x87c x of the right arm of the upgrade screen: starts at 660, past the right edge, slides in to its stop and out again (Fn46a810)
	int	mRightArmY = 0;// +0x880 y of the right arm and its head on the upgrade screen: 416
	int	mRightArmStopX = 0;// +0x884 x where the right arm stops sliding in: 463 (SetupUpgradeAnims)
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
	bool	mTopRowsAllowed = false;// +0x8e3 gate of SpawnControl: ResetGame sets it and nothing clears it; a new top row comes only while it is set (and no bounce, bonus, warp or end runs)
	bool	mNewRowBouncing = false;// +0x8e4 AddTopRow sets it when the new top row starts to bounce, OnBrickBounced clears it once the row is still; meanwhile no new row, start or save
	bool	mPickUpButtonDown = false;// +0x8e5 the pick-up button (down key or left mouse button) is held; PickUp refuses without it; MouseUp and clicks that are no pick-up clear it
	bool	mBackdropTintChanging = false;// +0x8e6 the backdrop's tint is still moving to the colour of the first held brick: set when the hand changes (PickUpDone, nukes), cleared by FadeBackdrop on arrival
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
	bool	mFieldCleared = false;// +0x8fb nothing is left in rows 1 to 19 once everything has settled (ClearBonus is paid); FillTopRow and ResetGame clear it; the next row waits for it in a bonus
	bool	mPlayStarted = false;// +0x8fc the game has really begun: the first pick-up, shot, set or weapon start sets it (event 0x2a fires once); rows, countdowns and the freeze wait for it
	bool	mSlowMotion = false;// +0x8ff when set only every eighth Update does the game's work (the others only record the demo step); nothing ever sets it
	bool	mLevelCompleteBlocked = false;// +0x900 LevelComplete returns at once while it is set; the constructor clears it and nothing sets it
	bool	mScreenWiping = false;// +0x901 the end-of-game screen wipe runs: Update waits for the wipe to end and then closes the board, Draw only draws the wipe; RedrawAfterDialog sets it
	bool	mGameOverAnimDone = false;// +0x905 the end-of-game blasts and fades are through (UpdateGameOver sets it, then makes the end buttons); UpdateGameOver returns at once while it is set
	bool	mMeterPaused = false;// +0x906 while set the weapon meter does not run down in UpdateSpecialMode; the constructor clears it and nothing sets it
	bool	mClearWaitStarted = false;// +0x907 mode 2: the wait for the field to be cleared has begun (BeginClearWait); set by ResetGame, cleared when the weapon starts so that the next quiet field begins it
	bool	mStackWarningActive = false;// +0x909 the warning of a too high stack at the ship's column is on (CheckShipColumn sets it, 400 updates); SpawnControl holds the next top row back meanwhile
	bool	mTutorialStageForced = false;// +0x90a the first stage's tutorial hints apply whatever the stage number is; the constructor clears it and nothing sets it
	bool	mClickRepeating = false;// +0x90b set while Update repeats the held left button's click (classic game, weapon on); the pick-up sounds stay silent then; ResetState clears it
	bool	mBoosterDue = false;// +0x90e a booster brick is to go into the next new top row: set by AdvanceChain (long chain) and PayTimeBonus, cleared by FillTopRow when it picks the column
	bool	mClicksBlocked = false;// +0x90f while set MouseDown ignores the buttons and MouseUp keeps the pick-up flag: set when a bonus round starts, a box closes or the screen is redrawn after a box
	bool	mBonusRoundOn = false;// +0x910 a bonus round is on (from LevelComplete's bonus branch or a restored bonus game until its tally ends): no meter, no stage counting, bricks from a bonus file
	bool	mStartBannerDoneInBonus = false;// +0x911 set by UpdateStartBanner when the start banner has run through while a bonus round is on; StartBonusRound and ResetGame clear it; it holds back the round's no-move end
	bool	mBricksRevealing = false;// +0x912 the bonus round's bricks are still being shown one by one (StartBonusRound sets it, RevealBricks clears it); the first click and the end checks wait
	bool	mBonusRoundStarted = false;// +0x913 the player's first click has started the bonus round after its bricks appeared (MouseDown, event 0x28 fired); StartBonusRound clears it; the time runs from it
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
	float	mStackDangerPeak = 0.0f;// +0x934 the highest mStackDanger seen lately: CheckStackHeight raises it at once, Update lowers it by 0.2 per update toward mStackDanger; nothing reads it
	float	mWarpShipY = 0.0f;// +0x938 the ship's y in the warp: -312 above the screen when the flight starts, speeds up to 423 (UpdateWarp); it also sets the scenery's scroll speed
	float	mScoreMultiplier = 0.0f;// +0x940 multiplies the points of the bricks gone and the bonuses, 1 to 6: 1 + level / 8 at set-up, +0.125 or +0.5625 per stage skipped at a gate, +3 per minute in survival
	float	mBannerWavePhase = 0.0f;// +0x944 the phase of the wave that moves the banner's columns (AnimateBanner): 0 when the banner is made, +0.07 per update while its time runs
	unsigned char	mDemoRecordAction = 0;// +0x948 the action of this update for the demo recording, in the codes of mDemoSteps: 0 none, 1 fire, 2 pick up, 0xc minus the column for a move; cleared every update
	unsigned char	mDemoRecordLastAction = 0;// +0x949 the action RecordStep wrote last; a different action first closes the wait that came before it
	int	mDemoRecordArg = 0;// +0x94c the recorded action's argument: the updates waited while no action comes (counts up each update), or the brick kind * 256 + count for a pick up or a fire
	int	mDemoRecordLastArg = 0;// +0x950 mDemoRecordArg as of the last RecordStep: the length of the wait that RecordStep writes (in runs of 255) when the next action comes
	int	mDemoStepIndex = 0;// +0x954 the step of mDemoSteps that the demo plays now; past the last step the demo starts a new game
	int	mDemoAddedRowIndex = 0;// +0x958 the next row of mDemoAddedRows that AddTopRow adds while the demo plays
	int	mDemoWaitLeft = 0;// +0x95c the updates left of the demo's current wait step (set from the step's argument, counts down, the demo goes on at 0)
	int	mClearBonusWorth = 0;// +0x960 what clearing a stage is worth, 10000 to 500000 by the level (SetNextLevelScore); the clear count takes 5% of it for each brick left
	int	mHighScoreSoundDelay = 0;// +0x964 updates (200, set by ResetGame) that the game over waits before the new-high-score sound; counts down while mHighScoreSoundDone is false
	int	mFieldPointerX = 0;// +0x968 the pointer's last x while it stood in the field (166 to 605), seen by MouseMove; the crosshair of the psychic weapon (mode 3) is drawn there
	int	mFieldPointerY = 0;// +0x96c the pointer's last y that went with mFieldPointerX; the crosshair's row comes from it
	int	mHighestStack = 0;// +0x970 the height in rows of the highest column stack at the last CheckStackHeight; 14 and over starts the siren, under 6 lets a sparse field start a bonus
	int	mButtonsHeld = 0;// +0x974 the buttons held as the original counts them: a left press sets 1, a right press 0, a release takes 1 (left) or 2 (right), never below 0; nothing reads it
	int	mWhiteFadeAlpha = 0;// +0x978 the alpha (0 to 255) of the white overlay at the end of the demo: rises 6 per update to white, then falls 6 per update (mFadingFromWhite)
	int	mStackWarningTimer = 0;// +0x97c the updates (400) that the ship column's stack warning (mStackWarningActive) holds the next row back; 0 when the column has room
	int	mScreenWipeKind = 0;// +0x980 the kind (0 to 3, drawn at random by the constructor) of the wipe that takes the field away at the end of a game; -1 until it is drawn
	ScriptRows	mScriptRows;// +0x984 the rows of a scripted game (the demo, the tutorial and a level file): ten brick codes a row, 100 rows; LoadLevel clears them
	int	mPauseCount = 0;// +0x1924 the number of pauses in force (Pause adds one, the end of a pause takes one); the game runs only at 0, and the animations draw from a frozen tick above 0
	int	mPauseDarkening = 0;// +0x1928 how far the pause has darkened the field: +20 per update while paused up to the window height, -40 per update after; the shade's alpha is half of it, at most 230
	int	mPausedUpdates = 0;// +0x192c counts the updates that run while the game is paused; ResetGame clears it; nothing reads it
	int	mScriptRowIndex = 0;// +0x1930 the row of mScriptRows that FillTopRow plays next (-1 none); it moves on after each row and becomes -1 when the next row is empty
	int	mExtraRowsDue = 0;// +0x1934 extra top rows owed (set by CheckSparseField for a sparse field, +1 by a periodic timer in Tick): SpawnControl brings one after a 100-update wait and takes it off
	int	mStageNumber = 0;// +0x193c the stage, from 1: minutes played + 1 in survival (UpdateRamp), raised at a warp gate one stage at a time (AdvanceLevel); picks hint panels, rules and speeds
	int	mLevelNumber = 0;// +0x1940 the level shown as "Level N" and saved as the mode's level reached: starts as the constructor's level, becomes stage / 2 + 1 at a warp gate; every fourth has an upgrade
	int	mEndFlashAlpha = 0;// +0x1948 the alpha of the white flash at the end of a game: 255 when the end page starts, falls 2 per update (the overlay's fill); the panels wait while it is above 240
	int	mShipTilt = 0;// +0x194c which way the ship leans: 0 after a move to the left, 1 to the right, 2 level (set when the tilt frames end); picks the ship's picture row
	int	mShipFlameFrame = 0;// +0x1950 the frame (0 to 7) of the ship's flame; advances every 10 updates (UpdateAnimCounters)
	int	mShipFrame = 0;// +0x1954 the frame of the ship's picture: 2 when level, 0 to 3 while it tilts (a step every 10 updates); the warp-out and the arrival run it up and down too
	int	mLastPointerX = 0;// +0x1958 the pointer's x at its last mouse event (MouseMove, MouseDown, MouseUp); the demo's clicks and the hover of the field read it
	int	mLastPointerY = 0;// +0x195c the pointer's y at its last mouse event; the field's row is (y - 34) / 25 + 1
	int	mStartTextAlpha = 0;// +0x1960 the alpha of the text bar at the start of play (bricks to pass, new starting point, survival intro): 0 before play (drawn full), 255 at the start, -4 per update, -1 gone
	int	mChainPoints = 0;// +0x196c the points of the bricks gone in the current chain of sets (OnBrickGone adds them); PayComboBonus pays them times mChainCount and clears it
	int	mScoreShown = 0;// +0x1970 the score on the display: catches up with mScore by a step that grows with the gap (Update), and jumps to it when a game is restored
	int	mScore = 0;// +0x1974 the score (profile setting 0x51); AddScore adds to it and keeps the mode's best; mScoreShown counts up to it on the display
	int	mScoreAtStageStart = 0;// +0x1978 mScore when the game began or when a classic game came out of a warp; profile setting 0x61; a restored score is this plus mPlayPoints and mBonusRoundPoints
	int	mChainCount = 0;// +0x1980 the sets made in a row (AdvanceChain adds one); a chain of 2 or more pays a bonus (PayComboBonus), 3 or more makes a fuel cell due; cleared when the chain ends
	int	mLastChainCount = 0;// +0x1984 the chain count noted when no group was moving (OnBrickArrived); PayComboBonus and ResetGame clear it; nothing reads it
	int	mShipX = 0;// +0x198c the ship's x in pixels, the left of its column (166 + 44 * column); shots, pick ups and the beam read it; 0x182 after a level file is loaded
	int	mHeldBricksNotLanded = 0;// +0x1990 the picked-up or caught bricks that have not yet landed in the hand (+1 when taken, -1 in OnBrickLanded); firing, pick up and the pause wait for 0
	int	mTakenBrickCount = 0;// +0x1994 the bricks taken from the field that have not yet gone: +1 as each goes into mTakenBricks, -1 in OnBrickGone; 0 means no removal is running
	int	mWarpFrameInterval = 0;// +0x1998 the updates between two steps of the ship's warp-out frame (mShipFrame) while the screen fades to white (6 after a level is complete, 8 after a level is loaded)
	int	mWarpShakeX = 0;// +0x199c the ship's sideways shake in pixels while the warp charges, flies and arrives (a random -5 to 5 or -2 to 1 each update); 0 otherwise
	int	mWarpPictureLift = 0;// +0x19a0 the height that the warp pictures at the ship are lifted by; StartLevelUp sets it to 0 and nothing sets another value
	int	mWarpChargeAlpha = 0;// +0x19a4 the alpha (0 to 255) of the jet and flame flash on the ship while the warp charges: +2 per update, the warp starts at 255 and its sound at 0x96
	int	mWarpGlowAlpha = 0;// +0x19a8 the alpha (0 to 255) of the glow drawn over the warp picture of the ship; it rises by mWarpGlowStep per update while the field flies away
	int	mWarpGlowStep = 0;// +0x19ac how much mWarpGlowAlpha rises each update (12, set by StartLevelUp)
	int	mEffectsUpdateCount = 0;// +0x19b0 counts the calls of UpdateEffects since the level was loaded; a negative value hides the arriving ship's picture
	int	mRampTime = 0;// +0x19b4 the time played in updates (100 a second) that drives the speed table and, in survival, the stage (6000 a stage); not counted in bonus rounds; profile setting 0x60
	int	mPlayTime = 0;// +0x19b8 the updates of play counted every update that play runs, bonus rounds included; the time on the end page; profile setting 0x54
	int	mRowsLeftToPass = 0;// +0x19c0 copy of mRowsToPass made whenever a level's rules are set; AddTopRow runs while it is not 0 (the port never counts it down)
	int	mRowsToPass = 0;// +0x19c4 the rows a stage takes to pass (NumRowsToPass: 25 by default, grows by mRowsToPassChange each stage, at most 70)
	int	mRowsToPassChange = 0;// +0x19c8 how much mRowsToPass grows each stage (NumRowsToPassChangeBy of the level file, else 0)
	int	mRowInterval = 0;// +0x19cc the time in updates between top rows (DropTime * 100 of the level file); UpdateRamp sets it from the speed table
	int	mRowIntervalReduction = 0;// +0x19d0 how much shorter the row interval has become: +1 every mRowReductionPeriod updates while play runs, never more than mRowInterval - 200
	int	mRowReductionPeriod = 0;// +0x19d4 the updates between two steps of mRowIntervalReduction (8, one less on each even stage from the mode's start stage on, at least 5)
	int	mRowTimer = 0;// +0x19d8 the updates since the last top row finished bouncing (0 in OnBrickBounced, -1 after a reset); the next row comes when it reaches the row time
	int	mRemovalPoints = 0;// +0x19dc the points of the bricks of the current removal; OnBrickGone adds them and the group's floating text shows them where the first brick was
	int	mWarpGateAlpha = 0;// +0x19e0 alpha 0-255 of the warp screen's gates: 0 on reaching the gates, +2 an update up to 255 (UpdateWarp); DrawWarpScreen fades the gates in with it
	int	mEasyGateGlow = 0;// +0x19e4 glow alpha 0-255 of the easy gate: +8 an update while the pointer is over it, -8 otherwise (UpdateWarp); the pointer is never over it and nothing reads it
	int	mSlowGateGlow = 0;// +0x19e8 glow alpha 0-255 of the slow gate: +8 an update while the pointer is over it, -8 otherwise (UpdateWarp); DrawWarpScreen draws the gate again with it
	int	mFastGateGlow = 0;// +0x19ec glow alpha 0-255 of the fast gate, moved like mSlowGateGlow by the pointer; drawn only when the fast gate is offered
	int	mWarpLightningAlpha = 0;// +0x19f0 alpha of the lightning flash on the warp backdrop: 0xff when a flash starts, -3 an update to 0 (UpdateScenery); drawn in 3D while the warp runs
	int	mWarpLightningPeriod = 0;// +0x19f4 the updates between two lightning flashes of the warp (100-299, drawn again after each flash); a flash starts when mUpdateCnt is a multiple of it
	int	mWarpLightningY = 0;// +0x19f8 the y (50-399) at which the lightning picture is drawn, drawn again at each flash
	int	mFreezeUpdatesLeft = 0;// +0x19fc the updates the freeze effect has left (1000 from BeginClearWait, counts down once the game runs); the freeze ends and event 0x28 (0) fires at 0
	int	mSlowGateStage = 0;// +0x1a04 the stage the slow gate of the warp screen leads to (the current stage + 2); a click on the gate advances mStageNumber up to it
	int	mFastGateStage = 0;// +0x1a08 the stage the fast gate leads to (a jump of up to 8 stages ahead); -1 when no fast gate is offered; its level is this / 2 + 1
	int	mBestLevelReached = 0;// +0x1a0c the furthest level the player has reached with this pilot (profile record mMode + 8); the fast gate needs it above 3 and stages up to twice it minus 2
	int	mStageBricksCleared = 0;// +0x1a14 the bricks cleared toward the stage's quota mBricksToPass (OnBrickGone counts each; a booster adds 15%); saved in profile record 0x56; the brickometer fill
	int	mBricksToPass = 0;// +0x1a18 the stage's quota of bricks (NumBricksToPass of the level file: 200 for level 1, then plus mBricksToPassChange, at most mBricksToPassMax)
	int	mBricksToPassChange = 0;// +0x1a1c how much the quota mBricksToPass grows for each level (NumBricksToPassChangeBy of the level file, 20 for level 1)
	int	mBricksToPassMax = 0;// +0x1a20 the highest the quota mBricksToPass may grow (MaxBricksToPass of the global rules, 1000)
	int	mLongestChain = 0;// +0x1a24 the longest chain of the game outside bonus rounds (AdvanceChain); saving the game raises it to mBestChain; nothing else reads it
	int	mExtraColors = 0;// +0x1a28 the colours in play beyond the first four (0-2): the level file's AddNewColor adds one, survival adds one at ramp time 18000; bricks are made from 4 + this kinds
	int	mTimeBonusUpdates = 0;// +0x1a2c the updates since the first set of the stage while the time bonus runs (PayTimeBonus pays at 200..700); 0 when it stops
	int	mTimeBonusStepsPaid = 0;// +0x1a30 the steps of the time bonus paid so far (0-6, at 200, 300, 400, 500, 600 and 700 updates); sets the height of the bonus's floating text
	int	mPowerupActive = 0;// +0x1a34 1 while the pilot's power-up is running (the gauge full and the weapon in use), 0 while it charges; set in BeginPlay, cleared when the gauge is empty
	int	mMode = 0;// +0x1a38 the pilot of the game (0 laser, 1 atomic, 2 turbot, 3 psychic) and the base of its profile records: best score at mMode, stage at +4, level at +8
	int	mAtomicSymbolTimer = 0;// +0x1a40 updates since the eighth atomic explosion reached frame 7 (the atomic symbol fades in with twice it as alpha); past 149 it becomes -5 and the blast sounds; -1 none
	int	mAtomicFlashAlpha = 0;// +0x1a44 alpha of the white flash over the screen while the atomic blast grows (0 at its start, +1 an update, +10 later up to 255); -1 when no blast runs
	int	mTurbotWaveY = 0;// +0x1a48 the y of the turbot's wave picture: starts at the screen's height and falls 10 an update while the wave sweeps; -65536 when none
	int	mTurbotFlashAlpha = 0;// +0x1a4c the white flash of the turbot mode: counts up to 100 while it charges (it also sets the beams' rate), 255 when the wave starts, then -2 an update; -1 none
	int	mTimeBonusSets = 0;// +0x1a50 the chains begun since the time bonus clock started (AdvanceChain); the first starts the clock, and PayTimeBonus needs 3 to 8 of them for each step
	int	mSetSize = 0;// +0x1a54 the number of bricks of one kind together that make a set and are taken out (4); the bot and the move search use it too
	int	mTutorialPickTimer = 0;// +0x1a58 the tutorial's updates since the first brick was picked up; at 1000 the tutorial's hint dialog opens; restarted by the second pick-up
	int	mTutorialPickCount = 0;// +0x1a5c the bricks picked up so far while the tutorial is on (counted in the pick-up loop of the hand); the first one starts mTutorialPickTimer
	int	mGameType = 0;// +0x1a60 the kind of game this Board plays: 0 the demonstration, 1 the classic game, 2 survival (constructor argument); also the table of its Hall of Fame records
	// port(autosplit): three inert values that only the autosplit block's observer (LiveSplitObserver.cpp) reads; the game's own code never does
	unsigned int	mBoardSerial = 0;   // 1, 2, ... in the order the Boards are made (the demonstration's included): which game the observer is looking at
	bool			mRestoredGame = false;  // RestoreGame ran: the board continues a saved game
	int				mGameOverCause = -1;    // why GameOver was called: 0 a new row pushed a column to the ship, 1 the stack filled the field; -1 not from either (a test command)
	int	mSupportedBrickCount = 0;// +0x1a68 the bricks that rise on a support brick (event 0x10 from Brick::SetTargetY counts up, 0x11 from FireLanded down); the next row waits for 0
	int	mSlowGateX = 0;// +0x1a70 the x of the slow gate on the warp screen: 182 when two gates are shown, else centred; its click area is 120 wide
	int	mFastGateX = 0;// +0x1a74 the x of the fast gate on the warp screen (402); -1 when none is offered
	int	mSirenOffDelay = 0;// +0x1a7c the updates until the siren and its song stop after the stack has dropped below 13 rows (300 / (15 - height), CheckStackHeight); -1 none
	int	mScreenShakeX = 0;// +0x1a80 the x shift of the frame, the panels and the logo: random -2..2 while a strong hit shakes (UpdateStartBanner), -5..5 in turbot mode and at the game's end; else 0
	int	mScreenWipeSteps = 0;// +0x1a84 the steps the screen wipe runs each update: 2000 for the kind that ends at once, 4 for kind 3, else 6
	int	mCountedBrickType = 0;// +0x1a88 the brick type that counts toward the quota (OnBrickGone); -1 counts every brick, the only value ever set
	int	mWeaponPower = 0;// +0x1a8c the power level of the pilot's weapon (profile record mMode + 0x33): the reach of the pointer's marking, and the share of buried bricks the atomic weapon takes
	int	mWeaponDurationLevel = 0;// +0x1a90 the level of the weapon's duration for the laser (profile record 0x37) or the psychic (0x3a), -1 for the others; ApplyWeaponLevels picks how long it lasts from it
	int	mWeaponChargeLevel = 0;// +0x1a94 the level of the weapon's charging speed (profile record mMode + 0x3b): ApplyWeaponLevels turns it into the gauge's charge factor
	int	mGameOverDangerY = 0;// +0x1a98 the y of the game-over danger picture: mUpdateCnt + 20 when the siren starts, -12 an update (UpdateAlertFlash); -500000000 none; nothing draws it
	int	mGameOverDangerHold = 0;// +0x1a9c counts the 149 updates the danger picture waits once it passes y 21; -500000000 before, -250000000 after
	int	mTurbotPowerupAlpha = 0;// +0x1aa0 alpha 0-255 of the turbot's power-up drawn on the ship: 0 at BeginPlay, +2 an update
	int	mDangerPulseStep = 0;// +0x1aa4 the step of mDangerPulseAlpha each update; its size comes from the stack height (CheckStackHeight) and its sign flips at either end
	int	mDangerPulseAlpha = 0;// +0x1aa8 the pulsing alpha 0-255 of the danger zone's second layer while the stack is high (UpdateAlertFlash)
	int	mDangerZoneAlpha = 0;// +0x1aac alpha 0-255 of the glow at the bottom of the screen: +4 an update while the stack is 14 rows or more, -6 otherwise
	int	mExtraRowDelay = 0;// +0x1ab4 a countdown from 100 held between extra rows: set when extra rows become due, counts down while no power-up runs; an extra row waits until it is 0
	int	mPhase = 0;// +0x1ab8 the game's phase: index of the picture under the field (mPhasePictures) and key of PickSpawnDelay; 2-5 calm, 6-7 power-up, 8-9 siren, 10-13 chain flurry
	int	mFlurryTimer = 0;// +0x1abc the updates left of the chain flurry (chain length x 100 after a chain over 2, 400 at a bonus round, 700 at the level-up screen); at 0 the phase goes back
	int	mPhaseDelay = 0;// +0x1ac0 the updates until the phase steps on again (PickSpawnDelay of the current phase); the power-up picture fades with it
	int	mMaxMutators = 0;// +0x1ac4 the most mutators that may be in the field at once (MaxMutators of the level file, 6 by default; the speed ramp changes it)
	int	mRowsAtStart = 0;// +0x1ac8 the rows the field is filled with at the start of a level (NumRowsAtStart of the level file, 4 by default)
	int	mSupaTitleStep = 0;// +0x1acc how much of the SUPA banner's title is shown, 0-10 (the index of its width table); +1 every 4 updates while the banner opens
	int	mBonusBannerHue = 0;// +0x1ad0 the hue 0-255 of the running banner text while the power-up runs: +4 an update; 0 at BeginPlay
	int	mSupaBannerStep = 0;// +0x1ad4 the step of the SUPA banner: 0 none, 1 brackets slide in, 2 spread, 3 title, 4 name, 5 hold, 6 slide out (UpdateStartBanner)
	int	mSupaBannerY = 0;// +0x1ad8 the y of the SUPA banner's brackets: slides in from above the screen to 0 and back out
	int	mSupaBracketSpread = 0;// +0x1adc how far apart the banner's two brackets have slid, 0 to 231 (+12 an update in step 2)
	int	mSupaBracketFlash = 0;// +0x1ae0 how white the banner's brackets are, 255 at the start and -3 an update to 0 (their green and blue); drawn red at 0
	int	mSupaNameAlpha = 0;// +0x1ae4 alpha 0-255 of the weapon's name in the banner: +10 an update in step 4
	int	mSupaBannerTimer = 0;// +0x1ae8 counts updates inside a banner step: 4 for each step of the title, 149 for the hold
	int	mScreenShakeUpdates = 0;// +0x1aec the updates of screen shake left (25 when the laser takes a brick or the psychic pops its bricks); each update sets mScreenShakeX at random
	int	mPauseStartUpdate = 0;// +0x1af0 mUpdateCnt when the game was paused (Pause); the drawing uses it as its tick while the pause lasts so the animations stand still
	int	mResumeUpdate = 0;// +0x1af4 mUpdateCnt when the game was resumed from a pause; clicks in that same update are ignored
	int	mComboColor = 0;// +0x1af8 the colour of the combo display: 0 blue, 1 red, 2 yellow; moves on each time a chain reaches 2
	int	mComboTimeLeft = 0;// +0x1afc the updates the combo display stays up (300, or 600 for a chain over 5); the combo count is cleared when it runs out
	int	mComboChain = 0;// +0x1b00 chain length shown as the combo text 'x<n>' (picks the combo list row and font, DrawLogo); AdvanceChain sets it for chains of 2+, cleared when mComboTimeLeft ends
	int	mSpecialBannerHue = 0;// +0x1b04 hue 0-255 of the special banner texts: +4 (mod 256) an update while mSpecialBannerTexts is not empty (UpdateStates), drawn through HSLToRGB
	int	mExtraRowTimer = 0;// +0x1b08 updates until the next extra top row falls due (mExtraRowsDue +1, then reloaded from mExtraRowInterval); counts from mHardStage on while play runs unfrozen (UpdateStates)
	int	mExtraRowInterval = 0;// +0x1b0c updates between extra top rows: (mHardStage - stage + 60) * 100, at least 4500, set when a stage is set up; halved at each extra row down to 300; 500000000 at start
	int	mPowerBrickRowState = 0;// +0x1b10 power brick rows from mHardStage on (FillTopRow): 0 start, -1 none due, 1 or 2 a new row of that third of the stage gets a power brick, 3 given; ResetGame clears it
	int	mWarpOverlayCel = 0;// +0x1b14 cel of IMAGE_CP_WARP_OVERLAY drawn over the field while the warp runs: +1 every 5 updates, wrapping at the cel count (UpdateWarp)
	int	mComboPayDelay = 0;// +0x1b18 updates until the combo bonus is paid (PayComboBonus): 100 at every chain step (AdvanceChain), counted down by UpdateStates
	int	mLevelsCompleted = 0;// +0x1b1c levels completed in this game: the first level minus 1 (0 for a new game), +1 in LevelComplete; saved as profile setting 0x55 and restored; nothing else reads it
	int	mBoosterFlameY = 0;// +0x1b20 y the flames of a booster brick fly to, the top of the brickometer fill plus 48 (OnHyperBrick); handed to the PowerBrick as mBoosterY
	int	mSprocketOverlayCel = 0;// +0x1b24 cel of the sprocket overlay on the bricks while the atomic weapon (mode 1) runs: +1 every 10 updates, wrapping at the cel count, sent to the bricks as event 0x22
	int	mNextLevelScreenKind = 0;// +0x1b28 kind of the screen the next level begins with (UpgradeScreenKind: 0 plain, 1 bonus round, 2 upgrade, 3 mode unlock, 4 both); drawn on the slow gate
	int	mLevelEndScreenKind = 0;// +0x1b2c kind (same codes) of the screen that ends the level being played: copied from mNextLevelScreenKind in LoadLevel, saved as setting 99; picks bonus round, upgrade, unlock
	int	mNewRecordRow = 0;// +0x1b30 row in the profile's top-5 records of the game that has ended (AddRecord's result, -1 none): 0 plays the new high score sound, DrawEndPage lights the row
	int	mGameOverCounter = 0;// +0x1b34 counts up by 4 (mod 256) every update while the game ends (UpdateGameOver); nothing reads it
	int	mShipFadeAlpha = 0;// +0x1b38 alpha 0-255 of the ship's flame and charge shimmer while mStreaksFadingOut: 0 in LoadLevel, +2 an update up to 255 (UpdateEffects)
	int	mFrameDimming = 0;// +0x1b3c darkening of the frame, 0-128 (drawn shade is 255 minus it): +1 an update while the stack is high and the weapon meter full and idle, -2 otherwise (UpdateAlertFlash)
	int	mSurvivalBonusTimer = 0;// +0x1b40 updates left of the survival bonus display after a supercharge blast: 200 at the start, counted down by UpdateSparks; DrawSurvivalBonus shows it, sparks fall meanwhile
	int	mNewRankDelay = 0;// +0x1b44 updates before a new rank is announced: 100 (1 if the upgrade dialog was seen) when the warp panel finds a rank above the best (UpdateWarp); 0 sets mNewRankDelayOver
	int	mSupaButtonX = 0;// +0x1b48 x of the supa weapon's button on the frame: slides in (+2) from minus the width of IMAGE_SUPA_BUTTON_ALERT to 0 when the meter is full, out (-3) while the weapon runs
	int	mTurbotPhase = 0;// +0x1b4c phase of the turbot weapon (mode 2): 0 start, 1 field swept, 2 flash done, 3 both done and the wave rises; SweepTurbots and UpdateSpecialMode step it, BeginPlay clears it
	int	mTurbotOverlayCel = 0;// +0x1b50 cel of the turbot overlay on the bricks while the turbot weapon (mode 2) runs: +1 every 10 updates, wrapping at the cel count, sent to the bricks as event 0x24
	int	mTurbotWindUpCounter = 0;// +0x1b54 counts 0 to 100 once mTurbotPhase is 3; at 100 the flash goes full and the weapon's animation stops, then the wave rises (UpdateSpecialMode); BeginPlay clears it
	int	mTurbotBeamInterval = 0;// +0x1b58 updates between new turbot beams: 15 at the start, then 15 minus a tenth of mTurbotFlashAlpha at each beam (UpdateSpecialMode); a beam starts at a multiple of it
	int	mMutatorCooldown = 0;// +0x1b5c updates left before a mutator brick (kind 0xc) may be made again: 2000 whenever no brick is mutating, counted down while play runs (UpdateStates); CanPlaceKind refuses it
	int	mPerfectBannerAlpha = 0;// +0x1b60 alpha 0-255 of the PERFECT picture shown after ClearBonus: +8 an update up to 255, held, then -3 down to 0 (UpdateStates)
	int	mPerfectBannerState = 0;// +0x1b64 state of the PERFECT picture: 0 none, 1 fading in, 2 held (mPerfectBannerHold), 3 fading out; ClearBonus sets 1, LevelComplete and ResetGame clear it
	int	mPerfectBannerHold = 0;// +0x1b68 updates the PERFECT picture is held at full alpha: 175 set by ClearBonus, counted down in state 2 of mPerfectBannerState
	int	mCheckpointLevel = 0;// +0x1b6c the level a classic game continues from: the mode's best level rounded down to a multiple of four, -1 if below four; set when the game ends, shown on the end page +1
	int	mBonusTimeLeft = 0;// +0x1b70 updates left of the bonus round: 6000 (60 s at 100 an update) once its bricks are shown, -1 before; at 0 the clear count starts; OnHyperBrick adds 300; saved as 0x57
	int	mBonusFadeAlpha = 0;// +0x1b74 alpha 0-255 of the bonus round's fade-out: 0xff when the round is loaded, -2 an update while mBonusFadingOut (UpdateIntroAndTally)
	int	mBannerTimeLeft = 0;// +0x1b7c updates left of the banner made by BuildBanner (400); counted down in a bonus round while no supa banner runs; clicks and the tally wait until it is 0
	int	mBannerDuration = 0;// +0x1b80 the whole time of the banner in updates, as BuildBanner was given it; AnimateBanner fades the banner in over its first 75 updates with it
	int	mBannerTimeShown = 0;// +0x1b84 time left of the banner as of the last update (mBannerTimeLeft before its decrement); AnimateBanner fades the banner out over its last 75 updates with it
	int	mTallyBarWidth = 0;// +0x1b88 width in pixels of the dark bars that close in from the two sides of the field at the start of the clear count: +8 an update up to 320
	int	mTallyTitleAlpha = 0;// +0x1b8c alpha 0-255 of the tally's title text (DrawBonusStats): +8 an update in state 1 of mTallyState
	int	mTallyDelay = 0;// +0x1b90 updates the clear count tally waits before its next step (UpdateIntroAndTally), counted down; the next value depends on the state (128 at the start, 25, 3, 10, 20, 400)
	int	mTallyBricksLeft = 0;// +0x1b94 bricks left when the clear count started (rows 1 down plus the held bricks, StartClearCount); 0 means a clean field and its bonus
	int	mTallyBricksCounted = 0;// +0x1b98 bricks counted so far by the tally, up to mTallyBricksLeft; also saved as profile setting 100 when a bonus round is built
	int	mTallyPenalty = 0;// +0x1b9c the penalty of the bricks left: mTallyBricksLeft times a twentieth of mClearBonusWorth, at most mClearBonusWorth
	int	mTallyPenaltyShown = 0;// +0x1ba0 the penalty counted up so far on the tally, rising by mTallyAmountStep per brick to mTallyPenalty
	int	mTallyCleanBonus = 0;// +0x1ba4 the bonus for a clean field: mClearBonusWorth when no brick was left, else 0; the stage total is mClearBonusWorth minus mTallyPenalty plus this
	int	mTallyCleanBonusShown = 0;// +0x1ba8 the clean-field bonus counted up so far on the tally, rising by mTallyAmountStep to mTallyCleanBonus
	int	mTallyPaid = 0;// +0x1bac what has been paid into the score so far from the tally's total, at mTallyAmountStep an update (state 6 of mTallyState)
	int	mTallyState = 0;// +0x1bb0 step of the clear count tally 0-8: 0 bars close, 1 title, 2 and 3 bricks and penalty counted, 4 and 5 clean bonus, 6 total paid, 8 the end; a click jumps to 6
	int	mTallyAmountStep = 0;// +0x1bb4 the amount the tally adds per count: the penalty of one brick, then (by the size of the total) 500, 1000, 2000 or 4000 for paying it into the score
	int	mTallySoundCounter = 0;// +0x1bb8 counts the tally's steps so that the count sound plays on every second, fourth or eighth one (UpdateIntroAndTally)
	int	mHardStage = 0;// +0x1bbc the stage from which the game turns harder: 3 in the demonstration, 5 in the classic game, 500000000 (never) in survival; boosters, power rows, extra rows, quicker pieces
	int	mBonusWarpPhase = 0;// +0x1bc0 part of the warp into a bonus round while mBonusWarpIn: 1 the streaks run and the screen goes white, 2 white holds, 3 the streaks slow and the white fades; 0 not running
	int	mBonusWarpHold = 0;// +0x1bc4 counts the updates the white screen holds in part 2 of the bonus warp, up to 50 (UpdateIntroAndTally)
	int	mBonusWarpWhiteAlpha = 0;// +0x1bc8 alpha 0-255 of the white overlay in the bonus warp: +1 every second update in part 1 (to 80), 255 in part 2, -2 an update in part 3
	int	mRankNumber = 0;// +0x1bd4 the number of the rank of mScore (AstroPopApp::GetRankName), taken when the level-up text is made; its picture is mRankImage; above profile setting 0x71 it is a new rank
	float	mWeaponCharge = 0.0f;// +0x1bd8 the weapon's charge, 0 to mWeaponChargeMax: rises by mWeaponChargeStep an update while idle, falls by mWeaponDrainStep while the weapon runs; saved as setting 0x50
	float	mWeaponChargeMax = 0.0f;// +0x1bdc the charge at which the weapon is ready: 18000 (set by the constructor and ApplyWeaponLevels); the meter is full when mWeaponCharge reaches it
	float	mWeaponDrainStep = 0.0f;// +0x1be0 how much mWeaponCharge falls an update while the weapon runs, set by ApplyWeaponLevels from the pilot's duration level; the seconds shown are the charge over it over 100
	float	mWeaponChargeStep = 0.0f;// +0x1be4 how much mWeaponCharge rises an update while the field is quiet (a factor, 1.2 and up), set by ApplyWeaponLevels from mWeaponChargeLevel
	float	mBonusChance = 0.0f;// +0x1be8 chance (0-1) of a power brick in a new row: BonusChance of the level file, moved by mBonusChanceChange each level and interpolated along mSpeedRamp (FillTopRow)
	float	mPenaltyChance = 0.0f;// +0x1bec chance (0-1) of a penalty brick (stone, steel, mutator) in a new row: PenaltyChance of the level file, moved by mPenaltyChanceChange each level, along mSpeedRamp too
	float	mBonusChanceChange = 0.0f;// +0x1bf0 how much mBonusChance changes by each level (BonusChanceChangeBy of the level file, 0 if absent)
	float	mPenaltyChanceChange = 0.0f;// +0x1bf4 how much mPenaltyChance changes by each level (PenaltyChanceChangeBy of the level file, 0 if absent)
	float	mShipBounceY = 0.0f;// +0x1bf8 how far the ship is pushed down from its resting row when a brick lands on it: 0 to 5 pixels, -1 while at rest; ResetGame sets -1, the landing sets 0
	float	mShipBounceStep = 0.0f;// +0x1bfc the step of mShipBounceY an update: 1.0 down, its sign turns at 5 and again on reaching the rest (ResetGame sets 1.0)
	float	mBonusWarpSlowdown = 0.0f;// +0x1c00 how much the backdrop scroll is slowed (at most 9) in part 3 of the bonus warp: +0.09 an update from 0, the part ends at 9 (UpdateIntroAndTally, StepScrollLayer)
	int	mDropTicks = 0;// +0x1c04 updates a mutating brick waits before it drops (Brick::Update against mMutateTimer): 1000 at start, 900 at setup, MutationDelay of the level file, along mSpeedRamp too
	bool	mWeaponDialogDue = false;// +0x1c08 the pilot's weapon box is still to be shown at the first move: profile setting 0x2a plus mode is 1 (read in BeginPlay); StartByHand opens it, hints turned on again set it
	bool	mKeepLoopSoundsMuted = false;// +0x1c09 set when the options box is closed with its own button (ApplyOptions) and during CloseStory: Pause's resume then leaves the loop sounds muted
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
	void			RestoreGame(int theScore, int thePlayTime, int theLevelsCompleted, float theScoreMultiplier, float theWeaponCharge, int theSavedLevel);   // 0x467af0

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
