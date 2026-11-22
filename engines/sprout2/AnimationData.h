#ifndef __SPROUT2_ANIMATIONDATA_H__
#define __SPROUT2_ANIMATIONDATA_H__

// An animation file's frames and its named runs of them (AnimationData, vtables 0x55bcb8 and 0x55bcd8, its
// constructor 0x416700 (feedingfrenzy2); Talismania's differs: its frame loader, 0x61a1b0 (talismania), logs a Theora
// file it cannot open). An "animation.xml" names the Theora file and each run's first and last frame and rate; the
// frames of the file that no run takes are dropped and the runs after them moved down.

#include "MasterDocHandler.h"
#include "Object.h"
#include "Surface.h"

#include <map>
#include <string>
#include <vector>

namespace engine
{

// One run of frames (0x28 bytes (feedingfrenzy2))
struct AnimationRun
{
	std::string mName;      // +0x00: in lower case
	unsigned int mStart;    // +0x1c: its first frame
	unsigned int mEnd;      // +0x20: one past its last
	float mFrameTime;       // +0x24: the seconds each frame shows, 1 over the rate
};

class AnimationData : public Object
{
public:
	AnimationData() {}

	// 0x04 (0x4841f0 (feedingfrenzy2)): the runs and frames of an XML file of the archives, its root <animations>
	virtual void load(const std::string& thePath);
	// 0x0c (0x432240 (feedingfrenzy2)): a frame added at the end, its anchor at its centre
	virtual void addFrame(Surface* theFrame);
	// 0x10 (0x46f980 (feedingfrenzy2)): a run added under its name in lower case
	virtual void addAnimation(const std::string& theName, unsigned int theStart, unsigned int theEnd,
	                          float theFrameRate);
	// 0x18 (0x431d20 (feedingfrenzy2)): the frames of a Theora file that the runs take, added in order
	virtual void loadFrames(const std::string& thePath);
	// (Slots 0x08, 0x529399, a frame put into a run, and 0x14, 0x532b6f, a run taken out with its frames: called by
	// nothing translated yet.)

	int getFrameCount() const { return (int)mFrames.size(); }
	Surface* getFrame(int theIndex) const { return mFrames[theIndex]; }
	int getAnimationCount() const { return (int)mAnimations.size(); }
	const AnimationRun& getAnimation(int theIndex) const { return mAnimations[theIndex]; }
	// 0x45dca0 (feedingfrenzy2): the run's index under the name in lower case, -1 for none
	int findAnimation(const std::string& theName) const;

protected:
	std::vector<AnimationRun> mAnimations;      // +0x0c
	std::map<std::string, int> mAnimationIndex;  // +0x1c: each run's index by its name
	std::vector<Ref<Surface>> mFrames;          // +0x28
	// +0x38, a string: its writer not yet found
};

// The <animations> document (AnimationData::DocHandler, vtables 0x55bce8 and 0x55bcf4, 0x78 bytes (feedingfrenzy2))
class AnimationDataDocHandler : public SubDocHandler
{
public:
	explicit AnimationDataDocHandler(AnimationData* theData) : mData(theData) {}

	// 0x04 of its own table (0x4801d0 (feedingfrenzy2)): the root's fileName
	void begin(Params& theAttributes) override;
	// 0x00 (0x431f90 (feedingfrenzy2)): an <animation> with its name, startFrame, endFrame (the last frame shown) and
	// frameRate
	void startElement(const std::string& theName, Params& theAttributes) override;
	// 0x0c (0x412d90 (feedingfrenzy2)): the frames loaded from the file named, if one is
	void endDocument() override;

private:
	std::string mFileName;
	AnimationData* mData;
};

} // namespace engine

#endif
