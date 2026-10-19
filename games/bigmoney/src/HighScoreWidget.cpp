#include "HighScoreWidget.h"
#include "WinBMApp.h"
#include "Board.h"
#include "MrMoney.h"
#include "Res.h"

#include "Graphics.h"
#include "Font.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "Buffer.h"

#include <cstdlib>
#include <ctime>
#include "ModRules.h"

using namespace Sexy;

// The original opens "hiscores.dat" relative to the game folder. port: the framework's file helpers
// write it to the user dir; a read falls back to the game folder, so the original's file is picked up
// until the first save.
static const char* HIGHSCORE_FILE_NAME = "hiscores.dat";

// Record checksum: score ^ 0x42BEEF ^ (each name byte, sign-extended, * 0x11)
static int GetHighScoreChecksum(const char* theName, int theLength, int theScore)
{
	int aChecksum = theScore ^ HighScoreWidget::HIGHSCORE_CHECKSUM_KEY;
	for (int i = 0; i < theLength; i++)
		aChecksum ^= (signed char) theName[i] * 0x11;
	return aChecksum;
}

// port: hiscores.dat is read from a memory image instead of with fread. Little-endian.
namespace
{
class HighScoreReader
{
public:
	const uchar*	mData;
	int				mLength;
	int				mPos;

	HighScoreReader(const uchar* theData, int theLength) : mData(theData), mLength(theLength), mPos(0) {}

	bool ReadBytes(char* theDest, int theCount)
	{
		if ((theCount < 0) || (mPos + theCount > mLength))
			return false;
		for (int i = 0; i < theCount; i++)
			theDest[i] = (char) mData[mPos + i];
		mPos += theCount;
		return true;
	}

	bool ReadShort(short* theValue)
	{
		if (mPos + 2 > mLength)
			return false;
		*theValue = (short) (unsigned short) (mData[mPos] | (mData[mPos + 1] << 8));
		mPos += 2;
		return true;
	}

	bool ReadInt(int* theValue)
	{
		if (mPos + 4 > mLength)
			return false;
		*theValue = (int) ((unsigned int) mData[mPos] | ((unsigned int) mData[mPos + 1] << 8) |
			((unsigned int) mData[mPos + 2] << 16) | ((unsigned int) mData[mPos + 3] << 24));
		mPos += 4;
		return true;
	}
};
}

// One record: int16 length, the name bytes (no NUL), int32 score, int32 checksum. Returns false at the
// end of the data.
static bool ReadHighScoreRecord(HighScoreReader* theReader, std::string* theName, int* theScore, bool* isValid)
{
	short aLength;
	if (!theReader->ReadShort(&aLength))
		return false;

	// port: the original reads any length into a 256-byte stack buffer unchecked; a length outside
	// 0..255 (never written by the game) is treated like the end of the file
	if ((aLength < 0) || (aLength > 255))
		return false;

	char aName[256];
	if (!theReader->ReadBytes(aName, aLength))
		return false;
	aName[aLength] = 0;

	int aChecksum;
	if ((!theReader->ReadInt(theScore)) || (!theReader->ReadInt(&aChecksum)))
		return false;

	// The checksum covers all aLength bytes, the name only the part before an embedded NUL (strlen)
	*theName = aName;
	*isValid = aChecksum == GetHighScoreChecksum(aName, aLength, *theScore);
	return true;
}

// Writes one record in the format ReadHighScoreRecord reads
static void WriteHighScoreRecord(Buffer* theBuffer, const std::string& theName, int theScore)
{
	// The original writes the low 16 bits of the length (names are at most 140 px wide)
	short aLength = (short) theName.length();
	theBuffer->WriteShort(aLength);
	theBuffer->WriteBytes((const uchar*) theName.c_str(), aLength);
	theBuffer->WriteLong(theScore);
	theBuffer->WriteLong(GetHighScoreChecksum(theName.c_str(), aLength, theScore));
}

// 0x412b80
int HighScoreWidget::GetDayNumber()
{
	time_t aTime = time(NULL);
	tm* aLocalTime = localtime(&aTime);
	return aLocalTime->tm_year * 366 + aLocalTime->tm_yday;
}

// 0x412bb0
int HighScoreWidget::GetRank(int theMode, int theScore)
{
	// The lowest index whose score is below theScore; ties do not place
	int aRank = 0;
	for (int i = NUM_HIGH_SCORES - 1; i >= 0; i--)
	{
		if (mScores[theMode][i] < theScore)
			aRank = i + 1;
	}
	return aRank;
}

// 0x412bf0
void HighScoreWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNewGameButton);
	// port: the original also adds the internet submit button (+0x90): not ported
}

// 0x412c20
void HighScoreWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNewGameButton);
}

// 0x412eb0
void HighScoreWidget::LoadHighScores()
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(mFileName, &aBuffer))
		return;

	// port: the original never checks fread and reuses stale locals on a short file; here loading stops
	// at the end of the data and the records not read keep their defaults
	HighScoreReader aReader(aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	std::string aName;
	int aScore;
	bool isValid;
	for (int aMode = 0; aMode < NUM_HIGH_SCORE_MODES; aMode++)
	{
		int aDay;
		if ((!aReader.ReadInt(&aDay)) || (!ReadHighScoreRecord(&aReader, &aName, &aScore, &isValid)))
			break;

		// Today's best only counts on the day it was saved
		if ((aDay == GetDayNumber()) && (isValid))
		{
			mTodayNames[aMode] = aName;
			mTodayScores[aMode] = aScore;
		}

		int i;
		for (i = 0; i < NUM_HIGH_SCORES; i++)
		{
			if (!ReadHighScoreRecord(&aReader, &aName, &aScore, &isValid))
				break;

			// A tampered record keeps its name, but its score drops to score % 256
			mNames[aMode][i] = aName;
			mScores[aMode][i] = isValid ? aScore : aScore % 256;
		}
		if (i < NUM_HIGH_SCORES)
			break;
	}
}

// 0x413100
void HighScoreWidget::SaveHighScores()
{
	mApp->EraseFile(mFileName);

	Buffer aBuffer;
	for (int aMode = 0; aMode < NUM_HIGH_SCORE_MODES; aMode++)
	{
		// Always stamped with today, so an older "today" record is re-stamped
		aBuffer.WriteLong(GetDayNumber());
		WriteHighScoreRecord(&aBuffer, mTodayNames[aMode], mTodayScores[aMode]);
		for (int i = 0; i < NUM_HIGH_SCORES; i++)
			WriteHighScoreRecord(&aBuffer, mNames[aMode][i], mScores[aMode][i]);
	}

	// port: the original pops this up when fopen fails, before writing anything
	if (!mApp->WriteBufferToFile(mFileName, &aBuffer))
		mApp->Popup("Failed to create high score file!\nPlease report error to support@popcap.com.");
}

// 0x4133b0
void HighScoreWidget::DoCheckForHighScore()
{
	int aScore = mApp->mBoard->mScore;
	int aMode = mApp->mGameType;
	bool isBestToday = aScore > mTodayScores[aMode];
	int aRank = GetRank(aMode, aScore);

	if ((isBestToday) || (aRank > 0))
	{
		mApp->PlaySample(SOUND_ANEWHIGHSCORE);
		mApp->mMrMoney->PlayAnim(3);
	}

	// port: the original takes this branch only if app->mIsRegistered; unregistered, it built the same
	// messages from the ..._REGISTER string ids and called ShowRegisterToRecordScoreDialog. The Steam
	// build is registered (NoReg), so only this branch is ported.
	// The property strings already carry spaces at their ends, so some messages get double spaces.
	std::string aRankString = StrFormat("%d", aRank);
	std::string aMessage;
	if (isBestToday)
	{
		if (aRank < 1)
			aMessage = mApp->GetString("YOU_GOT_THE_HIGHEST_SCORE_OF_THE_DAY");
		else
			aMessage = mApp->GetString("HSOTD_AND_PLACE1") + " " + aRankString + " " + mApp->GetString("HSOTD_AND_PLACE2");
	}
	else if (aRank > 0)
		aMessage = mApp->GetString("YOU_GOT_PLACE_") + " " + aRankString + " " + mApp->GetString("_OF_ALL_TIME");
	else
		return;

	mApp->ShowRecordScoreDialog(aMessage);
}

// 0x413b10
void HighScoreWidget::Draw(Graphics* g)
{
	Graphics aG(*g);

	// Hides everything above screen y 252 (the widget is top-level, so mY is a screen y). The height
	// term only ever makes the clip taller than the widget (probably meant mHeight - (252 - mY)).
	aG.ClipRect(0, CLIP_SCREEN_Y - mY, mWidth, mHeight + (CLIP_SCREEN_Y - mY));
	aG.DrawImageBox(Rect(0, 0, mWidth, mHeight), mApp->mImages[IMAGE_OLDDIALOG]);

	aG.SetFont(mApp->mFont3); // replaced right away: every string below is in font 1
	int aMode = mApp->mGameType;
	aG.SetFont(mApp->mFont1);
	aG.SetColor(Color(255, 255, 255));

	std::string aString = mApp->GetString("HIGH_SCORES");
	aG.DrawString(aString, (mWidth - aG.GetFont()->StringWidth(aString)) / 2, 48);

	// Each row cycles red -> yellow -> green: hue |((cycle + row) * 10) % 170 - 85|
	for (int i = 0; i < NUM_HIGH_SCORES; i++)
	{
		int aY = 70 + i * 20;
		int aHue = abs(((mColorCycle + i) * 10) % 170 - 85);
		Color aRankColor((int) mApp->HSLToRGB(aHue, 255, 150));
		Color aNameColor((int) mApp->HSLToRGB(aHue, 255, 96));

		aG.SetColor(aRankColor);
		aG.DrawString(StrFormat("%d.", i + 1), 49, aY);

		aG.SetColor(aNameColor);
		aG.DrawString(mNames[aMode][i], 67, aY);

		aG.SetColor(aRankColor);
		aString = StrFormat("%d", mScores[aMode][i]);
		aG.DrawString(aString, 349 - aG.GetFont()->StringWidth(aString), aY);
	}

	aG.SetFont(mApp->mFont1);
	aG.SetColor(Color(255, 255, 255));
	aString = mApp->GetString("TODAYS_HIGH_SCORES");
	aG.DrawString(aString, (mWidth - aG.GetFont()->StringWidth(aString)) / 2, 176);

	aG.SetColor(Color((int) mApp->HSLToRGB(abs(((mColorCycle + 8) * 10) % 170 - 85), 255, 150)));
	if (mTodayScores[aMode] == 0)
	{
		aString = mApp->GetString("NOT_APPLICABLE_SHORTHAND");
		aG.DrawString(aString, (mWidth - aG.GetFont()->StringWidth(aString)) / 2, 198);
	}
	else
	{
		aString = StrFormat("%d", mTodayScores[aMode]);
		aG.DrawString(aString, 349 - aG.GetFont()->StringWidth(aString), 198);
		aG.DrawString(mTodayNames[aMode], 49, 198);
	}
}

// 0x4144e0
void HighScoreWidget::Update()
{
	Widget::Update();

	// About 2.8 s after the Board added the widget (its slide ends at tick 253)
	if (mUpdateCnt == CHECK_HIGH_SCORE_TICK)
		DoCheckForHighScore();

	if (mUpdateCnt % 10 == 0)
	{
		mColorCycle++;
		MarkDirty();
	}
}

// 0x414520
void HighScoreWidget::CheckForHighScore()
{
	DoCheckForHighScore();
}

// 0x414530
void HighScoreWidget::AddHighScore(const std::string& theName)
{
	if (theName.length() == 0)
		return;

	int aScore = mApp->mBoard->mScore;
	int aMode = mApp->mGameType;
	int aRank = GetRank(aMode, aScore);

	if (aScore > mTodayScores[aMode])
	{
		mTodayScores[aMode] = aScore;
		mTodayNames[aMode] = theName;
	}

	if (aRank != 0)
	{
		if (aRank <= 4)
		{
			for (int i = NUM_HIGH_SCORES - 1; i >= aRank; i--)
			{
				mNames[aMode][i] = mNames[aMode][i - 1];
				mScores[aMode][i] = mScores[aMode][i - 1];
			}
		}
		mNames[aMode][aRank - 1] = theName;
		mScores[aMode][aRank - 1] = aScore;
	}

	SaveHighScores();
}

// 0x414640
HighScoreWidget::HighScoreWidget(WinBMApp* theApp, bool theSeededTables)
{
	mApp = theApp;
	mColorCycle = 0;
	mFileName = ScopedUserFileName(HIGHSCORE_FILE_NAME, theSeededTables); // port(mods)

	// port: the original first creates the internet submit button (id 0, label
	// CLICK_HERE_TO_SUBMIT_TO_INTERNET_HIGH_SCORE_LIST, never sized or handled): not ported

	// Never sized either, so it never shows; the Board's own play-again button does this job
	mNewGameButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], NEW_GAME_BUTTON_ID, this);
	mNewGameButton->mLabel = mApp->GetString("CLICK_HERE_TO_START_A_NEW_GAME");
	mNewGameButton->SetFont(mApp->mFont3);
	mNewGameButton->SetColor(DialogButton::COLOR_LABEL, Color(224, 183, 40));

	mTodayScores[GAMETYPE_ACTION] = 0;
	mNames[GAMETYPE_ACTION][0] = "Ace";
	mScores[GAMETYPE_ACTION][0] = 1000;
	mNames[GAMETYPE_ACTION][1] = "John";
	mScores[GAMETYPE_ACTION][1] = 900;
	mNames[GAMETYPE_ACTION][2] = "Chris";
	mScores[GAMETYPE_ACTION][2] = 800;
	mNames[GAMETYPE_ACTION][3] = "Brian";
	mScores[GAMETYPE_ACTION][3] = 700;
	mNames[GAMETYPE_ACTION][4] = "Ben";
	mScores[GAMETYPE_ACTION][4] = 600;

	mTodayScores[GAMETYPE_STRATEGY] = 0;
	mNames[GAMETYPE_STRATEGY][0] = "Bradley";
	mScores[GAMETYPE_STRATEGY][0] = 1000;
	mNames[GAMETYPE_STRATEGY][1] = "Mike";
	mScores[GAMETYPE_STRATEGY][1] = 900;
	mNames[GAMETYPE_STRATEGY][2] = "Billy";
	mScores[GAMETYPE_STRATEGY][2] = 800;
	mNames[GAMETYPE_STRATEGY][3] = "Jason";
	mScores[GAMETYPE_STRATEGY][3] = 700;
	mNames[GAMETYPE_STRATEGY][4] = "Wim";
	mScores[GAMETYPE_STRATEGY][4] = 600;

	mTodayScores[GAMETYPE_PUZZLE] = 0;
	mNames[GAMETYPE_PUZZLE][0] = "Steve";
	mScores[GAMETYPE_PUZZLE][0] = 1000;
	mNames[GAMETYPE_PUZZLE][1] = "Terra";
	mScores[GAMETYPE_PUZZLE][1] = 900;
	mNames[GAMETYPE_PUZZLE][2] = "Roch";
	mScores[GAMETYPE_PUZZLE][2] = 800;
	mNames[GAMETYPE_PUZZLE][3] = "Kathy";
	mScores[GAMETYPE_PUZZLE][3] = 700;
	mNames[GAMETYPE_PUZZLE][4] = "Pam";
	mScores[GAMETYPE_PUZZLE][4] = 600;

	LoadHighScores();
}

// 0x414c50
HighScoreWidget::~HighScoreWidget()
{
	// (the original also deletes +0x94, +0x98 (always NULL) and the internet submit button)
	delete mNewGameButton;

	// Every destruction rewrites hiscores.dat, HighScoreDialog's private copy included
	SaveHighScores();
}

// 0x401c70 (one body with Board's ButtonPress)
void HighScoreWidget::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}
