#include "Settings.h"

Settings* Settings::GetSingleton()
{
	static Settings singleton;
	return std::addressof(singleton);
}

void Settings::Load()
{
	constexpr auto path = L"Data/SKSE/Plugins/PapyrusTweaks.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	//FIXES
	fixes.Load(ini);

	//TWEAKS
	tweaks.Load(ini);

	//EXPERIMENTAL
	experimental.Load(ini);

	ini.SaveFile(path);
}

void Settings::Fixes::Load(CSimpleIniA& a_ini)
{
	const char* section = "Fixes";

	detail::get_value(a_ini, fixToggleScriptSave, section, "bFixToggleScriptsCommand", ";Fixes ToggleScripts command not persisting when saving/stack dumping\n;Scripts will now stay turned off when toggled off, and on when toggled on.");
	detail::get_value(a_ini, fixScriptPageAllocation, section, "bFixScriptPageAllocation", ";Fix unintentionally allocating script pages when getting largest available page, but out of memory.");
	detail::get_value(a_ini, fixIsHostileToActorCrash, section, "bFixIsHostileToActorCrash", ";Fix crash when passing in NONE object to script function Actor.IsHostileToActor().");
}

void Settings::Tweaks::Load(CSimpleIniA& a_ini)
{
	const char* section = "Tweaks";

	detail::get_value(a_ini, maxOpsPerFrame, section, "iMaxOpsPerFrame", ";Maximum papyrus operations per frame. Higher number means better script performance on average\n;Has a very minor impact on framerate, and varies from script to script. (Default: 500, Vanilla value: 100). Recommended Range: 100-2000. Set to 0 to disable");
	detail::get_value(a_ini, disableGetFormFromFile, section, "bDisableGetFormFromFileErrorLogs", ";Disables `File \" % s \" does not exist or is not currently loaded.` logs from being printed when calling Game.GetFormFromFile().\n;This only disables the logging of the error, the error itself will still occur");
	detail::get_value(a_ini, improveBaseTypeMismatch, section, "bImproveBaseTypeMismatchLogs", ";Improves Base Type Mismatch error to show inheritance hierarchy of scripts; Also clarifies if the script was a genuine mismatch, or if the script doesn't exist");
	detail::get_value(a_ini, improveValidateArgsErrors, section, "bImproveValidateArgsLogs", ";Improves several error logs relating to incompatible arguments to better clarify what is incompatible");
	detail::get_value(a_ini, disableNoPropertyOnScript, section, "bDisableNoPropertyOnScriptErrorLogs", ";Disable \"Property %s on script %s attached to %s cannot be initialized because the script no longer contains that property\" log messages.\n;This only disables the logging of the warning, the warning itself will still occur");
	detail::get_value(a_ini, disableMissingScriptError, section, "bDisableMissingScriptError", ";Disable \"Cannot open store for class \"%s\", missing file?\" errors being logged.\n;This only disables the logging of the error, the error itself will still occur");
	detail::get_value(a_ini, stackDumpTimeoutThreshold, section, "iStackDumpTimeoutMS", ";Modify how long Papyrus can be \"overstressed\" before dumping stacks, in milliseconds (Default value: 15000, Vanilla value: 5000).\n;Set to 0 to disable the stack dump check, or -1 to disable this setting.\n;See https://www.nexusmods.com/skyrimspecialedition/articles/4625 for information on what a stack dump is");
	detail::get_value(a_ini, summarizeStackDumps, section, "bSummarizeStackDumps", ";Adds a summary of events when dumping stacks to log");
}

void Settings::Experimental::Load(CSimpleIniA& a_ini)
{
	const char* section = "Experimental";

	// 0.2.0
	a_ini.Delete(section, "bSpeedUpGetPlayer", true);
	a_ini.Delete(section, "bRunScriptsOnMainThread", true);

	// 3.1
	a_ini.Delete(section, "fMainThreadTaskletBudget", true);


	// Get deprecated values first in case user is updating from 3.1 or below
	detail::get_value(a_ini, speedUpNativeCalls, section, "bRunScriptsOnMainThreadOnly", ";Run scripts on main thread only, and desync most native function calls. This can drastically improve script performance, at a minor cost of framerate during heavy load.\n;The amount of performance and framerate cost is based on fMainThreadTaskletBudget, as well as the current script load");
	detail::get_value(a_ini, classesToExcludeFromSpeedUp, section, "sMainThreadClassesToExclude", ";(Requires bRunScriptsOnMainThreadOnly=true) List of script classes to exclude from being sped up by bRunScriptsOnMainThreadOnly.\n;It is strongly recommended to leave at defaults unless you absolutely know what you're doing");
	detail::get_value(a_ini, methodPrefixesToExcludeFromSpeedup, section, "sMainThreadMethodsToExclude", ";(Requires bRunScriptsOnMainThreadOnly=true) List of script method prefixes to exclude from being sped up (ex: \"Equip\" excludes \"EquipItem\" and \"EquipItemByID\", but does NOT exclude \"UnequipItem\".\n;It is strongly recommended to leave at defaults unless you absolutely know what you're doing");

	// 3.2
	a_ini.Delete(section, "bRunScriptsOnMainThreadOnly", true);
	a_ini.Delete(section, "sMainThreadClassesToExclude", true);
	a_ini.Delete(section, "sMainThreadMethodsToExclude", true);

    detail::get_value(a_ini, speedUpNativeCalls, section, "bSpeedUpNativeCalls", ";(Formerly bRunScriptsOnMainThreadOnly)\n;Speeds up native calls by desyncing them from framerate and instead syncing script calls to a spinlock, greatly improving script performance while still preventing concurrent execution of native calls");
	detail::get_value(a_ini, classesToExcludeFromSpeedUp, section, "sScriptClassesToExclude", ";(Formerly sMainThreadClassesToExclude, requires bSpeedUpNativeCalls=true)\n;List of script classes to exclude from being sped up by bSpeedUpNativeCalls.\n;It is strongly recommended to leave at defaults unless you absolutely know what you're doing");
	
	// 3.3 (Reset script method prefix exclusions for 3.2 and below to add new additions for everyone who updates)
	a_ini.Delete(section, "sScriptMethodsToExclude", true);
	detail::get_value(a_ini, methodPrefixesToExcludeFromSpeedup, section, "sScriptMethodPrefixesToExclude", ";(Formerly sMainThreadMethodsToExclude, requires bSpeedUpNativeCalls=true)\n;List of script method prefixes to exclude from being sped up (ex: \"Equip\" excludes \"EquipItem\" and \"EquipItemByID\", but does NOT exclude \"UnequipItem\".\n;It is strongly recommended to leave at defaults unless you absolutely know what you're doing\n;as a lot of modifying functions like `EquipItem` do not work properly if executed more than once in a single frame.\n; Defaults exclude everything except for read-only functions (ex: GetFormFromFile, HasKeyword, IsLoaded, etc.)");

	detail::get_value(a_ini, disableScriptsInPlayroom, section, "bDisableScriptsInPlayroomVR", ";(VR-ONLY) Pauses all non-playroom scripts while in the VR playroom, so mod scripts only initialize once you actually enter a save/new game.\n;This is only experimental as it intentionally alters script behavior");
	detail::get_value(a_ini, bypassCorruptedSave, section, "bBypassCorruptSaveMessage", ";Stops the game from resetting when loading a corrupted save\n;This will NOT fix a broken save, just allows you to load the save no matter what information is lost. ONLY USE AS A LAST RESORT TO RECOVER A SAVE YOU HAVE BEEN WARNED!!");
	detail::get_value(a_ini, bypassMemoryLimit, section, "bIgnoreMemoryLimit", ";Allows the Script Engine to use as much memory as needed, with no cap. This makes `iMaxAllocatedMemoryBytes` in Skyrim.ini useless/infinite\n;Note: Skyrim already ignores memory limit when stressed, this setting just keeps it ignored.");
}
