// galaxytest.cpp: parses every assets/bejeweled2/puzzles/*/galaxy.xml with GalaxyParser, prints the counts and the first
// planet's fields, and checks the parser's failure texts on a few broken files. Not part of the build; compile it by hand
// (it opens no window and plays no sound), from the worktree's root:
//   CXX=E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang++.exe
//   $CXX -std=gnu++17 -O1 -DLIBXMP_STATIC -Igames/bejeweled2/src -I. -Iframework -Ithird_party/stb \
//        -isystem E:/retools/SDL2-2.32.10/i686-w64-mingw32/include/SDL2 games/bejeweled2/tools/galaxytest.cpp \
//        games/bejeweled2/src/GalaxyParser.cpp -Lbuild/native/framework -lsexyframework //        -LE:/retools/SDL2-2.32.10/i686-w64-mingw32/lib -lSDL2 -o build/galaxytest.exe
//   PATH=build/native:$PATH build/galaxytest.exe [assets/bejeweled2/puzzles]   (build/native holds SDL2.dll and libc++.dll)
#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <string>
#include <vector>
#include "Buffer.h"
#include "GalaxyParser.h"

using namespace Sexy;
namespace fs = std::filesystem;

static int gFailures = 0;

static void Check(bool theCondition, const std::string& theWhat)
{
	if (!theCondition)
	{
		printf("  CHECK FAILED: %s\n", theWhat.c_str());
		gFailures++;
	}
}

static bool ReadFile(const std::string& thePath, std::string* theData)
{
	FILE* aFile = fopen(thePath.c_str(), "rb");
	if (aFile == NULL)
		return false;
	char aBuf[4096];
	size_t aCount;
	theData->clear();
	while ((aCount = fread(aBuf, 1, sizeof(aBuf), aFile)) > 0)
		theData->append(aBuf, aCount);
	fclose(aFile);
	return true;
}

static bool ParseText(GalaxyParser& theParser, const std::string& theText)
{
	Buffer aBuffer;
	aBuffer.SetData((uchar*)theText.data(), (int)theText.size());
	return theParser.ParseBuffer(aBuffer);
}

static void TestPack(const fs::path& thePackDir)
{
	std::string aText;
	std::string aPackName = thePackDir.filename().string();
	if (!ReadFile((thePackDir / "galaxy.xml").string(), &aText))
	{
		printf("%s: no galaxy.xml\n", aPackName.c_str());
		return;
	}
	GalaxyParser aParser;
	aParser.mPuzzlePack = aPackName;
	bool anOk = ParseText(aParser, aText);
	printf("pack '%s' (%d bytes): parse %s%s%s\n", aPackName.c_str(), (int)aText.size(), anOk ? "ok" : "FAILED",
		aParser.HasFailed() ? ": " : "", aParser.GetErrorText().c_str());
	Check(anOk && !aParser.HasFailed(), "the pack parses");

	int aPuzzles = 0, aMissingFiles = 0, aTutorials = 0, aMoves = 0, aTexts = 0;
	for (int p = 0; p < aParser.GetPlanetCount(); p++)
	{
		Planet& aPlanet = aParser.GetPlanet(p);
		aPuzzles += aPlanet.GetPuzzleCount();
		if (!aPlanet.mTutorialFile.empty())
			aTutorials++;
		aMoves += (int)aPlanet.mTutorialMoves.size();
		aTexts += (int)aPlanet.mTutorialText.size();
		for (int m = 0; m < aPlanet.GetPuzzleCount(); m++)
			if (!fs::exists(thePackDir / aPlanet.GetPuzzle(m).mFile))
				aMissingFiles++;
		if (!aPlanet.mTutorialFile.empty() && !fs::exists(thePackDir / aPlanet.mTutorialFile))
			aMissingFiles++;
	}
	printf("  mapimage '%s', pack '%s', %d planets, %d puzzles, %d tutorials (%d text lines, %d moves), %d files not found\n",
		aParser.mMapImage.c_str(), aParser.mPuzzlePack.c_str(), aParser.GetPlanetCount(), aPuzzles, aTutorials, aTexts,
		aMoves, aMissingFiles);
	Check(aParser.GetPlanetCount() > 0, "the pack has planets");
	Check(aMissingFiles == 0, "every puzzle and tutorial file exists in the pack's folder");

	if (aParser.GetPlanetCount() > 0)
	{
		const Planet& aPlanet = aParser.GetPlanet(0);
		printf("  first planet: name '%s' desc '%s' map (%d,%d) bkg '%s' aimage '%s' dimage '%s'\n", aPlanet.mName.c_str(),
			aPlanet.mDesc.c_str(), aPlanet.mMapX, aPlanet.mMapY, aPlanet.mBackdrop.c_str(), aPlanet.mAImage.c_str(),
			aPlanet.mDImage.c_str());
		printf("    tutorial file '%s', %d text lines, %d moves:", aPlanet.mTutorialFile.c_str(), (int)aPlanet.mTutorialText.size(),
			(int)aPlanet.mTutorialMoves.size());
		for (const TutorialMove& aMove : aPlanet.mTutorialMoves)
			printf(" (col %d, row %d)", aMove.mCol, aMove.mRow);
		printf("\n");
		for (const std::string& aLine : aPlanet.mTutorialText)
			printf("    text '%s'\n", aLine.c_str());
		for (int m = 0; m < aPlanet.GetPuzzleCount(); m++)
		{
			const Moon& aMoon = aPlanet.GetPuzzle(m);
			printf("    moon %d: file '%s' name '%s' dimage '%s' simage '%s' uimage '%s'\n", m, aMoon.mFile.c_str(),
				aMoon.mName.c_str(), aMoon.mDImage.c_str(), aMoon.mSImage.c_str(), aMoon.mUImage.c_str());
		}
		const Planet& aLast = aParser.GetPlanet(aParser.GetPlanetCount() - 1);
		printf("  last planet: name '%s' map (%d,%d) bkg '%s', %d puzzles\n", aLast.mName.c_str(), aLast.mMapX, aLast.mMapY,
			aLast.mBackdrop.c_str(), aLast.GetPuzzleCount());
	}

	// parsing again starts over: the same counts, and the pack's name stays
	int aPlanets = aParser.GetPlanetCount();
	ParseText(aParser, aText);
	Check(aParser.GetPlanetCount() == aPlanets && aParser.mPuzzlePack == aPackName, "a second ParseBuffer gives the same galaxy");
}

// a broken file: the parse fails and the text (with the line the parser was on) is the expected one
static void TestBroken(const char* theName, const std::string& theText, const std::string& theExpectedStart)
{
	GalaxyParser aParser;
	bool anOk = ParseText(aParser, theText);
	printf("broken '%s': %s, error '%s'\n", theName, anOk ? "ok" : "failed", aParser.GetErrorText().c_str());
	Check(!anOk && aParser.HasFailed(), std::string(theName) + " fails");
	Check(aParser.GetErrorText().compare(0, theExpectedStart.size(), theExpectedStart) == 0, std::string(theName) + " error text");
}

int main(int argc, char** argv)
{
	fs::path aRoot = argc > 1 ? argv[1] : "assets/bejeweled2/puzzles";
	int aPacks = 0;
	if (fs::exists(aRoot))
		for (const fs::directory_entry& anEntry : fs::directory_iterator(aRoot))
			if (anEntry.is_directory() && fs::exists(anEntry.path() / "galaxy.xml"))
			{
				TestPack(anEntry.path());
				aPacks++;
			}
	Check(aPacks > 0, "a pack was found under " + aRoot.string());

	TestBroken("wrong root", "<Planet></Planet>", "Invalid Section 'Planet'");
	TestBroken("wrong section in galaxy", "<Galaxy><Moon/></Galaxy>", "Invalid Section 'Moon'");
	TestBroken("text in galaxy", "<Galaxy>hello</Galaxy>", "Element Not Expected 'hello'");
	TestBroken("wrong section in planet", "<Galaxy><Planet><Planet/></Planet></Galaxy>", "Invalid Section 'Planet'");
	TestBroken("moon with content", "<Galaxy><Planet><Moon>x</Moon></Planet></Galaxy>", "Unexpected Element: 'x'");
	TestBroken("section in text", "<Galaxy><Planet><Tutorial><Text><b/></Text></Tutorial></Planet></Galaxy>", "Unexpected Section: 'b'");
	TestBroken("wrong section in tutorial", "<Galaxy><Planet><Tutorial><Foo/></Tutorial></Planet></Galaxy>", "Invalid Section 'Foo'");
	TestBroken("unclosed", "<Galaxy><Planet", "");

	// a tutorial's text lines and moves, and an empty file (parses to an empty galaxy that has not failed)
	GalaxyParser aParser;
	bool anOk = ParseText(aParser, "<Galaxy mapimage=Map><Planet name=A mapx=7 mapy=9 bkg=B aimage=C dimage=D desc=\"E F\">"
		"<Tutorial file=t.bpz><Text>one</Text><Text/><Move col=1 row=2 /></Tutorial><Moon file=\"a b.bpz\" name=M dimage=1 simage=2 uimage=3 />"
		"</Planet><Planet name=Z></Planet></Galaxy>");
	Check(anOk && aParser.GetPlanetCount() == 2 && aParser.mMapImage == "Map", "small galaxy parses");
	if (aParser.GetPlanetCount() == 2)
	{
		const Planet& aPlanet = aParser.GetPlanet(0);
		Check(aPlanet.mName == "A" && aPlanet.mMapX == 7 && aPlanet.mMapY == 9 && aPlanet.mBackdrop == "B" &&
			aPlanet.mAImage == "C" && aPlanet.mDImage == "D" && aPlanet.mDesc == "E F", "planet fields");
		Check(aPlanet.mTutorialFile == "t.bpz" && aPlanet.mTutorialText.size() == 2 && aPlanet.mTutorialText[0] == "one" &&
			aPlanet.mTutorialText[1].empty() && aPlanet.mTutorialMoves.size() == 1 && aPlanet.mTutorialMoves[0].mCol == 1 &&
			aPlanet.mTutorialMoves[0].mRow == 2, "tutorial fields");
		Check(aPlanet.GetPuzzleCount() == 1 && aPlanet.GetPuzzle(0).mFile == "a b.bpz" && aPlanet.GetPuzzle(0).mName == "M" &&
			aPlanet.GetPuzzle(0).mDImage == "1" && aPlanet.GetPuzzle(0).mSImage == "2" && aPlanet.GetPuzzle(0).mUImage == "3",
			"moon fields");
		Check(aParser.GetPlanet(1).GetPuzzleCount() == 0 && aParser.GetPlanet(1).mName == "Z", "planet without moons");
	}
	aParser.Fail("Unable to open galaxy.xml file");
	Check(aParser.HasFailed() && aParser.GetErrorText() == "Unable to open galaxy.xml file", "Fail before a parse");
	anOk = ParseText(aParser, "");
	Check(anOk && !aParser.HasFailed() && aParser.GetPlanetCount() == 0, "an empty buffer parses to an empty galaxy");

	printf("%s\n", gFailures == 0 ? "galaxytest: all checks passed" : "galaxytest: FAILED");
	return gFailures == 0 ? 0 : 1;
}
