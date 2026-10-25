#include "PakInterface.h"
#include "../platform/VFS.h"

#include <cstring>

PFILE* p_fopen(const char* theFileName, const char* theAccess)
{
	if ((theFileName == NULL) || (theAccess == NULL))
		return NULL;

	bool writing = (strchr(theAccess, 'w') != NULL) || (strchr(theAccess, 'a') != NULL) || (strchr(theAccess, '+') != NULL);
	if (writing)
	{
		FILE* aFP = Platform::VFS::Get().Open(theFileName, theAccess);
		if (aFP == NULL)
			return NULL;
		PFILE* aPFile = new PFILE();
		aPFile->mPos = 0;
		aPFile->mEOF = false;
		aPFile->mFP = aFP;
		return aPFile;
	}

	std::vector<uint8_t> aData;
	if (!Platform::VFS::Get().ReadAll(theFileName, aData))
		return NULL;

	if (strchr(theAccess, 'b') == NULL)
	{
		// Text mode as done by the Win32 CRT: CR LF pairs become LF and Ctrl-Z marks the end of file.
		size_t aDest = 0;
		for (size_t i = 0; i < aData.size(); i++)
		{
			uint8_t aChar = aData[i];
			if (aChar == 0x1A)
				break;
			if ((aChar == '\r') && (i + 1 < aData.size()) && (aData[i + 1] == '\n'))
				continue;
			aData[aDest++] = aChar;
		}
		aData.resize(aDest);
	}

	PFILE* aPFile = new PFILE();
	aPFile->mData.swap(aData);
	aPFile->mPos = 0;
	aPFile->mEOF = false;
	aPFile->mFP = NULL;
	return aPFile;
}

int p_fclose(PFILE* theFile)
{
	if (theFile == NULL)
		return EOF;
	int aResult = 0;
	if (theFile->mFP != NULL)
		aResult = fclose(theFile->mFP);
	delete theFile;
	return aResult;
}

int p_fseek(PFILE* theFile, long theOffset, int theOrigin)
{
	if (theFile->mFP != NULL)
		return fseek(theFile->mFP, theOffset, theOrigin);

	long aPos;
	if (theOrigin == SEEK_SET)
		aPos = theOffset;
	else if (theOrigin == SEEK_CUR)
		aPos = theFile->mPos + theOffset;
	else if (theOrigin == SEEK_END)
		aPos = (long)theFile->mData.size() + theOffset;
	else
		return -1;

	if (aPos < 0)
		return -1;
	theFile->mPos = aPos;
	theFile->mEOF = false;
	return 0;
}

int p_ftell(PFILE* theFile)
{
	if (theFile->mFP != NULL)
		return (int)ftell(theFile->mFP);
	return (int)theFile->mPos;
}

size_t p_fread(void* thePtr, int theSize, int theCount, PFILE* theFile)
{
	if (theFile->mFP != NULL)
		return fread(thePtr, theSize, theCount, theFile->mFP);

	if ((theSize <= 0) || (theCount <= 0))
		return 0;

	size_t aWanted = (size_t)theSize * (size_t)theCount;
	size_t anAvail = (theFile->mPos < (long)theFile->mData.size()) ? theFile->mData.size() - (size_t)theFile->mPos : 0;
	size_t aBytes = (aWanted < anAvail) ? aWanted : anAvail;
	if (aBytes > 0)
		memcpy(thePtr, &theFile->mData[theFile->mPos], aBytes);
	theFile->mPos += (long)aBytes;
	if (aBytes < aWanted)
		theFile->mEOF = true;
	return aBytes / theSize;
}

size_t p_fwrite(const void* thePtr, int theSize, int theCount, PFILE* theFile)
{
	if (theFile->mFP == NULL)
		return 0;
	return fwrite(thePtr, theSize, theCount, theFile->mFP);
}

int p_fgetc(PFILE* theFile)
{
	if (theFile->mFP != NULL)
		return fgetc(theFile->mFP);

	if (theFile->mPos >= (long)theFile->mData.size())
	{
		theFile->mEOF = true;
		return EOF;
	}
	return theFile->mData[theFile->mPos++];
}

int p_ungetc(int theChar, PFILE* theFile)
{
	if (theFile->mFP != NULL)
		return ungetc(theChar, theFile->mFP);

	if (theChar == EOF)
		return EOF;
	if (theFile->mPos > 0)
		theFile->mData[--theFile->mPos] = (uint8_t)theChar;
	else
		theFile->mData.insert(theFile->mData.begin(), (uint8_t)theChar);
	theFile->mEOF = false;
	return (uint8_t)theChar;
}

char* p_fgets(char* thePtr, int theSize, PFILE* theFile)
{
	if (theFile->mFP != NULL)
		return fgets(thePtr, theSize, theFile->mFP);

	if (theSize <= 0)
		return NULL;

	int anIdx = 0;
	while (anIdx < theSize - 1)
	{
		int aChar = p_fgetc(theFile);
		if (aChar == EOF)
			break;
		thePtr[anIdx++] = (char)aChar;
		if (aChar == '\n')
			break;
	}

	if (anIdx == 0)
		return NULL;
	thePtr[anIdx] = 0;
	return thePtr;
}

int p_feof(PFILE* theFile)
{
	if (theFile->mFP != NULL)
		return feof(theFile->mFP);
	return theFile->mEOF ? 1 : 0;
}
