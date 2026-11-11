// port: the "memreport" test command (SexyAppBase.h, -script; platform/MemReport.h): what the game holds in memory, by
// kind. It only reads, so a run that gives the command plays as a run that does not.

#include "SexyAppBase.h"
#include "MemoryImage.h"
#include "DDImage.h"
#include "SDLSoundManager.h"
#include "../platform/MemReport.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <vector>

using namespace Sexy;

namespace
{

struct ImageRow
{
	std::string mName;
	int mWidth, mHeight;
	size_t mBytes[6]; // bits, palette, alpha caches, HD bits, HD alpha caches, texture (an estimate)
};

const char* const kColumns[6] = {"bits", "palette", "alpha caches", "HD bits", "HD caches", "textures (est.)"};

size_t Total(const ImageRow& theRow)
{
	size_t aTotal = 0;
	for (int i = 0; i < 6; i++)
		aTotal += theRow.mBytes[i];
	return aTotal;
}

} // namespace

void SexyAppBase::AddMemReportHolders(Platform::MemReport* theReport)
{
}

bool SexyAppBase::DoMemReport(const std::string& theFileName)
{
	// the pictures: every MemoryImage alive, with the copies its blitters made and its texture
	std::vector<ImageRow> aRows;
	size_t aSums[6] = {0, 0, 0, 0, 0, 0};
	int aTexturedCount = 0;
	for (MemoryImage* anImage : mMemoryImageSet)
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
		if (anImage->mD3DData != NULL)
			aTexturedCount++;
		for (int i = 0; i < 6; i++)
			aSums[i] += aRow.mBytes[i];
		aRows.push_back(aRow);
	}

	Platform::MemReport aReport;
	aReport.mImageCount = (int) aRows.size();
	aReport.mImageTexturedCount = aTexturedCount;
	aReport.mImagePixelBytes = aSums[0] + aSums[1];
	aReport.mImageCopyBytes = aSums[2] + aSums[3] + aSums[4];
	aReport.mImageTextureBytes = aSums[5];
	SDLSoundManager* aSoundManager = dynamic_cast<SDLSoundManager*>(mSoundManager);
	if (aSoundManager != NULL)
		aReport.mSoundBytes = aSoundManager->GetSoundBytes(&aReport.mSoundCount);
	Platform::AddSongMemory(&aReport);
	AddMemReportHolders(&aReport);
	Platform::LogMemReport(aReport);

	if (theFileName.empty())
		return true;

	// the table: the sums, then every picture that came from a file, the largest first, then the pictures the game made
	// itself, which have no name, summed by their size
	FILE* aFile = fopen(theFileName.c_str(), "w");
	if (aFile == NULL)
		return false;
	std::sort(aRows.begin(), aRows.end(), [](const ImageRow& theLeft, const ImageRow& theRight) { return Total(theLeft) > Total(theRight); });
	const double MB = 1024.0 * 1024.0;
	fprintf(aFile, "images: %d, %d with a texture\n", (int) aRows.size(), aTexturedCount);
	for (int i = 0; i < 6; i++)
		fprintf(aFile, "  %-16s %8.2f MB\n", kColumns[i], aSums[i] / MB);
	std::map<std::pair<int, int>, ImageRow> aMadeBySize;      // a sum for each size: its mWidth counts the pictures
	for (const ImageRow& aRow : aRows)
	{
		if (aRow.mName.empty())
		{
			ImageRow& aSum = aMadeBySize[std::make_pair(aRow.mWidth, aRow.mHeight)];
			if (aSum.mName.empty())
			{
				aSum.mName = "(made by the game)";
				aSum.mWidth = 0;
				for (int i = 0; i < 6; i++)
					aSum.mBytes[i] = 0;
			}
			aSum.mWidth++;
			for (int i = 0; i < 6; i++)
				aSum.mBytes[i] += aRow.mBytes[i];
			continue;
		}
		fprintf(aFile, "  %5dx%-5d", aRow.mWidth, aRow.mHeight);
		for (int i = 0; i < 6; i++)
			fprintf(aFile, " %8zu", aRow.mBytes[i]);
		fprintf(aFile, "  %s\n", aRow.mName.c_str());
	}
	for (std::map<std::pair<int, int>, ImageRow>::iterator anItr = aMadeBySize.begin(); anItr != aMadeBySize.end(); ++anItr)
	{
		fprintf(aFile, "  %5dx%-5d", anItr->first.first, anItr->first.second);
		for (int i = 0; i < 6; i++)
			fprintf(aFile, " %8zu", anItr->second.mBytes[i]);
		fprintf(aFile, "  %d made by the game\n", anItr->second.mWidth);
	}
	fprintf(aFile, "sounds: %d, %.2f MB of samples\n", aReport.mSoundCount, aReport.mSoundBytes / MB);
	fprintf(aFile, "music: %d songs, %.2f MB of tracks, %.2f MB of module files\n", aReport.mSongCount, aReport.mSongTrackBytes / MB,
		aReport.mSongModuleFileBytes / MB);
	for (size_t i = 0; i < aReport.mOthers.size(); i++)
		fprintf(aFile, "%s: %.2f MB\n", aReport.mOthers[i].first.c_str(), aReport.mOthers[i].second / MB);
	fputs(Platform::DescribeHeaps().c_str(), aFile);
	fprintf(aFile, "summary: %s\n", Platform::FormatMemReport(aReport).c_str());
	return fclose(aFile) == 0;
}
