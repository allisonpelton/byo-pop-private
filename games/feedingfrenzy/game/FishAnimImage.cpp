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
		snprintf(aFileName, sizeof(aFileName), "%s%d%s", thePath, aNumber, anExtension);
		aNumber++;
		std::string aShiftedName = getShiftedName(std::string(thePath) + std::to_string(aNumber), *theShift);
		engine::Image* anImage = anApp->findImage(aShiftedName.c_str());
		if (anImage == nullptr)
		{
			if (!anApp->exists(aFileName))
				break;
			anImage = anApp->createImage(aFileName, false);
			anImage->applyFilter(*theShift);
			anApp->addImage(aShiftedName.c_str(), anImage);
		}
		engine::ImageNode* aNode = new engine::ImageNode(anImage);
		aNode->layout();
		addChild(aNode);
	}
	finishLoading();
}

// 0x41a6b0 and 0x40bf00: the parts are whole numbers, and each can come out negative before its absolute value
std::string FishAnimImage::getShiftedName(const std::string& theBase, const engine::HsvShiftFilter& theShift)
{
	float aSaturationPart = std::fabs((theShift.getSaturation() + SHIFT_NAME_OFFSET) * SHIFT_NAME_SCALE);
	float aHuePart = std::fabs(theShift.getHue() + HUE_NAME_OFFSET);
	float aValuePart = std::fabs((theShift.getValue() + SHIFT_NAME_OFFSET) * SHIFT_NAME_SCALE);
	char aSuffix[64];
	snprintf(aSuffix, sizeof(aSuffix), "_H_%d_S_%d_V_%d", (int)aHuePart, (int)aSaturationPart, (int)aValuePart);
	return theBase + aSuffix;
}

// 0x432550 and 0x426f10
engine::Image* FishAnimImage::getMenuImage(const std::string& theFile, engine::HsvShiftFilter* theShift)
{
	engine::App* anApp = engine::App::instance();
	engine::Image* anImage;
	if (theShift == nullptr)
		anImage = anApp->getImage(theFile.c_str());
	else
	{
		std::string aShiftedName = getShiftedName(theFile, *theShift);
		anImage = anApp->findImage(aShiftedName.c_str());
		if (anImage == nullptr)
		{
			anImage = anApp->createImage(theFile.c_str(), false);
			anImage->applyFilter(*theShift);
			anApp->addImage(aShiftedName.c_str(), anImage);
		}
	}
	anImage->setPivot(1);
	return anImage;
}
