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
		GT(0x6f929c), GT(0x6f92b0), GT(0x6f92c8), GT(0x007ab85c), GT(0x6f92d8), GT(0x007ab88c),
		GT(0x6f9300), GT(0x6f9318), GT(0x6f9330), GT(0x007ab8f4), GT(0x6f9354), GT(0x6f936c),
		GT(0x6f9388), GT(0x6f93ac), GT(0x6f93d4), GT(0x6f93f0), GT(0x6f940c), GT(0x6f9420),
		GT(0x6f9434), GT(0x007aba0c), GT(0x007aba1c), GT(0x6f9448), GT(0x6f9460), GT(0x6f9480),
		GT(0x6f9494), GT(0x6f94b8), GT(0x6f94dc),
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
