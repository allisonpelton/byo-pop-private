#ifndef __SPROUT2_CONTAINER_H__
#define __SPROUT2_CONTAINER_H__

// A component with children (engine::Container, vtable 0x5508d4 (feedingfrenzy2) with 70 slots; its constructor
// 0x468fb0 (feedingfrenzy2)). Its children are drawn in the order they were added, the last on top, and the mouse
// finds the top one first. Each child holds a reference to itself while it has a parent (Component::setParent).

#include "Component.h"

#include <vector>

namespace engine
{

class Container : public Component
{
public:
	Container();

	// 0x68
	std::string getClassName() override { return "Container"; }
	// 0x74 (0x41ec00 (feedingfrenzy2)): the top child under the point that has one of the state bits, else the
	// container itself if one of its own bits is among them; the point is left in the found child's space
	Component* hitTest(int theStates, Vector2f& thePoint) override;
	// 0x90 (0x4282f0) and 0x94 (0x433a90) (feedingfrenzy2)
	void enable() override;
	void disable() override;
	// 0x98 (0x51ba69) and 0x9c (0x51ba15) (feedingfrenzy2)
	void beginComposite() override;
	void endComposite() override;
	// 0xa0 (0x443850 (feedingfrenzy2)): every visible child, each in a copy of the state with its transform added
	void paint(DrawContext* theContext) override;
	// 0xa4 (0x43c640 (feedingfrenzy2)): the children that need it updated, and those marked for deletion taken out
	void update(UpdateContext& theContext) override;
	// 0xa8 (0x43c2a0 (feedingfrenzy2)): the clip rectangle and the children's bounds together, scaled and moved by
	// the container's own scale and position
	void computeBounds() override;
	// 0xac (0x52682d (feedingfrenzy2))
	void dump(int theIndent) override;
	// 0xb0 (0x4815b0 (feedingfrenzy2)): passed to the children in the container's space
	void paintRegion(DrawContext* theContext, const RectF& theRegion) override;
	// 0xe8 (0x43c490 (feedingfrenzy2)): the children's tree states kept
	void removeState(int theStates) override;

	// 0xf0 (0x46daf0 (feedingfrenzy2))
	virtual void removeChild(Component* theChild);
	// 0xf4 and 0xf8 (0x4398f0 (feedingfrenzy2)): a child came or went
	virtual void childAdded(Component* theChild) { addState(STATE_CHILDREN_CHANGED); }
	virtual void childRemoved(Component* theChild) { addState(STATE_CHILDREN_CHANGED); }
	// 0xfc (0x4ff651) and 0x100 (0x4773c0) (feedingfrenzy2): the rectangle the bounds start from
	virtual void setClipRect(const RectF& theRect);
	virtual void setClipRect(float theLeft, float theTop, float theRight, float theBottom);
	// 0x104 (0x5029dd (feedingfrenzy2))
	virtual const RectF& getClipRect() { return mClipRect; }
	// 0x108 (0x43ab90 (feedingfrenzy2)): on top of the others; a child already there stays where it is
	virtual void addChild(Component* theChild);
	// 0x10c (0x437e90 (feedingfrenzy2))
	virtual void removeAllChildren();
	// 0x110 (0x530720 (feedingfrenzy2)): null out of range (the original returns whatever was in its register)
	virtual Component* getChild(int theIndex);
	// 0x114 (0x41fdd0 (feedingfrenzy2))
	virtual int getChildCount() { return (int)mChildren.size(); }

protected:
	// 0x4348c0 (feedingfrenzy2): the child at the index taken out
	void eraseChild(size_t theIndex);

	std::vector<Component*> mChildren;   // +0x98
	RectF mClipRect;                     // +0xa8
};

} // namespace engine

#endif
