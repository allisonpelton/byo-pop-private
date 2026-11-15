#include "LevelDone.h"

#include "Edition.h"
#include "GameDialogs.h"
#include "Font.h"
#include "JBBase.h"
#include "Manager.h"
#include "PlayLayer.h"
#include "Res.h"
#include "User.h"

namespace mahjong
{

// 0x407cd0 (mahjongjapan)
LevelDone::LevelDone(int theTime, int theBestTime, bool theNewTime, int theScore, int theBestScore,
	bool theNewScore, const char* theNextTitle) : Layer(100, 0x46), mNextTitle(theNextTitle)
{
	mTime = jb::FormatTime(theTime);
	mScore = jb::FormatNumber(theScore);
	mBestScore = jb::FormatNumber(theBestScore);
	mBestTime = jb::FormatTime(theBestTime);
	FormatRecords(mMessage, theNewTime, theNewScore);
}

// 0x407ee0 (mahjongjapan)
void LevelDone::Load()
{
	Add(new LevelDoneScroll(mNextTitle.c_str(), mTime.c_str(), mScore.c_str(), mBestTime.c_str(),
		mBestScore.c_str(), mMessage.c_str()), 0, 0);
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0x100, 0x13a);
	aButton->SetText(STRING_LEVEL_DONE_OK);
	jb::LoadSound(SOUND_FANFARE, false);
	jb::PlaySound(SOUND_FANFARE);
}

// 0x407ff0 (mahjongjapan)
bool LevelDone::OnCommand(int theCmd)
{
	if (theCmd - 1 != 0)
		return false;
	PlayLayer* aPlay = (PlayLayer*)jb::gManager->Pop(true, 1);
	aPlay->ShowMap();
	return true;
}

// 0x408020 (mahjongjapan)
LevelDoneScroll::LevelDoneScroll(const char* theNextTitle, const char* theTime, const char* theScore,
	const char* theBestTime, const char* theBestScore, const char* theMessage)
	: mTime(theTime), mScore(theScore), mNextTitle(theNextTitle), mBestTime(theBestTime), mBestScore(theBestScore),
	  mMessage(theMessage)
{
}

// 0x408270 (mahjongjapan)
void LevelDoneScroll::Draw()
{
	DrawImage(0, 0, IMAGE_LEVEL_DONE_SCROLL, nullptr, true, 100, 100, 0xff);
	mHeadFont->DrawCentred(this, 0x80, 0x82, mHeading.c_str(), 0x14f, 100, 100, 0xff);
	mValueFont->DrawLeft(this, 0x94, 0x9d, mLabelTime.c_str(), -1, 100, 100, 0xff);
	mValueFont->DrawLeft(this, 0xcc, 0x9d, mTime.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawLeft(this, 0xa1, 0xad, mLabelBest.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawLeft(this, 0xcd, 0xad, mBestTime.c_str(), -1, 100, 100, 0xff);
	mValueFont->DrawLeft(this, 0x134, 0x9d, mLabelScore.c_str(), -1, 100, 100, 0xff);
	mValueFont->DrawLeft(this, 0x177, 0x9d, mScore.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawLeft(this, 0x14d, 0xad, mLabelBest.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawLeft(this, 0x178, 0xad, mBestScore.c_str(), -1, 100, 100, 0xff);
	mSmallFont->DrawCentred(this, 0x80, 0xbf, mMessage.c_str(), 0x14f, 100, 100, 0xff);
	mTextFont->DrawCentred(this, 0x80, 0xe1, mNextLabel.c_str(), 0x14f, 100, 100, mAlpha);
	mTextFont->DrawWrappedCentred(this, 0x8f, mLines < 3 ? 0x10c : 0x100, 0x136, 1000, mNextTitle.c_str(), 0x15,
		false, mAlpha);
}

// 0x4084e0 (mahjongjapan)
void LevelDoneScroll::Init()
{
	mHeading = jb::LoadDllString(STRING_LEVEL_COMPLETED);
	mLabelTime = jb::LoadDllString(STRING_TIME_LABEL);
	mLabelScore = jb::LoadDllString(STRING_SCORE_LABEL);
	mNextLabel = jb::LoadDllString(STRING_WISDOM_UNLOCKED);
	mLabelBest = jb::LoadDllString(STRING_BEST_PREFIX);
	mTextFont = jb::GetDllFont(FONT_RESULT, FONT_RESULT_MAP);
	mHeadFont = jb::GetDllFont(FONT_HEADING, FONT_HEADING_MAP);
	mValueFont = jb::GetDllFont(FONT_TEXT, FONT_TEXT_MAP);
	mClockFont = jb::GetDllFont(FONT_CLOCK, FONT_CLOCK_MAP);
	mSmallFont = jb::GetDllFont(FONT_SMALL, FONT_SMALL_MAP);
	mLines = mTextFont->DrawWrappedCentred(this, 0x8f, 0, 0x136, 1000, mNextTitle.c_str(), 0x14, true, 0xff);
	RegUpdate(true);
}

// 0x4085f0 (mahjongjapan)
bool LevelDoneScroll::Update()
{
	if (0 < mDelay)
	{
		mDelay = mDelay - GetDelta();
		return false;
	}
	if (mAlpha < 0xff)
	{
		mAlpha = mAlpha + (GetDelta() * 0xff) / 0x5dc;
		if (0xff < mAlpha)
		{
			mAlpha = 0xff;
			RegUpdate(false);
		}
	}
	return true;
}

} // namespace mahjong
