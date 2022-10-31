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
	tweaks.Load(ini);

	//TWEAKS
	tweaks.Load(ini);

	//EXPERIMENTAL
	experimental.Load(ini);

	ini.SaveFile(path);
}

void Settings::Fixes::Load(CSimpleIniA& a_ini)
{
	const char* section = "Tweaks";

	detail::get_value(a_ini, fixToggleScriptSave, section, "bFixToggleScriptSave", ";Fixes a bug where saving a game will always unfreeze scripts regardless of the frozen status.\nThis is mainly intended to fix \"ToggleScript\" console command not keeping scripts frozen on save");
}

void Settings::Tweaks::Load(CSimpleIniA& a_ini)
{
	const char* section = "Tweaks";

	detail::get_value(a_ini, maxOpsPerFrame, section, "iMaxOpsPerFrame", ";Maximum papyrus operations per frame. Higher number means better script performance on average, at a potentially minor cost of framerate, and varies from script to script. (Default: 500, Vanilla value: 100). Recommended Range: 100-2000");
	detail::get_value(a_ini, disableGetFormFromFile, section, "bDisableGetFormFromFileErrorLogs", ";Disables `File \" % s \" does not exist or is not currently loaded.` logs from being printed when calling Game.GetFormFromFile().\nThis only disables the logging of the error, the error itself will still occur");
	detail::get_value(a_ini, improveBaseTypeMismatch, section, "bImproveBaseTypeMismatchLogs", ";Improves Base Type Mismatch error to show inheritance hierarchy of scripts, as well as clarify if the script was a genuine mismatch, or if the script doesn't exist");
	detail::get_value(a_ini, improveValidateArgsErrors, section, "bImproveValidateArgsLogs", ";Improves several error logs relating to incompatible arguments to better clarify what is incompatible");
	detail::get_value(a_ini, disableNoPropertyOnScript, section, "bDisableNoPropertyOnyScriptErrorLogs", ";Disable \"Property %s on script %s attached to %s cannot be initialized because the script no longer contains that property\" log messages.\nThis only disables the logging of the warning, the warning itself will still occur");
	detail::get_value(a_ini, disableMissingScriptError, section, "bDisableMissingScriptError", ";Disable \"Cannot open store for class \"%s\", missing file?\" errors being logged.\nThis only disables the logging of the error, the error itself will still occur");
	detail::get_value(a_ini, stackDumpTimeoutThreshold, section, "iStackDumpTimeoutMS", ";Modify how long Papyrus can be \"overstressed\" before dumping stacks, in milliseconds (Default value: 15000, Vanilla value: 5000).\n Set to 0 to disable the stack dump check. See https://www.nexusmods.com/skyrimspecialedition/articles/4625 for information on what a stack dump is");
}

void Settings::Experimental::Load(CSimpleIniA& a_ini)
{
	const char* section = "Experimental";

	detail::get_value(a_ini, speedUpGameGetPlayer, section, "bSpeedUpGetPlayer", ";Speed up \"Game.GetPlayer\" calls by desyncing it from framerate. In theory, this shouldn't cause issues, but is experimental as this is not fully tested");
}
