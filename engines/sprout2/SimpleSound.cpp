#include "SimpleSound.h"

using namespace simplesound;

namespace
{

SimpleSound* gSimpleSound = nullptr;  // 0x5ac648 (feedingfrenzy2)

} // namespace

SimpleSound::SimpleSound()
{
	for (int i = 0; i < CATEGORY_COUNT; i++)
		mCategoryVolumes[i] = 1.0f;
}

void SimpleSound::setCategoryVolume(int theCategory, float theVolume)
{
	if ((unsigned)theCategory < (unsigned)CATEGORY_COUNT)
	{
		float aVolume = theVolume > 1.0f ? 1.0f : theVolume;
		mCategoryVolumes[theCategory] = aVolume >= 0.0f ? aVolume : 0.0f;
	}
	applyVolumes();
}

float SimpleSound::getCategoryVolume(int theCategory)
{
	if ((unsigned)theCategory < (unsigned)CATEGORY_COUNT)
		return mCategoryVolumes[theCategory];
	return 1.0f;
}

SimpleSound* simplesound::getSimpleSound()
{
	return gSimpleSound;
}

void simplesound::setSimpleSound(SimpleSound* theSimpleSound)
{
	gSimpleSound = theSimpleSound;
}
