#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

#include <xbyak/xbyak.h>

namespace hooks::fix::togglescript
{
	struct FixToggleScriptsSaveHook
	{
		struct CallThunk : Xbyak::CodeGenerator
		{
			explicit CallThunk(const std::uintptr_t funct)
			{
				Xbyak::Label funcLabel;

				jmp(ptr[rip + funcLabel]);
				L(funcLabel);
				dq(funct);
			}
		};
		static void thunk(RE::SkyrimVM* a_this, bool a_frozen);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->fixes.fixToggleScriptSave) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(53205, 54016), REL::VariantOffset(0x46, 0x46, 0x41) };

			if (REL::Module::IsVR()) {
				stl::write_thunk_call<FixToggleScriptsSaveHook>(target.address());
			} else {
				// Save code uses jmp over call, and expects `SkyrimVM->SetFrozen()` to return from the save function
				// so we can't use a regular thunk call
				auto callThunk = CallThunk(reinterpret_cast<std::uintptr_t>(thunk));
				auto& trampoline = SKSE::GetTrampoline();
				auto result = trampoline.allocate(callThunk);
				auto& trampoline2 = SKSE::GetTrampoline();
				trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);
			}

			logger::info("FixToggleScriptsSaveHook for saves hooked at address {:x}", target.address());
			logger::info("FixToggleScriptsSaveHook hooked at offset {:x}", target.offset());
		}

		REGISTER_HOOK(FixToggleScriptsSaveHook)
		REGISTER_HOOK_THUNK(1)
		REGISTER_HOOK_CUSTOM_SIZE(0x10)
	};

	struct FixToggleScriptsDumpHook
	{
		static void thunk(RE::SkyrimVM* a_this, bool a_frozen);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->fixes.fixToggleScriptSave) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(53209, 54020), REL::VariantOffset(0xCC, 0xCC, 0xCC) };
			stl::write_thunk_call<FixToggleScriptsDumpHook>(target.address());

			logger::info("FixToggleScriptsDumpHook for stack dumps hooked at address {:x}", target.address());
			logger::info("FixToggleScriptsDumpHook hooked at offset {:x}", target.offset());
		}

		REGISTER_HOOK(FixToggleScriptsDumpHook)
		REGISTER_HOOK_THUNK(1)
	};
}
