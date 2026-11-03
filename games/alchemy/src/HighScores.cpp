// The score table (see HighScores.h).

#include "HighScores.h"
#include "AlchemyApp.h"
#include "Board.h"
#include "Buffer.h"
#include "DialogButton.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "WidgetManager.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace Sexy;

// the file: for each kind of game, the best score of the day (the day, a name, a score and a check), then five scores
// (a name, a score and a check); every number is a little-endian 32 bit one, a name's length 16 bit. The original keeps
// it as hiscores.dat next to the program; the port's is in the user's data.
static const char* kScoreFileName = "userdata\\hiscores.dat";

// 0x409e60: the day, for the best score of the day
static int DayKey()
{
	time_t aNow = time(NULL);
	struct tm* aTime = localtime(&aNow);
	return aTime->tm_year * 0x16e + aTime->tm_yday;
}

// the check of a score and its name (0x40a15a, 0x40a341)
static unsigned int ScoreCheck(int theScore, const std::string& theName)
{
	unsigned int aCheck = (unsigned int) theScore ^ 0x42beef;
	for (size_t i = 0; i < theName.length(); i++)
		aCheck ^= (unsigned int) ((int) (signed char) theName[i] * 0x11);
	return aCheck;
}

static void PutInt(std::string& theData, int theValue)
{
	for (int i = 0; i < 4; i++)
		theData += (char) ((theValue >> (i * 8)) & 0xff);
}

static void PutName(std::string& theData, const std::string& theName)
{
	unsigned int aLength = (unsigned int) theName.length() & 0xffff;
	theData += (char) (aLength & 0xff);
	theData += (char) (aLength >> 8);
	theData.append(theName, 0, aLength);
}

static void PutScore(std::string& theData, const std::string& theName, int theScore)
{
	PutName(theData, theName);
	PutInt(theData, theScore);
	PutInt(theData, (int) ScoreCheck(theScore, theName));
}

// the reader of the file: stops at its end
struct ScoreFileReader
{
	const unsigned char*	mData;
	size_t					mSize;
	size_t					mPos;
	bool					mFailed;

	ScoreFileReader(const unsigned char* theData, size_t theSize) : mData(theData), mSize(theSize), mPos(0), mFailed(false) {}

	int GetInt()
	{
		if (mPos + 4 > mSize)
		{
			mFailed = true;
			return 0;
		}
		unsigned int aValue = mData[mPos] | (mData[mPos + 1] << 8) | (mData[mPos + 2] << 16) | ((unsigned int) mData[mPos + 3] << 24);
		mPos += 4;
		return (int) aValue;
	}

	std::string GetName()
	{
		if (mPos + 2 > mSize)
		{
			mFailed = true;
			return std::string();
		}
		int aLength = (short) (mData[mPos] | (mData[mPos + 1] << 8));
		mPos += 2;
		if (aLength < 0 || aLength > 255 || mPos + aLength > mSize)
		{
			mFailed = true;
			return std::string();
		}
		std::string aName((const char*) mData + mPos, aLength);
		mPos += aLength;
		return aName;
	}
};

// 0x40bad1
HighScoreWidget::HighScoreWidget(AlchemyApp* theApp)
{
	mApp = theApp;
	mReveal = 0;
	mBoardImage = NULL;
	mTableImage = NULL;
	mAnim = 0;

	mStartButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 1, this);
	mStartButton->mLabel = mApp->GetString("START_GAME_BUTTON");
	mStartButton->SetFont(mApp->mFont1);
	mStartButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xe0, 0xb7, 0x28));

	// The names a new table starts with. The original's are ten names inside the game, which the port reads from the
	// player's own Alchemy.exe (GameText.h); without it the port's placeholders stand in. Properties can replace
	// either: DEFAULT_NAME_STRATEGIC_1..5 and DEFAULT_NAME_TIME_TRIAL_1..5 in properties/default.xml. The scores are
	// the original's, 1000 down to 600.
	static const char* const aNames[NUM_MODES][NUM_ENTRIES] =
	{
		{"Adept", "Brewer", "Charmer", "Distiller", "Enchanter"},
		{"Apothecary", "Conjurer", "Herbalist", "Magus", "Oracle"}
	};
	// where the original keeps them in its image (the strategic table's five, then the time trial table's)
	static const unsigned int aNameVAs[NUM_MODES][NUM_ENTRIES] =
	{
		{0x4e4148, 0x4e4140, 0x4e4138, 0x4e4130, 0x4e412c},
		{0x4e4124, 0x4e411c, 0x4e4114, 0x4e410c, 0x4e4108}
	};
	static const char* const aModeIds[NUM_MODES] = {"DEFAULT_NAME_STRATEGIC_", "DEFAULT_NAME_TIME_TRIAL_"};
	for (int aMode = 0; aMode < NUM_MODES; aMode++)
	{
		mTodayScore[aMode] = 0;
		for (int i = 0; i < NUM_ENTRIES; i++)
		{
			char anId[64];
			sprintf(anId, "%s%d", aModeIds[aMode], i + 1);
			const std::string& aGameName = GameText(aNameVAs[aMode][i]);
			mName[aMode][i] = mApp->GetString(anId, aGameName.empty() ? aNames[aMode][i] : aGameName);
			mScore[aMode][i] = 1000 - i * 100;
		}
	}

	Load();
}

// 0x40be02
HighScoreWidget::~HighScoreWidget()
{
	delete mBoardImage;
	delete mTableImage;
	delete mStartButton;
	Save();
}

// 0x40a15a: a score whose check does not fit is cut to its last eight bits
void HighScoreWidget::Load()
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(kScoreFileName, &aBuffer))
		return;

	ScoreFileReader aReader(aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	for (int aMode = 0; aMode < NUM_MODES; aMode++)
	{
		int aDay = aReader.GetInt();
		std::string aTodayName = aReader.GetName();
		int aTodayScore = aReader.GetInt();
		unsigned int aTodayCheck = (unsigned int) aReader.GetInt();
		if (aReader.mFailed)
			return;
		if (aDay == DayKey() && aTodayCheck == ScoreCheck(aTodayScore, aTodayName))
		{
			mTodayName[aMode] = aTodayName;
			mTodayScore[aMode] = aTodayScore;
		}

		for (int i = 0; i < NUM_ENTRIES; i++)
		{
			std::string aName = aReader.GetName();
			int aScore = aReader.GetInt();
			unsigned int aCheck = (unsigned int) aReader.GetInt();
			if (aReader.mFailed)
				return;
			mName[aMode][i] = aName;
			mScore[aMode][i] = (aCheck == ScoreCheck(aScore, aName)) ? aScore : aScore % 0x100;
		}
	}
}

// 0x40a341
void HighScoreWidget::Save()
{
	std::string aData;
	for (int aMode = 0; aMode < NUM_MODES; aMode++)
	{
		PutInt(aData, DayKey());
		PutScore(aData, mTodayName[aMode], mTodayScore[aMode]);
		for (int i = 0; i < NUM_ENTRIES; i++)
			PutScore(aData, mName[aMode][i], mScore[aMode][i]);
	}

	if (!mApp->WriteBytesToFile(kScoreFileName, aData.data(), (unsigned long) aData.length()))
		mApp->Popup(mApp->GetString("HIGH_SCORE_FILE_WRITE_ERROR"));
}

// 0x409e88: the place of a score in the table (the first one it is above, 1 to 5), or 0
int HighScoreWidget::RankOf(int theMode, int theScore)
{
	int aRank = 0;
	for (int i = NUM_ENTRIES - 1; i >= 0; i--)
		if (mScore[theMode][i] < theScore)
			aRank = i + 1;
	return aRank;
}

// 0x40a57b: when the table is all shown, a score of the game that is in the table, or the best of the day, asks for a name
void HighScoreWidget::CheckScore()
{
	Board* aBoard = mApp->mBoard;
	int aMode = aBoard->mTimeTrial ? 1 : 0;
	int aScore = aBoard->mScore;
	bool isBestOfDay = mTodayScore[aMode] < aScore;
	int aRank = RankOf(aMode, aScore);
	char aRankText[32];
	sprintf(aRankText, "%d", aRank);

	SexyString aMessage;
	if (!isBestOfDay)
	{
		if (aRank < 1)
			return;
		aMessage = mApp->GetString("YOU_GOT_X_PLACEMENT_OF_ALL_TIME1") + " " + aRankText + " " +
			mApp->GetString("YOU_GOT_X_PLACEMENT_OF_ALL_TIME2");
	}
	else if (aRank > 0)
		aMessage = mApp->GetString("HIGH_SCORE_OF_ALL_TIME_AND_DAY_WITH_PLACEMENT1") + " " + aRankText + " " +
			mApp->GetString("HIGH_SCORE_OF_ALL_TIME_AND_DAY_WITH_PLACEMENT2");
	else
		aMessage = mApp->GetString("HIGH_SCORE_OF_THE_DAY1");

	mApp->ShowNameEntry(aMessage);
}

// 0x40b9e8: the name that was asked for goes with the score of the game that ended
void HighScoreWidget::SetName(const SexyString& theName)
{
	if (theName.empty())
		return;

	Board* aBoard = mApp->mBoard;
	int aMode = aBoard->mTimeTrial ? 1 : 0;
	int aScore = aBoard->mScore;
	int aRank = RankOf(aMode, aScore);

	if (aScore > mTodayScore[aMode])
	{
		mTodayScore[aMode] = aScore;
		mTodayName[aMode] = theName;
	}

	if (aRank != 0)
	{
		for (int i = NUM_ENTRIES - 1; i >= aRank; i--)
		{
			mName[aMode][i] = mName[aMode][i - 1];
			mScore[aMode][i] = mScore[aMode][i - 1];
		}
		mName[aMode][aRank - 1] = theName;
		mScore[aMode][aRank - 1] = aScore;
	}

	// (the original writes the file when the widget is deleted; a web page may be closed without that)
	Save();
}

// 0x409eb3
void HighScoreWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mStartButton);
}

// 0x409eda
void HighScoreWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mStartButton);
}

// a line of the statistics: the label on the left and the value on the right (0x409f01; the baseline is the line's y)
static void DrawLine(Graphics* g, const Rect& theRect, int theDX, int theDY, const SexyString& theLabel, const SexyString& theValue)
{
	g->DrawString(theLabel, theRect.mX + 0x10 + theDX, theRect.mY + theDY);
	int aWidth = g->GetFont()->StringWidth(theValue);
	g->DrawString(theValue, (theRect.mWidth - aWidth) + theRect.mX - 0x10 + theDX, theRect.mY + theDY);
}

// 0x40b0b3: the board's picture is taken when the widget gets its size; the table is drawn on top of it once: the
// statistics of the game (the same lines drawn at 25 places around the real one make their outline) and the heading of
// the scores
void HighScoreWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// the table's lower part is 12 pixels lower when the game has no internet scores (the property NoInternetScores); the port has none
	const int kLower = 0xc;

	Widget::Resize(theX, theY, theWidth, theHeight);

	if (mBoardImage == NULL)
	{
		mBoardImage = new MemoryImage(mApp);
		mBoardImage->Create(mWidth, mHeight);
		mBoardImage->SetImageMode(false, false);
		Graphics g(mBoardImage);
		mApp->mBoard->Draw(&g);
	}

	if (mTableImage == NULL)
	{
		mTableImage = new MemoryImage(mApp);
		mTableImage->Create(mWidth, mHeight);
		mTableImage->SetImageMode(false, false);
		Graphics g(mTableImage);
		g.DrawImage(mBoardImage, 0, 0);
		Graphics aClipped(g);
		aClipped.ClipRect(0xd, 0x29, 0x1d0, 0x19c);
		aClipped.DrawImageBox(Rect(0, 0x1e, 0x1ea, 0x1b2), mApp->mImages[IMAGE_DIALOG]);

		Board* aBoard = mApp->mBoard;
		static const int anOffsets[5] = {-2, -1, 1, 2, 0};
		Rect aRect(0x91, 0x44, 0x125, 0x56);
		for (int r = 0; r < 5; r++)
		{
			int anOffsetY = anOffsets[r];
			for (int c = 0; c < 5; c++)
			{
				int aDX = anOffsets[c];
				bool isLast = (r == 4 && c == 4);
				char aBuffer[64];

				g.SetFont(mApp->mFont1);
				g.SetColor(isLast ? Color(0xde, 0x8a, 8) : Color(0, 0, 0));
				sprintf(aBuffer, "%2d:%02d", aBoard->mTotalSeconds / 60, aBoard->mTotalSeconds % 60);
				DrawLine(&g, aRect, aDX, anOffsetY + 0xf, mApp->GetString("TOTAL_GAME_TIME"), aBuffer);
				sprintf(aBuffer, "%d", aBoard->mBoardsCleared);
				DrawLine(&g, aRect, aDX, anOffsetY + 0x26, mApp->GetString("HSWIDGET_BOARDS_CLEARED"), aBuffer);
				sprintf(aBuffer, "%d", aBoard->mLongestRun);
				DrawLine(&g, aRect, aDX, anOffsetY + 0x3d, mApp->GetString("HSWIDGET_LONGEST_RUN_WITHOUT_DISCARD"), aBuffer);
				sprintf(aBuffer, "%d", aBoard->mScore);
				DrawLine(&g, aRect, aDX, anOffsetY + 0x54, mApp->GetString("FINAL_SCORE"), aBuffer);

				// the rank: its label, and its name after it, ending at the right edge of the statistics
				g.SetFont(mApp->mAFont1);
				SexyString aLabel = mApp->GetString("RANK") + " ";
				int aX = 0x1b6 - g.GetFont()->StringWidth(aLabel) - g.GetFont()->StringWidth(aBoard->mRankName);
				if (aX > 0x91)
					aX = 0x91;
				if (isLast)
					g.SetColor(Color(0xff, 0, 0));
				g.DrawString(aLabel, aDX + aX, anOffsetY + 0xbe);
				if (isLast)
					g.SetColor(Color(0x94, 0xeb, 0x10));
				g.DrawString(aBoard->mRankName, g.GetFont()->StringWidth(aLabel) + aDX + aX, anOffsetY + 0xbe);
			}
		}

		// the box of the five scores, and the heading of the table
		g.Translate(0, kLower);
		g.DrawImageBox(Rect(0x2d, 0xcb, 0x18c, 0x9a), mApp->mImages[IMAGE_DBUTTON]);
		g.SetColor(Color(0, 0, 0));
		g.SetFont(mApp->mAFont1);
		WriteCenteredLine(&g, 0xe6, mApp->GetString(aBoard->mTimeTrial ? "TIME_TRIAL_HIGH_SCORES" : "STRATEGIC_GAME_HIGH_SCORES"));
		g.FillRect(0x38, 0xeb, 0x174, 2);
		g.Translate(0, -kLower);
	}

	// the button comes up under the table (the internet score button, which is above it in the original, is not here)
	mStartButton->Resize(mX + 0x2e, 0x180, 0x18c, 0x20);
}

// 0x40a957
void HighScoreWidget::Draw(Graphics* g)
{
	const int kLower = 0xc;         // as in Resize

	if (mReveal < mHeight)
		g->DrawImage(mBoardImage, 0, 0);

	Graphics aTable(*g);
	aTable.ClipRect(0, 0, mWidth, mReveal);
	aTable.DrawImage(mTableImage, 0, 0);
	aTable.SetFont(mApp->mFont1);

	int aMode = mApp->mBoard->mTimeTrial ? 1 : 0;

	// the five scores, in colours that go around
	aTable.Translate(0, kLower);
	int aY = 0x101;
	for (int i = 0; i < NUM_ENTRIES; i++)
	{
		int aHue = abs(((mAnim + 2 + i) * 10) % 0xaa - 0x55);
		Color aColor1((int) mApp->HSLToRGB(aHue, 0xff, 0x96));
		Color aColor2((int) mApp->HSLToRGB(aHue, 0xff, 0x60));

		char aBuffer[32];
		aTable.SetColor(aColor1);
		sprintf(aBuffer, "%d.", i + 1);
		aTable.DrawString(aBuffer, 0x41, aY);
		aTable.SetColor(aColor2);
		aTable.DrawString(mName[aMode][i], 0x53, aY);
		aTable.SetColor(aColor1);
		sprintf(aBuffer, "%d", mScore[aMode][i]);
		aTable.DrawString(aBuffer, 0x104 - aTable.GetFont()->StringWidth(aBuffer), aY);
		aY += 0x14;
	}
	aTable.Translate(0, -kLower);

	// the best score of the day
	aTable.SetFont(mApp->mFont1);
	aTable.SetColor(Color(0, 0, 0));
	SexyString aText = mApp->GetString("TODAYS_HIGH_SCORE");
	aTable.DrawString(aText, 0x15e - aTable.GetFont()->StringWidth(aText) / 2, 0x10a);
	if (mTodayScore[aMode] == 0)
		aText = mApp->GetString("NOT_APPLICABLE_SHORTHAND");
	else
	{
		char aBuffer[32];
		sprintf(aBuffer, "%d", mTodayScore[aMode]);
		aText = aBuffer;
	}
	aTable.DrawString(aText, 0x15e - aTable.GetFont()->StringWidth(aText) / 2, 300);
	aText = mTodayName[aMode];
	aTable.DrawString(aText, 0x15e - aTable.GetFont()->StringWidth(aText) / 2, 0x14e);

	// the lower edge of the dialog's picture that goes down with the uncovering
	Image* aDialog = mApp->mImages[IMAGE_DIALOG];
	int aCellW = aDialog->GetWidth() / 3;
	int aCellH = aDialog->GetHeight() / 3;
	int aBottom = mReveal - 0x13;
	if (aBottom < 0x1b2)
	{
		Graphics aEdge(*g);
		aEdge.ClipRect(0xd, 0x29, 0x1d0, 0x19c);
		int aEdgeY = (aBottom - aCellH) + 0x1e;
		aEdge.DrawImage(aDialog, 0, aEdgeY, Rect(0, aCellH * 2, aCellW, aCellH));
		int aRightX = 0x1ea - aCellW;
		aEdge.DrawImage(aDialog, aRightX, aEdgeY, Rect(aCellW * 2, aCellH * 2, aCellW, aCellH));
		aEdge.ClipRect(aCellW, 0x1e, 0x1ea - aCellW * 2, aBottom);
		int aTiles = (aRightX - 1) / aCellW;
		int aX = aCellW;
		for (int i = 0; i < aTiles; i++)
		{
			aEdge.DrawImage(aDialog, aX, aEdgeY, Rect(aCellW, aCellH * 2, aCellW, aCellH));
			aX += aCellW;
		}
	}
}

// 0x40afa9: the table is uncovered from the top (6 pixels an update); the button grows with it, and the rows' colours
// move every ten updates
void HighScoreWidget::Update()
{
	Widget::Update();

	if (mReveal < mHeight)
	{
		mReveal += 6;
		if (mReveal >= mHeight)
		{
			mReveal = mHeight;
			CheckScore();
		}

		int aEdge = mY - 0x1c + mReveal;
		int aHeight = aEdge - mStartButton->mY;
		if (aHeight > 0x20)
			aHeight = 0x20;
		if (aHeight < 1)
			aHeight = 0;
		mStartButton->Resize(mStartButton->mX, mStartButton->mY, mStartButton->mWidth, aHeight);
		MarkDirty();
	}

	if (mUpdateCnt % 10 == 0)
	{
		mAnim++;
		MarkDirty();
	}
}

// 0x409f5b
void HighScoreWidget::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}

// 0x409f6b: the button leads to the selector (the internet score button, id 0, is not in the port)
void HighScoreWidget::ButtonDepress(int theId)
{
	if (theId == 1)
		mApp->ShowSelector();
}
