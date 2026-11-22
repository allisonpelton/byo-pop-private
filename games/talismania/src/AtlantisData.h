#ifndef __TALISMANIA_ATLANTISDATA_H__
#define __TALISMANIA_ATLANTISDATA_H__

// The game's data classes and the loader that reads them from config/*.xml (the original's <Name>Class structs and
// <Name>ClassLoader sub-handlers, registered by 0x512550 under each top-level tag). Each kind is one struct named from
// the XML's attributes and one registry map by name; the original keeps each in its own std::map. The loader here is one
// document handler that dispatches by tag where the original stacks a sub-handler per element, and a data set's files
// are read at once where the original's loading thread reads them one at a time.
// Translated so far: the kinds of the base data set (config/scenes.xml and config/gameoptions.xml) and the data set
// list (config/datasets.xml).

#include "DocHandler.h"
#include "Geometry.h"

#include <map>
#include <string>
#include <vector>

// A scene's layer (handler 0x5c4950): where it draws and how a darkening layer fades
struct SceneLayerClass
{
	std::string name;
	engine::Vector2f baseOffset;
	std::string baseRegion;
	bool followCamera = false;
	bool darkenLayer = false;
	float darkenTime = 0.75f;
	std::string tieToRegion;
	float darkenOpacity = 0.6f;
	bool fullScreenOnly = false;
	engine::RectF clip;
};

// "scene" (SceneClass): named layers drawn in layerOrder, and the regions (offsets) layers are based on
struct SceneClass
{
	std::string name;
	std::vector<std::string> layerOrder;
	std::string defaultDarkenLayer;
	std::string defaultRegion;
	std::string defaultLayer;
	std::map<std::string, SceneLayerClass> layers;
	std::map<std::string, engine::Vector2f> regions;  // by name, the region's offset ("pos")
};

// A coin the game's random coin drop picks, with its weight
struct RandomCoin
{
	std::string anim;
	int value = 0;
	float weight = 0;
};

// "gameoptions" (GameMgrClass, handler 0x5c6c20): the rules and files of one game mode
struct GameMgrClass
{
	std::string name;
	std::string dataSet;
	float preChapterDelay = 0;
	float postChapterDelay = 0;
	bool autoPauseOnBlur = false;
	bool tooltipsAllowed = false;
	std::vector<std::string> splatsFiles;
	std::string hudClass;
	std::string mapClass;
	std::string journalClass;
	std::string growStarImg;
	std::string growStarFx;
	std::string growStarSound;
	float growStarSpeedMult = 0;
	std::string inGameMenuMusic;
	std::string inGameMenuLayout;
	int totalWorldPointsAvailable = 0;
	std::string inGameQuitConfirmDialog;
	std::string endGameConfirmDialog;
	std::vector<std::string> chapterProgression;
	std::vector<std::string> bonusLevelProgression;
	std::vector<std::string> continuesAt;
	std::vector<std::string> debugCellClasses;
	std::vector<RandomCoin> randomCoins;
};

// An <difficultymode> of the application options: which game options each mode of play uses
struct DifficultyModeClass
{
	std::string name;
	std::string heroGameMgrClass;
	std::string zenGameMgrClass;
};

// "appoptions" (AppClass, handler 0x5c8b60): the application's window, layouts and music
struct AppClass
{
	std::string name;
	int lowPriorityTargetFps = 10;
	int targetFps = 60;
	engine::Vector2<int> screenDimensions{800, 600};
	std::string vendorName;
	std::string gameKey;
	std::string gameTitle;
	std::string sceneClass;
	std::string mainMenuLayout;
	std::string optionsMenuLayout;
	std::string userSelectLayout;
	std::string userDeleteLayout;
	std::string userEntryLayout;
	std::string loadingScreenLayout;
	std::string initialDataSet;
	std::string titleMusic;
	std::string highScoreMenuLayout;
	std::string gameVersion;
	int gameBuild = 0;
	bool autoBetaLogin = false;
	std::string betaProductCode;
	bool allowDebugActions = false;
	std::string askNewGameLayout;
	std::string quitConfirmDialog;
	std::string windowOverlayLayout;
	std::string defaultDifficultyMode;
	std::string buildName;
	std::map<std::string, DifficultyModeClass> difficultyModes;
};

// "debugmanager"
struct DebugManagerClass
{
	std::string name;
	std::string sceneLayer;
};

// A <cursor> of the mouse cursor manager (0x5d4de0): the Windows cursor resource shown in these view modes
struct CursorClass
{
	std::vector<std::string> viewModes;
	int resourceId = 0;
	std::string hardwareCursorImg;
	std::string transparentColor;
};

// "mousecursormgr"
struct MouseCursorMgrClass
{
	std::string name;
	std::vector<CursorClass> cursors;
};

// A high score table's starting entry (0x5d5ad0)
struct HighScoreEntryClass
{
	std::string name;
	int score = 0;
};

// "highscorescreen" (HighScoreScreenClass)
struct HighScoreScreenClass
{
	std::string name;
	std::string layout;
	int maxEntries = 12;
	std::vector<HighScoreEntryClass> entries;
	std::vector<std::string> hideComponents;
};

// "gamemode" (GameModeClass): the difficulty modes a mode of play has, each naming a game options class
struct GameModeClass
{
	std::string name;
	std::string defaultDiffMode = "medium";
	bool timedMode = false;
	std::map<std::string, std::string> diffModes;  // difficulty mode name to game class
};

// A file a data set loads (<load>, handler 0x5c65b0)
struct DataSetFile
{
	std::string file;
	std::string description;
	bool theme = false;
	bool noUpdate = false;
	bool wait = true;
};

// "datasetloader" (DataSetLoaderClass): the files of one data set, in the order they are loaded
struct DataSetLoaderClass
{
	std::string name;
	bool reportUnusedOnShutdown = true;
	std::vector<DataSetFile> files;
};

// Every kind's registry, by name
class AtlantisData
{
public:
	std::map<std::string, SceneClass> scenes;
	std::map<std::string, GameMgrClass> gameOptions;
	std::map<std::string, AppClass> appOptions;
	std::map<std::string, DebugManagerClass> debugManagers;
	std::map<std::string, MouseCursorMgrClass> mouseCursorMgrs;
	std::map<std::string, HighScoreScreenClass> highScoreScreens;
	std::map<std::string, GameModeClass> gameModes;
	std::map<std::string, DataSetLoaderClass> dataSets;

	// Reads one config file into the registries; false when it cannot be opened or parsed
	bool loadFile(const std::string& thePath);
	// Reads every file of the data set, in order (the original loads them on a thread and the boot waits)
	bool loadDataSet(const std::string& theName);
};

// Splits a list attribute at the commas outside brackets (the tokenizer at 0x437930)
std::vector<std::string> splitList(const std::string& theValue);

#endif
