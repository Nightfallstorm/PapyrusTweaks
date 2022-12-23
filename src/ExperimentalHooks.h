#pragma once
#include "Settings.h"
#include <string>
#include <xbyak/xbyak.h>

namespace ExperimentalHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	struct CallableFromTaskletInterceptHook
	{
		static inline std::vector<RE::BSFixedString> excludedClasses;
		static inline std::vector<RE::BSFixedString> excludedMethodPrefixes;
		static inline std::set<RE::VMStackID> excludedStacks;
		static inline std::mutex excludedStacksLock;
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

			excludedStacksLock.lock();
			if (excludedStacks.find(a_stack->stackID) != excludedStacks.end()) {
				// stack called PapyrusTweaks.DisableFastMode(), return false to keep normal behavior
				excludedStacksLock.unlock();
				return false;
			}
			excludedStacksLock.unlock();

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
			if (Settings::GetSingleton()->experimental.classesToExcludeFromSpeedUp != "") {
				auto classes = Settings::GetSingleton()->experimental.classesToExcludeFromSpeedUp;
				classes.erase(remove(classes.begin(), classes.end(), ' '), classes.end());  // Trim whitespace
				std::stringstream class_stream(classes);                                    // create string stream from the string
				while (class_stream.good()) {
					std::string substr;
					getline(class_stream, substr, ',');  //get first string delimited by comma
					excludedClasses.push_back(substr);
					logger::info("Excluding class: {}", substr);
				}
			}

			if (Settings::GetSingleton()->experimental.methodPrefixesToExcludeFromSpeedup != "") {
				auto methodPrefixes = Settings::GetSingleton()->experimental.methodPrefixesToExcludeFromSpeedup;
				methodPrefixes.erase(remove(methodPrefixes.begin(), methodPrefixes.end(), ' '), methodPrefixes.end());  // Trim whitespace
				std::stringstream method_stream(methodPrefixes);                                                        // create string stream from the string
				while (method_stream.good()) {
					std::string substr;
					getline(method_stream, substr, ',');  // get first string delimited by comma
					excludedMethodPrefixes.push_back(substr);
					logger::info("Excluding method prefix: {}", substr);
				}
			}
		}

		static void ExcludeStackFromSpeedUp(RE::VMStackID a_id)
		{
			excludedStacksLock.lock();
			excludedStacks.insert(a_id);
			excludedStacksLock.unlock();
		}

		static void UnexcludeStackFromSpeedup(RE::VMStackID a_id)
		{
			excludedStacksLock.lock();
			if (excludedStacks.find(a_id) != excludedStacks.end()) {
				excludedStacks.erase(a_id);
			}
			excludedStacksLock.unlock();
		}

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

	struct AttemptFunctionCallHook
	{
		// Use function queue lock around `AttemptFunctionCall` when called from tasklets to prevent concurrent execution of native calls now that they are sped up.
		// The function queue lock is already used in `ProcessMessageQueue` for calling `AttemptFunctionCall` and `AttemptFunctionReturn`,
		// so it makes sense to use the same lock here.
		// This isn't the most sophisticated way to synchronize previously non-sped up native calls as all script functions will sync to the lock,
		// but it is one of the simplest approaches and shouldn't cause any measurable script performance loss outside of specific synthetic tests
		static std::uint64_t thunk(VM* a_vm, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::CodeTasklet>* a_tasklet, bool a_callingFromTasklets)
		{
			a_vm->funcQueueLock.Lock();
			auto result = func(a_vm, a_stack, a_tasklet, a_callingFromTasklets);  // VirtualMachine::AttemptFunctionCall
			a_vm->funcQueueLock.Unlock();
			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			// Note: AE inlines BSScript::Internal::Codetasklet::HandleCall() into BSScript::Internal::Codetasklet::VMProcess()
			// despite having the original now unused function at ID 105204
			// So the AE address/offset is quite different from SE/VR
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98548, 105176), REL::VariantOffset(0x56, 0x82F, 0x56) };
			stl::write_thunk_call<AttemptFunctionCallHook>(target.address());
			logger::info("AttemptFunctionCallHook hooked at address {:x}", target.address());
			logger::info("AttemptFunctionCallHook hooked at offset {:x}", target.offset());
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
			assert(xorCode.getSize() < 0x7);
			REL::safe_write(target.address(), xorCode.getCode(), xorCode.getSize());

			logger::info("Hooked BypassCorruptSaveHook at address {:x}", target.address());
			logger::info("Hooked BypassCorruptSaveHook at offset {:x}", target.offset());
		}
	};

	// Keep `IgnoreMemoryLimit` flag to 1 regardless of VM overstressed status
	// In other words, if VM is NOT overstressed, change the pseudocode `ADJ(skyrimVM)->memoryPagePolicy.ignoreMemoryLimit = 0;` to `ADJ(skyrimVM)->memoryPagePolicy.ignoreMemoryLimit = 1;`
	struct KeepIgnoreMemoryLimitFlag
	{
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(53195, 54006), REL::VariantOffset(0x101, 0x1AD, 0x101) };

			REL::safe_fill(target.address(), REL::NOP, 0x7);
			if (REL::Module::IsAE()) {
				// AE
				std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x86, (std::byte)0x94, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 };  // mov    BYTE PTR [rsi+0x94],0x1
				REL::safe_write(target.address(), setMemoryLimitCode, 0x7);
			} else if (REL::Module::IsSE()) {
				// SE
				std::byte setMemoryLimitCode[] = { (std::byte)0xc6, (std::byte)0x81, (std::byte)0x94, (std::byte)0, (std::byte)0, (std::byte)0, (std::byte)1 };  // mov    BYTE PTR [rcx+0x94],0x1
				REL::safe_write(target.address(), setMemoryLimitCode, 0x7);
			} else {
				// VR
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

		if (settings->experimental.speedUpNativeCalls) {
			CallableFromTaskletInterceptHook::Install();
			AttemptFunctionCallHook::Install();
		}
		if (settings->experimental.bypassCorruptedSave) {
			BypassCorruptSaveHook::Install();
		}
		if (settings->experimental.bypassMemoryLimit) {
			KeepIgnoreMemoryLimitFlag::Install();
		}
	}
}
