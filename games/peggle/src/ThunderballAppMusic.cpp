// ThunderballAppMusic.cpp: the music of Peggle's application class. Three pieces of music are on slots: the ode of the fever
// (0), the level music (1: one module whose orders are the pieces of music, the board plays the order a level asks for) and
// the opening (2, the menu's). The music ids PlayMusic takes are the game's own: 1000 and 2000 are two orders the board's
// ambience uses, 3000 the menu's, and any other positive id is the level music of order id - 1.

#include "ThunderballApp.h"

#include "MusicInterface.h"

using namespace Sexy;

static const int SLOT_ODE = 0;
static const int SLOT_LEVEL_MUSIC = 1;
static const int SLOT_OPENING = 2;

static const int ORDER_AMBIENCE_A = 0x3c;	  // the orders of the level music the two ambiences are
static const int ORDER_AMBIENCE_B = 0x3e;
static const double FADE_SPEED = 0.01;		  // the speed every fade of the music has
static const int MUSIC_INTENSITIES = 6;		  // the orders of a piece of level music: one for each intensity
static const int MAX_INTENSITY = 6;
static const int ROWS_PER_BAR = 16;			  // ThunderballApp.cpp lines 157, 158
static const int OPENING_RESTART_ROW = 0x51;  // line 159: when the opening is at this row, the ambience starts over

// 0x40bdd0
void ThunderballApp::PlayMusic(int theMusic, bool theLoop)
{
	bool aChanged = mCurMusic != theMusic;
	mCurMusic = theMusic;

	if (theMusic <= 0)
	{
		// the ode: nothing else plays
		mMusicInterface->StopAllMusic();
		mMusicInterface->SetSongVolume(theMusic, 1.0);
		mMusicInterface->PlayMusic(theMusic, 0, !theLoop);
	}
	else if (theMusic == MUSIC_AMBIENCE_A || theMusic == MUSIC_AMBIENCE_B)
	{
		mMusicInterface->FadeOut(SLOT_ODE, false, FADE_SPEED);
		mMusicInterface->FadeOut(SLOT_OPENING, false, FADE_SPEED);
		mMusicInterface->StopMusic(SLOT_LEVEL_MUSIC);
		mMusicInterface->PlayMusic(SLOT_LEVEL_MUSIC, theMusic == MUSIC_AMBIENCE_A ? ORDER_AMBIENCE_A : ORDER_AMBIENCE_B, false);
	}
	else if (theMusic == MUSIC_MENU)
	{
		mMusicInterface->FadeOut(SLOT_ODE, false, FADE_SPEED);
		mMusicInterface->FadeOut(SLOT_LEVEL_MUSIC, false, FADE_SPEED);
		mMusicInterface->StopMusic(SLOT_OPENING);
		mMusicInterface->SetSongVolume(SLOT_OPENING, 1.0);
		mMusicInterface->PlayMusic(SLOT_OPENING, 0, !theLoop);
	}
	else
	{
		// level music: the order of the module
		mCurMusic = theMusic;
		mMusicInterface->FadeOut(SLOT_ODE, false, FADE_SPEED);
		mMusicInterface->StopMusic(SLOT_LEVEL_MUSIC);
		mMusicInterface->FadeOut(SLOT_OPENING, false, FADE_SPEED);
		mMusicInterface->PlayMusic(SLOT_LEVEL_MUSIC, theMusic - 1, false);
	}

	if (aChanged)
	{
		if (mCurMusic == 0)
			SyncOdeVolume();
		else
			SetMusicVolume(mMusicVolume);
	}
}

// 0x40d180
void ThunderballApp::CheckPlayMusic(bool thePlay)
{
	if (thePlay)
	{
		PlayMusic(MUSIC_AMBIENCE_A, true);
		return;
	}
	mMusicInterface->FadeOut(SLOT_ODE, false, FADE_SPEED);
	mMusicInterface->FadeOut(SLOT_LEVEL_MUSIC, false, FADE_SPEED);
	mMusicInterface->FadeOut(SLOT_OPENING, false, FADE_SPEED);
}

// 0x40d200
void ThunderballApp::UpdateMusic()
{
	if (!mMusicIntensityPending)
	{
		if (mCurMusic == MUSIC_AMBIENCE_A && GetMusicRow(SLOT_OPENING) == OPENING_RESTART_ROW)
			PlayMusic(MUSIC_AMBIENCE_A, true);
	}
	else if (GetMusicRow(SLOT_LEVEL_MUSIC) / ROWS_PER_BAR != mMusicPendingRow / ROWS_PER_BAR)
	{
		// the next bar has come: the order of the new intensity starts
		PlayMusic(mCurMusic + 1, true);
		mMusicIntensityPending = false;
		mMusicPendingRow = -1;
	}
}

// 0x4059c0: the level music's id is the order + 1, and the six orders of a piece are its intensities
void ThunderballApp::SetMusicIntensityIncreasePending(int theIntensity)
{
	if (theIntensity > MAX_INTENSITY)
		return;

	int aOrder = mCurMusic - 1;
	mCurMusic = mCurMusic + ((aOrder / MUSIC_INTENSITIES) * MUSIC_INTENSITIES - aOrder) - 2 + theIntensity;
	int aRow = GetMusicRow(SLOT_LEVEL_MUSIC);
	if (aRow != -1)
	{
		mMusicIntensityPending = true;
		mMusicPendingRow = aRow;
	}
}

// 0x405900
void ThunderballApp::SetMusicVolume(double theVolume)
{
	if (mCurMusic != 0)
		SexyAppBase::SetMusicVolume(theVolume);
}

// 0x405940
void ThunderballApp::SetFeverVolume(double theVolume)
{
	mFeverVolume = theVolume;
	if (mCurMusic == 0)
		SyncOdeVolume();
}

// 0x4058b0
void ThunderballApp::SyncOdeVolume()
{
	if (mMusicInterface != nullptr)
		mMusicInterface->SetVolume(mMuteCount > 0 ? 0.0 : mFeverVolume);
}

// 0x405970
int ThunderballApp::GetMusicRow(int theMusic)
{
	int aPosition = mMusicInterface->GetMusicPosition(theMusic);
	if (aPosition == -1)
		return -1;
	return aPosition >> 16;
}
