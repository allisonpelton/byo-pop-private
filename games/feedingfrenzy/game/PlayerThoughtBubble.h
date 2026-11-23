#ifndef __FEEDINGFRENZY_PLAYERTHOUGHTBUBBLE_H__
#define __FEEDINGFRENZY_PLAYERTHOUGHTBUBBLE_H__

// The thought bubble above the player's fish (PlayerThoughtBubble, 0x164 bytes, constructor 0x40bc00, vtable 0x4a0ba0):
// while the tips are on it shows the picture of the biggest kind of fish the player can eat that it has not eaten yet,
// for a few seconds each time.

#include "FadeContainer.h"
#include "ImageNode.h"

namespace feedingfrenzy
{

class FishSpawner;
class PlayerFish;

class PlayerThoughtBubble : public engine::FadeContainer
{
public:
	PlayerThoughtBubble();  // 0x40bc00

	// 0x40bd30: the bubble's pictures; it belongs to this fish
	void init(PlayerFish* thePlayer);
	// 0x40bad0: the bubble forgets the kind it showed and the ones it has been through
	void reset();
	// 0x40bb10: the bubble starts (and picks a kind at once when the second argument says so), or fades out
	void show(bool theShow, bool theForce);
	// 0x40c370
	void update(engine::UpdateInfo& theInfo) override;

private:
	// 0x40c240: the bubble is to show the next kind, or to go
	void pickKind();
	// 0x40bf00: the bubble shows the kind
	void showKind(FishSpawner* theSpawner);

	PlayerFish* mPlayer;                      // +0x138
	engine::Ref<engine::ImageNode> mRedBubble;    // +0x13c
	engine::Ref<engine::ImageNode> mGreenBubble;  // +0x140
	engine::Ref<engine::ImageNode> mFishImage;    // +0x144: the picture of the kind shown
	float mSecondsLeft;                       // +0x148: the time the kind shows for, -1 for none
	FishSpawner* mSpawner;                    // +0x14c: the kind shown
	bool mShown;                              // +0x154: the bubble is on
	bool mForce;                              // +0x155: a kind is shown even if it has been eaten
	bool mAllShown;                           // +0x158: every kind has been through
};

} // namespace feedingfrenzy

#endif
