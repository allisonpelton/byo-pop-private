// port(mods): the fixes mod's toggles (see Fixes.h).

#include "Fixes.h"
#include "platform/ModManager.h"

using namespace Sexy;

bool Sexy::FixEnabled(const char* theFix)
{
	static const char* const kFixesMod = "fixes";

	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (aMod->mId != kFixesMod)
			continue;

		// a toggle with no value stored has its default, which mod.json gives as on; a name mod.json lacks gives no value
		Platform::JsonValue aValue = Platform::ModManager::Get().GetLiveValue(kFixesMod, theFix);
		return aValue.IsBool() && aValue.AsBool(false);
	}
	return false;
}
