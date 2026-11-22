// The game's high scores: the manager made by 0x671490 and the local server it keeps (LocalLeaderboardServer, vtable
// 0x86fc04 (bejeweled3), 0x44 bytes, made by 0x671ad0), which holds users\hiscores.dat. The manager also holds the
// Steam leaderboards' server (SteamServer); Steam is left out. The records screen asks the local server for its data
// and the server reads the file when asked (Update, 0x671d90).
#pragma once

#include "Buffer.h"

#include <string>

namespace Sexy {

class LocalLeaderboardServer {
public:
	Buffer mData;                           // +0x4: the file's bytes once they have been read
	std::string mFileName;                  // +0x8: users\hiscores.dat
	bool mClosed = false;                   // +0x3c: set by the shutdown call (0x671820): nothing more is read
	bool mHasData = false;                  // +0x3d: the file was read
	int mRequestId = 0;                     // +0x40: the request the data answers (0 none)

	explicit LocalLeaderboardServer(const std::string& theFileName);  // 0x671ad0
	// 0x671d90: reads the file once it is there
	void Update();
	// 0x671a30: the data when it has been read (false, and the flag cleared, when there is none)
	bool GetReceivedData(Buffer** theData);
};

class HighScoreManager {
public:
	LocalLeaderboardServer* mLocalServer = nullptr;  // +0x4
	// port: +0x0 is the Steam leaderboards' server (SteamServer, 0x671490), which the port does not have

	~HighScoreManager();

	// 0x671490: the one manager, made when first asked for
	static HighScoreManager* Get();
	// 0x6717a0: makes the local server for the file
	void LoadHighScores(const std::string& theFileName);
};

}  // namespace Sexy
