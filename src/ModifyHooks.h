#pragma once
#include <xbyak/xbyak.h>
#include "Settings.h"
/* TODO:
* 1. Throw stack overflows to prevent a script from causing FPS drops when 1000+ calls deep in a stack - Done!
* 2. Increase Stack Dump timeout threshold (maybe increase stackcount threshold too?)
*/
namespace ModifyHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	// Putting this here avoids a compile error when used in PCH
	template <class T>
	static void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);

		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}	

	struct StackOverFlowHook
	{
		static RE::BSFixedString* thunk(std::uint64_t unk0, RE::BSScript::Stack* a_stack, std::uint64_t* a_funcCallQuery)
		{
			if (a_stack != nullptr && a_stack->frames > 1000) {
				RE::BSScript::Internal::IFuncCallQuery::CallType ignore;
				RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> scriptInfo;
				RE::BSScript::Variable ignore2;
				RE::BSScrapArray<RE::BSScript::Variable> ignore3;
				RE::BSFixedString functionName;
				a_stack->owningTasklet.get()->GetFunctionCallInfo(ignore, scriptInfo, functionName, ignore2, ignore3);
				logger::info("Detected 1000+ recursive call on function {} for script {}", functionName, scriptInfo.get()->GetName());
				auto message = std::format("Warning, function {} in script {} got stuck in a recursion loop. Exited loop to prevent performance issues. Please notify author to fix and check papyrus logs for more info", functionName.c_str(), scriptInfo.get()->GetName());
				RE::DebugMessageBox(message.c_str());
				*a_funcCallQuery = 0;
			}
			return func(unk0, a_stack, a_funcCallQuery);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98130, 104853), OFFSET_3(0x7F, 0x7F, 0x7F) };
			write_thunk_call<StackOverFlowHook>(target.address());

			logger::info("StackFrameOverFlow hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("StackFrameOverFlow hooked at offset " + fmt::format("{:x}", target.offset()));
		}
	};

	struct StackOverFlowLogHook
	{
		static void thunk(RE::BSScript::Stack* a_stack, const char* a_source, std::uint32_t unk2, char* unk3, std::uint32_t sizeInBytes)
		{
			if (a_stack != nullptr && a_stack->frames > 1000) {
				func(a_stack, "StackFrameOverFlow exception, function call exceeded 1000 call stack limit - returning None", unk2, unk3, sizeInBytes);
			} else {
				func(a_stack, a_source, unk2, unk3, sizeInBytes);
			}
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98130, 104853), OFFSET_3(0x963, 0x97A, 0x963) };
			write_thunk_call<StackOverFlowLogHook>(target.address());

			logger::info("StackFrameOverFlowLog hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("StackFrameOverFlowLog hooked at offset " + fmt::format("{:x}", target.offset()));
		}
	};

	struct PapyrusOpsPerFrameHook
	{
		struct PapyrusOpsModifier : Xbyak::CodeGenerator
		{
			PapyrusOpsModifier(std::uintptr_t beginLoop, std::uintptr_t endLoop)
			{
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
		};

		// Install our hook at the specified address
		static inline void Install()
		{
			
			REL::Relocation<std::uintptr_t> target{ REL_ID(98520, 0), OFFSET_3(0x498, 0x0, 0x0) };  // TODO: AE, VR looks to match needs testing
			REL::Relocation<std::uintptr_t> beginLoop{ REL_ID(98520, 0), OFFSET_3(0xC0, 0x0, 0x0) };
			REL::Relocation<std::uintptr_t> endLoop{ REL_ID(98520, 0), OFFSET_3(0x4AB, 0x0, 0x0) };

			auto newCompareCheck = PapyrusOpsModifier(beginLoop.address(), endLoop.address());
			REL::safe_fill(target.address(), REL::NOP, 0x13);
			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(newCompareCheck.getSize());
			auto result = trampoline.allocate(newCompareCheck);
			auto& trampoline2 = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			auto res = trampoline2.write_branch<5>(target.address(), (std::uintptr_t) result);

			logger::info("PapyrusOpsPerFrameHook hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("PapyrusOpsPerFrameHook hooked at offset " + fmt::format("{:x}", target.offset()));
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

		static inline void installModifier(int timeoutMS) {
			REL::Relocation<std::uintptr_t> target{ REL_ID(53195, 0), OFFSET_3(0x6E, 0x0, 0x0) };
			auto newTimeoutCheck = StackDumpTimeoutModifier(timeoutMS);
			REL::safe_fill(target.address(), REL::NOP, 0x5); // Fill with NOP just in case
			REL::safe_write(target.address(), newTimeoutCheck.getCode(), newTimeoutCheck.getSize());

			logger::info("StackDumpTimeoutModifier hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("StackDumpTimeoutModifier hooked at offset " + fmt::format("{:x}", target.offset()));
		}

		static inline void installDisable() {
			REL::Relocation<std::uintptr_t> target{ REL_ID(53195, 0), OFFSET_3(0x6E, 0x0, 0x0) }; // TODO AE and VR
			REL::safe_fill(target.address(), REL::NOP, 0x21); // Disable the checks AND disable the stackdump flag
			logger::info("StackDumpTimeoutDisable hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("StackDumpTimeoutDisable hooked at offset " + fmt::format("{:x}", target.offset()));
		}
	};

	static inline void InstallHooks()
	{
		auto settings = Settings::GetSingleton();
		if (settings->fixes.recursionFix) {
			//StackOverFlowHook::Install();
			//StackOverFlowLogHook::Install();
		}
		StackDumpTimeoutHook::Install();
		PapyrusOpsPerFrameHook::Install();
	}
}
