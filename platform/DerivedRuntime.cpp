// Parts of the C++ runtime that a release exe does without (docs/PUBLISHING.md, "What a release holds"). A release's
// repository compiles them in (the exe's own objects, so that the linker takes them before libc++abi's); the trunk builds
// an empty file.
#ifdef BYOPOP_DERIVED

#include <cstddef>

// The name demangler of libc++abi, about 140 KB with its tables and unwind data, which nothing calls but the terminate
// handler, to print the type of an uncaught exception. This build throws none (it has no exceptions), so what can still
// reach the handler is a terminate with no exception (an error that would have thrown, ResourceManager.cpp), and it
// asks for no name.
extern "C" char* __cxa_demangle(const char* theMangledName, char* theBuffer, size_t* theLength, int* theStatus)
{
	(void) theMangledName;
	(void) theBuffer;
	(void) theLength;
	if (theStatus != nullptr)
		*theStatus = -2;  // libc++abi's "not a valid mangled name"
	return nullptr;
}

#endif
