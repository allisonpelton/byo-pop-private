#ifndef __PIXELUS_TEMPLESCREEN_H__
#define __PIXELUS_TEMPLESCREEN_H__

// TempleScreen.cpp: the puzzles of one temple, nine at a time (Pixelus.exe 0x413ce0 constructor, 0x412990 Draw,
// 0x4119e0 the thumbnails, 0x411600 MouseDown, 0x4102f0 hit test, 0x414510 buttons; vftable 0x50d868, listeners
// 0x50d84c / 0x50d838). Three columns of 96x96 pictures, one per puzzle: a silhouette until the puzzle is solved,
// then its picture and a medal for how few moves it took. The first temple is the cave where the tutorial is
// taught and its puzzles come in order; the last puzzle of every other temple is a mystery that opens when all
// the others are solved.
//
// port: the editor's buttons and its thumbnail captions are not ported. The god standing at the left is
// GodFigure (0x42e110 / 0x42e1f0), without its speech bubbles. The scroll bar is a widget of its own (Scrollbar.h).

#include "Widget.h"
#include "ButtonListener.h"
#include "ScrollListener.h"
#include "Scrollbar.h"
#include "KeyCodes.h"
#include <string>
#include <vector>

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class MemoryImage;
class Dialog;
class Character;
class ShackleBreak;

// 0x42e110 / 0x42e1f0: the temple's god, a body with a head and arms that sway

// the figure of Claudius, after the ten gods (0x42ebce)
enum { CLAUDIUS_FIGURE = 10 };
// 0x42e1f0: a character drawn in the way the temple, the story and the tutorial show it
void DrawCharacter(PixelusApp* theApp, Character* theCharacter, Graphics* g);

class GodFigure
{
public:
	PixelusApp*		mApp;
	int				mGod;			// +0x20 0 the cyclops of the tutorial, then the temples' gods in order
	Character*		mCharacter;		// the moods (0x42e110)

public:
	GodFigure(PixelusApp* theApp, int theGod, Widget* theOwner, int theMode);
	~GodFigure();
	void			Update(Widget* theOwner);
	void			Draw(Graphics* g);
	void			Poke();			// 0x42de40: a click on the god hurries its bubble
};

class TempleScreen : public Widget, public ButtonListener, public ScrollListener, public ScrollKeys
{
public:
	enum
	{
		ID_BACK = 3,
		ID_MORE = 4,
		ID_SKIP = 5,
		COLUMNS = 3,				// 0x53d250
		ROWS = 3,					// 0x53d254
		CELL = 96,
		PITCH = 106,				// 0x53d248 + 0x60
		LEFT = 231,					// 0x53d240 + 0x53d238
		TOP = 95,					// 0x53d244 + 0x53d23c
		FRAME_X = 224,
		FRAME_Y = 86,
		FRAME_W = 322,
		FRAME_H = 326
	};

	// 0xb0 bytes in the original (list at +0xb8)
	struct Thumb
	{
		std::string		mFile;
		std::string		mUuid;
		MemoryImage*	mShape;			// +0x88 the puzzle as a pale shape
		MemoryImage*	mPicture;		// +0x8c its picture
		int				mMedal;			// +0x94 0 none, 1 bronze, 2 silver, 3 gold
		bool			mSolved;		// +0x9c is 0xff
		int				mFade;			// +0x9c the shape turning into the picture
		bool			mMystery;		// +0xa0 the last puzzle of a temple, locked until the others are solved
		Thumb() : mShape(NULL), mPicture(NULL), mMedal(0), mSolved(false), mFade(0), mMystery(false) {}
	};

	PixelusApp*		mApp;				// +0x68
	MemoryImage*	mBackdrop;			// +0x94 the island as it was, or NULL
	ShackleBreak*	mShackles;			// +0x8c the chains of the mystery puzzle blowing apart (0x419700), or NULL
	class TempleFade* mFade;			// +0x94 the island fading out over the screen (0x41c610), or NULL
	DialogButton*	mBackButton;		// +0x78
	DialogButton*	mMoreButton;		// +0x7c
	DialogButton*	mSkipButton;		// +0x80
	PixelusScrollbar* mScrollbar;		// +0x84 its value is the row at the top, its page the rows in view
	std::vector<Thumb> mThumbs;			// +0xb8
	MemoryImage*	mPanel;				// +0xb0 the tinted panel of the temples that are not the tutorial's
	GodFigure*		mGod;				// +0x88
	int				mTemple;			// the temple shown
	int				mFirst;				// +0xc8 the first puzzle in view (a multiple of the columns)
	int				mFirstUnsolved;		// +0xb4 -1: all solved
	int				mTicks;				// +0x4c
	int				mTextAlpha;			// +0x98
	// the scrolling of the puzzles (0x411200, 0x40fe30, 0x40fd50): the slider's value is the row at the top plus a fraction
	// (mScrollFrac, +0x9c, from -1 to 1); a held thumb gives the fraction, a key, the wheel or the plaque start a run of rows
	// that glide by 0.04 an update
	float			mScrollFrac;		// +0x9c
	int				mScrollDir;			// +0xa0 0 up, 1 down
	bool			mScrollActive;		// +0xa4
	int				mScrollDone;		// +0xa8 rows glided in this run
	int				mScrollTarget;		// +0xac rows to glide (-1 as far as there are any)

public:
	TempleScreen(PixelusApp* theApp, MemoryImage* theBackdrop, int theMode);	// 0x413ce0
	virtual ~TempleScreen();										// 0x413a90

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// slot 1
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// slot 2
	virtual void		Draw(Graphics* g) override;										// 0x412990 slot 16
	virtual void		Update() override;												// slot 17
	virtual void		MouseMove(int x, int y) override;								// slot 25
	virtual void		MouseDown(int x, int y, int theClickCount) override;			// 0x411600 slot 27
	using Widget::MouseDown;
	virtual void		MouseWheel(int theDelta) override;								// 0x40fd20 slot 32
	virtual void		KeyDown(KeyCode theKey) override;								// 0x40fd50 slot 21
	virtual void		ButtonDepress(int theId) override;								// 0x414510
	virtual void		ScrollPosition(int theId, double thePosition) override;			// 0x40fe30 (sub-object +0x5c)
	virtual bool		LineUp() override;												// 0x4100b0 (sub-object +0x60)
	virtual bool		LineDown() override;												// 0x4100c0
	virtual bool		PageUp() override;												// 0x4100d0
	virtual bool		PageDown() override;												// 0x4100e0

	void				BuildThumbs();									// 0x4138d0 (0x413420 each)
	int					FindFirstUnsolved();							// 0x413a00
	int					HitTest(int x, int y) const;					// 0x4102f0
	void				ScrollTo(double thePosition, bool theHeld);		// 0x40fe30: the scroll bar moved to thePosition rows
	void				UpdateScroll();									// 0x411313: the glide
	double				GetScrollValue() const { return mFirst / COLUMNS + mScrollFrac; }
	void				UpdateButtons();								// 0x410210
	void				DrawThumbs(Graphics* g);						// 0x4119e0
	void				SkipTutorial();									// 0x4114d0 asks; 2027 does it
};

}

#endif
