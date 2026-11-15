#ifndef __PIXELUS_STORYSCREEN_H__
#define __PIXELUS_STORYSCREEN_H__

// StoryScreen.cpp: the conversations between Claudius and the gods (Pixelus.exe 0x41cf50 constructor, 0x41d290 Draw,
// 0x41ce50 Update, 0x41cdc0 Resize, 0x41ce10 button; vftable 0x50ed30; the script reader 0x42db50 and the speech 0x42d810).
// Each script is a text file data/story/<from><to>.txt: a line ending in a colon names who speaks (a temple's god, or
// Claudius), the lines below are what is said. All of a script's speeches stand one under the other on a sky; each
// comes in after the one before it (0.5 s, then 2 s apart), with its speaker's portrait on one side and the bubble on
// the other. A button at the bottom goes on to the island.
//
// A script is named by two temples: the temple the player is in and the one that has just opened (the first, "tutorial
// tutorial", is the introduction, and the last, "jupiter tutorial", the ending).
//
// port: the portrait is the speaker's head, still; the original renders its figure into a picture of its own
// (0x42e110) and has the speaker's mouth move while the bubble is on.

#include "Widget.h"
#include "ButtonListener.h"
#include <string>
#include <vector>

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class WidgetManager;
class Character;
class MemoryImage;

class StoryScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		ID_CONTINUE = 1,
		CLAUDIUS = 10			// the speaker after the ten temples' gods
	};

	// where the button leads (the original's field +0x70: 0x433d00, 0x432d40 or 0x432d80)
	enum After
	{
		AFTER_ENDING = 0,
		AFTER_MENU = 1,
		AFTER_ISLAND = 2
	};

	struct Speech
	{
		int						mSpeaker;		// an index of gTemples, or CLAUDIUS
		std::vector<std::string> mLines;
		int						mY;				// +0
		double					mStart;			// +0x10 seconds into the screen when it comes in
		MemoryImage*			mPortrait;		// +0x34 the speaker as drawn (160 by 160), +0x38 the frame with it
		MemoryImage*			mFramed;
		Character*				mCharacter;		// +0x30 the speaker's moods (0x42e110), which move from half a second after it comes in
	};

	PixelusApp*			mApp;
	DialogButton*		mContinueButton;		// +0x60
	std::vector<Speech>	mSpeeches;				// +0x74 (the script)
	int					mAfter;					// +0x70
	double				mTime;					// seconds (+0.01 an update)
	int					mMusicDelay;			// +0x64 updates until the music starts

public:
	// theFrom, theTo: the two temples that name the script
	StoryScreen(PixelusApp* theApp, bool theFromMenu, int theFrom, int theTo, int theMode);
	virtual ~StoryScreen();

	bool			HasSpeeches() const { return !mSpeeches.empty(); }

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x41cdc0
	virtual void	Update() override;													// 0x41ce50
	virtual void	Draw(Graphics* g) override;											// 0x41d290
	virtual void	ButtonDepress(int theId) override;									// 0x41ce10

private:
	void			Load(const std::string& theName);									// 0x42db50
	void			DrawSpeech(Graphics* g, const Speech& theSpeech, int theOrdinal);
};

}

#endif
