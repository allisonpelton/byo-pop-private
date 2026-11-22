// rundesk: runs a command on a private, invisible Windows desktop and waits for it.
//
//   rundesk.exe <program> [args...]
//
// Test runs of the game (games/bejeweled/tools/oracle/compare.sh, stress loops, -script and -autoplay runs)
// open a real window and raise it, which steals focus from whatever the user is doing. A window on another
// desktop is never shown and cannot take focus. Child processes inherit the desktop, so wrapping a shell
// script puts every game it starts there too. Rendering is unaffected: the game draws into a render-target
// texture and screenshots read that texture, not the window.
//
// Test runs are silent too: the children always get SDL's "dummy" audio driver, which plays nothing but
// still pulls audio at the device rate (the game does not depend on it). An SDL_AUDIODRIVER the caller
// set is replaced, so a real driver chosen for other programs cannot make hidden runs audible.
// And they run at below-normal priority (inherited by their children), so the machine stays responsive. The
// GPU's scheduling priority is not inherited, and Windows keeps a value set only once the process has made its
// first graphics call, so every half second rundesk sets it to below normal for each process in the job: a -fast
// run draws as often as it can, and without this a game on the invisible desktop competed on equal terms for the
// GPU with what the user was doing (the user, 2026-11-22: "Now why is peggle.exe eating most of my GPU??"). The
// game still takes the GPU time nothing else wants.
// BYOPOP_TEST_RUN=1 marks the run: the framework then never opens a URL (a game's "more games" link is a
// steam:// URL that would bring up the user's Steam client on their desktop).
//
// The rest of the command line is passed through verbatim, the standard handles are inherited, and the
// exit code is the child's. The desktop ("bejeweled-test") is created on first use and goes away when
// the last process using it ends.
//
// The child runs in a job object, which its own children join, and rundesk returns only when every process
// in the job has ended: a game that restarts itself (the mod menu's "Restart game", docs/MOD_MENU_DESIGN.md
// 6.4) stays on the invisible desktop and is waited for. Nothing is ended on purpose; but if rundesk itself is
// ended, the job ends with it (kill on close), so a stopped test leaves nothing running.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

// gdi32's D3DKMTSetProcessSchedulingPriorityClass (Windows 7 and later), found at run time; NULL if missing.
typedef LONG (APIENTRY* SetGpuPriorityFn)(HANDLE theProcess, INT thePriorityClass);
enum { GPU_PRIORITY_BELOW_NORMAL = 1 }; // D3DKMT_SCHEDULINGPRIORITYCLASS_BELOW_NORMAL
enum { WAIT_STEP_MS = 50, GPU_PRIORITY_EVERY_STEPS = 10 };
static SetGpuPriorityFn gSetGpuPriority = NULL;

// Sets the GPU priority of every process in the job to below normal (a process with no graphics yet ignores it,
// hence the repeats).
static void LowerJobGpuPriority(HANDLE theJob)
{
	enum { MAX_PROCESSES = 64 };
	struct
	{
		JOBOBJECT_BASIC_PROCESS_ID_LIST mList;
		ULONG_PTR mMore[MAX_PROCESSES];
	} anIds;
	if ((gSetGpuPriority == NULL) ||
		!QueryInformationJobObject(theJob, JobObjectBasicProcessIdList, &anIds, sizeof(anIds), NULL))
		return;
	for (DWORD i = 0; i < anIds.mList.NumberOfProcessIdsInList; i++)
	{
		HANDLE aProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
			(DWORD) anIds.mList.ProcessIdList[i]);
		if (aProcess != NULL)
		{
			gSetGpuPriority(aProcess, GPU_PRIORITY_BELOW_NORMAL);
			CloseHandle(aProcess);
		}
	}
}

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

	SetEnvironmentVariableW(L"SDL_AUDIODRIVER", L"dummy");
	SetEnvironmentVariableW(L"BYOPOP_TEST_RUN", L"1");

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

	HMODULE aGdi = LoadLibraryW(L"gdi32.dll");
	if (aGdi != NULL)
		gSetGpuPriority = (SetGpuPriorityFn) (void*) GetProcAddress(aGdi, "D3DKMTSetProcessSchedulingPriorityClass");

	HANDLE aJob = CreateJobObjectW(NULL, NULL);
	if (aJob != NULL)
	{
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION aLimits;
		ZeroMemory(&aLimits, sizeof(aLimits));
		aLimits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		SetInformationJobObject(aJob, JobObjectExtendedLimitInformation, &aLimits, sizeof(aLimits));
	}

	PROCESS_INFORMATION aProcess;
	ZeroMemory(&aProcess, sizeof(aProcess));
	if (!CreateProcessW(NULL, aCmd, NULL, NULL, TRUE, BELOW_NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED, NULL, NULL, &aStartup, &aProcess))
	{
		fprintf(stderr, "rundesk: cannot start the command (error %lu)\n", GetLastError());
		if (aJob != NULL)
			CloseHandle(aJob);
		CloseDesktop(aDesktop);
		return 127;
	}

	// In the job before it runs, so every process it starts is in it too. Without a job (a parent's job that
	// forbids one) only the child is waited for.
	BOOL isInJob = (aJob != NULL) && AssignProcessToJobObject(aJob, aProcess.hProcess);
	ResumeThread(aProcess.hThread);

	if (isInJob)
	{
		for (int aStep = 0; WaitForSingleObject(aProcess.hProcess, WAIT_STEP_MS) == WAIT_TIMEOUT; aStep++)
			if ((aStep % GPU_PRIORITY_EVERY_STEPS) == 0)
				LowerJobGpuPriority(aJob);
	}
	else
		WaitForSingleObject(aProcess.hProcess, INFINITE);
	DWORD anExitCode = 1;
	GetExitCodeProcess(aProcess.hProcess, &anExitCode);
	CloseHandle(aProcess.hThread);
	CloseHandle(aProcess.hProcess);

	// Then for the rest of the job: processes the child started and left running (a restarted game)
	for (int aStep = 0; isInJob; aStep++)
	{
		JOBOBJECT_BASIC_ACCOUNTING_INFORMATION anInfo;
		if (!QueryInformationJobObject(aJob, JobObjectBasicAccountingInformation, &anInfo, sizeof(anInfo), NULL) ||
			(anInfo.ActiveProcesses == 0))
			break;
		if ((aStep % GPU_PRIORITY_EVERY_STEPS) == 0)
			LowerJobGpuPriority(aJob);
		Sleep(WAIT_STEP_MS);
	}
	if (aJob != NULL)
		CloseHandle(aJob);
	CloseDesktop(aDesktop);
	return (int) anExitCode;
}
