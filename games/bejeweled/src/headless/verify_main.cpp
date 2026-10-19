// port(variant): bejeweled_verify, the headless verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9; Verifier.h).
// Not part of the original game.
//
// Native (bejeweled_verify.exe, i686 like the game):
//   bejeweled_verify <replay.bjr> [-trace] [-forcevariant]
//       plays the replay: the "REPLAY ..." lines on stdout as the game's -replay prints them (with -trace also
//       the board's settles, BoardTrace.h), then the result JSON as the last line. Exit status 0 when the
//       replay is ok, 1 when it is corrupt or cannot be played.
//   bejeweled_verify difftest --seeds <a>..<b> [--ticks <n>] [--mode normal|timed|both] [-v]
//       the plain and the -forcevariant board in lockstep (Verifier::Difftest): a summary per mode (-v: a line
//       per game). Exit status 0 when every game played alike.
//   bejeweled_verify rulesid <mod.json>
//       the RulesId of the mod's rules (Verifier::RulesIdJson) as JSON.
// Exit status 2 for a usage error or a file that cannot be read.
//
// Wasm (bejeweled_verify.wasm: -sSTANDALONE_WASM --no-entry, no JS glue; its WASI imports are stubbed by the
// host, games/bejeweled/tools/verify/wasmrun.js): call _initialize first, then
//   bv_alloc(n) / bv_free(p)          memory for the arguments
//   bv_verify(ptr, len)               a replay's bytes -> a NUL-terminated result JSON (valid until the next call);
//                                     the REPLAY lines (and TRACE lines with -trace) go to fd 1, named replay.bjr
//   bv_set_options(flags)             bv_verify's switches: 1 -trace, 2 -forcevariant
//   bv_rules_id(ptr, len)             a mod.json's text -> the RulesId JSON
//   bv_difftest(first, last, ticks, modes)  the difftest -> its report text (modes 1 normal, 2 timed, 3 both;
//                                     + 4: a line per game, as -v); ticks 0: the default cap; arguments it does
//                                     not take (Verifier::Difftest): "difftest: <why>", nothing played

#include "Verifier.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

using namespace Sexy;

#ifdef __EMSCRIPTEN__

#include <emscripten/emscripten.h>

namespace
{
std::string gResult;
int gOptions = 0;   // bv_set_options
}

extern "C"
{

EMSCRIPTEN_KEEPALIVE void* bv_alloc(int theSize)
{
	return malloc((theSize > 0) ? (size_t) theSize : 1);
}

EMSCRIPTEN_KEEPALIVE void bv_free(void* thePtr)
{
	free(thePtr);
}

EMSCRIPTEN_KEEPALIVE void bv_set_options(int theOptions)
{
	gOptions = theOptions;
}

EMSCRIPTEN_KEEPALIVE const char* bv_verify(const unsigned char* theBytes, int theLen)
{
	std::string aBytes((const char*) theBytes, (theLen > 0) ? (size_t) theLen : 0);
	gResult = Verifier::VerifyReplay(std::move(aBytes), "replay.bjr", (gOptions & 1) != 0, (gOptions & 2) != 0, NULL);
	fflush(stdout);
	return gResult.c_str();
}

EMSCRIPTEN_KEEPALIVE const char* bv_rules_id(const char* theJson, int theLen)
{
	gResult = Verifier::RulesIdJson(std::string(theJson, (theLen > 0) ? (size_t) theLen : 0));
	return gResult.c_str();
}

EMSCRIPTEN_KEEPALIVE const char* bv_difftest(unsigned theFirst, unsigned theLast, int theTicks, int theModes)
{
	// Verifier::Difftest checks the arguments (a report "difftest: <why>" for those it does not take)
	gResult.clear();
	Verifier::Difftest(theFirst, theLast, (theTicks != 0) ? theTicks : (int) Verifier::DEFAULT_TICKS, theModes & 3, (theModes & 4) != 0,
		&gResult);
	return gResult.c_str();
}

}

#else

namespace
{

int Usage()
{
	fprintf(stderr,
		"usage: bejeweled_verify <replay.bjr> [-trace] [-forcevariant]\n"
		"       bejeweled_verify difftest --seeds <a>..<b> [--ticks <n>] [--mode normal|timed|both] [-v]\n"
		"       bejeweled_verify rulesid <mod.json>\n");
	return 2;
}

bool ReadFile(const char* theName, std::string* theBytes, size_t theMax)
{
	FILE* aFile = fopen(theName, "rb");
	if (aFile == NULL)
		return false;
	char aChunk[65536];
	size_t aCount;
	while ((theBytes->size() <= theMax) && ((aCount = fread(aChunk, 1, sizeof(aChunk), aFile)) > 0))
		theBytes->append(aChunk, aCount);
	fclose(aFile);
	return true;
}

// The base name of a path, for the REPLAY lines (the game prints the -replay argument)
std::string BaseName(const std::string& thePath)
{
	size_t aSlash = thePath.find_last_of("/\\");
	return (aSlash == std::string::npos) ? thePath : thePath.substr(aSlash + 1);
}

int Difftest(int argc, char** argv)
{
	unsigned long aFirst = 1;
	unsigned long aLast = 1;
	int aTicks = Verifier::DEFAULT_TICKS;
	int aModes = Verifier::MODE_NORMAL | Verifier::MODE_TIMED;
	bool isVerbose = false;
	bool hasSeeds = false;
	for (int i = 2; i < argc; i++)
	{
		std::string anArg = argv[i];
		if ((anArg == "--seeds") && (i + 1 < argc))
		{
			std::string aRange = argv[++i];
			size_t aDots = aRange.find("..");
			char* anEnd = NULL;
			aFirst = strtoul(aRange.c_str(), &anEnd, 10);
			aLast = (aDots != std::string::npos) ? strtoul(aRange.c_str() + aDots + 2, NULL, 10) : aFirst;
			// Verifier::Difftest checks the range (a usage error, as the wasm export reports it)
			if ((aRange.empty() || !isdigit((unsigned char) aRange[0])) || (aLast > 0xFFFFFFFFul))
				return Usage();
			hasSeeds = true;
		}
		else if ((anArg == "--ticks") && (i + 1 < argc))
			aTicks = atoi(argv[++i]);
		else if ((anArg == "--mode") && (i + 1 < argc))
		{
			std::string aMode = argv[++i];
			aModes = (aMode == "normal") ? Verifier::MODE_NORMAL : (aMode == "timed") ? Verifier::MODE_TIMED :
				(aMode == "both") ? (Verifier::MODE_NORMAL | Verifier::MODE_TIMED) : 0;
			if (aModes == 0)
				return Usage();
		}
		else if (anArg == "-v")
			isVerbose = true;
		else
			return Usage();
	}
	if (!hasSeeds || (aTicks <= 0))
		return Usage();
	std::string aReport;
	int aDiffering = Verifier::Difftest((uint32_t) aFirst, (uint32_t) aLast, aTicks, aModes, isVerbose, &aReport);
	if (aDiffering < 0)
	{
		fputs(aReport.c_str(), stderr);
		return Usage();
	}
	fputs(aReport.c_str(), stdout);
	fflush(stdout);
	return (aDiffering == 0) ? 0 : 1;
}

}

int main(int argc, char** argv)
{
	if (argc < 2)
		return Usage();
	std::string aCommand = argv[1];
	if (aCommand == "difftest")
		return Difftest(argc, argv);
	if (aCommand == "rulesid")
	{
		std::string aText;
		if ((argc != 3) || !ReadFile(argv[2], &aText, 16 * 1024 * 1024))
			return Usage();
		printf("%s\n", Verifier::RulesIdJson(aText).c_str());
		return 0;
	}

	bool isTrace = false;
	bool forceVariant = false;
	const char* aFileName = NULL;
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-trace"))
			isTrace = true;
		else if (!strcmp(argv[i], "-forcevariant"))
			forceVariant = true;
		else if ((argv[i][0] != '-') && (aFileName == NULL))
			aFileName = argv[i];
		else
			return Usage();
	}
	if (aFileName == NULL)
		return Usage();
	// At most one byte more than a replay may have, which Replay::Read then refuses (as WatchController::Load)
	std::string aBytes;
	if (!ReadFile(aFileName, &aBytes, 16 * 1024 * 1024))
	{
		fprintf(stderr, "bejeweled_verify: cannot read %s\n", aFileName);
		return 2;
	}
	bool isOk = false;
	std::string aJson = Verifier::VerifyReplay(std::move(aBytes), BaseName(aFileName), isTrace, forceVariant, &isOk);
	printf("%s\n", aJson.c_str());
	fflush(stdout);
	return isOk ? 0 : 1;
}

#endif
