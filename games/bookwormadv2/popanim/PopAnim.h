// Sexy::PopAnim (constructor 0x52ac30, 0x168 bytes, vtable 0x7b9034; code 0x529e60-0x52f000) and
// Sexy::PopAnimListener (vtable 0x7abff0): PopCap's player for .pam animations, a Widget that loads an
// animation, plays it a frame at a time and draws it. Nothing here is specific to one game.
//
// A .pam file holds the animation's images, which it names (the files are found beside it), and a set of
// sprites. A sprite is a list of frames; each frame says what is on screen (an image or another sprite, with a
// transform, a color and optionally part of the image), whether the animation stops there, and what commands go
// to the game. The main sprite is the animation itself and loops unless a frame stops it. A PopAnim never
// starts on its own: the game plays it from a frame or a label (PlayFrame, PlayLabel), updates it every frame
// (UpdateF) and draws it (Draw) where it translated the Graphics to.
//
// A .txt "mod file" can stand for the .pam: it names the .pam (SetPamFile) and renames what the file calls its
// images, labels and commands (Remap, with a * at either end of the name as a wildcard) and more, so that one
// animation serves several uses (LoadMod, PopAnimModParser.h).
//
// Not ported: the mirrored variant (0x52db80 draws it, 0x54d850 flips the images; mMirror is set by nothing in
// the game's code, so it never runs).
#pragma once

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "Color.h"
#include "PopAnimData.h"
#include "SexyMatrix.h"
#include "Widget.h"

namespace Sexy {

class Buffer;
class Graphics;
class PopAnimModParser;

// What the game hears from an animation (two virtual functions, no destructor in the exe, empty there)
class PopAnimListener {
public:
	virtual ~PopAnimListener() {}
	// slot 0, 0x434bd0: a frame that stops the animation was reached
	virtual void PopAnimStopped(int theId) {}
	// slot 1, 0x434be0: a frame with commands was reached, once per command, in the file's order
	virtual void PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam) {}
};

class PopAnim : public Widget {
public:
	int mId;                      // +0x88, what the listener is told the animation is
	PopAnimListener* mListener;   // +0x8c
	// +0x90: the folders searched for an image, each relative to the folder of the .pam
	std::vector<std::string> mImageFolders;
	int mVersion;                 // +0xa0, the .pam's (0 to 3; the game's files are all 1)
	bool mLoaded;                 // +0xa4
	std::string mPamFile;         // +0xa8, the .pam a mod file names (SetPamFile)
	std::string mPamFileName;     // +0xc4, the path it was loaded from (what the scripts ask for)
	std::list<std::pair<std::string, std::string> > mRemaps;  // +0xe0, a mod file's Remap commands
	float mFraction;              // +0xec, how far past the current frame the animation is (0 to 1)
	int mFrameRate;               // +0xf0, frames a second
	int mPamX, mPamY, mPamWidth, mPamHeight;  // +0xf4, the animation's bounds in pixels
	std::string mError;           // +0x104, why the last load failed
	std::string mLabel;           // +0x120, the label last played
	std::vector<PAImage> mImages; // +0x13c
	PASpriteInst* mRoot;          // +0x14c
	std::shared_ptr<PAData> mData;  // +0x150, shared with the clones
	Color mAnimColor;             // +0x154, multiplies every color in the animation
	bool mPlaying;                // +0x164
	bool mMirror;                 // +0x165, nothing sets it in the exe; its mirrored drawing is not ported, leave it false
	bool mInterpolate;            // +0x166, draw between frames (the default); false steps from frame to frame
	bool mImageIsNew;             // +0x167, whether the last image looked up had to be loaded
	// Pictures that could not be loaded (the game's .jp2 files, for one): the animation loads and plays without
	// them. The exe stops the load with an error instead.
	std::vector<std::string> mMissingImages;

	PopAnim(int theId, PopAnimListener* theListener);  // 0x52ac30
	~PopAnim() override;                               // 0x52ae50

	// Loads a .pam, or a .txt mod file; a name without an extension is tried as a .pam, then as a .txt. False
	// when it cannot (mError says why). Replaces what the animation held. 0x52d4d0
	bool LoadAnimation(const std::string& theFileName);
	// A copy that plays on its own: the same pictures and definitions, its own place in the animation (at the
	// first frame), the same state otherwise. 0x52b150
	PopAnim* Clone() const;

	// Starts playing at a frame; false when there is none (and the animation stops). 0x52d7a0
	bool PlayFrame(int theFrame);
	// Starts playing at a label of the main sprite; false when there is none (the animation stops). 0x52d850
	bool PlayLabel(const std::string& theLabel);
	// The frame of a label of the main sprite, or -1. 0x529c80
	int GetLabelFrame(const std::string& theLabel) const;
	// Puts every sprite back at its first frame; does not stop or start the animation. 0x52d790
	void ResetAnim();
	// Moves the animation on one frame, if it is playing. 0x52db50
	void NextFrame();
	// The color that multiplies every color in the animation (white to leave them alone). 0x52d890
	void SetAnimColor(const Color& theColor);
	bool IsPlaying() const { return mPlaying; }
	// the path the animation was loaded from (what the scripts' GetPopAnimFile gives), and why a load failed
	const std::string& GetPamFileName() const { return mPamFileName; }
	const std::string& GetError() const { return mError; }

	// Widget overrides
	void Update() override;               // 0x52e960, slot 22
	void UpdateF(float theFrac) override; // 0x52e990, slot 24: theFrac of an update's time has passed
	void Draw(Graphics* g) override;      // 0x52e900, slot 26

protected:
	friend class PopAnimModParser;

	// slot 70, 0x52d090: finds an image beside the .pam (thePamDir is the .pam's folder, thePamFile its path).
	// An empty reference when there is none.
	virtual SharedImageRef LoadImage(const std::string& thePamDir, const std::string& thePamFile,
	                                 const std::string& theImageName);
	// slot 71, 0x52d4c0: called with each image the animation got; empty here
	virtual void ImageLoaded(SharedImageRef theImage);

private:
	void Reset();                                              // 0x52b000: empties the animation
	void CopyFrom(const PopAnim* theOther);                    // 0x52b220
	bool LoadPam(const std::string& thePath);                  // 0x52c280
	bool LoadMod(const std::string& theFileName);              // 0x52cd00
	bool LoadSprite(Buffer& theBuffer, PASpriteDef* theDef);   // 0x52b5d0
	void BuildInstances(PASpriteInst* theInst, PASpriteDef* theDef);  // 0x52c070
	std::string RemapName(const std::string& theName) const;   // 0x52b520
	void SetError(const std::string& theError);                // 0x52b500
	void SendFrameEvents(const PAFrame& theFrame);             // 0x52d8c0
	void AdvanceSprite(PASpriteInst* theInst);                 // 0x52d970
	void ResetSprite(PASpriteInst* theInst);                   // 0x52d730
	void DrawSprite(Graphics* g, PASpriteInst* theInst, const SexyMatrix3* theParentTransform,
	                const Color& theParentColor);              // 0x52e290

	PopAnim(const PopAnim&);
	PopAnim& operator=(const PopAnim&);
};

}  // namespace Sexy
