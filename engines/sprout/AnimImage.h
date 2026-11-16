#ifndef __SPROUT_ANIMIMAGE_H__
#define __SPROUT_ANIMIMAGE_H__

// The signal build's animated picture (Pizza Frenzy's; 0x158 bytes, constructor 0x481e50 (pizzafrenzy), vtable
// 0x503780 (pizzafrenzy); its name is "AnimImage"): a selector over the pictures of a sequence that steps through them
// at a rate, once or in a loop. Word Harmony's build has none of it.

#include "Selector.h"
#include <string>

namespace engine
{

class AnimImage : public Selector
{
public:
	AnimImage();  // 0x481e50 (pizzafrenzy)

	std::string getName() override { return "AnimImage"; }  // 0x60: 0x482050 (pizzafrenzy)
	// 0x94: 0x481c80 (pizzafrenzy): the picture follows the time (rate pictures a second) between the first and the
	// last, which it ends on when it does not loop, and wraps round when it does
	void update(UpdateInfo& theInfo) override;

	// 0x140: 0x482080 (pizzafrenzy): the pictures <thePath><n><theExtension> from theFirst up to the first that does
	// not exist, as nodes; without an extension it is the one of the first picture (.jpg, else .png). It then
	// finishes loading, and gives the number of pictures.
	int load(const char* thePath, const char* theExtension, int theFirst);
	// 0x144: 0x482210 (pizzafrenzy): the first picture shows, and the bounds are its bounds
	void finishLoading();
	// 0x150: 0x481c10 (pizzafrenzy): pictures a second
	void setRate(float theRate) { mRate = theRate; }
	// 0x15c: 0x481c30 (pizzafrenzy)
	void play() { mPlaying = true; }
	// 0x164: 0x481c50 (pizzafrenzy)
	void setLoop(bool theLoop) { mLoop = theLoop; }
	// 0x178: 0x481e30 (pizzafrenzy): the whole sequence
	void resetRange();

private:
	float mRate;      // +0x13c: pictures a second (10)
	float mPosition;  // +0x140: where in the sequence it is (the picture is the whole part)
	bool mPlaying;    // +0x144
	bool mLoop;       // +0x145
	int mFirst;       // +0x148: the first picture it plays
	int mLast;        // +0x14c: the last
};

} // namespace engine

#endif
