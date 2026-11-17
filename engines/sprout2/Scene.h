#ifndef __SPROUT2_SCENE_H__
#define __SPROUT2_SCENE_H__

// The root of what is drawn (engine::Scene, 0xfc bytes, vtable 0x560734 (feedingfrenzy2); its constructor 0x498260
// (feedingfrenzy2)): a container that, enabled, takes the engine's frame and input signals, updates its tree, draws
// it, and passes the mouse to the components under it. The mouse and the activation are not yet translated: enable
// connects the update and the draw only.

#include "Container.h"
#include "sigslot.h"

namespace engine
{

class Scene : public Container, public sigslot::HasSlots
{
public:
	Scene();

	// 0x68 (0x51fe7a (feedingfrenzy2))
	std::string getClassName() override { return "Scene"; }
	// 0x90 (0x496df0 (feedingfrenzy2)): the engine's update (+0x04), draw (+0x14), activate (+0x24) and mouse signals
	// (+0x74 to +0xc4) connected; the scene's children are not enabled
	void enable() override;
	// 0x94 (0x53dd4c (feedingfrenzy2)): the children disabled, then the signals disconnected
	void disable() override;
	// 0xa4 (0x437580 (feedingfrenzy2)): the tree updated, then the bounds recomputed
	void update(UpdateContext& theContext) override;

	// The component given every mouse event until it lets go (a pressed button), null for none (inlined where the
	// original sets it, and 0x508344 (feedingfrenzy2))
	void setMouseCapture(Component* theComponent) { mMouseCapture = theComponent; }

private:
	// 0x4c1e40 and 0x4c1e60 (feedingfrenzy2): the signals passed to slots 0xa4 and 0xa0
	void updateSlot(UpdateContext* theContext) { update(*theContext); }
	void drawSlot(DrawContext* theContext) { paint(theContext); }

	// +0xc8 (a flag, set), +0xcc and +0xd4 (a reference), +0xdc (the mouse event): the mouse's, not yet translated
	Ref<Component> mMouseCapture;  // +0xd8
};

} // namespace engine

#endif
