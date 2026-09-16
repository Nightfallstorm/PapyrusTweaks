#pragma once
#include "Util.h"
#include "configuration/Settings.h"
#include "hooks/Hooks.h"

#include <string>
#include <xbyak/xbyak.h>

namespace hooks::performance::nativespeedup
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct CallableFromTaskletInterceptHook
	{
		// Intercept VMProcess's check if a function is callable from taskelts (can be called without syncing to framerate)
		// and return true for all non-excluded functions/classes (making the call immediate instead of waiting until the next frame)
		static bool callableFromTaskletCheckIntercept(RE::BSScript::IFunction* a_function, [[maybe_unused]] bool a_callbableFromTasklets, [[maybe_unused]] RE::BSScript::Stack* a_stack);

		static void InitBlacklist();

		static void ExcludeStackFromSpeedUp(RE::VMStackID a_id);

		static void UnexcludeStackFromSpeedup(RE::VMStackID a_id);

		struct SwapCallableFromTaskletCheck : Xbyak::CodeGenerator
		{
			// We can trust rcx,rdx and r8 will be scratched over when we jump back to the skyirm code
			SwapCallableFromTaskletCheck(std::uintptr_t jmpIfCheckPasses, std::uintptr_t jmpIfCheckFails, std::uintptr_t func)
			{
				Xbyak::Label funcLabel;
				mov(rdx, r14b);  // move a_callableFromTasklets into place, a_function is already in place
				mov(r8, rbx);    // move a_stack into place

				// Note: We don't need to add/substract from the OS stack because we are overwriting a different call function (function->canBeCalledFromTasklets())
				call(ptr[rip + funcLabel]); // callableFromTaskletCheckIntercept(...)
				test(al, al);
				jz("CheckFails");
				L("CheckPasses");
				mov(rcx, jmpIfCheckPasses);
				jmp(rcx);
				L("CheckFails");
				mov(rcx, jmpIfCheckFails);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->experimental.speedUpNativeCalls) {
				return;
			}
			InitBlacklist();

			const REL::Relocation target{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x60A, 0x61D, 0x60A) };
			const REL::Relocation shouldCallImmediately{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x62A, 0x63D, 0x62A) };
			const REL::Relocation shouldSuspend{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x617, 0x62A, 0x617) };

			auto stackCheckCode = SwapCallableFromTaskletCheck(shouldCallImmediately.address(), shouldSuspend.address(), reinterpret_cast<uintptr_t>(callableFromTaskletCheckIntercept));
			REL::safe_fill(target.address(), REL::NOP, 0xD);

			auto& trampoline = SKSE::GetTrampoline();
			auto result = trampoline.allocate(stackCheckCode);
			auto& trampoline2 = SKSE::GetTrampoline();
			trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);

			logger::info("CallableFromTaskletInterceptHook hooked at address {:x}", target.address());
			logger::info("CallableFromTaskletInterceptHook hooked at offset {:x}", target.offset());
		}
		REGISTER_HOOK(CallableFromTaskletInterceptHook)
		REGISTER_HOOK_THUNK(1)
		REGISTER_HOOK_CUSTOM_SIZE(0x40)
	};

	struct AttemptFunctionCallHook
	{
		// Use function lock around `AttemptFunctionCall` when called from tasklets to prevent concurrent execution of native calls now that they are sped up.
		// This isn't the most sophisticated way to synchronize previously non-sped up native calls as all script functions will sync to the lock,
		// but it is one of the simplest approaches and shouldn't cause any measurable script performance loss outside of specific synthetic tests
		static std::uint64_t thunk(VM* a_vm, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::CodeTasklet>* a_tasklet, bool a_callingFromTasklets);

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static void Install()
		{
			if (!Settings::GetSingleton()->experimental.speedUpNativeCalls) {
				return;
			}
			// Note: AE inlines BSScript::Internal::Codetasklet::HandleCall() into BSScript::Internal::Codetasklet::VMProcess()
			// despite having the original now unused function at ID 105204
			// So the AE address/offset is quite different from SE/VR
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98548, 105176), REL::VariantOffset(0x56, 0x82F, 0x56) };
			stl::write_thunk_call<AttemptFunctionCallHook>(target.address());
			logger::info("AttemptFunctionCallHook hooked at address {:x}", target.address());
			logger::info("AttemptFunctionCallHook hooked at offset {:x}", target.offset());
		}
		REGISTER_HOOK(AttemptFunctionCallHook)
		REGISTER_HOOK_THUNK(1);
	};
}
