#pragma once
#include "Settings.h"
#include <xbyak/xbyak.h>

namespace ExperimentalHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	static bool appliedLoadHooks = false;
	static StackID currentMainThreadStackRunningID = -1;
	// Let each stack being processed from function messages run on main thread once. Additional hooks are provided to let all non-latent function calls
	// call immediately and return immediately since we can guarantee thread safety for those calls
	struct RunScriptsOnMainThread
	{
		static std::uint64_t thunk(VM* a_vm, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery, bool a_callingFromTasklets)
		{
			if (!RE::SkyrimVM::GetSingleton()->isFrozen && a_stack && a_stack->owningTasklet && a_stack->owningTasklet.get()) {
				currentMainThreadStackRunningID = a_stack->stackID;
				REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98520, 105176) };
				void (*VMProcess)(RE::BSScript::Internal::CodeTasklet*) = reinterpret_cast<void (*)(RE::BSScript::Internal::CodeTasklet*)>(target.address());
				VMProcess(a_stack->owningTasklet.get());
				currentMainThreadStackRunningID = -1;
				return 0;  // return value is discarded
			}

			return func(a_vm, a_stack, a_funcCallQuery, a_callingFromTasklets);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98134, 104857), REL::VariantOffset(0x18E, 0x18E, 0x18E) };
			stl::write_thunk_call<RunScriptsOnMainThread>(target.address());
			logger::info("RunScriptsOnMainThread hooked at address {:x}", target.address());
			logger::info("RunScriptsOnMainThread hooked at offset {:x}", target.offset());
		}
	};

	struct RunVMFunctionCallsUnsynced
	{
		static std::uint64_t thunk(VM* a_vm, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery, bool a_callingFromTasklets)
		{
			if (a_stack && a_stack->stackID == currentMainThreadStackRunningID) {
				return func(a_vm, a_stack, a_funcCallQuery, 0);  // Set as NOT callingFromTasklets so we can execute as main thread
			}
			return func(a_vm, a_stack, a_funcCallQuery, a_callingFromTasklets);  // Run normally
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98548, 105204), REL::VariantOffset(0x56, 0x56, 0x56) };
			stl::write_thunk_call<RunVMFunctionCallsUnsynced>(target.address());
			logger::info("RunVMFunctionCalls hooked at address {:x}", target.address());
			logger::info("RunVMFunctionCalls hooked at offset {:x}", target.offset());
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
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x883, 0x898, 0x883) };
			stl::write_thunk_call<ReturnVMFunctionCallsUnsynced>(target.address());
			logger::info("ReturnVMFunctionCalls hooked at address {:x}", target.address());
			logger::info("ReturnVMFunctionCalls hooked at offset {:x}", target.offset());
		}
	};

	static inline void InstallHooks()
	{
		auto settings = Settings::GetSingleton();

		if (settings->experimental.runScriptsOnMainThread) {
			RunScriptsOnMainThread::Install();
			RunVMFunctionCallsUnsynced::Install();
			ReturnVMFunctionCallsUnsynced::Install();
		}
	}

	static inline void installAfterLoadHooks()
	{
		if (appliedLoadHooks) {
			return;
		}

		if (RE::SkyrimVM::GetSingleton() && VM::GetSingleton()) {
			appliedLoadHooks = true;
			if (Settings::GetSingleton()->experimental.speedUpGameGetPlayer) {
				VM::GetSingleton()->SetCallableFromTasklets("Game", "GetPlayer", true);
				logger::info("Applied Game.GetPlayer speed up");
			}
		}
	}
}
