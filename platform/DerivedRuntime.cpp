// Parts of the C++ runtime that a release exe does without (docs/PUBLISHING.md, "What a release holds"). Only a derived
// repository compiles them in: tools/publish.py keeps this file's code through its BYOPOP_DERIVED conditional, and the trunk
// builds an empty file.
#ifdef BYOPOP_DERIVED

#include <cstddef>

// The name demangler of libc++abi, 140 KB with its tables, which nothing calls but the terminate handler, to print the
// type of an uncaught exception. The one thing this build can still end in is a terminate with no exception (an error
// that would have thrown, ResourceManager.cpp), so the handler asks for no name.
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
