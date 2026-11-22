// The high-score manager and its local server: see the header.
#include "HighScores.h"

#include "Bej3App.h"

using namespace Sexy;

namespace {

HighScoreManager* gHighScoreManager = nullptr;  // 0x8e1724

}  // namespace

LocalLeaderboardServer::LocalLeaderboardServer(const std::string& theFileName) : mFileName(theFileName)
{
}

void LocalLeaderboardServer::Update()
{
	// 0x671d90 reads the file when nothing has been read and the server is open; the data's use (0x6719a0 and
	// 0x671b80, which answer a request for the records) comes with the records screen
	if (!mHasData && !mClosed && gApp->FileExists(mFileName))
		mHasData = gApp->ReadBufferFromFile(mFileName, &mData);
}

bool LocalLeaderboardServer::GetReceivedData(Buffer** theData)
{
	if (mHasData) {
		*theData = &mData;
		return true;
	}
	mHasData = false;
	mRequestId = 0;
	return false;
}

// port: the original never destroys its manager (a process-lifetime singleton); the port frees the local server
HighScoreManager::~HighScoreManager()
{
	delete mLocalServer;
}

HighScoreManager* HighScoreManager::Get()
{
	if (gHighScoreManager == nullptr)
		gHighScoreManager = new HighScoreManager();
	return gHighScoreManager;
}

void HighScoreManager::LoadHighScores(const std::string& theFileName)
{
	mLocalServer = new LocalLeaderboardServer(theFileName);
}
