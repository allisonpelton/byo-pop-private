// covtest: a small 32-bit program for check.sh's origrun suite (tools/origrun/covtest.sh). It imports no DirectSound,
// as Bejeweled's exe does not, so origrun --coverage must add the stand-in to its imports; the recorder must then
// write the two functions it calls, each once though the second is called twice, and not the one it never calls.
// It is built for ASLR, so Windows moves it from its preferred base, which the recorder must allow for (the originals
// are not moved). No window and no sound: it is not a game.
//
//   covtest --addrs <file>   writes the three functions' addresses at the exe's preferred base, the address list
//   covtest                  calls them and exits
#include <windows.h>
#include <stdio.h>
#include <string.h>

__declspec(noinline) int CovCalledOnce(int x) { return x * 3 + 1; }
__declspec(noinline) int CovCalledTwice(int x) { return CovCalledOnce(x) - 2; }
__declspec(noinline) int CovNeverCalled(int x) { return x ^ 0x55; }

// A function's address as the exe's own file has it: at the preferred base from the file, since a moved image's header
// in memory has the base it got instead.
static unsigned long FileAddress(void* fn)
{
	char path[MAX_PATH];
	BYTE head[4096];
	DWORD got = 0;
	GetModuleFileNameA(NULL, path, MAX_PATH);
	HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	ReadFile(f, head, sizeof(head), &got, NULL);
	CloseHandle(f);
	DWORD preferred = ((IMAGE_NT_HEADERS32*)(head + ((IMAGE_DOS_HEADER*)head)->e_lfanew))->OptionalHeader.ImageBase;
	BYTE* base = (BYTE*)GetModuleHandleA(NULL);
	return (unsigned long)(preferred + ((BYTE*)fn - base));
}

int main(int argc, char** argv)
{
	if (argc > 2 && strcmp(argv[1], "--addrs") == 0) {
		FILE* f = fopen(argv[2], "w");
		if (!f) return 2;
		fprintf(f, "# covtest's functions\n%08lx\n%08lx\n%08lx\n", FileAddress((void*)CovCalledOnce),
			FileAddress((void*)CovCalledTwice), FileAddress((void*)CovNeverCalled));
		fclose(f);
		return 0;
	}
	volatile int v = CovCalledTwice(argc);
	v += CovCalledTwice(v);
	return v == 12345 ? CovNeverCalled(v) : 0;
}
