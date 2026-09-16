#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

#include <xbyak/xbyak.h>

namespace hooks::tweak::bypasscorruptsave
{

	// TODO: Expand messagebox message?
	struct BypassCorruptSaveHook
	{
		// strip the `ResetGame` callback
		struct XorRDX : Xbyak::CodeGenerator
		{
			XorRDX()
			{
				xor_(rdx, rdx);
			}
		};
		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->experimental.bypassCorruptedSave) {
				return;
			}
			REL::Relocation target{ RELOCATION_ID(53207, 54018), REL::VariantOffset(0x4D, 0x4D, 0x4D) };
			auto xorCode = XorRDX();
			REL::safe_fill(target.address(), REL::NOP, 0x7);
			assert(xorCode.getSize() < 0x7);
			REL::safe_write(target.address(), xorCode.getCode(), xorCode.getSize());

			logger::info("Hooked BypassCorruptSaveHook at address {:x}", target.address());
			logger::info("Hooked BypassCorruptSaveHook at offset {:x}", target.offset());
		}
		REGISTER_HOOK(BypassCorruptSaveHook)
	};
}
