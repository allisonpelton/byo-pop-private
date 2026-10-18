#ifndef __AUTOCRIT_INCLUDED__
#define __AUTOCRIT_INCLUDED__

#include "Common.h"
#include "CritSect.h"

namespace Sexy
{

class AutoCrit
{
	std::recursive_mutex*	mCritSec;
public:
	AutoCrit(const CritSect& theCritSect) : 
		mCritSec(const_cast<std::recursive_mutex*>(&theCritSect.mCriticalSection))
	{ 
		mCritSec->lock(); 
	}

	~AutoCrit()
	{ 
		mCritSec->unlock(); 
	}
};

}

#endif //__AUTOCRIT_INCLUDED__
