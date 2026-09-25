#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::tweak::debugmode
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct EnableLoadDocStrings
	{
		static RE::BSScript::CompiledScriptLoader* thunk(
			RE::BSScript::CompiledScriptLoader* a_unmadeSelf,
			RE::SkyrimScript::Logger* a_logger,
			bool a_loadDebugInformation,
			[[maybe_unused]] bool a_loadDocStrings
		);

		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			if (!Settings::GetSingleton()->VMtweaks.enableDocStrings) {
				return;
			}
			if (REL::Module::IsAtLeast({1,7,104,0})) {
				logger::info("Disabling EnableLoadDocStrings hook for AE 1.7");
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(53108, 53919), REL::VariantOffset(0x604, 0x664, 0x604) };
			stl::write_thunk_call<EnableLoadDocStrings>(target.address());
			logger::info("EnableLoadDocStrings hooked at address {:x}", target.address());
			logger::info("EnableLoadDocStrings hooked at offset {:x}", target.offset());
		}

		REGISTER_HOOK(EnableLoadDocStrings)
		REGISTER_HOOK_THUNK(1)
	};

	struct EnableLoadDebugInformation
	{
		static RE::BSScript::CompiledScriptLoader* thunk(
			RE::BSScript::CompiledScriptLoader* a_unmadeSelf,
			RE::SkyrimScript::Logger* a_logger, [[maybe_unused]]
			bool a_loadDebugInformation,
			bool a_loadDocStrings
		);

		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			if (!Settings::GetSingleton()->VMtweaks.enableDebugInfo) {
				return;
			}
			if (REL::Module::IsAtLeast({1,7,104,0})) {
				logger::info("Disabling EnableLoadDebugInformation hook for AE 1.7");
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(53108, 53919), REL::VariantOffset(0x604, 0x664, 0x604) };
			stl::write_thunk_call<EnableLoadDebugInformation>(target.address());
			logger::info("EnableLoadDebugInformation hooked at address {:x}", target.address());
			logger::info("EnableLoadDebugInformation hooked at offset {:x}", target.offset());
		}

		REGISTER_HOOK(EnableLoadDebugInformation)
		REGISTER_HOOK_THUNK(1);
	};
}
