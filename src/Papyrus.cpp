#include "Papyrus.h"
#include "Settings.h"
#include "ExperimentalHooks.h"

namespace Papyrus
{
	inline std::vector<std::int32_t> GetPapyrusTweaksVersion(RE::StaticFunctionTag*)
	{
		return { Version::MAJOR, Version::MINOR, Version::PATCH };
	}

	bool IsMainThreadTweakActive(VM*, StackID, RE::StaticFunctionTag*) {
		return Settings::GetSingleton()->experimental.runScriptsOnMainThread;
	}

	bool DisableFastMode(VM*, StackID stackID, RE::StaticFunctionTag*)
	{
		ExperimentalHooks::CallableFromTaskletInterceptHook::ExcludeStackFromSpeedUp(stackID);
		return true;
	}

	bool EnableFastMode(VM*, StackID stackID, RE::StaticFunctionTag*) {
		ExperimentalHooks::CallableFromTaskletInterceptHook::UnexcludeStackFromSpeedup(stackID);	
		return true;
	}

	bool Bind(VM* a_vm)
	{
		if (!a_vm) {
			logger::critical("couldn't get VM State"sv);
			return false;
		}

		logger::info("Binding functions..."sv);

		BIND(GetPapyrusTweaksVersion, true);

		logger::info("Registered GetPapyrusTweaksVersion"sv);

		BIND(IsMainThreadTweakActive);

		logger::info("Registed IsMainThreadTweakActive"sv);

		BIND(DisableFastMode);

		logger::info("Registed DisableFastMode"sv);

		BIND(EnableFastMode);

		logger::info("Registed EnableFastMode"sv);

		return true;
	}
}
