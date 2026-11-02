// Sexy::TwistSlider (0xb8 bytes; 0x7561fd-0x756249): the options' volume sliders. The track is drawn as the
// framework's is, three cels side by side (the middle one tiled), with the fill over it up to the thumb.
#pragma once

#include "Slider.h"

namespace Sexy {

class TwistSlider : public Slider {
public:
	TwistSlider(Image* theTrackImage, Image* theThumbImage, int theId, SliderListener* theListener);  // 0x7561fd
	~TwistSlider() override;  // 0x75622a

	void Draw(Graphics* g) override;  // 0x756249
};

}  // namespace Sexy
