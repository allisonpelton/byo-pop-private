#ifndef _H_CritSect
#define _H_CritSect

#include "Common.h"
#include <mutex>

namespace Sexy
{

// port: CRITICAL_SECTION replaced by std::recursive_mutex (same re-entrant semantics)
class CritSect 
{
private:
	std::recursive_mutex mCriticalSection;
	friend class AutoCrit;

public:
	CritSect(void) {}
	~CritSect(void) {}
};

}

#endif // _H_CritSect
