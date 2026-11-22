#ifndef __SPROUT2_MESSAGEMOUSE_H__
#define __SPROUT2_MESSAGEMOUSE_H__

// The window's mouse (dxdriver::MessageMouse, the engine's +0x2a8 (feedingfrenzy2)), fed by the app's widget from the
// window's mouse messages: while acquired, a move only keeps where the cursor is, and once a frame (and before each
// button) its distance from the window's centre goes out as a motion and the cursor is put back at the centre.

#include "Mouse.h"

namespace engine
{

class MessageMouse : public Mouse
{
public:
	// 0x49ee60 (feedingfrenzy2): not acquired, no button held. Port: the centre is the display's (the original's is
	// set by code not yet read)
	MessageMouse(int theWidth, int theHeight);

	// 0x04 (0x4a1f00 (feedingfrenzy2)): the cursor hidden and clipped to the window, where it was kept, and put at the
	// window's centre; whether the mouse is acquired. Port: the framework has no clip, and the cursor is put back at
	// the centre each frame
	bool acquire() override;
	// 0x08 (0x4a1ed0 (feedingfrenzy2)): the cursor put back where it was, let out of the window and shown
	void release() override;
	// 0x0c (0x4ff067 (feedingfrenzy2)): the original asks Windows (GetAsyncKeyState); port: the buttons as the window's
	// messages left them
	bool isButtonDown(int theButton) override;

	// +0x50
	bool isAcquired() const { return mAcquired; }
	// The window's WM_MOUSEMOVE while acquired (0x424780 (feedingfrenzy2)): only where the cursor is kept. Port: told
	// of every move, as the cursor's place before it is acquired is the window's last
	void setPosition(const Vector2<int>& thePosition) { mPosition = thePosition; }
	// The window's button messages while acquired (0x48e2e0 (feedingfrenzy2) for a press): where the cursor is kept,
	// the motion so far sent, the button's state kept and the press or release told
	void buttonDown(const Vector2<int>& thePosition, int theButton);
	void buttonUp(const Vector2<int>& thePosition, int theButton);
	// 0x42b8c0 (feedingfrenzy2), from the frame (0x438700 (feedingfrenzy2)) while acquired and from the buttons: the
	// cursor's distance from the centre sent as a motion when it is not none, and the cursor put back at the centre
	void sendMotion();

private:
	// The buttons the window tells of
	static const int BUTTON_COUNT = 2;

	bool mAcquired;                  // +0x50
	Vector2<int> mPosition;          // +0x54: where the cursor is, in the window
	Vector2<int> mCentre;            // +0x5c: the window's centre
	Vector2<int> mSavedPosition;     // +0x64: where the cursor was as it was acquired (on the screen in the original;
	                                 // port: in the window)
	bool mButtonDown[BUTTON_COUNT];  // +0x6c
};

} // namespace engine

#endif
