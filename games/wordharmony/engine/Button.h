#ifndef __WORDHARMONY_ENGINE_BUTTON_H__
#define __WORDHARMONY_ENGINE_BUTTON_H__

// Buttons. The base (0xfc bytes, constructor 0x432600, vtable 0x484408) grows towards a hover scale while the mouse is
// over it and back when it leaves, can flash (its opacity going down and up), plays a sound when the mouse enters and
// one when it is pressed, and tells its listeners, each under the name it was added with. The image button (0x100
// bytes, constructor 0x429c70, vtable 0x483810) shows its up, over and down images through a selector.

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
	virtual void buttonEnter(std::string theName) = 0;  // slot 0x00
	virtual void buttonPress(std::string theName) = 0;  // slot 0x04
	virtual void buttonExit(std::string theName) = 0;   // slot 0x08
};

class Button : public Container
{
public:
	Button();             // 0x432600
	~Button() override;   // 0x432040

	void update(UpdateInfo& theInfo) override;  // 0x70: 0x431c10
	// 0xd8: 0x431e90, the scale the normal and hover scales multiply
	virtual void setBaseScale(float x, float y);
	// 0xdc: 0x431b20
	virtual void setNormalScale(float x, float y);
	// 0xe0: 0x431b90
	virtual void setHoverScale(float x, float y);
	// 0xe4: 0x431c00, the seconds the growing and shrinking take
	virtual void setScaleTime(float theTime) { mScaleTime = theTime; }
	// 0xe8: 0x4327a0 (the listener is not counted)
	virtual void addListener(ButtonListener* theListener, std::string theName);
	// 0xec: 0x431f60, loads the two sounds by their names (an empty name is none)
	virtual void setSounds(const std::string& theEnterSound, const std::string& thePressSound);
	// 0xf0: 0x431f00, the seconds of one flash; 0 stops flashing
	virtual void setFlash(float thePeriod);
	// 0xf4: 0x431ef0, the pivot mode the image button gives its images (Image::setPivot)
	virtual void setPivotMode(int theMode) { mPivotMode = theMode; }

protected:
	void onMouseDown(Point& p) override;   // 0x80: 0x432300
	void onMouseUp(Point& p) override;     // 0x88: 0x431b10
	void onMouseEnter(Point& p) override;  // 0xa4: 0x432330
	void onMouseExit(Point& p) override;   // 0xac: 0x4323a0

	// 0x432180: 0 enter, 1 press, 2 exit
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
};

class ImageButton : public Button
{
public:
	ImageButton();  // 0x429c70

	void mouseMove(Point& p) override {}  // 0x9c: 0x429d50
	// 0xf8: 0x429d90, the up image (needed) and the over and down ones (each defaults to the one before)
	virtual bool setImages(Image* theUp, Image* theOver, Image* theDown);

protected:
	void onMouseDown(Point& p) override;   // 0x80: 0x429cd0
	void onMouseEnter(Point& p) override;  // 0xa4: 0x429cf0
	void onMouseExit(Point& p) override;   // 0xac: 0x429d30

public:
	Selector* mSelector;  // +0xfc (a child)
};

} // namespace engine

#endif
