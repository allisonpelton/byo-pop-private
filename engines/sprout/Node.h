#ifndef __SPROUT_NODE_H__
#define __SPROUT_NODE_H__

// The scene graph: nodes (0x88 bytes, constructor 0x43a3c0 (wordharmony), vtable 0x484b50 (wordharmony)), containers
// (0xa8 bytes, constructor 0x40f110 (wordharmony), vtable 0x481ca8 (wordharmony)) and the root window (0xcc bytes,
// constructor 0x438e70 (wordharmony), vtable 0x484a08 (wordharmony)).
//
// A node has a position in its parent, a scale, a rotation, an opacity, a colour offset and a font, which it applies to
// the render state before it draws (0x439df0 (wordharmony)); its bounds are in its parent's coordinates. Flags say what
// a node and its descendants need: each node keeps its own flags and the union with its children's, and setting a flag
// propagates up the tree (0x439000 (wordharmony)). A parent holds a reference on each child (the child adds it in
// setParent).

#include "BltFx.h"
#include "Build.h"
#include "Color4.h"
#include "Geometry.h"
#include "Object.h"
#include "Signal.h"
#include <string>
#include <vector>

namespace engine
{

class Font;
class Graphics;
class Node;

// What a frame's update passes down: the parent's origin on the screen and the seconds since the last frame
struct UpdateInfo
{
	Point mOrigin;   // +0x00
	float mSeconds;  // +0x08
};

// The flags (+0x5c a node's own, +0x60 with its descendants')
enum
{
	NODE_DRAWABLE = 0x01,
	NODE_MOUSE = 0x02,      // takes the mouse
	NODE_LISTENERS = 0x04,  // has update listeners
	NODE_LAYOUT = 0x08,     // its bounds need a layout
	NODE_REMOVE = 0x10      // its parent removes it at the next update
};

// The signal build's mouse event (Pizza Frenzy's; its scene keeps one at +0x140 (pizzafrenzy)): the node it goes to,
// the point (in that node's coordinates when the mouse is captured, else as the hit test left it) and the buttons
// down (1 the left, 2 the right)
struct MouseEvent
{
	Node* mTarget;          // +0x00
	Point mPosition;        // +0x04
	unsigned int mButtons;  // +0x0c
};

// A node's update listener (slot 0)
class NodeListener : public virtual Interface
{
public:
	virtual void update(UpdateInfo& theInfo, Node* theNode) = 0;
};

// A window's key handler (slots 0 and 4): returns true when it took the key
class KeyHandler : public virtual Interface
{
public:
	virtual bool keyDown(int theKey) = 0;
	virtual bool keyUp(int theKey) = 0;
};

class Node : public Object
{
public:
	Node();            // 0x43a3c0 (wordharmony)
	~Node() override;  // 0x439e90 (wordharmony)

	virtual float getX() { return mPosition.x; }                     // 0x04: 0x439a20 (wordharmony)
	virtual float getY() { return mPosition.y; }                     // 0x08: 0x439a30 (wordharmony)
	virtual Point* getPosition() { return &mPosition; }              // 0x0c: 0x439a40 (wordharmony)
	virtual Point* getAbsPosition() { return &mAbsPosition; }        // 0x10: 0x439a50 (wordharmony)
	virtual void setPosition(const Point& thePosition);              // 0x14: 0x439aa0 (wordharmony)
	virtual void setPosition(float x, float y);                      // 0x18: 0x439a70 (wordharmony)
	virtual void setScale(float theScaleX, float theScaleY);         // 0x1c: 0x439ae0 (wordharmony)
	// 0x20: 0x467b40 (pizzafrenzy): the signal build's one factor for both directions
	virtual void setScale(float theScale) { setScale(theScale, theScale); }
	virtual Rect* getBounds() { return &mBounds; }                   // 0x24: 0x439a60 (wordharmony)
	virtual void setOpacity(float theOpacity) { mOpacity = theOpacity; }  // 0x28: 0x439ad0 (wordharmony)
	virtual float getOpacity() { return mOpacity; }                 // 0x2c: 0x409d00 (wordharmony)
	virtual void setFont(Font* theFont);                             // 0x30: 0x439da0 (wordharmony)
	virtual void setName(const std::string& theName);                // 0x34: 0x43a050 (wordharmony)
	virtual std::string getName() { return mName; }                  // 0x38: 0x43a020 (wordharmony)
	// 0x3c: 0x439df0 (wordharmony); 0x64: 0x467fd0 (pizzafrenzy)
	virtual void applyTransform(Graphics* g);
	virtual bool contains(Point& p);                                 // 0x40: 0x439b10 (wordharmony)
	virtual Node* hitTest(unsigned int theFlags, Point& p);          // 0x48: 0x439d60 (wordharmony)
	virtual void toLocal(Point& p);                                  // 0x4c: 0x439b60 (wordharmony)
	// 0x54: 0x438fd0 (wordharmony); 0x78: 0x467eb0 (pizzafrenzy), where it is the mask's bit 1 (+0x74)
	virtual void setVisible(bool theVisible) { mVisible = theVisible; }
	// 0x58: 0x439bb0 (wordharmony); 0x7c: 0x467ed0 (pizzafrenzy): the signal build's also needs the drawable flag
	virtual bool isVisible() { return gSignalBuild ? mVisible && (mAllFlags & NODE_DRAWABLE) != 0 : mVisible; }
	// 0x5c: 0x438fe0 (wordharmony); 0x80: 0x467ef0 (pizzafrenzy): the signal build's sets or clears the own mouse flag
	virtual void setEnabled(bool theEnabled);
	// 0x60: 0x439bc0 (wordharmony); 0x84: 0x467f20 (pizzafrenzy): the signal build's: the own mouse flag, not masked
	virtual bool isEnabled();
	// The signal build's (Pizza Frenzy's) render state of a node that applyTransform gives the draw context: a colour,
	// how the colour is used (0 not at all, 1 added to the pixels', 2 multiplied with them), whether the node's draw is
	// smoothed, and a blit effect
	virtual void setColor(float theRed, float theGreen, float theBlue)  // 0x3c: 0x467bc0 (pizzafrenzy)
	{
		mColor.r = theRed;
		mColor.g = theGreen;
		mColor.b = theBlue;
	}
	virtual void setColor(const Color4& theColor) { mColor = theColor; }  // 0x40: 0x467b90 (pizzafrenzy)
	virtual Color4* getColor() { return &mColor; }                         // 0x44: 0x467bb0 (pizzafrenzy)
	virtual void setColorMode(int theMode) { mColorMode = theMode; }       // 0x48: 0x467bd0 (pizzafrenzy)
	virtual void setSmoothType(int theSmoothType) { mSmoothType = theSmoothType; }  // 0x50: 0x467be0 (pizzafrenzy)
	virtual void setBltFx(BltFx* theBltFx) { mBltFx = theBltFx; }          // 0x54: 0x467f90 (pizzafrenzy)
	// The signal build's activation (Pizza Frenzy's): a node connects to the application's signals when its screen is
	// shown, and drops them when it goes; a node of its own does nothing
	virtual void connect() {}     // 0x88: 0x4d0470 (pizzafrenzy)
	virtual void disconnect() {}  // 0x8c: 0x4d0470 (pizzafrenzy)
	// 0x114: 0x4670e0 (pizzafrenzy): what the mouse is over may have changed, which the scene at the top finds again
	virtual void refreshMouse();
	virtual void draw(Graphics* g) = 0;                              // 0x6c
	virtual void update(UpdateInfo& theInfo);                        // 0x70: 0x439bd0 (wordharmony)
	virtual void layout() = 0;                                       // 0x74
	// The mouse: the public slots call the protected ones, which do nothing here
	virtual void mouseDown(Point& p) { onMouseDown(p); }             // 0x7c: 0x439c00 (wordharmony)
	virtual void mouseUp(Point& p) { onMouseUp(p); }                 // 0x84: 0x439c10 (wordharmony)
	virtual void rightDown(Point& p) { onRightDown(p); }             // 0x8c: 0x439c20 (wordharmony)
	virtual void rightUp(Point& p) { onRightUp(p); }                 // 0x94: 0x439c30 (wordharmony)
	virtual void mouseMove(Point& p) {}                              // 0x9c: 0x43edb0 (wordharmony)
	virtual void mouseEnter(Point& p) { onMouseEnter(p); }           // 0xa0: 0x439c40 (wordharmony)
	virtual void mouseExit(Point& p) { onMouseExit(p); }             // 0xa8: 0x439c50 (wordharmony)
	virtual void setParent(Node* theParent);                         // 0xb0: 0x439c60 (wordharmony)
	virtual void clearParent(Node* theParent);                       // 0xb4: 0x439c80 (wordharmony)
	virtual void setFlags(unsigned int theFlags);                    // 0xbc: 0x439000 (wordharmony)
	virtual void clearFlags(unsigned int theFlags);                  // 0xc0: 0x439cf0 (wordharmony)
	virtual void notify(UpdateInfo& theInfo);                        // 0xc4: 0x439e60 (wordharmony)

	// 0x439cb0 (wordharmony)
	void addOwnFlags(unsigned int theFlags);
	// 0x439cd0 (wordharmony)
	void removeOwnFlags(unsigned int theFlags);
	// 0x439d30 (wordharmony), 0x467e10 (pizzafrenzy): the signal build leaves out the flags its mask (+0x74) holds, the
	// drawable flag of a hidden node (setVisible) and the mouse flag of a masked one (setMouseMasked)
	bool hasFlag(unsigned int theFlags) const { return (mAllFlags & ~getMaskedFlags() & theFlags) != 0; }
	// The signal build's mask (+0x74 (pizzafrenzy)), from the two fields that keep its bits
	unsigned int getMaskedFlags() const
	{
		if (!gSignalBuild)
			return 0;
		return (mVisible ? 0 : NODE_DRAWABLE) | (mMouseMasked ? NODE_MOUSE : 0);
	}
	bool hasOwnFlag(unsigned int theFlags) const { return (mFlags & theFlags) != 0; }     // 0x439d40 (wordharmony)
	unsigned int getAllFlags() const { return mAllFlags; }                                // 0x439d50 (wordharmony)
	// The signal build's mouse (Pizza Frenzy's): its scene hands the node under the mouse an event. Each public slot
	// sends the node's signal for it, then calls its handler, whose default calls the protected slot, which does
	// nothing; a node of the earlier build's takes the mouse through the slots above instead.
	virtual void mouseDownEvent(MouseEvent& e);   // 0xa0: 0x4680c0 (pizzafrenzy)
	virtual void handleMouseDownEvent(MouseEvent& e) { onMouseDownEvent(e); }    // 0xa4: 0x467c90 (pizzafrenzy)
	virtual void mouseUpEvent(MouseEvent& e);     // 0xac: 0x468100 (pizzafrenzy)
	virtual void handleMouseUpEvent(MouseEvent& e) { onMouseUpEvent(e); }        // 0xb0: 0x467ca0 (pizzafrenzy)
	virtual void rightDownEvent(MouseEvent& e);   // 0xb8: 0x468140 (pizzafrenzy)
	virtual void handleRightDownEvent(MouseEvent& e) { onRightDownEvent(e); }    // 0xbc: 0x467cb0 (pizzafrenzy)
	virtual void rightUpEvent(MouseEvent& e);     // 0xc4: 0x468180 (pizzafrenzy)
	virtual void handleRightUpEvent(MouseEvent& e) { onRightUpEvent(e); }        // 0xc8: 0x467cc0 (pizzafrenzy)
	virtual void mouseMoveEvent(MouseEvent& e);   // 0xd0: 0x4681c0 (pizzafrenzy)
	virtual void handleMouseMoveEvent(MouseEvent& e) {}                         // 0xd4: 0x492310 (pizzafrenzy)
	virtual void mouseEnterEvent(MouseEvent& e);  // 0xd8: 0x468200 (pizzafrenzy)
	virtual void handleMouseEnterEvent(MouseEvent& e) { onMouseEnterEvent(e); }  // 0xdc: 0x467cd0 (pizzafrenzy)
	virtual void mouseExitEvent(MouseEvent& e);   // 0xe4: 0x468240 (pizzafrenzy)
	virtual void handleMouseExitEvent(MouseEvent& e) { onMouseExitEvent(e); }    // 0xe8: 0x467ce0 (pizzafrenzy)

	// 0x43a510 (wordharmony), 0x468df0 (pizzafrenzy)
	void addListener(NodeListener* theListener);
	// 0x439fa0 (wordharmony)
	void removeAllListeners();
	// 0x468280 (pizzafrenzy): the listener's reference goes with it
	void removeListener(NodeListener* theListener);
	// 0x467d70 (pizzafrenzy), 0x467d80 (pizzafrenzy): the mask's bit 2, which keeps the mouse from a node while its
	// screen is not connected
	void setMouseMasked(bool theMasked) { mMouseMasked = theMasked; }

protected:
	virtual void onMouseDown(Point& p) {}   // 0x80
	virtual void onMouseUp(Point& p) {}     // 0x88
	virtual void onRightDown(Point& p) {}   // 0x90
	virtual void onRightUp(Point& p) {}     // 0x98
	virtual void onMouseEnter(Point& p) {}  // 0xa4
	virtual void onMouseExit(Point& p) {}   // 0xac
	// the signal build's, each 0x4d0470 (pizzafrenzy)
	virtual void onMouseDownEvent(MouseEvent& e) {}   // 0xa8
	virtual void onMouseUpEvent(MouseEvent& e) {}     // 0xb4
	virtual void onRightDownEvent(MouseEvent& e) {}   // 0xc0
	virtual void onRightUpEvent(MouseEvent& e) {}     // 0xcc
	virtual void onMouseEnterEvent(MouseEvent& e) {}  // 0xe0
	virtual void onMouseExitEvent(MouseEvent& e) {}   // 0xec

public:
	Node* mParent;                         // +0x0c
	std::string mName;                     // +0x10
	Point mPosition;                       // +0x2c
	Point mAbsPosition;                    // +0x34
	Point mScale;                          // +0x3c
	float mRotation;                       // +0x44
	float mOpacity;                        // +0x48
	Font* mFont;                           // +0x4c (counted)
	Color mColorOffset;                    // +0x50
	Color4 mColor;                         // +0x50 (pizzafrenzy)
	int mColorMode;                        // +0x60 (pizzafrenzy)
	int mSmoothType;                       // +0x64 (pizzafrenzy)
	Ref<BltFx> mBltFx;                     // +0x68 (pizzafrenzy)
	unsigned int mFlags;                   // +0x5c
	unsigned int mAllFlags;                // +0x60
	bool mVisible;                         // +0x64
	bool mEnabled;                         // +0x65
	Rect mBounds;                          // +0x68
	std::vector<NodeListener*> mListeners; // +0x78 (counted); +0x8c (pizzafrenzy)
	bool mMouseMasked;                     // +0x74 bit 2 (pizzafrenzy)
	// The signal build's mouse signals (Pizza Frenzy's node, 0x468ba0 (pizzafrenzy), makes them)
	Signal<MouseEvent&> mMouseDownSignal;   // +0x98
	Signal<MouseEvent&> mMouseUpSignal;     // +0xa8
	Signal<MouseEvent&> mRightDownSignal;   // +0xb8
	Signal<MouseEvent&> mRightUpSignal;     // +0xc8
	Signal<MouseEvent&> mMouseMoveSignal;   // +0xd8
	Signal<MouseEvent&> mMouseEnterSignal;  // +0xe8
	Signal<MouseEvent&> mMouseExitSignal;   // +0xf8
};

class Container : public Node
{
public:
	Container();            // 0x40f110 (wordharmony)
	~Container() override;  // 0x439460 (wordharmony)

	Node* hitTest(unsigned int theFlags, Point& p) override;  // 0x48: 0x4390f0 (wordharmony); 0x467110 (pizzafrenzy)
	Node* hitTestSignal(unsigned int theFlags, Point& thePoint);  // 0x467110 (pizzafrenzy)
	void draw(Graphics* g) override;                          // 0x6c: 0x4392b0 (wordharmony)
	void update(UpdateInfo& theInfo) override;                // 0x70: 0x439510 (wordharmony)
	void layout() override;                                   // 0x74: 0x439210 (wordharmony)
	void clearFlags(unsigned int theFlags) override;          // 0xc0: 0x439310 (wordharmony)
	virtual void removeChild(Node* theChild);                 // 0xc8: 0x4395e0 (wordharmony)
	virtual void setRect(float l, float t, float r, float b) { mRect.set(l, t, r, b); }  // 0xcc: 0x438ff0 (wordharmony)
	virtual void addChild(Node* theChild);                    // 0xd0: 0x4399b0 (wordharmony)
	virtual void removeAllChildren();                         // 0xd4: 0x4393e0 (wordharmony)
	// 0x88: 0x4671b0 (pizzafrenzy), 0x8c: 0x4671e0 (pizzafrenzy): a container connects and disconnects its children
	void connect() override { connectChildren(); }
	void disconnect() override { disconnectChildren(); }
	// 0x4671b0 (pizzafrenzy), 0x4671e0 (pizzafrenzy): each child's connect or disconnect
	void connectChildren();
	void disconnectChildren();

	std::vector<Node*> mChildren;  // +0x88 (each counted through its parent link)
	Rect mRect;                    // +0x98: the bounds before the children's are added

protected:
	// 0x439380 (wordharmony)
	void eraseChild(size_t theIndex);
};

// The root of the scene (the application's +0xa8): it routes the mouse to the node under it and keys to its handlers
class Window : public Container
{
public:
	Window();            // 0x438e70 (wordharmony)
	~Window() override;  // 0x438a50 (wordharmony)

	void addKeyHandler(KeyHandler* theHandler);     // 0x438f80 (wordharmony)
	void removeKeyHandler(KeyHandler* theHandler);  // 0x438b40 (wordharmony)
	void keyDown(int theKey);                       // 0x438770 (wordharmony)
	void keyUp(int theKey);                         // 0x4387b0 (wordharmony)
	void handleMouseDown(const Point& thePoint);    // 0x4387f0 (wordharmony)
	void handleMouseUp(const Point& thePoint);      // 0x438860 (wordharmony)
	void handleRightDown(const Point& thePoint);    // 0x4388d0 (wordharmony)
	void handleRightUp(const Point& thePoint);      // 0x438940 (wordharmony)
	void handleMouseMove(const Point& thePoint);    // 0x4389b0 (wordharmony)

	Node* mHover;                           // +0xa8 (counted)
	std::vector<KeyHandler*> mKeyHandlers;  // +0xac (counted)

private:
	// 0x4386c0 (wordharmony): the node under the mouse changes
	void setHover(Node* theNode, Point& p);
	Node* mouseTarget(const Point& p, Point& theLocal);
};

} // namespace engine

#endif
