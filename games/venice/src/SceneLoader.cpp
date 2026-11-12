// The scene loader (0x438160-0x438d00) and the config loader (0x439720).

#include "SceneLoader.h"

#include "Emitter.h"
#include "ImageRecord.h"
#include "Level.h"
#include "ObjectFactory.h"
#include "Profiles.h"
#include "SceneFiles.h"
#include "SceneObject.h"
#include "SceneTag.h"
#include "TextObject.h"
#include "Tuning.h"
#include "VeniceApp.h"
#include "VeniceGlobals.h"
#include "VeniceSound.h"
#include "platform/VFS.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

CursorList<SceneFile> gGameScenes;
CursorList<SceneFile> gRelaxedScenes;
CursorList<SceneFile> gTrickScenes;

// the original keeps the objects that are open in a stack of 256, which no file comes near
static const int MaxOpenObjects = 256;

// 0x442f60
bool ReadGameFile(const char* theName, std::string* theText)
{
	std::vector<uint8_t> aBytes;
	if (!Platform::VFS::Get().ReadAll(theName, aBytes))
		return false;
	theText->assign(reinterpret_cast<const char*>(aBytes.data()), aBytes.size());
	return true;
}

// 0x428580: <stage ms= loop_start= loop_end= visible= name=>
static void LoadStageTag(SceneObject* theObject, const SceneTag* theTag)
{
	int aVisible = 1;
	if (theTag->Has("visible"))
		aVisible = theTag->GetInt("visible");
	const char* aName = theTag->GetString("name");
	int aLoopEnd = theTag->GetInt("loop_end");
	int aLoopStart = theTag->GetInt("loop_start");
	float aMilliseconds = (float)theTag->GetFloat("ms");
	theObject->AddStage(aMilliseconds, aLoopStart, aLoopEnd, aVisible, aName);
}

// 0x4381a0
SceneObject* LoadSceneObjects(const char* theText, SceneObject* theParent)
{
	// the objects that are open: [1] is the one the text's objects are added to, and each <object> tag opens another above it
	SceneObject* anOpenObjects[MaxOpenObjects + 2] = {};
	if (theParent != nullptr)
		anOpenObjects[1] = theParent;
	int anOpenCount = 1;
	SceneObject* aFirstObject = nullptr;
	// the type attribute's text is kept from tag to tag: a tag that has none sees the last tag's
	std::string aType;

	SceneTag aTag;
	const char* aText = ReadTag(theText, &aTag);
	do {
		// a text that has no tag left ends here (the original reads past the end of its buffer)
		if (aTag.mName.empty() && *aText == '\0')
			break;
		if (aTag.Has("type"))
			aType = aTag.GetString("type");

		if (aTag.mName == "object" && aType == "level") {
			Level* aLevel = new Level();
			gScenes.Append(aLevel);
			anOpenObjects[anOpenCount + 1] = aLevel;
			aLevel->Load(aText, &aTag);
			aLevel->mType = ObjectTypeLevel;
			anOpenCount++;
		} else if (aTag.mName == "object") {
			std::string anObjectType = aTag.Has("type") ? aTag.GetString("type") : "generic";
			SceneObject* aNewObject = NewObjectByTypeName(anObjectType.c_str());
			SceneObject* anAddedObject = anOpenObjects[anOpenCount]->NewChild(aNewObject);
			anOpenObjects[anOpenCount + 1] = anAddedObject;
			anAddedObject->Load(aText, &aTag);
			if (anOpenCount == 1)
				aFirstObject = aNewObject;
			anOpenCount++;
		}

		if (aTag.mName == "/object") {
			anOpenObjects[anOpenCount]->OnObjectClosed();
			anOpenCount--;
			if (anOpenCount < 2)
				return aFirstObject;
		}
		if (aTag.mName == "load_object_from_file") {
			// the original also keeps the file's name in the object, for the level editor
			LoadSceneFile(aTag.GetString("file"), anOpenObjects[anOpenCount]);
		}
		if (aTag.mName == "stage")
			LoadStageTag(anOpenObjects[anOpenCount], &aTag);
		if (aTag.mName == "emitter_stage")
			static_cast<Emitter*>(anOpenObjects[anOpenCount])->LoadStageTag(&aTag);
		if (aTag.mName == "attribute")
			anOpenObjects[anOpenCount]->LoadAttributeTag(aText, &aTag);
		if (aTag.mName == "filter")
			anOpenObjects[anOpenCount]->LoadFilterTag(aText, &aTag);
		if (aTag.mName == "event")
			anOpenObjects[anOpenCount]->LoadEventTag(aText, &aTag);
		if (aTag.mName == "set_game_mode")
			CurrentProfile().mMode = aTag.GetInt("number");

		aText = ReadTag(aText, &aTag);
	} while (anOpenCount > 1);
	return anOpenObjects[anOpenCount + 1];
}

// 0x438160
SceneObject* LoadSceneFile(const char* theName, SceneObject* theParent)
{
	std::string aText;
	if (!ReadGameFile(theName, &aText))
		return nullptr;
	return LoadSceneObjects(aText.c_str(), theParent);
}

// 0x4407c0 is in the app's code (the tick base is the app's)

// 0x438bf0
void PushSceneByName(const char* theName)
{
	if (!gGameScenes.Begin())
		return;
	while (strcmp(gGameScenes.mCursor->mName, theName) != 0) {
		if (!gGameScenes.Next())
			return;
	}
	LoadSceneObjects(gGameScenes.mCursor->mText.c_str(), nullptr);
	if (gScenes.mTail != nullptr)
		static_cast<Level*>(gScenes.mTail)->mNumber = gGameScenes.mCursor->mNumber;
	ResetTickBase();
}

void PushSceneByText(const char* theText)
{
	const long FirstSceneNumber = 1;
	const long LastSceneNumber = 1999;
	long aNumber = atol(theText);
	if (aNumber < FirstSceneNumber || aNumber > LastSceneNumber)
		PushSceneByName(theText);
	else
		PushSceneByNumber((int)aNumber);
}

bool SceneFileExists(const char* theName)
{
	bool aFound = gGameScenes.Begin();
	while (aFound) {
		if (strcmp(gGameScenes.mCursor->mName, theName) == 0)
			return true;
		aFound = gGameScenes.Next();
	}
	return false;
}

// 0x438c90
void PushSceneByNumber(int theNumber)
{
	CursorList<SceneFile>* aList = CurrentProfile().mMode == GameModeJourney ? &gRelaxedScenes : &gGameScenes;
	if (theNumber < 1000) {
		bool aFound = aList->Begin();
		while (aFound) {
			if ((int)aList->mCursor->mNumber == theNumber) {
				LoadSceneObjects(aList->mCursor->mText.c_str(), nullptr);
				static_cast<Level*>(gScenes.mTail)->mNumber = aList->mCursor->mNumber;
				ResetTickBase();
				return;
			}
			aFound = aList->Next();
		}
	} else {
		bool aFound = gTrickScenes.Begin();
		while (aFound) {
			if ((int)gTrickScenes.mCursor->mNumber == theNumber - 1000) {
				LoadSceneObjects(gTrickScenes.mCursor->mText.c_str(), nullptr);
				static_cast<Level*>(gScenes.mTail)->mNumber = gTrickScenes.mCursor->mNumber;
				ResetTickBase();
				return;
			}
			aFound = gTrickScenes.Next();
		}
	}
	ResetTickBase();
}

// The first tag of a scene file names it (the name the game pushes it by) and may give its title
static void ReadSceneFileNames(SceneFile* theFile)
{
	SceneTag aFirstTag;
	ReadTag(theFile->mText.c_str(), &aFirstTag);
	const char* aName = aFirstTag.GetString("name");
	strcpy(theFile->mName, aName == nullptr ? "" : aName);
	if (aFirstTag.Has("title"))
		strcpy(theFile->mTitle, aFirstTag.GetString("title"));
	else
		theFile->mTitle[0] = 0;
}

// The config being read: its text, the tag the next step starts with and the text after that tag
static std::string sConfigText;
static SceneTag sConfigTag;
static const char* sConfigNext = nullptr;

// 0x439720 (its start)
void BeginLoadConfig()
{
	sConfigText.clear();
	ReadGameFile("config.xml", &sConfigText);
	sConfigNext = ReadTag(sConfigText.c_str(), &sConfigTag);
}

// 0x439720
bool ContinueLoadConfig(unsigned theMilliseconds)
{
	unsigned aStart = gApp->GetClockTime();
	while (sConfigTag.mName != "/game" && !(sConfigTag.mName.empty() && *sConfigNext == '\0')) {
		if (gApp->GetClockTime() - aStart >= theMilliseconds)
			return false;
		const SceneTag& aTag = sConfigTag;
		if (aTag.mName == "loading")
			gLoadState = aTag.GetInt("percent");
		if (aTag.mName == "global")
			LoadTuningTag(&aTag);
		if (aTag.mName == "image")
			AcquireImageRecord(aTag.GetString("name"), aTag.GetInt("frame_width"), aTag.GetInt("frame_height"),
				aTag.GetInt("num_frames"), aTag.GetInt("fps"));
		if (aTag.mName == "font")
			AcquireFont(aTag.GetString("name"), aTag.GetString("character_set"), aTag.GetInt("letter_height"));
		if (aTag.mName == "kern")
			AddFontKerning(aTag.GetString("char"), (float)aTag.GetFloat("left"), (float)aTag.GetFloat("right"));
		if (aTag.mName == "sound")
			LoadSound(aTag.GetString("name"), aTag.GetInt("loop") != 0, false);

		if (aTag.mName == "level_file" || aTag.mName == "level_file_game" || aTag.mName == "level_file_relaxed") {
			// the levels are numbered from 1 in the order of the file; a plain level_file (a screen) has no number
			CursorList<SceneFile>& aList = aTag.mName == "level_file_relaxed" ? gRelaxedScenes : gGameScenes;
			aList.Append(new SceneFile());
			SceneFile* aFile = aList.mCursor;
			if (aTag.mName == "level_file")
				aFile->mNumber = 0;
			else
				aFile->mNumber = aFile->mPrev == nullptr ? 1 : aFile->mPrev->mNumber + 1;
			ReadGameFile(aTag.GetString("name"), &aFile->mText);
			ReadSceneFileNames(aFile);
		}
		if (aTag.mName == "level_file_trick") {
			gTrickScenes.Append(new SceneFile());
			SceneFile* aFile = gTrickScenes.mCursor;
			aFile->mNumber = aFile->mPrev == nullptr ? 0 : aFile->mPrev->mNumber + 1;
			ReadGameFile(aTag.GetString("name"), &aFile->mText);
			ReadSceneFileNames(aFile);
		}
		sConfigNext = ReadTag(sConfigNext, &sConfigTag);
	}
	PushSceneByName("main_menu");
	return true;
}
