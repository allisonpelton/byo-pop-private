// SharedPtr.h: the reference counted objects of a level (the original's Sexy::SmartPtr<T>, whose objects count themselves in
// the word after their vftable). The counts are not atomic: only the game's own thread touches the pieces.

#ifndef __SHAREDPTR_H__
#define __SHAREDPTR_H__

namespace Sexy
{

class RefCounted
{
public:
	RefCounted() : mRefCount(0) {}
	RefCounted(const RefCounted&) : mRefCount(0) {}				// a copy is a new object with no holders
	RefCounted& operator=(const RefCounted&) { return *this; } // and assigning one keeps the count
	virtual ~RefCounted() {}

	void AddRef() { mRefCount++; }
	void Release()
	{
		mRefCount--;
		if (mRefCount < 1)
			delete this;
	}

	int mRefCount; // +4
};

template <typename T>
class SharedPtr
{
public:
	SharedPtr() : mObject(nullptr) {}
	SharedPtr(T* theObject) : mObject(theObject)
	{
		if (mObject != nullptr)
			mObject->AddRef();
	}
	SharedPtr(const SharedPtr& theOther) : mObject(theOther.mObject)
	{
		if (mObject != nullptr)
			mObject->AddRef();
	}
	~SharedPtr()
	{
		if (mObject != nullptr)
			mObject->Release();
	}

	SharedPtr& operator=(T* theObject)
	{
		if (theObject != nullptr)
			theObject->AddRef();
		if (mObject != nullptr)
			mObject->Release();
		mObject = theObject;
		return *this;
	}
	SharedPtr& operator=(const SharedPtr& theOther) { return *this = theOther.mObject; }

	T* operator->() const { return mObject; }
	T& operator*() const { return *mObject; }
	T* get() const { return mObject; }
	operator T*() const { return mObject; }

private:
	T* mObject;
};

} // namespace Sexy

#endif
