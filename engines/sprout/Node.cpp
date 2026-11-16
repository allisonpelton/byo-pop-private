#include "Node.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace engine;

bool engine::gSignalBuild = false;
bool engine::gEarlyBuild = false;

// 0x43a3c0 (wordharmony)
Node::Node()
	: mParent(nullptr), mPosition(0, 0), mScale(1.0f, 1.0f), mRotation(0), mOpacity(1.0f), mFont(nullptr),
	  mColorMode(0), mSmoothType(0), mFlags(0), mAllFlags(0), mVisible(true), mEnabled(true), mMouseMasked(false),
	  mListenersMasked(false)
{
}

// 0x439e90 (wordharmony): the listeners' memory goes, not their references (0x439fa0 (wordharmony) releases them)
Node::~Node()
{
	if (mFont != nullptr)
		mFont->release();
}

// 0x439aa0 (wordharmony)
void Node::setPosition(const Point& thePosition)
{
	mPosition = Point(thePosition.x, thePosition.y);
	setFlags(NODE_LAYOUT);
}

// 0x439a70 (wordharmony)
void Node::setPosition(float x, float y)
{
	mPosition = Point(x, y);
	setFlags(NODE_LAYOUT);
}

// 0x439ae0 (wordharmony)
void Node::setScale(float theScaleX, float theScaleY)
{
	mScale.x = theScaleX;
	mScale.y = theScaleY;
	setFlags(NODE_LAYOUT);
}

// 0x439da0 (wordharmony)
void Node::setFont(Font* theFont)
{
	if (theFont != nullptr)
		theFont->addRef();
	if (mFont != nullptr)
		mFont->release();
	mFont = theFont;
	setFlags(NODE_LAYOUT);
}

// 0x43a050 (wordharmony): (the original also kept a map from names to nodes in the application, which only the particle
// streams' anchors read, and no data has one)
void Node::setName(const std::string& theName) { mName = theName; }

// 0x439df0 (wordharmony), 0x467fd0 (pizzafrenzy), 0x43fb10 (feedingfrenzy): the signal build's moves the origin by the position times the scale
// so far, and has the colour only when it has a colour mode, instead of a colour offset
void Node::applyTransform(Graphics* g)
{
	if (gSignalBuild && !gEarlyBuild)
	{
		float aScaleY = g->getScaleY() * mPosition.y;
		float aScaleX = g->getScaleX() * mPosition.x;
		g->translate(aScaleX, aScaleY);
		g->scale(mScale.x, mScale.y);
		g->rotate(mRotation);
		g->multiplyOpacity(mOpacity);
		if (mFont != nullptr)
			g->setFont(mFont);
		if (mColorMode != 0)
		{
			g->setColor(mColor.r, mColor.g, mColor.b);
			g->setColorMode(mColorMode);
		}
		if (mSmoothType != 0)
			g->setSmoothType(mSmoothType);
		if (mBltFx != nullptr)
			g->setBltFx(mBltFx);
		return;
	}
	g->translate(mPosition.x, mPosition.y);
	g->scale(mScale.x, mScale.y);
	g->rotate(mRotation);
	g->multiplyOpacity(mOpacity);
	g->addColorOffset(mColorOffset.r, mColorOffset.g, mColorOffset.b);
	if (mFont != nullptr)
		g->setFont(mFont);
	// 0x43fb10 (feedingfrenzy): the node's own blit effect, which the signal build also has
	if (gEarlyBuild && mBltFx != nullptr)
		g->setBltFx(mBltFx);
}

// 0x439b10 (wordharmony)
bool Node::contains(Point& p)
{
	return mBounds.contains(p.x, p.y);
}

// 0x439d60 (wordharmony)
Node* Node::hitTest(unsigned int theFlags, Point& p)
{
	// 0x467e70 (pizzafrenzy): the flags with the mask (+0x74) taken out, then the shape
	if (gSignalBuild)
		return hasFlag(theFlags) && contains(p) ? this : nullptr;
	if (mVisible && mEnabled)
	{
		if (contains(p) && (mAllFlags & theFlags) != 0)
			return this;
	}
	return nullptr;
}

// 0x439b60 (wordharmony): the root's own position is not subtracted
void Node::toLocal(Point& p)
{
	Node* aParent = mParent;
	if (aParent != nullptr)
	{
		aParent->toLocal(p);
		p.x = p.x - mPosition.x;
		p.y = p.y - mPosition.y;
	}
}

// 0x439bd0 (wordharmony)
void Node::update(UpdateInfo& theInfo)
{
	mAbsPosition.x = mPosition.x + theInfo.mOrigin.x;
	mAbsPosition.y = mPosition.y + theInfo.mOrigin.y;
	notify(theInfo);
}

// 0x439c60 (wordharmony): the parent's link is a reference on the child
void Node::setParent(Node* theParent)
{
	mParent = theParent;
	addRef();
}

// 0x439c80 (wordharmony)
void Node::clearParent(Node* theParent)
{
	mParent = nullptr;
	release();
}

// 0x439000 (wordharmony)
void Node::setFlags(unsigned int theFlags)
{
	if ((theFlags & ~mAllFlags) != 0)
	{
		mAllFlags = mAllFlags | theFlags;
		if (mParent != nullptr)
			mParent->setFlags(theFlags);
	}
}

// 0x439cf0 (wordharmony)
void Node::clearFlags(unsigned int theFlags)
{
	if ((theFlags & ~mFlags & mAllFlags) != 0)
	{
		mAllFlags = (~theFlags & mAllFlags) | mFlags;
		if (mParent != nullptr)
			mParent->clearFlags(theFlags);
	}
}

// 0x439e60 (wordharmony)
void Node::notify(UpdateInfo& theInfo)
{
	std::vector<NodeListener*>::iterator anIt = mListeners.begin();
	if (anIt != mListeners.end())
	{
		do
		{
			(*anIt)->update(theInfo, this);
			++anIt;
		} while (anIt != mListeners.end());
	}
}

// 0x439cb0 (wordharmony)
void Node::addOwnFlags(unsigned int theFlags)
{
	mFlags = mFlags | theFlags;
	setFlags(theFlags);
}

// 0x439cd0 (wordharmony)
void Node::removeOwnFlags(unsigned int theFlags)
{
	mFlags = mFlags & ~theFlags;
	clearFlags(theFlags);
}

// 0x43a510 (wordharmony), 0x468df0 (pizzafrenzy)
void Node::addListener(NodeListener* theListener)
{
	if (std::find(mListeners.begin(), mListeners.end(), theListener) == mListeners.end())
	{
		theListener->addRef();
		mListeners.push_back(theListener);
	}
	mFlags = mFlags | NODE_LISTENERS;
	setFlags(NODE_LISTENERS);
}

// 0x438fe0 (wordharmony); 0x467ef0 (pizzafrenzy)
void Node::setEnabled(bool theEnabled)
{
	if (!gSignalBuild)
	{
		mEnabled = theEnabled;
		return;
	}
	if (theEnabled)
		addOwnFlags(NODE_MOUSE);
	else
		removeOwnFlags(NODE_MOUSE);
}

// 0x439bc0 (wordharmony); 0x467f20 (pizzafrenzy)
bool Node::isEnabled()
{
	if (!gSignalBuild)
		return mEnabled;
	return !mMouseMasked && hasOwnFlag(NODE_MOUSE);
}

// 0x4670e0 (pizzafrenzy)
void Node::refreshMouse()
{
	if (mParent != nullptr)
		mParent->refreshMouse();
}

// 0x468280 (pizzafrenzy)
void Node::removeListener(NodeListener* theListener)
{
	std::vector<NodeListener*>::iterator anIt = std::find(mListeners.begin(), mListeners.end(), theListener);
	if (anIt != mListeners.end())
	{
		mListeners.erase(anIt);
		theListener->release();
	}
}

// 0x439fa0 (wordharmony)
void Node::removeAllListeners()
{
	for (size_t i = 0; i < mListeners.size(); i++)
		mListeners[i]->release();
	mListeners.clear();
}

// 0x40f110 (wordharmony)
Container::Container()
{
}

// 0x439460 (wordharmony)
Container::~Container()
{
	removeAllChildren();
}

// 0x4390f0 (wordharmony): the point becomes the container's local point; each child starts from it, and the last child
// hit wins
Node* Container::hitTest(unsigned int theFlags, Point& thePoint)
{
	if (gSignalBuild)
		return hitTestSignal(theFlags, thePoint);
	if (!mEnabled || contains(thePoint))
	{
		Node* aHit = nullptr;
		thePoint.x = thePoint.x - mPosition.x;
		thePoint.y = thePoint.y - mPosition.y;
		Point aLocal = thePoint;
		std::vector<Node*>::iterator anIt = mChildren.begin();
		if (anIt != mChildren.end())
		{
			do
			{
				thePoint = aLocal;
				Node* aChildHit = (*anIt)->hitTest(theFlags, thePoint);
				if (aChildHit != nullptr)
					aHit = aChildHit;
				++anIt;
			} while (anIt != mChildren.end());
			if (aHit != nullptr)
				return aHit;
		}
		if (mEnabled && mVisible && hasFlag(theFlags))
			return this;
	}
	return nullptr;
}

// 0x467110 (pizzafrenzy): a container whose flags (its children's among them) match and that holds the point tries
// its children from the last added (the top) down, the first hit winning; it is the target itself only when its own
// flags (+0x6c, 0x467e30 (pizzafrenzy)) match, so a HUD with buttons in it lets a press through to the screen under it
Node* Container::hitTestSignal(unsigned int theFlags, Point& thePoint)
{
	if (!hasFlag(theFlags) || !contains(thePoint))
		return nullptr;
	thePoint.x = thePoint.x - mPosition.x;
	thePoint.y = thePoint.y - mPosition.y;
	Point aLocal = thePoint;
	for (std::vector<Node*>::reverse_iterator anIt = mChildren.rbegin(); anIt != mChildren.rend(); ++anIt)
	{
		Node* aHit = (*anIt)->hitTest(theFlags, thePoint);
		if (aHit != nullptr)
			return aHit;
		thePoint = aLocal;
	}
	if ((mFlags & ~getMaskedFlags() & theFlags) != 0)
		return this;
	return nullptr;
}

// 0x4392b0 (wordharmony), 0x4672b0 (pizzafrenzy): the signal build looks only at each child's drawable flag, not at
// the container's own visibility (a hidden screen still draws when it is asked to, as the city select's postcards ask
// the game screen)
void Container::draw(Graphics* g)
{
	if (gSignalBuild || mVisible)
	{
		std::vector<Node*>::iterator anIt = mChildren.begin();
		if (anIt != mChildren.end())
		{
			do
			{
				if ((*anIt)->hasFlag(NODE_DRAWABLE))
				{
					g->pushState();
					(*anIt)->applyTransform(g);
					(*anIt)->draw(g);
					g->popState();
				}
				++anIt;
			} while (anIt != mChildren.end());
		}
	}
}

// 0x439510 (wordharmony): updates the children that have listeners (or wait to be removed), and removes those that
// asked to be. A child that takes itself out of this container while it updates (Feeding Frenzy's shield glow, 0x4190d0
// feedingfrenzy, moves between two groups) can leave the index past the end, so the loop stops at the end.
void Container::update(UpdateInfo& theInfo)
{
	Node::update(theInfo);
	theInfo.mOrigin.x = theInfo.mOrigin.x + mPosition.x;
	theInfo.mOrigin.y = theInfo.mOrigin.y + mPosition.y;
	size_t i = 0;
	if (i != mChildren.size())
	{
		do
		{
			Node* aChild = mChildren[i];
			if (aChild->hasFlag(NODE_LISTENERS | NODE_REMOVE))
				aChild->update(theInfo);
			if (aChild->hasOwnFlag(NODE_REMOVE))
			{
				mChildren[i]->clearParent(this);
				mChildren.erase(mChildren.begin() + i);
				clearFlags(~mFlags);
				setFlags(NODE_LAYOUT);
			}
			else
				i++;
		} while (i < mChildren.size());
	}
	theInfo.mOrigin.x = theInfo.mOrigin.x - mPosition.x;
	theInfo.mOrigin.y = theInfo.mOrigin.y - mPosition.y;
}

// 0x439210 (wordharmony): the bounds are the rectangle united with the children's bounds, scaled and moved to the
// position
void Container::layout()
{
	mBounds = mRect;
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			if ((*anIt)->hasFlag(NODE_LAYOUT))
				(*anIt)->layout();
			mBounds.unite(*(*anIt)->getBounds());
			++anIt;
		} while (anIt != mChildren.end());
	}
	mBounds.scale(fabsf(mScale.x), fabsf(mScale.y));
	mBounds.offset(mPosition.x, mPosition.y);
	clearFlags(NODE_LAYOUT);
}

// 0x439310 (wordharmony): the flags are those not cleared, the own ones and the children's
void Container::clearFlags(unsigned int theFlags)
{
	if ((theFlags & ~mFlags & mAllFlags) != 0)
	{
		mAllFlags = (~theFlags & mAllFlags) | mFlags;
		std::vector<Node*>::iterator anIt = mChildren.begin();
		if (anIt != mChildren.end())
		{
			do
			{
				mAllFlags = mAllFlags | (*anIt)->getAllFlags();
				++anIt;
			} while (anIt != mChildren.end());
		}
		if (mParent != nullptr)
			mParent->clearFlags(theFlags);
	}
}

// 0x439380 (wordharmony)
void Container::eraseChild(size_t theIndex)
{
	mChildren[theIndex]->clearParent(this);
	mChildren.erase(mChildren.begin() + theIndex);
	clearFlags(~mFlags);
	setFlags(NODE_LAYOUT);
}

// 0x4395e0 (wordharmony)
void Container::removeChild(Node* theChild)
{
	std::vector<Node*>::iterator anIt = std::find(mChildren.begin(), mChildren.end(), theChild);
	if (anIt != mChildren.end())
		eraseChild(anIt - mChildren.begin());
}

// 0x4399b0 (wordharmony)
void Container::addChild(Node* theChild)
{
	if (std::find(mChildren.begin(), mChildren.end(), theChild) != mChildren.end())
		return;
	mChildren.push_back(theChild);
	setFlags(theChild->getAllFlags());
	setFlags(NODE_LAYOUT);
	theChild->setParent(this);
}

// 0x4671b0 (pizzafrenzy)
void Container::connectChildren()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			(*anIt)->connect();
			++anIt;
		} while (anIt != mChildren.end());
	}
}

// 0x4671e0 (pizzafrenzy)
void Container::disconnectChildren()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			(*anIt)->disconnect();
			++anIt;
		} while (anIt != mChildren.end());
	}
}

// 0x4393e0 (wordharmony)
void Container::removeAllChildren()
{
	std::vector<Node*> aChildren;
	aChildren.swap(mChildren);
	std::vector<Node*>::iterator anIt = aChildren.begin();
	if (anIt != aChildren.end())
	{
		do
		{
			(*anIt)->clearParent(this);
			++anIt;
		} while (anIt != aChildren.end());
	}
	clearFlags(~mFlags);
	setFlags(NODE_LAYOUT);
}

// 0x438e70 (wordharmony)
Window::Window()
	: mHover(nullptr)
{
	mEnabled = false;
	setName("Window");
}

// 0x438a50 (wordharmony)
Window::~Window()
{
	if (mHover != nullptr)
		mHover->release();
}

// 0x438f80 (wordharmony)
void Window::addKeyHandler(KeyHandler* theHandler)
{
	if (std::find(mKeyHandlers.begin(), mKeyHandlers.end(), theHandler) != mKeyHandlers.end())
		return;
	mKeyHandlers.push_back(theHandler);
	theHandler->addRef();
}

// 0x438b40 (wordharmony)
void Window::removeKeyHandler(KeyHandler* theHandler)
{
	std::vector<KeyHandler*>::iterator anIt = std::find(mKeyHandlers.begin(), mKeyHandlers.end(), theHandler);
	if (anIt != mKeyHandlers.end())
	{
		theHandler->release();
		mKeyHandlers.erase(anIt);
	}
}

// 0x438770 (wordharmony): the last handler added first, until one takes the key
void Window::keyDown(int theKey)
{
	size_t i = mKeyHandlers.size();
	if (i != 0)
	{
		do
		{
			if (mKeyHandlers[i - 1]->keyDown(theKey))
				return;
			i--;
		} while (i != 0);
	}
}

// 0x4387b0 (wordharmony)
void Window::keyUp(int theKey)
{
	size_t i = mKeyHandlers.size();
	if (i != 0)
	{
		do
		{
			if (mKeyHandlers[i - 1]->keyUp(theKey))
				return;
			i--;
		} while (i != 0);
	}
}

// 0x4386c0 (wordharmony): a hover node that only the window still holds (removed from the scene) is dropped first
void Window::setHover(Node* theNode, Point& thePoint)
{
	if (mHover != nullptr && mHover->refCount() == 1)
	{
		if (mHover != nullptr)
			mHover->release();
		mHover = nullptr;
	}
	if (theNode != mHover)
	{
		if (mHover != nullptr)
			mHover->mouseExit(thePoint);
		if (theNode != nullptr)
			theNode->addRef();
		if (mHover != nullptr)
			mHover->release();
		mHover = theNode;
		if (theNode != nullptr)
			theNode->mouseEnter(thePoint);
	}
}

// The common part of 0x4387f0 (wordharmony), 0x438860 (wordharmony), 0x4388d0 (wordharmony), 0x438940 (wordharmony) and
// 0x4389b0 (wordharmony): the node under the mouse that takes it
Node* Window::mouseTarget(const Point& p, Point& theLocal)
{
	theLocal = p;
	Node* aNode = hitTest(NODE_MOUSE, theLocal);
	setHover(aNode, theLocal);
	if (aNode != nullptr && aNode->hasFlag(NODE_MOUSE))
		return aNode;
	return nullptr;
}

// 0x4387f0 (wordharmony)
void Window::handleMouseDown(const Point& thePoint)
{
	Point aLocal;
	Node* aNode = mouseTarget(thePoint, aLocal);
	if (aNode != nullptr)
		aNode->mouseDown(aLocal);
}

// 0x438860 (wordharmony)
void Window::handleMouseUp(const Point& thePoint)
{
	Point aLocal;
	Node* aNode = mouseTarget(thePoint, aLocal);
	if (aNode != nullptr)
		aNode->mouseUp(aLocal);
}

// 0x4388d0 (wordharmony)
void Window::handleRightDown(const Point& thePoint)
{
	Point aLocal;
	Node* aNode = mouseTarget(thePoint, aLocal);
	if (aNode != nullptr)
		aNode->rightDown(aLocal);
}

// 0x438940 (wordharmony)
void Window::handleRightUp(const Point& thePoint)
{
	Point aLocal;
	Node* aNode = mouseTarget(thePoint, aLocal);
	if (aNode != nullptr)
		aNode->rightUp(aLocal);
}

// 0x4389b0 (wordharmony)
void Window::handleMouseMove(const Point& thePoint)
{
	Point aLocal;
	Node* aNode = mouseTarget(thePoint, aLocal);
	if (aNode != nullptr)
		aNode->mouseMove(aLocal);
}

// The signal build's mouse slots: the node's signal, then the handler

// 0x4680c0 (pizzafrenzy)
void Node::mouseDownEvent(MouseEvent& e)
{
	mMouseDownSignal.emit(e);
	handleMouseDownEvent(e);
}

// 0x468100 (pizzafrenzy)
void Node::mouseUpEvent(MouseEvent& e)
{
	mMouseUpSignal.emit(e);
	handleMouseUpEvent(e);
}

// 0x468140 (pizzafrenzy)
void Node::rightDownEvent(MouseEvent& e)
{
	mRightDownSignal.emit(e);
	handleRightDownEvent(e);
}

// 0x468180 (pizzafrenzy)
void Node::rightUpEvent(MouseEvent& e)
{
	mRightUpSignal.emit(e);
	handleRightUpEvent(e);
}

// 0x4681c0 (pizzafrenzy)
void Node::mouseMoveEvent(MouseEvent& e)
{
	mMouseMoveSignal.emit(e);
	handleMouseMoveEvent(e);
}

// 0x468200 (pizzafrenzy)
void Node::mouseEnterEvent(MouseEvent& e)
{
	mMouseEnterSignal.emit(e);
	handleMouseEnterEvent(e);
}

// 0x468240 (pizzafrenzy)
void Node::mouseExitEvent(MouseEvent& e)
{
	mMouseExitSignal.emit(e);
	handleMouseExitEvent(e);
}
