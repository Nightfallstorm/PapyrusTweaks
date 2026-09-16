#include "VRPlayroom.h"

namespace
{
	bool hasEnteredPlayroom = false;
	REL::Relocation<bool*> isInPlayroom{ REL::Offset(0x2FEB1C0) };
	REL::Relocation<bool*> bLoadVRPlayroom{ REL::Offset(0x1EAC188) };
}

namespace
{
	bool IsVRPlayroomQuestInStack(const RE::BSScript::Stack* a_stack, const RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery)
	{
		RE::BSScript::StackFrame* stackFrame = a_stack->top;
		if (stackFrame == nullptr) {
			RE::BSScript::Internal::IFuncCallQuery::CallType ignore;
			RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> scriptInfo;
			RE::BSScript::Variable ignore2;
			RE::BSScrapArray<RE::BSScript::Variable> ignore3;
			RE::BSFixedString ignore1;
			a_funcCallQuery->get()->GetFunctionCallInfo(ignore, scriptInfo, ignore1, ignore2, ignore3);
			if (scriptInfo && scriptInfo.get() && string::icontains(scriptInfo->GetName(), "vrplayroom")) {
				return true;  // function query is for VRPlayroom stuff, but a stackframe may not have been created yet, let it through
			}
			return false;
		}
		while (stackFrame != nullptr) {  // Loop through all frames in the stack
			if (stackFrame->owningFunction && stackFrame->owningFunction.get()) {
				if (string::icontains(std::string(stackFrame->owningFunction.get()->GetObjectTypeName().c_str()), "vrplayroom")) {
					return true;
				}
			}
			stackFrame = stackFrame->previousFrame;
		}
		return false;
	}
}

namespace hooks::tweak::vrplayroom
{
	bool VRPlayroomScriptDisable::stackCheckIntercept(std::uint32_t a_objectPackedData, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery>* a_funcCallQuery)
	{
		// original check
		if ((a_objectPackedData & 1) == 0 || ((a_objectPackedData & 2) == 0 && a_stack->stackType.underlying() - 1 > 1)) {
			return false;
		}

		if (!*(bLoadVRPlayroom.get())) {
			// skip VRPlayroom checks if player has bLoadVRPlayroom disabled
			return true;
		}

		// add new playroom check. Only allow calls that have VRPlayroomQuest somewhere in the stack to proceed
		// All Quests can start initializing, including VRPlayroomQuest, before VRPlayroomQuest set `isInPlayroom` to true.
		// Therefore, we will track when the player has entered the playroom, to only allow VRPlayroomQuest until that flag is set
		if (!hasEnteredPlayroom && *(isInPlayroom.get())) {
			logger::info("We entered the playroom!");
			hasEnteredPlayroom = true;
		}

		// check if player hasn't entered the playroom yet (only allow VRPlayroomQuest), or has entered the playroom AND is still in there
		if (!hasEnteredPlayroom || (hasEnteredPlayroom && *(isInPlayroom.get()))) {
			if (!IsVRPlayroomQuestInStack(a_stack, a_funcCallQuery)) {
				return false;
			}
		}

		return true;
	}

	std::uint64_t ReturnToMainMenuHook::thunk(std::uint64_t unk0, std::uint64_t unk1, std::uint64_t unk2, std::uint64_t unk3)
	{
		hasEnteredPlayroom = false;  // reset hasEnteredPlayroom since we are returning to main menu
		return func(unk0, unk1, unk2, unk3);
	}

	// Stack dumps occur since we are intentionally pausing all the mod scripts that aren't "VRPlayroom"
	// Stack dumps are harmless anyway, but to prevent log spam, we will skip stack dumping while the player is in the playroom
	void StackDumpBlockHook::thunk(RE::SkyrimVM* a_vm)
	{
		if (*bLoadVRPlayroom.get() && *isInPlayroom.get()) {
			// Don't stack dump
			return;
		}
		return func(a_vm);
	}

	void LogStackDumpBlockHook::thunk(std::uint64_t unk0, std::uint64_t unk1, std::uint64_t unk2)
	{
		if (*bLoadVRPlayroom.get() && *isInPlayroom.get()) {
			// Don't log a stack dump occurring
			return;
		}
		return func(unk0, unk1, unk2);
	}
}