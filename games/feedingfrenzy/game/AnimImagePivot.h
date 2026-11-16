#ifndef __FEEDINGFRENZY_ANIMIMAGEPIVOT_H__
#define __FEEDINGFRENZY_ANIMIMAGEPIVOT_H__

// The engine's AnimImage has a function in this build (slot 0xf8, 0x4461c0) that the shared AnimImage.h does not
// have: every picture of the animation gets a pivot (the image's, 1 for the centre), and each is laid out again.

#include "AnimImage.h"
#include "ImageNode.h"

namespace feedingfrenzy
{

// 0x4461c0
inline void setAnimationPivot(engine::AnimImage* theAnimation, int thePivot)
{
	for (size_t aFrame = 0; aFrame < theAnimation->mChildren.size(); aFrame++)
	{
		engine::ImageNode* aNode = static_cast<engine::ImageNode*>(theAnimation->mChildren[aFrame]);
		aNode->mImage->setPivot(thePivot);
		aNode->layout();
	}
}

} // namespace feedingfrenzy

#endif
