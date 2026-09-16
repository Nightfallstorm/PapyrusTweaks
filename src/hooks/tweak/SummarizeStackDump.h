#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::tweak::summarizeStackDump
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	// Adds a brief summary of running stacks
	struct SummarizeStackDumpHook
	{
		static void thunk(RE::BSScript::IVMDebugInterface* a_vm);

		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::uint32_t idx = 1;

		static void Install()
		{
			if (!Settings::GetSingleton()->loggertweaks.summarizeStackDumps) {
				return;
			}
			stl::write_vfunc<RE::BSScript::Internal::VirtualMachine, 3, SummarizeStackDumpHook>();
			logger::info("SummarizeStackDumpHook placed!");
		}
		REGISTER_HOOK(SummarizeStackDumpHook)
	};
}
