#ifndef __SPROUT2_HOST_REGISTRYKEY_H__
#define __SPROUT2_HOST_REGISTRYKEY_H__

// The port's dxdriver::RegistryKey (0x20 bytes, its constructor 0x48b1a0 (feedingfrenzy2)): a key of the registry,
// the original's HKEY, over the framework's registry (the user folder's registry.ini). The framework keeps the values
// below the application's key, Software\<the app's mRegKey>, so a key here is a path below that one, and a key outside
// it cannot be opened. The framework's registry holds values only, and a key is there while something is stored below
// it; a key is made as RegCreateKeyEx makes it by an empty binary default value, which no read of the engine sees.

#include "PersistKey.h"

namespace engine
{

class RegistryKey : public PersistKey
{
public:
	// 0x48b4f0 (slot 0x00) and 0x4905e0 (slot 0x04) (feedingfrenzy2), and 0x48b4a0 (feedingfrenzy2), which they share
	// (RegCreateKeyEx): the key at the path below the application's key ("" for that key itself), made when it is
	// not there
	explicit RegistryKey(const std::string& thePath);

	// port: the original's path below Software\ turned into the path below the application's key; false when it is
	// not inside it
	static bool pathBelowApplication(const std::string& thePath, std::string& thePathBelow);

	bool getSubKeyNames(std::vector<std::string>& theNames) override;
	bool deleteSubKey(const std::string& theName) override;
	// port: the key's mark, the default value, is left out (the engine writes no default value of its own)
	bool getValueNames(std::vector<std::string>& theNames) override;
	bool writeString(const std::string& theName, const std::string& theValue) override;
	std::string readString(const std::string& theName, const std::string& theDefault) override;

	// port: the path below the application's key
	const std::string& getPath() const { return mPath; }

private:
	// port: the framework's name of a value of this key, below the application's key
	std::string valueName(const std::string& theName) const;

	std::string mPath;  // port (the original keeps the HKEY at +0x14)
};

} // namespace engine

#endif
