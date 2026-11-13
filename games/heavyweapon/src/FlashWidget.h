#ifndef __HEAVYWEAPON_FLASHWIDGET_H__
#define __HEAVYWEAPON_FLASHWIDGET_H__

// Sexy::FlashWidget (the framework's, 0x461780-0x469200): a widget that plays a Flash movie. The original hosts the
// Flash ActiveX control in it and draws the control into an image of the widget's size; the port plays the movie with
// the shared SWF player (engines/swf, SwfMovie), one movie frame every 100 / frame rate updates, so a test run plays
// the same frames every time. Heavy Weapon plays its three cutscenes in it (TitleScreen, CutsceneScreen).
// Original: 0xbc bytes, constructor 0x468b80(int theId, FlashListener*), vtable 0x5327dc, destructor 0x467b70.

#include "SwfMovie.h"
#include "Widget.h"

#include <string>

namespace Sexy
{

class MemoryImage;

// The framework's FlashListener
class FlashListener
{
public:
	virtual ~FlashListener() {}
	// The movie stopped by itself
	virtual void	FlashAnimEnded(int theId) {}
	// The movie's FSCommand
	virtual void	FlashCommand(int theId, const std::string& theCommand, const std::string& theParam) {}
};

class FlashWidget : public Widget, public SwfListener
{
public:
	enum
	{
		STATE_IDLE = 0,
		STATE_PLAYING = 1,
		STATE_STOPPED = 2
	};

	int				mState;					// +0x58 STATE_
	int				mId;					// +0x5c
	FlashListener*	mFlashListener;			// +0x60
	int				mPauseCount;			// +0x84 the movie plays while none are left
	bool			mPausedWhileInactive;	// +0x88 the widget paused the movie while the app is not active
	bool			mPauseWhenInactive;		// +0x94
	Color			mBkgColor;				// +0xac filled under the movie when its alpha is not 0

public:
	FlashWidget(int theId, FlashListener* theListener);							// 0x468b80
	virtual ~FlashWidget();														// 0x467b70

	// 0x4645a0: loads the movie (a path in the game's folder) and plays it from its first frame, unless paused. False
	// when it cannot be played; the widget then stays idle.
	bool			StartAnimation(const std::string& theFileName);
	void			Pause();													// 0x461800
	void			Unpause();													// 0x461830

	virtual void	Update() override;											// 0x461850 slot 17
	virtual void	Draw(Graphics* g) override;									// 0x4618f0 slot 16
	virtual void	SwfCommand(const std::string& theCommand, const std::string& theArgs) override; // port

private:
	SwfMovie		mMovie;					// port: the movie, in place of the ActiveX control
	MemoryImage*	mImage;					// +0x80 the movie's picture
	int				mFrameTime;				// port: updates times the frame rate since the last frame
	bool			mImageStale;			// +0x89 a frame went by since the image was drawn
};

}

#endif
