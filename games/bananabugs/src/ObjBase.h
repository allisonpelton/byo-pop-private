// ObjBase: the scene graph every screen and sprite of Banana Bugs is built from (0x4509f0..0x453e30 in the original).
// An object has 18 animated values (position, depth, scale, colour, rotation and more), a parent whose values its
// own are relative to, and tracks of keys that move the values over time. Each key holds the values to reach, how
// long to take and how to ease, and per value a chain of modifiers (waves, orbits, counters, random jitter).
// Objects draw sorted by depth, and the one under the mouse gets the enter, leave, press and release calls.
#pragma once

#include "SexyMatrix.h"

namespace Sexy
{

class Anim;
class GameApp;
class Graphics;
class ObjBase;

// The animated values, in the order the original's arrays hold them
enum ObjState
{
	STATE_X = 0,
	STATE_Y = 1,
	STATE_Z = 2,           // the depth objects draw in, lowest first
	STATE_SCALE_X = 3,
	STATE_SCALE_Y = 4,
	STATE_OFFSET_X = 5,    // the picture's offset from the position, scaled with it
	STATE_OFFSET_Y = 6,
	STATE_RED = 7,         // colour, 0 to 1
	STATE_GREEN = 8,
	STATE_BLUE = 9,
	STATE_ALPHA = 10,
	STATE_ROTATION = 11,   // radians
	STATE_ORBIT_ANGLE = 12,
	STATE_FRAME = 13,      // the Anim's cel
	STATE_ORBIT_RADIUS = 14,
	STATE_EXTRA_WIDTH = 15,  // added to the cel's source rectangle
	STATE_EXTRA_HEIGHT = 16,
	STATE_PATH = 17,       // the position along a path modifier's table
	NUM_STATES = 18
};

// How a key's time maps to its progress (AnimKey::mEasing)
enum KeyEasing
{
	EASE_LINEAR_CLAMPED = 0, // linear, or done at once for a key shorter than a frame
	EASE_IN_SQUARE = 1,
	EASE_IN_CUBE = 2,
	EASE_LINEAR = 3,
	EASE_COSINE = 4,
	EASE_COSINE_TOO = 5,     // the same curve as EASE_COSINE
	EASE_BY_DISTANCE = 6,    // the share of the way to the next key's position covered
	EASE_UNSET = 7,          // the original reads a value it never set; no key uses it
};

// What ends a key early (AnimKey::mEndCondition); KEY_SOUND plays a sound as the key starts instead
enum KeyEnd
{
	KEY_END_TIME = 0,
	KEY_END_ABOVE = 1,          // the value passed mEndValue
	KEY_END_DRAWN_ABOVE = 2,    // the drawn value passed mEndValue (scaled to the window)
	KEY_END_DRAWN_BELOW = 3,
	KEY_END_ARRIVED = 4,        // within 8 of the next key's position
	KEY_END_OFF_SCREEN = 5,
	KEY_END_AT_ONCE = 6,
	KEY_SOUND = 7,              // mEndChannel is the sound, mEndValue how many steps it may be pitched up at random (SoundQueue)
};

// The modifier kinds (KeyModifier::mKind), numbered as the original numbers them
enum ModifierKind
{
	MOD_HOLD = 0,             // the key's value plus the base, as any unknown kind
	MOD_TOWARD_NEXT = 1,      // from this key's value to the next key's, by progress
	MOD_SINE = 2,             // a sine wave scaled by the key's value
	MOD_ORBIT_SIN = 3,        // minus sin(orbit angle) * orbit radius
	MOD_ORBIT_COS = 4,        // plus cos(orbit angle) * orbit radius
	MOD_ADD_VALUE = 5,        // plus the key's value each frame
	MOD_FACE_MOTION = 6,      // the rotation follows the direction of motion
	MOD_TURN_TOWARD = 7,      // the rotation turns toward the next key's position
	MOD_COUNT_UP = 8,         // a counter that wraps at mAmount
	MOD_COUNT_DOWN = 9,
	MOD_COUNT_BY_PULSE = 10,  // a counter whose step follows the parent's key pulse
	MOD_PING_PONG = 11,
	MOD_BASE_PLUS_VALUE = 12, // the same as an unknown kind: base plus the key's value
	MOD_RANDOM = 13,          // a random whole number every mRate frames
	MOD_RANDOM_HUNDREDTHS = 14,
	MOD_WRAP_DEGREES = 15,    // every mRate frames, the angle less mAmount degrees, kept positive
	MOD_SPIN = 16,            // every mRate frames, mAmount degrees more
	MOD_SPIN_BY_SPEED = 17,   // turning by the speed of motion
	MOD_SPIN_BY_START_SPEED = 18, // turning by the speed at the key's start, slowing by 1% a frame
	MOD_FLICKER = 19,         // red, green and blue to mAmount or mBase at random
	MOD_COPY_ORBIT_ANGLE = 20,
	MOD_DECAY = 21,           // an offset that shrinks each frame
	MOD_PATH = 22,            // a value from mTable, indexed by the path position
};

// One modifier of one value of a key (0x44 bytes in the original)
struct KeyModifier
{
	const char* mName;   // "this will be name for xml"; nothing reads it
	float mAmount;
	int mDirection;      // MOD_PING_PONG's direction, +1 or -1
	KeyModifier* mNext;
	int mKind;           // ModifierKind
	float mRate;
	float mBase;
	float mPulseStep;    // MOD_COUNT_BY_PULSE's last step
	float mStep;         // mRate / 100
	float mAngle;        // degrees, for the spins
	const float* mTable; // MOD_PATH's values
	float mAccum;        // MOD_PING_PONG's value; MOD_FACE_MOTION's smoothed angle
	float mLastAngle;    // MOD_FACE_MOTION's last angle
	float mInitialAmount;
	float mInitialRate;
	float mInitialBase;

	KeyModifier();
	void Set(int theKind, const float* theTable, float theRate, float theAmount, float theBase);
	void Reset();
};

// One key of a track (0x13c bytes in the original); shared by the tracks it is in, by a count
struct AnimKey
{
	int mEndCondition;    // KeyEnd
	int mEndChannel;      // the value an end condition reads; KEY_SOUND's sound
	float mEndValue;      // KEY_SOUND's volume
	float mDuration;      // frames; KEY_END_BY_DISTANCE's distance
	int mEventId;         // which key ended, for the object's Notify
	int mRefCount;
	bool mRemoveAtEnd;    // the object goes when its last track ends here
	float mUnusedOne;     // set to 1 by Init; nothing reads it
	int mDrawMode;        // Graphics draw mode while this key's Anim shows
	int mEasing;          // KeyEasing
	float mValue[NUM_STATES];
	float mNextValue[NUM_STATES]; // the next key's values (this key's own for the last), set as the key starts
	KeyModifier* mModifiers[NUM_STATES];
	bool mKeepCurrent[NUM_STATES]; // at this key's end the next key takes the value as it is
	Anim* mAnim;          // the picture to switch to, if any
	AnimKey* mNext;

	AnimKey();
	~AnimKey();
	void AddRef();
	void Release();
	void ResetModifiers();
	void SetNextValues(AnimKey* theNext);
	void SetDuration(int theFrames);
	void AddModifier(int theChannel, bool keepCurrent, float theValue, int theKind, float theRate, float theAmount,
		float theBase, const float* theTable);
	void SetRemoveAtEnd() { mRemoveAtEnd = true; } // 0x453580
	// AddModifier as most keys call it: not kept, no rate, amount, base or table
	void AddValue(int theChannel, float theValue, int theKind)
	{
		AddModifier(theChannel, false, theValue, theKind, 0.0f, 0.0f, 0.0f, nullptr);
	}
	void AddDecay(int theChannel, float theValue, float theStart, float theScale, float theRate, float theAmount);
	void SetDecay(int theChannel, float theStart, float theScale);
	float Progress(ObjBase* theObj);
};

// A sequence of keys (0x20 bytes in the original). A track that ends may hand over to the next track.
struct AnimTrack
{
	int mKeyCount;
	AnimKey* mFirst;
	AnimTrack* mNext;
	int mDelay;          // frames waited, after one first frame of keys (none when set with skipFirstFrame)
	int mDelayLeft;
	AnimKey* mCurrent;
	AnimKey* mLast;
	bool mKeyStarting;

	AnimTrack();
	~AnimTrack();
	void CopyFrom(const AnimTrack& theTrack);
	void Clear();
	void AddKey(AnimKey* theKey, AnimKey* theNext);
	void Restart();
	void SetDelay(int theFrames, bool skipFirstFrame);
	AnimKey* GetCurrent() const { return mCurrent; }           // 0x4790d0, 0x453a30
	Anim* GetCurrentAnim() const;
	int GetCurrentDrawMode() const;
	int Update(ObjBase* theObj);
};

// The results of AnimTrack::Update
enum TrackStep
{
	TRACK_WAIT = 0,
	TRACK_RUNNING = 1,
	TRACK_NEXT_KEY = 2,
	TRACK_NEXT_KEY_WITH_ANIM = 3,
	TRACK_ENDED = 4,
	TRACK_ENDED_REMOVE = 5,
	TRACK_NEXT_TRACK = 6,
};

const int OBJ_MAX_TRACKS = 32;
const int OBJ_MAX_DRAWN = 50000;
const int OBJ_MAX_DELETED = 50000;

// The child lists each object keeps
enum ObjList
{
	LIST_ACTIVE = 0,   // updated and drawn
	LIST_INACTIVE = 1,
	LIST_OWNED = 2,    // every child; deleted with the parent
	NUM_OBJ_LISTS = 3
};

// The object types the original marks (mType)
enum ObjType
{
	OBJ_TYPE_PLAIN = 0,
	OBJ_TYPE_ROOT = 1,
	OBJ_TYPE_EMITTER = 6,
	OBJ_TYPE_OWNED_ELSEWHERE = 7, // its owner is told when its parent goes
	OBJ_TYPE_STRING = 8,          // an ObjString
	OBJ_TYPE_STRING_GLYPH = 9,    // a shown character of one
};

class ObjBase
{
public:
	float mKeyPulse;         // 0 to 1 and back over a key, for children's MOD_COUNT_BY_PULSE
	bool mChildColorOwn;     // the children's colour is their own, not multiplied by this one's
	int mUnusedC;            // set to 0 by the constructor; nothing in the engine reads it
	int mUnused10;           // set to 0 by the constructor; nothing in the engine reads it
	ObjBase* mParent;
	ObjBase* mOwner;
	int mType;               // ObjType
	float mKeyTime;          // frames into the current key
	float mKeySpeed;         // frames a frame
	Anim* mAnim;
	bool mOwnsAnim;
	int mDrawMode;
	bool mNotUpdatedYet;
	float mMouseX;           // the mouse in the object's own space
	float mMouseY;
	bool mMouseable;
	bool mHoverToParent;     // the mouse calls go to the parent
	int mDeleteIndex;        // the place in the delete list, or -1
	bool mWholePixels;       // drawn at whole-pixel positions
	AnimTrack* mTracks[OBJ_MAX_TRACKS];
	AnimTrack* mCurrentTrack;
	AnimKey* mCurrentKey;
	int mTrackCount;
	bool mHidden;
	int mButtonId;           // a window's or menu's number for a button object; 0 for others
	int mHoverState;         // a button's: 1 once its hover track has started, 0 after its leave track
	int mUnusedF0;           // set to 0 by the constructor; nothing reads it
	int mUnusedF4;           // set to 0 by the constructor; nothing reads it
	float mScreenX;          // the drawn position in the 1920x1200 design space, from its centre
	float mScreenY;
	float mState[NUM_STATES];  // the values as animated
	float mBase[NUM_STATES];   // the values keys are relative to
	float mDrawn[NUM_STATES];  // the values after the parent's, in window pixels
	ObjBase* mFirstChild[NUM_OBJ_LISTS];
	ObjBase* mNextSibling[NUM_OBJ_LISTS];
	float mHitWidth;
	float mHitHeight;
	float mHitRadius;
	float mPrevX;            // the position at the start of the frame
	float mPrevY;
	float mVelX;
	float mVelY;
	float mSmoothVelX;
	float mSmoothVelY;
	float mKeyStartSpeed;    // the larger smoothed speed as the key started
	int mUpdateLevel;        // updated only while the engine's level is below this (ObjBase::SetUpdateLevel)
	ObjBase* mEventTarget;   // told of key ends (Notify)
	ObjBase* mMouseListener; // gets the mouse calls
	bool mActive;
	int mUpdateCount;
	ObjBase* mLastChild[NUM_OBJ_LISTS];
	ObjBase* mPrevSibling[NUM_OBJ_LISTS];
	bool mFollowMouse;       // set to false by the constructor; nothing sets it

	ObjBase();
	virtual ~ObjBase();
	virtual void Init();                                       // slot 1
	virtual void Activate(ObjBase* theObj, bool hidden);       // slot 2
	virtual void Deactivate(ObjBase* theObj);                 // slot 3
	virtual void Notify(ObjBase* theObj);                      // slot 4
	virtual bool Update();                                     // slot 5
	virtual void Delete();                                     // slot 6
	virtual void Draw(Graphics* g);                            // slot 7
	virtual bool MouseDown(ObjBase* theObj, bool isDown);      // slot 8
	virtual bool MouseUp(ObjBase* theObj);                     // slot 9
	virtual void MouseEnter(ObjBase* theObj, bool isNew);      // slot 10
	virtual void MouseLeave(ObjBase* theObj);                  // slot 11
	virtual void Slot12(int theValue);                         // slot 12, empty in the base
	virtual void Slot13(int theFirst, int theSecond, int theThird); // slot 13, empty in the base

	void CopyFrom(ObjBase* theObj);
	void SetEventTarget(ObjBase* theTarget) { mEventTarget = theTarget; }     // 0x4514a0
	void SetMouseListener(ObjBase* theListener) { mMouseListener = theListener; } // 0x4514b0
	void SetAdditive() { mDrawMode = 1; }                     // 0x4514f0
	void ResetStates();
	void SetParent(ObjBase* theParent, ObjBase* theOwner);
	void UpdateHitSize();
	bool IsActive() const { return mActive; }                  // 0x451690
	void SetHidden(bool hidden) { mHidden = hidden; }          // 0x4516a0
	void SetMouseable(float theWidth, float theHeight);
	void ListAdd(int theList, ObjBase* theParent);
	void ListRemove(int theList, ObjBase* theParent);
	void SetDesignX(float theX);
	void SetDesignY(float theY);
	void SetState(int theChannel, float theValue);
	void SetStateInt(int theChannel, int theValue) { SetState(theChannel, (float)theValue); } // 0x451870
	int AddTrack(AnimTrack* theTrack, AnimTrack* theNext);
	void ClearTracks();
	void StartTrack(int theIndex);
	void ComputeDrawn();
	bool UpdateTree();
	bool UpdateTreeAtLevel();
	void CollectDrawn();

	// The engine's state (globals in the original)
	static int sObjectCount;           // 0x60e6b8: gives each new object a depth just above the last
	static GameApp* sApp;              // 0x60e6bc
	static ObjBase* sLevel;            // 0x60e6c0: the level being played, for the objects that look at it
	static ObjBase* sRoot;             // 0x60e6c4
	static int sFrame;                 // 0x60e6c8: the frame number UpdateAll was given
	static int sDrawnCount;            // 0x60e6cc
	static ObjBase* sDrawn[OBJ_MAX_DRAWN];     // 0x60e6d0
	static ObjBase* sDeleted[OBJ_MAX_DELETED]; // 0x63f410
	static int sDeletedCount;          // 0x670150
	static float sMouseX;              // 0x670154: the mouse in window pixels
	static float sMouseY;
	static float sMouseDesignX;        // 0x67015c: in the design space, from its centre
	static float sMouseDesignY;
	static float sMouseAverageX;       // 0x670164: the mean of this and the last position
	static float sMouseAverageY;
	static ObjBase* sHover;            // 0x67016c
	static ObjBase* sPressed;          // 0x670170
	static int sKeyPressed;            // 0x670174: taken once by TakeKeyPressed
	static int sKeyHeld;               // 0x670178
	static float sHoverMinZ;           // 0x67017c: objects lower than this get no mouse
	static int sUpdateLevel;           // 0x670180
	static Sexy::GameApp* sBounds;     // 0x670184: the app whose size KEY_END_OFF_SCREEN reads
	static bool sMouseHeld;            // 0x60e649
	static bool sMouseReleased;        // 0x60e64a

	static void SetHoverMinZ(float theZ) { sHoverMinZ = theZ; }   // 0x451dc0
	static float GetHoverMinZ() { return sHoverMinZ; }             // 0x451dd0
	static void UpdateAll(int theFrame);
	static void DrawAll(Graphics* g);
	static bool MousePress();
	static void MouseDrag(int x, int y);
	static void MouseRelease();
	static void SetKeyHeld(int theKey) { sKeyHeld = theKey; }      // 0x4520e0
	static void SetKeyPressed(int theKey) { sKeyPressed = theKey; } // 0x4520f0
	static int TakeKeyPressed();
	static void SetUpdateLevel(int theLevel) { sUpdateLevel = theLevel; } // 0x452110
	static void FollowScreenScale();
	static void InitEngine(GameApp* theApp);
	static void SetLevel(ObjBase* theLevel) { sLevel = theLevel; }  // 0x452260
	static void SetMousePos(int x, int y);
	static void SetBounds(GameApp* theApp) { sBounds = theApp; }    // 0x453920
};

// The game's own random generator (0x4733b0) and its seed (0x5fffe8)
unsigned int GameRandomNext();
void GameRandomSeed(unsigned int theSeed);
unsigned int GameRandomState();
// 0x404890: a whole number from theMin to theMax, both included
int GameRandomRange(int theMax, int theMin);

// The float maths helpers the original calls (each rounds its result to a float)
float FloatCos(float theAngle);   // 0x404850
float FloatSin(float theAngle);   // 0x404870
float FloatAbs(float theValue);   // 0x405f30
float FloatSqrt(float theValue);  // 0x405f70
float FloatAtan2(float theY, float theX); // 0x405f90
float FloatSign(float theValue);  // 0x452900
// A place given from the design space's corner, from its centre instead (0x473440)
inline float FromCorner(float theValue, float theSize) { return (float)(theValue - (double)theSize * 0.5); }

// The original's constants: pi and 2 pi as floats, which it widens to doubles
const double PI_F = 3.1415927410125732;
const double TWO_PI_F = 6.2831854820251465;

} // namespace Sexy
