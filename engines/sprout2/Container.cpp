#include "Container.h"

#include "DrawContext.h"

#include <algorithm>

namespace engine
{

Container::Container()
{
}

Component* Container::hitTest(int theStates, Vector2f& thePoint)
{
	if ((theStates & mTreeStates & ~mOffStates) == 0 || !contains(thePoint))
		return nullptr;
	thePoint.x = thePoint.x - mPosition.x;
	thePoint.y = thePoint.y - mPosition.y;
	Vector2f aLocalPoint = thePoint;
	for (size_t i = mChildren.size(); i > 0; i--)
	{
		Component* aHit = mChildren[i - 1]->hitTest(theStates, thePoint);
		if (aHit != nullptr)
			return aHit;
		thePoint = aLocalPoint;
	}
	if ((theStates & mOwnStates & ~mOffStates) == 0)
		return nullptr;
	return this;
}

void Container::enable()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->enable();
}

void Container::disable()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->disable();
}

void Container::beginComposite()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->beginComposite();
}

void Container::endComposite()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->endComposite();
}

void Container::paint(DrawContext* theContext)
{
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		Component* aChild = mChildren[i];
		if ((~aChild->getOffStates() & aChild->getTreeStates() & STATE_VISIBLE) == 0)
			continue;
		theContext->pushState();
		aChild->applyTransform(theContext);
		aChild->paint(theContext);
		// The original has popState's work written out here
		theContext->popState();
	}
}

void Container::update(UpdateContext& theContext)
{
	mAbsolutePosition.x = theContext.mPosition.x + mPosition.x;
	mAbsolutePosition.y = theContext.mPosition.y + mPosition.y;
	if (mPixelFx != nullptr)
		mPixelFx->update(theContext.mElapsed);
	applyModifiers(theContext);
	removeState(STATE_ANIMATED | STATE_DELETE | STATE_CHILDREN_CHANGED);

	// The original adds the position and takes it off again, rounding each time, rather than keeping a copy
	theContext.mPosition.x = theContext.mPosition.x + mPosition.x;
	theContext.mPosition.y = theContext.mPosition.y + mPosition.y;
	size_t i = 0;
	while (i < mChildren.size())
	{
		Component* aChild = mChildren[i];
		if ((~aChild->getOffStates() & aChild->getTreeStates() & (STATE_ANIMATED | STATE_DELETE | STATE_CHILDREN_CHANGED)) != 0)
			aChild->update(theContext);
		if ((~aChild->getOffStates() & aChild->getOwnStates() & STATE_DELETE) != 0)
		{
			aChild->removeOwnState(STATE_DELETE);
			eraseChild(i);
		}
		else
			i++;
	}
	theContext.mPosition.x = theContext.mPosition.x - mPosition.x;
	theContext.mPosition.y = theContext.mPosition.y - mPosition.y;
}

void Container::computeBounds()
{
	mBounds = mClipRect;
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		Component* aChild = mChildren[i];
		if ((~aChild->getOffStates() & aChild->getTreeStates() & STATE_BOUNDS_DIRTY) != 0)
			aChild->computeBounds();
		const RectF& aChildBounds = aChild->getBounds();
		mBounds.left = minOf(mBounds.left, aChildBounds.left);
		mBounds.right = maxOf(mBounds.right, aChildBounds.right);
		mBounds.top = minOf(mBounds.top, aChildBounds.top);
		mBounds.bottom = maxOf(mBounds.bottom, aChildBounds.bottom);
	}
	mBounds.left = mScale.x * mBounds.left;
	mBounds.right = mScale.x * mBounds.right;
	mBounds.top = mScale.y * mBounds.top;
	mBounds.bottom = mScale.y * mBounds.bottom;
	if (mBounds.left > mBounds.right)
		std::swap(mBounds.left, mBounds.right);
	if (mBounds.top > mBounds.bottom)
		std::swap(mBounds.top, mBounds.bottom);
	mBounds.left = mBounds.left + mPosition.x;
	mBounds.right = mBounds.right + mPosition.x;
	mBounds.top = mBounds.top + mPosition.y;
	mBounds.bottom = mBounds.bottom + mPosition.y;
	removeState(STATE_BOUNDS_DIRTY);
}

void Container::dump(int theIndent)
{
	Component::dump(theIndent);
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->dump(theIndent + 1);
	Engine* anEngine = getEngine();
	for (int i = 0; i < theIndent; i++)
		anEngine->log("\t");
	anEngine->log("</%s >\n", getClassName().c_str());
}

void Container::paintRegion(DrawContext* theContext, const RectF& theRegion)
{
	float aLeftOffset = -mPosition.x;
	float aTopOffset = -mPosition.y;
	RectF aRegion(theRegion.left + aLeftOffset, theRegion.top + aTopOffset, theRegion.right + aLeftOffset,
		aTopOffset + theRegion.bottom);
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->paintRegion(theContext, aRegion);
}

void Container::removeState(int theStates)
{
	if ((theStates & ~mOwnStates & mTreeStates) == 0)
		return;
	mTreeStates = (~theStates & mTreeStates) | mOwnStates;
	for (size_t i = 0; i < mChildren.size(); i++)
		mTreeStates |= mChildren[i]->getTreeStates();
	if (mParent != nullptr)
		mParent->removeState(theStates);
}

void Container::removeChild(Component* theChild)
{
	std::vector<Component*>::iterator anIt = std::find(mChildren.begin(), mChildren.end(), theChild);
	if (anIt != mChildren.end())
		eraseChild(anIt - mChildren.begin());
}

void Container::setClipRect(const RectF& theRect)
{
	mClipRect = theRect;
	addState(STATE_BOUNDS_DIRTY);
}

void Container::setClipRect(float theLeft, float theTop, float theRight, float theBottom)
{
	mClipRect = RectF(theLeft, theTop, theRight, theBottom);
	addState(STATE_BOUNDS_DIRTY);
}

void Container::addChild(Component* theChild)
{
	if (std::find(mChildren.begin(), mChildren.end(), theChild) != mChildren.end())
		return;
	mChildren.push_back(theChild);
	addState(theChild->getTreeStates());
	addState(STATE_BOUNDS_DIRTY);
	theChild->setParent(this);
	childAdded(theChild);
	addState(STATE_CHILDREN_CHANGED);
}

void Container::removeAllChildren()
{
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		mChildren[i]->clearParent();
		childRemoved(mChildren[i]);
	}
	mChildren.clear();
	removeState(~mOwnStates);
	addState(STATE_BOUNDS_DIRTY);
	addState(STATE_CHILDREN_CHANGED);
}

Component* Container::getChild(int theIndex)
{
	if (theIndex < 0 || theIndex >= getChildCount())
		return nullptr;
	return mChildren[theIndex];
}

void Container::eraseChild(size_t theIndex)
{
	Component* aChild = mChildren[theIndex];
	childRemoved(aChild);
	aChild->clearParent();
	mChildren.erase(mChildren.begin() + theIndex);
	removeState(~mOwnStates);
	addState(STATE_BOUNDS_DIRTY);
	addState(STATE_CHILDREN_CHANGED);
}

} // namespace engine
