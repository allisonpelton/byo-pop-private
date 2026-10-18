// rundesk: runs a command on a private, invisible Windows desktop and waits for it.
//
//   rundesk.exe <program> [args...]
//
// Test runs of the game (tools/oracle/compare.sh, stress loops, -script and -autoplay runs) open a real
// window and raise it, which steals focus from whatever the user is doing. A window on another desktop
// is never shown and cannot take focus. Child processes inherit the desktop, so wrapping a shell script
// puts every game it starts there too. Rendering is unaffected: the game draws into a render-target
// texture and screenshots read that texture, not the window.
//
// Test runs are silent too: unless SDL_AUDIODRIVER is already set, the children get SDL's "dummy" audio
// driver, which plays nothing but still pulls audio at the device rate (the game does not depend on it).
// And they run at below-normal priority (inherited by their children), so the machine stays responsive.
//
// The rest of the command line is passed through verbatim, the standard handles are inherited, and the
// exit code is the child's. The desktop ("bejeweled-test") is created on first use and goes away when
// the last process using it ends.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

// The command line after the program name of this process, as typed (keeps the caller's quoting).
static wchar_t* SkipArg0(wchar_t* theCmdLine)
{
	wchar_t* p = theCmdLine;
	if (*p == L'"')
	{
		p++;
		while ((*p != 0) && (*p != L'"'))
			p++;
		if (*p == L'"')
			p++;
	}
	else
	{
		while ((*p != 0) && (*p != L' ') && (*p != L'\t'))
			p++;
	}
	while ((*p == L' ') || (*p == L'\t'))
		p++;
	return p;
}

int main(void)
{
	wchar_t* aCmd = SkipArg0(GetCommandLineW());
	if (*aCmd == 0)
	{
		fprintf(stderr, "usage: rundesk <program> [args...]\n");
		return 2;
	}

	if (GetEnvironmentVariableW(L"SDL_AUDIODRIVER", NULL, 0) == 0)
		SetEnvironmentVariableW(L"SDL_AUDIODRIVER", L"dummy");

	const wchar_t* aDesktopName = L"bejeweled-test";
	HDESK aDesktop = CreateDesktopW(aDesktopName, NULL, NULL, 0, GENERIC_ALL, NULL);
	if (aDesktop == NULL)
	{
		fprintf(stderr, "rundesk: CreateDesktop failed (error %lu)\n", GetLastError());
		return 3;
	}

	wchar_t aDesktopPath[64];
	swprintf(aDesktopPath, 64, L"WinSta0\\%ls", aDesktopName);

	STARTUPINFOW aStartup;
	ZeroMemory(&aStartup, sizeof(aStartup));
	aStartup.cb = sizeof(aStartup);
	aStartup.lpDesktop = aDesktopPath;
	aStartup.dwFlags = STARTF_USESTDHANDLES;
	aStartup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	aStartup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	aStartup.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	PROCESS_INFORMATION aProcess;
	ZeroMemory(&aProcess, sizeof(aProcess));
	if (!CreateProcessW(NULL, aCmd, NULL, NULL, TRUE, BELOW_NORMAL_PRIORITY_CLASS, NULL, NULL, &aStartup, &aProcess))
	{
		fprintf(stderr, "rundesk: cannot start the command (error %lu)\n", GetLastError());
		CloseDesktop(aDesktop);
		return 127;
	}

	WaitForSingleObject(aProcess.hProcess, INFINITE);
	DWORD anExitCode = 1;
	GetExitCodeProcess(aProcess.hProcess, &anExitCode);
	CloseHandle(aProcess.hThread);
	CloseHandle(aProcess.hProcess);
	CloseDesktop(aDesktop);
	return (int) anExitCode;
}
