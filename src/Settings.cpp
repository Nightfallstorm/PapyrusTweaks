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
}

void Settings::Experimental::Load(CSimpleIniA& a_ini)
{
	const char* section = "Experimental";

	// 0.2.0
	a_ini.Delete(section, "bSpeedUpGetPlayer", true);
	a_ini.Delete(section, "bRunScriptsOnMainThread", true);

	detail::get_value(a_ini, runScriptsOnMainThread, section, "bRunScriptsOnMainThreadOnly", ";Run scripts on main thread only, and desync most native function calls. This can drastically improve script performance, at a minor cost of framerate during heavy load.\n;The amount of performance and framerate cost is based on fExtraTaskletBudgetMS in Skyrim's INI settings, as well as the current script load");
	
	// Keep blacklist hidden/hardcoded for now, may expose them in the future
	//detail::get_value(a_ini, mainThreadClassesToBlacklist, section, "sMainThreadClassesToBlacklist", ";List of classes to exclude from being sped up by bRunScriptsOnMainThread.");
	//detail::get_value(a_ini, mainThreadMethodsToBlacklist, section, "sMainThreadMethodsToBlacklist", ";List of method names to exclude from being sped up by bRunScriptsOnMainThread. Note that if a method OR class name matches, it will be excluded.");
	detail::get_value(a_ini, disableScriptsInPlayroom, section, "bDisableScriptsInPlayroomVR", ";(VR-ONLY) Pauses all non-playroom scripts while in the VR playroom, so mod scripts only initialize once you actually enter a save/new game.\n;This is only experimental as it intentionally alters script behavior");
}
