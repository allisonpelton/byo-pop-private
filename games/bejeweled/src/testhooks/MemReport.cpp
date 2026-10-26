#include "MemReport.h"
#include "SexyAppBase.h"
#include "MemoryImage.h"
#include "DDImage.h"
#include "SDLSoundManager.h"
#include "platform/Audio.h"

#include <algorithm>
#include <cstdio>
#include <vector>


using namespace Sexy;

std::string DescribeHeaps(); // MemReportHeaps.cpp, apart since <windows.h> clashes with the framework's types

namespace
{

// The sounds' samples are the manager's own (protected); a pointer to the member, taken in a derived class, reads
// them from the app's manager
struct SoundSamples : public SDLSoundManager
{
	static size_t Bytes(SDLSoundManager* theManager, int* theCount)
	{
		std::shared_ptr<Platform::SoundData> (SDLSoundManager::*aSounds)[MAX_SOURCE_SOUNDS] = &SoundSamples::mSounds;
		size_t aBytes = 0;
		*theCount = 0;
		for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		{
			const std::shared_ptr<Platform::SoundData>& aData = (theManager->*aSounds)[i];
			if (aData == NULL)
				continue;
			aBytes += aData->Bytes();
			(*theCount)++;
		}
		return aBytes;
	}
};

struct ImageRow
{
	std::string mName;
	int mWidth, mHeight;
	size_t mBytes[6]; // bits, palette, alpha caches, HD bits, HD alpha caches, texture (an estimate)
};

const char* const kColumns[6] = {"bits", "palette", "alpha caches", "HD bits", "HD caches", "textures (est.)"};

size_t Total(const ImageRow& theRow, bool withTextures)
{
	size_t aTotal = 0;
	for (int i = 0; i < (withTextures ? 6 : 5); i++)
		aTotal += theRow.mBytes[i];
	return aTotal;
}

} // namespace

bool WriteMemReport(SexyAppBase* theApp, const std::string& theFileName)
{
	FILE* aFile = fopen(theFileName.c_str(), "w");
	if (aFile == NULL)
		return false;

	std::vector<ImageRow> aRows;
	size_t aSums[6] = {0, 0, 0, 0, 0, 0};
	for (MemoryImage* anImage : theApp->mMemoryImageSet)
	{
		size_t aPixels = (size_t) anImage->mWidth * anImage->mHeight;
		size_t aHDPixels = (size_t) anImage->mHDWidth * anImage->mHDHeight;
		ImageRow aRow;
		aRow.mName = anImage->mFilePath;
		DDImage* aDDImage = dynamic_cast<DDImage*>(anImage);
		if (aRow.mName.empty() && (aDDImage != NULL) && aDDImage->mIsScreen)
			aRow.mName = "(screen)";
		aRow.mWidth = anImage->mWidth;
		aRow.mHeight = anImage->mHeight;
		aRow.mBytes[0] = (anImage->mBits != NULL) ? aPixels * 4 : 0;
		aRow.mBytes[1] = ((anImage->mColorIndices != NULL) ? aPixels : 0) + ((anImage->mColorTable != NULL) ? 256 * 4 : 0);
		aRow.mBytes[2] = ((anImage->mNativeAlphaData != NULL) ? aPixels * 4 : 0) + ((anImage->mRLAlphaData != NULL) ? aPixels : 0) +
			((anImage->mRLAdditiveData != NULL) ? aPixels : 0);
		aRow.mBytes[3] = anImage->mHDBits.capacity() * 4;
		aRow.mBytes[4] = ((anImage->mHDNativeAlphaData != NULL) ? aHDPixels * 4 : 0) + ((anImage->mHDRLAlphaData != NULL) ? aHDPixels : 0) +
			((anImage->mHDRLAdditiveData != NULL) ? aHDPixels : 0);
		aRow.mBytes[5] = (anImage->mD3DData != NULL) ? ((aHDPixels > 0) ? aHDPixels : aPixels) * 4 : 0;
		for (int i = 0; i < 6; i++)
			aSums[i] += aRow.mBytes[i];
		aRows.push_back(aRow);
	}
	std::sort(aRows.begin(), aRows.end(), [](const ImageRow& a, const ImageRow& b) { return Total(a, true) > Total(b, true); });

	const double MB = 1024.0 * 1024.0;
	fprintf(aFile, "images: %d\n", (int) aRows.size());
	for (int i = 0; i < 6; i++)
		fprintf(aFile, "  %-16s %8.2f MB\n", kColumns[i], aSums[i] / MB);
	for (const ImageRow& aRow : aRows)
	{
		fprintf(aFile, "  %5dx%-5d", aRow.mWidth, aRow.mHeight);
		for (int i = 0; i < 6; i++)
			fprintf(aFile, " %8zu", aRow.mBytes[i]);
		fprintf(aFile, "  %s\n", aRow.mName.c_str());
	}

	SDLSoundManager* aSoundManager = dynamic_cast<SDLSoundManager*>(theApp->mSoundManager);
	if (aSoundManager != NULL)
	{
		int aCount = 0;
		size_t aBytes = SoundSamples::Bytes(aSoundManager, &aCount);
		fprintf(aFile, "sounds: %d, %.2f MB of samples\n", aCount, aBytes / MB);
	}

	fputs(DescribeHeaps().c_str(), aFile);
	return fclose(aFile) == 0;
}
