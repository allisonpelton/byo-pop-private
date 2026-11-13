#include "HighScoreDisplayDialog.h"

#include <algorithm>
#include <cmath>
#include <ctime>

#include "Application.h"
#include "Buffer.h"
#include "Common.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "NcMovingDialog.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

// The names the places start with (HighScoreNames.inc is made by tools/highscorenames.py from the exe; they are the game's own text,
// so the file is not kept in the repository)
#include "HighScoreNames.inc"

namespace
{

// 0x454c30: where the tables are kept
const char* const TABLES_FILE_NAME = "users\\hiscores.dat";

// The scores the places start with: the first place has the first and each later place 50000 less
const int FIRST_PLACE_SCORE = 750000;
const int PLACE_SCORE_STEP = 50000;

// What a name and its score are checked against when the file is read: the score and the letters of the name are mixed into a number
const unsigned int CHECK_SEED = 0x42beef;
const int CHECK_LETTER_FACTOR = 0x11;

// How many days a year has for the date a best of the day is kept under (the exe's own count: 366)
const int DAYS_PER_YEAR = 0x16e;

// The names of the two tables
const char* const TABLE_TITLES[2] = { "Classic", "Marathon" };

// Where the second table is drawn, to the right of the first
const int SECOND_TABLE_X = 0x1a0;

// Where the lines are: the first place and how far a place is below the one before it, and the last y a place may have
const int FIRST_PLACE_Y = 0x10e;
const int PLACE_HEIGHT = 0x16;
const int PLACES_END_Y = 0x1ea;

// Where the pieces of a line are, from the left of its table: the number (its right side), the name, the score (its right side) and
// the small cookie of a game that was tough
const int NUMBER_RIGHT_X = 0x4f;
const int NAME_X = 0x55;
const int SCORE_RIGHT_X = 0x143;
const int COOKIE_X = 0x157;
const int COOKIE_RISE = 0xe;

// Where the other lines are
const int SUBTITLE_Y = 0xb4;
const int TITLE_Y = 0xf0;
const int TODAY_TITLE_Y = 0x1fe;
const int TODAY_COOKIE_Y = 0x20e;
const int TODAY_LINE_Y = 0x21c;

// How the colours of the new lines move: the time is run through a sine as many lines apart as this, and it moves the colours this far
const double LINE_PHASE = 0.23;
const double COLOR_SWING = 30.0;
const int PALE_HUE = 0x94;
const int PALE_LIGHTNESS = 0xe1;
const int DEEP_HUE = 0x37;
const int DEEP_LIGHTNESS = 0xb4;

// How much the time runs up in a step
const double TIME_STEP = 0.01;

// How wide the button of the table is, and how far it is from the bottom (it is centred)
const int BUTTON_WIDTH = 0xfa;
const int BUTTON_HALF_WIDTH = 0x7d;
const int BUTTON_BOTTOM_GAP = 5;

// The tall the table's box is meant to be, and the width LayoutDialog is given
const int PREFERRED_HEIGHT = 0x28a;
const int LAYOUT_WIDTH = 0x320;

// 0x454a40: leaving the table for the main menu
void WipeToMainMenu()
{
	gApp->HighScoresToMainMenu();
}

// The number of the day today is, as the table keeps it
int GetToday()
{
	time_t aNow = time(nullptr);
	struct tm* aTime = localtime(&aNow);
	return aTime->tm_year * DAYS_PER_YEAR + aTime->tm_yday;
}

// What goes with a name and its score in the file
int GetCheck(const std::string& theName, int theLength, int theScore)
{
	unsigned int aCheck = (unsigned int) theScore ^ CHECK_SEED;
	for (int i = 0; i < theLength && i < (int) theName.length(); i++)
		aCheck ^= (unsigned int) ((signed char) theName[i] * CHECK_LETTER_FACTOR);
	return (int) aCheck;
}

// 0x455480: what follows the number of a place
const char* GetOrdinalSuffix(int thePlace)
{
	if ((thePlace % 100) / 10 == 1)
		return "th";
	switch (thePlace % 10)
	{
	case 1: return "st";
	case 2: return "nd";
	case 3: return "rd";
	default: return "th";
	}
}

// The colours that run through the lines of the new place and of the best of the day
void GetMovingColors(double theTime, int theLine, Color& thePale, Color& theDeep)
{
	int aSwing = (int) (sin(theTime - (double) theLine * LINE_PHASE) * COLOR_SWING);
	thePale = Color((int) gApp->HSLToRGB(aSwing + PALE_HUE, 255, aSwing + PALE_LIGHTNESS));
	theDeep = Color((int) gApp->HSLToRGB(aSwing + DEEP_HUE, 255, aSwing + DEEP_LIGHTNESS));
}

// A piece of text with its right side at theRightX
void DrawRightAligned(Graphics* g, const std::string& theText, int theRightX, int theY)
{
	g->DrawString(theText, theRightX - g->GetFont()->StringWidth(theText), theY);
}

}

// 0x456080
HighScoreDisplayDialog::HighScoreDisplayDialog() :
	Dialog(IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_HIGH_SCORE_TABLE, true, "", "", "Main Menu", Dialog::BUTTONS_FOOTER)
{
	mSubtitle = "These are the top gnome bashers!";
	mTodayTable = -1;
	mNewTable = -1;
	mNewIndex = -1;
	mShown = false;
	mTime = 0.0;
	mTodaysCookie = false;

	// the places start with the same scores in both tables, and the two lists of names take turns (the two lists of ten fill the twenty places)
	bool aTakeFromFirstList = true;
	int aNextFirstName = 0;
	int aNextSecondName = 0;
	int aScore = FIRST_PLACE_SCORE;
	for (int aPlace = 0; aScore > FIRST_PLACE_SCORE - NUM_PLACES * PLACE_SCORE_STEP; aPlace++)
	{
		mCookies[aPlace] = false;
		for (int aTable = 0; aTable < NUM_TABLES; aTable++)
		{
			mTodaysScore[aTable] = 0;
			mScores[aTable][aPlace] = aScore;
			if (aTakeFromFirstList && aNextFirstName < NUM_PLACES)
			{
				mNames[aTable][aPlace] = DEFAULT_NAMES_A[aNextFirstName++];
				aTakeFromFirstList = false;
			}
			else
			{
				mNames[aTable][aPlace] = DEFAULT_NAMES_B[aNextSecondName++];
				aTakeFromFirstList = true;
			}
		}
		aScore -= PLACE_SCORE_STEP;
	}

	Load();
}

// 0x4563d0: the tables are kept
HighScoreDisplayDialog::~HighScoreDisplayDialog()
{
	Save();
}

// 0x454a50: the button of the table sits at the bottom, in the middle
void HighScoreDisplayDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aButtonHeight = mYesButton->mHeight;
	mYesButton->Resize(theWidth / 2 - BUTTON_HALF_WIDTH + theX, theY - aButtonHeight - BUTTON_BOTTOM_GAP + theHeight, BUTTON_WIDTH,
		aButtonHeight);
}

// 0x454c20
int HighScoreDisplayDialog::GetPreferredHeight(int theWidth)
{
	return PREFERRED_HEIGHT;
}

// 0x454b70
void HighScoreDisplayDialog::Update()
{
	WidgetContainer::Update();
	mTime += TIME_STEP;
	MarkDirty();
}

// 0x455550: the picture with both tables on it, a column of ten places each and below it the best of the day; the lines of a place that was
// just made and of a best that was just beaten move through the colours
void HighScoreDisplayDialog::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_HIGHSCOREBACKGROUND, 0, 0);

	g->SetColor(Color(0xeb, 0xff, 0xff));
	g->SetFont(FONT_EUROFURENCE14BOLD);
	g->DrawString(mSubtitle, mWidth / 2 - g->GetFont()->StringWidth(mSubtitle) / 2, SUBTITLE_Y);

	for (int aTable = 0; aTable < NUM_TABLES; aTable++)
	{
		int aTableX = aTable != 0 ? SECOND_TABLE_X : 0;

		g->SetColor(Color(255, 255, 255, 255));
		g->SetFont(FONT_PTBANANASPLIT27);
		std::string aTitle = TABLE_TITLES[aTable];
		g->DrawString(aTitle, (mWidth / 2 - g->GetFont()->StringWidth(aTitle)) / 2 + aTableX, TITLE_Y);

		int aY = FIRST_PLACE_Y;
		for (int aPlace = 0; aY < PLACES_END_Y; aPlace++, aY += PLACE_HEIGHT)
		{
			Color aPaleColor;
			Color aDeepColor;
			GetMovingColors(mTime, aPlace, aPaleColor, aDeepColor);
			bool aIsNew = mNewTable == aTable && mNewIndex == aPlace;
			Color aPlainColor(0xeb, 0xff, 0xff);

			g->SetColor(aIsNew ? aDeepColor : aPlainColor);
			g->SetFont(FONT_EUROFURENCE14BOLD);
			DrawRightAligned(g, StrFormat("%d.", aPlace + 1), aTableX + NUMBER_RIGHT_X, aY);

			g->SetColor(aIsNew ? aDeepColor : aPaleColor);
			g->SetFont(FONT_EUROFURENCE14BOLD);
			g->DrawString(mNames[aTable][aPlace], aTableX + NAME_X, aY);

			g->SetColor(aIsNew ? aDeepColor : aPlainColor);
			g->SetFont(FONT_EUROFURENCE14BOLD);
			DrawRightAligned(g, StrFormat("%d", mScores[aTable][aPlace]), aTableX + SCORE_RIGHT_X, aY);

			if (aTable == 0 && mCookies[aPlace])
			{
				g->SetColorizeImages(true);
				g->DrawImage(IMAGE_TOUGH_COOKIE_MINI, COOKIE_X, aY - COOKIE_RISE);
				g->SetColorizeImages(false);
			}
		}

		if (mTodaysScore[aTable] != 0)
		{
			Color aPaleColor;
			Color aDeepColor;
			GetMovingColors(mTime, mNewIndex, aPaleColor, aDeepColor);
			Color aPlainColor(0xeb, 0xff, 0xff);
			bool aIsToday = mTodayTable == aTable;

			g->SetColor(Color(255, 255, 255, 255));
			g->SetFont(FONT_PTBANANASPLIT18);
			std::string aTodayTitle = "Today's High Score";
			g->DrawString(aTodayTitle, (mWidth / 2 - g->GetFont()->StringWidth(aTodayTitle)) / 2 + aTableX, TODAY_TITLE_Y);

			g->SetColor(aIsToday ? aDeepColor : aPaleColor);
			g->SetFont(FONT_EUROFURENCE14BOLD);
			g->DrawString(mTodaysName[aTable], aTableX + NAME_X, TODAY_LINE_Y);

			g->SetColor(aIsToday ? aDeepColor : aPlainColor);
			g->SetFont(FONT_EUROFURENCE14BOLD);
			DrawRightAligned(g, StrFormat("%d", mTodaysScore[aTable]), aTableX + SCORE_RIGHT_X, TODAY_LINE_Y);

			if (aTable == 0 && mTodaysCookie)
			{
				g->SetColorizeImages(true);
				g->DrawImage(IMAGE_TOUGH_COOKIE_MINI, COOKIE_X, TODAY_COOKIE_Y);
				g->SetColorizeImages(false);
			}
		}
	}
}

// 0x454ba0
void HighScoreDisplayDialog::ButtonPress(int theId)
{
	gApp->PlaySample(SOUND_UI_BUTTON_CLICK);
}

// 0x454be0: the table goes, and the main menu comes back if the player was looking at it
void HighScoreDisplayDialog::ButtonDepress(int theId)
{
	if (theId == Dialog::ID_FOOTER)
	{
		gApp->KillDialog(DIALOG_HIGH_SCORE_TABLE);
		if (mShown)
			gApp->DoZoomWipe(WipeToMainMenu, WIPE_HAMMER);
	}
}

// 0x454ab0
int HighScoreDisplayDialog::GetPlace(int theTable, int theScore) const
{
	int aPlace = 0;
	for (int anIndex = NUM_PLACES - 1; anIndex >= 0; anIndex--)
	{
		if (mScores[theTable][anIndex] < theScore)
			aPlace = anIndex + 1;
	}
	return aPlace;
}

// 0x456490
void HighScoreDisplayDialog::Prepare()
{
	int aScore = GetStats()->GetInt(STAT_SCORE);
	int aTable = gApp->mGameMode == GAME_MODE_MARATHON ? 1 : 0;
	int aTodaysBest = mTodaysScore[aTable];
	int aPlace = GetPlace(aTable, aScore);
	if (aPlace != 0)
	{
		std::string aText;
		std::string aPlaceText = StrFormat("%d", aPlace) + GetOrdinalSuffix(aPlace);
		if (aTodaysBest < aScore)
			aText = "You got the highest score of the day and " + aPlaceText + " place of all time!";
		else
			aText = "You got " + aPlaceText + " place of all time!";

		Dialog* aDialog = gApp->DoDialog(DIALOG_HIGH_SCORES, true, "Well Done!", aText, "Ok", Dialog::BUTTONS_FOOTER);
		gApp->CenterDialog(aDialog);
	}
}

// 0x455f00
void HighScoreDisplayDialog::RecordScore(const std::string& theName)
{
	if (theName.empty())
		return;

	int aScore = GetStats()->GetInt(STAT_SCORE);
	int aTable = gApp->mGameMode == GAME_MODE_MARATHON ? 1 : 0;
	int aPlace = GetPlace(aTable, aScore);

	if (mTodaysScore[aTable] < aScore)
	{
		mTodaysScore[aTable] = aScore;
		mTodaysName[aTable] = theName;
		mTodayTable = aTable;
		if (aTable == 0)
			mTodaysCookie = gApp->mGameMode == GAME_MODE_TOUGH_COOKIE;
	}

	if (aPlace != 0)
	{
		// the places below it move down one
		if (aPlace < NUM_PLACES)
		{
			for (int anIndex = NUM_PLACES - 1; aPlace <= anIndex; anIndex--)
			{
				mNames[aTable][anIndex] = mNames[aTable][anIndex - 1];
				mScores[aTable][anIndex] = mScores[aTable][anIndex - 1];
				if (aTable == 0)
					mCookies[anIndex] = mCookies[anIndex - 1];
			}
		}

		mNames[aTable][aPlace - 1] = theName;
		mScores[aTable][aPlace - 1] = aScore;
		if (aTable == 0)
			mCookies[aPlace - 1] = gApp->mGameMode == GAME_MODE_TOUGH_COOKIE;
		mNewTable = aTable;
		mNewIndex = aPlace - 1;
	}
}

// 0x454c60: a best of the day is taken only when it is from today, and a place whose score does not match its check is cut down to
// what is left after dividing it by 256
void HighScoreDisplayDialog::Load()
{
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(TABLES_FILE_NAME, &aBuffer, false))
		return;

	mTodaysCookie = aBuffer.ReadByte() != 0;
	for (int aPlace = 0; aPlace < NUM_PLACES; aPlace++)
		mCookies[aPlace] = aBuffer.ReadByte() != 0;

	for (int aTable = 0; aTable < NUM_TABLES; aTable++)
	{
		int aDay = (int) aBuffer.ReadLong();
		int aLength = (int) aBuffer.ReadLong();
		std::string aName = aBuffer.ReadString();
		int aScore = (int) aBuffer.ReadLong();
		int aCheck = (int) aBuffer.ReadLong();
		if (aDay == GetToday() && aCheck == GetCheck(aName, aLength, aScore))
		{
			mTodaysName[aTable] = aName;
			mTodaysScore[aTable] = aScore;
		}

		for (int aPlace = 0; aPlace < NUM_PLACES; aPlace++)
		{
			aLength = (int) aBuffer.ReadLong();
			aName = aBuffer.ReadString();
			aScore = (int) aBuffer.ReadLong();
			aCheck = (int) aBuffer.ReadLong();
			mNames[aTable][aPlace] = aName;
			if (aCheck == GetCheck(aName, aLength, aScore))
				mScores[aTable][aPlace] = aScore;
			else
				mScores[aTable][aPlace] = aScore % 256;
		}
	}
}

// 0x455080
void HighScoreDisplayDialog::Save()
{
	gApp->EraseFile(TABLES_FILE_NAME);

	Buffer aBuffer;
	aBuffer.WriteByte(mTodaysCookie);
	for (int aPlace = 0; aPlace < NUM_PLACES; aPlace++)
		aBuffer.WriteByte(mCookies[aPlace]);

	for (int aTable = 0; aTable < NUM_TABLES; aTable++)
	{
		int aLength = (int) mTodaysName[aTable].length();
		aBuffer.WriteLong(GetToday());
		aBuffer.WriteLong(aLength);
		aBuffer.WriteString(mTodaysName[aTable]);
		aBuffer.WriteLong(mTodaysScore[aTable]);
		aBuffer.WriteLong(GetCheck(mTodaysName[aTable], aLength, mTodaysScore[aTable]));

		for (int aPlace = 0; aPlace < NUM_PLACES; aPlace++)
		{
			aLength = (int) mNames[aTable][aPlace].length();
			aBuffer.WriteLong(aLength);
			aBuffer.WriteString(mNames[aTable][aPlace]);
			aBuffer.WriteLong(mScores[aTable][aPlace]);
			aBuffer.WriteLong(GetCheck(mNames[aTable][aPlace], aLength, mScores[aTable][aPlace]));
		}
	}

	gApp->WriteBufferToFile(TABLES_FILE_NAME, &aBuffer);
}

// 0x456810
HighScoreDisplayDialog* Sexy::MakeHighScoreDisplay()
{
	HighScoreDisplayDialog* aDialog = new HighScoreDisplayDialog();
	LayoutDialog(aDialog, LAYOUT_WIDTH);
	aDialog->Resize(0, 0, gApp->mWidth, gApp->mHeight);
	gApp->AddDialog(DIALOG_HIGH_SCORE_TABLE, aDialog);
	return aDialog;
}
