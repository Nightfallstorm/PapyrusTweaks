#pragma once
#include "Settings.h"
#include <string>
#include <xbyak/xbyak.h>

namespace ExperimentalHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	static bool appliedLoadHooks = false;
	static inline std::set<RE::VMStackID> excludedStacks;

	struct UpdateTaskletsHook
	{
		static void thunk(VM* a_vm, float a_budget)
		{
			if (a_vm->vmTasks.size()) {
				auto copiedTasks = RE::BSTArray<RE::BSScript::Internal::CodeTasklet*>(a_vm->vmTasks);
				a_vm->vmTasks.clear();
				auto initialTime = RE::BSTimer::GetCurrentGlobalTimeMult();
				RE::BSScript::Internal::CodeTasklet* taskletToProcess = copiedTasks.back();
				while (taskletToProcess) {
					VMProcess(taskletToProcess);
					copiedTasks.pop_back();
					if (copiedTasks.size()) {
						taskletToProcess = copiedTasks.back();
					} else {
						taskletToProcess = nullptr;
					}
					if (RE::BSTimer::GetCurrentGlobalTimeMult() - initialTime >= a_budget) {
						break;
					}
				}

				if (copiedTasks.size()) {
					// push leftovers for the next frame
					for (auto taskletToPush : copiedTasks) {
						a_vm->vmTasks.push_back(taskletToPush);
					}
				}
				copiedTasks.clear();
			}
		}

		static inline void VMProcess(RE::BSScript::Internal::CodeTasklet* a_self)
		{
			using func_t = decltype(&VMProcess);
			REL::Relocation<func_t> funct{ RELOCATION_ID(98520, 105176) };
			return funct(a_self);
		}

		static inline std::uint32_t idx = 5;
		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			stl::write_vfunc<VM, UpdateTaskletsHook>();
			logger::info("UpdateTaskletsHook vfunc set!");
		}
	};

	struct SkipTasksToJobHook
	{
		static std::uint64_t thunk([[maybe_unused]] VM* a_vm, [[maybe_unused]] std::uint64_t a_joblist)
		{
			// Skip
			// TODO Maybe implement the job queue ourselves for multi-threading support?
			return 0;
		}
		static inline std::uint32_t idx = 0x12;
		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			stl::write_vfunc<VM, SkipTasksToJobHook>();
			logger::info("SkipTasksToJobHook vfunc set!");
		}
	};

	struct CallVMUpdatesHook
	{
		// Call SkyrimVM::Update and SkyrimVM::UpdateTasklets here, as vanilla only calls on main thread if paused
		static void thunk(std::uint64_t unk, std::uint64_t unk1, std::uint64_t unk2, std::uint64_t unk3)
		{
			ProcessRegisteredUpdates(RE::SkyrimVM::GetSingleton(), 0.0);
			ProcessTasklets(RE::SkyrimVM::GetSingleton(), 0.0);
			func(unk, unk1, unk2, unk3);  // execute original call
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void ProcessRegisteredUpdates(RE::SkyrimVM* a_vm, float a_budget)
		{
			using func_t = decltype(&ProcessRegisteredUpdates);
			REL::Relocation<func_t> funct{ RELOCATION_ID(53115, 53926) };
			return funct(a_vm, a_budget);
		}

		static void ProcessTasklets(RE::SkyrimVM* a_vm, float a_budget)
		{
			using func_t = decltype(&ProcessRegisteredUpdates);
			REL::Relocation<func_t> funct{ RELOCATION_ID(53116, 53927) };
			return funct(a_vm, a_budget);
		}
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(35565, 36564), REL::VariantOffset(0x344, 0x2D6, 0x3E9) };
			stl::write_thunk_call<CallVMUpdatesHook>(target.address());
			// remove original VM update calls in main thread, which only occurred on game pause
			REL::Relocation<std::uintptr_t> originalUpdate{ RELOCATION_ID(35565, 36564), REL::VariantOffset(0x38C, 0x553, 0x432) };
			REL::Relocation<std::uintptr_t> originalUpdateTasklets{ RELOCATION_ID(35565, 36564), REL::VariantOffset(0x39B, 0x542, 0x441) };
			REL::safe_fill(originalUpdate.address(), REL::NOP, 0x5);
			REL::safe_fill(originalUpdateTasklets.address(), REL::NOP, 0x5);
			logger::info("CallVMUpdatesHook hooked at address {:x}", target.address());
			logger::info("CallVMUpdatesHook hooked at offset {:x}", target.offset());
		}
	};

	struct CallableFromTaskletInterceptHook
	{
		static inline std::vector<RE::BSFixedString> blacklistedNames;

		static bool callableFromTaskletCheckIntercept(RE::BSScript::IFunction* a_function, [[maybe_unused]] bool a_callbableFromTasklets, [[maybe_unused]] RE::BSScript::Stack* a_stack)
		{
			if (a_function->CanBeCalledFromTasklets()) {
				// already fast, no need to check blacklist
				return true;
			}

			if (a_function->GetIsNative()) {
				auto nativeFunction = reinterpret_cast<RE::BSScript::NF_util::NativeFunctionBase*>(a_function);
				if (nativeFunction->GetIsLatent()) {
					// is latent, return false to keep it delayed
					return false;
				}
			}

			if (excludedStacks.find(a_stack->stackID) != excludedStacks.end()) {
				// stack called DisableFastMode(), return false to keep normal behavior
				return false;
			}

			for (auto objectName : blacklistedNames) {
				if (a_function->GetObjectTypeName() == objectName || a_function->GetName() == objectName) {
					return false;
				}
			}

			return true;
		}

		static void InitBlacklist()
		{
			if (Settings::GetSingleton()->experimental.mainThreadClassesToBlacklist != "") {
				auto mainThreadClasses = Settings::GetSingleton()->experimental.mainThreadClassesToBlacklist;
				mainThreadClasses.erase(remove(mainThreadClasses.begin(), mainThreadClasses.end(), ' '), mainThreadClasses.end());  // Trim whitespace
				std::stringstream class_stream(mainThreadClasses);                                                                  //create string stream from the string
				while (class_stream.good()) {
					std::string substr;
					getline(class_stream, substr, ',');  //get first string delimited by comma
					blacklistedNames.push_back(substr);
				}
			}

			if (Settings::GetSingleton()->experimental.mainThreadMethodsToBlacklist != "") {
				auto mainThreadMethods = Settings::GetSingleton()->experimental.mainThreadMethodsToBlacklist;
				mainThreadMethods.erase(remove(mainThreadMethods.begin(), mainThreadMethods.end(), ' '), mainThreadMethods.end());  // Trim whitespace
				std::stringstream method_stream(mainThreadMethods);                                                                 //create string stream from the string
				while (method_stream.good()) {
					std::string substr;
					getline(method_stream, substr, ',');  //get first string delimited by comma
					blacklistedNames.push_back(substr);
				}
			}
		}

		struct SwapCallableFromTaskletCheck : Xbyak::CodeGenerator
		{
			// We can trust rcx,rdx and r8 will be scratched over when we jump back to the skyirm code
			SwapCallableFromTaskletCheck(std::uintptr_t jmpIfCheckPasses, std::uintptr_t jmpIfCheckFails, std::uintptr_t func)
			{
				Xbyak::Label funcLabel;
				mov(rdx, r14b);  // move a_callableFromTasklets into place, a_function is already in place
				mov(r8, rbx);    // Get stack for debugging purposes
				call(ptr[rip + funcLabel]);
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
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x60A, 0x61D, 0x60A) };
			REL::Relocation<std::uintptr_t> shouldCallImmediately{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x62A, 0x63D, 0x62A) };
			REL::Relocation<std::uintptr_t> shouldSuspend{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x617, 0x62A, 0x617) };

			auto stackCheckCode = SwapCallableFromTaskletCheck(shouldCallImmediately.address(), shouldSuspend.address(), reinterpret_cast<uintptr_t>(callableFromTaskletCheckIntercept));
			REL::safe_fill(target.address(), REL::NOP, 0xD);

			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(stackCheckCode.getSize());
			auto result = trampoline.allocate(stackCheckCode);
			auto& trampoline2 = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);

			logger::info("CallableFromTaskletInterceptHook hooked at address {:x}", target.address());
			logger::info("CallableFromTaskletInterceptHook hooked at offset {:x}", target.offset());
			InitBlacklist();
		}
	};

	// TODO: Expand messagebox message? Also AE/VR
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
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53207, 0), REL::VariantOffset(0x4D, 0x0, 0x0) };
			auto xorCode = XorRDX();
			REL::safe_fill(target.address(), REL::NOP, 0x7);
			assert(xorCode.getSize < 0x7);
			REL::safe_write(target.address(), xorCode.getCode(), xorCode.getSize());

			logger::info("Hooked BypassCorruptSaveHook at address {:x}", target.address());
			logger::info("Hooked BypassCorruptSaveHook at offset {:x}", target.offset());
		}
	};

	// TODO: AE/VR
	struct KeepIgnoreMemoryLimitFlag
	{
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53195, 0), REL::VariantOffset(0x101, 0x0, 0x0) };
			std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x81, (std::byte)0x94, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 }; // mov    BYTE PTR [rcx+0x94],0x1
			REL::safe_fill(target.address(), REL::NOP, 0x7);
			REL::safe_write(target.address(), setMemoryLimitCode, 0x7);

			logger::info("Hooked KeepIgnoreMemoryLimitFlag at address {:x}", target.address());
			logger::info("Hooked KeepIgnoreMemoryLimitFlag at offset {:x}", target.offset());
		}
	};

	static inline void InstallHooks()
	{
		auto settings = Settings::GetSingleton();

		if (settings->experimental.runScriptsOnMainThread) {
			UpdateTaskletsHook::Install();
			SkipTasksToJobHook::Install();
			CallVMUpdatesHook::Install();
			CallableFromTaskletInterceptHook::Install();
		}
		if (settings->experimental.bypassCorruptedSave) {
			BypassCorruptSaveHook::Install();
		}
		if (settings->experimental.bypassMemoryLimit) {
			KeepIgnoreMemoryLimitFlag::Install();
		}
	}
}
