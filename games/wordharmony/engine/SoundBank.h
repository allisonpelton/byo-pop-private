#ifndef __WORDHARMONY_ENGINE_SOUNDBANK_H__
#define __WORDHARMONY_ENGINE_SOUNDBANK_H__

// The game's sound effects by name: the sound bank (0x14 bytes, constructor 0x426ac0, at the controller's +0x710)
// holds a group of voices for each sound of the sounds file (config/globalSounds.xml), so a sound can play that many
// times at once.

#include "Xml.h"
#include <map>
#include <string>
#include <vector>

namespace engine
{

class Sound;
class SoundSystem;

// The voices of one sound (0x1c bytes, constructor 0x435a10, vtable 0x484838)
class SoundGroup
{
public:
	// 0x435a10: theCount voices, each in theCategory
	SoundGroup(SoundSystem* theSystem, const char* theName, int theCount, int theCategory);
	virtual ~SoundGroup();  // 0x435970

	// 0x04: 0x4358e0, on the first voice not playing; nothing when all play
	virtual void play(float theVolume, float thePitch);
	// 0x435940
	int getPlayingCount();
	// 0x435af0: a voice not playing, or (theMake) a new one
	Sound* getFreeSound(bool theMake);

	std::string mName;            // +0x04: the original keeps the caller's pointer (a temporary's characters)
	std::vector<Sound*> mSounds;  // +0x0c
	SoundSystem* mSystem;         // +0x18
};

class SoundBank
{
public:
	SoundBank();   // 0x426ac0
	~SoundBank() {}  // 0x4269d0: the map goes, its groups are not deleted

	// 0x426760: loads theFile under theName and makes its group, unless the name is there; the bank's volume and
	// pitch become this sound's (bank-wide, as in the original)
	bool add(std::string theName, std::string theFile, int theCount, float theVolume, float thePitch, int theCategory);
	// 0x425e80: null for a name not there
	SoundGroup* getGroup(std::string theName);
	// 0x425f10
	void play(std::string theName, float theVolume, float thePitch);
	// 0x425cf0: reads a sounds file of the archive
	bool load(std::string theFile);

	std::map<std::string, SoundGroup*> mGroups;  // +0x04
	float mVolume;                               // +0x0c
	float mPitch;                                // +0x10
};

// The sounds file's reader (constructor 0x42bdf0, its element 0x42beb0)
class SoundsHandler : public XmlHandler
{
public:
	SoundsHandler() : mBank(nullptr) {}

	void setBank(SoundBank* theBank) { mBank = theBank; }  // 0x42c1f0
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;  // 0x42beb0

	SoundBank* mBank;  // +0x14
};

} // namespace engine

#endif
