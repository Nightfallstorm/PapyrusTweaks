#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::tweak::basetypemismatch
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	// "Error: Unable to bind script MCMFlaskUtilsScript to FlaskUtilsMCM (7E007E63) because their base types do not match"
	struct BaseTypeMismatch
	{
		static bool thunk(const char* a_buffer, std::size_t bufferCount, const char* a_format, const char* a_scriptName, const char* a_objectName);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->loggertweaks.improveBaseTypeMismatch) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(52730, 53574), REL::VariantOffset(0x3B2, 0x52C, 0x3B2) };
			stl::write_thunk_call<BaseTypeMismatch>(target.address());

			logger::info("BaseTypeMismatch hooked at address {:x}", target.address());
			logger::info("BaseTypeMismatch at offset {:x}", target.offset());
		}
		REGISTER_HOOK_THUNK(1)
		REGISTER_HOOK(BaseTypeMismatch);
	};
}
