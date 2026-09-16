#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::fix::scriptpage
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct FixScriptPageAllocation
	{
		static RE::BSScript::IMemoryPagePolicy::AllocationStatus thunk(RE::BSScript::SimpleAllocMemoryPagePolicy* self, RE::BSTAutoPointer<RE::BSScript::MemoryPage>& a_newPage);

		static inline std::uint32_t idx = 3;

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->fixes.fixScriptPageAllocation) {
				return;
			}
			stl::write_vfunc<RE::BSScript::SimpleAllocMemoryPagePolicy, FixScriptPageAllocation>();

			logger::info("FixScriptPageAllocation set!");
		}

		REGISTER_HOOK(FixScriptPageAllocation)
	};
}
