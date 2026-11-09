// The --coverage recorder in the sound stand-in (coverage.c).
#pragma once

void coverage_install(void);   // at the stand-in's load: breakpoints on the listed functions, if origrun asked for them
void coverage_detach(void);    // at its unload: the closing line of the hits file
