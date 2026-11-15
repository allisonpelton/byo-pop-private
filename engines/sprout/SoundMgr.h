#ifndef __SPROUT_SOUNDMGR_H__
#define __SPROUT_SOUNDMGR_H__

// The signal build's sounds by name (0x2c bytes, constructor 0x46f3d0 (pizzafrenzy), vtable 0x502124 (pizzafrenzy); the
// application keeps one at its +0x3c, made and loaded in 0x409220 (pizzafrenzy) from res\manifests\soundManifest.xml).
// The game plays its sounds and music through it by name. It replaces the listener build's SoundBank (SoundBank.h): a
// sound has its own volume and pitch, and a name can also stand for a group of sounds, one drawn at random by weight.
//
// The file's elements: <sound name file count vol pitch category> makes a sound (count voices of its file, each in
// the sound system's volume category); a <randomSound name> element holds <sound name vol pitch weight> elements, each
// naming a sound of the manifest (or another random sound) to play, with its own volume and pitch, by its weight.

#include "Object.h"
#include "Xml.h"

#include <map>
#include <string>
#include <vector>

namespace engine
{

class Sound;
class SoundSystem;

// The voices of one sound (0x24 bytes, constructor 0x490e10 (pizzafrenzy), vtable 0x5046c4 (pizzafrenzy)), so it can
// play that many times at once
class ManifestSound
{
public:
	// 0x490e10 (pizzafrenzy): theCount voices of the loaded sound theName, each in theCategory
	ManifestSound(SoundSystem* theSystem, const std::string& theName, int theCount, float theVolume, float thePitch,
		int theCategory);
	// 0x490b20 (pizzafrenzy): every voice closed (not a slot in the original, which deletes the type it made)
	virtual ~ManifestSound();

	// 0x00: 0x490f00 (pizzafrenzy): the first voice not playing, with the sound's volume and pitch set; with
	// theCreate and every voice playing, a new voice (in category 0) is added and returned; otherwise null
	virtual Sound* getVoice(bool theCreate);
	// 0x04: 0x490ab0 (pizzafrenzy): the first voice not playing starts with the volume and pitch times the sound's;
	// nothing when all play
	virtual void play(float theVolume, float thePitch);

private:
	float mVolume;                // +0x04
	float mPitch;                 // +0x08
	std::string mName;            // +0x0c: the original keeps the caller's pointer (a temporary's characters)
	std::vector<Sound*> mVoices;  // +0x14
	SoundSystem* mSystem;         // +0x20
};

// One sound a random sound may play (0x24 bytes, made in 0x46f2e0 (pizzafrenzy))
struct RandomSoundEntry
{
	std::string mName;  // +0x00: a sound or random sound of the manifest
	float mPitch;       // +0x1c
	float mVolume;      // +0x20
};

// A name that stands for several sounds (0x14 bytes, made in 0x46f2e0 (pizzafrenzy)): each entry with the sum of the
// weights up to and including its own
class RandomSound
{
public:
	RandomSound() : mTotalWeight(0) {}  // 0x46f2e0 (pizzafrenzy)
	~RandomSound();                     // port: the original never frees them

	// 0x46f2e0 (pizzafrenzy), its tail: theEntry (kept) with theWeight added to the total
	void addEntry(RandomSoundEntry* theEntry, float theWeight);
	// 0x46df70 (pizzafrenzy): one entry drawn by weight (one random number, unless there is only one entry); null
	// without entries
	RandomSoundEntry* pick();

private:
	struct Choice
	{
		RandomSoundEntry* mEntry;
		float mCumulativeWeight;
	};
	std::vector<Choice> mChoices;  // +0x04
	float mTotalWeight;            // +0x10
};

class SoundMgr : public Object
{
public:
	SoundMgr() {}          // 0x46f3d0 (pizzafrenzy)
	~SoundMgr() override;  // 0x46f090 (pizzafrenzy), 0x46f4b0 (pizzafrenzy)

	// 0x46ddc0 (pizzafrenzy): reads a manifest file of the archive
	void load(const std::string& theFile);
	// 0x46f1c0 (pizzafrenzy): loads theFile under theName and makes its sound, unless the name is there; false (and a
	// warning in the log) for a file the sound system cannot load
	bool add(const std::string& theName, const std::string& theFile, int theCount, float theVolume, float thePitch,
		int theCategory);
	// 0x46f2e0 (pizzafrenzy): theName with its volume and pitch becomes an entry of the random sound theRandomName
	// (made first when it is not there)
	void addRandom(const std::string& theRandomName, const std::string& theName, float theWeight, float theVolume,
		float thePitch);

	// 0x46e420 (pizzafrenzy): plays the sound of that name with theVolume and thePitch; a random sound plays the sound
	// it draws, with the drawn entry's own volume and pitch instead of these; false for a name that is neither
	bool play(const std::string& theName, float theVolume, float thePitch);
	// 0x46e4c0 (pizzafrenzy): a voice of the sound of that name, not playing, with the sound's volume and pitch set (a
	// random sound's drawn name is looked up the same way, until a sound is found); null for none
	Sound* getVoice(const std::string& theName);

private:
	// 0x46dfe0 (pizzafrenzy): null for a name that is not a sound
	ManifestSound* getSound(const std::string& theName);

	std::map<std::string, ManifestSound*> mSounds;      // +0x0c
	std::map<std::string, RandomSound*> mRandomSounds;  // +0x18
};

// The manifest file's reader (constructor 0x490fd0 (pizzafrenzy), its elements 0x491170 (pizzafrenzy) and
// 0x491120 (pizzafrenzy)); a <sound> inside a <randomSound> is an entry of that random sound
class SoundMgrDocHandler : public XmlHandler
{
public:
	SoundMgrDocHandler() : mManifest(nullptr) {}  // 0x490fd0 (pizzafrenzy)

	void setManifest(SoundMgr* theManifest) { mManifest = theManifest; }  // 0x490fc0 (pizzafrenzy)
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;  // 0x491170 (pizzafrenzy)
	void endElement(const std::string& theName) override;                                         // 0x491120 (pizzafrenzy)

private:
	SoundMgr* mManifest;  // +0x14
	std::string mRandomName;   // +0x18: the random sound being read, "" outside one
};

} // namespace engine

#endif
