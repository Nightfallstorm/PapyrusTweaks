#pragma once
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

#include <xbyak/xbyak.h>

namespace hooks::performance::maxops
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct PapyrusOpsPerFrameHook
	{
		// The original papyrus ops instruction (cmp register, 100) is too small to do any direct
		// jump and/or increase the size beyond anything meaningful. We must carve out
		// some space in the target hook to create a jump to this code cave, recreate
		// the ops comparison with an instruction that accepts much larger numbers, and recreate
		// the carved out instructions
		struct PapyrusOpsModifier : Xbyak::CodeGenerator
		{
			PapyrusOpsModifier(const std::uintptr_t beginLoop, const std::uintptr_t endLoop)
			{
				if (REL::Module::IsAE()) {
					inc(r15d);
					cmp(r15d, Settings::GetSingleton()->VMtweaks.maxOpsPerFrame);
					jb("KeepLooping");
					mov(rcx, endLoop);
					jmp(rcx);
					L("KeepLooping");
					mov(rcx, beginLoop);
					jmp(rcx);
				} else {
					inc(r14d);
					cmp(r14d, Settings::GetSingleton()->VMtweaks.maxOpsPerFrame);
					mov(r8d, 10760);
					jb("KeepLooping");
					mov(rcx, endLoop);
					jmp(rcx);
					L("KeepLooping");
					mov(rcx, beginLoop);
					jmp(rcx);
				}
			}
		};

		// Install our hook at the specified address
		static void Install()
		{
			if (Settings::GetSingleton()->VMtweaks.maxOpsPerFrame == 0) {
				return;
			}
			const REL::Relocation target{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0x498, 0xAB1, 0x498) };
			const REL::Relocation beginLoop{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0xC0, 0xB0, 0xC0) };
			const REL::Relocation endLoop{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0x4AB, 0xABE, 0x4AB) };

			auto newCompareCheck = PapyrusOpsModifier(beginLoop.address(), endLoop.address());
			const int fillRange = REL::Module::IsAE() ? 0xD : 0x13;
			REL::safe_fill(target.address(), REL::NOP, fillRange);
			auto& trampoline = SKSE::GetTrampoline();
			auto result = trampoline.allocate(newCompareCheck);
			auto& trampoline2 = SKSE::GetTrampoline();
			trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);

			logger::info("PapyrusOpsPerFrameHook hooked at address {:x}", target.address());
			logger::info("PapyrusOpsPerFrameHook hooked at offset {:x}", target.offset());
		}

		REGISTER_HOOK(PapyrusOpsPerFrameHook)
		REGISTER_HOOK_THUNK(1)
		REGISTER_HOOK_CUSTOM_SIZE(0x30)
	};
}
