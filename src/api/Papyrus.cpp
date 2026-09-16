#include "Papyrus.h"
#include "../configuration/Settings.h"
#include "../hooks/performance/NativeSpeedUp.h"
#include "Version.h"

namespace Papyrus
{
	inline std::vector<std::int32_t> GetPapyrusTweaksVersion(RE::StaticFunctionTag*)
	{
		return { Project::Version::MAJOR, Project::Version::MINOR, Project::Version::PATCH };
	}

	bool IsNativeCallSpeedUpActive(VM*, StackID, RE::StaticFunctionTag*)
	{
		return Settings::GetSingleton()->experimental.speedUpNativeCalls;
	}

	bool DisableFastMode(VM*, StackID stackID, RE::StaticFunctionTag*)
	{
		hooks::performance::nativespeedup::CallableFromTaskletInterceptHook::ExcludeStackFromSpeedUp(stackID);
		return true;
	}

	bool EnableFastMode(VM*, StackID stackID, RE::StaticFunctionTag*) {
		hooks::performance::nativespeedup::CallableFromTaskletInterceptHook::UnexcludeStackFromSpeedup(stackID);
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

		// Note for aspiring SKSE modders:
		// Adding true here sets `callableFromTasklets` to true, so calling this function doesn't wait a whole frame to execute, but
		// runs the risk of simulatenous execution of said function when doing so. Since we are just returning a single value
		// that won't change during the lifetime of Skyrim, it is safe to speed up this call.
		BIND(IsNativeCallSpeedUpActive, true);

		logger::info("Registed IsMainThreadTweakActive"sv);

		BIND(DisableFastMode);

		logger::info("Registed DisableFastMode"sv);

		BIND(EnableFastMode);

		logger::info("Registed EnableFastMode"sv);

		return true;
	}
}
