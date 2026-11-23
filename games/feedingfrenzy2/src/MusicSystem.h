#ifndef __FEEDINGFRENZY2_MUSICSYSTEM_H__
#define __FEEDINGFRENZY2_MUSICSYSTEM_H__

// The music system: layers of clips, each layer playing one clip at a time after a drawn delay, in its own order. A
// stage's ambient sounds are one (its ambientSound file, the stage's +0x318). The clips are the sound system's
// handles, each of a stream registered under its clip's name.

#include "DocHandler.h"
#include "Geometry.h"
#include "Object.h"

#include <string>
#include <vector>

namespace engine
{
struct UpdateContext;
}

namespace simplesound
{
class SoundHandle;
}

// The order a layer plays its clips in ("sequence")
enum MusicSequence
{
	MUSIC_SEQUENCE_RANDOM = 0,           // "random": any
	MUSIC_SEQUENCE_RANDOM_NOT_LAST = 1,  // "random-not-last", the default: any but the one before
	MUSIC_SEQUENCE_SEQUENTIAL = 2,       // "sequential": the next, from the last to the first
	MUSIC_SEQUENCE_LOOP = 3              // "loop": the one looping, until another is asked for
};

// What a layer does with its clip when it is stopped ("immediate")
enum MusicStopMode
{
	MUSIC_STOP_AT_ONCE = 0,     // "stop", the default
	MUSIC_STOP_FADE_OUT = 1,    // "fadeOut": over the layer's fade-out time
	MUSIC_STOP_PLAY_FINAL = 2   // "playFinal": the final clip played on, once
};

// A layer (MusicLayer, 0x84 bytes, vtable 0x55f31c (feedingfrenzy2); its constructor 0x40c360 (feedingfrenzy2))
class MusicLayer : public engine::Object
{
public:
	MusicLayer();
	// 0x482a20 (feedingfrenzy2): each clip of the list let go of (its slot 0x10)
	~MusicLayer() override;

	// 0x4142c0 (feedingfrenzy2): a clip of the stream registered under the name added to the list: not looping, no
	// fades, at the layer's volume; null when there is no such stream
	simplesound::SoundHandle* addClip(const std::string& theName, const std::string& thePath, int theCategory);
	// 0x486860 (feedingfrenzy2): the layer started: its first delay drawn, and its first clip (the start clip, or
	// one chosen) faded in at once when the delay is none
	void start();
	// 0x4868c0 (feedingfrenzy2): the layer stopped, its clip by the stop mode
	void stop();
	// 0x439ac0 (feedingfrenzy2), each frame: the delay counted down, and the next clip chosen and played when it runs
	// out, or when the clip ends and the delay drawn after it is none
	void update(engine::UpdateContext* theContext);
	// 0x48cbe0 (feedingfrenzy2): the volume, the playing clip's too (the others keep theirs)
	void setVolume(float theVolume);

	MusicSequence mSequence;              // +0x1c
	engine::FloatRange mDelay;            // +0x20: "delay", the seconds between clips
	engine::FloatRange mInitialDelay;     // +0x28: "initialDelay", the seconds before the first
	float mFadeInTime;                    // +0x30: "fadeInTime"
	float mFadeOutTime;                   // +0x34: "fadeOutTime"
	float mVolume;                        // +0x38: "volume"
	simplesound::SoundHandle* mStartClip;   // +0x40: the clip played first ("start" or "startOnly")
	simplesound::SoundHandle* mFinalClip;   // +0x44: the clip played on at a stop ("final" or "finalOnly")
	simplesound::SoundHandle* mBridgeClip;  // +0x48: played once between two clips ("bridge")
	MusicStopMode mStopMode;              // +0x54
	std::string mName;                    // +0x60: "name"

private:
	// 0x41ce70 (feedingfrenzy2): the next clip chosen by the sequence (the bridge played when it is another), null
	// when there are none
	simplesound::SoundHandle* chooseClip();
	// 0x41e3a0 (feedingfrenzy2): the chosen clip played from its start (looping in a loop layer), and its place in
	// the list kept
	void playClip();

	std::vector<simplesound::SoundHandle*> mClips;  // +0x0c
	float mDelayLeft;                     // +0x3c
	simplesound::SoundHandle* mClip;      // +0x4c: the clip chosen
	int mClipIndex;                       // +0x50: its place in the list
	bool mStarted;                        // +0x58
	int mRequestedIndex;                  // +0x5c: the clip a loop layer is to go to next, -1 for none
};

// The music system (MusicSystem, 0x24 bytes, vtable 0x55d228 (feedingfrenzy2); its constructor 0x4953e0
// (feedingfrenzy2))
class MusicSystem : public engine::Object
{
public:
	// 0x48f9a0 (feedingfrenzy2): the layers let go of, and the file read into new ones
	void load(const std::string& thePath);
	// 0x486650 (feedingfrenzy2)
	void addLayer(MusicLayer* theLayer);
	// 0x486810 (feedingfrenzy2): each layer started
	void start();
	// 0x4866a0 (feedingfrenzy2): each layer stopped
	void stop();
	// 0x439b80 (feedingfrenzy2), the stage's slot 0x6c: each layer's update
	void update(engine::UpdateContext* theContext);
	// 0x486700 (feedingfrenzy2): each layer's volume
	void setVolume(float theVolume);

private:
	// 0x48fa10 (feedingfrenzy2)
	void clear();

	std::vector<engine::Ref<MusicLayer>> mLayers;  // +0x0c
};

// The reader of a music file (MusicSystemDocHandler, vtable 0x55f250 (feedingfrenzy2); its constructor 0x495440
// (feedingfrenzy2)): <musicSystem category>, then each <musicLayer>'s attributes and its <clip>s
class MusicSystemDocHandler : public engine::AbstractDocHandler
{
public:
	explicit MusicSystemDocHandler(MusicSystem* theSystem);

	// 0x00 of its +0x0c table (0x426470 (feedingfrenzy2)): the category, a layer begun, or a clip added to it (and
	// made its start, final or bridge clip by its "special")
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 of its +0x0c table (0x415de0 (feedingfrenzy2)): a layer ended, added to the system
	void endElement(const std::string& theName) override;

private:
	// 0x04 (0x425bb0 (feedingfrenzy2)): a new layer from the attributes
	void beginLayer(engine::Params& theAttributes);

	MusicSystem* mSystem;
	engine::Ref<MusicLayer> mLayer;  // +0x30: the layer being read
	int mCategory;                   // +0x38: "category", the sound system's category of every clip
};

#endif
