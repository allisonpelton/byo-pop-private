#ifndef __FEEDINGFRENZY_WAVEFX_H__
#define __FEEDINGFRENZY_WAVEFX_H__

// The water's waves on a stage's background (engine::WaveFx, a blit effect)

#include "BltFx.h"

namespace feedingfrenzy
{

class WaveFx : public engine::BltFx
{
public:
	void advance(float theSeconds);
};

} // namespace feedingfrenzy

#endif
