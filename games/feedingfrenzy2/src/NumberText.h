#ifndef __FEEDINGFRENZY2_NUMBERTEXT_H__
#define __FEEDINGFRENZY2_NUMBERTEXT_H__

// The game's ways of writing a number for a text: grouped by commas, and the timed mode's clock.

#include <string>

// 0x422970 (feedingfrenzy2): the number unsigned ("%u", or "%06u" padded to six digits with zeros, its mode 1), with
// a comma before each three digits from the right
std::string groupedNumber(int theNumber, bool isPadded = false);

// 0x44ca60 (feedingfrenzy2) as 0x48c830 (feedingfrenzy2) calls it: the seconds as the hours, then two digits each of
// the minutes and the seconds ("H:mm:ss" by GetTimeFormatA). port: the locale's time separator, which the original
// reads (LOCALE_STIME), is the colon of the user's locale
std::string stageClock(int theSeconds);

// 0x408920 (feedingfrenzy2): the number by "%d" (0x41b1a0 (feedingfrenzy2)) then grouped by the locale (0x44e580
// (feedingfrenzy2)). port: the user's locale groups by threes with commas, as groupedNumber, the sign kept in front
std::string localeNumber(int theNumber);

#endif
