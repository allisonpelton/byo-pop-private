// Parts of the C++ runtime that a release exe does without (docs/PUBLISHING.md, "What a release holds"). A release's
// repository compiles them in (the exe's own objects, so that the linker takes them before libc++abi's); the trunk builds
// an empty file.
#ifdef BYOPOP_DERIVED

#include <cstddef>
#include <cstdlib>
#include <new>

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

// operator new of libc++abi throws std::bad_alloc when memory runs out, which brings in the exception machinery (the
// unwinder, its tables and the personality routine, some 70 KB) for a throw that nothing in this build catches: it ended the
// program (std::terminate, then abort), and this ends it the same way, with no exception.
static void* AllocateOrAbort(size_t theSize)
{
	void* aBlock = malloc(theSize != 0 ? theSize : 1);
	if (aBlock == nullptr)
		abort();
	return aBlock;
}

void* operator new(size_t theSize) { return AllocateOrAbort(theSize); }
void* operator new[](size_t theSize) { return AllocateOrAbort(theSize); }
void* operator new(size_t theSize, const std::nothrow_t&) noexcept { return malloc(theSize != 0 ? theSize : 1); }
void* operator new[](size_t theSize, const std::nothrow_t&) noexcept { return malloc(theSize != 0 ? theSize : 1); }
void operator delete(void* theBlock) noexcept { free(theBlock); }
void operator delete[](void* theBlock) noexcept { free(theBlock); }
void operator delete(void* theBlock, size_t) noexcept { free(theBlock); }
void operator delete[](void* theBlock, size_t) noexcept { free(theBlock); }
void operator delete(void* theBlock, const std::nothrow_t&) noexcept { free(theBlock); }
void operator delete[](void* theBlock, const std::nothrow_t&) noexcept { free(theBlock); }

#endif
