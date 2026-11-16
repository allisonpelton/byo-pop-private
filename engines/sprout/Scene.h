#ifndef __SPROUT_SCENE_H__
#define __SPROUT_SCENE_H__

// The signal build's root of the scene graph (Pizza Frenzy's; Iggle Pop's Mac build names the class Scene): a
// container the game's application makes (0x15c bytes, constructor 0x471a40 (pizzafrenzy), vtable
// 0x502428 (pizzafrenzy)) and connects to the application's signals, which then update and draw it each frame and
// bring it the mouse. The earlier build's root is the Window (Node.h), which the application makes and drives itself;
// Feeding Frenzy's (0x4a8888 (feedingfrenzy), gEarlyBuild) is a scene that gives the mouse to the nodes' mouse slots
// (Point arguments) as the window does, and has no refreshMouse.
//
// The mouse goes to the node under it that takes the mouse (the hit test with NODE_MOUSE), or only to the node that
// has captured it; the node under it gets the enter and exit events as it changes. Each event is the scene's one
// record (+0x140): the target, the point the hit test left (in the captured node's coordinates when one is) and the
// buttons down.

#include "Node.h"
#include "Signal.h"

namespace engine
{

class Graphics;

class Scene : public Container, public HasSlots
{
public:
	Scene();            // 0x471a40 (pizzafrenzy)
	~Scene() override;  // 0x471b30 (pizzafrenzy), 0x471760 (pizzafrenzy)

	// 0x88: 0x471b50 (pizzafrenzy): the application's update, draw and mouse signals to the scene's handlers
	void connect() override;
	// 0x8c: 0x471690 (pizzafrenzy): the scene's connections to them dropped
	void disconnect() override;
	// 0x94: 0x4710f0 (pizzafrenzy): the frame's update, then the layout of the bounds
	void update(UpdateInfo& theInfo) override;
	// 0x114: 0x4715b0 (pizzafrenzy): the node under the mouse found again where the mouse is, and sent a move
	void refreshMouse() override;

	// +0x13c: the node that has the mouse (not counted), or null
	void setCapture(Node* theNode) { mCapture = theNode; }
	Node* getCapture() const { return mCapture; }

private:
	void onAppUpdate(UpdateInfo& theInfo);  // 0x471130 (pizzafrenzy)
	void onAppDraw(Graphics* g);            // 0x471120 (pizzafrenzy)
	void onAppMouseDown(Point& thePoint);   // 0x4714c0 (pizzafrenzy)
	void onAppMouseUp(Point& thePoint);     // 0x4714f0 (pizzafrenzy)
	void onAppRightDown(Point& thePoint);   // 0x471520 (pizzafrenzy)
	void onAppRightUp(Point& thePoint);     // 0x471550 (pizzafrenzy)
	void onAppMouseMove(Point& thePoint);   // 0x471580 (pizzafrenzy)
	// 0x4712a0 (pizzafrenzy): the event's target for a point of the screen, the enter and exit events sent as it changes
	Node* findTarget(const Point& thePoint);
	// 0x44cef0 (feedingfrenzy): the early build's, whose events are the nodes' mouse slots (Node.h)
	Node* findEarlyTarget(Point& thePoint);

	Ref<Node> mHover;    // +0x138: the node under the mouse
	Node* mCapture;      // +0x13c
	MouseEvent mEvent;   // +0x140
};

} // namespace engine

#endif
