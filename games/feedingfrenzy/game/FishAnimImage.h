#ifndef __FEEDINGFRENZY_FISHANIMIMAGE_H__
#define __FEEDINGFRENZY_FISHANIMIMAGE_H__

// A fish's animation (FishAnimImage, 0x14c bytes, vtable 0x49f5b0, destructor 0x445dd0): the engine's animated picture
// whose frames can be shifted in hue, saturation and value when the fish has a colour offset ("hsvOffset" of its
// file). Each shifted frame is made once and kept under its own name, like the shifted fish of a school.

#include "AnimImage.h"
#include "Graphics.h"
#include "ImageFilter.h"
#include <string>

namespace feedingfrenzy
{

class FishAnimImage : public engine::AnimImage
{
public:
	// 0x41a6b0 (the frames' path, their extension, the number of the first and the colour offset, or null): without
	// an offset it is the engine's load (0x446210); with one each frame <path><n><extension> is looked up as shifted
	// ("<path><n+1>_H_<hue+180>_S_<(saturation+1)*100>_V_<(value+1)*100>"), made by shifting the file's picture when it
	// is not there yet, and added to the animation. The shift ends the way a plain load does (the first picture
	// shows, the bounds are its bounds).
	void loadShifted(const char* thePath, const char* theExtension, int theFirst, engine::HsvShiftFilter* theShift);

	// The name a picture is kept under when the colour offset has shifted it: "<theBase>_H_<hue+180>_S_<(saturation+1)*100>_V_<(value+1)*100>"
	static std::string getShiftedName(const std::string& theBase, const engine::HsvShiftFilter& theShift);

	// 0x432550 and 0x426f10: a fish's menu picture as the stage's HUD and its end screen show it, centred on its pivot:
	// the file's picture, or with a colour offset the shifted one (made from the file and kept under its shifted name
	// when it is not there yet)
	static engine::Image* getMenuImage(const std::string& theFile, engine::HsvShiftFilter* theShift);
};

} // namespace feedingfrenzy

#endif
