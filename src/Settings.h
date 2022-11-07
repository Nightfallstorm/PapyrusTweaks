#pragma once

class Settings
{
public:
	[[nodiscard]] static Settings* GetSingleton();

	void Load();

	struct Fixes
	{
		void Load(CSimpleIniA& a_ini);

		bool fixToggleScriptSave{ true };
		bool fixScriptPageAllocation{ true };
	} fixes;

	struct Tweaks
	{
		void Load(CSimpleIniA& a_ini);

		std::uint32_t maxOpsPerFrame{ 500 };
		bool disableGetFormFromFile{ false };
		bool improveBaseTypeMismatch{ true };
		bool improveValidateArgsErrors{ true };
		bool disableNoPropertyOnScript{ false };
		bool disableMissingScriptError{ false };
		int stackDumpTimeoutThreshold{ 15000 };
	} tweaks;

	struct Experimental
	{
		void Load(CSimpleIniA& a_ini);

		bool speedUpGameGetPlayer{ false };

		bool runScriptsOnMainThread{ false };

		bool disableScriptsInPlayroom{ false };

	} experimental;

private:
	struct detail
	{
		template <class T>
		static void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
		{
			if constexpr (std::is_same_v<bool, T>) {
				a_value = a_ini.GetBoolValue(a_section, a_key, a_value);
				a_ini.SetBoolValue(a_section, a_key, a_value, a_comment);

				GetSingleton()->settingsMap.emplace(a_key, a_value);
			} else if constexpr (std::is_floating_point_v<T>) {
				a_value = static_cast<T>(a_ini.GetDoubleValue(a_section, a_key, a_value));
				a_ini.SetDoubleValue(a_section, a_key, a_value, a_comment);

				GetSingleton()->settingsMap.emplace(a_key, a_value != 1.0);
			} else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
				a_value = string::lexical_cast<T>(a_ini.GetValue(a_section, a_key, std::to_string(a_value).c_str()));
				a_ini.SetValue(a_section, a_key, std::to_string(a_value).c_str(), a_comment);

				GetSingleton()->settingsMap.emplace(a_key, a_value != 0);
			} else {
				a_value = a_ini.GetValue(a_section, a_key, a_value.c_str());
				a_ini.SetValue(a_section, a_key, a_value.c_str(), a_comment);
			}
		}
	};

	robin_hood::unordered_flat_map<std::string, bool> settingsMap{};
};
