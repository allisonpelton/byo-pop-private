#ifndef __PIZZAFRENZY_SETTINGS_H__
#define __PIZZAFRENZY_SETTINGS_H__

// The game's settings (0x58 bytes, constructor 0x418bb0, vtable 0x4fa868): the engine's preferences (full screen, music
// and sound) and the one value of the game's own, whether the story shows again (the registry value ReplayStory). The
// application makes it in configure with the string table's publisher and game texts, loads it there and keeps it.

#include "GenericGameOptions.h"

namespace pizzafrenzy
{

class Settings : public engine::GenericGameOptions
{
public:
	// 0x418bb0: the registry key Software\<publisher>\<game>
	Settings(const std::string& thePublisher, const std::string& theGame)
		: GenericGameOptions(thePublisher, theGame), mReplayStory(false)
	{
	}
	~Settings() override {}  // 0x418c40

	// 0x34: 0x418c70: the engine's values, then ReplayStory (1 when it is not there)
	void load() override;
	// 0x38: 0x418d20
	void save() override;

	void setReplayStory(bool theReplay) { mReplayStory = theReplay; }  // 0x3c: 0x418b90
	bool isReplayStory() { return mReplayStory; }                      // 0x40: 0x418ba0

private:
	bool mReplayStory;  // +0x4c
};

} // namespace pizzafrenzy

#endif
