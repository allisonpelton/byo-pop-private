#ifndef __SEXY_PERFTIMER_H__
#define __SEXY_PERFTIMER_H__

// port: the profiling helpers of the framework are not ported; the macros compile to nothing (as in
// builds without SEXY_PERF_ENABLED).

#include "Common.h"

#define SEXY_PERF_BEGIN(theName) 
#define SEXY_PERF_END(theName) 
#define SEXY_AUTO_PERF_MULTI(theName,theSuffix)
#define SEXY_AUTO_PERF(theName) 

#define SEXY_PERF_BEGIN_COND(theName,theCond) 
#define SEXY_PERF_END_COND(theName,theCond) 
#define SEXY_AUTO_PERF_MULTI_COND(theName,theSuffix,theCond) 
#define SEXY_AUTO_PERF_COND(theName) 

#endif
