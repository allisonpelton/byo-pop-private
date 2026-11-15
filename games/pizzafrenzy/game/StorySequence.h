#ifndef __PIZZAFRENZY_STORYSEQUENCE_H__
#define __PIZZAFRENZY_STORYSEQUENCE_H__

// A story (Iggle Pop's StorySequence, 0x4c bytes, constructor 0x44dcb0, vtable 0x4ffbbc) and its actions (Iggle Pop's
// StoryAction, vtable 0x4ffa54), read by the story file's handlers from res/story/*.xml: a sequence of actions that
// the story screen (StoryScreen.h) plays one after the other. An action starts, may be told what the player pressed,
// and is done when the next one may start: a line of a character's speech, an event for the screen, a picture, a
// sound, a pause, or a wait for a press.

#include "DocHandler.h"
#include "Graphics.h"
#include "Node.h"
#include "Object.h"
#include "ScreenLayout.h"
#include "TextTyper.h"

#include <string>
#include <vector>

namespace pizzafrenzy
{

class StoryScreen;
class StorySequence;

class StoryAction : public engine::Object
{
public:
	~StoryAction() override {}  // 0x44d1a0: lets go of the sequence

	// 0x04: pure in the base (0x4ac97d)
	virtual void start() = 0;
	// 0x08: pure in the base
	virtual void stop() = 0;
	// 0x0c: 0x44d240: the sequence it plays in (counted); the sequence says it before it starts the action
	virtual void setSequence(StorySequence* theSequence);
	// 0x10: 0x492310: while it is the current action of a running sequence
	virtual void update(engine::UpdateInfo& theInfo) {}
	// 0x14: pure in the base
	virtual bool isDone() = 0;
	// 0x18: 0x492310: what the player pressed (an action of the screen, "fastForwardStep" and "continue")
	virtual void action(const std::string& theAction) {}

protected:
	engine::Ref<StorySequence> mSequence;  // +0x0c
};

class StorySequence : public engine::Object
{
public:
	StorySequence();            // 0x44dcb0
	~StorySequence() override;  // 0x44dbc0, 0x44dd40

	// 0x44d360: the screen the actions play on (counted); nothing plays yet
	void setScreen(StoryScreen* theScreen);
	// 0x44d630: the first action starts
	void start();
	// 0x44d680: the current action stops, and the sequence no longer runs
	void stop();
	// 0x44d6d0: the next action starts; after the last, the sequence is done
	void next();
	// 0x44d730: what the player pressed goes to the current action
	void action(const std::string& theAction);
	// 0x44d780: the current action is updated, and the next starts when it is done
	void update(engine::UpdateInfo& theInfo);
	// 0x44d8a0: the actions and the screen are let go
	void clear();
	// 0x4797f0: the last action is over
	bool isDone() const { return mDone; }
	// 0x44e160: an action goes at the end (the story file's handlers add them)
	void addAction(StoryAction* theAction);

	// What the actions ask of the screen
	// 0x44d3b0: the speaker's portrait and bubble show, and no other's; the text of the speaker (null for a name that
	// is no speaker's)
	engine::TextTyper* showSpeaker(const std::string& theSpeaker);
	// 0x44d3c0: an event of the screen (StoryScreen::handleEvent)
	void handleEvent(const std::string& theEvent, const std::string& theParam);
	// 0x44d3d0: the node of the screen takes the image as its only child and fades in, or fades out for none
	void setNodeImage(const std::string& theNode, engine::Node* theImage);
	// 0x44d450: the node takes the picture of that name (the picture of the current city for "currentCity"), or fades
	// out for a name with no picture
	void showImage(const std::string& theNode, const std::string& theImage);

	std::string mName;  // +0x14: the name the file gives it ("No Name" for none); nothing reads it

private:
	// 0x43ca00: the picture of a story's image action: the current city's postcard, or a picture of the archive with
	// its pivot at the centre; null for no name
	engine::Image* findImage(const std::string& theName);

	int mCurrent;   // +0x0c: the index of the current action
	bool mRunning;  // +0x10: the current action has started and the sequence has not ended
	bool mDone;     // +0x11: the last action ended while it ran
	std::vector<engine::Ref<StoryAction>> mActions;  // +0x34
	engine::Ref<StoryScreen> mScreen;                // +0x40
};

// The handler of a story file's elements (Iggle Pop's StorySequence::StorySequenceDocHandler, 0x40 bytes, constructor
// 0x44d300, vtable 0x4ffc54 and, for its XML handler part, 0x4ffba8): a "sequence" element makes the sequence, and
// each element in it an action, whose handler goes on the stack for the element
class StorySequenceDocHandler : public engine::BaseDocHandler
{
public:
	StorySequenceDocHandler(const std::string& theName, engine::DocHandlerStack* theParent) :
		engine::BaseDocHandler(theName, theParent)
	{
	}  // 0x44d300

	// 0x44dd60
	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override;

	// +0x34: the sequence the file made
	engine::Ref<StorySequence> mSequence;
};

} // namespace pizzafrenzy

#endif
