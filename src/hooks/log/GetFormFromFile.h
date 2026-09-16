#pragma once

#include "configuration/Settings.h"
#include "hooks/Hooks.h"

#undef GetObject

namespace LoggerHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	// "Error: File \" % s \" does not exist or is not currently loaded."
	struct GetFormFromFileHook
	{
		// Disable this log entry through NOPs
		static void Install()
		{
			if (!Settings::GetSingleton()->loggertweaks.disableGetFormFromFile) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(54832, 55465), REL::VariantOffset(0x7E, 0x7E, 0x81) };
			REL::safe_fill(target.address(), REL::NOP, 0x5);  // Remove the call to setup the log
			const REL::Relocation target2{ RELOCATION_ID(54832, 55465), REL::VariantOffset(0x97, 0x97, 0x9A) };
			REL::safe_fill(target2.address(), REL::NOP, 0x4);  // Remove the call to log the GetFormFromFile error
			logger::info("GetFormFromFileHook hooked at address {:x}", target.address());
			logger::info("GetFormFromFileHook at offset {:x}", target.offset());
		}

		REGISTER_HOOK(GetFormFromFileHook);
	};
}
