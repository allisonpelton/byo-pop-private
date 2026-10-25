// port(variant): the out-of-line parts of the verifier's framework shims (games/bejeweled/docs/CODE_MODS_DESIGN.md
// 6.9; the headers are in headless/shim). Not part of the original game. framework/Common.cpp includes SDL and
// the VFS, so its few functions the board code calls are replaced here, with the same behaviour: the global
// Rand() stream (a board the verifier plays always owns an MTRand, so nothing it hashes reads this one) and
// the string formatting. Also GameConfigMods.cpp's file-name scoping (that file reads the mods).

#include "Common.h"
#include "MTRand.h"
#include "SexyApp.h"
#include "Widget.h"
#include "DDImage.h"
#include "GameConfig.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <algorithm>
#include <cstdarg>
#include <cstdio>

using namespace Sexy;

// ---- framework/Common.cpp ----

static MTRand gMTRand;
static uint gRandCount = 0;

int Sexy::Rand()
{
	gRandCount++;
	return gMTRand.Next();
}

int Sexy::Rand(int range)
{
	gRandCount++;
	return gMTRand.Next((unsigned long) range);
}

float Sexy::Rand(float range)
{
	gRandCount++;
	return gMTRand.Next(range);
}

void Sexy::SRand(ulong theSeed)
{
	gMTRand.SRand(theSeed);
}

uint Sexy::GetRandCount()
{
	return gRandCount;
}

std::string Sexy::GetRandState()
{
	return gMTRand.Serialize();
}

void Sexy::GetRandMTState(uint32_t* theState, int* theIndex)
{
	gMTRand.GetState(theState, theIndex);
}

std::string Sexy::vformat(const char* fmt, va_list argPtr)
{
	const int bufSize = 161;
	char stackBuffer[bufSize];

	va_list anArgs;
	va_copy(anArgs, argPtr);
	int numChars = vsnprintf(stackBuffer, bufSize, fmt, anArgs);
	va_end(anArgs);

	if (numChars < 0)
		return std::string();
	if (numChars < bufSize)
		return std::string(stackBuffer, numChars);

	std::string result;
	result.resize(numChars + 1);
	va_copy(anArgs, argPtr);
	vsnprintf(&result[0], numChars + 1, fmt, anArgs);
	va_end(anArgs);
	result.resize(numChars);
	return result;
}

std::string Sexy::StrFormat(const char* fmt ...)
{
	va_list argList;
	va_start(argList, fmt);
	std::string result = vformat(fmt, argList);
	va_end(argList);
	return result;
}

// ---- GameConfigMods.cpp ----

// The verifier has no mods: every file keeps its name (only a difftest board's save at its end is written, into
// the app's memory)
std::string GameConfig::ScopedFileName(const std::string& theFileName)
{
	return theFileName;
}

// ---- platform/VFS.cpp ----

// port(mods): platform/ModSettings.cpp (the RulesId of a mod with settings, Verifier::RulesIdJson) checks paths with
// VFS::Normalize; VFS.cpp itself reads the host file system, which the standalone verifier has not. A copy of it.
std::string Platform::VFS::Normalize(const std::string& thePath)
{
	std::string p = thePath;
	std::replace(p.begin(), p.end(), '\\', '/');
	std::vector<std::string> parts;
	size_t start = 0;
	while (start <= p.size())
	{
		size_t end = p.find('/', start);
		if (end == std::string::npos)
			end = p.size();
		std::string part = p.substr(start, end - start);
		if (part == "..")
		{
			if (!parts.empty() && parts.back() != "..")
				parts.pop_back();
			else
				parts.push_back(part);
		}
		else if (!part.empty() && part != ".")
			parts.push_back(part);
		start = end + 1;
	}
	std::string out;
	for (size_t i = 0; i < parts.size(); i++)
	{
		if (i)
			out += '/';
		out += parts[i];
	}
	return out;
}

// ---- SDL ----

extern "C" void SDL_Log(const char* fmt, ...)
{
	va_list anArgs;
	va_start(anArgs, fmt);
	fputs("INFO: ", stderr);
	vfprintf(stderr, fmt, anArgs);
	fputs("\n", stderr);
	va_end(anArgs);
}

// ---- SexyAppBase ----

SexyAppBase::SexyAppBase()
{
	mWidgetManager = NULL;
	mDDInterface = NULL;
	mSoundManager = NULL;
	mMusicInterface = NULL;
	mWidth = 640;
	mHeight = 480;
	mHasFocus = true;
	mShutdown = false;
	mCustomCursorsEnabled = false;
	mUpdateCount = 0;
	mRandSeed = 0;
}

SexyAppBase::~SexyAppBase()
{
	ProcessSafeDeleteList();
}

void SexyAppBase::ProcessSafeDeleteList()
{
	// A widget's destructor can hand over more (a board's ~Board saves, nothing else here)
	while (!mSafeDeleteList.empty())
	{
		std::vector<Widget*> aList;
		aList.swap(mSafeDeleteList);
		for (size_t i = 0; i < aList.size(); i++)
			delete aList[i];
	}
}

DDImage* SexyAppBase::CreateColorizedImage(Image* theImage, const Color& theColor)
{
	DDImage* anImage = new DDImage(mDDInterface);
	if (theImage != NULL)
		anImage->Create(theImage->GetWidth(), theImage->GetHeight());
	return anImage;
}

DDImage* SexyAppBase::CreateCrossfadeImage(Image* theImage1, const Rect& theRect1, Image* theImage2, const Rect& theRect2, double theFadeFactor)
{
	DDImage* anImage = new DDImage(mDDInterface);
	anImage->Create(theRect1.mWidth, theRect1.mHeight);
	return anImage;
}

bool SexyAppBase::WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer)
{
	mFiles[theFileName] = std::string((const char*) theBuffer->GetDataPtr(), (size_t) theBuffer->GetDataLen());
	return true;
}

bool SexyAppBase::ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo)
{
	std::map<std::string, std::string>::const_iterator anItr = mFiles.find(theFileName);
	if (anItr == mFiles.end())
		return false;
	theBuffer->Clear();
	theBuffer->WriteBytes((const uchar*) anItr->second.data(), (int) anItr->second.size());
	theBuffer->SeekFront();
	return true;
}

bool SexyAppBase::WriteBytesToFile(const std::string& theFileName, const void* theData, unsigned long theDataLen)
{
	mFiles[theFileName] = std::string((const char*) theData, (size_t) theDataLen);
	return true;
}
