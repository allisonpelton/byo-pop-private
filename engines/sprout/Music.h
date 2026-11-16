#ifndef __SPROUT_MUSIC_H__
#define __SPROUT_MUSIC_H__

// The music: a music system runs layers, and each layer plays one clip at a time from its list, picked at random, in
// order or as chosen, with a pause between clips, a start clip, an ending clip and a bridge played between two clips.
// The clips are voices of sounds the sound system streams from their files. The music file (config/music.xml and the
// like) lists the layers and their clips.

#include "Object.h"
#include "Geometry.h"
#include "Xml.h"
#include <string>
#include <vector>

namespace engine
{

class Sound;
struct UpdateInfo;

// 0x424160 (wordharmony): streams thePath under theName and makes a voice of it (createSound(name, 0, 1)); null, with a
// log line, when either fails
Sound* registerClip(const std::string& thePath, const std::string& theName);

// How a layer picks its next clip (+0x1c)
enum
{
	MUSIC_RANDOM = 0,
	MUSIC_RANDOM_NOT_LAST = 1,
	MUSIC_SEQUENTIAL = 2,
	MUSIC_LOOP = 3  // the clip loops until another is chosen
};

// How a layer stops (+0x54)
enum
{
	MUSIC_STOP_IMMEDIATE = 0,
	MUSIC_STOP_FADE_OUT = 1,
	MUSIC_STOP_PLAY_FINAL = 2  // stops the clip and plays the ending clip once
};

// A layer (0x84 bytes, constructor 0x424810 (wordharmony), vtable 0x483138 (wordharmony))
class MusicLayer : public Object
{
public:
	MusicLayer();            // 0x424810 (wordharmony)
	~MusicLayer() override;  // 0x424220 (wordharmony): closes the clips of its list

	// 0x4245f0 (wordharmony): the next clip by the sequence, into mCurrent (null for an empty list); a bridge plays
	// when the index changes
	Sound* pickClip();
	// 0x424740 (wordharmony): (re)starts the current clip and finds its index in the list
	void playCurrent();
	// 0x4247c0 (wordharmony): the clip to play next; theNow cuts the current one and plays it at once
	void select(int theIndex, bool theNow);
	// 0x424910 (wordharmony): after the initial delay, the start clip (or a picked one) fades in
	void start();
	// 0x424970 (wordharmony)
	void stop();
	// 0x459140 (feedingfrenzy): the layer's volume, and the clip that plays now takes it
	void setVolume(float theVolume);
	// 0x424a10 (wordharmony)
	void update(UpdateInfo& theInfo);
	// 0x424b80 (wordharmony): registers a clip and adds it to the list, with no loop, no fades and the layer's volume
	Sound* addClip(const std::string& thePath, const std::string& theName);

	std::vector<Sound*> mClips;  // +0x10
	int mSequence;               // +0x1c
	Point mDelay;                // +0x20: the pause between clips (a range)
	Point mInitialDelay;         // +0x28: the pause before the first (a range)
	float mFadeIn;               // +0x30
	float mFadeOut;              // +0x34
	float mVolume;               // +0x38: the constructor leaves it; the file's reader always sets it
	float mTimeLeft;             // +0x3c
	Sound* mStartClip;           // +0x40
	Sound* mFinalClip;           // +0x44
	Sound* mBridgeClip;          // +0x48
	Sound* mCurrent;             // +0x4c
	int mIndex;                  // +0x50
	int mStopMode;               // +0x54
	bool mRunning;               // +0x58
	int mChosen;                 // +0x5c: -1 when none
	std::string mName;           // +0x60
};

// The music system (0x20 bytes, constructor 0x425150 (wordharmony), vtable 0x48315c (wordharmony))
class MusicSystem : public Object
{
public:
	MusicSystem() {}           // 0x425150 (wordharmony)
	~MusicSystem() override;   // 0x424db0 (wordharmony): deletes its layers

	void update(UpdateInfo& theInfo);       // 0x424be0 (wordharmony)
	void start();                           // 0x424c10 (wordharmony)
	void stop();                            // 0x424c40 (wordharmony)
	void select(int theIndex, bool theNow); // 0x424c70 (wordharmony)
	// 0x424ca0 (wordharmony): deletes the layers there are and reads a music file's
	bool load(std::string theFile);
	void addLayer(MusicLayer* theLayer);    // 0x4251e0 (wordharmony)
	void clearLayers();                     // 0x424e90 (wordharmony)
	// 0x44d940 (feedingfrenzy): every layer's volume
	void setVolume(float theVolume);

	std::vector<MusicLayer*> mLayers;  // +0x10
};

// The music file's reader (0x20 bytes, constructor 0x4252f0 (wordharmony), vtable 0x483178 (wordharmony))
class MusicHandler : public XmlHandler
{
public:
	MusicHandler() : mLayer(nullptr), mSystem(nullptr) {}  // 0x4252f0 (wordharmony)

	// 0x425230 (wordharmony): also deletes the system's layers
	void setSystem(MusicSystem* theSystem);

	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;  // 0x4259a0 (wordharmony)
	void endElement(const std::string& theName) override;  // 0x4253c0 (wordharmony): a layer read goes to the system

private:
	void startLayer(const XmlAttributes& theAttributes);  // 0x20: 0x425400 (wordharmony)

	MusicLayer* mLayer;    // +0x14: the layer being read
	MusicSystem* mSystem;  // +0x18
};

} // namespace engine

#endif
