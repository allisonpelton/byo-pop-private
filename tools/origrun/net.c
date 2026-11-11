// origrun's network block, inside the game (docs/ORIGRUN.md, "The network block"). Built into the dsound stand-in beside
// the virtual clock; installed from DllMain, always, with no switch to turn it off. The user allowed it on 2026-11-11
// ("Yes, block them"): a copy's network calls fail at once, so that no run reaches a server (two of Seven Seas' coverage
// runs clicked its original's "Submit to Internet High Score List" by random input, on a host that has a network).
//
// What a PopCap game of 2001 to 2010 uses, and what is blocked of it:
//   WinSock (ws2_32.dll and wsock32.dll, where the SexyAppFramework games import by ordinal): connect, WSAConnect,
//     WSAConnectByNameA/W, gethostbyname, getaddrinfo, GetAddrInfoW, WSAAsyncGetHostByName, sendto, WSASendTo
//   WinINet: InternetOpenA/W, InternetOpenUrlA/W, InternetConnectA/W, HttpSendRequestA/W, HttpSendRequestExA/W
//   URLMon: URLDownloadToFileA/W, URLDownloadToCacheFileA/W, URLOpenStreamA/W, URLOpenBlockingStreamA/W, URLOpenPullStreamA/W
//   WinHTTP: WinHttpOpen, WinHttpConnect, WinHttpSendRequest
// Each returns the function's own failure code with the matching last error (a WinSock call SOCKET_ERROR or NULL with
// WSAENETUNREACH or WSAHOST_NOT_FOUND, a WinINet or WinHTTP call NULL or FALSE with ERROR_INTERNET_CANNOT_CONNECT, a URLMon
// call INET_E_DOWNLOAD_FAILURE or INET_E_CANNOT_CONNECT), never calls Windows', and logs one line to <run>\logs\net.log with
// the function and the host, address or URL ("NET BLOCKED"); origrun reads those lines for the run's RESULT.
//
// The same machinery carries the launch guard (the user allowed it on 2026-11-11, "allow blocking programs, files and links"): the
// program, file and link launches ShellExecuteA/W, ShellExecuteExA/W, WinExec and CreateProcessA/W (shell32, kernel32, kernelbase)
// fail at once with ERROR_ACCESS_DENIED and a "LAUNCH BLOCKED <function> <target>" line in the same log; see the "launches" section.
//
// How the calls are caught. Not by the import table (the games import wsock32 by ordinal, and a library loaded later or a
// GetProcAddress lookup would be missed): the first five bytes of each function in the DLL itself become a jump to the hook,
// the way the last-input hook does it (vclock.c, lastinput_detour), so every caller in the process reaches it. A forwarded
// export (wsock32's connect forwards to ws2_32's) is left alone: the function it forwards to is patched on its own. The DLLs
// that are mapped when the stand-in loads are patched at once, without loading anything (the loader lock is held); the others
// when they load, which the loader tells the stand-in of (LdrRegisterDllNotification) and which the module scan the other
// hooks make (hooks_rescan) looks for again.
// Written from the public Win32 documentation.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define NET_CANNOT_CONNECT   12029L          // ERROR_INTERNET_CANNOT_CONNECT = ERROR_WINHTTP_CANNOT_CONNECT
#define NET_HR_DOWNLOAD_FAIL ((LONG)0x800C0008L)   // INET_E_DOWNLOAD_FAILURE
#define NET_HR_CANNOT_CONNECT ((LONG)0x800C0004L)  // INET_E_CANNOT_CONNECT

static HANDLE gLog = INVALID_HANDLE_VALUE;
static LONG   gBlocked;                      // calls blocked in this process
static LONG   gInstalled;                    // net_install ran

// ---------------------------------------------------------------------------------------------------------- the log

static void net_open_log(void)
{
	char dir[MAX_PATH], path[MAX_PATH];
	DWORD n = GetEnvironmentVariableA("ORIGRUN_DIR", dir, sizeof(dir));
	if (!n || n >= sizeof(dir)) return;
	wsprintfA(path, "%s\\logs", dir); CreateDirectoryA(path, NULL);
	wsprintfA(path, "%s\\logs\\net.log", dir);
	gLog = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

// One line, written in one WriteFile (an append is atomic, so threads and processes do not interleave).
static void net_line(const char* fmt, ...)
{
	char buf[1400];
	va_list ap; va_start(ap, fmt);
	int n = _vsnprintf(buf, sizeof(buf) - 3, fmt, ap);
	va_end(ap);
	if (n < 0 || n > (int)sizeof(buf) - 3) n = (int)sizeof(buf) - 3;
	for (int i = 0; i < n; i++) if ((unsigned char)buf[i] < 0x20) buf[i] = '?';
	buf[n++] = '\r'; buf[n++] = '\n';
	if (gLog != INVALID_HANDLE_VALUE) { DWORD w; WriteFile(gLog, buf, (DWORD)n, &w, NULL); }
	else { buf[n] = 0; OutputDebugStringA(buf); }
}

static void blocked(const char* fn, const char* what)
{
	InterlockedIncrement(&gBlocked);
	net_line("pid %lu: NET BLOCKED %s %s", GetCurrentProcessId(), fn, what ? what : "");
}

static const char* wide(const WCHAR* w, char* buf, int cap)
{
	if (!w) return "(null)";
	if (!WideCharToMultiByte(CP_ACP, 0, w, -1, buf, cap, NULL, NULL)) lstrcpynA(buf, "(unreadable)", cap);
	return buf;
}

static void fmt_sockaddr(const struct sockaddr* a, int len, char* out, int cap)
{
	if (!a || len < (int)sizeof(a->sa_family)) { lstrcpynA(out, "(no address)", cap); return; }
	if (a->sa_family == AF_INET && len >= (int)sizeof(struct sockaddr_in)) {
		const struct sockaddr_in* s = (const struct sockaddr_in*)a;
		const unsigned char* b = (const unsigned char*)&s->sin_addr;
		_snprintf(out, cap, "%u.%u.%u.%u:%u", b[0], b[1], b[2], b[3], (unsigned)(((s->sin_port & 0xff) << 8) | (s->sin_port >> 8)));
	} else if (a->sa_family == AF_INET6 && len >= (int)sizeof(struct sockaddr_in6)) {
		const struct sockaddr_in6* s = (const struct sockaddr_in6*)a;
		const unsigned char* b = (const unsigned char*)&s->sin6_addr;
		_snprintf(out, cap, "[%x:%x:%x:%x:%x:%x:%x:%x]:%u", (b[0] << 8) | b[1], (b[2] << 8) | b[3], (b[4] << 8) | b[5], (b[6] << 8) | b[7],
			(b[8] << 8) | b[9], (b[10] << 8) | b[11], (b[12] << 8) | b[13], (b[14] << 8) | b[15],
			(unsigned)(((s->sin6_port & 0xff) << 8) | (s->sin6_port >> 8)));
	} else _snprintf(out, cap, "address family %d", (int)a->sa_family);
	out[cap - 1] = 0;
}

// ---------------------------------------------------------------------------------------------------------- WinSock

static int WINAPI net_connect(UINT_PTR s, const struct sockaddr* name, int len)
{
	char a[120]; fmt_sockaddr(name, len, a, sizeof(a));
	blocked("connect", a);
	SetLastError(WSAENETUNREACH);
	return SOCKET_ERROR;
}
static int WINAPI net_WSAConnect(UINT_PTR s, const struct sockaddr* name, int len, void* callerData, void* calleeData, void* sqos, void* gqos)
{
	char a[120]; fmt_sockaddr(name, len, a, sizeof(a));
	blocked("WSAConnect", a);
	SetLastError(WSAENETUNREACH);
	return SOCKET_ERROR;
}
static BOOL WINAPI net_WSAConnectByNameA(UINT_PTR s, LPSTR node, LPSTR service, LPDWORD localLen, void* local, LPDWORD remoteLen, void* remote, const void* timeout, void* reserved)
{
	char a[300]; _snprintf(a, sizeof(a), "%s:%s", node ? node : "(null)", service ? service : "(null)"); a[sizeof(a) - 1] = 0;
	blocked("WSAConnectByNameA", a);
	SetLastError(WSAENETUNREACH);
	return FALSE;
}
static BOOL WINAPI net_WSAConnectByNameW(UINT_PTR s, LPWSTR node, LPWSTR service, LPDWORD localLen, void* local, LPDWORD remoteLen, void* remote, const void* timeout, void* reserved)
{
	char n[200], v[60], a[300]; _snprintf(a, sizeof(a), "%s:%s", wide(node, n, sizeof(n)), wide(service, v, sizeof(v))); a[sizeof(a) - 1] = 0;
	blocked("WSAConnectByNameW", a);
	SetLastError(WSAENETUNREACH);
	return FALSE;
}
static void* WINAPI net_gethostbyname(const char* name)
{
	blocked("gethostbyname", name ? name : "(null)");
	SetLastError(WSAHOST_NOT_FOUND);
	return NULL;
}
static HANDLE WINAPI net_WSAAsyncGetHostByName(HWND hwnd, UINT msg, const char* name, char* buf, int buflen)
{
	blocked("WSAAsyncGetHostByName", name ? name : "(null)");
	SetLastError(WSAENETDOWN);
	return NULL;
}
static int WINAPI net_getaddrinfo(const char* node, const char* service, const void* hints, void** res)
{
	blocked("getaddrinfo", node ? node : "(null)");
	if (res) *res = NULL;
	SetLastError(WSAHOST_NOT_FOUND);
	return WSAHOST_NOT_FOUND;   // EAI_NONAME
}
static int WINAPI net_GetAddrInfoW(const WCHAR* node, const WCHAR* service, const void* hints, void** res)
{
	char n[260];
	blocked("GetAddrInfoW", wide(node, n, sizeof(n)));
	if (res) *res = NULL;
	SetLastError(WSAHOST_NOT_FOUND);
	return WSAHOST_NOT_FOUND;
}
static int WINAPI net_sendto(UINT_PTR s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
	char a[120]; fmt_sockaddr(to, tolen, a, sizeof(a));
	blocked("sendto", a);
	SetLastError(WSAENETUNREACH);
	return SOCKET_ERROR;
}
static int WINAPI net_WSASendTo(UINT_PTR s, void* bufs, DWORD nbufs, LPDWORD sent, DWORD flags, const struct sockaddr* to, int tolen, void* ov, void* routine)
{
	char a[120]; fmt_sockaddr(to, tolen, a, sizeof(a));
	blocked("WSASendTo", a);
	SetLastError(WSAENETUNREACH);
	return SOCKET_ERROR;
}

// ---------------------------------------------------------------------------------------------------------- WinINet

static void* WINAPI net_InternetOpenA(LPCSTR agent, DWORD access, LPCSTR proxy, LPCSTR bypass, DWORD flags)
{
	char a[300]; _snprintf(a, sizeof(a), "agent '%s'", agent ? agent : "(null)"); a[sizeof(a) - 1] = 0;
	blocked("InternetOpenA", a);
	SetLastError(NET_CANNOT_CONNECT);
	return NULL;
}
static void* WINAPI net_InternetOpenW(LPCWSTR agent, DWORD access, LPCWSTR proxy, LPCWSTR bypass, DWORD flags)
{
	char n[200], a[300]; _snprintf(a, sizeof(a), "agent '%s'", wide(agent, n, sizeof(n))); a[sizeof(a) - 1] = 0;
	blocked("InternetOpenW", a);
	SetLastError(NET_CANNOT_CONNECT);
	return NULL;
}
static void* WINAPI net_InternetOpenUrlA(void* inet, LPCSTR url, LPCSTR headers, DWORD hlen, DWORD flags, DWORD_PTR ctx)
{
	blocked("InternetOpenUrlA", url ? url : "(null)");
	SetLastError(NET_CANNOT_CONNECT);
	return NULL;
}
static void* WINAPI net_InternetOpenUrlW(void* inet, LPCWSTR url, LPCWSTR headers, DWORD hlen, DWORD flags, DWORD_PTR ctx)
{
	char n[1000];
	blocked("InternetOpenUrlW", wide(url, n, sizeof(n)));
	SetLastError(NET_CANNOT_CONNECT);
	return NULL;
}
static void* WINAPI net_InternetConnectA(void* inet, LPCSTR server, DWORD port, LPCSTR user, LPCSTR pass, DWORD service, DWORD flags, DWORD_PTR ctx)
{
	char a[300]; _snprintf(a, sizeof(a), "%s:%u", server ? server : "(null)", (unsigned)(port & 0xffff)); a[sizeof(a) - 1] = 0;
	blocked("InternetConnectA", a);
	SetLastError(NET_CANNOT_CONNECT);
	return NULL;
}
static void* WINAPI net_InternetConnectW(void* inet, LPCWSTR server, DWORD port, LPCWSTR user, LPCWSTR pass, DWORD service, DWORD flags, DWORD_PTR ctx)
{
	char n[200], a[300]; _snprintf(a, sizeof(a), "%s:%u", wide(server, n, sizeof(n)), (unsigned)(port & 0xffff)); a[sizeof(a) - 1] = 0;
	blocked("InternetConnectW", a);
	SetLastError(NET_CANNOT_CONNECT);
	return NULL;
}
static BOOL WINAPI net_HttpSendRequestA(void* req, LPCSTR headers, DWORD hlen, void* optional, DWORD optlen)
{
	char a[200]; _snprintf(a, sizeof(a), "request %p, %lu bytes of body", req, (unsigned long)optlen); a[sizeof(a) - 1] = 0;
	blocked("HttpSendRequestA", a);
	SetLastError(NET_CANNOT_CONNECT);
	return FALSE;
}
static BOOL WINAPI net_HttpSendRequestW(void* req, LPCWSTR headers, DWORD hlen, void* optional, DWORD optlen)
{
	char a[200]; _snprintf(a, sizeof(a), "request %p, %lu bytes of body", req, (unsigned long)optlen); a[sizeof(a) - 1] = 0;
	blocked("HttpSendRequestW", a);
	SetLastError(NET_CANNOT_CONNECT);
	return FALSE;
}
static BOOL WINAPI net_HttpSendRequestExA(void* req, void* in, void* out, DWORD flags, DWORD_PTR ctx)
{
	char a[200]; _snprintf(a, sizeof(a), "request %p", req); a[sizeof(a) - 1] = 0;
	blocked("HttpSendRequestExA", a);
	SetLastError(NET_CANNOT_CONNECT);
	return FALSE;
}
static BOOL WINAPI net_HttpSendRequestExW(void* req, void* in, void* out, DWORD flags, DWORD_PTR ctx)
{
	char a[200]; _snprintf(a, sizeof(a), "request %p", req); a[sizeof(a) - 1] = 0;
	blocked("HttpSendRequestExW", a);
	SetLastError(NET_CANNOT_CONNECT);
	return FALSE;
}

// ---------------------------------------------------------------------------------------------------------- URLMon

static LONG WINAPI net_URLDownloadToFileA(void* caller, LPCSTR url, LPCSTR file, DWORD reserved, void* cb)
{
	char a[1200]; _snprintf(a, sizeof(a), "%s to %s", url ? url : "(null)", file ? file : "(null)"); a[sizeof(a) - 1] = 0;
	blocked("URLDownloadToFileA", a);
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_DOWNLOAD_FAIL;
}
static LONG WINAPI net_URLDownloadToFileW(void* caller, LPCWSTR url, LPCWSTR file, DWORD reserved, void* cb)
{
	char u[600], f[400], a[1100]; _snprintf(a, sizeof(a), "%s to %s", wide(url, u, sizeof(u)), wide(file, f, sizeof(f))); a[sizeof(a) - 1] = 0;
	blocked("URLDownloadToFileW", a);
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_DOWNLOAD_FAIL;
}
static LONG WINAPI net_URLDownloadToCacheFileA(void* caller, LPCSTR url, LPSTR file, DWORD cch, DWORD reserved, void* cb)
{
	blocked("URLDownloadToCacheFileA", url ? url : "(null)");
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_DOWNLOAD_FAIL;
}
static LONG WINAPI net_URLDownloadToCacheFileW(void* caller, LPCWSTR url, LPWSTR file, DWORD cch, DWORD reserved, void* cb)
{
	char u[1000];
	blocked("URLDownloadToCacheFileW", wide(url, u, sizeof(u)));
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_DOWNLOAD_FAIL;
}
static LONG WINAPI net_URLOpenStreamA(void* caller, LPCSTR url, DWORD reserved, void* cb)
{
	blocked("URLOpenStreamA", url ? url : "(null)");
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_CANNOT_CONNECT;
}
static LONG WINAPI net_URLOpenStreamW(void* caller, LPCWSTR url, DWORD reserved, void* cb)
{
	char u[1000];
	blocked("URLOpenStreamW", wide(url, u, sizeof(u)));
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_CANNOT_CONNECT;
}
static LONG WINAPI net_URLOpenPullStreamA(void* caller, LPCSTR url, DWORD reserved, void* cb)
{
	blocked("URLOpenPullStreamA", url ? url : "(null)");
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_CANNOT_CONNECT;
}
static LONG WINAPI net_URLOpenPullStreamW(void* caller, LPCWSTR url, DWORD reserved, void* cb)
{
	char u[1000];
	blocked("URLOpenPullStreamW", wide(url, u, sizeof(u)));
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_CANNOT_CONNECT;
}
static LONG WINAPI net_URLOpenBlockingStreamA(void* caller, LPCSTR url, void** stream, DWORD reserved, void* cb)
{
	blocked("URLOpenBlockingStreamA", url ? url : "(null)");
	if (stream) *stream = NULL;
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_CANNOT_CONNECT;
}
static LONG WINAPI net_URLOpenBlockingStreamW(void* caller, LPCWSTR url, void** stream, DWORD reserved, void* cb)
{
	char u[1000];
	blocked("URLOpenBlockingStreamW", wide(url, u, sizeof(u)));
	if (stream) *stream = NULL;
	SetLastError(NET_CANNOT_CONNECT);
	return NET_HR_CANNOT_CONNECT;
}

// ---------------------------------------------------------------------------------------------------------- WinHTTP

static void* WINAPI net_WinHttpOpen(LPCWSTR agent, DWORD access, LPCWSTR proxy, LPCWSTR bypass, DWORD flags)
{
	char n[200], a[300]; _snprintf(a, sizeof(a), "agent '%s'", wide(agent, n, sizeof(n))); a[sizeof(a) - 1] = 0;
	blocked("WinHttpOpen", a);
	SetLastError(NET_CANNOT_CONNECT);
	return NULL;
}
static void* WINAPI net_WinHttpConnect(void* session, LPCWSTR server, DWORD port, DWORD reserved)
{
	char n[200], a[300]; _snprintf(a, sizeof(a), "%s:%u", wide(server, n, sizeof(n)), (unsigned)(port & 0xffff)); a[sizeof(a) - 1] = 0;
	blocked("WinHttpConnect", a);
	SetLastError(NET_CANNOT_CONNECT);
	return NULL;
}
static BOOL WINAPI net_WinHttpSendRequest(void* req, LPCWSTR headers, DWORD hlen, void* optional, DWORD optlen, DWORD total, DWORD_PTR ctx)
{
	char a[200]; _snprintf(a, sizeof(a), "request %p, %lu bytes of body", req, (unsigned long)optlen); a[sizeof(a) - 1] = 0;
	blocked("WinHttpSendRequest", a);
	SetLastError(NET_CANNOT_CONNECT);
	return FALSE;
}

// ---------------------------------------------------------------------------------------------------------- launches
// The launch guard (docs/ORIGRUN.md, "The launch guard"): a program, file or link the copy starts. Seven Seas' "Submit to Internet
// High Score List" writes score<N>.html and opens it with ShellExecuteA, so the browser, not the game, goes to the network; "More
// Games" and "Buy" are links of the same kind. Each call fails at once with ERROR_ACCESS_DENIED (ShellExecute's SE_ERR_ACCESSDENIED,
// WinExec's value below 32) and one "LAUNCH BLOCKED" line in the same log.

#define LAUNCH_DENIED 5L                     // ERROR_ACCESS_DENIED = SE_ERR_ACCESSDENIED

static LONG gLaunches;                       // launches blocked in this process

static void launched(const char* fn, const char* what)
{
	InterlockedIncrement(&gLaunches);
	net_line("pid %lu: LAUNCH BLOCKED %s %s", GetCurrentProcessId(), fn, what ? what : "");
}

// "<verb> <file> <parameters>" of a ShellExecute call
static void fmt_shell(const char* verb, const char* file, const char* params, char* out, int cap)
{
	_snprintf(out, cap, "%s %s%s%s", verb ? verb : "(default verb)", file ? file : "(null)", params && params[0] ? " " : "", params ? params : "");
	out[cap - 1] = 0;
}

static void* WINAPI net_ShellExecuteA(HWND hwnd, LPCSTR verb, LPCSTR file, LPCSTR params, LPCSTR dir, INT show)
{
	char a[700]; fmt_shell(verb, file, params, a, sizeof(a));
	launched("ShellExecuteA", a);
	SetLastError(ERROR_ACCESS_DENIED);
	return (void*)(INT_PTR)LAUNCH_DENIED;
}
static void* WINAPI net_ShellExecuteW(HWND hwnd, LPCWSTR verb, LPCWSTR file, LPCWSTR params, LPCWSTR dir, INT show)
{
	char v[100], f[400], p[400], a[700];
	fmt_shell(verb ? wide(verb, v, sizeof(v)) : NULL, file ? wide(file, f, sizeof(f)) : NULL, params ? wide(params, p, sizeof(p)) : NULL, a, sizeof(a));
	launched("ShellExecuteW", a);
	SetLastError(ERROR_ACCESS_DENIED);
	return (void*)(INT_PTR)LAUNCH_DENIED;
}
static BOOL WINAPI net_ShellExecuteExA(SHELLEXECUTEINFOA* info)
{
	char a[700];
	if (info) fmt_shell(info->lpVerb, info->lpFile, info->lpParameters, a, sizeof(a)); else lstrcpynA(a, "(null)", sizeof(a));
	launched("ShellExecuteExA", a);
	if (info) { info->hInstApp = (HINSTANCE)(INT_PTR)LAUNCH_DENIED; info->hProcess = NULL; }
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE;
}
static BOOL WINAPI net_ShellExecuteExW(SHELLEXECUTEINFOW* info)
{
	char v[100], f[400], p[400], a[700];
	if (info) fmt_shell(info->lpVerb ? wide(info->lpVerb, v, sizeof(v)) : NULL, info->lpFile ? wide(info->lpFile, f, sizeof(f)) : NULL,
		info->lpParameters ? wide(info->lpParameters, p, sizeof(p)) : NULL, a, sizeof(a));
	else lstrcpynA(a, "(null)", sizeof(a));
	launched("ShellExecuteExW", a);
	if (info) { info->hInstApp = (HINSTANCE)(INT_PTR)LAUNCH_DENIED; info->hProcess = NULL; }
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE;
}
static UINT WINAPI net_WinExec(LPCSTR cmd, UINT show)
{
	launched("WinExec", cmd ? cmd : "(null)");
	SetLastError(ERROR_ACCESS_DENIED);
	return (UINT)LAUNCH_DENIED;
}
static BOOL WINAPI net_CreateProcessA(LPCSTR app, LPSTR cmd, void* procAttr, void* threadAttr, BOOL inherit, DWORD flags, void* env, LPCSTR dir, void* startup, PROCESS_INFORMATION* info)
{
	char a[700]; _snprintf(a, sizeof(a), "%s | %s", app ? app : "(no application)", cmd ? cmd : "(no command line)"); a[sizeof(a) - 1] = 0;
	launched("CreateProcessA", a);
	if (info) memset(info, 0, sizeof(*info));
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE;
}
static BOOL WINAPI net_CreateProcessW(LPCWSTR app, LPWSTR cmd, void* procAttr, void* threadAttr, BOOL inherit, DWORD flags, void* env, LPCWSTR dir, void* startup, PROCESS_INFORMATION* info)
{
	char p[300], c[400], a[700];
	_snprintf(a, sizeof(a), "%s | %s", app ? wide(app, p, sizeof(p)) : "(no application)", cmd ? wide(cmd, c, sizeof(c)) : "(no command line)"); a[sizeof(a) - 1] = 0;
	launched("CreateProcessW", a);
	if (info) memset(info, 0, sizeof(*info));
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE;
}

// ---------------------------------------------------------------------------------------------------------- the patches

typedef struct { const char* dll; const char* name; void* hook; HMODULE at; int guard; } NetDef;   // guard: 1 for a launch
static NetDef gNet[] = {
	{ "ws2_32.dll",  "connect",                  (void*)net_connect },
	{ "wsock32.dll", "connect",                  (void*)net_connect },
	{ "ws2_32.dll",  "WSAConnect",               (void*)net_WSAConnect },
	{ "ws2_32.dll",  "WSAConnectByNameA",        (void*)net_WSAConnectByNameA },
	{ "ws2_32.dll",  "WSAConnectByNameW",        (void*)net_WSAConnectByNameW },
	{ "ws2_32.dll",  "gethostbyname",            (void*)net_gethostbyname },
	{ "wsock32.dll", "gethostbyname",            (void*)net_gethostbyname },
	{ "ws2_32.dll",  "WSAAsyncGetHostByName",    (void*)net_WSAAsyncGetHostByName },
	{ "wsock32.dll", "WSAAsyncGetHostByName",    (void*)net_WSAAsyncGetHostByName },
	{ "ws2_32.dll",  "getaddrinfo",              (void*)net_getaddrinfo },
	{ "wship6.dll",  "getaddrinfo",              (void*)net_getaddrinfo },
	{ "ws2_32.dll",  "GetAddrInfoW",             (void*)net_GetAddrInfoW },
	{ "ws2_32.dll",  "sendto",                   (void*)net_sendto },
	{ "wsock32.dll", "sendto",                   (void*)net_sendto },
	{ "ws2_32.dll",  "WSASendTo",                (void*)net_WSASendTo },
	{ "wininet.dll", "InternetOpenA",            (void*)net_InternetOpenA },
	{ "wininet.dll", "InternetOpenW",            (void*)net_InternetOpenW },
	{ "wininet.dll", "InternetOpenUrlA",         (void*)net_InternetOpenUrlA },
	{ "wininet.dll", "InternetOpenUrlW",         (void*)net_InternetOpenUrlW },
	{ "wininet.dll", "InternetConnectA",         (void*)net_InternetConnectA },
	{ "wininet.dll", "InternetConnectW",         (void*)net_InternetConnectW },
	{ "wininet.dll", "HttpSendRequestA",         (void*)net_HttpSendRequestA },
	{ "wininet.dll", "HttpSendRequestW",         (void*)net_HttpSendRequestW },
	{ "wininet.dll", "HttpSendRequestExA",       (void*)net_HttpSendRequestExA },
	{ "wininet.dll", "HttpSendRequestExW",       (void*)net_HttpSendRequestExW },
	{ "urlmon.dll",  "URLDownloadToFileA",       (void*)net_URLDownloadToFileA },
	{ "urlmon.dll",  "URLDownloadToFileW",       (void*)net_URLDownloadToFileW },
	{ "urlmon.dll",  "URLDownloadToCacheFileA",  (void*)net_URLDownloadToCacheFileA },
	{ "urlmon.dll",  "URLDownloadToCacheFileW",  (void*)net_URLDownloadToCacheFileW },
	{ "urlmon.dll",  "URLOpenStreamA",           (void*)net_URLOpenStreamA },
	{ "urlmon.dll",  "URLOpenStreamW",           (void*)net_URLOpenStreamW },
	{ "urlmon.dll",  "URLOpenPullStreamA",       (void*)net_URLOpenPullStreamA },
	{ "urlmon.dll",  "URLOpenPullStreamW",       (void*)net_URLOpenPullStreamW },
	{ "urlmon.dll",  "URLOpenBlockingStreamA",   (void*)net_URLOpenBlockingStreamA },
	{ "urlmon.dll",  "URLOpenBlockingStreamW",   (void*)net_URLOpenBlockingStreamW },
	{ "winhttp.dll", "WinHttpOpen",              (void*)net_WinHttpOpen },
	{ "winhttp.dll", "WinHttpConnect",           (void*)net_WinHttpConnect },
	{ "winhttp.dll", "WinHttpSendRequest",       (void*)net_WinHttpSendRequest },
	// the launch guard (kernel32's two are a stub in front of kernelbase's on some Windows versions, or a forwarder: both are listed)
	{ "shell32.dll",    "ShellExecuteA",         (void*)net_ShellExecuteA,   NULL, 1 },
	{ "shell32.dll",    "ShellExecuteW",         (void*)net_ShellExecuteW,   NULL, 1 },
	{ "shell32.dll",    "ShellExecuteExA",       (void*)net_ShellExecuteExA, NULL, 1 },
	{ "shell32.dll",    "ShellExecuteExW",       (void*)net_ShellExecuteExW, NULL, 1 },
	{ "kernel32.dll",   "WinExec",               (void*)net_WinExec,         NULL, 1 },
	{ "kernelbase.dll", "WinExec",               (void*)net_WinExec,         NULL, 1 },
	{ "kernel32.dll",   "CreateProcessA",        (void*)net_CreateProcessA,  NULL, 1 },
	{ "kernel32.dll",   "CreateProcessW",        (void*)net_CreateProcessW,  NULL, 1 },
	{ "kernelbase.dll", "CreateProcessA",        (void*)net_CreateProcessA,  NULL, 1 },
	{ "kernelbase.dll", "CreateProcessW",        (void*)net_CreateProcessW,  NULL, 1 },
};
#define NNET ((int)(sizeof(gNet) / sizeof(gNet[0])))

// The address of an export by name, read from the export table without loading anything; NULL for a forwarded export (its
// address in the table lies inside the export directory) or a name the DLL does not export.
static BYTE* find_export(HMODULE m, const char* name)
{
	BYTE* base = (BYTE*)m;
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
	IMAGE_NT_HEADERS32* nt = (IMAGE_NT_HEADERS32*)(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return NULL;
	IMAGE_DATA_DIRECTORY dd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!dd.VirtualAddress || !dd.Size) return NULL;
	IMAGE_EXPORT_DIRECTORY* ex = (IMAGE_EXPORT_DIRECTORY*)(base + dd.VirtualAddress);
	DWORD* names = (DWORD*)(base + ex->AddressOfNames);
	WORD* ords = (WORD*)(base + ex->AddressOfNameOrdinals);
	DWORD* funcs = (DWORD*)(base + ex->AddressOfFunctions);
	for (DWORD i = 0; i < ex->NumberOfNames; i++) {
		if (strcmp((const char*)(base + names[i]), name) != 0) continue;
		DWORD rva = funcs[ords[i]];
		if (rva >= dd.VirtualAddress && rva < dd.VirtualAddress + dd.Size) return NULL;   // a forwarder
		return base + rva;
	}
	return NULL;
}

static int detour(BYTE* f, void* hook)
{
	DWORD old;
	if (!VirtualProtect(f, 5, PAGE_EXECUTE_READWRITE, &old)) return 0;
	LONG rel = (LONG)((BYTE*)hook - (f + 5));
	f[0] = 0xE9; memcpy(f + 1, &rel, 4);
	VirtualProtect(f, 5, old, &old);
	FlushInstructionCache(GetCurrentProcess(), f, 5);
	return 1;
}

static LONG gScanBusy;
// Patch every listed function of the DLLs that are mapped now and not patched yet (a DLL unloaded and mapped again, at
// another address, is patched again). Loads nothing.
static void net_scan(void)
{
	if (InterlockedExchange(&gScanBusy, 1)) return;
	for (int i = 0; i < NNET; i++) {
		HMODULE m = GetModuleHandleA(gNet[i].dll);
		if (!m || m == gNet[i].at) continue;
		gNet[i].at = m;
		BYTE* f = find_export(m, gNet[i].name);
		if (!f) continue;   // forwarded: the DLL it forwards to is patched on its own
		const char* what = gNet[i].guard ? "launch guard" : "network block";
		if (detour(f, gNet[i].hook)) net_line("pid %lu: %s: %s!%s patched", GetCurrentProcessId(), what, gNet[i].dll, gNet[i].name);
		else net_line("pid %lu: %s: %s!%s could not be patched (err %lu)", GetCurrentProcessId(), what, gNet[i].dll, gNet[i].name, GetLastError());
	}
	InterlockedExchange(&gScanBusy, 0);
}

// The module scan the other hooks make (vclock.c, hooks_rescan): a DLL mapped since the last look.
void net_rescan(void)
{
	if (gInstalled) net_scan();
}

// The loader's own notice of a DLL that has been mapped (Vista and later): the loader lock is held, nothing is loaded here.
typedef struct { USHORT Length, MaximumLength; PWSTR Buffer; } NetUStr;
typedef struct { ULONG Flags; const NetUStr* FullDllName; const NetUStr* BaseDllName; PVOID DllBase; ULONG SizeOfImage; } NetLdrData;
typedef VOID (CALLBACK *NetLdrNotify)(ULONG reason, const NetLdrData* data, PVOID ctx);
typedef LONG (NTAPI *NetLdrRegister)(ULONG flags, NetLdrNotify fn, PVOID ctx, PVOID* cookie);
static VOID CALLBACK net_loaded(ULONG reason, const NetLdrData* data, PVOID ctx)
{
	if (reason == 1 /* LDR_DLL_NOTIFICATION_REASON_LOADED */) net_scan();
}

// Called from the stand-in's DllMain on process attach, first of all.
void net_install(HMODULE self)
{
	HMODULE pin = NULL;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)(void*)net_install, &pin)) return;   // pinned: the jumps point into this DLL
	net_open_log();
	gInstalled = 1;
	char exe[MAX_PATH]; GetModuleFileNameA(NULL, exe, MAX_PATH);
	net_line("pid %lu %s: network block installed: WinSock, WinINet, URLMon and WinHTTP calls fail at once; launch guard installed: ShellExecute, WinExec and CreateProcess calls fail at once; a DLL mapped later is patched when it loads", GetCurrentProcessId(), exe);
	net_scan();
	HMODULE nt = GetModuleHandleA("ntdll.dll");
	NetLdrRegister reg = nt ? (NetLdrRegister)(void*)GetProcAddress(nt, "LdrRegisterDllNotification") : NULL;
	PVOID cookie = NULL;
	if (reg) reg(0, net_loaded, NULL, &cookie);
}

LONG net_blocked_count(void) { return gBlocked; }

// Called on process detach: the total.
void net_detach(void)
{
	if (!gInstalled) return;
	net_line("pid %lu exits: network block: %ld call%s blocked; launch guard: %ld launch%s blocked", GetCurrentProcessId(), (long)gBlocked, gBlocked == 1 ? "" : "s",
		(long)gLaunches, gLaunches == 1 ? "" : "es");
}
