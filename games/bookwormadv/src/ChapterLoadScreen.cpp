#include "ChapterLoadScreen.h"

#include <algorithm>

#include "Color.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

ChapterLoadScreen::ChapterLoadScreen(GameApp* theApp) : mApp(theApp)
{
	Resize(225, 200, 350, 200);
	mHasAlpha = true;
	mHasTransparencies = true;
	// the hints
	mMessages = {
		GT(0x007ab820), GT(0x007ab834), GT(0x007ab84c), GT(0x007ab85c), GT(0x007ab878), GT(0x007ab88c),
		GT(0x007ab8a0), GT(0x007ab8b8), GT(0x007ab8d0), GT(0x007ab8f4), GT(0x007ab918), GT(0x007ab930),
		GT(0x007ab94c), GT(0x007ab970), GT(0x007ab998), GT(0x007ab9b4), GT(0x007ab9d0), GT(0x007ab9e4),
		GT(0x007ab9f8), GT(0x007aba0c), GT(0x007aba1c), GT(0x007aba38), GT(0x007aba50), GT(0x007aba70),
		GT(0x007aba84), GT(0x007abaa8), GT(0x007abacc),
	};
	mCurrent = (unsigned int)Rand() % mMessages.size();
	mShown.push_back(mCurrent);
}

void ChapterLoadScreen::Update()
{
	// (redrawn when the app has loaded a resource, which marks the box dirty)
	if (--mTimer >= 1)
		return;
	mTimer = 50;
	// A hint not shown yet, while there are any; then any hint
	std::vector<int> aCandidates;
	for (int i = 0; i < (int)mMessages.size(); i++)
		if (std::find(mShown.begin(), mShown.end(), i) == mShown.end())
			aCandidates.push_back(i);
	if (!aCandidates.empty()) {
		mCurrent = aCandidates[(unsigned int)Rand() % aCandidates.size()];
		mShown.push_back(mCurrent);
	} else {
		mCurrent = (unsigned int)Rand() % mMessages.size();
	}
}

void ChapterLoadScreen::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), IMAGE_DIALOG);
	g->SetFont(FONT_LARGE_OUTLINE);
	g->SetColor(Color::White);
	g->DrawImage(IMAGE_DIALOG_BANNER_LOADING, (mWidth - IMAGE_DIALOG_BANNER_LOADING->GetWidth()) / 2, 17);
	g->SetFont(FONT_SMALL);
	g->SetColor(Color::Black);
	g->WriteString(mMessages[mCurrent], 0, 100, mWidth - 10, 0, true, 0, -1, -1);
	g->SetColor(Color::White);
	g->FillRect(70, 110, mWidth - 140, 20);
	g->SetColor(Color(255, 0, 0));
	if (mApp->mChapterLoadTotal > 0)
		g->FillRect(70, 110, (int)((double)mApp->mChapterLoadCount / mApp->mChapterLoadTotal * (mWidth - 140)), 20);
}
