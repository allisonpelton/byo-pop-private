// What a .pam file holds, as Sexy::PopAnim (0x529e60-0x52f000) keeps it in memory: the images, the sprite
// definitions with their frames, and the tree of sprite instances that plays them. The exe's names for these
// classes are lost but for Sexy::PASpriteInst (vtable 0x7b902c); the others are named by what they do. Nothing
// here is specific to one game.
//
// A sprite definition is a list of frames; each frame lists every object on screen at that frame, in object
// number order (the file stores only the changes from the frame before, and the loader replays them), and may
// stop the animation or hand commands to the game. An object is an image, or another sprite, which plays on its
// own frame counter. The definitions are shared by a PopAnim and its clones, the instance trees are not.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Color.h"
#include "Rect.h"
#include "SexyMatrix.h"
#include "SharedImage.h"

namespace Sexy {

// A command a frame gives to the game (0x38 bytes): a name and one argument, both text
struct PACommand {
	std::string mCommand;
	std::string mParam;
};

// One object of one frame (0x54 bytes). mObjectNum is the object's slot in the sprite, which is also its place
// in the drawing order; mResNum is an index into the PopAnim's images, or into the animation's sprites when
// mIsSprite. The transform is the object's own, relative to the sprite that holds it.
struct PAObjectPos {
	int mObjectNum = 0;
	bool mIsSprite = false;
	int mResNum = 0;
	SexyMatrix3 mTransform;
	bool mHasSrcRect = false;  // draw only mSrcRect of the image
	Rect mSrcRect;
	Color mColor;  // multiplies the parent's color

	PAObjectPos();
};

// One frame of a sprite (0x24 bytes)
struct PAFrame {
	std::vector<PAObjectPos> mObjectPosVector;  // in object number order
	bool mHasStop = false;                      // reaching this frame stops the whole animation
	std::vector<PACommand> mCommands;
};

// A sprite definition (0x2c bytes): the frames, the frame numbers of its labels and, for each object slot, the
// definition of the sprite that was put there (null for slots that only ever held images)
struct PASpriteDef {
	std::vector<PAFrame> mFrames;
	std::map<std::string, int> mLabels;
	std::vector<PASpriteDef*> mObjectDefs;
};

// What a .pam file defines (0x18 bytes in the exe, there a pointer to the main sprite, the vector of the other
// sprites and a count of the PopAnims sharing it): shared between a PopAnim and its clones. mObjectDefs of the
// definitions point into mSprites, which is never resized after loading.
struct PAData {
	PASpriteDef mMain;
	std::vector<PASpriteDef> mSprites;
};

// One image of the animation (0x50 bytes): the picture, the name the file gave it without its options, how it is
// drawn (0 normal, 1 additive: the file's name had "(add)" in it) and the transform that puts it in its sprite.
// mImage is empty for a picture that could not be loaded.
struct PAImage {
	SharedImageRef mImage;
	std::string mName;
	int mDrawMode = 0;
	SexyMatrix3 mTransform;

	PAImage();
};

// A sprite as it is playing (Sexy::PASpriteInst, 0x1c bytes): the frame it is at, the instances of the sprites
// it holds (one per object slot) and its definition
class PASpriteInst {
public:
	int mFrameNum = 0;
	std::vector<PASpriteInst*> mChildren;
	PASpriteDef* mDef = nullptr;

	PASpriteInst() {}
	virtual ~PASpriteInst();  // 0x529e10; deletes the children

private:
	PASpriteInst(const PASpriteInst&);
	PASpriteInst& operator=(const PASpriteInst&);
};

}  // namespace Sexy
