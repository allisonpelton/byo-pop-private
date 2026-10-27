// port: test hooks that need windows.h, a file of its own because it clashes with the framework's typedefs. A crash
// writes where it was to the log (so does an exception nobody caught), and UsedVirtualMemory tells how much of the
// 32-bit address space the process has used. The crash log and UsedVirtualMemory are for Windows x86 only (elsewhere
// UsedVirtualMemory is 0).
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <typeinfo>
#include <SDL.h>
#if defined(_WIN32) && defined(__i386__)
#include <windows.h>
// A crash writes where it was (the address and the return addresses on the stack that are in the exe's image, to
// symbolize with llvm-symbolizer: the offsets are virtual addresses) to the log, then goes on to the system's handling
// (a vectored handler: the C runtime's own unhandled exception filter would not hear a stack overflow)
// (nothing here allocates: the crash may come from a damaged heap, and SDL_Log would allocate)
static void CrashLine(const char* theFormat, DWORD a = 0, DWORD b = 0, DWORD c = 0, DWORD d = 0)
{
	char aBuffer[256];
	int aLength = wsprintfA(aBuffer, theFormat, a, b, c, d);
	DWORD aWritten;
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), aBuffer, (DWORD)aLength, &aWritten, nullptr);
}

static LONG WINAPI LogCrash(EXCEPTION_POINTERS* theInfo)
{
	// (a vectored handler hears every exception, the C++ ones too: only the system's errors, the access violation and the
	// stack overflow among them)
	if ((theInfo->ExceptionRecord->ExceptionCode & 0xFFFF0000) != 0xC0000000)
		return EXCEPTION_CONTINUE_SEARCH;
	const CONTEXT* aContext = theInfo->ContextRecord;
	const unsigned char* aBase = (const unsigned char*)GetModuleHandle(nullptr);
	const IMAGE_NT_HEADERS* aHeaders = (const IMAGE_NT_HEADERS*)(aBase + ((const IMAGE_DOS_HEADER*)aBase)->e_lfanew);
	const unsigned char* anEnd = aBase + aHeaders->OptionalHeader.SizeOfImage;
	CrashLine("crash: exception %08lx at %08lx (image %08lx to %08lx)\n", theInfo->ExceptionRecord->ExceptionCode,
	          (DWORD)(size_t)theInfo->ExceptionRecord->ExceptionAddress, (DWORD)(size_t)aBase, (DWORD)(size_t)anEnd);
	if (theInfo->ExceptionRecord->NumberParameters > 1)
		CrashLine("crash: access %08lx at address %08lx\n", (DWORD)theInfo->ExceptionRecord->ExceptionInformation[0],
		          (DWORD)theInfo->ExceptionRecord->ExceptionInformation[1]);
	CrashLine("crash: eip %08lx esp %08lx ebp %08lx eax %08lx\n", aContext->Eip, aContext->Esp, aContext->Ebp, aContext->Eax);
	const DWORD* aStack = (const DWORD*)aContext->Esp;
	const DWORD* aStackBase = (const DWORD*)((NT_TIB*)NtCurrentTeb())->StackBase;
	int aLogged = 0;
	for (const DWORD* p = aStack; p < aStackBase && p < aStack + 8192 && aLogged < 40; p++)
		if (*p >= (DWORD)(size_t)aBase && *p < (DWORD)(size_t)anEnd) {
			CrashLine("crash: stack %d: %08lx\n", (DWORD)(p - aStack), *p);
			aLogged++;
		}
	return EXCEPTION_CONTINUE_SEARCH;
}

static struct CrashLogInstaller {
	CrashLogInstaller() { AddVectoredExceptionHandler(1, LogCrash); }
} sCrashLogInstaller;

size_t UsedVirtualMemory()
{
	MEMORYSTATUSEX aStatus;
	aStatus.dwLength = sizeof(aStatus);
	if (!GlobalMemoryStatusEx(&aStatus))
		return 0;
	return (size_t)(aStatus.ullTotalVirtual - aStatus.ullAvailVirtual);
}
#else
size_t UsedVirtualMemory() { return 0; }
#endif

// An exception nobody caught ends the program through std::terminate: the log says what it was
static void LogTerminate()
{
	try {
		std::exception_ptr aThrown = std::current_exception();
		if (aThrown)
			std::rethrow_exception(aThrown);
		SDL_Log("terminate: called without an exception");
	} catch (const std::exception& e) {
		SDL_Log("terminate: %s (%s)", e.what(), typeid(e).name());
	} catch (...) {
		SDL_Log("terminate: an exception that is not a std::exception");
	}
	std::abort();
}

static struct TerminateLogInstaller {
	TerminateLogInstaller() { std::set_terminate(LogTerminate); }
} sTerminateLogInstaller;
