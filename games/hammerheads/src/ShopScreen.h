#ifndef __HAMMERHEADS_SHOPSCREEN_H__
#define __HAMMERHEADS_SHOPSCREEN_H__

// The shop (0x45dd50-0x460c00): the screen a shop gnome opens, where the player buys health, hammers and power-ups.
// Original: class Sexy::ShopScreen : public Widget (sizeof 0x100, made by 0x41e810).

#include <string>

#include "Widget.h"

namespace Sexy
{

class ShopScreen : public Widget
{
public:
	ShopScreen();										// 0x45e760
	virtual ~ShopScreen();

	void			Opened();							// vfd8 (0x41f130 calls it right after making the screen)
	void			Closing();							// vfdc (0x421ca0 calls it before the screen is deleted)

	int				GetCreatureKind();
	void			ShowRefusal(int theReason);
	void			ShowThanks(const std::string& theLine);
};

}

#endif
