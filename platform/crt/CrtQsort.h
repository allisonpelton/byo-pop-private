// The C runtime's qsort, translated from the games' exes, so that elements the comparator calls equal (or never
// equal, as in Cosmic Bugs' and Water Bugs' draw list) end in the original's order. Shared by every game that sorts
// through the runtime's qsort; the one copy is platform/crt/CrtQsort.cpp.
#pragma once

#include <cstddef>

namespace Platform
{

// Sorts theNum elements of theWidth bytes at theBase, smallest first by theCompare, in the order the exes' qsort leaves them
void CrtQsort(void* theBase, size_t theNum, size_t theWidth, int (*theCompare)(const void*, const void*));

} // namespace Platform
