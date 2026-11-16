#ifndef __SPROUT2_PIXELFX_H__
#define __SPROUT2_PIXELFX_H__

// An effect computed per pixel as a component's images are drawn (engine::PixelFx, vtable 0x55c0dc (feedingfrenzy2):
// ColorFx, RainbowShiftFx and HSVShiftFx), set on a component (Component::setPixelFx) and animated by its update.

#include "Object.h"

namespace engine
{

class PixelFx : public virtual Interface
{
public:
	// 0x04: the effect advanced by the frame's time
	virtual void update(float theElapsed) = 0;
};

} // namespace engine

#endif
