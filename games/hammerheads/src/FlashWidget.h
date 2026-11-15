#ifndef __HAMMERHEADS_FLASHWIDGET_H__
#define __HAMMERHEADS_FLASHWIDGET_H__

// Sexy::FlashWidget (the framework's, 0x461780-0x47b780 here): a widget that plays a Flash movie. The original hosts the Flash
// ActiveX control in it, in the control's windowless mode, draws the control into an image of the widget's size and pauses the
// movie while the application has lost the focus. The port plays the movie with the shared SWF player (engines/swf, SwfMovie), one
// movie frame every 100 / frame rate updates, so that a test run plays the same frames every time. Hammer Heads plays one movie in
// it, the how-to-play page (HowToPlayWidget). The mouse and the keys that the original passes on to the control are left out: the
// page does not take them.
// Original: class Sexy::FlashWidget : public Widget (0xf8 bytes; the constructor is 0x481790, the destructor 0x47fde0).

#include <string>

#include "Color.h"
#include "SwfMovie.h"
#include "Widget.h"

namespace Sexy
{

class MemoryImage;

// The framework's FlashListener
class FlashListener
{
public:
	virtual ~FlashListener() {}
	// The movie has stopped by itself and the widget is not paused
	virtual void	FlashAnimEnded(int theId) {}
	// The movie's FSCommand
	virtual void	FlashCommand(int theId, const std::string& theCommand, const std::string& theParam) {}
};

class FlashWidget : public Widget, public SwfListener
{
public:
	enum
	{
		STATE_IDLE = 0,			// nothing loaded, or paused
		STATE_PLAYING = 1,
		STATE_STOPPED = 2		// the movie has stopped by itself
	};

	int				mState;					// +0x80
	int				mId;					// +0x84
	FlashListener*	mFlashListener;			// +0x88
	int				mPauseCount;			// +0xac: the movie plays while none are left
	bool			mPausedWhileInactive;	// +0xb0: the widget has paused the movie because the application has lost the focus
	bool			mImageStale;			// +0xc4: a frame has gone by since the image was drawn
	bool			mPauseWhenInactive;		// +0xd0

public:
	FlashWidget(int theId, FlashListener* theListener);							// 0x481790
	virtual ~FlashWidget();														// 0x47fde0

	// 0x47c320: loads the movie (a path in the game's folder) and plays it from its first frame; false when it cannot be played
	bool			StartAnimation(const std::string& theFileName);
	// 0x479080, 0x4790d0: the movie stops at the first pause and plays again at the last unpause
	void			Pause();
	void			Unpause();
	// 0x479100: the movie starts again from its first frame (the control's Rewind and Play)
	void			Restart();

	virtual void	Update() override;											// 0x479130
	virtual void	Draw(Graphics* g) override;									// 0x479ca0
	virtual void	SwfCommand(const std::string& theCommand, const std::string& theArgs) override;	// port

private:
	SwfMovie		mMovie;					// port: the movie, in place of the ActiveX control
	MemoryImage*	mImage;					// +0xa8: the movie's picture
	int				mFrameTime;				// port: updates times the frame rate since the last frame
};

}

#endif
