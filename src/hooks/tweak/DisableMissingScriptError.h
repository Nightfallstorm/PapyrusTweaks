#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::tweak::missingscript
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	// "Cannot open store for class \"%s\", missing file?"
	struct DisableMissingScriptError
	{
		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->loggertweaks.disableMissingScriptError) {
				return;
			}
			REL::Relocation target{ RELOCATION_ID(97831, 104575), REL::VariantOffset(0xC4, 0x123, 0xC4) };
			if (REL::Module::IsAE()) {
				constexpr std::byte newJump[] = { (std::byte)0xe9, (std::byte)0x8F, (std::byte)0xFF, (std::byte)0xFF, (std::byte)0xFF };
				REL::safe_fill(target.address(), REL::NOP, 0x5);
				REL::safe_write(target.address(), newJump, 0x5);
			} else {
				constexpr std::byte newJump[] = { (std::byte)0xe9, (std::byte)0xbc, (std::byte)0x00, (std::byte)0x00, (std::byte)0x00 };
				REL::safe_fill(target.address(), REL::NOP, 0x9);
				REL::safe_write(target.address(), newJump, 0x5);
			}

			logger::info("DisableMissingScriptError installed at address {:x}", target.address());
			logger::info("DisableMissingScriptError installed at offset {:x}", target.offset());
		}

		REGISTER_HOOK(DisableMissingScriptError);
	};
}
