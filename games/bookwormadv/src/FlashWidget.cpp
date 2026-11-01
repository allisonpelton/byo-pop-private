#include "FlashWidget.h"

#include <cstdint>

#include <SDL.h>

#include "Graphics.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

using namespace Sexy;

static_assert(sizeof(ulong) == sizeof(uint32_t), "MemoryImage bits are 32-bit pixels");

FlashWidget::FlashWidget(int theId, FlashListener* theListener) : mId(theId), mFlashListener(theListener)
{
	mMovie.mListener = this;
}

FlashWidget::~FlashWidget()
{
	delete mImage;
}

bool FlashWidget::StartAnimation(const std::string& theFileName)
{
	if (!mMovie.Load(theFileName)) {
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
	mState = mPauseCount == 0 ? STATE_PLAYING : STATE_IDLE;
	return true;
}

void FlashWidget::Pause()
{
	mPauseCount++;
	if (mState != STATE_STOPPED)
		mState = STATE_IDLE;
}

void FlashWidget::Unpause()
{
	mPauseCount--;
	if (mPauseCount == 0 && mState != STATE_STOPPED)
		mState = STATE_PLAYING;
}

void FlashWidget::Update()
{
	// MyFlashWidget's Update (0x482520) leaves this out while a dialog is open (the app's mDialogMap, +0x328).
	// port: the framework's paused the movie while the app was not active; that is not ported
	if (!gSexyAppBase->mDialogMap.empty())
		return;
	Widget::Update();
	if (mState == STATE_PLAYING) {
		mFrameTime += (int)(mMovie.GetFrameRate() + 0.5f);
		if (mFrameTime >= 100) {
			mFrameTime -= 100;
			mMovie.NextFrame();
			mImageStale = true;
		}
		// the control told the widget when the movie had stopped
		if (mState == STATE_PLAYING && !mMovie.IsPlaying()) {
			mState = STATE_STOPPED;
			if (mFlashListener != nullptr && mPauseCount == 0)
				mFlashListener->FlashAnimEnded(mId);
		}
	}
	if (mRedraw && mImageStale)
		MarkDirty();
}

void FlashWidget::Draw(Graphics* g)
{
	if (mWidth <= 0 || mHeight <= 0)
		return;
	if (mImage == nullptr || mImage->mWidth != mWidth || mImage->mHeight != mHeight) {
		delete mImage;
		mImage = new MemoryImage(gSexyAppBase);
		mImage->Create(mWidth, mHeight);
		mImage->SetImageMode(false, false);
		mMovie.Invalidate();
		mImageStale = true;
	}
	if (mImageStale && mRedraw) {
		mMovie.Render(reinterpret_cast<uint32_t*>(mImage->GetBits()), mWidth, mHeight, mWidth);
		mImage->BitsChanged();
		mImageStale = false;
	}
	g->DrawImage(mImage, 0, 0);
}

void FlashWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	Widget::MouseUp(x, y, theBtnNum, theClickCount);
	if (mMouseOwner != nullptr)
		mMouseOwner->MouseUp(x, y, theBtnNum, theClickCount);
}

void FlashWidget::SwfCommand(const std::string& theCommand, const std::string& theArgs)
{
	if (mFlashListener != nullptr)
		mFlashListener->FlashCommand(mId, theCommand, theArgs);
}
