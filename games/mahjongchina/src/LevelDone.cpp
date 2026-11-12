#include "LevelDone.h"

#include "Edition.h"
#include "Font.h"
#include "JBBase.h"
#include "Manager.h"
#include "PlayLayer.h"
#include "Res.h"
#include "User.h"

namespace mahjong
{

// 0x407f10 (mahjongchina) (China shows no records: the best time, the best score and whether they are new are not used)
LevelDone::LevelDone(int theTime, int, bool, int theScore, int, bool, const char* theNextTitle) : Dialog(0x7d, 0x96),
	mNextTitle(theNextTitle)
{
	mTime = jb::FormatTime(theTime);
	mScore = jb::FormatNumber(theScore);
}

// 0x408040 (mahjongchina)
void LevelDone::Create()
{
	int aLeft[] = {3, 3, 0};
	int aRight[] = {3, 3, 0};
	int aTop[] = {2, 2, 1, 3, 0};
	int aBottom[] = {3, 2, 2, 1, 0};
	SetCorners(2, 2, 1, 1);
	SetEdges(aLeft, aRight, aTop, aBottom);
	std::string aLabel;
	DialogButton* aButton = new DialogButton(1);
	Add(aButton, 0xea, 0xe0);
	aButton->SetText(STRING_LEVEL_DONE_OK);
	SetTitle(STRING_LEVEL_COMPLETED);
	aLabel = jb::LoadDllString(STRING_TIME_LABEL);
	AddText(aLabel.c_str(), 1, 0x82, 0x55, false);
	AddText(mTime.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x88, 0x55, true);
	aLabel = jb::LoadDllString(STRING_SCORE_LABEL);
	AddText(aLabel.c_str(), 1, 0x122, 0x55, false);
	AddText(mScore.c_str(), 1, GetTextWidth(aLabel.c_str()) + 0x128, 0x55, true);
	Add(new LevelDoneScroll(mNextTitle.c_str()), 0x78, 0x67);
	jb::LoadSound(SOUND_FANFARE, false);
	jb::PlaySound(SOUND_FANFARE);
}

// 0x408240 (mahjongchina)
bool LevelDone::OnCommand(int theCmd)
{
	if (theCmd - 1 != 0)
		return false;
	PlayLayer* aPlay = (PlayLayer*)jb::gManager->Pop(true, 1);
	aPlay->ShowMap();
	return true;
}

// 0x408380 (mahjongchina)
void LevelDoneScroll::Draw()
{
	DrawImage(0, 0, IMAGE_LEVEL_DONE_BOX, nullptr, true, 100, 100, mAlpha);
	mTextFont->DrawCentred(this, -3, 0x16, mNextLabel.c_str(), 0x136, 100, 100, mAlpha);
	mTextFont->DrawWrappedCentred(this, 0x14, mLines < 3 ? 0x36 : 0x2c, 0x10e, 1000, mNextTitle.c_str(), 0x15,
		false, mAlpha);
}

// 0x408410 (mahjongchina)
void LevelDoneScroll::Init()
{
	mNextLabel = jb::LoadDllString(STRING_WISDOM_UNLOCKED);
	mTextFont = jb::GetDllFont(FONT_RESULT, FONT_RESULT_MAP);
	mLines = mTextFont->DrawWrappedCentred(this, 0x14, 0x38, 0x10e, 1000, mNextTitle.c_str(), 0x14, true, 0xff);
	RegUpdate(true);
}

// 0x408470 (mahjongchina)
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
