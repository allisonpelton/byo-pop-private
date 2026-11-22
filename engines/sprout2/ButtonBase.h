#ifndef __SPROUT2_BUTTONBASE_H__
#define __SPROUT2_BUTTONBASE_H__

// What every button is built on (ButtonBase, vtable 0x55f034 (feedingfrenzy2) with 87 slots; its constructor 0x482850
// (feedingfrenzy2)): a container that follows the mouse through four states, tells its listeners of each change with
// the action it was given for them, and plays a sound as the mouse comes onto it and as it is pressed. A button may
// grow between two scales as the mouse comes on and goes off it, and may flash after the mouse leaves. How a state
// looks is the subclass's (updateLook).

#include "Container.h"

namespace engine
{

class IButtonListener;

// The button's states (+0xd0)
enum ButtonState
{
	BUTTON_NORMAL = 0,     // the mouse is elsewhere
	BUTTON_OVER = 1,       // the mouse is over it
	BUTTON_PRESSED_OUT = 2,  // pressed, and the mouse dragged off it
	BUTTON_DOWN = 3        // pressed, with the mouse over it
};

// Where a button's scale is between its normal and its high scale (+0xf8)
enum ButtonScalePhase
{
	SCALE_HIGH = 0,       // at the high scale
	SCALE_GROWING = 1,    // from the normal scale to the high one, the mouse having come on
	SCALE_SHRINKING = 2,  // back to the normal scale, the mouse having gone
	SCALE_NORMAL = 3      // at the normal scale
};

class ButtonBase : public Container
{
public:
	// 0x482850 (feedingfrenzy2): normal, every scale 1, half a second to grow, not flashing
	ButtonBase();

	// 0x90 (0x47d220 (feedingfrenzy2)): the mouse taken again (the children are not enabled)
	void enable() override { removeOffState(STATE_INPUT); }
	// 0x94 (0x41f590 (feedingfrenzy2)): back to normal, and the mouse no longer taken
	void disable() override;
	// 0xa4 (0x438200 (feedingfrenzy2)): the scale and the flash moved on, then the children updated
	void update(UpdateContext& theContext) override;
	// 0xb4 (0x40e8b0), 0xb8 (0x40e900), 0xcc (0x4171c0), 0xd0 (0x40e960) and 0xd4 (0x49f440) (feedingfrenzy2): a press
	// takes the scene's mouse until the release, which is an action when it comes over the button
	void mouseDown(MouseEvent& theEvent) override;
	void mouseUp(MouseEvent& theEvent) override;
	void mouseEnter(MouseEvent& theEvent) override;
	void mouseLeave(MouseEvent& theEvent) override;
	void mouseCancel(MouseEvent& theEvent) override;

	// 0x118 (0x408c50 (feedingfrenzy2)): the scale the normal and high scales multiply, set as the scale at once
	virtual void setBaseScale(float theScaleX, float theScaleY);
	// 0x11c (0x48b950) and 0x120 (0x48b900) (feedingfrenzy2): the scales the button grows between, as two floats
	virtual void setNormalScale(Vector2f theScale);
	virtual void setHighScale(Vector2f theScale);
	// 0x124 (0x48b9a0 (feedingfrenzy2)): the seconds the growing and shrinking take
	virtual void setScaleTime(float theTime) { mScaleTime = theTime; }
	// 0x128 (0x472630 (feedingfrenzy2)): a listener told of every change, with the action
	virtual void addListener(IButtonListener* theListener, std::string theAction);
	// (0x12c (0x500be4 (feedingfrenzy2)), the two sounds' instances set directly, is not called in feedingfrenzy2)
	// 0x130 (0x481510 (feedingfrenzy2)): the sounds of the mouse coming on and of a press, by name (an empty name leaves
	// the sound as it is)
	virtual void setSounds(const std::string& theEnterSound, const std::string& theDownSound);
	// 0x134 (0x500b87 (feedingfrenzy2)): the seconds of each half of the flash after the mouse leaves; 0 for none
	virtual void setFlashPeriod(float thePeriod);
	// 0x138 (0x41b190 (feedingfrenzy2)): the pivot (Pivot) of the images a subclass is given
	virtual void setPivot(int thePivot) { mPivot = thePivot; }
	// 0x13c: the state's look
	virtual void updateLook() = 0;
	// 0x140 to 0x150 (0x41dea0, 0x41cf00, 0x41b110, 0x41b090, 0x418d40 (feedingfrenzy2)): each listener told, in the
	// order they were added
	virtual void notifyEnter();
	virtual void notifyLeave();
	virtual void notifyDown();
	virtual void notifyUp();
	virtual void notifyAction();
	// 0x154 (0x500bd4 (feedingfrenzy2)): the flash at a point, 1 the start of a half and 0 its end
	virtual void applyFlash(float theFlash) { setOpacity(theFlash); }
	// (0x158 (0x500bc8 (feedingfrenzy2)), the flash's look taken away, is not called in feedingfrenzy2)

protected:
	// 0x4a3160 (feedingfrenzy2): the state changed, and its look with it
	void setState(int theState);
	// 0x402c70 (feedingfrenzy2): the flash begun, if the button has one
	void startFlash();

	// One listener and its action (8 bytes and a string; the original allocates each)
	struct Listener
	{
		IButtonListener* mListener;  // +0x00
		std::string mAction;         // +0x04
	};

	std::vector<Listener> mListeners;  // +0xb8
	// +0xc8 and +0xcc: the sounds' instances, from the engine's sounds (its slots 0x7c and 0x84, the name, 1 and 0):
	// the sounds are not yet translated, so the port keeps the names
	std::string mEnterSound;
	std::string mDownSound;
	int mState;                     // +0xd0: ButtonState
	bool mScaling;                  // +0xd4: the normal and high scales differ
	Vector2f mNormalScale;          // +0xd8
	Vector2f mHighScale;            // +0xe0
	Vector2f mBaseScale;            // +0xe8
	float mScaleTimer;              // +0xf0: the seconds left of growing or shrinking
	float mScaleTime;               // +0xf4
	int mScalePhase;                // +0xf8: ButtonScalePhase
	float mFlashTimer;              // +0xfc: the seconds left of this half of the flash
	float mFlashPeriod;             // +0x100
	bool mFlashIn;                  // +0x104: this half goes from 1 down to 0 (the other from 0 up to 1)
	bool mFlashing;                 // +0x105
	int mPivot;                     // +0x108: Pivot
};

// What a button tells (IButtonListener, vtable 0x55c7e0 (feedingfrenzy2)), each with the action it was added with
class IButtonListener : public virtual Interface
{
public:
	// 0x00 to 0x0c: the mouse came onto the button, left it, pressed it and let it go
	virtual void buttonEnter(const std::string& theAction) = 0;
	virtual void buttonLeave(const std::string& theAction) = 0;
	virtual void buttonDown(const std::string& theAction) = 0;
	virtual void buttonUp(const std::string& theAction) = 0;
	// 0x10: the button clicked
	virtual void buttonAction(const std::string& theAction) = 0;
};

} // namespace engine

#endif
