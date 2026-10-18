#ifndef __PAKINTERFACE_H__
#define __PAKINTERFACE_H__

// port: portable replacement for PakLib's PakInterface.h. Bejeweled ships loose files (no main.pak),
// so these behave like the original's stdio fallback, but every file is looked up through
// Platform::VFS (mod overlay, case-insensitive names, '\' or '/' separators).
//
// Read-only opens load the whole file into memory. Without 'b' in the mode, the Win32 CRT text-mode
// rules are applied (CR LF -> LF, Ctrl-Z ends the file) so parsing is identical on every platform.
// Modes that write ('w', 'a', '+') use a FILE* from Platform::VFS::Open (i.e. the user directory).

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <vector>

struct PFILE
{
	std::vector<uint8_t>	mData;	// file contents (read-only modes)
	long					mPos;
	bool					mEOF;
	FILE*					mFP;	// writable modes only
};

PFILE*	p_fopen(const char* theFileName, const char* theAccess);
int		p_fclose(PFILE* theFile);
int		p_fseek(PFILE* theFile, long theOffset, int theOrigin);
int		p_ftell(PFILE* theFile);
size_t	p_fread(void* thePtr, int theSize, int theCount, PFILE* theFile);
size_t	p_fwrite(const void* thePtr, int theSize, int theCount, PFILE* theFile);
int		p_fgetc(PFILE* theFile);
int		p_ungetc(int theChar, PFILE* theFile);
char*	p_fgets(char* thePtr, int theSize, PFILE* theFile);
int		p_feof(PFILE* theFile);

#endif //__PAKINTERFACE_H__
