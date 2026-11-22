// port: b3_curvetest evaluates CurvedVal curve strings (engines/prime/CurvedVal.h), headless.
//     build/native/b3_curvetest.exe [-steps=<n>] [-exe=<Bejeweled3.exe>] [<curve string>...]
// With -exe it takes every curve string the exe holds (a NUL-ended text that starts "a" or "b", a flags character, and
// three numbers with commas); with neither it reads assets/bejeweled3/bejeweled3.exe. For each curve it prints the
// parsed ranges and flags and the output at -steps + 1 inputs across the input range (10 by default), then a hash of
// all of them. The curves are the game's: the output is for comparing builds and goes into no file of the repository.
#include "CurvedVal.h"

#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace Sexy;

namespace {

bool IsNumberChar(char c)
{
	return (c >= '0' && c <= '9') || c == '.' || c == '-';
}

// "b;0,1,0.01,1,..." and the like: a version letter, a flags character, then at least three numbers and commas
bool LooksLikeCurve(const char* theText, size_t theLength)
{
	if (theLength < 8 || (theText[0] != 'a' && theText[0] != 'b'))
		return false;
	size_t i = 2;
	for (int aNumber = 0; aNumber < 3; aNumber++) {
		size_t aStart = i;
		while (i < theLength && IsNumberChar(theText[i]))
			i++;
		if (i == aStart || i >= theLength || theText[i] != ',')
			return false;
		i++;
	}
	for (size_t k = 0; k < theLength; k++)
		if ((unsigned char)theText[k] < 0x20 || (unsigned char)theText[k] > 0x7e)
			return false;
	return true;
}

std::vector<std::string> CurvesInExe(const char* thePath)
{
	std::vector<std::string> aCurves;
	std::ifstream aFile(thePath, std::ios::binary);
	if (!aFile)
		return aCurves;
	std::vector<char> aData((std::istreambuf_iterator<char>(aFile)), std::istreambuf_iterator<char>());
	size_t aStart = 0;
	for (size_t i = 0; i < aData.size(); i++) {
		if (aData[i] != 0)
			continue;
		if (i > aStart && LooksLikeCurve(&aData[aStart], i - aStart))
			aCurves.push_back(std::string(&aData[aStart], i - aStart));
		aStart = i + 1;
	}
	return aCurves;
}

unsigned long long gHash = 1469598103934665603ULL;

void HashBytes(const void* theData, size_t theSize)
{
	const unsigned char* p = (const unsigned char*)theData;
	for (size_t i = 0; i < theSize; i++) {
		gHash ^= p[i];
		gHash *= 1099511628211ULL;
	}
}

}  // namespace

int main(int argc, char** argv)
{
	int aSteps = 10;
	const char* anExe = nullptr;
	std::vector<std::string> aCurves;
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-steps=", 7) == 0)
			aSteps = atoi(argv[i] + 7);
		else if (strncmp(argv[i], "-exe=", 5) == 0)
			anExe = argv[i] + 5;
		else
			aCurves.push_back(argv[i]);
	}
	if (anExe != nullptr || aCurves.empty()) {
		const char* aPath = anExe ? anExe : "assets/bejeweled3/bejeweled3.exe";
		aCurves = CurvesInExe(aPath);
		if (aCurves.empty()) {
			fprintf(stderr, "no curve strings in %s\n", aPath);
			return 1;
		}
	}
	for (const std::string& aCurve : aCurves) {
		CurvedVal v;
		v.SetCurve(aCurve);
		printf("%s\n  out %.9g..%.9g  in %.9g..%.9g  inc %.9g  flags noclip=%d single=%d sync=%d hermite=%d autoinc=%d\n ",
			aCurve.c_str(), v.mOutMin, v.mOutMax, v.mInMin, v.mInMax, v.mIncRate, v.mNoClip, v.mSingleTrigger,
			v.mOutputSync, v.mIsHermite, v.mAutoInc);
		for (int k = 0; k <= aSteps; k++) {
			double anOut = v.GetOutVal(v.mInMin + (v.mInMax - v.mInMin) * k / aSteps);
			HashBytes(&anOut, sizeof(anOut));
			printf(" %.9g", anOut);
		}
		printf("\n");
	}
	printf("%d curves, hash %016llx\n", (int)aCurves.size(), gHash);
	return 0;
}
