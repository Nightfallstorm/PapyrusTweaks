#pragma once
#include "Settings.h"
#include <xbyak/xbyak.h>

namespace VRHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	static bool hasEnteredPlayroom = false;
	struct VRPlayroomScriptDisable
	{
		// keep original checks, but also add VRPlayroom check for non-VRPlayroom scripts
		static bool stackCheckIntercept(
			std::uint32_t a_objectPackedData, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery)
		{
			// original check
			if ((a_objectPackedData & 1) == 0 || ((a_objectPackedData & 2) == 0 && a_stack->stackType.underlying() - 1 > 1)) {
				return false;
			}

			// add new playroom check. Only allow calls that have VRPlayroomQuest somewhere in the stack to proceed
			bool* isInPlayroomPtr = reinterpret_cast<bool*>(REL::Offset(0x2FEB1C0).address());
			bool isInPlayroom = *isInPlayroomPtr;
			// All Quests can start initializing, including VRPlayroomQuest, before VRPlayroomQuest set `isInPlayroom` to true.
			// Therefore, we will track when the player has entered the playroom, to only allow VRPlayroomQuest until that flag is set
			if (!hasEnteredPlayroom && isInPlayroom) {
				logger::info("We entered the playroom!");
				hasEnteredPlayroom = true;
			}

			// check if player hasn't entered the playroom yet (only allow VRPlayroomQuest), or has entered the playroom AND is still in there
			if (!hasEnteredPlayroom || (hasEnteredPlayroom && isInPlayroom)) {
				if (!IsVRPlayroomQuestInStack(a_stack, a_funcCallQuery)) {
					return false;
				}
			}

			return true;
		}

		static bool IsVRPlayroomQuestInStack(RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery)
		{
			RE::BSScript::StackFrame* stackFrame = a_stack->top;
			if (stackFrame == nullptr) {
				RE::BSScript::Internal::IFuncCallQuery::CallType ignore;
				RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> scriptInfo;
				RE::BSScript::Variable ignore2;
				RE::BSScrapArray<RE::BSScript::Variable> ignore3;
				RE::BSFixedString ignore1;
				a_funcCallQuery->get()->GetFunctionCallInfo(ignore, scriptInfo, ignore1, ignore2, ignore3);
				if (scriptInfo && scriptInfo.get() && findStringIC(scriptInfo.get()->GetName(), "vrplayroom")) {
					return true;  // function query is for VRPlayroom stuff, but a stackframe may not have been created yet, let it through
				}
				return false;
			}
			while (stackFrame != nullptr) {  // Loop through all frames in the stack
				if (stackFrame->owningFunction && stackFrame->owningFunction.get()) {
					if (findStringIC(std::string(stackFrame->owningFunction.get()->GetObjectTypeName().c_str()), "vrplayroom")) {
						return true;
					}
				}
				stackFrame = stackFrame->previousFrame;
			}
			return false;
		}

		// copied from stackoverflow. Just checks if string contained in other string no case sensitivity
		static bool findStringIC(const std::string& strHaystack, const std::string& strNeedle)
		{
			auto it = std::search(
				strHaystack.begin(), strHaystack.end(),
				strNeedle.begin(), strNeedle.end(),
				[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
			return (it != strHaystack.end());
		}

		struct StackCheck : Xbyak::CodeGenerator
		{
			StackCheck(std::uintptr_t jmpIfStackPasses, std::uintptr_t jmpIfStackFails, std::uintptr_t func)
			{
				Xbyak::Label funcLabel;

				mov(rdx, rbx);  // move a_stack into position, rdx will be scratched over after this call
				mov(r8, r12);   // move funcQuery into position, r8 will get scratched over as well
				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				test(al, al);  // rax will be scratched over
				jz("StackFails");
				L("StackPasses");
				mov(rcx, jmpIfStackPasses);  // rcx will be scratched over
				jmp(rcx);
				L("StackFails");
				mov(rcx, jmpIfStackFails);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		// Install our hook at the specified address
		static inline void Install()
		{
			assert(REL::Module::IsVR());
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98130, 0x0), 0x1A5 };
			REL::Relocation<std::uintptr_t> stackPasses{ RELOCATION_ID(98130, 0x0), 0x252 };
			REL::Relocation<std::uintptr_t> stackFails{ RELOCATION_ID(98130, 0x0), 0x1C6 };

			auto stackCheckCode = StackCheck(stackPasses.address(), stackFails.address(), reinterpret_cast<uintptr_t>(stackCheckIntercept));
			REL::safe_fill(target.address(), REL::NOP, 0x21);

			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(stackCheckCode.getSize());
			auto result = trampoline.allocate(stackCheckCode);
			auto& trampoline2 = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline2.write_branch<5>(target.address(), (std::uintptr_t)result);

			logger::info("VRPlayroomScriptDisable hooked at address {:x}", target.address());
			logger::info("VRPlayroomScriptDisable hooked at offset {:x}", target.offset());
		}
	};

	struct ReturnToMainMenuHook
	{
		static std::uint64_t thunk(std::uint64_t unk0, std::uint64_t unk1, std::uint64_t unk2, std::uint64_t unk3)
		{
			hasEnteredPlayroom = false;  // reset hasEnteredPlayroom since we are returning to main menu
			return func(unk0, unk1, unk2, unk3);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			assert(REL::Module::IsVR());
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(52368, 0x0), 0x38 };

			stl::write_thunk_call<ReturnToMainMenuHook>(target.address());

			logger::info("VRPlayroomScriptDisableMainMenuCallback hooked at address {:x}", target.address());
			logger::info("VRPlayroomScriptDisableMainMenuCallback hooked at offset {:X}", target.offset());
		}
	};

	static inline void InstallHooks()
	{
		if (!REL::Module::IsVR()) {
			return;
		}
		if (Settings::GetSingleton()->experimental.disableScriptsInPlayroom) {
			VRPlayroomScriptDisable::Install();
			ReturnToMainMenuHook::Install();
		}
	}
}
