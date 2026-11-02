// bjt_curvetest: evaluates CurvedVal curve strings (src/CurvedVal.h), headless. Each argument is a curve string; for
// each it prints the parsed ranges and the output at 11 inputs across the input range.
//     build/native/games/bejeweledtwist/bjt_curvetest.exe "b;0,1,0.01,1,~###         ~####"
#include "CurvedVal.h"

#include <SDL.h>
#include <cstdio>

using namespace Sexy;

int main(int argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		CurvedVal v;
		v.SetCurve(argv[i]);
		printf("%s\n  out %g..%g  in %g..%g  inc %g  flags noclip=%d single=%d sync=%d hermite=%d autoinc=%d\n ", argv[i],
			v.mOutMin, v.mOutMax, v.mInMin, v.mInMax, v.mIncRate, v.mNoClip, v.mSingleTrigger, v.mOutputSync, v.mIsHermite,
			v.mAutoInc);
		for (int k = 0; k <= 10; k++)
			printf(" %.4f", v.GetOutVal(v.mInMin + (v.mInMax - v.mInMin) * k / 10.0));
		printf("\n");
	}
	return 0;
}
