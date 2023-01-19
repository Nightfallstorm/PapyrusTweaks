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
			if (stackDumpTimeoutMS < 0) {
				return;
			} else if (stackDumpTimeoutMS == 0) {
				installDisable();
			} else {
				installModifier(stackDumpTimeoutMS);
			}
		}

		static inline void installModifier(int timeoutMS)
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x6E, 0x71, 0x6E) };
			auto newTimeoutCheck = StackDumpTimeoutModifier(timeoutMS);
			assert(newTimeoutCheck.getSize() <= 0x5);
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

	struct FixToggleScriptsSaveHook
	{
		struct CallThunk : Xbyak::CodeGenerator
		{
			CallThunk(std::uintptr_t funct)
			{
				Xbyak::Label funcLabel;

				jmp(ptr[rip + funcLabel]);
				L(funcLabel);
				dq(funct);
			}
		};
		static void thunk(RE::SkyrimVM* a_this, bool a_frozen)
		{
			if (RE::Script::GetProcessScripts()) {  // Only unfreeze script processing if script processing is enabled
				a_this->frozenLock.Lock();
				a_this->isFrozen = a_frozen;
				a_this->frozenLock.Unlock();
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53205, 54016), REL::VariantOffset(0x46, 0x46, 0x41) };

			if (REL::Module::IsVR()) {
				stl::write_thunk_call<FixToggleScriptsSaveHook>(target.address());
			} else {
				// Save code uses jmp over call, and expects `SkyrimVM->SetFrozen()` to return from the save function
				// so we can't use a regular thunk call
				auto callThunk = CallThunk(reinterpret_cast<std::uintptr_t>(thunk));
				auto& trampoline = SKSE::GetTrampoline();
				SKSE::AllocTrampoline(callThunk.getSize());
				auto result = trampoline.allocate(callThunk);
				auto& trampoline2 = SKSE::GetTrampoline();
				SKSE::AllocTrampoline(14);
				trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);
			}

			logger::info("FixToggleScriptsSaveHook for saves hooked at address {:x}", target.address());
			logger::info("FixToggleScriptsSaveHook hooked at offset {:x}", target.offset());
		}
	};

	struct FixToggleScriptsDumpHook
	{
		static void thunk(RE::SkyrimVM* a_this, bool a_frozen)
		{
			if (RE::Script::GetProcessScripts()) {  // Only unfreeze script processing if script processing is enabled
				a_this->frozenLock.Lock();
				a_this->isFrozen = a_frozen;
				a_this->frozenLock.Unlock();
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53209, 54020), REL::VariantOffset(0xCC, 0xCC, 0xCC) };
			stl::write_thunk_call<FixToggleScriptsDumpHook>(target.address());

			logger::info("FixToggleScriptsDumpHook for stack dumps hooked at address {:x}", target.address());
			logger::info("FixToggleScriptsDumpHook hooked at offset {:x}", target.offset());
		}
	};

	struct FixScriptPageAllocation
	{
		// BSScript::SimpleAllocMemoryPagePolicy::GetLargestAvailablePage
		static RE::BSScript::IMemoryPagePolicy::AllocationStatus thunk(RE::BSScript::SimpleAllocMemoryPagePolicy* self, RE::BSTAutoPointer<RE::BSScript::MemoryPage>& a_newPage)
		{
			self->dataLock.Lock();
			int availablePageSize = self->maxAllocatedMemory - self->currentMemorySize;
			int currentMemorySizeTemp = self->currentMemorySize;
			if (availablePageSize < 0) {
				// set equal so the original function will return kOutOfMemory instead of unintentionally allocating a page
				self->currentMemorySize = self->maxAllocatedMemory;
			}
			RE::BSScript::IMemoryPagePolicy::AllocationStatus result = func(self, a_newPage);
			if (availablePageSize < 0) {
				// set back to original size
				self->currentMemorySize = currentMemorySizeTemp;
			}
			self->dataLock.Unlock();
			return result;
		}

		static inline std::uint32_t idx = 3;

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			stl::write_vfunc<RE::BSScript::SimpleAllocMemoryPagePolicy, FixScriptPageAllocation>();

			logger::info("FixScriptPageAllocation set!");
		}
	};

	struct FixIsHostileToActorCrash
	{
		// Easiest hook here is to replace the original IsHostileToActor callback with our own
		static std::uint64_t thunk(std::uint64_t unk, char* functionName, char* className, std::uintptr_t callback, VM** a_vm)
		{
			IsHostileToActor = callback; // Store the original so we can invoke it later
			return func(unk, functionName, className, reinterpret_cast<std::uintptr_t>(IsHostileToActorEx), a_vm);
		};

	    // Actor.IsHostileToActor(Actor akActor) crashes when akActor is NONE.
		// We will instead log an error and return false back, fixing the crash
		static bool IsHostileToActorEx(VM* a_vm, RE::VMStackID a_stackID, RE::Actor* a_self, RE::Actor* a_other) {
			if (!a_other) {
				a_vm->TraceStack(
					"Actor argument is NONE for Actor.IsHostileToActor()! Mod authors: This normally crashes the game in vanilla, but is fixed by Papyrus Tweaks NG",
					a_stackID,
					RE::BSScript::ErrorLogger::Severity::kError
				);
				return false;
			} else {
				return IsHostileToActor(a_vm, a_stackID, a_self, a_other);
			}
		}

		static inline REL::Relocation<decltype(IsHostileToActorEx)> IsHostileToActor;  // Original IsHostileToActor

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53960, 54784), REL::VariantOffset(0x2CA6, 0x2FDB, 0x2BA4) };
			stl::write_thunk_call<FixIsHostileToActorCrash>(target.address());

			logger::info("FixIsHostileToActorCrash hooked at address {:x}", target.address());
			logger::info("FixIsHostileToActorCrash hooked at offset {:x}", target.offset());
		}
	};
	static inline void InstallHooks()
	{
		auto settings = Settings::GetSingleton();
		if (settings->tweaks.maxOpsPerFrame > 0) {
			PapyrusOpsPerFrameHook::Install();
		}
		if (settings->fixes.fixToggleScriptSave) {
			FixToggleScriptsSaveHook::Install();
			FixToggleScriptsDumpHook::Install();
		}
		if (settings->fixes.fixScriptPageAllocation) {
			FixScriptPageAllocation::Install();
		}
		if (settings->fixes.fixIsHostileToActorCrash) {
			FixIsHostileToActorCrash::Install();
		}
		if (settings->tweaks.stackDumpTimeoutThreshold > 0) {
			StackDumpTimeoutHook::Install();
		}

	}
}
