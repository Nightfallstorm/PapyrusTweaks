#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

#include <xbyak/xbyak.h>

namespace hooks::tweak::stackdump
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct StackDumpTimeoutHook
	{
		struct StackDumpTimeoutModifier : Xbyak::CodeGenerator
		{
			StackDumpTimeoutModifier(const int timeoutMS)
			{
				// swap (add eax, 5000h) with this line
				add(eax, timeoutMS);
			}
		};

		// Install our hook at the specified address
		static void Install()
		{
			const auto stackDumpTimeoutMS = std::min(Settings::GetSingleton()->VMtweaks.stackDumpTimeoutThreshold, 50000000);
			if (stackDumpTimeoutMS < 0) {
				return;
			}
			if (stackDumpTimeoutMS == 0) {
				installDisable();
			} else {
				installModifier(stackDumpTimeoutMS);
			}
		}

		static void installModifier(int timeoutMS)
		{
			const REL::Relocation target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x6E, 0x71, 0x6E) };
			const auto newTimeoutCheck = StackDumpTimeoutModifier(timeoutMS);
			assert(newTimeoutCheck.getSize() <= 0x5);
			REL::safe_fill(target.address(), REL::NOP, 0x5);  // Fill with NOP just in case
			REL::safe_write(target.address(), newTimeoutCheck.getCode(), newTimeoutCheck.getSize());

			logger::info("StackDumpTimeoutModifier hooked at address {:x}", target.address());
			logger::info("StackDumpTimeoutModifier hooked at offset {:x}", target.offset());
		}

		static void installDisable()
		{
			REL::Relocation target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x6E, 0x71, 0x6E) };
			REL::safe_fill(target.address(), REL::NOP, 0x21);  // Disable the checks AND disable the stackdump flag
			logger::info("StackDumpTimeoutDisable hooked at address {:x}", target.address());
			logger::info("StackDumpTimeoutDisable hooked at offset {:x}", target.offset());
		}

		REGISTER_HOOK(StackDumpTimeoutHook)
	};
}
