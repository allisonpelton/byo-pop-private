#include "Object.h"
#include <cstdio>
#include <cstdlib>
#include "Cubby.h"

using namespace Sexy;

static ObjectManager* gObjectManager = nullptr;		// 0x5975a4

// The numbers that make up object names (0x4025c0): one count for each type name.
static std::map<std::string, int>& GetNameCounters()
{
	static std::map<std::string, int> aCounters;
	return aCounters;
}

// 0x4026f0: the next name of a type, "<type>_00001", then _00002 and so on
static std::string MakeName(const char* theTypeName)
{
	std::map<std::string, int>& aCounters = GetNameCounters();
	int aNumber;
	std::map<std::string, int>::iterator anItr = aCounters.find(theTypeName);
	if (anItr == aCounters.end())
	{
		aNumber = 1;
		aCounters[theTypeName] = 1;
	}
	else
		aNumber = ++anItr->second;

	char aBuffer[16];
	snprintf(aBuffer, sizeof(aBuffer), "_%05d", aNumber);
	return std::string(theTypeName) + aBuffer;
}

// 0x402840: a name given by hand moves the type's count to its last five digits plus one
static void NoteName(const char* theTypeName, const std::string& theName)
{
	if (theName.size() < 5)
		return;
	long aNumber = atol(theName.substr(theName.size() - 5).c_str());
	if (aNumber < 90000)
		GetNameCounters()[theTypeName] = (int)(aNumber + 1);
}

// The C library's qsort of the original's Visual C++ 7.1: a quicksort that finishes pieces of 8 elements or fewer with
// a selection sort. Its order for equal keys is part of how the cubbies are drawn, so the host's qsort is not used.
static void CrtSwap(char* a, char* b, size_t theWidth)
{
	if (a == b)
		return;
	for (size_t i = 0; i < theWidth; i++)
	{
		char aTemp = a[i];
		a[i] = b[i];
		b[i] = aTemp;
	}
}

static void CrtShortSort(char* lo, char* hi, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	while (hi > lo)
	{
		char* aMax = lo;
		for (char* p = lo + theWidth; p <= hi; p += theWidth)
			if (theCompare(p, aMax) > 0)
				aMax = p;
		CrtSwap(aMax, hi, theWidth);
		hi -= theWidth;
	}
}

static void CrtQSort(void* theBase, size_t theCount, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	const size_t CUTOFF = 8;
	char* lostk[30];
	char* histk[30];
	int aStackPos = 0;

	if (theCount < 2 || theWidth == 0)
		return;

	char* lo = (char*)theBase;
	char* hi = (char*)theBase + theWidth * (theCount - 1);
	for (;;)
	{
		size_t aSize = (hi - lo) / theWidth + 1;
		bool aRecurse = false;
		if (aSize <= CUTOFF)
			CrtShortSort(lo, hi, theWidth, theCompare);
		else
		{
			char* mid = lo + (aSize / 2) * theWidth;
			if (theCompare(lo, mid) > 0)
				CrtSwap(lo, mid, theWidth);
			if (theCompare(lo, hi) > 0)
				CrtSwap(lo, hi, theWidth);
			if (theCompare(mid, hi) > 0)
				CrtSwap(mid, hi, theWidth);

			char* loguy = lo;
			char* higuy = hi;
			for (;;)
			{
				if (mid > loguy)
				{
					do
						loguy += theWidth;
					while (loguy < mid && theCompare(loguy, mid) <= 0);
				}
				if (mid <= loguy)
				{
					do
						loguy += theWidth;
					while (loguy <= hi && theCompare(loguy, mid) <= 0);
				}
				do
					higuy -= theWidth;
				while (higuy > mid && theCompare(higuy, mid) > 0);
				if (higuy < loguy)
					break;
				CrtSwap(loguy, higuy, theWidth);
				if (mid == higuy)
					mid = loguy;
			}

			higuy += theWidth;
			if (mid < higuy)
			{
				do
					higuy -= theWidth;
				while (higuy > mid && theCompare(higuy, mid) == 0);
			}
			if (mid >= higuy)
			{
				do
					higuy -= theWidth;
				while (higuy > lo && theCompare(higuy, mid) == 0);
			}

			if (higuy - lo >= hi - loguy)
			{
				if (lo < higuy)
				{
					lostk[aStackPos] = lo;
					histk[aStackPos] = higuy;
					aStackPos++;
				}
				if (loguy < hi)
				{
					lo = loguy;
					aRecurse = true;
				}
			}
			else
			{
				if (loguy < hi)
				{
					lostk[aStackPos] = loguy;
					histk[aStackPos] = hi;
					aStackPos++;
				}
				if (lo < higuy)
				{
					hi = higuy;
					aRecurse = true;
				}
			}
		}
		if (aRecurse)
			continue;
		aStackPos--;
		if (aStackPos < 0)
			break;
		lo = lostk[aStackPos];
		hi = histk[aStackPos];
	}
}

// 0x402fd0: cubbies sort by ascending depth value
static int CompareCubbyDepth(const void* theFirst, const void* theSecond)
{
	double aFirstY = (*(Cubby* const*)theFirst)->mY;
	double aSecondY = (*(Cubby* const*)theSecond)->mY;
	if (aFirstY == aSecondY)
		return 0;
	return aSecondY <= aFirstY ? 1 : -1;
}


// ObjectRef

// 0x402f50
ObjectRef::ObjectRef() :
	mObject(nullptr)
{
}

// 0x402eb0: a ref to the object's name (a blank name is not registered)
ObjectRef::ObjectRef(const Object* theObject) :
	mName(theObject->mName),
	mObject(nullptr)
{
	if (!mName.empty())
		ObjectManager::Get()->AddRef(this);
}

ObjectRef::ObjectRef(const ObjectRef& theRef) :
	mName(theRef.mName),
	mObject(nullptr)
{
	if (!mName.empty())
		ObjectManager::Get()->AddRef(this);
}

// 0x402e40
ObjectRef::~ObjectRef()
{
	if (ObjectManager* aManager = ObjectManager::Existing())
		aManager->RemoveRef(this);
}

// 0x402f80
ObjectRef& ObjectRef::operator=(const ObjectRef& theRef)
{
	SetName(theRef.mName);
	return *this;
}

void ObjectRef::SetName(const std::string& theName)
{
	if (!mName.empty())
		ObjectManager::Get()->RemoveRef(this);
	mObject = nullptr;
	mName = theName;
	if (!mName.empty())
		ObjectManager::Get()->AddRef(this);
}

void ObjectRef::Set(const Object* theObject)
{
	ObjectRef aTemp(theObject);
	*this = aTemp;
}

// 0x402dc0
bool ObjectRef::IsValid() const
{
	return mObject != nullptr && !mObject->mDead;
}

// 0x402de0
Object* ObjectRef::Get() const
{
	return IsValid() ? mObject : nullptr;
}

// 0x402e00
void ObjectRef::Kill() const
{
	if (IsValid())
		Get()->Kill();
}


// Object

// 0x402ae0
Object::Object(int theLayers) :
	mDead(false),
	mLayers(theLayers),
	mActive(true),
	mTickCount(0),
	mAge(0)
{
}

// 0x402b70
Object::~Object()
{
	if (ObjectManager* aManager = ObjectManager::Existing())
		aManager->Remove(this);
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i].Kill();
}

// 0x402c90
void Object::SetName(const std::string& theName)
{
	if (theName.empty())
		mName = MakeName(TypeName());
	else
	{
		NoteName(TypeName(), theName);
		mName = theName;
	}
	ObjectManager::Get()->Add(this);
}

// 0x402a00
void Object::FixedUpdate()
{
	if (mTickCount == 0)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
			if (Object* aChild = mChildren[i].Get())
				aChild->mParent.Set(this);
	}
	mTickCount++;
}

// 0x402950
void Object::Update(float theFrac)
{
	mAge = theFrac * 0.01 + mAge;
}

// 0x402970
void Object::Kill()
{
	mDead = true;
}

// 0x41ee00
void Object::Draw(Graphics* g)
{
}


// SimpleObject

// 0x405c20
SimpleObject::SimpleObject(int theLayers) :
	Object(theLayers),
	mBounds(0, 0, 0, 0),
	mX(0),
	mY(0)
{
}

// 0x405c00
SimpleObject::~SimpleObject()
{
}


// ObjectManager

// 0x4042d0
ObjectManager::ObjectManager() :
	mCurrentLayer(0),
	mFixedStepCount(0),
	mAccumulator(0)
{
	gObjectManager = this;
}

ObjectManager::~ObjectManager()
{
	if (gObjectManager == this)
		gObjectManager = nullptr;
}

// 0x404380
ObjectManager* ObjectManager::Get()
{
	if (gObjectManager == nullptr)
		new ObjectManager();
	return gObjectManager;
}

ObjectManager* ObjectManager::Existing()
{
	return gObjectManager;
}

// 0x4044a0: registers the object under its name and points the refs of that name at it
void ObjectManager::Add(Object* theObject)
{
	mObjects.push_back(theObject);
	mByName[theObject->mName] = theObject;
	for (size_t i = 0; i < mRefs.size(); i++)
		if (mRefs[i]->mName == theObject->mName)
			mRefs[i]->mObject = theObject;

	if (typeid(*theObject) == typeid(Cubby))
	{
		mCubbiesByDepth.push_back(static_cast<Cubby*>(theObject));
		CrtQSort(&mCubbiesByDepth[0], mCubbiesByDepth.size(), sizeof(Cubby*), CompareCubbyDepth);
	}
}

// 0x4039f0: the object registered under this object's name leaves the manager and the refs of that name go null
void ObjectManager::Remove(Object* theObject)
{
	std::map<std::string, Object*>::iterator anItr = mByName.find(theObject->mName);
	if (anItr == mByName.end())
		return;

	Object* aRegistered = anItr->second;
	for (size_t i = 0; i < mObjects.size(); i++)
		if (mObjects[i] == aRegistered)
		{
			mObjects.erase(mObjects.begin() + i);
			i--;
		}
	for (size_t i = 0; i < mCubbiesByDepth.size(); i++)
		if (mCubbiesByDepth[i] == aRegistered)
		{
			mCubbiesByDepth.erase(mCubbiesByDepth.begin() + i);
			i--;
		}
	mByName.erase(anItr);

	for (size_t i = 0; i < mRefs.size(); i++)
		if (mRefs[i]->mName == theObject->mName)
			mRefs[i]->mObject = nullptr;
}

// 0x403d70: every object goes, in order
void ObjectManager::Clear()
{
	while (!mObjects.empty())
	{
		Object* anObject = mObjects[0];
		if (!anObject->mDead)
			anObject->Kill();
		delete anObject;
	}
	mByName.clear();
	mCubbiesByDepth.clear();
}

// 0x403100: one step; dead objects are deleted
void ObjectManager::FixedUpdate()
{
	mFixedStepCount++;
	for (int i = 0; i < (int)mObjects.size(); i++)
	{
		Object* anObject = mObjects[i];
		if (anObject->mActive)
		{
			if (!anObject->mDead)
				anObject->FixedUpdate();
			else
			{
				delete mObjects[i];
				i--;
			}
		}
	}
}

// 0x403160
void ObjectManager::Update(float theFrac)
{
	mAccumulator = theFrac * 0.01 + mAccumulator;
	while (mAccumulator > 0.01)
	{
		FixedUpdate();
		mAccumulator = mAccumulator - 0.01;
	}

	for (int i = 0; i < (int)mObjects.size(); i++)
	{
		Object* anObject = mObjects[i];
		if (anObject->mActive)
		{
			if (!anObject->mDead)
				anObject->Update(theFrac);
			else
			{
				delete mObjects[i];
				i--;
			}
		}
	}
}

// 0x403200
void ObjectManager::Draw(Graphics* g)
{
	for (mCurrentLayer = 1; mCurrentLayer < 0x1000; mCurrentLayer *= 2)
	{
		if (mCurrentLayer == LAYER_CUBBIES)
		{
			for (size_t i = 0; i < mCubbiesByDepth.size(); i++)
				if (mCubbiesByDepth[i]->mActive)
					mCubbiesByDepth[i]->Draw(g);
		}
		else
		{
			for (size_t i = 0; i < mObjects.size(); i++)
			{
				Object* anObject = mObjects[i];
				if (anObject->mActive && (anObject->mLayers & mCurrentLayer) == mCurrentLayer)
					anObject->Draw(g);
			}
		}
	}
	mCurrentLayer = 0;
}

// 0x403640
Object* ObjectManager::FindByName(const std::string& theName)
{
	std::map<std::string, Object*>::iterator anItr = mByName.find(theName);
	return anItr != mByName.end() ? anItr->second : nullptr;
}

// 0x403500: the ref points at the first object of its name
void ObjectManager::AddRef(ObjectRef* theRef)
{
	mRefs.push_back(theRef);
	for (size_t i = 0; i < mObjects.size(); i++)
		if (mObjects[i]->mName == theRef->mName)
		{
			theRef->mObject = mObjects[i];
			return;
		}
}

// 0x403410
void ObjectManager::RemoveRef(ObjectRef* theRef)
{
	for (size_t i = 0; i < mRefs.size(); i++)
		if (mRefs[i] == theRef)
		{
			mRefs.erase(mRefs.begin() + i);
			i--;
		}
}

// 0x403340
void ObjectManager::ActivateAll()
{
	for (size_t i = 0; i < mObjects.size(); i++)
		mObjects[i]->mActive = true;
}

// 0x403310
void ObjectManager::DeactivateAll()
{
	for (size_t i = 0; i < mObjects.size(); i++)
		mObjects[i]->mActive = false;
}

// 0x403370: every object whose active flag is theActive goes
void ObjectManager::RemoveByActive(bool theActive)
{
	for (size_t i = 0; i < mCubbiesByDepth.size(); i++)
		if (mCubbiesByDepth[i]->mActive == theActive)
		{
			mCubbiesByDepth.erase(mCubbiesByDepth.begin() + i);
			i--;
		}
	for (size_t i = 0; i < mObjects.size(); i++)
		if (mObjects[i]->mActive == theActive)
		{
			if (!mObjects[i]->mDead)
				mObjects[i]->Kill();
			delete mObjects[i];
			i--;
		}
}
