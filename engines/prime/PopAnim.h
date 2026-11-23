// Sexy::PopAnim and Sexy::PopAnimListener as Bejeweled 3's "prime" framework has them (vtable 0x8704c4 (bejeweled3),
// 0xc00 bytes, code 0x440750 (bejeweled3)-0x44b0b0 (bejeweled3)): PopCap's player for .pam animations (versions 4 and
// 5, which this reads, and the older ones), a Widget that loads an animation, plays it a frame at a time and draws it.
// Nothing here is specific to one game; the game's own part is the listener.
//
// A .pam file holds the animation's images, which it names (the files are found beside it, or by the app's resource
// manager when the name has a "|" and an id in it), and a set of sprites. A sprite is a list of frames; each frame says
// what is on screen (an image or another sprite, with a transform, a color and optionally part of the image), whether
// the animation stops there, and what commands go to the game. The main sprite is the animation itself. Unlike the
// older player's, this one counts frames in floats and gives every sprite its own frame rate, a repeat count and a
// delay, lets an object run its sprite slower or faster (mTimeScale) and play ahead (mPreloadFrames), draws between two
// frames (mInterpolate), puts particle effects on a sprite and has the listener look at every object it draws.
//
// Not translated: the mirrored drawing (DrawSpriteMirrored 0x448da0 (bejeweled3); mMirror is set by nothing in the
// game), saving and loading the animation's state (0x445540 (bejeweled3), 0x4453b0 (bejeweled3), 0x445760 (bejeweled3),
// 0x445a00 (bejeweled3), used by saved games only), the debugging outlines (+0xbfb), and the particle effect formats
// the exe reads besides .ppf (the ".ip3" names FrameHit tries, which PIEffect::Load does not read, so those tries fail).
//
// The older player, Bejeweled Twist's copy (games/bejeweledtwist/popanim), is the same idea in a simpler form.
#pragma once

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "Color.h"
#include "MTRand.h"
#include "PopAnimData.h"
#include "SexyMatrix.h"
#include "Widget.h"

namespace Sexy {

class Buffer;
class Graphics;
class PIEffect;
class PopAnimModParser;

// What the game hears from an animation and may decide while it draws: seven virtual functions of the exe's
// listener class (the names are this port's). The slot is the one in the exe's vtable, in bytes.
class PopAnimListener {
public:
	virtual ~PopAnimListener() {}
	// slot 0: the "playsample" command (the sample, how loud and where between the speakers; the exe's arguments
	// are not all visible in its code, these are what the command's text gives)
	virtual void PopAnimPlaySample(int theId, const std::string& theSample, double theVolume, double thePan) {}
	// slot 4: the game's own particle effect for an "addparticleeffect" command; null to let the animation make one
	virtual PIEffect* PopAnimCreateEffect(int theId, const std::string& theName) { return nullptr; }
	// slot 8: asked before each object of a sprite is drawn, while its slot's mWantsPreDraw is set; the answer is
	// the new mWantsPreDraw
	virtual bool PopAnimPreDraw(int theId, Graphics* g, PASpriteInst* theInst, PASlot* theSlot,
	                            const SexyMatrix3* theParentTransform, const Color* theParentColor) { return true; }
	// slot 0xc: asked after each object, with mWantsPostDraw
	virtual bool PopAnimPostDraw(int theId, Graphics* g, PASpriteInst* theInst, PASlot* theSlot) { return true; }
	// slot 0x10: asked before an object's image is drawn, with the matrix it will be drawn with (the listener may
	// change it): 0 to stop being asked (and draw), 2 to skip the drawing, 3 to draw it again, else draw it
	virtual int PopAnimDrawImage(int theId, PASpriteInst* theInst, PASlot* theSlot, SexyMatrix3* theMatrix,
	                             Image* theImage) { return 1; }
	// slot 0x14: a frame that stops the animation was reached
	virtual void PopAnimStopped(int theId) {}
	// slot 0x18: a frame with commands was reached, once per command, in the file's order; true when the game
	// dealt with it (and the animation's own commands, "delay", "playsample" and "addparticleeffect", do not run)
	virtual bool PopAnimCommand(int theId, PASpriteInst* theInst, const std::string& theCommand,
	                            const std::string& theParam) { return false; }
};

class PopAnim : public Widget {
public:
	int mId;                      // +0xbc, what the listener is told the animation is
	PopAnimListener* mListener;   // +0xc0
	// +0xd0: the folders searched for an image, each relative to the folder of the .pam
	std::vector<std::string> mImageFolders;
	int mVersion;                 // +0xdc, the .pam's (0 to 5)
	std::list<std::pair<std::string, std::string> > mRemaps;  // +0xe4, a mod file's Remap commands
	// +0x108: the whole animation is drawn at this scale (the game's art size over the manifest's base)
	float mDrawScale;
	// +0x10c: the size the pictures were made at, as a fraction of the base: each picture is drawn at 1 / this
	float mImageScale;
	bool mLoaded;                 // +0x110
	std::string mPamFile;         // +0x118, the .pam a mod file names (SetPamFile)
	std::string mPamFileName;     // +0x134, the path it was loaded from
	int mFrameRate;               // +0x16c, the file's own (the sprites have theirs)
	int mPamX, mPamY, mPamWidth, mPamHeight;  // +0x170, the animation's bounds in pixels
	std::string mError;           // +0x180, why the last load failed
	std::string mLabel;           // +0x19c, the label last played
	std::vector<PAImage> mImages; // +0x1b8
	PASpriteInst* mMainInst;      // +0x1d0
	PAData* mData;                // +0x1d4, shared with the clones (counted in mData's count, +0x38 in the exe)
	int mDataUsers;               // port: how many PopAnims share mData
	float mTransitionTime;        // +0x1d8, how long the transition TransitionToLabel started lasts (0: none), in updates
	float mTransitionElapsed;     // +0x1dc
	float mStartDelay;            // +0x1e0, time before the animation starts moving (nothing sets it)
	bool mRandSeeded;             // +0xba8: mRand has been seeded (by the first effect made)
	MTRand mRand;                 // +0x1e4: seeds the effects the animation makes
	// +0xbb0, +0xbb8: where the effects that are attached to an emitter or follow a sprite are put off from their
	// place; nothing in the exe sets them, so they stay 0
	double mEffectOffsetX, mEffectOffsetY;
	SexyMatrix3 mTransform;       // +0xbc0, where the animation is, put on every object (identity unless the game moves it)
	Color mAnimColor;             // +0xbe4, multiplies every color in the animation
	bool mDrawAdditive;           // +0xbf4, the whole animation is drawn with the additive blend
	bool mTransformsStale;        // +0xbf5, the objects' transforms must be worked out again before drawing
	bool mPlaying;                // +0xbf6
	bool mMirror;                 // +0xbf7, nothing sets it in the exe; its mirrored drawing is not translated
	bool mInterpolate;            // +0xbf8, draw between frames (the default); false steps from frame to frame
	bool mImageIsNew;             // +0xbf9, whether the last image looked up had to be loaded
	bool mPaused;                 // +0xbfa, UpdateF does nothing while it is set

	PopAnim(int theId, PopAnimListener* theListener);  // 0x440750 (bejeweled3)
	~PopAnim() override;                               // 0x4410a0 (bejeweled3)

	// Loads a .pam, or a .txt mod file (the first is the exe's own extension test: the part of the name from its
	// first dot). False when it cannot (mError says why). Replaces what the animation held. 0x4440a0 (bejeweled3)
	bool LoadAnimation(const std::string& theFileName);
	// A copy that plays on its own: the same pictures and definitions, its own place in the animation (at the
	// first frame), the same state otherwise. 0x440a30 (bejeweled3)
	PopAnim* Clone() const;

	// Starts playing at a frame; false when there is none (and the animation stops). theReset also takes the
	// particle effects off and runs the animation up to its work range. 0x444680 (bejeweled3)
	bool PlayFrame(int theFrame, bool theReset);
	// Starts playing at a label of the main sprite (the first frame of the work range for an empty label); false
	// when there is none (the animation stops). 0x444800 (bejeweled3)
	bool PlayLabel(const std::string& theLabel, bool theReset);
	// PlayLabel with a transition: the objects the old animation and the new one both have (by name) move and fade
	// from where they were over theTransitionTime updates. Nothing that predates it calls it: PlayLabel and PlayFrame
	// behave as before. 0x444910 (bejeweled3)
	bool TransitionToLabel(const std::string& theLabel, int theTransitionTime);
	// The frame of a label of the main sprite, or -1. 0x43f230 (bejeweled3)
	int GetLabelFrame(const std::string& theLabel) const;
	// Whether the animation is playing, or an effect of it still runs. 0x445390 (bejeweled3)
	bool IsActive() const;
	// The color that multiplies every color in the animation (white to leave them alone)
	void SetAnimColor(const Color& theColor) { mAnimColor = theColor; }
	bool IsPlaying() const { return mPlaying; }
	const std::string& GetPamFileName() const { return mPamFileName; }
	const std::string& GetError() const { return mError; }

	// Widget overrides
	void Update() override;                // 0x44a2d0 (bejeweled3), slot 22
	void UpdateF(float theFrac) override;  // 0x44a3f0 (bejeweled3), slot 24: theFrac of an update's time has passed
	void Draw(Graphics* g) override;       // 0x44a180 (bejeweled3), slot 26

	// slot 81, 0x449600 (bejeweled3): draws a sprite instance (the main one from Draw, the others from itself). The parent's
	// transform is null for the main sprite, theAdditive says that everything below is drawn additively.
	virtual void DrawSprite(Graphics* g, PASpriteInst* theInst, const SexyMatrix3* theParentTransform,
	                        const Color* theParentColor, bool theAdditive, bool theForceStill);

protected:
	friend class PopAnimModParser;

	// slot 88, 0x443ac0 (bejeweled3): finds an image beside the .pam (thePamDir is the .pam's folder, thePamFile its path), by
	// the app's shared images. An empty reference when there is none, with mError set for a missing name.
	virtual SharedImageRef GetImageHook(const std::string& thePamDir, const std::string& thePamFile,
	                                    const std::string& theImageName);
	// slot 90: called with each image the animation got; empty here (a bare return in the exe)
	virtual void ImageLoaded(PAImage& theImage);

private:
	void Reset();                                              // 0x441270 (bejeweled3), with 0x441370 (bejeweled3): empties the animation
	void CopyFrom(const PopAnim* theOther);                    // 0x440bf0 (bejeweled3) and 0x440f10 (bejeweled3), 0x441020 (bejeweled3)
	bool LoadPam(const std::string& thePath);                  // 0x442590 (bejeweled3), slot 86
	bool LoadMod(const std::string& theFileName);              // 0x443770 (bejeweled3), slot 87
	bool LoadSprite(Buffer& theBuffer, PASpriteDef* theDef);   // 0x441500 (bejeweled3)
	bool GetImage(const std::string& thePamDir, const std::string& thePamFile, PAImage* theEntry,
	              const std::string& theImageName);            // 0x443f30 (bejeweled3), slot 89
	void BuildInstance(PASpriteInst* theInst, PASpriteDef* theDef);  // 0x4422b0 (bejeweled3)
	bool SetupMainSprite(const std::string& theName);          // 0x44a410 (bejeweled3)
	std::string RemapName(const std::string& theName) const;   // 0x441410 (bejeweled3)
	void SetError(const std::string& theError);                // 0x4413f0 (bejeweled3)

	void UpdateAnimation(float theFrac);                       // 0x448cc0 (bejeweled3)
	void AdvanceSprite(PASpriteInst* theInst, PAObjectPos* theObject, float theFrac);  // 0x448830 (bejeweled3)
	void FrameEvents(PASpriteInst* theInst, PAObjectPos* theObject);                   // 0x448ae0 (bejeweled3)
	void FrameHitSprites(PASpriteInst* theInst, PAObjectPos* theObject);               // 0x447640 (bejeweled3)
	void FrameHit(PASpriteInst* theInst, PAFrame& theFrame, PAObjectPos* theObject);   // 0x445c00 (bejeweled3)
	void BlendObject(PASpriteInst* theInst, int theObjectIndex, bool theForceStill, SexyMatrix3* theMatrix,
	                 Color* theColor) const;                   // 0x447770 (bejeweled3)
	void UpdateTransforms(PASpriteInst* theInst, const SexyMatrix3* theParentTransform, const Color* theParentColor,
	                      bool theForceStill);                 // 0x447f70 (bejeweled3)
	void UpdateEffects(PASpriteInst* theInst, PAObjectPos* theObject);                 // 0x4482a0 (bejeweled3)
	void RemoveStaleEffects(PASpriteInst* theInst, bool theAll);                       // 0x4485e0 (bejeweled3)
	void ResetSprite(PASpriteInst* theInst);                   // 0x444310 (bejeweled3)
	void DrawEffects(Graphics* g, PASpriteInst* theInst, const SexyMatrix3* theParentTransform,
	                 const Color* theParentColor, bool theInFront);   // 0x444410 (bejeweled3)
	// port: the part of FrameHit (0x445c00 (bejeweled3)) that makes the effect an "addparticleeffect" command names
	// when the listener does not: the resource manager's (duplicated), or a file. Null when there is none
	PIEffect* MakeEffect(const std::string& theEffectName);
	// port: the part of FrameHit that seeds a new effect, runs it ahead and puts it on the sprite
	void AddEffect(PASpriteInst* theInst, PIEffect* theEffect, const PAEffectLayer& theLayer, PAObjectPos* theObject);
	void ResetPlayback();                                      // 0x4445a0 (bejeweled3)
	void RunToWorkRange();                                     // 0x4445f0 (bejeweled3)

	PopAnim(const PopAnim&);
	PopAnim& operator=(const PopAnim&);
};

}  // namespace Sexy
