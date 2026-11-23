#include "GameMusicPlayer.h"
#include "App.h"

using namespace pizzafrenzy;

namespace
{
// The voice of a music clip: a streamed sound of the music's volume category
const int MUSIC_SOUND_CATEGORY = 1;
}

// 0x4504c0
GameMusicClip::GameMusicClip(const std::string& theFile, float theFadeInTime, float theFadeOutTime, float theVolume,
                             bool theLoop, int theCategory) :
	mFile(theFile), mFadeInTime(theFadeInTime), mFadeOutTime(theFadeOutTime), mVolume(theVolume), mLoop(theLoop),
	mCategory(theCategory)
{
}

// port: the state that makes the change is kept until its call ends
void GameMusicPlayerState::changeState(GameMusicPlayerState* theState)
{
	engine::Ref<GameMusicPlayerState> aKeep = this;
	mPlayer->changeState(theState);
}

// 0x450930: the player starts paused, enabled and with nothing to play
GameMusicPlayer::GameMusicPlayer() : mSound(nullptr), mStopping(false), mEnabled(true)
{
	changeState(new PauseState(this));
}

// 0x450720
void GameMusicPlayer::update()
{
	engine::UpdateInfo anInfo;
	anInfo.mSeconds = 0;
	mStates.update(anInfo);
}

// 0x450aa0: a clip that already plays is let play, unless it is being stopped
void GameMusicPlayer::play(GameMusicClip* theClip)
{
	if (theClip == nullptr)
		return;
	if (theClip == mCurrent)
	{
		if (getState()->isPlaying() && !mStopping)
			return;
	}
	getState()->playClip(theClip);
}

// 0x450820
void GameMusicPlayer::setEnabled(bool theEnabled)
{
	mEnabled = theEnabled;
	if (theEnabled)
		getState()->resume();
	else
		getState()->pause();
}

// 0x450610
void PauseState::enter()
{
	if (mPlayer->mSound != nullptr)
		mPlayer->mSound->halt();
}

// 0x450ed0
void PauseState::playClip(GameMusicClip* theClip)
{
	mPlayer->mNext = theClip;
	if (mPlayer->mEnabled)
		changeState(new PlayState(mPlayer));
}

// 0x450af0
void PauseState::resume()
{
	changeState(new PlayState(mPlayer));
}

// 0x450630
void PauseState::stop()
{
	if (mPlayer->mSound != nullptr)
		mPlayer->mSound->stop();
}

// 0x450650
void PlayState::enter()
{
	if (mPlayer->mSound != nullptr)
		mPlayer->mSound->play();
}

// 0x450b90: a clip that waits is started by stopping the one that plays; a voice that has ended pauses the player
void PlayState::update(engine::UpdateInfo& theInfo)
{
	if (mPlayer->mNext != nullptr)
		changeState(new StoppingState(mPlayer));
	else if (mPlayer->mSound != nullptr && !mPlayer->mSound->isPlaying())
		changeState(new PauseState(mPlayer));
}

// 0x450fb0
void PlayState::playClip(GameMusicClip* theClip)
{
	mPlayer->mNext = theClip;
}

// 0x450ca0
void PlayState::pause()
{
	changeState(new PauseState(mPlayer));
}

// 0x450650
void PlayState::resume()
{
	if (mPlayer->mSound != nullptr)
		mPlayer->mSound->play();
}

// 0x450d40
void PlayState::stop()
{
	mPlayer->mNext = nullptr;
	changeState(new StoppingState(mPlayer));
}

// 0x450670: a voice that plays is stopped and the player marked as stopping; without one nothing is
void StoppingState::enter()
{
	if (mPlayer->mSound != nullptr)
	{
		mPlayer->mSound->stop();
		mPlayer->mStopping = true;
	}
}

// 0x450690: when the voice has stopped it is closed, and the clip that waits starts
void StoppingState::update(engine::UpdateInfo& theInfo)
{
	if (mPlayer->mSound == nullptr)
	{
		finish();
	}
	else if (!mPlayer->mSound->isPlaying())
	{
		mPlayer->mStopping = false;
		mPlayer->mSound->close();
		mPlayer->mSound = nullptr;
		finish();
	}
}

// 0x450fb0
void StoppingState::playClip(GameMusicClip* theClip)
{
	mPlayer->mNext = theClip;
}

// 0x4506e0
bool StoppingState::isPlaying()
{
	return mPlayer->mSound != nullptr && mPlayer->mSound->isPlaying();
}

// 0x450e00
void StoppingState::pause()
{
	changeState(new StoppingState(mPlayer));
}

// 0x450ea0
void StoppingState::stop()
{
	mPlayer->mNext = nullptr;
}

// 0x450ff0: without a clip that waits the player pauses; otherwise the clip becomes the current one, its file is
// loaded as a stream and a voice of it is made, set from the clip, started and played
void StoppingState::finish()
{
	if (mPlayer->mNext == nullptr)
	{
		changeState(new PauseState(mPlayer));
		return;
	}
	mPlayer->mCurrent = mPlayer->mNext;
	mPlayer->mNext = nullptr;
	GameMusicClip* aClip = mPlayer->mCurrent;
	engine::App::instance()->loadSoundStream(aClip->getFile(), aClip->getFile());
	mPlayer->mSound = engine::App::instance()->createSound(aClip->getFile(), false, MUSIC_SOUND_CATEGORY);
	if (mPlayer->mSound != nullptr)
	{
		mPlayer->mSound->setFadeIn(aClip->getFadeInTime());
		mPlayer->mSound->setFadeOut(aClip->getFadeOutTime());
		mPlayer->mSound->setLoop(aClip->getLoop());
		mPlayer->mSound->setVolume(aClip->getVolume());
		mPlayer->mSound->setCategory(aClip->getCategory());
		mPlayer->mSound->play();
		changeState(new PlayState(mPlayer));
	}
	else
	{
		mPlayer->mCurrent = nullptr;
		changeState(new PauseState(mPlayer));
	}
}
