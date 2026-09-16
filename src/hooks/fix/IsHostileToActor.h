#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::fix::ishostiletoactor
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct FixIsHostileToActorCrash
	{
		// Easiest hook here is to replace the original IsHostileToActor callback with our own
		static std::uint64_t thunk(std::uint64_t unk, char* functionName, char* className, std::uintptr_t callback, VM** a_vm);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->fixes.fixIsHostileToActorCrash) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(53960, 54784), REL::VariantOffset(0x2CA6, 0x2FDB, 0x2BA4) };
			stl::write_thunk_call<FixIsHostileToActorCrash>(target.address());

			logger::info("FixIsHostileToActorCrash hooked at address {:x}", target.address());
			logger::info("FixIsHostileToActorCrash hooked at offset {:x}", target.offset());
		}

		REGISTER_HOOK(FixIsHostileToActorCrash)
		REGISTER_HOOK_THUNK(1)
	};
}
