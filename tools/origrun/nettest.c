// nettest: a small 32-bit program for check.sh's origrun suite (tools/origrun/nettest.sh). It loads the stand-in
// (dsound.dll beside it) as a game does, opens a listening socket on the loopback, then calls every function the network
// block hooks (net.c) at that port or on a host that exists, some through its import and some through LoadLibrary and
// GetProcAddress (winhttp.dll is loaded after the stand-in). Each must fail at once with the failure code and last error
// the block returns, which differ from Windows' own for the same call (a real WinINet call on a made-up handle sets
// ERROR_INVALID_HANDLE, not ERROR_INTERNET_CANNOT_CONNECT), and nothing may reach the listener. It writes its report to
// nettest.out beside the exe: one line per call, and "NETTEST n of n as expected" last. No window, no sound: it is not a game.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wininet.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>

static FILE* gOut;
static int gN, gOk;

static void check(const char* what, int ok, const char* detail)
{
	gN++; gOk += ok != 0;
	fprintf(gOut, "%s %s%s%s\n", ok ? "ok  " : "FAIL", what, detail && detail[0] ? ": " : "", detail ? detail : "");
}

typedef int (WINAPI *PGetAddrInfoW)(const WCHAR*, const WCHAR*, const void*, void**);
typedef BOOL (WINAPI *PWSAConnectByNameA)(SOCKET, LPSTR, LPSTR, LPDWORD, LPSOCKADDR, LPDWORD, LPSOCKADDR, const struct timeval*, LPWSAOVERLAPPED);
typedef HANDLE (WINAPI *PWinHttpOpen)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
typedef HANDLE (WINAPI *PWinHttpConnect)(HANDLE, LPCWSTR, WORD, DWORD);
typedef BOOL (WINAPI *PWinHttpSendRequest)(HANDLE, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
typedef LONG (WINAPI *PURLDownloadToFileW)(void*, LPCWSTR, LPCWSTR, DWORD, void*);
typedef LONG (WINAPI *PURLOpenBlockingStreamA)(void*, LPCSTR, void**, DWORD, void*);
typedef LONG (WINAPI *PURLOpenStreamA)(void*, LPCSTR, DWORD, void*);
typedef BOOL (WINAPI *PHttpSendRequestW)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD);
typedef HINTERNET (WINAPI *PInternetOpenW)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);

int main(void)
{
	char path[MAX_PATH], url[200], host[40], tmp[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	char* l = strrchr(path, '\\'); if (l) l[1] = 0;
	char outp[MAX_PATH]; lstrcpyA(outp, path); lstrcatA(outp, "nettest.out");
	gOut = fopen(outp, "w");
	if (!gOut) return 2;
	lstrcpyA(tmp, path); lstrcatA(tmp, "nettest.download");

	HMODULE standIn = LoadLibraryA("dsound.dll");   // the stand-in, from the exe's folder
	{
		char p[MAX_PATH] = ""; if (standIn) GetModuleFileNameA(standIn, p, MAX_PATH);
		char* s = strrchr(p, '\\');
		check("the stand-in dsound.dll is the one beside the exe", standIn && s && !_strnicmp(p, path, (size_t)(s - p + 1)) && GetProcAddress(standIn, "DirectSoundCreate"), p);
	}

	WSADATA wd; WSAStartup(MAKEWORD(2, 2), &wd);
	SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKET us = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in a; memset(&a, 0, sizeof(a)); a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	bind(ls, (struct sockaddr*)&a, sizeof(a)); listen(ls, 4);
	bind(us, (struct sockaddr*)&a, sizeof(a));
	struct sockaddr_in ta, ua; int tl = sizeof(ta), ul = sizeof(ua);
	getsockname(ls, (struct sockaddr*)&ta, &tl); getsockname(us, (struct sockaddr*)&ua, &ul);
	unsigned port = ntohs(ta.sin_port);
	check("a listener on the loopback", port != 0, "");
	_snprintf(host, sizeof(host), "%u", port);
	_snprintf(url, sizeof(url), "http://127.0.0.1:%u/nettest", port);
	char detail[80];

	// WinSock, by import
	SOCKET c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int r = connect(c, (struct sockaddr*)&ta, sizeof(ta)); int e = WSAGetLastError();
	_snprintf(detail, sizeof(detail), "returned %d, error %d", r, e);
	check("connect to the listener fails with WSAENETUNREACH", r == SOCKET_ERROR && e == WSAENETUNREACH, detail);
	r = WSAConnect(c, (struct sockaddr*)&ta, sizeof(ta), NULL, NULL, NULL, NULL); e = WSAGetLastError();
	_snprintf(detail, sizeof(detail), "returned %d, error %d", r, e);
	check("WSAConnect to the listener fails with WSAENETUNREACH", r == SOCKET_ERROR && e == WSAENETUNREACH, detail);
	struct hostent* he = gethostbyname("localhost"); e = WSAGetLastError();
	_snprintf(detail, sizeof(detail), "returned %p, error %d", (void*)he, e);
	check("gethostbyname(localhost) fails with WSAHOST_NOT_FOUND", !he && e == WSAHOST_NOT_FOUND, detail);
	struct addrinfo* ai = (struct addrinfo*)1; r = getaddrinfo("localhost", NULL, NULL, &ai);
	_snprintf(detail, sizeof(detail), "returned %d", r);
	check("getaddrinfo(localhost) fails with WSAHOST_NOT_FOUND", r == WSAHOST_NOT_FOUND && !ai, detail);
	r = sendto(us, "x", 1, 0, (struct sockaddr*)&ua, ul); e = WSAGetLastError();
	_snprintf(detail, sizeof(detail), "returned %d, error %d", r, e);
	check("sendto to a loopback socket fails with WSAENETUNREACH", r == SOCKET_ERROR && e == WSAENETUNREACH, detail);
	{
		WSABUF b; b.buf = "x"; b.len = 1; DWORD sent = 0;
		r = WSASendTo(us, &b, 1, &sent, 0, (struct sockaddr*)&ua, ul, NULL, NULL); e = WSAGetLastError();
		_snprintf(detail, sizeof(detail), "returned %d, error %d", r, e);
		check("WSASendTo to a loopback socket fails with WSAENETUNREACH", r == SOCKET_ERROR && e == WSAENETUNREACH, detail);
	}
	{
		HANDLE ah = WSAAsyncGetHostByName(NULL, WM_USER, "localhost", detail, sizeof(detail)); e = WSAGetLastError();
		check("WSAAsyncGetHostByName fails", ah == NULL && e == WSAENETDOWN, "");
	}
	// WinSock, through GetProcAddress
	HMODULE ws = GetModuleHandleA("ws2_32.dll");
	PGetAddrInfoW pGai = (PGetAddrInfoW)(void*)GetProcAddress(ws, "GetAddrInfoW");
	if (pGai) {
		void* res = (void*)1; r = pGai(L"localhost", NULL, NULL, &res);
		_snprintf(detail, sizeof(detail), "returned %d", r);
		check("GetAddrInfoW(localhost) found with GetProcAddress fails", r == WSAHOST_NOT_FOUND && !res, detail);
	}
	PWSAConnectByNameA pCbn = (PWSAConnectByNameA)(void*)GetProcAddress(ws, "WSAConnectByNameA");
	if (pCbn) {
		BOOL ok = pCbn(c, "127.0.0.1", host, NULL, NULL, NULL, NULL, NULL, NULL); e = WSAGetLastError();
		_snprintf(detail, sizeof(detail), "returned %d, error %d", ok, e);
		check("WSAConnectByNameA to the listener fails with WSAENETUNREACH", !ok && e == WSAENETUNREACH, detail);
	}
	// WinINet, by import (a made-up handle: Windows' own answer would be ERROR_INVALID_HANDLE)
	HINTERNET fake = (HINTERNET)(size_t)0x1234;
	HINTERNET h = InternetOpenA("nettest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0); e = (int)GetLastError();
	_snprintf(detail, sizeof(detail), "returned %p, error %d", (void*)h, e);
	check("InternetOpenA fails with ERROR_INTERNET_CANNOT_CONNECT", !h && e == 12029, detail);
	h = InternetOpenUrlA(fake, url, NULL, 0, 0, 0); e = (int)GetLastError();
	_snprintf(detail, sizeof(detail), "returned %p, error %d", (void*)h, e);
	check("InternetOpenUrlA fails with ERROR_INTERNET_CANNOT_CONNECT", !h && e == 12029, detail);
	h = InternetConnectA(fake, "127.0.0.1", (INTERNET_PORT)port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0); e = (int)GetLastError();
	_snprintf(detail, sizeof(detail), "returned %p, error %d", (void*)h, e);
	check("InternetConnectA fails with ERROR_INTERNET_CANNOT_CONNECT", !h && e == 12029, detail);
	BOOL b = HttpSendRequestA(fake, NULL, 0, NULL, 0); e = (int)GetLastError();
	_snprintf(detail, sizeof(detail), "returned %d, error %d", b, e);
	check("HttpSendRequestA fails with ERROR_INTERNET_CANNOT_CONNECT", !b && e == 12029, detail);
	// WinINet, through GetProcAddress
	HMODULE wi = GetModuleHandleA("wininet.dll");
	PInternetOpenW pIo = (PInternetOpenW)(void*)GetProcAddress(wi, "InternetOpenW");
	if (pIo) { h = pIo(L"nettest", 0, NULL, NULL, 0); e = (int)GetLastError(); check("InternetOpenW found with GetProcAddress fails", !h && e == 12029, ""); }
	PHttpSendRequestW pHsr = (PHttpSendRequestW)(void*)GetProcAddress(wi, "HttpSendRequestW");
	if (pHsr) { b = pHsr(fake, NULL, 0, NULL, 0); e = (int)GetLastError(); check("HttpSendRequestW found with GetProcAddress fails", !b && e == 12029, ""); }
	// URLMon
	LONG hr = URLDownloadToFileA(NULL, url, tmp, 0, NULL);
	_snprintf(detail, sizeof(detail), "returned 0x%08lX", (unsigned long)hr);
	check("URLDownloadToFileA fails with INET_E_DOWNLOAD_FAILURE", hr == (LONG)0x800C0008L && GetFileAttributesA(tmp) == INVALID_FILE_ATTRIBUTES, detail);
	HMODULE um = LoadLibraryA("urlmon.dll");   // loaded after the stand-in
	PURLDownloadToFileW pUdw = um ? (PURLDownloadToFileW)(void*)GetProcAddress(um, "URLDownloadToFileW") : NULL;
	if (pUdw) { WCHAR wu[220], wt[MAX_PATH]; MultiByteToWideChar(CP_ACP, 0, url, -1, wu, 220); MultiByteToWideChar(CP_ACP, 0, tmp, -1, wt, MAX_PATH);
		hr = pUdw(NULL, wu, wt, 0, NULL); _snprintf(detail, sizeof(detail), "returned 0x%08lX", (unsigned long)hr);
		check("URLDownloadToFileW (urlmon loaded later) fails with INET_E_DOWNLOAD_FAILURE", hr == (LONG)0x800C0008L, detail); }
	PURLOpenBlockingStreamA pUbs = um ? (PURLOpenBlockingStreamA)(void*)GetProcAddress(um, "URLOpenBlockingStreamA") : NULL;
	if (pUbs) { void* st = (void*)1; hr = pUbs(NULL, url, &st, 0, NULL); _snprintf(detail, sizeof(detail), "returned 0x%08lX", (unsigned long)hr);
		check("URLOpenBlockingStreamA fails with INET_E_CANNOT_CONNECT and no stream", hr == (LONG)0x800C0004L && !st, detail); }
	PURLOpenStreamA pUos = um ? (PURLOpenStreamA)(void*)GetProcAddress(um, "URLOpenStreamA") : NULL;
	if (pUos) { hr = pUos(NULL, url, 0, NULL); _snprintf(detail, sizeof(detail), "returned 0x%08lX", (unsigned long)hr);
		check("URLOpenStreamA fails with INET_E_CANNOT_CONNECT", hr == (LONG)0x800C0004L, detail); }
	// WinHTTP, loaded after the stand-in
	HMODULE wh = LoadLibraryA("winhttp.dll");
	PWinHttpOpen pWo = wh ? (PWinHttpOpen)(void*)GetProcAddress(wh, "WinHttpOpen") : NULL;
	if (pWo) { HANDLE s = pWo(L"nettest", 0, NULL, NULL, 0); e = (int)GetLastError();
		_snprintf(detail, sizeof(detail), "returned %p, error %d", (void*)s, e); check("WinHttpOpen (winhttp loaded later) fails", !s && e == 12029, detail); }
	PWinHttpConnect pWc = wh ? (PWinHttpConnect)(void*)GetProcAddress(wh, "WinHttpConnect") : NULL;
	if (pWc) { HANDLE s = pWc((HANDLE)fake, L"127.0.0.1", (WORD)port, 0); e = (int)GetLastError();
		_snprintf(detail, sizeof(detail), "returned %p, error %d", (void*)s, e); check("WinHttpConnect fails", !s && e == 12029, detail); }
	PWinHttpSendRequest pWs = wh ? (PWinHttpSendRequest)(void*)GetProcAddress(wh, "WinHttpSendRequest") : NULL;
	if (pWs) { b = pWs((HANDLE)fake, NULL, 0, NULL, 0, 0, 0); e = (int)GetLastError();
		_snprintf(detail, sizeof(detail), "returned %d, error %d", b, e); check("WinHttpSendRequest fails", !b && e == 12029, detail); }

	// The launch guard. Every target below would write nettest-launched.txt beside the exe if it ran (cmd.exe /c echo), so a launch
	// that was not blocked shows as that file; Windows' own answer to each call is success, the guard's is ERROR_ACCESS_DENIED (5).
	{
		char canary[MAX_PATH], cmdline[2 * MAX_PATH], params[2 * MAX_PATH];
		lstrcpyA(canary, path); lstrcatA(canary, "nettest-launched.txt"); DeleteFileA(canary);
		_snprintf(params, sizeof(params), "/c echo launched> \"%s\"", canary); params[sizeof(params) - 1] = 0;
		_snprintf(cmdline, sizeof(cmdline), "cmd.exe %s", params); cmdline[sizeof(cmdline) - 1] = 0;
		WCHAR wparams[2 * MAX_PATH], wcmd[2 * MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, params, -1, wparams, 2 * MAX_PATH); MultiByteToWideChar(CP_ACP, 0, cmdline, -1, wcmd, 2 * MAX_PATH);

		SetLastError(0);
		HINSTANCE si = ShellExecuteA(NULL, "open", "cmd.exe", params, NULL, SW_HIDE); e = (int)GetLastError();
		_snprintf(detail, sizeof(detail), "returned %p, error %d", (void*)si, e);
		check("ShellExecuteA fails with SE_ERR_ACCESSDENIED and ERROR_ACCESS_DENIED", (INT_PTR)si == 5 && e == ERROR_ACCESS_DENIED, detail);
		SetLastError(0);
		si = ShellExecuteW(NULL, L"open", L"cmd.exe", wparams, NULL, SW_HIDE); e = (int)GetLastError();
		_snprintf(detail, sizeof(detail), "returned %p, error %d", (void*)si, e);
		check("ShellExecuteW fails with SE_ERR_ACCESSDENIED and ERROR_ACCESS_DENIED", (INT_PTR)si == 5 && e == ERROR_ACCESS_DENIED, detail);
		SHELLEXECUTEINFOA sei; memset(&sei, 0, sizeof(sei)); sei.cbSize = sizeof(sei); sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		sei.lpVerb = "open"; sei.lpFile = "cmd.exe"; sei.lpParameters = params; sei.nShow = SW_HIDE;
		SetLastError(0); b = ShellExecuteExA(&sei); e = (int)GetLastError();
		_snprintf(detail, sizeof(detail), "returned %d, error %d, hInstApp %p, hProcess %p", b, e, (void*)sei.hInstApp, (void*)sei.hProcess);
		check("ShellExecuteExA fails with ERROR_ACCESS_DENIED and starts no process", !b && e == ERROR_ACCESS_DENIED && (INT_PTR)sei.hInstApp == 5 && !sei.hProcess, detail);
		HMODULE sh = GetModuleHandleA("shell32.dll");
		typedef BOOL (WINAPI *PShellExecuteExW)(SHELLEXECUTEINFOW*);
		PShellExecuteExW pSeW = sh ? (PShellExecuteExW)(void*)GetProcAddress(sh, "ShellExecuteExW") : NULL;
		if (pSeW) {
			SHELLEXECUTEINFOW sew; memset(&sew, 0, sizeof(sew)); sew.cbSize = sizeof(sew); sew.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
			sew.lpVerb = L"open"; sew.lpFile = L"cmd.exe"; sew.lpParameters = wparams; sew.nShow = SW_HIDE;
			SetLastError(0); b = pSeW(&sew); e = (int)GetLastError();
			_snprintf(detail, sizeof(detail), "returned %d, error %d, hProcess %p", b, e, (void*)sew.hProcess);
			check("ShellExecuteExW found with GetProcAddress fails with ERROR_ACCESS_DENIED", !b && e == ERROR_ACCESS_DENIED && !sew.hProcess, detail);
		}
		SetLastError(0);
		UINT we = WinExec(cmdline, SW_HIDE); e = (int)GetLastError();
		_snprintf(detail, sizeof(detail), "returned %u, error %d", we, e);
		check("WinExec returns 5 (below 32: failed) with ERROR_ACCESS_DENIED", we == 5 && e == ERROR_ACCESS_DENIED, detail);
		STARTUPINFOA stA; memset(&stA, 0, sizeof(stA)); stA.cb = sizeof(stA);
		PROCESS_INFORMATION pi; memset(&pi, 0xAB, sizeof(pi));
		char cmdbuf[2 * MAX_PATH]; lstrcpyA(cmdbuf, cmdline);
		SetLastError(0); b = CreateProcessA(NULL, cmdbuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &stA, &pi); e = (int)GetLastError();
		_snprintf(detail, sizeof(detail), "returned %d, error %d, hProcess %p", b, e, (void*)pi.hProcess);
		check("CreateProcessA fails with ERROR_ACCESS_DENIED and returns no handles", !b && e == ERROR_ACCESS_DENIED && !pi.hProcess && !pi.hThread, detail);
		HMODULE k32 = GetModuleHandleA("kernel32.dll");
		typedef BOOL (WINAPI *PCreateProcessW)(LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
		PCreateProcessW pCpW = (PCreateProcessW)(void*)GetProcAddress(k32, "CreateProcessW");
		if (pCpW) {
			STARTUPINFOW stW; memset(&stW, 0, sizeof(stW)); stW.cb = sizeof(stW);
			WCHAR wbuf[2 * MAX_PATH]; lstrcpyW(wbuf, wcmd);
			memset(&pi, 0xAB, sizeof(pi));
			SetLastError(0); b = pCpW(NULL, wbuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &stW, &pi); e = (int)GetLastError();
			_snprintf(detail, sizeof(detail), "returned %d, error %d, hProcess %p", b, e, (void*)pi.hProcess);
			check("CreateProcessW found with GetProcAddress fails with ERROR_ACCESS_DENIED", !b && e == ERROR_ACCESS_DENIED && !pi.hProcess, detail);
		}
		Sleep(1500);   // a launch that got through would have written its file by now
		check("nothing was started: no nettest-launched.txt", GetFileAttributesA(canary) == INVALID_FILE_ATTRIBUTES, "");
		DeleteFileA(canary);
	}

	// nothing reached the listener or the datagram socket
	fd_set rs; struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 300000;
	FD_ZERO(&rs); FD_SET(ls, &rs); FD_SET(us, &rs);
	r = select(0, &rs, NULL, NULL, &tv);
	_snprintf(detail, sizeof(detail), "select said %d", r);
	check("nothing reached the listener or the datagram socket", r == 0, detail);

	fprintf(gOut, "NETTEST %d of %d as expected\n", gOk, gN);
	fclose(gOut);
	return gOk == gN ? 0 : 1;
}
