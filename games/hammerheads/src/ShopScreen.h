#ifndef __HAMMERHEADS_SHOPSCREEN_H__
#define __HAMMERHEADS_SHOPSCREEN_H__

// The shop (0x45dd20-0x45f580): the screen a shop gnome opens, where the player buys health, hammers and power-ups. Nine
// cubbies on a shelf each send up an item (the items are the creatures of kinds 28 to 38, ShopItem.h; hitting one buys it) and
// a mole behind the counter says what the hammer is over, writes the thanks for a purchase and gives the reasons for a refusal.
// Original: class Sexy::ShopScreen : public Widget (sizeof 0x100, made by 0x41e810).

#include <string>

#include "ButtonListener.h"
#include "CubbyContainer.h"
#include "ShopItem.h"
#include "Widget.h"

namespace Sexy
{

class Image;
class NcButton;
class SoundInstance;

class ShopScreen : public Widget, public ButtonListener, public CubbyContainer
{
public:
	// What the mole is doing (mMood). The codes from REFUSE_NO_COINS (4) to REFUSE_HEALTH_FULL (12) are the reasons for a
	// refusal (ShopItem.h), each with its own line.
	enum
	{
		MOOD_WELCOME = 0,		// the first words; until its time is up
		MOOD_BROWSING = 1,		// waits, looks at what the hammer is over
		MOOD_LOOKING = 2,		// the same, with the mole looking up
		MOOD_THANKS = 3			// writes the thanks for a purchase (the writing sound plays)
	};

	// the nine cubbies are the container's list (the container's part starts at +0x84)
	NcButton*		mBackButton;			// +0x98: the arrow back to the game (button 0)
	NcButton*		mMenuButton;			// +0x9c: the game menu (button 1)
	std::string		mSpeech;				// +0xa0: what the mole says now
	std::string		mThanksLine;			// +0xbc: the line of the purchase just made (the mole's thanks)
	double			mMoodTimer;				// +0xd8: how long the mood lasts (from 4)
	int				mMood;					// +0xe0
	double			mBrowseTimer;			// +0xe8: the time nothing has been under the hammer; at 2 the mole says to browse
	SoundInstance*	mWritingSound;			// +0xf0: the writing sound, while the thanks are written
	double			mFade;					// +0xf8: how far the screen is dimmed behind the game menu (0 to 1)

public:
	ShopScreen();										// 0x45e760
	virtual ~ShopScreen();								// 0x45e1e0

	void			Opened();							// vfd8 (0x45e310; 0x41f130 calls it right after making the screen)
	void			Closing();							// vfdc (0x45ded0; 0x421ca0 calls it before the screen is deleted)

	// vfe0 (0x45ef90): the kind of the item the next cubby sends up. Hearts, the flame and hammers are offered first (a
	// shuffled list of the ones the player can use and that are not on the shelf), then the power-ups the level allows, and
	// when nothing is left, any of the first group.
	int				GetCreatureKind();
	void			ShowRefusal(int theReason);			// 0x45df70: the mole says why not (a ShopRefusal)
	void			ShowThanks(const std::string& theLine);	// 0x45ee20: the mole writes the line

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x45ddf0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x45de20
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x45de50: the buttons stay at the right
	virtual void	Update() override;								// 0x45ddd0
	virtual void	UpdateF(float theFrac) override;				// 0x45e910
	virtual void	Draw(Graphics* g) override;						// 0x45dfb0
	virtual void	MouseEnter() override;							// 0x45dd50
	virtual void	MouseLeave() override;							// 0x45dd90
	virtual void	MouseDown(int x, int y, int theClickCount) override;	// 0x45dd20
	using Widget::MouseDown;
	virtual void	ButtonDepress(int theId) override;				// 0x45df40 (the ButtonListener part)

	virtual Image*	GetBackdrop() override;							// 0x45df30 (the container's slot 0): the shop's backdrop
};

}

#endif
