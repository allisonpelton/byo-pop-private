#ifndef __WORDHARMONY_ENGINE_NODE_H__
#define __WORDHARMONY_ENGINE_NODE_H__

// The scene graph: nodes (0x88 bytes, constructor 0x43a3c0, vtable 0x484b50), containers (0xa8 bytes, constructor
// 0x40f110, vtable 0x481ca8) and the root window (0xcc bytes, constructor 0x438e70, vtable 0x484a08).
//
// A node has a position in its parent, a scale, a rotation, an opacity, a colour offset and a font, which it applies
// to the render state before it draws (0x439df0); its bounds are in its parent's coordinates. Flags say what a node
// and its descendants need: each node keeps its own flags and the union with its children's, and setting a flag
// propagates up the tree (0x439000). A parent holds a reference on each child (the child adds it in setParent).

#include "Geometry.h"
#include "Object.h"
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
	Node();            // 0x43a3c0
	~Node() override;  // 0x439e90

	virtual float getX() { return mPosition.x; }                     // 0x04: 0x439a20
	virtual float getY() { return mPosition.y; }                     // 0x08: 0x439a30
	virtual Point* getPosition() { return &mPosition; }              // 0x0c: 0x439a40
	virtual Point* getAbsPosition() { return &mAbsPosition; }        // 0x10: 0x439a50
	virtual void setPosition(const Point& thePosition);              // 0x14: 0x439aa0
	virtual void setPosition(float x, float y);                      // 0x18: 0x439a70
	virtual void setScale(float theScaleX, float theScaleY);         // 0x1c: 0x439ae0
	virtual void setRotation(float theRotation) { mRotation = theRotation; }  // 0x20: 0x439b00
	virtual Rect* getBounds() { return &mBounds; }                   // 0x24: 0x439a60
	virtual void setOpacity(float theOpacity) { mOpacity = theOpacity; }  // 0x28: 0x439ad0
	virtual float getOpacity() { return mOpacity; }                 // 0x2c: 0x409d00
	virtual void setFont(Font* theFont);                             // 0x30: 0x439da0
	virtual void setName(const std::string& theName);                // 0x34: 0x43a050
	virtual std::string getName() { return mName; }                  // 0x38: 0x43a020
	virtual void applyTransform(Graphics* g);                        // 0x3c: 0x439df0
	virtual bool contains(Point& p);                                 // 0x40: 0x439b10
	virtual Node* hitTest(Point& p);                                 // 0x44: 0x439b30
	virtual Node* hitTest(unsigned int theFlags, Point& p);          // 0x48: 0x439d60
	virtual void toLocal(Point& p);                                  // 0x4c: 0x439b60
	virtual void parentToLocal(Point& p);                            // 0x50: 0x439b90
	virtual void setVisible(bool theVisible) { mVisible = theVisible; }  // 0x54: 0x438fd0
	virtual bool isVisible() { return mVisible; }                    // 0x58: 0x439bb0
	virtual void setEnabled(bool theEnabled) { mEnabled = theEnabled; }  // 0x5c: 0x438fe0
	virtual bool isEnabled() { return mEnabled; }                    // 0x60: 0x439bc0
	virtual void start() {}                                          // 0x64: 0x401000
	virtual void stop() {}                                           // 0x68: 0x401000
	virtual void draw(Graphics* g) = 0;                              // 0x6c
	virtual void update(UpdateInfo& theInfo);                        // 0x70: 0x439bd0
	virtual void layout() = 0;                                       // 0x74
	virtual void dump(int theIndent);                                // 0x78: 0x43a0a0
	// The mouse: the public slots call the protected ones, which do nothing here
	virtual void mouseDown(Point& p) { onMouseDown(p); }             // 0x7c: 0x439c00
	virtual void mouseUp(Point& p) { onMouseUp(p); }                 // 0x84: 0x439c10
	virtual void rightDown(Point& p) { onRightDown(p); }             // 0x8c: 0x439c20
	virtual void rightUp(Point& p) { onRightUp(p); }                 // 0x94: 0x439c30
	virtual void mouseMove(Point& p) {}                              // 0x9c: 0x43edb0
	virtual void mouseEnter(Point& p) { onMouseEnter(p); }           // 0xa0: 0x439c40
	virtual void mouseExit(Point& p) { onMouseExit(p); }             // 0xa8: 0x439c50
	virtual void setParent(Node* theParent);                         // 0xb0: 0x439c60
	virtual void clearParent(Node* theParent);                       // 0xb4: 0x439c80
	virtual Node* getParent() { return mParent; }                    // 0xb8: 0x439ca0
	virtual void setFlags(unsigned int theFlags);                    // 0xbc: 0x439000
	virtual void clearFlags(unsigned int theFlags);                  // 0xc0: 0x439cf0
	virtual void notify(UpdateInfo& theInfo);                        // 0xc4: 0x439e60

	// 0x439cb0
	void addOwnFlags(unsigned int theFlags);
	// 0x439cd0
	void removeOwnFlags(unsigned int theFlags);
	bool hasFlag(unsigned int theFlags) const { return (mAllFlags & theFlags) != 0; }     // 0x439d30
	bool hasOwnFlag(unsigned int theFlags) const { return (mFlags & theFlags) != 0; }     // 0x439d40
	unsigned int getAllFlags() const { return mAllFlags; }                                // 0x439d50
	// 0x43a510
	void addListener(NodeListener* theListener);
	// 0x439fa0
	void removeAllListeners();

protected:
	virtual void onMouseDown(Point& p) {}   // 0x80
	virtual void onMouseUp(Point& p) {}     // 0x88
	virtual void onRightDown(Point& p) {}   // 0x90
	virtual void onRightUp(Point& p) {}     // 0x98
	virtual void onMouseEnter(Point& p) {}  // 0xa4
	virtual void onMouseExit(Point& p) {}   // 0xac

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
	unsigned int mFlags;                   // +0x5c
	unsigned int mAllFlags;                // +0x60
	bool mVisible;                         // +0x64
	bool mEnabled;                         // +0x65
	Rect mBounds;                          // +0x68
	std::vector<NodeListener*> mListeners; // +0x78 (counted)
};

class Container : public Node
{
public:
	Container();            // 0x40f110
	~Container() override;  // 0x439460

	Node* hitTest(Point& p) override;                         // 0x44: 0x439060
	Node* hitTest(unsigned int theFlags, Point& p) override;  // 0x48: 0x4390f0
	void start() override;                                    // 0x64: 0x4391b0
	void stop() override;                                     // 0x68: 0x4391e0
	void draw(Graphics* g) override;                          // 0x6c: 0x4392b0
	void update(UpdateInfo& theInfo) override;                // 0x70: 0x439510
	void layout() override;                                   // 0x74: 0x439210
	void dump(int theIndent) override;                        // 0x78: 0x439620
	void clearFlags(unsigned int theFlags) override;          // 0xc0: 0x439310
	virtual void removeChild(Node* theChild);                 // 0xc8: 0x4395e0
	virtual void setRect(float l, float t, float r, float b) { mRect.set(l, t, r, b); }  // 0xcc: 0x438ff0
	virtual void addChild(Node* theChild);                    // 0xd0: 0x4399b0
	virtual void removeAllChildren();                         // 0xd4: 0x4393e0

	std::vector<Node*> mChildren;  // +0x88 (each counted through its parent link)
	Rect mRect;                    // +0x98: the bounds before the children's are added

protected:
	// 0x439380
	void eraseChild(size_t theIndex);
};

// The root of the scene (the application's +0xa8): it routes the mouse to the node under it and keys to its handlers
class Window : public Container
{
public:
	Window();            // 0x438e70
	~Window() override;  // 0x438a50

	void addKeyHandler(KeyHandler* theHandler);     // 0x438f80
	void removeKeyHandler(KeyHandler* theHandler);  // 0x438b40
	void keyDown(int theKey);                       // 0x438770
	void keyUp(int theKey);                         // 0x4387b0
	void handleMouseDown(const Point& p);           // 0x4387f0
	void handleMouseUp(const Point& p);             // 0x438860
	void handleRightDown(const Point& p);           // 0x4388d0
	void handleRightUp(const Point& p);             // 0x438940
	void handleMouseMove(const Point& p);           // 0x4389b0
	const Point& getMousePosition() const { return mMousePosition; }

	Node* mHover;                           // +0xa8 (counted)
	std::vector<KeyHandler*> mKeyHandlers;  // +0xac (counted)
	Point mUnknownBC;                       // +0xbc
	Point mMousePosition;                   // +0xc4

private:
	// 0x4386c0: the node under the mouse changes
	void setHover(Node* theNode, Point& p);
	Node* mouseTarget(const Point& p, Point& theLocal);
};

} // namespace engine

#endif
