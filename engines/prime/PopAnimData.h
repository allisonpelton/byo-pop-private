// What a .pam file holds, as Bejeweled 3's Sexy::PopAnim (code 0x440750 (bejeweled3) to 0x44b0b0 (bejeweled3)) keeps it in memory: the images,
// the sprite definitions with their frames, and the tree of sprite instances that plays them. The exe's names for
// these classes are lost; they are named by what they do, and the offsets in the comments are the exe's (the
// containers there are VS2008's, so a vector sits one pointer, the debug proxy, before its first element).
//
// A sprite definition is a list of frames; each frame lists every object on screen at that frame, in object
// number order (the file stores only the changes from the frame before, and the loader replays them), may stop
// the animation and hands commands to the game. An object is an image, or another sprite, which plays on its own
// frame counter. The definitions are shared by a PopAnim and its clones, the instance trees are not.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Color.h"
#include "Common.h"
#include "Rect.h"
#include "SexyMatrix.h"
#include "SharedImage.h"

namespace Sexy {

class PIEffect;
class PASpriteInst;
struct PASpriteDef;

// A command a frame gives to the game (0x38 bytes): a name and one argument, both text
struct PACommand {
	std::string mCommand;  // +0
	std::string mParam;    // +0x1c
};

// One object of one frame (0x64 bytes). mObjectNum is the object's slot in the sprite, which is also its place in
// the drawing order; mResNum is an index into the PopAnim's images, or into the animation's sprites when
// mIsSprite. The transform is the object's own, relative to the sprite that holds it.
struct PAObjectPos {
	std::string mName;         // +0 (a char pointer in the exe): what a frame called the object, may be empty
	int mObjectNum = 0;        // +4
	bool mIsSprite = false;    // +8
	bool mAdditive = false;    // +9: drawn with the additive blend (and, for a sprite, everything it holds)
	int mResNum = 0;           // +0xc
	int mPreloadFrames = 0;    // +0x10: how many frames a sprite object is run ahead when its frame is reached
	int mImageFrame = 0;       // +0x14: which of an image's pictures (or cels) is drawn
	float mTimeScale = 1.0f;   // +0x18: a sprite object's frames last this many times as long
	SexyMatrix3 mTransform;    // +0x1c
	bool mHasSrcRect = false;  // +0x40: draw only mSrcRect of the image
	Rect mSrcRect;             // +0x44
	Color mColor;              // +0x54: multiplies the parent's color

	PAObjectPos();
};

// One frame of a sprite (0x34 bytes)
struct PAFrame {
	std::vector<PAObjectPos> mObjectPosVector;  // +8, in object number order
	bool mHasStop = false;                      // +0x18: reaching this frame stops the whole animation
	std::vector<PACommand> mCommands;           // +0x24
};

// What one object slot of a sprite definition holds (8 bytes)
struct PADefObject {
	std::string mName;               // +0 (a char pointer in the exe), from the frame that put the object there
	PASpriteDef* mSprite = nullptr;  // +4: the sprite put there (null for slots that only held images)
};

// A sprite definition (0x60 bytes)
struct PASpriteDef {
	std::string mName;             // +0
	std::vector<PAFrame> mFrames;  // +0xc
	int mWorkStart = 0;            // +0x1c: the frames from here ...
	int mWorkLength = 0;           // +0x20: ... for this many more are the ones that play
	// the scripts ask for "ESCAPE" where a .pam says "escape": a label's case does not matter
	std::map<std::string, int, StringLessNoCase> mLabels;  // +0x24
	std::vector<PADefObject> mObjectDefs;                  // +0x50
	float mFrameRate = 0;                                  // +0x5c: frames a second
};

// What a .pam file defines (0x3c bytes in the exe): shared between a PopAnim and its clones. mObjectDefs of the
// definitions point into mSprites, which is never resized after loading.
struct PAData {
	PASpriteDef* mMain = nullptr;       // +0: the main sprite when the file has one (versions 4 and up may not)
	std::vector<PASpriteDef> mSprites;  // +0x10
	~PAData() { delete mMain; }
};

// One image of the animation (0x6c bytes): the pictures (an image name with commas gives several, which the
// objects pick from by frame), the grid the file gave, how many cels it has, the name the file gave it without
// its options, how it is drawn and the transform that puts it in its sprite.
struct PAImage {
	std::vector<SharedImageRef> mImages;  // +0xc
	int mGridWidth = -1;                  // +0x18 (-1 for versions before 4)
	int mGridHeight = -1;                 // +0x1c
	int mNumCols = 1;                     // +0x20
	int mNumRows = 1;                     // +0x24
	std::string mName;                    // +0x28
	bool mAdditive = false;               // +0x44: the file's name had "add" in its options
	SexyMatrix3 mTransform;               // +0x48: the file's six numbers in a matrix (six floats there)

	PAImage();
};

// The slot of one object of a sprite instance (0x78 bytes): the sprite instance an object that is a sprite plays
// on, the transform and color the game may put on the object, and whether the game still wants to hear of it
struct PASlot {
	std::string mName;                  // +0 (a char pointer in the exe)
	PASpriteInst* mChild = nullptr;     // +4: null for an object that is an image
	// +8: what the slot was drawn with when a transition (PopAnim::TransitionToLabel) started: its transform ...
	SexyMatrix3 mPreviousTransform;
	Color mPreviousColor;               // +0x2c: ... and its color
	bool mHasPrevious = false;          // +0x3c: the old animation had an object of this slot's name
	SexyMatrix3 mTransform;             // +0x40: multiplies the object's own transform
	Color mColor;                       // +0x64: multiplies the object's color
	bool mWantsPreDraw = true;          // +0x74: the listener is asked before the object is drawn while this is set
	bool mWantsImageDraw = true;        // +0x75: ... when its image is drawn ...
	bool mWantsPostDraw = true;         // +0x76: ... and after it is drawn; what the listener answers sets each

	PASlot();
};

// A particle effect an animation plays at a sprite (0x40 bytes)
struct PAEffectLayer {
	PIEffect* mEffect = nullptr;    // +4, owned
	std::string mName;              // +8: the effect's name, for "once"
	int mUpdateStamp = 0;           // +0x24: the PopAnim's update count when the layer was last wanted
	bool mBehind = false;           // +0x28: drawn before the sprite's objects
	bool mAttachToEmitter = false;  // +0x29
	bool mFollowsSprite = false;    // +0x2a: moves with the sprite's transform
	double mX = 0;                  // +0x30
	double mY = 0;                  // +0x38
};

// A sprite as it is playing (0x84 bytes): the frame it is at, the instances of the sprites it holds (in slots,
// one per object slot) and its definition
class PASpriteInst {
public:
	PASpriteInst* mParent = nullptr;      // +4
	int mDelayFrames = 0;                 // +8: frames to wait before moving on (the "delay" command)
	float mFrameNum = 0;                  // +0xc
	int mRepeatCount = 0;                 // +0x10: how many times the sprite has run through its frames
	bool mFrameChanged = true;            // +0x14
	int mLastParentFrame = -1;            // +0x18: the parent's frame count when this last ran
	SexyMatrix3 mTransform;               // +0x1c: where it is in the animation (set by the transform pass)
	Color mColor;                         // +0x40: its color
	std::vector<PASlot> mSlots;           // +0x58
	PASpriteDef* mDef = nullptr;          // +0x68
	std::vector<PAEffectLayer> mEffects;  // +0x74

	PASpriteInst();
	virtual ~PASpriteInst();  // port: the exe has a virtual deleting destructor; it frees the effects, the slots and the children

private:
	PASpriteInst(const PASpriteInst&);
	PASpriteInst& operator=(const PASpriteInst&);
};

}  // namespace Sexy
