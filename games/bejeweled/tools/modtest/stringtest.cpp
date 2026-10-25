// Loads properties through SexyAppBase with the configured mods and prints selected strings.
//   stringtest <gamedir> <modsdir> <userdir> <id> [<id> ...]
#include "SexyAppBase.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"

#include <cstdio>

int main(int argc, char** argv)
{
	if (argc < 5)
	{
		printf("usage: stringtest <gamedir> <modsdir> <userdir> <id>...\n");
		return 2;
	}
	Platform::VFS::Get().SetBaseDir(argv[1]);
	Platform::VFS::Get().SetUserDir(argv[3]);
	Platform::ModManager::Get().SetModsDir(argv[2]);
	Platform::ModManager::Get().Refresh();
	Sexy::SexyAppBase* anApp = new Sexy::SexyAppBase();
	if (!anApp->LoadProperties())
		return 1;
	for (int i = 4; i < argc; i++)
		printf("%s = \"%s\"\n", argv[i], anApp->GetString(argv[i]).c_str());
	return 0;
}
