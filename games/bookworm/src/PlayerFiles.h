#ifndef __BOOKWORM_PLAYERFILES_H__
#define __BOOKWORM_PLAYERFILES_H__

// port(mod): what the mods keep in the player's folder, beside the saved games (BookWormApp::GetSaveGamePath):
// the daily board's results ("daily-board.txt", DailyBoard.h) and the word journal ("word-journal.txt",
// WordJournal.h), text files a line per record, with the dates as yyyymmdd.

#include "Common.h"

namespace Sexy
{
class BookWormApp;

int				LocalDate();                                        // today's local date, yyyymmdd
SexyString		DateName(int theDate, bool withDay);               // "25 Oct", or "Sat 25 Oct"
// The path of theFileName in the current player's folder, for VFS reads and writes ("userdata/players/<name>/")
SexyString		PlayerFilePath(BookWormApp* theApp, const char* theFileName);
// Appends theLines to the file (theHeader first when the file is new) and syncs the user dir (web)
bool			AppendPlayerFile(BookWormApp* theApp, const char* theFileName, const SexyString& theHeader,
					const SexyString& theLines);

}

#endif
