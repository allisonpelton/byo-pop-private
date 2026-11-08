// TypingTutorLessons.cpp: the typing tutor's lessons: the list (TutorLessons, the original's RTTI class Sexy::LessonManager,
// not the level table of src/LessonManager.h), a lesson, a task, and the reader of data\lessonN.xml, which makes a lesson the
// first time the tutor asks for it. Also the three records of UserProfile that the tutor's Finish writes (added for T1).
//
// The file format: one XML tag or tag with its text per line (the lines are fed to the framework's XMLParser one at a time,
// a line counts when it is ended by CR or LF), the tag names compared in capitals:
//   <LESSON> ... </LESSON>              one lesson per file
//   <HEADINGS> <x>heading</x> ... </HEADINGS>   the headings the tasks pick from (the text of each element; its tag is not read)
//   <DESCRIPTION>text</DESCRIPTION>     the text at the right of the panel
//   <MAXERRORS>n</MAXERRORS> <MINWPM>n</MINWPM> <LESSONTIME>seconds</LESSONTIME> <MINTASKS>n</MINTASKS>
//   <MINACCURACY>n</MINACCURACY>        the limits for a pass (see TypingTutor::Finish)
//   <TASK> <HEADING>n</HEADING> <TEXT>a line</TEXT> ... </TASK>   n counts the headings from 1
// A <TEXT File="name"> element names a text file (read with the C library's stdio); the number in its element is the line of
// the file to take (counted from 0), or -1 for a random one (drawn from gGameRand), and the line, with its newline, is the
// task's text.
//
// Helpers of the original that are not translated: 0x414f20 compares two strings (a strcmp that returns whether they are equal:
// == here) and 0x4134d0 is atol of a SexyString; the SexyString wrappers 0x416290, 0x416310 and 0x4165f0 are std::string.
//
// Quirks of the original that are kept: the character read last is handled twice (the loop learns of the end of the file at
// the read after it); the number of lines of a text file that is counted for the random one includes the read that fails
// at the end (it is one more than the lines when the last line is ended by a newline), so the line picked can be the one
// past the end, which reads nothing and leaves the buffer as the line before it left it; a <HEADING> number is not checked.
// Deviations (port:): the line buffer is a std::string (the original's holds 1024 characters), the fgets buffer starts empty;
// the file of a lesson is closed (the original never does); a text file that cannot be opened and a heading number out of range
// are skipped (the original crashes or reads garbage); a missing lesson file returns NULL (the original returns whatever its
// register held); the XML elements the original makes with operator new for every value are locals.

#include "TypingTutor.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "GameText.h"
#include "XMLParser.h"
#include "PakInterface.h"
#include "Common.h"
#include <stdlib.h>
#include <string.h>

using namespace Sexy;

// 0x416620: the string with the letters a to z made capitals
static std::string UpperCaseTag(const std::string& theString)
{
	std::string aResult = theString;
	unsigned int aCharIdx = 0;
	if (aResult.length() != 0)
	{
		do
		{
			if (('`' < aResult[aCharIdx]) && (aResult[aCharIdx] < '{'))
				aResult[aCharIdx] = aResult[aCharIdx] - 0x20;
			aCharIdx = aCharIdx + 1;
		} while (aCharIdx < aResult.length());
	}
	return aResult;
}

// 0x420bd0 (the first four values are then set to -1 by the reader)
TutorLesson::TutorLesson()
{
	mMaxErrors = -1;
	mMinWPM = -1;
	mMinTasks = -1;
	mMinAccuracy = -1;
	mLessonTime = -1;			// port: the original leaves it unset
	mUserState = -1;			// port: the original leaves it unset; NextLesson stores the user's result
}

// the loop of 0x427300 that frees a lesson: each task with its lines and heading, then the task list and the description
TutorLesson::~TutorLesson()
{
	unsigned int aTaskIdx = 0;
	while (true)
	{
		if (mTasks.empty() || ((unsigned int) mTasks.size() <= aTaskIdx))
			break;
		if (mTasks[aTaskIdx] != NULL)
			delete mTasks[aTaskIdx];
		aTaskIdx++;
	}
}

// inlined in 0x460b30
TutorLessons::TutorLessons(WinWSApp* theApp)
{
	mCurrent = -1;
	mApp = theApp;
}

// 0x427300 (deleting 0x429900): the lessons go
TutorLessons::~TutorLessons()
{
	unsigned int aLessonIdx = 0;
	while (aLessonIdx < mLessons.size())
	{
		delete mLessons[aLessonIdx];
		aLessonIdx++;
	}
}

// 0x413bb0: the lesson before, which has been read already (the user's result for it is stored in it again)
TutorLesson* TutorLessons::PrevLesson()
{
	// (the original tests mCurrent == 0 first and does nothing with the result)
	mCurrent = mCurrent - 1;
	mLessons[mCurrent]->mUserState = mApp->mUser->mField94[mCurrent];
	return mLessons[mCurrent];
}

// 0x429930: the next lesson (the 20th is the last that can be asked for): the one that is read already, or the one that
// data\lesson<n>.xml holds (see the top of the file). The lesson gets the user's result for it (-1 not taken, 0 failed, 1 passed)
TutorLesson* TutorLessons::NextLesson()
{
	TutorLesson* aLesson = NULL;				// local_24
	TutorExercise* aTask = NULL;				// local_30
	bool inHeadings = false;					// local_1d
	bool inTask = false;						// local_1e
	std::vector<std::string> aHeadings;			// local_48 the lesson's <HEADINGS>
	char aBuffer[4096];							// local_7e98 where the lines of the text files are read
	aBuffer[0] = 0;								// port: the original's is not set

	if (mCurrent + 1 > 0x13)
		return NULL;

	mCurrent = mCurrent + 1;
	if (!mLessons.empty() && ((unsigned int) mCurrent < mLessons.size()))
	{
		mLessons[mCurrent]->mUserState = mApp->mUser->mField94[mCurrent];
		return mLessons[mCurrent];
	}

	std::string aFileName = StrFormat(GTC(0x0052c990), mCurrent);		// "data\lesson%d.xml"
	PFILE* aFile = p_fopen(aFileName.c_str(), "r");
	if (aFile == NULL)
		return NULL;

	XMLParser aParser;
	XMLElement anElement;
	char aChar = 0;								// local_29: stays as it was after a read that fails at the end of the file
	std::string aLine;							// local_3dd8 with its length local_34

	while (true)
	{
		if (p_feof(aFile))
		{
			p_fclose(aFile);					// port: the original never closes the file
			if (!mLessons.empty() && ((unsigned int) mCurrent < mLessons.size()))
				mLessons[mCurrent]->mUserState = mApp->mUser->mField94[mCurrent];
			if (mLessons.empty())
				return NULL;					// port: the original reads the element before the list
			return mLessons.back();
		}

		size_t aRead = p_fread(&aChar, 1, 1, aFile);
		if ((aRead != 1) && !p_feof(aFile))
		{
			p_fclose(aFile);					// port
			return NULL;
		}

		if ((aChar == '\r') || (aChar == '\n'))
		{
			if (0 < (int) aLine.length())
			{
				aParser.SetStringSource(aLine);
				aLine = "";
				aParser.NextElement(&anElement);

				std::string aTag = UpperCaseTag(anElement.mValue);
				bool isStart = (anElement.mType == XMLElement::TYPE_START);
				bool isEnd = (anElement.mType == XMLElement::TYPE_END);

				if (isStart && (aTag == GT(0x0052c984)))				// <LESSON>
				{
					aLesson = new TutorLesson();						// 0x420bd0
					aLesson->mMaxErrors = -1;
					aLesson->mMinWPM = -1;
					aLesson->mMinTasks = -1;
					aLesson->mMinAccuracy = -1;
					continue;
				}

				if (isEnd && (aTag == GT(0x0052c984)))					// </LESSON>
				{
					mLessons.push_back(aLesson);
				}
				else if (isStart && (aTag == GT(0x0052c978)))			// <HEADINGS>
				{
					inHeadings = true;
				}
				else if (isEnd && (aTag == GT(0x0052c978)))				// </HEADINGS>
				{
					inHeadings = false;
				}
				else
				{
					bool aCheckTask = true;				// LAB_0042a286: whether a task's end, <HEADING> and <TEXT> are tried
					if (isStart)
					{
						if (inHeadings)
						{
							// any element in <HEADINGS>: its text is a heading
							XMLElement aTextElement;
							aParser.NextElement(&aTextElement);
							aHeadings.push_back(aTextElement.mValue);
							aCheckTask = false;
						}
						else if (aTag == GT(0x0052c96c))				// <DESCRIPTION>
						{
							XMLElement aTextElement;
							aParser.NextElement(&aTextElement);
							aLesson->mDescription = aTextElement.mValue;
							aCheckTask = false;
						}
						else if (aTag == GT(0x0052c960))				// <MAXERRORS>
						{
							XMLElement aTextElement;
							aParser.NextElement(&aTextElement);
							aLesson->mMaxErrors = atol(aTextElement.mValue.c_str());
							aCheckTask = false;
						}
						else if (aTag == GT(0x0052c958))				// <MINWPM>
						{
							XMLElement aTextElement;
							aParser.NextElement(&aTextElement);
							aLesson->mMinWPM = atol(aTextElement.mValue.c_str());
							aCheckTask = false;
						}
						else if (aTag == GT(0x0052c94c))				// <LESSONTIME>
						{
							XMLElement aTextElement;
							aParser.NextElement(&aTextElement);
							aLesson->mLessonTime = atol(aTextElement.mValue.c_str());
							aCheckTask = false;
						}
						else if (aTag == GT(0x0052c940))				// <MINTASKS>
						{
							XMLElement aTextElement;
							aParser.NextElement(&aTextElement);
							aLesson->mMinTasks = atol(aTextElement.mValue.c_str());
							aCheckTask = false;
						}
						else if (aTag == GT(0x0052c934))				// <MINACCURACY>
						{
							XMLElement aTextElement;
							aParser.NextElement(&aTextElement);
							aLesson->mMinAccuracy = atol(aTextElement.mValue.c_str());
							aCheckTask = false;
						}
						else if (aTag == GT(0x0052c92c))				// <TASK>
						{
							inTask = true;
							aTask = new TutorExercise();				// 0x420c10
							aTask->mCurText = 0;
							aTask->mCurChar = 0;
							aCheckTask = false;
						}
					}

					if (aCheckTask)
					{
						if (isEnd && (aTag == GT(0x0052c92c)))			// </TASK>
						{
							inTask = false;
							aLesson->mTasks.push_back(aTask);
						}

						if (inTask && isStart)
						{
							if (aTag == GT(0x0052c924))					// <HEADING>n</HEADING>: the n-th of the lesson's headings
							{
								XMLElement aTextElement;
								aParser.NextElement(&aTextElement);
								int aHeadingNum = atol(aTextElement.mValue.c_str());
								if ((aHeadingNum >= 1) && ((unsigned int) aHeadingNum <= aHeadings.size()))		// port: not checked in the original
									aTask->mHeading = aHeadings[aHeadingNum - 1];
							}
							else if (aTag == GT(0x0052c91c))			// <TEXT>
							{
								XMLParamMap::iterator aFileAttr = anElement.mAttributes.find(GT(0x0052c914));	// File
								if (aFileAttr == anElement.mAttributes.end())
								{
									// the line is the element's text
									XMLElement aTextElement;
									aParser.NextElement(&aTextElement);
									aTask->mTexts.push_back(aTextElement.mValue);
								}
								else
								{
									// a line of a text file: the number in the element, or a random one if it is -1
									std::string aTextFileName = aFileAttr->second;
									PFILE* aTextFile = p_fopen(aTextFileName.c_str(), "r");
									XMLElement aTextElement;
									aParser.NextElement(&aTextElement);
									unsigned int aLineNum = (unsigned int) atol(aTextElement.mValue.c_str());
									if (aTextFile != NULL)		// port: the original does not test it
									{
										if (aLineNum == 0xffffffff)
										{
											unsigned int aLineCount = 0;
											while (!p_feof(aTextFile))
											{
												aLineCount++;
												p_fgets(aBuffer, 0x1000, aTextFile);
											}
											p_fclose(aTextFile);
											aTextFile = p_fopen(aTextFileName.c_str(), "r");
											aLineNum = (unsigned int) gGameRand->NextNoAssert();
											aLineNum = aLineNum % aLineCount;
										}
										if (aTextFile != NULL)
										{
											if (0 < (int) aLineNum)
											{
												do
												{
													p_fgets(aBuffer, 0x1000, aTextFile);
													aLineNum = aLineNum - 1;
												} while (aLineNum != 0);
											}
											p_fgets(aBuffer, 0x1000, aTextFile);
											aTask->mTexts.push_back(std::string(aBuffer));
											p_fclose(aTextFile);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			aLine += aChar;
		}
	}
}

// 0x413bf0: a finished lesson's raw words per minute are added to the lesson's record: the best, the last, the total and the
// number of runs (the stats screen shows the total over the runs)
void UserProfile::RecordTutorWPM(int theValue, int theLesson)
{
	StatRecord& aRecord = mRecordsE0[theLesson];
	if (aRecord.mField[0] < theValue)
		aRecord.mField[0] = theValue;
	aRecord.mField[2] = aRecord.mField[2] + theValue;
	aRecord.mField[3] = aRecord.mField[3] + 1;
	aRecord.mField[1] = theValue;
}

// 0x413c50: the same for the adjusted words per minute (mField[4..7])
void UserProfile::RecordTutorAdjustedWPM(int theValue, int theLesson)
{
	StatRecord& aRecord = mRecordsE0[theLesson];
	if (aRecord.mField[4] < theValue)
		aRecord.mField[4] = theValue;
	aRecord.mField[6] = aRecord.mField[6] + theValue;
	aRecord.mField[7] = aRecord.mField[7] + 1;
	aRecord.mField[5] = theValue;
}

// 0x413cb0: the same for the accuracy (mField[8..11])
void UserProfile::RecordTutorAccuracy(int theValue, int theLesson)
{
	StatRecord& aRecord = mRecordsE0[theLesson];
	if (aRecord.mField[8] < theValue)
		aRecord.mField[8] = theValue;
	aRecord.mField[10] = aRecord.mField[10] + theValue;
	aRecord.mField[11] = aRecord.mField[11] + 1;
	aRecord.mField[9] = theValue;
}
