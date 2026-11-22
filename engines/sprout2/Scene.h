#ifndef __SPROUT2_SCENE_H__
#define __SPROUT2_SCENE_H__

// The root of what is drawn (engine::Scene, 0xfc bytes, vtable 0x560734 (feedingfrenzy2); its constructor 0x498260
// (feedingfrenzy2)): a container that, enabled, takes the engine's frame and input signals, updates its tree, draws
// it, and passes the mouse to the components under it. The component under the mouse that takes input is told when
// the mouse comes onto it and leaves it; a component may capture the mouse (a pressed button), and then has every
// event, in its own space, until it lets go. While the window is inactive the mouse goes nowhere.

#include "Container.h"
#include "sigslot.h"

namespace engine
{

class Scene : public Container, public sigslot::HasSlots
{
public:
	// 0x498260 (feedingfrenzy2): active, nothing under the mouse
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
	// 0x495860 (feedingfrenzy2): going inactive cancels the capture's press, or the hovered component's hover; coming
	// back the mouse is looked at again (while still inactive, so nothing is hit)
	void activateSlot(bool theActive);
	// 0x40dcb0, 0x40e220, 0x542097, 0x54206a (feedingfrenzy2): the buttons' bits set or cleared, then the event
	// passed to the target's mouseDown, mouseUp, rightDown or rightUp
	void mouseDownSlot(const Vector2<int>& thePosition);
	void mouseUpSlot(const Vector2<int>& thePosition);
	void rightDownSlot(const Vector2<int>& thePosition);
	void rightUpSlot(const Vector2<int>& thePosition);
	// 0x41eea0 (feedingfrenzy2)
	void mouseMoveSlot(const Vector2<int>& thePosition);
	// 0x54203a (feedingfrenzy2): the wheel's turn kept in the event (and left there), which goes to the target's
	// mouseMove
	void mouseWheelSlot(const Vector2<int>& thePosition, int theDelta);
	// 0x41ed70 (feedingfrenzy2): the event's target for a position, null while inactive or over nothing: the capture,
	// or the component under the mouse that takes input. A new component under the mouse is told the mouse came onto
	// it, after the old one is told it left. A hovered component the tree let go of is let go of first
	Component* findMouseTarget(const Vector2<int>& thePosition);

	bool mActive;                  // +0xc8: the window is active
	Vector2<int> mMousePosition;   // +0xcc: the last position looked at
	Ref<Component> mHovered;       // +0xd4: the component under the mouse
	Ref<Component> mMouseCapture;  // +0xd8
	MouseEvent mMouseEvent;        // +0xdc: filled for each event (its scene this)
};

} // namespace engine

#endif
