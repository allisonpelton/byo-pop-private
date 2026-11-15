#include "FlashWidget.h"

#include <cstdint>

#include <SDL.h>

#include "Graphics.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

using namespace Sexy;

static_assert(sizeof(ulong) == sizeof(uint32_t), "MemoryImage bits are 32-bit pixels");

namespace
{
	const int FRAME_TIME = 100;		// the update rate: a movie frame every 100 / frame rate updates
}

// 0x468b80
FlashWidget::FlashWidget(int theId, FlashListener* theListener)
{
	mState = STATE_IDLE;
	mId = theId;
	mFlashListener = theListener;
	mPauseCount = 0;
	mPausedWhileInactive = !gSexyAppBase->mActive;
	mPauseWhenInactive = true;
	mBkgColor = Color(0, 0, 0, 0);
	mImage = NULL;
	mFrameTime = 0;
	mImageStale = true;
	mMovie.mListener = this;
}

// 0x467b70
FlashWidget::~FlashWidget()
{
	delete mImage;
}

// 0x4645a0
bool FlashWidget::StartAnimation(const std::string& theFileName)
{
	if (!mMovie.Load(theFileName))
	{
		SDL_Log("FlashWidget: %s: %s", theFileName.c_str(), mMovie.GetError().c_str());
		mState = STATE_IDLE;
		return false;
	}
	// port: the control showed the first frame as soon as it had the movie
	mMovie.Play();
	mMovie.NextFrame();
	mFrameTime = 0;
	mImageStale = true;
	mMovie.Invalidate();
	mState = STATE_PLAYING;
	return true;
}

// 0x461800: the first pause stops the movie (so the next Update reports it ended, as the control did)
void FlashWidget::Pause()
{
	mPauseCount++;
	if (mPauseCount == 1)
		mMovie.Stop();
}

// 0x461830
void FlashWidget::Unpause()
{
	mPauseCount--;
	if (mPauseCount == 0)
		mMovie.Play();
}

// 0x461850 slot 17: the movie pauses while the app is not active; the listener hears when it has stopped, which a
// pause counts as
void FlashWidget::Update()
{
	if (mPauseWhenInactive)
	{
		if (!gSexyAppBase->mActive)
		{
			if (!mPausedWhileInactive)
			{
				Pause();
				mPausedWhileInactive = true;
			}
		}
		else if (mPausedWhileInactive)
		{
			Unpause();
			mPausedWhileInactive = false;
		}
	}
	if (mState == STATE_PLAYING)
	{
		if (mPauseCount == 0)
		{
			mFrameTime += (int)(mMovie.GetFrameRate() + 0.5f);
			if (mFrameTime >= FRAME_TIME)
			{
				mFrameTime -= FRAME_TIME;
				mMovie.NextFrame();
				mImageStale = true;
			}
		}
		if (!mMovie.IsPlaying())
		{
			mState = STATE_STOPPED;
			if (mFlashListener != NULL)
				mFlashListener->FlashAnimEnded(mId);
		}
	}
	if (mImageStale)
		MarkDirty();
}

// 0x4618f0 slot 16: the background colour under the movie's picture
void FlashWidget::Draw(Graphics* g)
{
	if (mWidth <= 0 || mHeight <= 0)
		return;
	if (mImage == NULL || mImage->mWidth != mWidth || mImage->mHeight != mHeight)
	{
		delete mImage;
		mImage = new MemoryImage(gSexyAppBase);
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
	// 0x461780 slot 48
	if (mBkgColor.mAlpha != 0)
	{
		g->SetColor(mBkgColor);
		g->FillRect(0, 0, mWidth, mHeight);
	}
	g->DrawImage(mImage, 0, 0);
}

// port: the control's FSCommand event
void FlashWidget::SwfCommand(const std::string& theCommand, const std::string& theArgs)
{
	if (mFlashListener != NULL)
		mFlashListener->FlashCommand(mId, theCommand, theArgs);
}
