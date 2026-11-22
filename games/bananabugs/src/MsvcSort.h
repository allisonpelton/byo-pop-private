// MSVC 2005's std::sort (Dinkumware's introsort), as the original sorts the objects to draw (0x452300..0x4528a0).
// The host library's sort orders equal elements differently, and objects of equal depth draw in the order this one
// leaves them, so the port keeps the original's algorithm.
#pragma once

namespace Sexy
{
namespace MsvcSort
{

const int ISORT_MAX = 32;

// 0x452760: sorts three elements in place
template <typename T, typename Less>
void Med3(T* theFirst, T* theMid, T* theLast, Less theLess)
{
	if (theLess(*theMid, *theFirst)) {
		T aTemp = *theMid;
		*theMid = *theFirst;
		*theFirst = aTemp;
	}
	if (theLess(*theLast, *theMid)) {
		T aTemp = *theLast;
		*theLast = *theMid;
		*theMid = aTemp;
	}
	if (theLess(*theMid, *theFirst)) {
		T aTemp = *theMid;
		*theMid = *theFirst;
		*theFirst = aTemp;
	}
}

// 0x452630: the median of three, or of nine for more than 40 elements
template <typename T, typename Less>
void Median(T* theFirst, T* theMid, T* theLast, Less theLess)
{
	if (40 < theLast - theFirst) {
		int aStep = (int)((theLast - theFirst + 1) / 8);
		Med3(theFirst, theFirst + aStep, theFirst + 2 * aStep, theLess);
		Med3(theMid - aStep, theMid, theMid + aStep, theLess);
		Med3(theLast - 2 * aStep, theLast - aStep, theLast, theLess);
		Med3(theFirst + aStep, theMid, theLast - aStep, theLess);
	} else {
		Med3(theFirst, theMid, theLast, theLess);
	}
}

template <typename T>
void IterSwap(T* theLeft, T* theRight)
{
	T aTemp = *theLeft;
	*theLeft = *theRight;
	*theRight = aTemp;
}

// 0x4523f0: partitions around the median; the equal range comes back in thePFirst and thePLast
template <typename T, typename Less>
void UnguardedPartition(T* theFirst, T* theLast, Less theLess, T** thePFirst, T** thePLast)
{
	T* aMid = theFirst + (theLast - theFirst) / 2;
	Median(theFirst, aMid, theLast - 1, theLess);
	T* aPFirst = aMid;
	T* aPLast = aPFirst + 1;

	while (theFirst < aPFirst && !theLess(*(aPFirst - 1), *aPFirst) && !theLess(*aPFirst, *(aPFirst - 1)))
		--aPFirst;
	while (aPLast < theLast && !theLess(*aPLast, *aPFirst) && !theLess(*aPFirst, *aPLast))
		++aPLast;

	T* aGFirst = aPLast;
	T* aGLast = aPFirst;

	for (;;) {
		for (; aGFirst < theLast; ++aGFirst) {
			if (theLess(*aPFirst, *aGFirst))
				;
			else if (theLess(*aGFirst, *aPFirst))
				break;
			else
				IterSwap(aPLast++, aGFirst);
		}
		for (; theFirst < aGLast; --aGLast) {
			if (theLess(*(aGLast - 1), *aPFirst))
				;
			else if (theLess(*aPFirst, *(aGLast - 1)))
				break;
			else
				IterSwap(--aPFirst, aGLast - 1);
		}
		if (aGLast == theFirst && aGFirst == theLast) {
			*thePFirst = aPFirst;
			*thePLast = aPLast;
			return;
		}

		if (aGLast == theFirst) {
			if (aPLast != aGFirst)
				IterSwap(aPFirst, aPLast);
			++aPLast;
			IterSwap(aPFirst++, aGFirst++);
		} else if (aGFirst == theLast) {
			if (--aGLast != --aPFirst)
				IterSwap(aGLast, aPFirst);
			IterSwap(aPFirst, --aPLast);
		} else {
			IterSwap(aGFirst++, --aGLast);
		}
	}
}

// 0x452590 (with 0x452870, the rotate it moves each run with): each element goes back to the first place whose
// element is not greater
template <typename T, typename Less>
void InsertionSort(T* theFirst, T* theLast, Less theLess)
{
	if (theFirst == theLast)
		return;
	for (T* aNext = theFirst; ++aNext != theLast;) {
		T* aNext1 = aNext;
		T aValue = *aNext;
		if (theLess(aValue, *theFirst)) {
			for (T* aMove = aNext; aMove != theFirst; --aMove)
				*aMove = *(aMove - 1);
			*theFirst = aValue;
		} else {
			for (T* aFirst1 = aNext1; theLess(aValue, *--aFirst1); aNext1 = aFirst1)
				*aNext1 = *aFirst1;
			*aNext1 = aValue;
		}
	}
}

// 0x452820
template <typename T, typename Less>
void PushHeap(T* theFirst, int theHole, int theTop, T theValue, Less theLess)
{
	for (int anIdx = (theHole - 1) / 2; theTop < theHole && theLess(*(theFirst + anIdx), theValue);
		anIdx = (theHole - 1) / 2) {
		*(theFirst + theHole) = *(theFirst + anIdx);
		theHole = anIdx;
	}
	*(theFirst + theHole) = theValue;
}

// 0x4527b0
template <typename T, typename Less>
void AdjustHeap(T* theFirst, int theHole, int theBottom, T theValue, Less theLess)
{
	int aTop = theHole;
	int anIdx = 2 * theHole + 2;
	for (; anIdx < theBottom; anIdx = 2 * anIdx + 2) {
		if (theLess(*(theFirst + anIdx), *(theFirst + (anIdx - 1))))
			--anIdx;
		*(theFirst + theHole) = *(theFirst + anIdx);
		theHole = anIdx;
	}
	if (anIdx == theBottom) {
		*(theFirst + theHole) = *(theFirst + (theBottom - 1));
		theHole = theBottom - 1;
	}
	PushHeap(theFirst, theHole, aTop, theValue, theLess);
}

// 0x4526e0
template <typename T, typename Less>
void MakeHeap(T* theFirst, T* theLast, Less theLess)
{
	int aBottom = (int)(theLast - theFirst);
	for (int aHole = aBottom / 2; 0 < aHole;) {
		--aHole;
		AdjustHeap(theFirst, aHole, aBottom, *(theFirst + aHole), theLess);
	}
}

// 0x452720
template <typename T, typename Less>
void SortHeap(T* theFirst, T* theLast, Less theLess)
{
	for (; 1 < theLast - theFirst; --theLast) {
		T aValue = *(theLast - 1);
		*(theLast - 1) = *theFirst;
		AdjustHeap(theFirst, 0, (int)(theLast - 1 - theFirst), aValue, theLess);
	}
}

// 0x452300
template <typename T, typename Less>
void Sort(T* theFirst, T* theLast, int theIdeal, Less theLess)
{
	int aCount;
	for (; ISORT_MAX < (aCount = (int)(theLast - theFirst)) && 0 < theIdeal;) {
		T* aMidFirst;
		T* aMidLast;
		UnguardedPartition(theFirst, theLast, theLess, &aMidFirst, &aMidLast);
		theIdeal /= 2;
		theIdeal += theIdeal / 2;
		if (aMidFirst - theFirst < theLast - aMidLast) {
			Sort(theFirst, aMidFirst, theIdeal, theLess);
			theFirst = aMidLast;
		} else {
			Sort(aMidLast, theLast, theIdeal, theLess);
			theLast = aMidFirst;
		}
	}
	if (ISORT_MAX < aCount) {
		MakeHeap(theFirst, theLast, theLess);
		SortHeap(theFirst, theLast, theLess);
	} else if (1 < aCount) {
		InsertionSort(theFirst, theLast, theLess);
	}
}

template <typename T, typename Less>
void Sort(T* theFirst, T* theLast, Less theLess)
{
	Sort(theFirst, theLast, (int)(theLast - theFirst), theLess);
}

} // namespace MsvcSort
} // namespace Sexy
