#pragma once

#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::log::validationsignature
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	struct ValidationSignaturesHook
	{
		static std::uint64_t thunk(RE::BSScript::IFunction** a_function, RE::BSScrapArray<RE::BSScript::Variable>* a_varArray, char* a_outString, std::int32_t a_bufferSize);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->loggertweaks.improveValidateArgsErrors) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x63D, 0x650, 0x63D) };
			stl::write_thunk_call<ValidationSignaturesHook>(target.address());

			logger::info("ValidationSignaturesHook hooked at address {:x}", target.address());
			logger::info("ValidationSignaturesHook at offset {:x}", target.offset());
		}
		REGISTER_HOOK(ValidationSignaturesHook)
		REGISTER_HOOK_THUNK(1)
	};
}
