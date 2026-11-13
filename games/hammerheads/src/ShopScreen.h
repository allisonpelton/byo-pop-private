#ifndef __HAMMERHEADS_SHOPSCREEN_H__
#define __HAMMERHEADS_SHOPSCREEN_H__

#include <string>

#include "Widget.h"

namespace Sexy
{

class ShopScreen : public Widget
{
public:
	int				GetCreatureKind();
	void			ShowRefusal(int theReason);
	void			ShowThanks(const std::string& theLine);
};

}

#endif
