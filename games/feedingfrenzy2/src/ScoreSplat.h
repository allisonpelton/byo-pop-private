#ifndef __FEEDINGFRENZY2_SCORESPLAT_H__
#define __FEEDINGFRENZY2_SCORESPLAT_H__

// The HUD's points bubbles. A bubble is a text that flies from where the points were scored to the score's text,
// first jumping up and aside, growing as it rises and shrinking as it lands, and on landing adds its points to the
// score shown. The bubbles are queued and shown one after another, a points bubble at most every 0.09 seconds and
// any other text every 0.3.

#include "Container.h"
#include "Font.h"
#include "KeyInterpolator.h"
#include "TextItem.h"
#include "TimeDriver.h"
#include "sigslot.h"

#include <deque>
#include <string>

class GameHud;

// A text flying to the HUD's score (0x214 bytes, vtables 0x556f04 and 0x556ffc (feedingfrenzy2); its constructor
// 0x47a3c0 (feedingfrenzy2)). Its update alone moves it: neither its modifiers nor its children's are run.
class ScoreSplat : public engine::TextItem
{
public:
	// 0x47a3c0 (feedingfrenzy2): a text item with no keys, stopped
	ScoreSplat();

	// 0xa4 (0x433b30 (feedingfrenzy2)): the flight's time advanced and the text placed; once it lands its points
	// signalled and the text taken out at the next update
	void update(engine::UpdateContext& theContext) override;

	// 0x4302f0 (feedingfrenzy2): the text centred and in a font by its points (from the shell's theme), measured in the
	// font given; a flight from the start (its x kept on the screen) by a jump of up to 60 pixels aside and 80 up to
	// the end, at the speed in pixels a second
	void setup(const std::string& theText, engine::Font* theMeasureFont, float theSpeed, int thePoints,
		const engine::Vector2f& theFrom, const engine::Vector2f& theTo);
	// 0x416020 (feedingfrenzy2): the text centred, its shadow a pixel down and right at 0.9, in the font, rising 90
	// pixels from the point over the first quarter of a second and a half, growing from half its size as it does,
	// then held there fading out; no points
	void setupRising(const std::string& theText, engine::Font* theFont, const engine::Vector2f& thePoint);
	// 0x47cee0 (feedingfrenzy2): added to its container, placed and updated every frame
	void show();
	void setContainer(engine::Container* theContainer) { mContainer = theContainer; }
	int getPoints() const { return mPoints; }

	sigslot::signal1<int> mLandedSignal;        // +0xe8: the points, as the text lands

private:
	// 0x435720 (feedingfrenzy2): the position, scale and opacity at the flight's time
	void place();

	engine::Vector2KeyInterpolator mPath;       // +0xf8
	engine::FloatKeyInterpolator mScaleKeys;    // +0x144
	engine::FloatKeyInterpolator mOpacityKeys;  // +0x190
	engine::Container* mContainer;              // +0x1dc: the HUD's points bubbles' container (the HUD's +0x240)
	engine::TimeDriver mFlight;                 // +0x1e0: from 0 to 1 over the flight
	int mPoints;                                // +0x208
};

// The queues of texts shown one after another (the global 0x5acc20 (feedingfrenzy2), vtable 0x557014): the points
// bubbles in one, the other texts in the other, each with the time until its next may be shown
class ScoreSplatQueue
{
public:
	// 0x4194c0 (feedingfrenzy2): shown at once when its queue is empty and its wait over, else queued
	void push(ScoreSplat* theSplat);
	// 0x4301d0 (feedingfrenzy2), from the HUD's update: each wait counted down, and when over the next text in its
	// queue shown
	void update(float theSeconds);
	// 0x498bf0 (feedingfrenzy2), from the HUD's reset: both queues emptied
	void clear();

private:
	float mPointsWait = 0.0f;                         // +0x0c
	float mOthersWait = 0.0f;                         // +0x10
	std::deque<engine::Ref<ScoreSplat>> mPoints;      // +0x14
	std::deque<engine::Ref<ScoreSplat>> mOthers;      // +0x28
};

// The global 0x5acc20 (feedingfrenzy2)
ScoreSplatQueue& scoreSplatQueue();

// 0x427250 (feedingfrenzy2): a points bubble ("+%d") from the point on the screen to the HUD's score at 400 pixels a
// second, coloured by the frenzy's multiplier (from 1), its landing adding to the score shown, then queued
void addPointsBubble(const engine::Vector2f& theFrom, GameHud* theHud, int thePoints, int theMultiplier);

#endif
