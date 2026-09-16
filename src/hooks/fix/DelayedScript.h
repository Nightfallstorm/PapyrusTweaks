#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::fix::delayedscript
{
	struct FixDelayedTypeCast
	{
		static bool thunk(RE::BSScript::LinkerProcessor* self, RE::BSFixedString* a_name, RE::BSScript::TypeInfo& a_typeOut);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->fixes.fixDelayedScriptBreakage) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(98721, 105384), REL::VariantOffset(0x2C8, 0x30D, 0x2C8) };
			stl::write_thunk_call<FixDelayedTypeCast>(target.address());
			logger::info("FixDelayedTypeCast hooked at address {:x}", target.address());
			logger::info("FixDelayedTypeCast hooked at offset {:x}", target.offset());

			const REL::Relocation target1{ RELOCATION_ID(98722, 105385), REL::VariantOffset(0x160, 0x1C7, 0x160) };
			stl::write_thunk_call<FixDelayedTypeCast>(target1.address());
			logger::info("FixDelayedTypeCast hooked at address {:x}", target1.address());
			logger::info("FixDelayedTypeCast hooked at offset {:x}", target1.offset());
		}

		REGISTER_HOOK(FixDelayedTypeCast)
		REGISTER_HOOK_THUNK(2)
	};

	struct FixDelayedTypeCastVFunc
	{
		static bool thunk(RE::BSScript::UnlinkedTypes::LinkerConvertTypeFunctor* self, RE::BSFixedString* a_name, RE::BSScript::TypeInfo& a_typeOut);

		static inline std::uint32_t idx = 1;

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->fixes.fixDelayedScriptBreakage) {
				return;
			}
			stl::write_vfunc<RE::BSScript::UnlinkedTypes::LinkerConvertTypeFunctor, FixDelayedTypeCastVFunc>();
			logger::info("FixDelayedTypeCastVFunc hook set!");
		}

		REGISTER_HOOK(FixDelayedTypeCastVFunc)
	};
}
