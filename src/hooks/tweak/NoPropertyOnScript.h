#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::tweak::noproperty
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	// "Property %s on script %s attached to %s cannot be initialized because the script no longer contains that property"
	struct NoPropertyOnScriptHook
	{
		static void Install()
		{
			if (!Settings::GetSingleton()->loggertweaks.disableNoPropertyOnScript) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(52767, 53611), REL::VariantOffset(0x6FC, 0x6CB, 0x6FC) };
			REL::safe_fill(target.address(), REL::NOP, 0x3);  // erase the call to log the warning

			logger::info("NoPropertyOnScript installed at address {:x}", target.address());
			logger::info("NoPropertyOnScript installed at offset {:x}", target.offset());
		}
		REGISTER_HOOK(NoPropertyOnScriptHook);
	};
}
