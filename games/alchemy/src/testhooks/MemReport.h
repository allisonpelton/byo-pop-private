#ifndef __MEMREPORT_H__
#define __MEMREPORT_H__

#include <string>

namespace Sexy { class SexyAppBase; }

// port: script command "memreport <file>": what the game holds in memory, by owner (the images' pixels and the
// blitters' caches, the sounds' samples, the heap), for profiling. Returns false when <file> cannot be written.
bool WriteMemReport(Sexy::SexyAppBase* theApp, const std::string& theFileName);

#endif
