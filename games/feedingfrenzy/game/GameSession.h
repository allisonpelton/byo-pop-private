#ifndef __FEEDINGFRENZY_GAMESESSION_H__
#define __FEEDINGFRENZY_GAMESESSION_H__

// What the player has in the game that is being played (GameSession, 0x18 bytes, constructor 0x435690, the controller's
// +0x58): the fish left, the score, the continues, the score that wins the next extra fish, the time played and the
// mode of the game.

namespace feedingfrenzy
{

// How the game is played (+0x14)
enum GameMode
{
	GAME_MODE_NORMAL = 0,
	GAME_MODE_TIMED = 1   // the stages have a time, and there is no end to the fish left
};

class GameSession
{
public:
	GameSession() : mMode(GAME_MODE_NORMAL) {}  // 0x435690

	// 0x4356a0: a new game: three fish, no score, two continues, an extra fish at 6000, no time
	void reset();
	// 0x4356c0: a continue is used: one fewer, and three fish again
	void useContinue();
	// 0x4356d0: the timed game has none
	int getContinues() const { return mMode == GAME_MODE_TIMED ? 0 : mContinues; }
	int getScore() const { return mScore; }               // 0x4356e0
	void addLives(int theCount) { mLives += theCount; }   // 0x4356f0
	// 0x435700: the timed game does not lose any
	void loseLife()
	{
		if (mMode != GAME_MODE_TIMED)
			mLives--;
	}
	int getLives() const { return mLives; }  // +0x00
	int getMode() const { return mMode; }                  // 0x435710
	void setMode(int theMode) { mMode = theMode; }         // +0x14
	void addSecondsPlayed(float theSeconds) { mSecondsPlayed += theSeconds; }  // 0x435720
	float getSecondsPlayed() const { return mSecondsPlayed; }                  // 0x435730
	void resetSecondsPlayed() { mSecondsPlayed = 0.0f; }                       // 0x44df10
	void clearContinues() { mContinues = 0; }                                  // 0x435740
	// 0x435750: the points are added, and a score that passes the next mark wins an extra fish, with a message and a
	// sound; a score below nothing is made nothing
	void addScore(int thePoints);

private:
	int mLives;                 // +0x00
	int mScore;                 // +0x04
	int mContinues;             // +0x08
	int mNextExtraLifeScore;    // +0x0c
	float mSecondsPlayed;       // +0x10
	int mMode;                  // +0x14: a GameMode
};

} // namespace feedingfrenzy

#endif
