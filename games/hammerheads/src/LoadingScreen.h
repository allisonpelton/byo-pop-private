#ifndef __HAMMERHEADS_LOADINGSCREEN_H__
#define __HAMMERHEADS_LOADINGSCREEN_H__

// The screen shown while the game's files load: the publisher's and the game's logos fade in and out (and a distributor's first, when
// the properties say the game is cobranded), then the title comes down with the hammer swinging under it and the loading bar and the
// picture on the bottom come up. When everything is loaded the bar slides away and "click to play" turns in. A click on the screen
// starts the wipe to the main menu.
// Original: class Sexy::LoadingScreen : public Widget, public ButtonListener, public FlashListener (0xc8 bytes; the constructor is
// 0x458ec0, the destructor 0x459040). The original could also play a flash movie over it; no movie is ever given to it, so that part
// is left out.

#include "ButtonListener.h"
#include "Widget.h"

namespace Sexy
{

class HyperlinkWidget;
class Image;

class LoadingScreen : public Widget, public ButtonListener
{
public:
	// the parts of the show, in the order they play (the distributor's logo only when cobranded)
	enum
	{
		STAGE_DISTRIBUTOR_LOGO = 0,
		STAGE_PUBLISHER_LOGO = 1,
		STAGE_GAME_LOGO = 2,
		STAGE_BEFORE_TITLE = 3,			// over in the update it is entered in
		STAGE_TITLE = 4,				// the title and the hammer come down and the bar and the picture on the bottom come up
		STAGE_LOADING = 5				// everything is in place until the loading is done
	};

	int				mStage;					// +0x88
	double			mStageProgress;			// +0x90: how far the stage is (0 to 1)
	double			mBarFade;				// +0x98: from 0 to 1 once the loading is done: the bar goes, "click to play" comes
	double			mTime;					// +0xa0: runs up by the frame time; the hammer and the title move with it
	double			mTimeAtTitle;			// +0xa8: the time at which the title had come in
	bool			mCobranded;				// +0xb0: the property "Cobranded"
	double			mDistributorSpeed;		// +0xb8: the property "DistributorAnimSpeed": how fast the distributor's logo shows
	HyperlinkWidget*	mPlayButton;		// +0xc0: the invisible button over the whole screen that starts the game

public:
	LoadingScreen();															// 0x458ec0
	virtual ~LoadingScreen();													// 0x459040

	// 0x459120: the play button is made (called once the screen is sized, before it is added to the manager)
	void			Begin();
	// 0x458b70: the files are loaded (Application::LoadingThreadCompleted): the button works and the sound plays
	void			LoadingFinished();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;	// 0x458150
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4581d0
	virtual void	UpdateF(float theFrac) override;							// 0x458c30
	virtual void	Draw(Graphics* g) override;									// 0x458aa0
	virtual void	MouseDown(int x, int y, int theClickCount) override;		// 0x458c00
	using Widget::MouseDown;
	virtual void	ButtonDepress(int theId) override;							// 0x458bb0 (the ButtonListener part)

private:
	void			DrawBackdrop(Graphics* g, double theProgress);				// 0x458250
	void			DrawLoadingBar(Graphics* g, double theFade);				// 0x4582c0
	void			DrawClickToPlay(Graphics* g, double theFade);				// 0x4583a0
	void			DrawDistributorLogo(Graphics* g, double theProgress);		// 0x4584a0
	void			DrawPublisherLogo(Graphics* g, double theProgress);			// 0x458570
	void			DrawGameLogo(Graphics* g, double theProgress);				// 0x458640
	void			DrawTitleAndHammer(Graphics* g, double theProgress);		// 0x458710
	void			DrawBottom(Graphics* g, double theProgress);				// 0x458a10
};

}

#endif
