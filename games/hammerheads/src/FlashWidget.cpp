#include "FlashWidget.h"

#include <cstdint>

#include <SDL.h>

#include "Application.h"
#include "Graphics.h"
#include "MemoryImage.h"

using namespace Sexy;

static_assert(sizeof(ulong) == sizeof(uint32_t), "MemoryImage bits are 32-bit pixels");

namespace
{

// The update rate: a movie frame every this many / frame rate updates
const int FRAME_TIME = 100;

}

// 0x481790
FlashWidget::FlashWidget(int theId, FlashListener* theListener)
{
	mState = STATE_IDLE;
	mId = theId;
	mFlashListener = theListener;
	mPauseCount = 0;
	mPausedWhileInactive = !gApp->mHasFocus;
	mImageStale = true;
	mPauseWhenInactive = true;
	mImage = nullptr;
	mFrameTime = 0;
	mMovie.mListener = this;
}

// 0x47fde0
FlashWidget::~FlashWidget()
{
	delete mImage;
}

// 0x47c320
bool FlashWidget::StartAnimation(const std::string& theFileName)
{
	if (!mMovie.Load(theFileName))
	{
		SDL_Log("FlashWidget: %s: %s", theFileName.c_str(), mMovie.GetError().c_str());
		mState = STATE_IDLE;
		return false;
	}

	// the control showed the first frame as soon as it had the movie
	if (mPauseCount == 0)
		mMovie.Play();
	mMovie.NextFrame();
	mFrameTime = 0;
	mImageStale = true;
	mMovie.Invalidate();
	mState = STATE_PLAYING;
	return true;
}

// 0x479080
void FlashWidget::Pause()
{
	mPauseCount++;
	if (mState != STATE_STOPPED)
		mState = STATE_IDLE;
	if (mPauseCount == 1 && mState != STATE_STOPPED)
		mMovie.Stop();
}

// 0x4790d0
void FlashWidget::Unpause()
{
	mPauseCount--;
	if (mPauseCount == 0 && mState != STATE_STOPPED)
	{
		mState = STATE_PLAYING;
		mMovie.Play();
	}
}

// 0x479100
void FlashWidget::Restart()
{
	mMovie.GotoFrame(0);
	mMovie.Play();
	mFrameTime = 0;
	mImageStale = true;
}

// 0x479130: the movie pauses while the application has no focus (nothing else happens then), and plays again when it has it; the
// listener hears when the movie has stopped by itself
void FlashWidget::Update()
{
	if (mPauseWhenInactive)
	{
		if (!gApp->mHasFocus)
		{
			if (mPausedWhileInactive)
				return;
			Pause();
			mPausedWhileInactive = true;
			return;
		}
		if (mPausedWhileInactive)
		{
			Unpause();
			mPausedWhileInactive = false;
		}
	}

	// the control ran by itself: the port moves the movie on a frame every so many updates
	if (mPauseCount == 0 && mMovie.IsPlaying())
	{
		mFrameTime += (int) (mMovie.GetFrameRate() + 0.5f);
		if (mFrameTime >= FRAME_TIME)
		{
			mFrameTime -= FRAME_TIME;
			mMovie.NextFrame();
			mImageStale = true;
		}
	}

	if (mState == STATE_PLAYING && !mMovie.IsPlaying())
	{
		mState = STATE_STOPPED;
		if (mFlashListener != nullptr && mPauseCount == 0)
			mFlashListener->FlashAnimEnded(mId);
	}

	if (mImageStale)
		MarkDirty();
}

// 0x479ca0: the picture of the movie, drawn again when a frame has gone by
void FlashWidget::Draw(Graphics* g)
{
	if (mWidth <= 0 || mHeight <= 0)
		return;

	if (mImage == nullptr || mImage->mWidth != mWidth || mImage->mHeight != mHeight)
	{
		delete mImage;
		mImage = new MemoryImage(gApp);
		mImage->Create(mWidth, mHeight);
		mImage->SetImageMode(false, false);
		mMovie.Invalidate();
		mImageStale = true;
	}

	if (mImageStale)
	{
		mMovie.Render(reinterpret_cast<uint32_t*>(mImage->GetBits()), mWidth, mHeight, mWidth);
		mImage->BitsChanged();
		mImageStale = false;
	}
	g->DrawImage(mImage, 0, 0);
}

// port: the control's FSCommand event
void FlashWidget::SwfCommand(const std::string& theCommand, const std::string& theArgs)
{
	if (mFlashListener != nullptr)
		mFlashListener->FlashCommand(mId, theCommand, theArgs);
}
