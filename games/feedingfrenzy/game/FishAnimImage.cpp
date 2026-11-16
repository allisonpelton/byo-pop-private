#include "FishAnimImage.h"
#include "App.h"
#include "ImageNode.h"
#include <cmath>
#include <cstdio>

using namespace feedingfrenzy;

namespace
{
// The numbers a shifted picture's name carries (0x41a6b0): the hue turned half a circle, the saturation and the value
// each one more and times a hundred
const float HUE_NAME_OFFSET = 180.0f;
const float SHIFT_NAME_OFFSET = 1.0f;
const float SHIFT_NAME_SCALE = 100.0f;
const char* const DEFAULT_EXTENSION = ".png";
}

// 0x41a6b0
void FishAnimImage::loadShifted(const char* thePath, const char* theExtension, int theFirst,
                                engine::HsvShiftFilter* theShift)
{
	if (theShift == nullptr)
	{
		load(thePath, theExtension, theFirst);
		return;
	}
	engine::App* anApp = engine::App::instance();
	const char* anExtension = theExtension == nullptr ? DEFAULT_EXTENSION : theExtension;
	int aNumber = theFirst;
	while (true)
	{
		char aFileName[512];
		char aShiftedName[512];
		snprintf(aFileName, sizeof(aFileName), "%s%d%s", thePath, aNumber, anExtension);
		aNumber++;
		float aSaturationPart = std::fabs((theShift->getSaturation() + SHIFT_NAME_OFFSET) * SHIFT_NAME_SCALE);
		float aHuePart = std::fabs(theShift->getHue() + HUE_NAME_OFFSET);
		float aValuePart = std::fabs((theShift->getValue() + SHIFT_NAME_OFFSET) * SHIFT_NAME_SCALE);
		snprintf(aShiftedName, sizeof(aShiftedName), "%s%d_H_%d_S_%d_V_%d", thePath, aNumber, (int)aHuePart,
			(int)aSaturationPart, (int)aValuePart);
		engine::Image* anImage = anApp->findImage(aShiftedName);
		if (anImage == nullptr)
		{
			if (!anApp->exists(aFileName))
				break;
			anImage = anApp->createImage(aFileName, false);
			anImage->applyFilter(*theShift);
			anApp->addImage(aShiftedName, anImage);
		}
		engine::ImageNode* aNode = new engine::ImageNode(anImage);
		aNode->layout();
		addChild(aNode);
	}
	finishLoading();
}
