#ifndef __SPROUT2_COMPONENT_H__
#define __SPROUT2_COMPONENT_H__

// A component: one node of the scene's tree (engine::Component, vtable 0x56039c (feedingfrenzy2) with 60 slots; its
// constructor 0x45c4d0 and destructor 0x457630 (feedingfrenzy2)). A component has a position in its parent's space,
// a scale, a rotation, an opacity, and a look it passes on to what it draws (a font, a colour and how the colour
// applies, how images are placed, a pixel effect). Each frame the tree is updated (update), its bounds recomputed
// where they changed (computeBounds), and painted (paint) with the transform each component adds to the draw
// context's state (applyTransform).
//
// Three sets of flag bits say what a component is and what is pending: its own (mOwnStates, +0x6c), the tree's (its
// own with all its children's, mTreeStates, +0x70), and the ones switched off (mOffStates, +0x74). A bit set in a
// child's tree states is set in every ancestor's (addState), so an update or a paint only walks the branches that
// need it.

#include "Engine.h"
#include "Geometry.h"
#include "Object.h"
#include "PixelFx.h"

#include <string>
#include <vector>

namespace engine
{

class Container;
class DrawContext;
class Font;

// The state bits
enum ComponentState
{
	STATE_VISIBLE = 0x01,          // drawn (a component that paints sets it; setVisible switches it off)
	STATE_INPUT = 0x02,            // takes the mouse (setTakesInput; a disabled button switches it off)
	STATE_ANIMATED = 0x04,         // has a pixel effect or modifiers to update every frame
	STATE_BOUNDS_DIRTY = 0x08,     // its bounds are to be recomputed
	STATE_DELETE = 0x10,           // to be taken out of its parent at the next update
	STATE_CHILDREN_CHANGED = 0x20  // its children changed since the last update
};

// What the update passes on to every modifier of a component (engine::Modifier: interpolators of a position, a scale,
// an opacity or a colour, applied in the order they were added)
class Modifier : public virtual Interface
{
public:
	// 0x00: the component changed for this frame
	virtual void modify(UpdateContext& theContext, class Component* theComponent) = 0;
};

// What a scene passes to the component the mouse is over or has captured (the scene's +0xdc (feedingfrenzy2))
struct MouseEvent
{
	class Component* mTarget;   // +0x00
	Vector2f mPosition;         // +0x04: in the target's space
	int mButtons;               // +0x0c: bit 0 the left button down, bit 1 the right
	// +0x10: never read
	int mWheelDelta;            // +0x14: the wheel's turn, for a wheel event
};

class Component : public Object
{
public:
	// 0x45c4d0 (feedingfrenzy2)
	Component();
	// 0x457630 (feedingfrenzy2): a named component leaves the engine's table of names
	~Component() override;

	// 0x04 (0x50123f), 0x08 (0x4ff712), 0x0c (0x43cce0) (feedingfrenzy2)
	virtual float getX() { return mPosition.x; }
	virtual float getY() { return mPosition.y; }
	virtual const Vector2f& getPosition() { return mPosition; }
	// 0x10 (0x447bb0 (feedingfrenzy2)): the position in the scene's space
	virtual Vector2f getAbsolutePosition();
	// 0x14 (0x43c200 (feedingfrenzy2))
	virtual void setPosition(const Vector2f& thePosition);
	// 0x18 (0x43c270 (feedingfrenzy2))
	virtual void setPosition(float x, float y);
	// 0x1c (0x43c220 (feedingfrenzy2))
	virtual void setScale(float theScaleX, float theScaleY);
	// 0x20 (0x435250 (feedingfrenzy2))
	virtual void setScale(float theScale) { setScale(theScale, theScale); }
	// 0x24 (0x48d5d0 (feedingfrenzy2))
	virtual const Vector2f& getScale() { return mScale; }
	// 0x28 (0x4381f0) and 0x2c (0x4ff70e) (feedingfrenzy2): in radians
	virtual void setRotation(float theRotation) { mRotation = theRotation; }
	virtual float getRotation() { return mRotation; }
	// 0x30 (0x43c290 (feedingfrenzy2)): the bounds in the parent's space, as computeBounds left them
	virtual const RectF& getBounds() { return mBounds; }
	// 0x34 (0x43c190) and 0x38 (0x495b00) (feedingfrenzy2)
	virtual void setOpacity(float theOpacity) { mOpacity = theOpacity; }
	virtual float getOpacity() { return mOpacity; }
	// 0x3c (0x471ae0) and 0x40 (0x5083fe) (feedingfrenzy2)
	virtual void setFont(Font* theFont);
	virtual Font* getFont() { return mFont; }
	// 0x44 (0x455b20 (feedingfrenzy2)): the alpha left as it is
	virtual void setColor(float theRed, float theGreen, float theBlue);
	// 0x48 (0x473e70) and 0x4c (0x4ff70a) (feedingfrenzy2)
	virtual void setColor(const Color& theColor) { mColor = theColor; }
	virtual const Color& getColor() { return mColor; }
	// 0x50 (0x475f40) and 0x54 (0x4ff706) (feedingfrenzy2): how the colour applies (ColorMode); 0 leaves the parent's
	virtual void setColorMode(int theColorMode) { mColorMode = theColorMode; }
	virtual int getColorMode() { return mColorMode; }
	// 0x58 (0x45f890 (feedingfrenzy2)): how images are placed (FilterMode); 0 leaves the parent's
	virtual void setFilterMode(int theFilterMode) { mFilterMode = theFilterMode; }
	// 0x5c (0x48d880 (feedingfrenzy2))
	virtual void setPixelFx(PixelFx* thePixelFx);
	// 0x60 (0x4506c0 (feedingfrenzy2)): the name, under which the engine's table finds the component
	virtual void setName(const std::string& theName);
	// 0x64 (0x50102b (feedingfrenzy2))
	virtual const std::string& getName() { return mName; }
	// 0x68 (0x5248a7 (feedingfrenzy2))
	virtual std::string getClassName() { return "Component"; }
	// 0x6c (0x443c20 (feedingfrenzy2)): the component's transform and look added to the context's state on top
	virtual void applyTransform(DrawContext* theContext);
	// 0x70 (0x41ebe0 (feedingfrenzy2)): the point (in the parent's space) inside the bounds
	virtual bool contains(const Vector2f& thePoint);
	// 0x74 (0x41ed40 (feedingfrenzy2)): the component under the point (in the parent's space) that has one of the
	// state bits, null for none; a container leaves the point in the found component's space
	virtual Component* hitTest(int theStates, Vector2f& thePoint);
	// 0x78 (0x4ff6da (feedingfrenzy2)): a point of the scene's space brought into the component's
	virtual void toLocal(Vector2f& thePoint);
	// 0x7c (0x4ff6c9 (feedingfrenzy2)): into the parent's space
	virtual void toParentLocal(Vector2f& thePoint);
	// 0x80 (0x455900) and 0x84 (0x444190) (feedingfrenzy2)
	virtual void setVisible(bool theVisible);
	virtual bool isVisible();
	// 0x88 (0x493a10) and 0x8c (0x4ff6c1) (feedingfrenzy2)
	virtual void setTakesInput(bool theTakesInput);
	virtual bool takesInput() { return hasOwnState(STATE_INPUT); }
	// 0x90 and 0x94: the component switched on and off (a container passes it to its children; a scene connects to
	// the engine's signals and disconnects, a button takes the mouse and stops)
	virtual void enable() {}
	virtual void disable() {}
	// 0x98 and 0x9c: a container passes them to its children; a composite container starts and stops redrawing its
	// cached picture
	virtual void beginComposite() {}
	virtual void endComposite() {}
	// 0xa0: the component drawn in the context's state
	virtual void paint(DrawContext* theContext) = 0;
	// 0xa4 (0x43c5b0 (feedingfrenzy2)): the frame's update, theContext's position the parent's in the scene's space
	virtual void update(UpdateContext& theContext);
	// 0xa8: the bounds recomputed
	virtual void computeBounds() = 0;
	// 0xac (0x526a4b (feedingfrenzy2)): a line describing the component in the log, indented (debug)
	virtual void dump(int theIndent);
	// 0xb0: the part of the component inside the rectangle (in the parent's space) drawn
	virtual void paintRegion(DrawContext* theContext, const RectF& theRegion) {}
	// 0xb4 to 0xd4: the mouse, from the scene (0xc8 is neither called nor overridden in feedingfrenzy2)
	virtual void mouseDown(MouseEvent& theEvent) {}
	virtual void mouseUp(MouseEvent& theEvent) {}
	virtual void rightDown(MouseEvent& theEvent) {}
	virtual void rightUp(MouseEvent& theEvent) {}
	// a move, and a turn of the wheel with the event's delta set
	virtual void mouseMove(MouseEvent& theEvent) {}
	virtual void mouseEnter(MouseEvent& theEvent) {}
	virtual void mouseLeave(MouseEvent& theEvent) {}
	// the window lost the focus while the mouse was over or held by the component
	virtual void mouseCancel(MouseEvent& theEvent) {}
	// 0xd8 (0x437da0) and 0xdc (0x437d80) (feedingfrenzy2): a parent holds a reference to its child
	virtual void setParent(Container* theParent);
	virtual void clearParent();
	// 0xe0 (0x43dff0 (feedingfrenzy2))
	virtual Container* getParent() { return mParent; }
	// 0xe4 (0x43c240 (feedingfrenzy2)): bits set in the tree's states, and so in every ancestor's
	virtual void addState(int theStates);
	// 0xe8 (0x4441b0 (feedingfrenzy2)): bits cleared from the tree's states unless the component has them itself
	virtual void removeState(int theStates);
	// 0xec (0x43c540 (feedingfrenzy2)): every modifier applied, in order
	virtual void applyModifiers(UpdateContext& theContext);

	// 0x43c600 (feedingfrenzy2): one of the bits is the component's own and not switched off
	bool hasOwnState(int theStates) const { return (~mOffStates & mOwnStates & theStates) != 0; }
	// 0x4059b0 and 0x407ea0 (feedingfrenzy2)
	void addOwnState(int theStates);
	// 0x4350d0 (feedingfrenzy2): a modifier added unless the component has it; the component is then updated every
	// frame
	void addModifier(Modifier* theModifier);
	// 0x484530 (feedingfrenzy2): the modifier taken out, if the component has it
	void removeModifier(Modifier* theModifier);
	void removeOwnState(int theStates);
	int getOwnStates() const { return mOwnStates; }
	int getTreeStates() const { return mTreeStates; }
	int getOffStates() const { return mOffStates; }
	// Bits switched off and back on (inlined where the original uses them: transitions, the screen manager)
	void addOffState(int theStates) { mOffStates |= theStates; }
	void removeOffState(int theStates) { mOffStates &= ~theStates; }

protected:
	Container* mParent;              // +0x0c
	std::string mName;               // +0x10
	Vector2f mPosition;              // +0x2c
	Vector2f mAbsolutePosition;      // +0x34: set by the update
	Vector2f mScale;                 // +0x3c
	float mRotation;                 // +0x44
	float mOpacity;                  // +0x48
	Ref<Font> mFont;                 // +0x4c
	Color mColor;                    // +0x50
	int mColorMode;                  // +0x60
	int mFilterMode;                 // +0x64
	Ref<PixelFx> mPixelFx;           // +0x68
	int mOwnStates;                  // +0x6c
	int mTreeStates;                 // +0x70
	int mOffStates;                  // +0x74
	RectF mBounds;                   // +0x78
	std::vector<Ref<Modifier>> mModifiers;  // +0x88
};

} // namespace engine

#endif
