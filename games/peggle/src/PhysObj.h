// PhysObj.h: a piece of a level (the original's Sexy::PhysObj and the base of its Poly, Line, Ball, Brick, Hole and Gun). It has
// a box, the flags that say how the ball meets it, a mover, a peg's data and a picture. The level editor's functions
// (EditGetSetValHook, EditDraw*, EditContains, EditIntersects, EditGetDragMode, EditReflect, and the two that move a piece
// with its mover, vtable slots 10 and 11) are left out: the editor is not part of the game.

#ifndef __PHYSOBJ_H__
#define __PHYSOBJ_H__

#include "Mover.h"
#include "PegInfo.h"
#include "SharedPtr.h"

#include <string>

namespace Sexy
{

class DataSync;
class Graphics;
class Image;

// What GetClass gives: which class a piece is, and the number a level file names it by (0x404ca0 makes one from the number)
enum PhysObjClass
{
	CLASS_PHYSOBJ = 1,
	CLASS_LINE = 2,
	CLASS_POLY = 3,
	CLASS_SHOT_BALL = 4, // a Ball(false): the ball the cannon shoots (the kind KIND_SHOT_BALL)
	CLASS_PEG = 5,		 // a Ball(true): a round peg of a level (the kind KIND_PEG)
	CLASS_BRICK = 6,
	CLASS_GUN = 7,
	CLASS_HOLE = 8
};

// The values of mKind (+0x10): which sort of piece this one is
enum PhysObjKind
{
	KIND_PLAIN = 0, // a PhysObj as such
	KIND_HOLE = 1,
	KIND_SHOT_BALL = 2, // a ball in play: it moves, so it is saved with its speed and its hat
	KIND_PEG = 3,		// a round peg of a level
	KIND_LINE = 4,
	KIND_POLY = 5
};

class PhysObj : public RefCounted
{
public:
	PhysObj();			   // 0x480660
	~PhysObj() override;   // 0x47dfb0 (0x47d230)

	// the virtual functions of the original, by vtable slot
	virtual void SyncState(DataSync& theSync);		// 2, 0x480750
	virtual int GetClass() const { return CLASS_PHYSOBJ; } // 3: the number a level file names the class by
	virtual unsigned int GetTypeFlags() const;		// 4, 0x479890
	virtual void SetPos(float theX, float theY) {}	// 17
	virtual void Translate(float theDeltaX, float theDeltaY); // 18, 0x476b00
	virtual void SetVelocity(float theDeltaX, float theDeltaY) {} // 19
	virtual void CreateImage(bool theRemake) {}		// 20: makes the picture a peg of this kind has (a Brick's is made from its triangles)
	virtual void DeleteImage();						// 21, 0x4769c0
	virtual void Update();							// 22, 0x479a30
	virtual void UpdateSlowMo(float theSubFrame);	// 23, 0x479b00
	virtual void DrawShadow(Graphics* g) {}			// 24
	virtual void Draw(Graphics* g) {}				// 25
	virtual void DrawLight(Graphics* g) {}			// 26, 0x479b70 (PegInfo::DrawLight; to be translated with the drawing)
	virtual void AddedToGame() {}					// 27, 0x52a680: the piece has been put in the board's lists (Ball's is 0x477070)
	virtual void NotifyCollision() {}				// 28
	virtual void GetCenter(float* theCenter) const; // 29, 0x476a00
	virtual float GetXPos() const;					// 30, 0x47d350
	virtual float GetYPos() const;					// 31, 0x47d370
	virtual bool IsCollidableWith(const PhysObj* theOther) const { return mCollidable; } // 32, 0x47d390
	virtual void SetActiveWithGrowAnim(bool theActive) { SetActive(theActive); } // 33, 0x454790

	void SetPegKind(int theKind);	// 0x479870: a peg becomes a blue, orange, green or purple one (and its picture is made again)
	void SetActive(bool theActive); // 0x4769f0: a piece that is not active is neither hit nor drawn
	void Reset();					// 0x47f740: everything a level file may set goes back to its default
	bool LoadImage();				// 0x47f830: gets the picture named mImageName; false when there is none
	void UpdateFromMover();			// 0x479940: puts the piece where its mover has it at mMoverTime
	void SetMoverTime(int theTime); // 0x479980
	void MoveToPhase(float thePhase, bool theJump); // 0x4799a0: runs the mover until its cycle is at thePhase (a negative phase counts back from the next cycle), or jumps there at once
	void PlayHitEffects(int theSound); // 0x476bf0: the sound and the bubbles of a piece that toggles

	int mSerialNumber;		   // +0x58: counts the pieces made (the original's global at 0x650a68)
	int mKind;				   // +0x10: see PhysObjKind
	float mLeft;			   // +0x14: the box around the piece
	float mTop;				   // +0x18
	float mRight;			   // +0x1c
	float mBottom;			   // +0x20
	bool mCollidable;		   // +0x24 (physobj_collidable)
	bool mVisible;			   // +0x25 (physobj_visible)
	bool mMoving;			   // +0x26 (physobj_moving): the mover is running
	bool mBackground;		   // +0x27 (physobj_background): drawn behind the rest
	bool mForeground;		   // +0x28 (physobj_foreground)
	bool mForeground2;		   // +0x29 (physobj_foreground2)
	bool mBaseObject;		   // +0x2a (physobj_baseobj)
	bool mBallStopReset;	   // +0x2b (physobj_ballstopreset)
	bool mDrawSorted;		   // +0x2c (physobj_drawsort)
	bool mDrawFloating;		   // +0x2d (physobj_drawfloat)
	bool mDrawFloatingCopy;	   // +0x2e: set from mDrawFloating when a level is read (and always for the movers of shape 8, 9 and 13)
	bool mImageCached;		   // +0x2f: the picture was in the level pictures' cache already when the level's background was loaded
	bool mUnknown30;		   // +0x30: one more flag of the version 15 run
	float mBounciness;		   // +0x34 (physobj_bouncy)
	float mRolliness;		   // +0x38 (physobj_rolly)
	float mMaxBounceVelocity;  // +0x3c (physobj_maxbouncevel)
	int mSolidColor;		   // +0x40 (physobj_solidcolor)
	int mOutlineColor;		   // +0x44 (physobj_outlinecolor): 0xffffff when it has none
	SharedPtr<Mover> mMover;	   // +0x48: how the piece moves, or none
	int mMoverTime;			   // +0x4c: the update the mover is at; counts while the piece moves
	int mMovingTimer;		   // +0x50: updates until mMoving is cleared; 0 means it never is
	int mSound;				   // +0x54 (physobj_sound)
	std::string mId;		   // +0x5c (physobj_id)
	std::string mLogic;		   // +0x78 (physobj_logic): the name of the logic that rules the piece
	std::string mImageName;	   // +0x94 (physobj_image)
	int mSubId;				   // +0xb0 (physobj_subid)
	Image* mImage;			   // +0xb4: the picture, which belongs to the level's ImageMgr unless mOwnsImage is set
	float mImageDX;			   // +0xb8 (physobj_imagedx): the picture's offset from the box
	float mImageDY;			   // +0xbc (physobj_imagedy)
	float mImageRotation;	   // +0xc0 (physobj_imagerot)
	bool mOwnsImage;		   // +0xc4: mImage is deleted with the piece
	int mInactiveTimer;		   // +0xc8: updates until the piece is made inactive; 0 means never
	int mFlipperFlags;		   // +0xcc (physobj_flipperflags)
	SharedPtr<PegInfo> mPegInfo; // +0xd0: what makes the piece a peg, or none
	int mUnknownD4;			   // +0xd4: set to 0 by the constructor and read by nothing found yet
	int mUnknownD8;			   // +0xd8: likewise
	int mSavedValueDC;		   // +0xdc: saved by the files of the replay's kind (sync mode 2) only
	bool mSavedFlagE0;		   // +0xe0: likewise
	bool mFlagE1;			   // +0xe1: read by GetTypeFlags (bit 0x20000 when clear)
};

// The way the current sync is going, which changes what some SyncState functions read and write (the original's global
// at 0x650a5c): 0 and 1 are the two kinds of level file, 2 is a saved game or a replay's sync point, 3 is the editor.
enum SyncMode
{
	SYNC_LEVEL = 0,
	SYNC_LEVEL_FLAGGED = 1,
	SYNC_STATE = 2,
	SYNC_EDITOR = 3
};
extern int gSyncMode;			// 0x650a5c
extern int gPhysObjCount;	// 0x650a68: the pieces made so far, which gives each its number (the board's Clear starts it again from 0)
extern bool gSyncNoImages;		// 0x650a60: when set a piece does not load its picture as it is read
extern bool gSyncKeepSavedRotation; // test only: a piece with a mover keeps the rotation the file gave it (the original's reader replaces it with the mover's)

} // namespace Sexy

#endif
