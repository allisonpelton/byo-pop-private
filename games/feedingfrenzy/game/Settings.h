#ifndef __FEEDINGFRENZY_SETTINGS_H__
#define __FEEDINGFRENZY_SETTINGS_H__

// The game's settings (the registry key Software\GameHouse\FeedingFrenzy; constructor 0x433870, load 0x4339c0, save
// 0x4332c0, the object 0x68 bytes in the controller at +0xd4): the engine's preferences (full screen, music and sound)
// and the game's own: whether the tool tips show, the name a high score is entered under first, and the mouse
// sensitivity. The original keeps four more values the port leaves out: the link under the main menu (ShowLink,
// VisitURL and ILink: an outbound button) and the screen saver's selection (SaverSelection).

#include "GenericGameOptions.h"
#include "ScreenLayout.h"

#include <string>

namespace feedingfrenzy
{

class Settings : public engine::GenericGameOptions
{
public:
	Settings();  // 0x433870

	void load() override;  // 0x4339c0
	void save() override;  // 0x4332c0

	bool areTipsEnabled() const { return mTipsEnabled; }                                   // 0x433190 (+0x03)
	void setTipsEnabled(bool theEnabled) { mTipsEnabled = theEnabled; }                    // 0x4331b0
	const std::string& getDefaultScoreName() const { return mDefaultScoreName; }           // +0x04
	void setDefaultScoreName(const std::string& theName) { mDefaultScoreName = theName; }  // 0x433960
	int getMouseSensitivity() const { return mMouseSensitivity; }                          // 0x433210 (+0x20)
	void setMouseSensitivity(int theSensitivity) { mMouseSensitivity = theSensitivity; }   // 0x433170
	// 0x4331c0: how much faster the player's fish swims for the sensitivity (0.8, 1.1, 1.3 and 1.55); a value out of range
	// is put back to 1
	float getMouseSpeedFactor();

	// 0x433540: the options screen shows the settings: the boxes checked (the full screen as the display has it), the
	// mouse sensitivity's button, the game's version; without a sound system the sound and music are off, cannot be
	// turned on, and the text that says why shows
	void updateScreen(engine::ScreenLayout* theScreen, const std::string& theVersion);

private:
	bool mTipsEnabled;              // +0x03
	std::string mDefaultScoreName;  // +0x04
	int mMouseSensitivity;          // +0x20
};

} // namespace feedingfrenzy

#endif
