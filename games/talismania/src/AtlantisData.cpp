#include "AtlantisData.h"

#include "AtlantisTheme.h"
#include "Engine.h"
#include "BoardDataReader.h"
#include "LevelDataReader.h"
#include "Random.h"

#include <set>

std::vector<std::string> splitList(const std::string& theValue)
{
	std::vector<std::string> aList;
	if (theValue.empty())
		return aList;
	std::string aToken;
	int aDepth = 0;
	for (size_t i = 0; i < theValue.size(); i++)
	{
		char aChar = theValue[i];
		if (aChar == '[' || aChar == '(')
			aDepth++;
		else if ((aChar == ']' || aChar == ')') && aDepth > 0)
			aDepth--;
		if (aChar == ',' && aDepth == 0)
		{
			aList.push_back(aToken);
			aToken.clear();
		}
		else
			aToken += aChar;
	}
	aList.push_back(aToken);
	return aList;
}

namespace
{

std::vector<std::string> getList(const engine::Params& theParams, const char* theName,
								 const std::vector<std::string>& theDefault)
{
	if (!theParams.has(theName))
		return theDefault;
	return splitList(theParams.getString(theName, ""));
}

// Reads the elements of the files into the registries. The element being filled is the last one opened of its kind;
// the child elements (layer, region, randcoin, cursor, entry, diffmode, load) add to it.
class DataHandler : public engine::AbstractDocHandler
{
public:
	explicit DataHandler(AtlantisData& theData) : mData(theData), mBoards(theData), mLevels(theData) {}

	void startElement(const std::string& theName, engine::Params& theAttributes) override
	{
		const engine::Params& p = theAttributes;
		if (theName == "atlantisdata" || theName == "randomcoins")
			return;
		if (mBoards.startElement(theName, p))
			return;
		if (mLevels.startElement(theName, p))
			return;
		if (theName == "scene")
		{
			mScene = SceneClass();
			mScene.name = p.getString("name", "");
			mScene.layerOrder = getList(p, "layerorder", mScene.layerOrder);
			mScene.defaultDarkenLayer = p.getString("defaultdarkenlayer", "");
			mScene.defaultRegion = p.getString("defaultregion", "");
		}
		else if (theName == "layer")
			readLayer(p);
		else if (theName == "region")
		{
			std::string aName = p.getString("name", "");
			mScene.regions[aName] = p.getFloatPair("pos", engine::Vector2f());
		}
		else if (theName == "gameoptions")
		{
			mGameOptions = GameMgrClass();
			std::string aBase = p.getString("baseclass", "");
			if (!aBase.empty())
			{
				std::map<std::string, GameMgrClass>::iterator aBaseIt = mData.gameOptions.find(aBase);
				if (aBaseIt == mData.gameOptions.end())
					engine::getEngine()->log("GAMEMGR CLASS %s CANNOT FIND BASE CLASS %s\n",
											 p.getString("name", "").c_str(), aBase.c_str());
				else
					mGameOptions = aBaseIt->second;
			}
			readGameOptions(p);
		}
		else if (theName == "randcoin")
		{
			RandomCoin aCoin;
			aCoin.anim = p.getString("anim", "");
			aCoin.value = p.getInt("value", 0);
			aCoin.weight = p.getFloat("weight", 0);
			if (!aCoin.anim.empty() && aCoin.value > 0 && aCoin.weight > 0)
				mGameOptions.randomCoins.push_back(aCoin);
		}
		else if (theName == "appoptions")
		{
			mApp = AppClass();
			readAppOptions(p);
		}
		else if (theName == "difficultymode")
		{
			DifficultyModeClass aMode;
			aMode.name = p.getString("name", "");
			aMode.heroGameMgrClass = p.getString("herogamemgrclass", "");
			aMode.zenGameMgrClass = p.getString("zengamemgrclass", "");
			mApp.difficultyModes[aMode.name] = aMode;
		}
		else if (theName == "debugmanager")
		{
			DebugManagerClass aManager;
			aManager.name = p.getString("name", "");
			aManager.sceneLayer = p.getString("scenelayer", "");
			mData.debugManagers[aManager.name] = aManager;
		}
		else if (theName == "mousecursormgr")
		{
			mCursors = MouseCursorMgrClass();
			mCursors.name = p.getString("name", "");
		}
		else if (theName == "cursor")
		{
			CursorClass aCursor;
			aCursor.viewModes = getList(p, "viewmodes", aCursor.viewModes);
			aCursor.resourceId = p.getInt("resourceid", 0);
			aCursor.hardwareCursorImg = p.getString("hardwarecursorimg", "");
			aCursor.transparentColor = p.getString("transparentcolor", "");
			mCursors.cursors.push_back(aCursor);
		}
		else if (theName == "highscorescreen")
		{
			mHighScores = HighScoreScreenClass();
			mHighScores.name = p.getString("name", "");
			mHighScores.layout = p.getString("layout", "");
			mHighScores.maxEntries = p.getInt("maxentries", 12);
		}
		else if (theName == "hidecomponent")
			mHighScores.hideComponents.push_back(p.getString("name", ""));
		else if (theName == "entry")
		{
			HighScoreEntryClass anEntry;
			anEntry.name = p.getString("name", "");
			if (anEntry.name.empty())
				engine::getEngine()->log("NAMELESS ENTRY\n");
			// The default is drawn even when the attribute is there: one draw of the C runtime's rand per entry
			int aDefaultScore = (engine::crtRand() % 0x76d + 100) * 10;
			anEntry.score = p.getInt("score", aDefaultScore);
			mHighScores.entries.push_back(anEntry);
		}
		else if (theName == "gamemode")
		{
			mGameMode = GameModeClass();
			mGameMode.name = p.getString("name", "");
			mGameMode.defaultDiffMode = p.getString("defaultdiffmode", "medium");
			mGameMode.timedMode = p.getBool("timedmode", false);
		}
		else if (theName == "diffmode")
		{
			std::string aName = p.getString("name", "");
			std::string aGameClass = p.getString("gameclass", "");
			if (aName.empty() || aGameClass.empty())
				engine::getEngine()->log("EMPTY GAME CLASS OR DIFF MODE \"%s\", \"%s\"\n", aGameClass.c_str(),
										 aName.c_str());
			else
				mGameMode.diffModes[aName] = aGameClass;
		}
		else if (theName == "datasetloader")
		{
			mDataSet = DataSetLoaderClass();
			std::string aBase = p.getString("baseclass", "");
			if (!aBase.empty())
			{
				std::map<std::string, DataSetLoaderClass>::iterator aBaseIt = mData.dataSets.find(aBase);
				if (aBaseIt == mData.dataSets.end())
					engine::getEngine()->log("DATASETLOADER CLASS %s CANNOT FIND BASE CLASS %s\n",
											 p.getString("name", "").c_str(), aBase.c_str());
				else
					mDataSet = aBaseIt->second;
			}
			mDataSet.name = p.getString("name", "");
			mDataSet.reportUnusedOnShutdown = p.getBool("reportunusedonshutdown", mDataSet.reportUnusedOnShutdown);
		}
		else if (theName == "load")
		{
			DataSetFile aFile;
			aFile.file = p.getString("file", "");
			aFile.description = p.getString("description", "");
			aFile.theme = p.getBool("theme", false);
			aFile.noUpdate = p.getBool("noupdate", false);
			aFile.wait = p.getBool("wait", true);
			mDataSet.files.push_back(aFile);
		}
		else if (mReportedKinds.insert(theName).second)
			engine::getEngine()->log("Data element \"%s\" not yet translated\n", theName.c_str());
	}

	void endElement(const std::string& theName) override
	{
		if (mBoards.endElement(theName))
			return;
		if (mLevels.endElement(theName))
			return;
		if (theName == "scene")
			mData.scenes[mScene.name] = mScene;
		else if (theName == "gameoptions")
			mData.gameOptions[mGameOptions.name] = mGameOptions;
		else if (theName == "appoptions")
			mData.appOptions[mApp.name] = mApp;
		else if (theName == "mousecursormgr")
			mData.mouseCursorMgrs[mCursors.name] = mCursors;
		else if (theName == "highscorescreen")
			mData.highScoreScreens[mHighScores.name] = mHighScores;
		else if (theName == "gamemode")
			mData.gameModes[mGameMode.name] = mGameMode;
		else if (theName == "datasetloader")
			mData.dataSets[mDataSet.name] = mDataSet;
	}

private:
	// 0x5c4950
	void readLayer(const engine::Params& p)
	{
		SceneLayerClass aLayer;
		aLayer.name = p.getString("name", "");
		aLayer.baseOffset = p.getFloatPair("baseoffset", engine::Vector2f());
		aLayer.baseRegion = p.getString("baseregion", mScene.defaultRegion);
		aLayer.followCamera = p.getBool("followcamera", false);
		aLayer.darkenLayer = p.getBool("darkenlayer", false);
		aLayer.darkenTime = p.getFloat("darkentime", 0.75f);
		aLayer.tieToRegion = p.getString("tietoregion", "");
		aLayer.darkenOpacity = p.getFloat("darkenopacity", 0.6f);
		aLayer.fullScreenOnly = p.getBool("fullscreenonly", false);
		engine::RectI aClip = p.getRect("clip", engine::RectI());
		aLayer.clip = engine::RectF((float)aClip.left, (float)aClip.top, (float)aClip.right, (float)aClip.bottom);
		bool aIsDefault = p.getBool("default", false);
		if (aIsDefault || mScene.defaultLayer.empty())
			mScene.defaultLayer = aLayer.name;
		mScene.layers[aLayer.name] = aLayer;
	}

	// 0x5c6c20: each attribute keeps the field's value (a base class's) when it is not there
	void readGameOptions(const engine::Params& p)
	{
		GameMgrClass& g = mGameOptions;
		g.name = p.getString("name", "");
		g.dataSet = p.getString("dataset", g.dataSet);
		g.preChapterDelay = p.getFloat("prechapterdelay", g.preChapterDelay);
		g.postChapterDelay = p.getFloat("postchapterdelay", g.postChapterDelay);
		g.autoPauseOnBlur = p.getBool("autopauseonblur", g.autoPauseOnBlur);
		g.tooltipsAllowed = p.getBool("tooltipsallowed", g.tooltipsAllowed);
		g.splatsFiles = getList(p, "splatsfiles", g.splatsFiles);
		g.hudClass = p.getString("hudclass", g.hudClass);
		g.mapClass = p.getString("mapclass", g.mapClass);
		g.journalClass = p.getString("journalclass", g.journalClass);
		g.growStarImg = p.getString("growstarimg", g.growStarImg);
		g.growStarFx = p.getString("growstarfx", g.growStarFx);
		g.growStarSound = p.getString("growstarsound", g.growStarSound);
		g.growStarSpeedMult = p.getFloat("growstarspeedmult", g.growStarSpeedMult);
		g.inGameMenuMusic = p.getString("ingamemenumusic", g.inGameMenuMusic);
		g.inGameMenuLayout = p.getString("ingamemenulayout", g.inGameMenuLayout);
		g.totalWorldPointsAvailable = p.getInt("totalworldpointsavailable", g.totalWorldPointsAvailable);
		g.inGameQuitConfirmDialog = p.getString("ingamequitconfirmdialog", g.inGameQuitConfirmDialog);
		g.endGameConfirmDialog = p.getString("endgameconfirmdialog", g.endGameConfirmDialog);
		g.chapterProgression = getList(p, "chapterprogression", g.chapterProgression);
		g.bonusLevelProgression = getList(p, "bonuslevelprogression", g.bonusLevelProgression);
		g.continuesAt = getList(p, "continuesat", g.continuesAt);
		g.debugCellClasses = getList(p, "debugcellclasses", g.debugCellClasses);
	}

	// 0x5c8b60
	void readAppOptions(const engine::Params& p)
	{
		AppClass& a = mApp;
		a.name = p.getString("name", "");
		a.lowPriorityTargetFps = p.getInt("lowprioritytargetfps", 10);
		a.targetFps = p.getInt("targetfps", 60);
		a.screenDimensions = p.getIntPair("screendimensions", engine::Vector2<int>(800, 600));
		a.vendorName = p.getString("vendorname", "");
		a.gameKey = p.getString("gamekey", "");
		a.gameTitle = p.getString("gametitle", "");
		a.sceneClass = p.getString("sceneclass", "");
		a.mainMenuLayout = p.getString("mainmenulayout", "");
		a.optionsMenuLayout = p.getString("optionsmenulayout", "");
		a.userSelectLayout = p.getString("userselectlayout", "");
		a.userDeleteLayout = p.getString("userdeletelayout", "");
		a.userEntryLayout = p.getString("userentrylayout", "");
		a.loadingScreenLayout = p.getString("loadingscreenlayout", "");
		a.initialDataSet = p.getString("initialdataset", "");
		a.titleMusic = p.getString("titlemusic", "");
		a.highScoreMenuLayout = p.getString("highscoremenulayout", "");
		a.gameVersion = p.getString("gameversion", "");
		a.gameBuild = p.getInt("gamebuild", 0);
		a.autoBetaLogin = p.getBool("autobetalogin", false);
		a.betaProductCode = p.getString("betaproductcode", "");
		a.allowDebugActions = p.getBool("allowdebugactions", false);
		a.askNewGameLayout = p.getString("asknewgamelayout", "");
		a.quitConfirmDialog = p.getString("quitconfirmdialog", "");
		a.windowOverlayLayout = p.getString("windowoverlayout", "");
		a.defaultDifficultyMode = p.getString("defaultdifficultymode", "medium");
		a.buildName = p.getString("buildname", "");
	}

	AtlantisData& mData;
	BoardDataReader mBoards;
	LevelDataReader mLevels;
	std::set<std::string> mReportedKinds;  // each kind not yet translated is reported once
	SceneClass mScene;
	GameMgrClass mGameOptions;
	AppClass mApp;
	MouseCursorMgrClass mCursors;
	HighScoreScreenClass mHighScores;
	GameModeClass mGameMode;
	DataSetLoaderClass mDataSet;
};

} // namespace

bool AtlantisData::loadFile(const std::string& thePath)
{
	DataHandler aHandler(*this);
	return engine::getEngine()->parseXml(thePath, &aHandler);
}

bool AtlantisData::loadDataSet(const std::string& theName)
{
	if (!beginDataSet(theName))
		return false;
	while (!isDataSetDone())
		if (!loadNextFile())
			return false;
	return true;
}

bool AtlantisData::beginDataSet(const std::string& theName)
{
	std::map<std::string, DataSetLoaderClass>::iterator aSet = dataSets.find(theName);
	mLoadFiles.clear();
	mLoadNext = 0;
	if (aSet == dataSets.end())
	{
		engine::getEngine()->log("Cannot find data set \"%s\"\n", theName.c_str());
		return false;
	}
	// Copied: a file of the set may itself add data sets
	mLoadFiles = aSet->second.files;
	return true;
}

bool AtlantisData::loadNextFile()
{
	if (isDataSetDone())
		return false;
	const DataSetFile& aFile = mLoadFiles[mLoadNext++];
	engine::getEngine()->log("Loading %s\n", aFile.description.c_str());
	if (aFile.theme && theme != nullptr)
	{
		theme->addFile(aFile.file);
		return true;
	}
	return loadFile(aFile.file);
}

float AtlantisData::getDataSetProgress() const
{
	return mLoadFiles.empty() ? 1.0f : (float)mLoadNext / (float)mLoadFiles.size();
}
