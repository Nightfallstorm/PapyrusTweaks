#pragma once
#include "Settings.h"
#include <xbyak/xbyak.h>

namespace ModifyHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct PapyrusOpsPerFrameHook
	{
		struct PapyrusOpsModifier : Xbyak::CodeGenerator
		{
			PapyrusOpsModifier(std::uintptr_t beginLoop, std::uintptr_t endLoop)
			{
				if (REL::Module::IsAE()) {
					inc(r15d);
					cmp(r15d, Settings::GetSingleton()->tweaks.maxOpsPerFrame);
					jb("KeepLooping");
					mov(rcx, endLoop);
					jmp(rcx);
					L("KeepLooping");
					mov(rcx, beginLoop);
					jmp(rcx);
				} else {
					inc(r14d);
					cmp(r14d, Settings::GetSingleton()->tweaks.maxOpsPerFrame);
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
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0x498, 0xAB1, 0x498) };
			REL::Relocation<std::uintptr_t> beginLoop{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0xC0, 0xB0, 0xC0) };
			REL::Relocation<std::uintptr_t> endLoop{ RELOCATION_ID(98520, 105176), REL::VariantOffset(0x4AB, 0xABE, 0x4AB) };

			auto newCompareCheck = PapyrusOpsModifier(beginLoop.address(), endLoop.address());
			int fillRange = REL::Module::IsAE() ? 0xD : 0x13;
			REL::safe_fill(target.address(), REL::NOP, fillRange);
			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(newCompareCheck.getSize());
			auto result = trampoline.allocate(newCompareCheck);
			auto& trampoline2 = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);

			logger::info("PapyrusOpsPerFrameHook hooked at address {:x}", target.address());
			logger::info("PapyrusOpsPerFrameHook hooked at offset {:x}", target.offset());
		}
	};

	struct StackDumpTimeoutHook
	{
		struct StackDumpTimeoutModifier : Xbyak::CodeGenerator
		{
			StackDumpTimeoutModifier(int timeoutMS)
			{
				// swap (add eax, 5000h) with this line
				add(eax, timeoutMS);
			}
		};

		// Install our hook at the specified address
		static inline void Install()
		{
			auto stackDumpTimeoutMS = Settings::GetSingleton()->tweaks.stackDumpTimeoutThreshold;
			if (stackDumpTimeoutMS == 0) {
				installDisable();
			} else {
				installModifier(stackDumpTimeoutMS);
			}
		}

		static inline void installModifier(int timeoutMS)
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x6E, 0x71, 0x6E) };
			auto newTimeoutCheck = StackDumpTimeoutModifier(timeoutMS);
			REL::safe_fill(target.address(), REL::NOP, 0x5);  // Fill with NOP just in case
			REL::safe_write(target.address(), newTimeoutCheck.getCode(), newTimeoutCheck.getSize());

			logger::info("StackDumpTimeoutModifier hooked at address {:x}", target.address());
			logger::info("StackDumpTimeoutModifier hooked at offset {:x}", target.offset());
		}

		static inline void installDisable()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x6E, 0x71, 0x6E) };
			REL::safe_fill(target.address(), REL::NOP, 0x21);  // Disable the checks AND disable the stackdump flag
			logger::info("StackDumpTimeoutDisable hooked at address {:x}", target.address());
			logger::info("StackDumpTimeoutDisable hooked at offset {:x}", target.offset());
		}
	};

	/*  struct FixToggleScriptsSaveHook
	{
		struct CallThunk : Xbyak::CodeGenerator
		{
			CallThunk(std::uintptr_t func)
			{
				Xbyak::Label funcLabel;

				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				ret();
				L(funcLabel);
				dq(func);
			}
		};
		static void thunk(RE::SkyrimVM* a_this, bool a_frozen)
		{
			if (RE::Script::GetProcessScripts()) { // Only unfreeze script processing if script processing is enabled
				a_this->frozenLock.Lock();
				a_this->isFrozen = a_frozen;
				a_this->frozenLock.Unlock();
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53205, 54016), OFFSET_3(0x46, 0x46, 0x41) };

			auto callThunk = CallThunk(reinterpret_cast<std::uintptr_t>(thunk));
			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(callThunk.getSize());
			auto result = trampoline.allocate(callThunk);
			auto& trampoline2 = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);

			logger::info("FixToggleScriptsSaveHook hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("FixToggleScriptsSaveHook hooked at offset {}", fmt::format("{:x}", target.offset()));
		}
	}; */

	static inline void InstallHooks()
	{
		StackDumpTimeoutHook::Install();
		PapyrusOpsPerFrameHook::Install();
		if (Settings::GetSingleton()->fixes.fixToggleScriptSave) {
			//FixToggleScriptsSaveHook::Install(); TODO: Enable on CLIB-NG
			// ALSO TODO: Fix For stack dumps as well
		}
	}
}
