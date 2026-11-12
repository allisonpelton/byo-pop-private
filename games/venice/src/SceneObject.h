// The engine's scene objects: an object has stages (timed animations of its attributes), each stage has filters (functions
// that modify an attribute over the stage's time) and events (a trigger and an action), and the object has child objects.
// Every screen, level and plate of the game is such an object, made by the scene loader from the data files.

#ifndef VENICE_SCENEOBJECT_H
#define VENICE_SCENEOBJECT_H

#include "CursorList.h"
#include "SceneTag.h"

#include <cstdint>
#include <string>

class ImageRecord;
class SceneObject;
class SceneStage;

// The numbers of an object's attributes: each object has twelve floats, a local set (the values the stages animate) and a world
// set (the local values moved through the parent's position, rotation, scale and colour)
enum SceneAttribute
{
	AttributeXScale = 0,
	AttributeYScale = 1,
	AttributeX = 2,
	AttributeY = 3,
	AttributeZ = 4,
	AttributeRotation = 5,
	AttributeRed = 6,
	AttributeGreen = 7,
	AttributeBlue = 8,
	AttributeAlpha = 9,
	AttributeFrameNumber = 10,
	AttributeAll = 11,
	AttributeCount = 12
};

// The values that stand for "no value given" in a stage's attribute: 99999 for an attribute the stage leaves alone (its end
// is the object's value when the stage starts) and 999999 for a start that continues from the end of the previous stage
const float AttributeUnsetValue = 99999.0f;
const float AttributeContinueValue = 999999.0f;
// The value a filter returns when it is finished
const float FilterFinishedValue = 9999999.0f;

enum FilterType
{
	FilterSuperCos = 0,
	FilterSquared = 1,
	FilterCubed = 2,
	FilterSquareRoot = 3,
	FilterSined = 4,
	FilterCosined = 5,
	FilterDecaySined = 6,
	FilterDecayCosined = 7,
	FilterLoopAnim = 8,
	FilterPingPongAnim = 9,
	FilterHotSpot = 10,
	FilterVelocity = 11,
	FilterVelocityLooped = 12
};

enum EventTrigger
{
	TriggerNone = 0,
	TriggerClick = 1,
	TriggerClickAnywhere = 2,
	TriggerHot = 3,
	TriggerNotHot = 4,
	TriggerStart = 5,
	TriggerEnd = 6,
	TriggerLessThan = 7,
	TriggerCode = 8,
	TriggerInterval = 9
};

enum EventAction
{
	ActionNone = 0,
	ActionInsertLevel = 1,
	ActionSendLevel = 2,
	ActionGoToStage = 3,
	ActionSendParentToStage = 4,
	ActionNextStage = 5,
	ActionReparent = 6, // no data file names it
	ActionMove = 7,
	ActionPlaySound = 8,
	ActionDimPrevious = 9,
	ActionPausePrevious = 10,
	ActionDimPreviousNoPause = 11,
	ActionBrightenPrevious = 12,
	ActionStopMusic = 13,
	ActionFadeMusic = 14,
	ActionFadePreviousMusic = 15,
	ActionMakeSparkles = 16,
	ActionMakeParticles = 17,
	ActionSetLevelNumber = 18,
	ActionNewUserName = 19,
	ActionChangeUserName = 20,
	ActionDeleteUser = 21,
	ActionGetInitialUser = 22,
	ActionKillAllPrevious = 23,
	ActionKillAll = 24,
	ActionEnterHighScore = 25,
	ActionClearScore = 26,
	ActionResetProgress = 27,
	ActionChangeImage = 28,
	ActionBumpTickCounter = 29,
	ActionCustom = 30,
	ActionSaveScore = 31,
	ActionSetMode = 32,
	ActionBumpScore = 33,
	ActionClearButtonState = 34,
	ActionEnable3DHardware = 35
};

// The object types (SceneObject::mType); the data files do not name them, the code that makes each kind sets its own
enum ObjectType
{
	ObjectTypeBase = 0,
	ObjectTypeLayer = 1,
	ObjectTypeCursor = 2,
	ObjectTypeLevel = 6,
	ObjectTypeSparkleLayer = 7,
	ObjectTypeText = 9,
	ObjectTypeInputBox = 12,
	ObjectTypeEmitter = 14,
	ObjectTypeTooltip = 15,
	ObjectTypeShooter = 16,
	ObjectTypePlate = 17,
	ObjectTypeSlot = 19,
	ObjectTypeProgressBar = 20,
	ObjectTypePanel = 21,
	ObjectTypeEndlessPanel = 22,
	ObjectTypePigeon = 23,
	ObjectTypePowerup = 24,
	ObjectTypeTurret = 26,
	ObjectTypeShooterPath = 28,
	ObjectTypeScrollPoint = 29,
	ObjectTypeLevelSelect = 30,
	ObjectTypeCupid = 31,
	ObjectTypeTutorial = 32,
	ObjectTypeWarp = 33,
	ObjectTypeCloud = 34,
	ObjectTypeShooterCursor = 35
};

// 0x34 bytes in the original; one filter of a stage
class SceneFilter
{
public:
	SceneFilter* mNext = nullptr;
	SceneFilter* mPrev = nullptr;
	SceneStage* mStage = nullptr;
	int mType = 0;
	int mAttribute = 0;
	float mAmplitude = 0;
	float mPeriod = 0;
	float mOffset = 0;
	bool mOnce = false; // finished when the stage reaches its end
	float mTickCount = 0;
	float mDuration = 0; // in frames; 0 means the stage's own progress is used
	bool mResetsAttribute = false;

	float Apply(float* theProgress); // 0x42a1c0
};

// 0x11c bytes in the original; one event of a stage
class SceneEvent
{
public:
	SceneEvent* mNext = nullptr;
	SceneEvent* mPrev = nullptr;
	SceneStage* mStage = nullptr;
	int mTrigger = TriggerNone;
	int mAction = ActionNone;
	char mAltKey = 0;
	char mParm1[64] = {};
	char mParm2[64] = {};
	char mParm3[64] = {};
	char mParm4[64] = {};

	void Set(int theTrigger, int theAction, const char* theParm1, const char* theParm2, const char* theParm3,
		const char* theParm4, const char* theAltKey); // 0x422c50
	void Check(); // 0x422e20: fires the action when the trigger holds
	void Fire(); // 0x422fd0: the actions
	bool ClickHandled(bool theClickAnywhere); // 0x424120
	bool LessThanHolds(); // 0x424200
};

// 0xf4 bytes in the original
class SceneStage
{
public:
	SceneStage* mNext = nullptr;
	SceneStage* mPrev = nullptr;
	SceneObject* mOwner = nullptr;
	float mStart[AttributeCount] = {};
	float mEnd[AttributeCount] = {};
	bool mHasStartValue[AttributeCount] = {}; // the start is taken from the object's value when the stage starts
	unsigned int mAttributeMask = 0; // bit n set: attribute n is animated by this stage
	CursorList<SceneFilter> mFilters;
	CursorList<SceneEvent> mEvents;
	bool mVisible = true;
	int mLoopStart = 0;
	int mLoopEnd = 0;
	float mFrame = 0;
	float mLength = 0; // in frames of 4 ms
	float mProgress = 0;
	int mCarriesFilters = 0; // the next stage takes over this stage's filters when it starts
	char mName[24] = {};

	SceneFilter* AddFilter(int theType, int theAttribute, float theAmplitude, float thePeriod, float theOffset,
		bool theOnce); // 0x42a070
	SceneEvent* AddEvent(int theTrigger, int theAction, const char* theParm1, const char* theParm2,
		const char* theParm3, const char* theParm4, const char* theAltKey); // 0x42a170
	SceneEvent* AddEventFromNames(const char* theTrigger, const char* theAction, const char* theParm1,
		const char* theParm2, const char* theParm3, const char* theParm4, const char* theAltKey); // 0x42a120
	SceneFilter* FindFilter(int theType, int theAttribute); // 0x42a840
	bool Update(); // 0x429f10: true while the stage is still running
	void ApplyFilters(float* theProgress); // 0x42a890: the filters of every attribute
	void ApplyAttributeFilters(float* theProgress); // 0x42a8f0
	void CheckEvents(); // 0x42a980
};

// The scene object (0x148 bytes in the original, with the classes' own fields after them).
class SceneObject
{
public:
	SceneObject* mNext = nullptr; // the next sibling (the next scene for a level)
	SceneObject* mPrev = nullptr;
	SceneObject* mParent = nullptr;
	int mType = ObjectTypeBase;
	bool mIsDead = false;
	int mHasShadow = 0;
	int mClipLeft = 0;
	int mClipTop = 0;
	int mClipRight = 0;
	int mClipBottom = 0;
	char mName[24] = {};
	int mNoCulling = 0; // the draw tests the object against the screen unless this is set (and no render target is active)
	bool mIsHot = false; // the result of IsHot on the previous update
	int mBlitStyle = 0;
	CursorList<SceneStage> mStages; // the cursor is the current stage
	CursorList<SceneObject> mChildren;
	ImageRecord* mImage = nullptr;
	ImageRecord* mParticleImage = nullptr;
	float mWorld[AttributeCount] = {};
	float mLocal[AttributeCount] = {};
	float mTickCount = 0;
	float mZOrder = 0;
	int mVisible = 0; // the visible flag of the current stage, or 0 when the window is not the one a fullscreen-only object needs
	float mSourceLeft = 0;
	float mSourceTop = 0;
	float mSourceRight = 0;
	float mSourceBottom = 0;
	int mTilesLeft = 0;
	int mTilesRight = 0;
	int mTilesUp = 0;
	int mTilesDown = 0;
	int mBilinear = 1;
	int mSelectable = 1;
	int mSnap = 1;
	bool mFlipX = false;
	bool mFlipY = false;
	int mFullscreenOnly = 0;

	SceneObject();
	virtual ~SceneObject(); // 0x425ee0
	virtual SceneStage* AddStage(float theMilliseconds, int theLoopStart, int theLoopEnd, int theVisible,
		const char* theName); // 0x4284b0
	virtual bool IsHot(); // 0x429360
	virtual SceneStage* GoToStage(SceneStage* theStage); // 0x428920
	virtual SceneStage* GoToStageByName(const char* theName); // 0x428970
	virtual void Load(const char* theText, SceneTag* theTag); // 0x426570
	virtual void SetAttribute(int theAttribute, float theStart, float theEnd); // 0x428610
	virtual void LoadAttributeTag(const char* theText, SceneTag* theTag); // 0x4286a0
	virtual SceneFilter* AddFilter(int theType, int theAttribute, float theAmplitude, float thePeriod,
		float theOffset); // 0x428700
	virtual void LoadFilterTag(const char* theText, SceneTag* theTag); // 0x428810
	virtual SceneEvent* AddEventFromNames(const char* theTrigger, const char* theAction, const char* theParm1,
		const char* theParm2, const char* theParm3, const char* theParm4, const char* theAltKey); // 0x428790
	virtual SceneEvent* AddEvent(int theTrigger, int theAction, const char* theParm1, const char* theParm2,
		const char* theParm3, const char* theParm4, const char* theAltKey); // 0x4287d0
	virtual void LoadEventTag(const char* theText, SceneTag* theTag); // 0x4288a0
	virtual SceneObject* NewChild(SceneObject* theChild); // 0x428120: the child goes in by its z order
	virtual SceneObject* AddChild(SceneObject* theChild); // 0x4281e0: the child goes on the end
	virtual void MakeSparkles(int theCount, float theRed, float theGreen, float theBlue, float theAlpha); // 0x429870
	virtual void MakeParticles(int theCount); // 0x429b00
	virtual void OnObjectClosed() {} // 0x403510: the loader read this object's closing tag
	virtual void AddToAttribute(int theAttribute, float theDelta); // 0x4260a0
	virtual void Move(float theDeltaX, float theDeltaY); // 0x426110
	virtual void Rotate(float theDegrees); // 0x4262a0
	virtual void OnCustomEvent() {} // 0x429d20: what the Custom event action calls
	virtual bool Update(); // 0x426dc0: false when the object is dead or has no stage left
	virtual void UpdateChildren(); // 0x426f10
	virtual void Draw(); // 0x426f60
	virtual void DrawSelf(); // 0x4270f0
	virtual void DrawHot(); // 0x427760
	virtual void DrawChildren(); // 0x4277d0
	virtual void OnFirstUpdate() {} // 0x425ff0
	virtual void Rewind(); // 0x426000
	virtual void Kill(); // 0x429d30

	void Reset(); // 0x425ce0
	void ResetAttributes(float* theAttributes); // 0x42aed0
	void UpdateWorldValues(); // 0x427df0
	void SetImage(const char* theName, int theFrameWidth, int theFrameHeight, int theFrameCount, int theFramesPerSecond);
	void SetImageFrom(SceneObject* theObject); // 0x427fe0
	void SetImageRecord(ImageRecord* theImage); // 0x428080
	void InsertChildByZOrder(SceneObject* theChild); // 0x428160
	void SetZOrder(float theZOrder); // 0x429720
	SceneObject* GetRoot(); // 0x428220
	SceneObject* FindChildByName(const char* theName, SceneObject* theAfter); // 0x428250
	SceneObject* FindChildByType(int theType, SceneObject* theAfter); // 0x4282b0
	int CountChildrenOfType(int theType); // 0x428460
	SceneObject* FindLowestChild(); // 0x4283f0
	SceneObject* FindNearestSibling(int theType); // 0x428310
	SceneStage* NextStage(); // 0x4289d0
	void ApplyStageDefaults(SceneStage* theStage); // 0x428a90
	SceneStage* LoopBackStage(); // 0x428b60
	bool HitTest(float theX, float theY, float theWidth, float theHeight, float theRotation); // 0x428bd0
	void AdoptNewestChild(SceneObject* theSource, bool theByZOrder); // 0x429420: the newest child of theSource becomes a child here
	int CountStagesBefore(); // 0x4295e0
	float DistanceTo(SceneObject* theObject); // 0x429790
	int AngleTo(SceneObject* theObject); // 0x429810
	void FlipHorizontally(); // 0x429c90
	void SnapToGrid(); // 0x426370
	float GetWorldX(float theX, float theY); // 0x427950: the parent's transform of a local point
	float GetWorldY(float theX, float theY); // 0x427a30
	float GetWorldRotation(float theRotation); // 0x427b10
	float GetWorldAlpha(float theAlpha); // 0x427b70
	float GetWorldXScale(float theScale); // 0x427ba0
	float GetWorldYScale(float theScale); // 0x427bd0
	float GetLocalX(float theWorldX, float theWorldY); // 0x427c00
	float GetLocalY(float theWorldX, float theWorldY); // 0x427c30
	float GetLocalRotation(float theRotation); // 0x427c60
	void RemoveChildAtCursor(); // 0x435330
};

#endif
