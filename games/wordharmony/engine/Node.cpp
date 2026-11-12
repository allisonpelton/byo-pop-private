#include "Node.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace engine;

// 0x43a3c0
Node::Node()
	: mParent(nullptr), mPosition(0, 0), mScale(1.0f, 1.0f), mRotation(0), mOpacity(1.0f), mFont(nullptr), mFlags(0),
	  mAllFlags(0), mVisible(true), mEnabled(true)
{
}

// 0x439e90: the listeners' memory goes, not their references (0x439fa0 releases them)
Node::~Node()
{
	if (mFont != nullptr)
		mFont->release();
}

// 0x439aa0
void Node::setPosition(const Point& thePosition)
{
	mPosition = Point(thePosition.x, thePosition.y);
	setFlags(NODE_LAYOUT);
}

// 0x439a70
void Node::setPosition(float x, float y)
{
	mPosition = Point(x, y);
	setFlags(NODE_LAYOUT);
}

// 0x439ae0
void Node::setScale(float theScaleX, float theScaleY)
{
	mScale.x = theScaleX;
	mScale.y = theScaleY;
	setFlags(NODE_LAYOUT);
}

// 0x439da0
void Node::setFont(Font* theFont)
{
	if (theFont != nullptr)
		theFont->addRef();
	if (mFont != nullptr)
		mFont->release();
	mFont = theFont;
	setFlags(NODE_LAYOUT);
}

// 0x43a050: (the original also kept a map from names to nodes in the application, which only the particle streams'
// anchors read, and no data has one)
void Node::setName(const std::string& theName) { mName = theName; }

// 0x439df0
void Node::applyTransform(Graphics* g)
{
	g->translate(mPosition.x, mPosition.y);
	g->scale(mScale.x, mScale.y);
	g->rotate(mRotation);
	g->multiplyOpacity(mOpacity);
	g->addColorOffset(mColorOffset.r, mColorOffset.g, mColorOffset.b);
	if (mFont != nullptr)
		g->setFont(mFont);
}

// 0x439b10
bool Node::contains(Point& p)
{
	return mBounds.contains(p.x, p.y);
}

// 0x439d60
Node* Node::hitTest(unsigned int theFlags, Point& p)
{
	if (mVisible && mEnabled)
	{
		if (contains(p) && (mAllFlags & theFlags) != 0)
			return this;
	}
	return nullptr;
}

// 0x439b60: the root's own position is not subtracted
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

// 0x439bd0
void Node::update(UpdateInfo& theInfo)
{
	mAbsPosition.x = mPosition.x + theInfo.mOrigin.x;
	mAbsPosition.y = mPosition.y + theInfo.mOrigin.y;
	notify(theInfo);
}

// 0x439c60: the parent's link is a reference on the child
void Node::setParent(Node* theParent)
{
	mParent = theParent;
	addRef();
}

// 0x439c80
void Node::clearParent(Node* theParent)
{
	mParent = nullptr;
	release();
}

// 0x439000
void Node::setFlags(unsigned int theFlags)
{
	if ((theFlags & ~mAllFlags) != 0)
	{
		mAllFlags = mAllFlags | theFlags;
		if (mParent != nullptr)
			mParent->setFlags(theFlags);
	}
}

// 0x439cf0
void Node::clearFlags(unsigned int theFlags)
{
	if ((theFlags & ~mFlags & mAllFlags) != 0)
	{
		mAllFlags = (~theFlags & mAllFlags) | mFlags;
		if (mParent != nullptr)
			mParent->clearFlags(theFlags);
	}
}

// 0x439e60
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

// 0x439cb0
void Node::addOwnFlags(unsigned int theFlags)
{
	mFlags = mFlags | theFlags;
	setFlags(theFlags);
}

// 0x439cd0
void Node::removeOwnFlags(unsigned int theFlags)
{
	mFlags = mFlags & ~theFlags;
	clearFlags(theFlags);
}

// 0x43a510
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

// 0x439fa0
void Node::removeAllListeners()
{
	for (size_t i = 0; i < mListeners.size(); i++)
		mListeners[i]->release();
	mListeners.clear();
}

// 0x40f110
Container::Container()
{
}

// 0x439460
Container::~Container()
{
	removeAllChildren();
}

// 0x4390f0: the point becomes the container's local point; each child starts from it, and the last child hit wins
Node* Container::hitTest(unsigned int theFlags, Point& p)
{
	if (!mEnabled || contains(p))
	{
		Node* aHit = nullptr;
		p.x = p.x - mPosition.x;
		p.y = p.y - mPosition.y;
		Point aLocal = p;
		std::vector<Node*>::iterator anIt = mChildren.begin();
		if (anIt != mChildren.end())
		{
			do
			{
				p = aLocal;
				Node* aChildHit = (*anIt)->hitTest(theFlags, p);
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

// 0x4392b0
void Container::draw(Graphics* g)
{
	if (mVisible)
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

// 0x439510: updates the children that have listeners (or wait to be removed), and removes those that asked to be
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
		} while (i != mChildren.size());
	}
	theInfo.mOrigin.x = theInfo.mOrigin.x - mPosition.x;
	theInfo.mOrigin.y = theInfo.mOrigin.y - mPosition.y;
}

// 0x439210: the bounds are the rectangle united with the children's bounds, scaled and moved to the position
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

// 0x439310: the flags are those not cleared, the own ones and the children's
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

// 0x439380
void Container::eraseChild(size_t theIndex)
{
	mChildren[theIndex]->clearParent(this);
	mChildren.erase(mChildren.begin() + theIndex);
	clearFlags(~mFlags);
	setFlags(NODE_LAYOUT);
}

// 0x4395e0
void Container::removeChild(Node* theChild)
{
	std::vector<Node*>::iterator anIt = std::find(mChildren.begin(), mChildren.end(), theChild);
	if (anIt != mChildren.end())
		eraseChild(anIt - mChildren.begin());
}

// 0x4399b0
void Container::addChild(Node* theChild)
{
	if (std::find(mChildren.begin(), mChildren.end(), theChild) != mChildren.end())
		return;
	mChildren.push_back(theChild);
	setFlags(theChild->getAllFlags());
	setFlags(NODE_LAYOUT);
	theChild->setParent(this);
}

// 0x4393e0
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

// 0x438e70
Window::Window()
	: mHover(nullptr), mUnknownBC(0, 0)
{
	mEnabled = false;
	setName("Window");
}

// 0x438a50
Window::~Window()
{
	if (mHover != nullptr)
		mHover->release();
}

// 0x438f80
void Window::addKeyHandler(KeyHandler* theHandler)
{
	if (std::find(mKeyHandlers.begin(), mKeyHandlers.end(), theHandler) != mKeyHandlers.end())
		return;
	mKeyHandlers.push_back(theHandler);
	theHandler->addRef();
}

// 0x438b40
void Window::removeKeyHandler(KeyHandler* theHandler)
{
	std::vector<KeyHandler*>::iterator anIt = std::find(mKeyHandlers.begin(), mKeyHandlers.end(), theHandler);
	if (anIt != mKeyHandlers.end())
	{
		theHandler->release();
		mKeyHandlers.erase(anIt);
	}
}

// 0x438770: the last handler added first, until one takes the key
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

// 0x4387b0
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

// 0x4386c0: a hover node that only the window still holds (removed from the scene) is dropped first
void Window::setHover(Node* theNode, Point& p)
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
			mHover->mouseExit(p);
		if (theNode != nullptr)
			theNode->addRef();
		if (mHover != nullptr)
			mHover->release();
		mHover = theNode;
		if (theNode != nullptr)
			theNode->mouseEnter(p);
	}
}

// The common part of 0x4387f0, 0x438860, 0x4388d0, 0x438940 and 0x4389b0: the node under the mouse that takes it
Node* Window::mouseTarget(const Point& p, Point& theLocal)
{
	theLocal = p;
	Node* aNode = hitTest(NODE_MOUSE, theLocal);
	setHover(aNode, theLocal);
	if (aNode != nullptr && aNode->hasFlag(NODE_MOUSE))
		return aNode;
	return nullptr;
}

// 0x4387f0
void Window::handleMouseDown(const Point& p)
{
	Point aLocal;
	Node* aNode = mouseTarget(p, aLocal);
	if (aNode != nullptr)
		aNode->mouseDown(aLocal);
}

// 0x438860
void Window::handleMouseUp(const Point& p)
{
	Point aLocal;
	Node* aNode = mouseTarget(p, aLocal);
	if (aNode != nullptr)
		aNode->mouseUp(aLocal);
}

// 0x4388d0
void Window::handleRightDown(const Point& p)
{
	Point aLocal;
	Node* aNode = mouseTarget(p, aLocal);
	if (aNode != nullptr)
		aNode->rightDown(aLocal);
}

// 0x438940
void Window::handleRightUp(const Point& p)
{
	Point aLocal;
	Node* aNode = mouseTarget(p, aLocal);
	if (aNode != nullptr)
		aNode->rightUp(aLocal);
}

// 0x4389b0
void Window::handleMouseMove(const Point& p)
{
	Point aLocal;
	Node* aNode = mouseTarget(p, aLocal);
	if (aNode != nullptr)
		aNode->mouseMove(aLocal);
}
