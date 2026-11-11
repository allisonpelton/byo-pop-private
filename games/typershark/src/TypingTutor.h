#ifndef __TYPERSHARK_TYPINGTUTOR_H__
#define __TYPERSHARK_TYPINGTUTOR_H__

// TypingTutor.cpp / TypingTutorDraw.cpp / TypingTutorKeys.cpp / TypingTutorLessons.cpp: the typing tutor, the lessons
// screen of the main menu ("Typing Tutor"): a full-window widget that shows one of 19 lessons (lesson 0 is the typing test),
// the lesson's tasks (a heading and a few lines of text to type), a keyboard picture with the finger and key to use next,
// a clock, the error count, and the buttons next / previous lesson, options and back to the menu. The app makes it in
// ButtonDepress case 0x66 (WinWSAppButtons.cpp) and keeps it at WinWSApp::mTypingTutor. While no key has been typed, sharks
// swim across the water behind the panel (state 0); typing the first key starts the lesson (state 1); when the last task is
// done or the time is up the statistics show (state 2) and a dialog asks whether to see the stats (or, for the typing test,
// shows the result).
//
// Original: class Sexy::TypingTutor : public Widget, public ButtonListener, vftable 0x531544 (the ButtonListener part,
// vftable 0x531528, is at +0x58), sizeof 0x264, constructor 0x460b30, destructor body 0x45bd30 (deleting 0x45e770).
//
// The tutor's own lesson list is a second class of the export (RTTI Sexy::LessonManager, vftable 0x52c728, constructor
// inlined in 0x460b30, destructor body 0x427300, deleting destructor 0x429900); its name clashes with src/LessonManager.h
// (the level table of a game, another class), so the port calls it TutorLessons, and its lesson TutorLesson, its task
// TutorExercise and the small drawing record of a finger, key label or key glow TutorSprite (0x28 bytes).
//
// The std::vector / std::string / std::map instantiations of the export (0x4422f0, 0x4512a0, 0x4512e0, 0x451310, 0x451515,
// 0x4515cf, 0x4516c0, 0x44cc70, 0x44cecf, 0x44d090, 0x44d0e0, 0x44d120, 0x44d1a2, 0x44462e, 0x4445b0, 0x444550, 0x450c70,
// 0x450ca0, 0x454440, 0x441a70, 0x4433e0, 0x45c0b0) are the standard containers here.

#include "Widget.h"
#include "ButtonListener.h"
#include <string>
#include <vector>

namespace Sexy
{

class DialogButton;
class Enemy;
class Graphics;
class Image;
class UserProfile;
class WSTimer;
class WidgetManager;
class WinWSApp;

// a picture with a short text at a place (0x28 bytes, destructor 0x4433e0, vector push 0x45c0b0): a finger of the hands
// picture, a ring around a finger with the key's name, or a key of the keyboard picture that lights up
struct TutorSprite
{
	Image*					mImage;				// +0x00
	std::string				mText;				// +0x04 (the std::string of the export is 0x1c bytes)
	int						mX;					// +0x20
	int						mY;					// +0x24

	TutorSprite() : mImage(NULL), mX(0), mY(0) {}
	TutorSprite(Image* theImage, const std::string& theText, int theX, int theY) : mImage(theImage), mText(theText), mX(theX), mY(theY) {}
};

// one task of a lesson: a heading and the lines to type (0x34 bytes, constructor 0x420c10, freed by 0x427300)
struct TutorExercise
{
	std::string				mHeading;			// +0x00 <HEADING n> picks one of the lesson's <HEADINGS>
	int						mCurText;			// +0x1c the line being typed
	int						mCurChar;			// +0x20 the character of the line that is next
	std::vector<std::string> mTexts;			// +0x24 (begin at +0x28) the lines, from <TEXT> or from a text file

	TutorExercise() : mCurText(0), mCurChar(0) {}
};

// one lesson of data\lessonN.xml (0x44 bytes, constructor 0x420bd0, freed by 0x427300)
class TutorLesson
{
public:
	int						mMaxErrors;			// +0x00 <MAXERRORS> most errors that still pass (-1: any number)
	int						mMinWPM;			// +0x04 <MINWPM> least adjusted words per minute that pass (-1: none)
	int						mMinTasks;			// +0x08 <MINTASKS> least tasks that must be finished (-1: all of them; 0: none)
	int						mMinAccuracy;		// +0x0c <MINACCURACY> least accuracy in percent (-1: none)
	int						mLessonTime;		// +0x10 <LESSONTIME> seconds the lesson lasts (-1: no limit); the original leaves it unset when the tag is missing
	std::string				mDescription;		// +0x14 <DESCRIPTION> the text at the right of the panel
	std::vector<TutorExercise*> mTasks;			// +0x30 (begin at +0x34)
	int						mUserState;			// +0x40 the user's result for the lesson: -1 not finished yet, 0 failed, 1 passed (UserProfile::mField94)

public:
	TutorLesson();								// 0x420bd0 (the caller then sets the first four ints to -1)
	~TutorLesson();								// the loop of 0x427300 that frees a lesson's tasks and the lesson
};

// the list of lessons (0x1c bytes; ctor inlined in 0x460b30; vftable 0x52c728, deleting destructor 0x429900)
class TutorLessons
{
public:
	std::vector<TutorLesson*> mLessons;			// +0x08 (begin; the first +0x04 is the vector's padding) the lessons loaded so far
	int						mCurrent;			// +0x14 the lesson that is shown, -1 before the first
	WinWSApp*				mApp;				// +0x18

public:
	TutorLessons(WinWSApp* theApp);				// inlined in 0x460b30
	~TutorLessons();							// 0x427300 (deleting 0x429900)

	TutorLesson*			NextLesson();		// 0x429930 the next lesson, read from data\lesson<n>.xml the first time it is asked for
	TutorLesson*			PrevLesson();		// 0x413bb0
};

class TypingTutor : public Widget, public ButtonListener
{
public:
	enum
	{
		NEXT_BUTTON_ID = 0x58,				// the next lesson
		PREV_BUTTON_ID = 0x59,				// the previous lesson
		MENU_BUTTON_ID = 0x5a,				// back to the menu (asks first)
		OPTIONS_BUTTON_ID = 0x5b			// the options dialog
	};

	enum
	{
		STATE_START = 0,					// no key typed yet: the lesson is shown, sharks swim behind it
		STATE_TYPING = 1,
		STATE_FINISHED = 2					// the statistics are shown
	};

public:
	// +0x58 the ButtonListener part (vftable 0x531528)
	DialogButton*			mNextButton;		// +0x5c id 0x58, hidden after the last lesson
	DialogButton*			mPrevButton;		// +0x60 id 0x59, hidden at the first lesson
	DialogButton*			mOptionsButton;		// +0x64 id 0x5b
	DialogButton*			mMenuButton;		// +0x68 id 0x5a
	TutorLessons*			mLessons;			// +0x6c
	TutorLesson*			mLesson;			// +0x70 the lesson that is shown
	int						mTimeLeft;			// +0x74 seconds left of a timed lesson
	int						mTask;				// +0x78 the task that is being typed
	int						mErrors;			// +0x7c the keys typed wrong (each character counts once)
	int						mCleanKeys;			// +0x80 the right keys that were typed without a wrong key before them
	int						mCorrectKeys;		// +0x84 all the right keys
	int						mShift;				// +0x88 which shift key the next character needs: 0 none, 1 right, -1 left
	int						mRawWPM;			// +0x8c words per minute from all the right keys
	int						mAdjustedWPM;		// +0x90 words per minute from the clean keys
	int						mSeconds;			// +0x94 seconds since the first key
	bool					mCleanChar;			// +0x98 no wrong key typed for the character that is next
	int						mAccuracy;			// +0x9c percent of the keys that were right the first time
	char					mExpectedChar;		// +0xa0 the character that is next
	TutorSprite				mFingers[9];		// +0xa4 the nine finger pictures of the hands (0x28 each, to +0x20c); 4 is the thumb
	std::vector<TutorSprite> mLabels;			// +0x20c (begin at +0x210) the rings with the key's name around the finger to use
	std::vector<TutorSprite> mGlows;			// +0x21c (begin at +0x220) the keys of the keyboard picture that light up
	WSTimer*				mSecondTimer;		// +0x22c counts the lesson's seconds
	WSTimer*				mBlinkTimer;		// +0x230 blinks the cursor
	WSTimer*				mErrorTimer;		// +0x234 flashes the highlighted key after a wrong key
	bool					mFlashOn;			// +0x238 the glows are drawn (off while they flash)
	int						mState;				// +0x23c STATE_START, STATE_TYPING or STATE_FINISHED
	std::vector<Enemy*>		mCreatures;			// +0x240 (begin at +0x244) the sharks and piranhas that swim behind the panel
	bool					mCursorOn;			// +0x250 the cursor under the next character is drawn
	int						mFirstLine;			// +0x254 the first of the four lines that are shown
	int						mPauseCount;		// +0x258 above 0 while the window has lost the focus or a dialog is up
	WinWSApp*				mApp;				// +0x25c
	bool					mFlag260;			// +0x260 constructor 0, never read
	bool					mFlag261;			// +0x261 the "view stats" dialog is up; the app reads it in ButtonDepress
	bool					mFlag262;			// +0x262 the "quit" or "test completed" dialog is up; the app reads it in ButtonDepress

public:
	TypingTutor(WinWSApp* theApp);														// 0x460b30
	virtual ~TypingTutor() override;													// 0x45bd30 (deleting 0x45e770), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x44b260 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x441920 slot 2
	virtual void			Draw(Graphics* g) override;									// 0x44b400 slot 16
	virtual void			Update() override;											// 0x4601a0 slot 17
	virtual void			GotFocus() override;										// 0x442080 slot 18
	virtual void			LostFocus() override;										// 0x442020 slot 19
	virtual void			KeyChar(SexyChar theChar) override;							// 0x460680 slot 20
	virtual void			MouseMove(int x, int y) override;							// 0x441a40 slot 25

	virtual void			ButtonPress(int theId) override;							// 0x45bf50 listener slot 3: the click sound
	virtual void			ButtonDepress(int theId) override;							// 0x460900 listener slot 4

	void					SetPaused(bool thePause);									// 0x441a00
	void					DrawCursor(Graphics* g, int theY, int theX, char theChar);	// 0x441990
	void					AddCreature();												// 0x454010
	void					GoToNextLesson();											// 0x460430
	void					GoToPrevLesson();											// 0x4604f0
	void					AdvanceChar();												// 0x4605a0
	void					ResetLesson();												// 0x45e7a0
	void					ResetFingers();												// 0x45bf70 (TypingTutorKeys.cpp)
	void					SetupHighlight();											// 0x45e810 (TypingTutorKeys.cpp)
	void					Finish();													// 0x45fb10
};

}

#endif
