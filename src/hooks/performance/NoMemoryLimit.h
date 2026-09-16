#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

namespace hooks::performance::nomemorylimit
{
	// Keep `IgnoreMemoryLimit` flag to 1 regardless of VM overstressed status
	// In other words, if VM is NOT overstressed, change the pseudocode `ADJ(skyrimVM)->memoryPagePolicy.ignoreMemoryLimit = 0;` to `ADJ(skyrimVM)->memoryPagePolicy.ignoreMemoryLimit = 1;`
	struct KeepIgnoreMemoryLimitFlag
	{
		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->experimental.bypassMemoryLimit) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x101, 0x1AD, 0x101) };

			REL::safe_fill(target.address(), REL::NOP, 0x7);
			if (REL::Module::IsAE()) {
				// AE
				constexpr std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x86, (std::byte)0x94, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 };  // mov    BYTE PTR [rsi+0x94],0x1
				REL::safe_write(target.address(), setMemoryLimitCode, 0x7);
			} else if (REL::Module::IsSE()) {
				// SE
				constexpr std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x81, (std::byte)0x94, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 };  // mov    BYTE PTR [rcx+0x94],0x1
				REL::safe_write(target.address(), setMemoryLimitCode, 0x7);
			} else {
				// VR
				constexpr std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x81, (std::byte)0x9C, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 };  // mov    BYTE PTR [rcx+0x9C],0x1
				REL::safe_write(target.address(), setMemoryLimitCode, 0x7);
			}

			logger::info("Hooked KeepIgnoreMemoryLimitFlag at address {:x}", target.address());
			logger::info("Hooked KeepIgnoreMemoryLimitFlag at offset {:x}", target.offset());
		}

		REGISTER_HOOK(KeepIgnoreMemoryLimitFlag)
	};
}
