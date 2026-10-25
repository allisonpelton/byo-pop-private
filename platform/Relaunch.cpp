#include "Relaunch.h"

#include <SDL.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace Platform
{

std::string QuoteCommandLineArg(const std::string& theArg)
{
	if (!theArg.empty() && (theArg.find_first_of(" \t\n\v\"") == std::string::npos))
		return theArg;

	// Backslashes are literal unless they precede a quote: then each one is doubled, and the quote escaped
	std::string aResult = "\"";
	size_t aBackslashes = 0;
	for (char c : theArg)
	{
		if (c == '\\')
		{
			aBackslashes++;
			continue;
		}
		if (c == '"')
			aResult.append(aBackslashes * 2 + 1, '\\');
		else
			aResult.append(aBackslashes, '\\');
		aBackslashes = 0;
		aResult += c;
	}
	aResult.append(aBackslashes * 2, '\\');
	aResult += '"';
	return aResult;
}

#ifdef _WIN32
static std::wstring Widen(const std::string& theText)
{
	if (theText.empty())
		return std::wstring();
	int aLength = MultiByteToWideChar(CP_UTF8, 0, theText.data(), (int) theText.size(), NULL, 0);
	std::wstring aResult((size_t) (aLength > 0 ? aLength : 0), L'\0');
	if (aLength > 0)
		MultiByteToWideChar(CP_UTF8, 0, theText.data(), (int) theText.size(), &aResult[0], aLength);
	return aResult;
}

static std::wstring ModulePath()
{
	std::wstring aPath(MAX_PATH, L'\0');
	for (;;)
	{
		DWORD aLength = GetModuleFileNameW(NULL, &aPath[0], (DWORD) aPath.size());
		if (aLength == 0)
			return std::wstring();
		if (aLength < aPath.size())
		{
			aPath.resize(aLength);
			return aPath;
		}
		if (aPath.size() >= 32768)
			return std::wstring();
		aPath.resize(aPath.size() * 2);
	}
}

#endif

void AllowRelaunchForeground()
{
#ifdef _WIN32
	AllowSetForegroundWindow(ASFW_ANY);
#endif
}

bool Relaunch(const std::vector<std::string>& theArgs, const std::string& theGameName)
{
	bool isStarted = false;
#ifdef _WIN32
	std::wstring anExe = ModulePath();
	if (!anExe.empty())
	{
		// (a Windows path holds no quote; the quoting looks only at ASCII, so it works on UTF-8 as on UTF-16)
		std::wstring aCommandLine = L"\"" + anExe + L"\"";
		for (const std::string& anArg : theArgs)
			aCommandLine += L" " + Widen(QuoteCommandLineArg(anArg));

		// The test switch must not start a third copy from the second
		SetEnvironmentVariableW(L"BYOPOP_RELAUNCH_TEST_ARGS", NULL);

		STARTUPINFOW aStartup;
		ZeroMemory(&aStartup, sizeof(aStartup));
		aStartup.cb = sizeof(aStartup);
		PROCESS_INFORMATION aProcess;
		ZeroMemory(&aProcess, sizeof(aProcess));
		std::vector<wchar_t> aBuffer(aCommandLine.begin(), aCommandLine.end());
		aBuffer.push_back(L'\0');
		if (CreateProcessW(anExe.c_str(), aBuffer.data(), NULL, NULL, FALSE, 0, NULL, NULL, &aStartup, &aProcess))
		{
			CloseHandle(aProcess.hThread);
			CloseHandle(aProcess.hProcess);
			isStarted = true;
		}
		else
			SDL_Log("restart: cannot start the game again (error %lu)", (unsigned long) GetLastError());
	}
	else
		SDL_Log("restart: cannot find the program's path");
#else
	(void) theArgs;
	SDL_Log("restart: starting the game again is not supported here");
#endif

	if (!isStarted)
	{
		std::string aText = "Could not restart " + theGameName + ". Your game was saved; start it again.";
		// A test run's desktop is invisible: a message box there would wait for ever
		if ((SDL_getenv("BYOPOP_TEST_RUN") == NULL) && (SDL_getenv("BEJ_TEST_DESKTOP") == NULL))
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, theGameName.c_str(), aText.c_str(), NULL);
		SDL_Log("%s", aText.c_str());
	}
	return isStarted;
}

}
