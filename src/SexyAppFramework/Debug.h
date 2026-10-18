#ifndef __DEBUG_INCLUDED__
#define __DEBUG_INCLUDED__

// port: reduced version of the framework's Debug.h (Debug.cpp/memory tracking is not ported).
// Asserts are compiled out like in the shipped release build; define SEXY_ENABLE_ASSERTS to turn
// them on for debugging.

#include "Common.h"
#include <cassert>

#define SEXY_TRACE(theStr)

extern void SexyTraceFmt(const SexyChar* fmt ...);
extern void OutputDebug(const SexyChar* fmt ...);

#ifdef SEXY_ENABLE_ASSERTS

#define DBG_ASSERTE(exp)	assert(exp)
#define DBG_ASSERT(exp)		assert(exp)

#else

#define DBG_ASSERTE(exp)	((void)0)
#define DBG_ASSERT(exp)		((void)0)

#endif // SEXY_ENABLE_ASSERTS

#endif //__DEBUG_INCLUDED__
