#include "ModManager.h"
#include "ImageLoader.h"
#include "VFS.h"
#include "WebPersist.h"

#include <SDL.h>
#include <algorithm>
#include <cstdio>

namespace Platform
{

ModManager& ModManager::Get()
{
	static ModManager sManager;
	return sManager;
}

static bool ReadTextFile(const std::string& thePath, std::string& theOut)
{
	FILE* f = fopen(thePath.c_str(), "rb");
	if (!f)
		return false;
	char buf[4096];
	size_t n;
	theOut.clear();
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		theOut.append(buf, n);
	fclose(f);
	return true;
}

static std::string ConfigPath()
{
	return VFS::Get().GetUserDir() + "/mods.json";
}

void ModManager::LoadManifest(ModInfo& theMod)
{
	std::string manifestPath = VFS::FindCaseInsensitive(theMod.mPath, "mod.json");
	if (manifestPath.empty())
		return;
	std::string text;
	if (!ReadTextFile(manifestPath, text))
		return;
	JsonValue m;
	std::string err;
	if (!ParseJson(text, m, &err) || !m.IsObject())
	{
		theMod.mError = "mod.json: " + (err.empty() ? std::string("not an object") : err);
		SDL_Log("mod %s: %s", theMod.mId.c_str(), theMod.mError.c_str());
		return;
	}
	theMod.mHasManifest = true;
	theMod.mManifest = m;
	if (m["id"].IsString() && !m["id"].AsString().empty())
		theMod.mId = m["id"].AsString();
	theMod.mName = m["name"].AsString(theMod.mId);
	theMod.mVersion = m["version"].AsString("");
	theMod.mAuthor = m["author"].AsString("");
	theMod.mDescription = m["description"].AsString("");
	theMod.mPriority = m["priority"].AsInt(0);
	theMod.mAffectsScores = m["affectsScores"].AsBool(false) || m.Has("tunables") || m.Has("rules");
}

void ModManager::Refresh()
{
	mMods.clear();
	std::vector<std::string> names = ListDirectory(mModsDir);
	std::sort(names.begin(), names.end());
	for (const std::string& n : names)
	{
		std::string path = mModsDir + "/" + n;
		if (!IsDirectory(path))
			continue;
		ModInfo mod;
		mod.mId = n;
		mod.mName = n;
		mod.mPath = path;
		LoadManifest(mod);
		mMods.push_back(mod);
	}

	// Saved order / enabled flags
	std::string text;
	JsonValue cfg;
	std::vector<std::string> order;
	if (ReadTextFile(ConfigPath(), text) && ParseJson(text, cfg) && cfg.IsObject())
	{
		for (const JsonValue& v : cfg["order"].Items())
			order.push_back(v.AsString());
		for (const JsonValue& v : cfg["disabled"].Items())
			for (ModInfo& m : mMods)
				if (m.mId == v.AsString())
					m.mEnabled = false;
	}
	auto rank = [&](const ModInfo& m) {
		auto it = std::find(order.begin(), order.end(), m.mId);
		return it == order.end() ? (int)order.size() : (int)(it - order.begin());
	};
	std::stable_sort(mMods.begin(), mMods.end(), [&](const ModInfo& a, const ModInfo& b) {
		if (a.mPriority != b.mPriority)
			return a.mPriority > b.mPriority;
		return rank(a) < rank(b);
	});
	Apply();
}

std::vector<const ModInfo*> ModManager::GetEnabledMods() const
{
	std::vector<const ModInfo*> out;
	for (const ModInfo& m : mMods)
		if (m.mEnabled)
			out.push_back(&m);
	return out;
}

void ModManager::SetEnabled(const std::string& theId, bool theEnabled)
{
	for (ModInfo& m : mMods)
		if (m.mId == theId)
			m.mEnabled = theEnabled;
}

void ModManager::Move(const std::string& theId, int theDelta)
{
	for (size_t i = 0; i < mMods.size(); i++)
	{
		if (mMods[i].mId != theId)
			continue;
		long j = (long)i + theDelta;
		if (j >= 0 && j < (long)mMods.size())
			std::swap(mMods[i], mMods[(size_t)j]);
		return;
	}
}

void ModManager::SaveConfig()
{
	JsonValue cfg = JsonValue::MakeObject();
	JsonValue order = JsonValue::MakeArray(), disabled = JsonValue::MakeArray();
	for (const ModInfo& m : mMods)
	{
		order.Push(JsonValue(m.mId));
		if (!m.mEnabled)
			disabled.Push(JsonValue(m.mId));
	}
	cfg.Set("order", order);
	cfg.Set("disabled", disabled);
	MakeDirs(VFS::Get().GetUserDir());
	FILE* f = fopen(ConfigPath().c_str(), "wb");
	if (f)
	{
		std::string s = cfg.Dump();
		fwrite(s.data(), 1, s.size(), f);
		fclose(f);
	}
}

void ModManager::FlattenInto(const JsonValue& theValue, const std::string& thePrefix)
{
	if (theValue.IsObject())
	{
		for (const auto& kv : theValue.Members())
			FlattenInto(kv.second, thePrefix.empty() ? kv.first : thePrefix + "." + kv.first);
	}
	else if (!thePrefix.empty())
		mTunables[thePrefix] = theValue;
}

void ModManager::Apply()
{
	std::vector<VFSRoot> roots;
	ClearImageScaleOverrides();
	ClearImagePatches();
	mTunables.clear();
	mStrings.clear();
	mPropertyFiles.clear();
	std::vector<std::string> scoreMods;

	std::vector<const ModInfo*> enabled = GetEnabledMods();
	for (const ModInfo* m : enabled)
		roots.push_back({m->mPath, m->mId});
	VFS::Get().SetModRoots(roots);

	// "files": { "<game path>": "<file in mod>" |
	//            { "file": ..., "order": N, "start": sec, "loopStart": sec, "volume": x } }
	// Lower-priority mods are visited first so higher-priority remaps overwrite them.
	std::map<std::string, VFSRemap> remaps;
	for (auto it = enabled.rbegin(); it != enabled.rend(); ++it)
	{
		const ModInfo& m = **it;
		for (const auto& kv : m.mManifest["files"].Members())
		{
			const JsonValue& v = kv.second;
			std::string file = v.IsString() ? v.AsString() : v["file"].AsString("");
			std::string real = file.empty() ? "" : VFS::FindCaseInsensitive(m.mPath, file);
			if (real.empty())
			{
				SDL_Log("mod %s: file '%s' for '%s' not found", m.mId.c_str(), file.c_str(), kv.first.c_str());
				continue;
			}
			VFSRemap r;
			r.mRealPath = real;
			r.mModId = m.mId;
			r.mStartOrder = v["order"].AsInt(-1);
			r.mStartTime = v["start"].AsNumber(0);
			r.mLoopStart = v["loopStart"].AsNumber(0);
			r.mVolume = v["volume"].AsNumber(1.0);
			remaps[kv.first] = r;
		}
	}
	VFS::Get().SetRemaps(remaps);

	// port(variant): a mod that replaces data/diff.txt (a file in the mod or a "files" remap) changes the
	// level parameters, so it counts for the score key
	std::string diffMod;
	VFS::Get().Resolve("data/diff.txt", &diffMod);
	mImageOps.clear();

	// Data merges run lowest priority first so higher-priority mods win.
	for (auto it = mMods.rbegin(); it != mMods.rend(); ++it)
	{
		ModInfo& m = *it;
		if (!m.mEnabled)
			continue;
		const JsonValue& man = m.mManifest;
		if (man["imageScale"].AsInt(1) > 1)
			SetModDefaultImageScale(m.mId, man["imageScale"].AsInt(1));
		for (const auto& kv : man["images"].Members())
			if (kv.second["scale"].AsInt(1) > 1)
				SetImageScaleOverride(m.mId, kv.first, kv.second["scale"].AsInt(1));

		// "imagePatches": [{"target": "images/gemsheet6", "image": "patches/x@2x.png", "x": 0, "y": 520,
		//                   "scale": 2 (optional, else from the @Nx suffix), "mode": "replace" | "over"}]
		for (const JsonValue& p : man["imagePatches"].Items())
		{
			std::string image = p["image"].AsString("");
			std::string real = image.empty() ? "" : VFS::FindCaseInsensitive(m.mPath, image);
			if (real.empty() || !p["target"].IsString())
			{
				SDL_Log("mod %s: image patch '%s' not found", m.mId.c_str(), image.c_str());
				continue;
			}
			int scale = p["scale"].AsInt(0);
			if (scale <= 0)
			{
				size_t at = image.rfind('@');
				scale = at != std::string::npos ? atoi(image.c_str() + at + 1) : 1;
			}
			AddImagePatch(p["target"].AsString(), real, p["x"].AsInt(0), p["y"].AsInt(0), std::max(1, scale),
				p["mode"].AsString("replace") != "over");
		}

		const JsonValue& strings = man["strings"];
		if (strings.IsObject())
		{
			for (const auto& kv : strings.Members())
				mStrings[kv.first] = kv.second.AsString();
		}
		else if (strings.IsArray())
		{
			// Store host paths: two mods may both ship "properties/strings.xml".
			for (const JsonValue& v : strings.Items())
			{
				std::string real = VFS::FindCaseInsensitive(m.mPath, v.AsString());
				if (real.empty())
					SDL_Log("mod %s: strings file '%s' not found", m.mId.c_str(), v.AsString().c_str());
				else
					mPropertyFiles.push_back(real);
			}
		}

		const JsonValue& tun = man["tunables"];
		m.mTunablesJson = JsonValue();
		if (tun.IsObject())
		{
			FlattenInto(tun, "");
			m.mTunablesJson = tun;
		}
		else if (tun.IsString())
		{
			std::string p = VFS::FindCaseInsensitive(m.mPath, tun.AsString());
			std::string text;
			JsonValue t;
			std::string err;
			if (!p.empty() && ReadTextFile(p, text) && ParseJson(text, t, &err))
			{
				FlattenInto(t, "");
				m.mTunablesJson = t;
			}
			else
				SDL_Log("mod %s: cannot read tunables %s %s", m.mId.c_str(), tun.AsString().c_str(), err.c_str());
		}

		// port(variant): "rules" (inline, or a JSON file in the mod like "tunables"), "features" and
		// "imageOps"; GameConfig::Load merges the rules and features (game/GameRules.h)
		m.mHasRules = man.Has("rules");
		m.mHasFeatures = man.Has("features");
		m.mHasImageOps = man.Has("imageOps");
		m.mOverridesDiff = !diffMod.empty() && (diffMod == m.mId);
		m.mRulesJson = man["rules"];
		m.mRulesError.clear();
		if (m.mRulesJson.IsString())
		{
			std::string name = m.mRulesJson.AsString();
			std::string p = VFS::FindCaseInsensitive(m.mPath, name);
			std::string text;
			JsonValue r;
			std::string err;
			if (p.empty())
				m.mRulesError = "rules file " + name + ": not found";
			else if (!ReadTextFile(p, text))
				m.mRulesError = "rules file " + name + ": cannot be read";
			else if (!ParseJson(text, r, &err))
				m.mRulesError = "rules file " + name + ": " + err;
			else
				m.mRulesJson = r;
			if (!m.mRulesError.empty())
				SDL_Log("mod %s: %s", m.mId.c_str(), m.mRulesError.c_str());
		}
		m.mFeaturesJson = man["features"];
		for (const JsonValue& op : man["imageOps"].Items())
			mImageOps.push_back({m.mId, op});

		if (m.AffectsScores())
			scoreMods.push_back(m.mId + "@" + m.mVersion);
	}

	// Stable key for separate high-score tables (FNV-1a over the gameplay-affecting mods).
	mScoreKey.clear();
	if (!scoreMods.empty())
	{
		std::sort(scoreMods.begin(), scoreMods.end());
		uint32_t h = 2166136261u;
		for (const std::string& s : scoreMods)
			for (unsigned char c : s + ";")
				h = (h ^ c) * 16777619u;
		char buf[16];
		snprintf(buf, sizeof(buf), "%08x", h);
		mScoreKey = buf;
	}
	SDL_Log("mods: %s", GetSummary().c_str());
}

bool ModManager::HasTunable(const std::string& theKey) const
{
	return mTunables.count(theKey) != 0;
}

double ModManager::GetTunable(const std::string& theKey, double theDefault) const
{
	auto it = mTunables.find(theKey);
	return it == mTunables.end() ? theDefault : it->second.AsNumber(theDefault);
}

int ModManager::GetTunableInt(const std::string& theKey, int theDefault) const
{
	auto it = mTunables.find(theKey);
	return it == mTunables.end() || !it->second.IsNumber() ? theDefault : (int)it->second.AsNumber();
}

bool ModManager::GetTunableBool(const std::string& theKey, bool theDefault) const
{
	auto it = mTunables.find(theKey);
	return it == mTunables.end() ? theDefault : it->second.AsBool(theDefault);
}

std::vector<double> ModManager::GetTunableArray(const std::string& theKey, const std::vector<double>& theDefault) const
{
	auto it = mTunables.find(theKey);
	if (it == mTunables.end() || !it->second.IsArray())
		return theDefault;
	std::vector<double> out;
	for (const JsonValue& v : it->second.Items())
		out.push_back(v.AsNumber());
	return out.empty() ? theDefault : out;
}

bool ModManager::HasRulesOrFeatures() const
{
	for (const ModInfo* m : GetEnabledMods())
		if (m->mHasRules || m->mHasFeatures)
			return true;
	return false;
}

void ModManager::WriteStatus(const std::string& theText)
{
	std::string path = VFS::Get().GetUserDir() + "/mods-status.json";
	if (theText.empty())
	{
		mStatusWritten.clear();
		// Only a stale file from an earlier start is removed: a user dir without one stays untouched
		if (IsFile(path))
		{
			remove(path.c_str());
			SyncUserData();
		}
		return;
	}
	if (theText == mStatusWritten)
		return;
	MakeDirs(VFS::Get().GetUserDir());
	FILE* f = fopen(path.c_str(), "wb");
	if (!f)
	{
		SDL_Log("mods: cannot write %s", path.c_str());
		return;
	}
	fwrite(theText.data(), 1, theText.size(), f);
	fclose(f);
	mStatusWritten = theText;
	SyncUserData();
}

std::string ModManager::GetSummary() const
{
	std::string s;
	for (const ModInfo* m : GetEnabledMods())
		s += (s.empty() ? "" : ", ") + m->mName;
	return s.empty() ? "none" : s;
}

}
