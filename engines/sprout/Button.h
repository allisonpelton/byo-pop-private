#ifndef __SPROUT_BUTTON_H__
#define __SPROUT_BUTTON_H__

// Buttons. The base (0xfc bytes, constructor 0x432600 (wordharmony), vtable 0x484408 (wordharmony); 0x17c bytes,
// constructor 0x47ad40 (pizzafrenzy), vtable 0x502f80 (pizzafrenzy)) grows towards a hover scale while the mouse is
// over it and back when it leaves, can flash (its opacity going down and up), plays a sound when the mouse enters and
// one when it is pressed, and tells its listeners, each under the name it was added with. The image button (0x100
// bytes, constructor 0x429c70 (wordharmony), vtable 0x483810 (wordharmony); 0x188 bytes, constructor
// 0x478c90 (pizzafrenzy) (pizzafrenzy), vtable 0x502bb0 (pizzafrenzy)) shows its up, over and down images through a
// selector.
//
// The signal build's button takes the mouse as events (Node.h) and keeps a state instead of a pressed flag: up, over,
// down, and down with the mouse gone off it (STATE_*). A press captures the mouse in the scene until it is let go.
// The listener's five calls are the enter, the exit, the down, the up (also when the mouse leaves a pressed button)
// and the click (the release over the button); the click is the earlier build's press, which the screen sends its
// action for.

#include "Geometry.h"
#include "Node.h"
#include <string>
#include <vector>

namespace engine
{

class Image;
class Selector;
class Sound;

// A button's listener (vtable slots 0, 4 and 8); the argument is the name the listener was added under
class ButtonListener : public virtual Interface
{
public:
	virtual void buttonEnter(std::string theName) = 0;  // slot 0x00 (both builds)
	virtual void buttonPress(std::string theName) = 0;  // slot 0x04; the signal build's slot 0x10, the click
	virtual void buttonExit(std::string theName) = 0;   // slot 0x08; the signal build's slot 0x04
	// The signal build's slots 0x08 and 0x0c, which the screen leaves alone (0x496410 (pizzafrenzy), nothing)
	virtual void buttonDown(std::string theName) {}
	virtual void buttonUp(std::string theName) {}
};

class Button : public Container
{
public:
	Button();             // 0x432600 (wordharmony), 0x47ad40 (pizzafrenzy)
	~Button() override;   // 0x432040 (wordharmony)

	// 0x70: 0x431c10 (wordharmony); 0x94: 0x47a150 (pizzafrenzy)
	void update(UpdateInfo& theInfo) override;
	// The signal build's: the mouse goes to the button only while its screen is connected (0x88:
	// 0x47a4c0 (pizzafrenzy), 0x8c: 0x47a490 (pizzafrenzy), which also lets go of a press)
	void connect() override;
	void disconnect() override;
	// The signal build's mouse events (0xa0: 0x47a4d0 (pizzafrenzy), 0xac: 0x47a520 (pizzafrenzy), 0xd8:
	// 0x47a5d0 (pizzafrenzy), 0xe4: 0x47a670 (pizzafrenzy)); the move is ignored (0xd0: 0x492310 (pizzafrenzy))
	void mouseDownEvent(MouseEvent& e) override;
	void mouseUpEvent(MouseEvent& e) override;
	void mouseEnterEvent(MouseEvent& e) override;
	void mouseExitEvent(MouseEvent& e) override;
	void mouseMoveEvent(MouseEvent& e) override {}
	// 0xdc: 0x431b20 (wordharmony); 0x130: 0x47a080 (pizzafrenzy)
	virtual void setNormalScale(float x, float y);
	// 0xe0: 0x431b90 (wordharmony); 0x134: 0x47a0e0 (pizzafrenzy)
	virtual void setHoverScale(float x, float y);
	// 0xe4: 0x431c00 (wordharmony); 0x138: 0x47a140 (pizzafrenzy), the seconds the growing and shrinking take
	virtual void setScaleTime(float theTime) { mScaleTime = theTime; }
	// 0xe8: 0x4327a0 (wordharmony); 0x13c: 0x47aeb0 (pizzafrenzy) (the listener is not counted)
	virtual void addListener(ButtonListener* theListener, std::string theName);
	// 0xec: 0x431f60 (wordharmony); 0x140: 0x47a730 (pizzafrenzy), loads the two sounds by their names (an empty name
	// is none)
	virtual void setSounds(const std::string& theEnterSound, const std::string& thePressSound);
	// 0xf4: 0x431ef0 (wordharmony); 0x148: 0x47a420 (pizzafrenzy), the pivot mode the image button gives its images
	// (Image::setPivot)
	virtual void setPivotMode(int theMode) { mPivotMode = theMode; }
	// The signal build's 0x12c: 0x47a400 (pizzafrenzy), the scale the hover and normal scales multiply, which is also
	// the button's scale now
	virtual void setBaseScale(float x, float y);
	// The signal build's 0x14c: 0x4d0470 (pizzafrenzy), 0x478bc0 (pizzafrenzy) in the image button,
	// 0x495730 (pizzafrenzy) in the check box: the images follow the state
	virtual void showState() {}
	// The signal build's 0x160: 0x47aa70 (pizzafrenzy), the release over the button; the check box flips first
	virtual void click() { notifyListeners(NOTIFY_PRESS); }

protected:
	// The signal build's states (+0x140 (pizzafrenzy))
	enum
	{
		STATE_UP = 0,
		STATE_OVER = 1,
		STATE_OUT_DOWN = 2,  // pressed, and the mouse has gone off the button
		STATE_DOWN = 3
	};
	// What notifyListeners tells: the earlier build's press is the signal build's click
	enum
	{
		NOTIFY_ENTER = 0,  // 0x150: 0x47a8f0 (pizzafrenzy)
		NOTIFY_PRESS = 1,  // 0x160: 0x47aa70 (pizzafrenzy)
		NOTIFY_EXIT = 2,   // 0x154: 0x47a950 (pizzafrenzy)
		NOTIFY_DOWN = 3,   // 0x158: 0x47a9b0 (pizzafrenzy)
		NOTIFY_UP = 4      // 0x15c: 0x47aa10 (pizzafrenzy)
	};

	void onMouseDown(Point& p) override;   // 0x80: 0x432300 (wordharmony)
	void onMouseUp(Point& p) override;     // 0x88: 0x431b10 (wordharmony)
	void onMouseEnter(Point& p) override;  // 0xa4: 0x432330 (wordharmony)
	void onMouseExit(Point& p) override;   // 0xac: 0x4323a0 (wordharmony)

	// 0x432180 (wordharmony): NOTIFY_*
	void notifyListeners(int theEvent);

	struct ListenerEntry
	{
		ButtonListener* mListener;  // +0x00
		std::string mName;          // +0x04
	};

	enum
	{
		SCALE_HOVER = 0,    // at the hover scale
		SCALE_GROWING = 1,
		SCALE_SHRINKING = 2,
		SCALE_NORMAL = 3
	};

public:
	bool mPressed;                            // +0xa8
	std::vector<ListenerEntry*> mListeners;   // +0xac
	Sound* mEnterSound;                       // +0xbc
	Sound* mPressSound;                       // +0xc0
	bool mScaling;                            // +0xc4: the normal and hover scales differ
	Point mNormalScale;                       // +0xc8
	Point mHoverScale;                        // +0xd0
	Point mBaseScale;                         // +0xd8
	float mScaleTimeLeft;                     // +0xe0
	float mScaleTime;                         // +0xe4
	int mScaleState;                          // +0xe8
	float mFlashTimeLeft;                     // +0xec
	float mFlashPeriod;                       // +0xf0
	bool mFlashUp;                            // +0xf4: the opacity rises with the time left
	bool mFlashing;                           // +0xf5
	int mPivotMode;                           // +0xf8
	int mState;  // +0x140 (pizzafrenzy): STATE_*
};

class ImageButton : public Button
{
public:
	ImageButton();  // 0x429c70 (wordharmony), 0x478c90 (pizzafrenzy)

	void mouseMove(Point& p) override {}  // 0x9c: 0x429d50 (wordharmony)
	// 0xf8: 0x429d90 (wordharmony); 0x16c: 0x478900 (pizzafrenzy), the up image (needed) and the over and down ones
	// (each defaults to the one before in the earlier build; in the signal build a missing over image is the up image
	// lightened by 0.3, and a missing down image is the over image, or the up image lightened by 0.4 when there is no
	// over one)
	virtual bool setImages(Image* theUp, Image* theOver, Image* theDown);
	// 0x14c: 0x478bc0 (pizzafrenzy)
	void showState() override;

protected:
	void onMouseDown(Point& p) override;   // 0x80: 0x429cd0 (wordharmony)
	void onMouseEnter(Point& p) override;  // 0xa4: 0x429cf0 (wordharmony)
	void onMouseExit(Point& p) override;   // 0xac: 0x429d30 (wordharmony)

public:
	Selector* mSelector;  // +0xfc (a child); +0x17c (pizzafrenzy)
};

// A check box (0x108 bytes, built inline by the layout reader, vtable 0x483dc8 (wordharmony); 0x18c bytes, built inline
// by the layout reader, vtable 0x503ca0 (pizzafrenzy)): four images, unchecked and checked, each with its over image; a
// press flips it (the signal build's, its click)
class CheckBox : public Button
{
public:
	CheckBox();

	void mouseMove(Point& p) override {}  // 0x9c: 0x429d50 (wordharmony)
	// 0xf8: 0x434ce0 (wordharmony); 0x16c: 0x4954a0 (pizzafrenzy), the unchecked and checked images are needed; an over
	// image defaults to its plain one. The signal build's gives the images the pivot mode.
	virtual bool setImages(Image* theUnchecked, Image* theUncheckedOver, Image* theChecked, Image* theCheckedOver);
	// 0xfc: 0x42c1e0 (wordharmony); 0x170: 0x4957e0 (pizzafrenzy)
	virtual bool isChecked() { return mChecked; }
	// 0x100: 0x435020 (wordharmony); 0x174: 0x4957f0 (pizzafrenzy)
	virtual void setChecked(bool theChecked);
	// 0x104: 0x434ff0 (wordharmony), shows the plain image of the state; 0x178: 0x495770 (pizzafrenzy), the signal
	// build's shows the over image while the mouse is over the button, and the checked ones when it is checked
	virtual void updateImage();
	// 0x14c: 0x495730 (pizzafrenzy)
	void showState() override { updateImage(); }
	// 0x160: 0x495740 (pizzafrenzy)
	void click() override;

protected:
	void onMouseDown(Point& p) override;   // 0x80: 0x434f10 (wordharmony)
	void onMouseEnter(Point& p) override;  // 0xa4: 0x434f60 (wordharmony)
	void onMouseExit(Point& p) override;   // 0xac: 0x434fa0 (wordharmony)

public:
	bool mChecked;        // +0xfc; +0x17c (pizzafrenzy)
	Selector* mSelector;  // +0x100 (a child); +0x180 (pizzafrenzy)
};

} // namespace engine

#endif
