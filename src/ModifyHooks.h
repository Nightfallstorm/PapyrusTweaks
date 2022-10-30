#pragma once
#include "Settings.h"
#include <xbyak/xbyak.h>
/* TODO:
* 1. Throw stack overflows to prevent a script from causing FPS drops when 1000+ calls deep in a stack - Done!
* 2. Increase Stack Dump timeout threshold (maybe increase stackcount threshold too?)
*/
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

			logger::info("PapyrusOpsPerFrameHook hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("PapyrusOpsPerFrameHook hooked at offset " + fmt::format("{:x}", target.offset()));
		}
	};


	static StackID currentMainThreadStackRunningID = -1;
	// Let each stack being processed from function messages run on main thread once. Additional hooks are provided to let all non-latent function calls
	// call immediately and return immediately since we can guarantee thread safety for those calls
	struct RunScriptsOnMainThread
	{
		static std::uint64_t thunk(VM* a_vm, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery, bool a_callingFromTasklets)
		{
			if (!RE::SkyrimVM::GetSingleton()->isFrozen && a_stack && a_stack->owningTasklet && a_stack->owningTasklet.get()) {
				currentMainThreadStackRunningID = a_stack->stackID;
				REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98520, 0), REL::VariantOffset(0x0, 0x0, 0x0) };
				void (*VMProcess)(RE::BSScript::Internal::CodeTasklet*) = reinterpret_cast<void (*)(RE::BSScript::Internal::CodeTasklet*)>(target.address());
				VMProcess(a_stack->owningTasklet.get());
				currentMainThreadStackRunningID = -1;
				return 0; // return value is discarded
			}

			return func(a_vm, a_stack, a_funcCallQuery, a_callingFromTasklets);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98134, 105176), REL::VariantOffset(0x18E, 0x0, 0x0) };
			stl::write_thunk_call<RunScriptsOnMainThread>(target.address());
			logger::info("RunScriptsOnMainThread hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("RunScriptsOnMainThread hooked at offset {}", fmt::format("{:x}", target.offset()));
		}
	};

	struct RunVMFunctionCallsUnsynced
	{
		static std::uint64_t thunk(VM* a_vm, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery, bool a_callingFromTasklets)
		{
			if (a_stack && a_stack->stackID == currentMainThreadStackRunningID) {
				return func(a_vm, a_stack, a_funcCallQuery, 0);  // Set as NOT callingFromTasklets so we can execute as main thread
			}
			return func(a_vm, a_stack, a_funcCallQuery, a_callingFromTasklets); // Run normally
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98548, 0), REL::VariantOffset(0x56, 0x0, 0x0) };
			stl::write_thunk_call<RunVMFunctionCallsUnsynced>(target.address());
			logger::info("RunVMFunctionCalls hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("RunVMFunctionCalls hooked at offset {}", fmt::format("{:x}", target.offset()));
		}
	};

	struct ReturnVMFunctionCallsUnsynced
	{
		static std::uint64_t thunk(VM* a_vm, RE::BSScript::Stack* a_stack, bool unk, bool a_callingFromTasklets)
		{
			if (a_stack && a_stack->stackID == currentMainThreadStackRunningID) {
				//logger::info("Running unsynced on stackID {}", a_stack->stackID);
				return func(a_vm, a_stack, unk, 1);  // Set as callingFromTasklets so it returns immediately rather than suspending the stack for later
			}
			return func(a_vm, a_stack, unk, a_callingFromTasklets);  // Run normally
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98130, 0), REL::VariantOffset(0x883, 0x0, 0x0) };
			stl::write_thunk_call<ReturnVMFunctionCallsUnsynced>(target.address());
			logger::info("ReturnVMFunctionCalls hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("ReturnVMFunctionCalls hooked at offset {}", fmt::format("{:x}", target.offset()));
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
			REL::Relocation<std::uintptr_t> target{ REL_ID(53195, 0), OFFSET_3(0x6E, 0x0, 0x0) };
			auto newTimeoutCheck = StackDumpTimeoutModifier(timeoutMS);
			REL::safe_fill(target.address(), REL::NOP, 0x5);  // Fill with NOP just in case
			REL::safe_write(target.address(), newTimeoutCheck.getCode(), newTimeoutCheck.getSize());

			logger::info("StackDumpTimeoutModifier hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("StackDumpTimeoutModifier hooked at offset " + fmt::format("{:x}", target.offset()));
		}

		static inline void installDisable()
		{
			REL::Relocation<std::uintptr_t> target{ REL_ID(53195, 0), OFFSET_3(0x6E, 0x0, 0x0) };  // TODO AE and VR
			REL::safe_fill(target.address(), REL::NOP, 0x21);                                      // Disable the checks AND disable the stackdump flag
			logger::info("StackDumpTimeoutDisable hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("StackDumpTimeoutDisable hooked at offset " + fmt::format("{:x}", target.offset()));
		}
	};

	struct FixToggleScriptsSaveHook
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
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53205, 0), OFFSET_3(0x46, 0x0, 0x0) };

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
	};

	static inline void InstallHooks()
	{
		StackDumpTimeoutHook::Install();
		PapyrusOpsPerFrameHook::Install();
		if (Settings::GetSingleton()->fixes.fixToggleScriptSave) {
			FixToggleScriptsSaveHook::Install();
		}
		if (Settings::GetSingleton()->experimental.runScriptsOnMainThread) {
			RunScriptsOnMainThread::Install();
			RunVMFunctionCallsUnsynced::Install();
			ReturnVMFunctionCallsUnsynced::Install();
		}
	}
}
