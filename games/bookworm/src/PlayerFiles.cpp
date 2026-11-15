#include "PlayerFiles.h"
#include "BookWormApp.h"
#include "UserManager.h"
#include "platform/VFS.h"
#include <ctime>

using namespace Sexy;

static const char* const gDayNames[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char* const gMonthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// port(mod): today's date as yyyymmdd (the word journal and the daily board)
int Sexy::LocalDate()
{
	time_t aNow = time(NULL);
	struct tm* aTime = localtime(&aNow);
	if (aTime == NULL)
		return 20000101;
	return (aTime->tm_year + 1900) * 10000 + (aTime->tm_mon + 1) * 100 + aTime->tm_mday;
}

// port(mod): a date as "12 Nov", with its weekday when asked (the word journal)
SexyString Sexy::DateName(int theDate, bool withDay)
{
	int aYear = theDate / 10000;
	int aMonth = (theDate / 100) % 100;
	int aDay = theDate % 100;
	if ((aMonth < 1) || (aMonth > 12))
		return StrFormat("%d", theDate);
	SexyString aName = StrFormat("%d %s", aDay, gMonthNames[aMonth - 1]);
	if (!withDay)
		return aName;
	// Sakamoto's day of the week, 0 = Sunday
	static const int aMonthOffsets[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	int aY = (aMonth < 3) ? aYear - 1 : aYear;
	int aWeekDay = (aY + aY / 4 - aY / 100 + aY / 400 + aMonthOffsets[aMonth - 1] + aDay) % 7;
	return SexyString(gDayNames[aWeekDay]) + " " + aName;
}

// port(mod): the path of a file in the current player's folder (the word journal and the replays)
SexyString Sexy::PlayerFilePath(BookWormApp* theApp, const char* theFileName)
{
	SexyString aName = theApp->mUserManager->GetCurrentUserName();
	SexyString aPath = "userdata/players/";
	if (aName.length() != 0)
		aPath += aName + "/";
	return aPath + theFileName;
}

// port(mod): adds lines to a file in the current player's folder, with a header when it is new
bool Sexy::AppendPlayerFile(BookWormApp* theApp, const char* theFileName, const SexyString& theHeader,
	const SexyString& theLines)
{
	SexyString aPath = PlayerFilePath(theApp, theFileName);
	FILE* aFile = Platform::VFS::Get().Open(aPath, "rb");
	bool isNew = aFile == NULL;
	if (aFile != NULL)
		fclose(aFile);

	aFile = Platform::VFS::Get().Open(aPath, "ab");
	if (aFile == NULL)
		return false;
	if (isNew)
		fputs(theHeader.c_str(), aFile);
	fputs(theLines.c_str(), aFile);
	fclose(aFile);
	theApp->SyncUserData();  // web persistence of the user dir
	return true;
}
