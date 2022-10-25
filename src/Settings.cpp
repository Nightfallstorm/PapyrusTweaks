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
	static const char* section = "Fixes";

	detail::get_value(a_ini, recursionFix, section, "Break Recursion Loops", ";Break scripts stuck in a recursion loop to prevent FPS drop");
}

void Settings::Tweaks::Load(CSimpleIniA& a_ini)
{
	const char* section = "Tweaks";

	detail::get_value(a_ini, maxOpsPerFrame, section, "Max Ops Per Frame", ";Maximum papyrus operations per frame. Higher number means better script performance on average, at a potentially minor cost of framerate. (Default: 500, Vanilla value: 100). Recommended Range: 100-2000");
	detail::get_value(a_ini, disableGetFormFromFile, section, "Disable GetFormFromFile error logs", ";Disables `File \" % s \" does not exist or is not currently loaded.` logs from being printed when calling Game.GetFormFromFile(). This only disables the logging of the error, the error itself will still occur");
	detail::get_value(a_ini, improveBaseTypeMismatch, section, "Improve Base Type Mismatch logs", ";Improves Base Type Mismatch error to show inheritance hierarchy of scripts, as well as clarify if the script was a genuine mismatch, or if the script doesn't exist");

}

void Settings::Experimental::Load(CSimpleIniA& a_ini)
{
	const char* section = "Experimental";
	// TBD
}
