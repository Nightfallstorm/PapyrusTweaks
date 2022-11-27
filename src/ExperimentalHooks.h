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

	struct ProcessTaskletsHook
	{
		static inline float taskletBudget = 0;
		// Manual implementation of VM->ProcessExtraTasklets. The original doesn't respect the papyrus budget,
		// but our implementation will to prevent framerate spikes
		static void thunk(VM* a_vm, [[maybe_unused]] float a_budget)
		{
			if (a_vm->vmTasks.size()) {
				auto copiedTasks = RE::BSTArray<RE::BSScript::Internal::CodeTasklet*>(a_vm->vmTasks);
				a_vm->vmTasks.clear();
				
				auto initialTime = 1000.0 * GetCounter() / GetFrequency();
				auto taskletToProcess = copiedTasks.back();
				while (taskletToProcess) {
					VMProcess(taskletToProcess);
					copiedTasks.pop_back();
					if (copiedTasks.size()) {
						taskletToProcess = copiedTasks.back();
					} else {
						taskletToProcess = nullptr;
					}
					
					if (taskletBudget > 0 && (1000.0 * GetCounter() / GetFrequency()) - initialTime > taskletBudget) {
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

		// copied from https://gist.github.com/nkuln/1300858/2b6f3f48131652ddff924bcb02664b669b8df714 
		static inline LONGLONG GetCounter()
		{
			LARGE_INTEGER counter;

			if (!::QueryPerformanceCounter(&counter))
				return -1;

			return counter.QuadPart;
		}

		static inline LONGLONG frequency;
		static inline LONGLONG GetFrequency()
		{
			if (frequency != 0) {
				return frequency;
			} else {
				LARGE_INTEGER freq;

				if (!::QueryPerformanceFrequency(&freq))
					return -1;

				frequency = freq.QuadPart;
				return frequency;
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
		static inline void Install(float a_budget)
		{
			taskletBudget = a_budget;
			stl::write_vfunc<VM, ProcessTaskletsHook>();
			logger::info("UpdateTaskletsHook vfunc set!");
		}
	};

	struct SkipTasksToJobHook
	{
		// Skip VM->TasksToJobs to keep the tasklets inside the VM instead of outsourcing it to the job lists
		// Normally, VM only processes tasklets when the world is paused, but we will process it manually ourselves in a different hook
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
		// Call SkyrimVM::UpdateTasklets here in addition to the normal Processing of updates
		static void thunk(RE::SkyrimVM* a_vm, float a_budget)
		{
			func(a_vm, a_budget);  // SkyrimVM::ProcessRegistedUpdates
			ProcessTasklets(RE::SkyrimVM::GetSingleton(), 0.0);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void ProcessTasklets(RE::SkyrimVM* a_vm, float a_budget)
		{
			using func_t = decltype(&ProcessTasklets);
			REL::Relocation<func_t> funct{ RELOCATION_ID(53116, 53927) };
			return funct(a_vm, a_budget);
		}
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(38118, 39074), REL::VariantOffset(0x1E, 0x1E, 0x32) };
			stl::write_thunk_call<CallVMUpdatesHook>(target.address());
			logger::info("CallVMUpdatesHook hooked at address {:x}", target.address());
			logger::info("CallVMUpdatesHook hooked at offset {:x}", target.offset());
		}
	};

	struct CallableFromTaskletInterceptHook
	{
		static inline std::vector<RE::BSFixedString> excludedClasses;
		static inline std::vector<RE::BSFixedString> excludedMethodPrefixes;

		// Intercept VMProcess's check if a function is callable from taskelts (can be called without syncing to framerate)
		// and return true for all non-excluded functions/classes (making the call immediate instead of waiting until the next frame)
		static bool callableFromTaskletCheckIntercept(RE::BSScript::IFunction* a_function, [[maybe_unused]] bool a_callbableFromTasklets, [[maybe_unused]] RE::BSScript::Stack* a_stack)
		{
			if (a_function->CanBeCalledFromTasklets()) {
				// already fast, no need to check excluded functions
				return true;
			}

			if (a_function->GetIsNative()) {
				auto nativeFunction = reinterpret_cast<RE::BSScript::NF_util::NativeFunctionBase*>(a_function);
				if (nativeFunction->GetIsLatent()) {
					// is latent, return false to keep it delayed since it takes real-world time anyways
					return false;
				}
			}

			if (excludedStacks.find(a_stack->stackID) != excludedStacks.end()) {
				// stack called PapyrusTweaks.DisableFastMode(), return false to keep normal behavior
				return false;
			}

			for (auto className : excludedClasses) {
				if (a_function->GetObjectTypeName() == className) {
					return false;
				}
			}

			for (auto methodPrefix : excludedMethodPrefixes) {
				if (std::string_view(a_function->GetName()).starts_with(methodPrefix)) {
					return false;
				}
			}
			//logger::info("Speeding up {}.{}", a_function->GetObjectTypeName(), a_function->GetName());
			return true;
		}

		static void InitBlacklist()
		{
			if (Settings::GetSingleton()->experimental.mainThreadClassesToExclude != "") {
				auto mainThreadClasses = Settings::GetSingleton()->experimental.mainThreadClassesToExclude;
				mainThreadClasses.erase(remove(mainThreadClasses.begin(), mainThreadClasses.end(), ' '), mainThreadClasses.end());  // Trim whitespace
				std::stringstream class_stream(mainThreadClasses);                                                                  //create string stream from the string
				while (class_stream.good()) {
					std::string substr;
					getline(class_stream, substr, ',');  //get first string delimited by comma
					excludedClasses.push_back(substr);
					logger::info("Excluding class: {}", substr);
				}
			}

			if (Settings::GetSingleton()->experimental.mainThreadMethodPrefixesToExclude != "") {
				auto mainThreadMethods = Settings::GetSingleton()->experimental.mainThreadMethodPrefixesToExclude;
				mainThreadMethods.erase(remove(mainThreadMethods.begin(), mainThreadMethods.end(), ' '), mainThreadMethods.end());  // Trim whitespace
				std::stringstream method_stream(mainThreadMethods);                                                                 //create string stream from the string
				while (method_stream.good()) {
					std::string substr;
					getline(method_stream, substr, ',');  //get first string delimited by comma
					excludedMethodPrefixes.push_back(substr);
					logger::info("Excluding method prefix: {}", substr);
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
				mov(r8, rbx);    // move a_stack into place
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
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53207, 54018), REL::VariantOffset(0x4D, 0x4D, 0x4D) };
			auto xorCode = XorRDX();
			REL::safe_fill(target.address(), REL::NOP, 0x7);
			assert(xorCode.getSize < 0x7);
			REL::safe_write(target.address(), xorCode.getCode(), xorCode.getSize());

			logger::info("Hooked BypassCorruptSaveHook at address {:x}", target.address());
			logger::info("Hooked BypassCorruptSaveHook at offset {:x}", target.offset());
		}
	};

	// Keep `IgnoreMemoryLimit` flag to 1 regardless of VM overstressed status
	struct KeepIgnoreMemoryLimitFlag
	{
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x101, 0x1AD, 0x101) };

			REL::safe_fill(target.address(), REL::NOP, 0x7);
			if (REL::Module::IsAE()) {
				std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x86, (std::byte)0x94, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 };  // mov    BYTE PTR [rsi+0x94],0x1
				REL::safe_write(target.address(), setMemoryLimitCode, 0x7);
			} else if (REL::Module::IsSE) {
				std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x81, (std::byte)0x94, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 };  // mov    BYTE PTR [rcx+0x94],0x1
				REL::safe_write(target.address(), setMemoryLimitCode, 0x7);
			} else {
				std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x81, (std::byte)0x9C, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 };  // mov    BYTE PTR [rcx+0x9C],0x1
				REL::safe_write(target.address(), setMemoryLimitCode, 0x7);
			}			

			logger::info("Hooked KeepIgnoreMemoryLimitFlag at address {:x}", target.address());
			logger::info("Hooked KeepIgnoreMemoryLimitFlag at offset {:x}", target.offset());
		}
	};

	static inline void InstallHooks()
	{
		auto settings = Settings::GetSingleton();

		if (settings->experimental.runScriptsOnMainThread) {
			if (settings->experimental.mainThreadTaskletTime < 0) {
				settings->experimental.mainThreadTaskletTime = 0;
			}
			ProcessTaskletsHook::Install(settings->experimental.mainThreadTaskletTime);
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
