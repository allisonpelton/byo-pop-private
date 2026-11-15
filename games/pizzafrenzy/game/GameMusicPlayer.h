#ifndef __PIZZAFRENZY_GAMEMUSICPLAYER_H__
#define __PIZZAFRENZY_GAMEMUSICPLAYER_H__

// The game's music (Iggle Pop's GameMusicPlayer, constructor 0x450930; the application keeps it at +0x130). It plays
// one music clip at a time through the sound system: a clip asked for while another plays waits until that one has
// stopped.
// Its state machine has three states, each a GameMusicPlayerState: paused (Iggle Pop's PauseState), playing (PlayState)
// and stopping (StoppingState).

#include "Object.h"
#include "Sound.h"
#include "State.h"

#include <string>

namespace pizzafrenzy
{

// A music entry of the tile manifest (0x44 bytes, constructor 0x4504c0)
class GameMusicClip : public engine::Object
{
public:
	GameMusicClip(const std::string& theFile, float theFadeInTime, float theFadeOutTime, float theVolume, bool theLoop,
	              int theCategory);  // 0x4504c0

	const std::string& getFile() const { return mFile; }
	float getFadeInTime() const { return mFadeInTime; }    // 0x450470
	float getFadeOutTime() const { return mFadeOutTime; }  // 0x450480
	float getVolume() const { return mVolume; }            // 0x450490
	bool getLoop() const { return mLoop; }                 // 0x4504a0
	int getCategory() const { return mCategory; }          // 0x4504b0

private:
	std::string mFile;  // +0x0c
	float mFadeInTime;  // +0x28
	float mFadeOutTime; // +0x2c
	float mVolume;      // +0x30
	bool mLoop;         // +0x34
	int mCategory;      // +0x38
};

class GameMusicPlayer;

// A state of the player (constructor 0x450850): the player's calls come to the current one
class GameMusicPlayerState : public engine::State
{
public:
	explicit GameMusicPlayerState(GameMusicPlayer* thePlayer) : mPlayer(thePlayer) {}  // 0x450850

	virtual void playClip(GameMusicClip* theClip) = 0;  // 0x14
	virtual bool isPlaying() = 0;                       // 0x18
	virtual void pause() = 0;                           // 0x1c
	virtual void resume() = 0;                          // 0x20
	virtual void stop() = 0;                            // 0x24

protected:
	// port: makes the player's state of a new one, kept alive while this one's call ends
	void changeState(GameMusicPlayerState* theState);

	GameMusicPlayer* mPlayer;  // +0x0c
};

class GameMusicPlayer : public engine::Object
{
public:
	GameMusicPlayer();  // 0x450930

	// 0x04: 0x450710, the state machine's changeState (0x454ee0)
	void changeState(GameMusicPlayerState* theState) { mStates.changeState(theState); }
	// 0x08: 0x450720: the state's update, once a frame (the application's update, 0x401d90, calls it)
	void update();
	// 0x450aa0: plays the clip, unless it is the one that plays
	void play(GameMusicClip* theClip);
	// 0x450820
	void setEnabled(bool theEnabled);

	engine::Ref<GameMusicClip> mCurrent;  // +0x0c: the clip that plays
	engine::Sound* mSound;                // +0x10: its voice, the sound system's
	engine::Ref<GameMusicClip> mNext;     // +0x14: the clip that waits
	bool mStopping;                       // +0x18: the voice is being stopped
	bool mEnabled;                        // +0x34

private:
	GameMusicPlayerState* getState() { return static_cast<GameMusicPlayerState*>(mStates.getState()); }  // 0x411960

	engine::StateMgr mStates;  // +0x1c
};

// Paused (vtable 0x50000c): nothing plays; a clip asked for starts when the music is enabled
class PauseState : public GameMusicPlayerState
{
public:
	explicit PauseState(GameMusicPlayer* thePlayer) : GameMusicPlayerState(thePlayer) {}  // 0x4508e0

	void enter() override;                                // 0x450610
	void update(engine::UpdateInfo& theInfo) override {}  // 0x4d0470
	void playClip(GameMusicClip* theClip) override;       // 0x450ed0
	bool isPlaying() override { return false; }           // 0x4529d0
	void pause() override {}                              // 0x4d0470
	void resume() override;                               // 0x450af0
	void stop() override;                                 // 0x450630
};

// Playing (vtable 0x500034)
class PlayState : public GameMusicPlayerState
{
public:
	explicit PlayState(GameMusicPlayer* thePlayer) : GameMusicPlayerState(thePlayer) {}

	void enter() override;                           // 0x450650
	void update(engine::UpdateInfo& theInfo) override;  // 0x450b90
	void playClip(GameMusicClip* theClip) override;  // 0x450fb0
	bool isPlaying() override { return true; }       // 0x451610
	void pause() override;                           // 0x450ca0
	void resume() override;                          // 0x450650
	void stop() override;                            // 0x450d40
};

// Stopping (vtable 0x50005c): the voice fades out, and the clip that waits starts when it has
class StoppingState : public GameMusicPlayerState
{
public:
	explicit StoppingState(GameMusicPlayer* thePlayer) : GameMusicPlayerState(thePlayer) {}

	void enter() override;                           // 0x450670
	void update(engine::UpdateInfo& theInfo) override;  // 0x450690
	void playClip(GameMusicClip* theClip) override;  // 0x450fb0
	bool isPlaying() override;                       // 0x4506e0
	void pause() override;                           // 0x450e00
	void resume() override {}                        // 0x4d0470
	void stop() override;                            // 0x450ea0

private:
	void finish();  // 0x28: 0x450ff0
};

} // namespace pizzafrenzy

#endif
