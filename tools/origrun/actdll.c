// A second module for actest.exe: it fills the table of activation calls from its own import table, so the calls are made
// from a DLL (the way ddraw.dll and the d3d DLLs call user32). Built by build.sh.
#include "actcheck.h"
__declspec(dllexport) void actdll_api(ActApi* a) { ACT_FILL_FROM_IMPORTS(a); }
BOOL WINAPI DllMain(HINSTANCE h, DWORD r, LPVOID p) { return TRUE; }
