// The type names of the data files and the classes they make (0x4381a0, and the table of the game's own classes at 0x4076b0).

#include "ObjectFactory.h"

#include "Cursor.h"
#include "Emitter.h"
#include "InputBox.h"
#include "Layer.h"
#include "SceneObject.h"
#include "TextObject.h"

#include <cstring>

namespace
{
	struct ClassEntry
	{
		const char* mName;
		SceneObject* (*mCreate)();
	};

	template <class T> SceneObject* Create()
	{
		return new T();
	}

	// the order of the original's lookups: the engine's classes, then the game's
	const ClassEntry kClasses[] = {
		{"text", Create<TextObject>},
		{"input_box", Create<InputBox>},
		{"layer", Create<Layer>},
		{"cursor", Create<Cursor>},
		{"emitter", Create<Emitter>},
	};
}

SceneObject* NewObjectByTypeName(const char* theTypeName)
{
	for (const ClassEntry& anEntry : kClasses) {
		if (strcmp(theTypeName, anEntry.mName) == 0)
			return anEntry.mCreate();
	}
	return new SceneObject();
}
