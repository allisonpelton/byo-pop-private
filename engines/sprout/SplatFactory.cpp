#include "SplatFactory.h"

using namespace engine;

// 0x474f40 (pizzafrenzy)
SplatFactory::SplatFactory()
{
}

// 0x473aa0 (pizzafrenzy)
void SplatFactory::layout()
{
	mRect = Rect();
	setFlags(NODE_LAYOUT);
}
