#ifndef __SPROUT_BLTFX_H__
#define __SPROUT_BLTFX_H__

// The signal build's blit effect (Iggle Pop's engine::BltFx, which WaveFx derives from): a counted reference a node
// keeps at +0x68 (0x467f90 (pizzafrenzy)) and applies to the render state at +0x2c (0x478060 (pizzafrenzy)). The draw
// context hands it to the blitters, which choose another loop for the effect whose type (+0x0c, 0x411960 (pizzafrenzy))
// is 1. Nothing in Pizza Frenzy's exe makes one: its only caller of the node's setter is the screen's own copy of
// the slot, so the field is null in every run and the host's drawing has no effect to apply.

#include "Object.h"

namespace engine
{

class BltFx : public Object
{
public:
	int mType = 0;  // +0x0c
};

} // namespace engine

#endif
