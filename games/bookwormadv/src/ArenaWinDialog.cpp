#include "ArenaWinDialog.h"

#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// 0x401180: dialog 0x15 with "Continue" as its one button (the footer's), 0x1c4 wide and 0x154 high (0x168 for
// the first win's), in the middle of the screen
ArenaWinDialog::ArenaWinDialog(bool theFirstTime, const std::string& theCallback)
	: GameDialog(GameApp::DIALOG_ARENA_WIN, _S(""), _S(""), GT(0x7a95e8), BUTTONS_FOOTER), mFirstTime(theFirstTime),
	  mCallback(theCallback)
{
	int aHeight = 0x154 + (theFirstTime ? 0x14 : 0);
	GameDialog::Resize((mApp->mWidth - 0x1c4) / 2, (mApp->mHeight - aHeight) / 2, 0x1c4, aHeight);
}

void ArenaWinDialog::SetStats(const std::string& theTime, const std::string& thePotions, const std::string& theWords,
                              const std::string& theLevel, const std::string& theAverageWordLength)
{
	// 0x4431a0
	mTime = theTime;
	mPotions = thePotions;
	mWords = theWords;
	mLevel = theLevel;
	mAverageWordLength = theAverageWordLength;
}

void ArenaWinDialog::Draw(Graphics* g)
{
	// 0x4014c0: the banner over the top, the heading in orange under it, and then the film reel and the unlock's
	// text (the first win), or the list of how the run went
	GameDialog::Draw(g);
	g->SetFont(FONT_LARGE);
	g->SetColor(Color(0xf1, 0x50, 0x22));
	g->DrawImage(IMAGE_ARENA_WIN_BANNER, (mWidth - IMAGE_ARENA_WIN_BANNER->mWidth) / 2, 5);
	Rect aRect(mBackgroundInsets.mLeft + mContentInsets.mLeft + 2, mBackgroundInsets.mTop + mContentInsets.mTop + 0x2b,
	           mWidth - mBackgroundInsets.mRight - mContentInsets.mRight - mBackgroundInsets.mLeft -
	               mContentInsets.mLeft - 4,
	           0);
	WriteWordWrapped(g, aRect, GT(0x7a95f4), -1, 0);
	if (mFirstTime) {
		g->DrawImage(IMAGE_FILM_REEL, (mWidth - IMAGE_FILM_REEL->mWidth) / 2, 100);
		aRect.mY = 0xdc;
		WriteWordWrapped(g, aRect, GT(0x7a9610), -1, 0);
	} else {
		// each line is its label (the game's, in the brown of the markup) and the value in black
		g->WriteString(GT(0x7a96bc) + mTime, aRect.mX, aRect.mY + 0x32, aRect.mWidth, 0, true, 0, -1, -1);
		g->WriteString(GT(0x7a96dc) + mPotions, aRect.mX, aRect.mY + 0x4b, aRect.mWidth, 0, true, 0, -1, -1);
		g->WriteString(GT(0x7a96fc) + mWords, aRect.mX, aRect.mY + 0x64, aRect.mWidth, 0, true, 0, -1, -1);
		g->WriteString(GT(0x7a971c) + mLevel, aRect.mX, aRect.mY + 0x7d, aRect.mWidth, 0, true, 0, -1, -1);
		g->WriteString(GT(0x7a973c) + mAverageWordLength, aRect.mX, aRect.mY + 0x96, aRect.mWidth, 0, true, 0, -1, -1);
	}
}
