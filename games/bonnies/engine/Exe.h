#ifndef __BONNIES_EXE_H__
#define __BONNIES_EXE_H__

// The game's data that lives in its exe (tables, cursor shapes, texts): read at run time from the user's own copy,
// the game inside BonniesBookstore.exe in the game folder (src/main.cpp), by the original's addresses, so that none of
// it is in this repository.

#include <cstdint>
#include <string>
#include <vector>

namespace bonnies
{

// Reads the exe's sections; false when it is missing or not the expected build
bool loadExe(const std::vector<uint8_t>& theData);
// The bytes at an address of the loaded image (zeros for an address in no section's file data)
const uint8_t* exePtr(uint32_t theAddress);
uint32_t exeU32(uint32_t theAddress);
int32_t exeI32(uint32_t theAddress);
float exeFloat(uint32_t theAddress);
// The zero-ended string at an address
const char* exeString(uint32_t theAddress);
// theCount string pointers from an array of them at theAddress
std::vector<const char*> exeStringArray(uint32_t theAddress, int theCount);

// Runs what the exe's C++ static initializers (the CRT's __initterm table, 0x466000-0x466284) do to its data: they
// store numbers and construct the global strings from literals, nothing else. The numbers go into the loaded image;
// the strings are kept by their addresses. Answers how many initializers were understood; the others are listed in
// theNotUnderstood.
int runStaticInitializers(std::vector<uint32_t>* theNotUnderstood = nullptr);
// The text a static initializer gave the global string at theAddress ("" when none did)
const std::string& exeGlobalString(uint32_t theAddress);

} // namespace bonnies

#endif
