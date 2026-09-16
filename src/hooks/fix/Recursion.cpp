#include "Recursion.h"

namespace
{
	bool IsCallInStack(RE::BSScript::Stack* a_stack, const char* scriptName, const char* functionName) {
		RE::BSScript::StackFrame* stackFrame = a_stack->top;
		if (stackFrame == nullptr) {
			return false;
		}
		stackFrame = stackFrame->previousFrame; // Get the frame before the current function call, as we don't want to check against ourselves
		while (stackFrame != nullptr) { // Loop through all frames in the stack
			if (stackFrame->owningFunction && stackFrame->owningFunction.get()) {
				if (string::iequals(std::string(stackFrame->owningFunction->GetObjectTypeName().c_str()), scriptName) &&
					string::iequals(std::string(stackFrame->owningFunction->GetName().c_str()), functionName)) {
					return true;
					}
			}
			stackFrame = stackFrame->previousFrame;
		}
		return false;
	}
}

namespace hooks::fix::recursion
{
	RE::BSFixedString* StackOverFlowHook::thunk(std::uint64_t unk0, RE::BSScript::Stack* a_stack, std::uint64_t* a_funcCallQuery)
	{
		if (a_stack != nullptr && a_stack->frames > 1000) {
			RE::BSScript::Internal::IFuncCallQuery::CallType ignore;
			RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> scriptInfo;
			RE::BSScript::Variable ignore2;
			RE::BSScrapArray<RE::BSScript::Variable> ignore3;
			RE::BSFixedString functionName;
			if (a_stack->owningTasklet == nullptr) {
				// Unlikely to ever occur, but just in case
				return func(unk0, a_stack, a_funcCallQuery);
			}
			a_stack->owningTasklet->GetFunctionCallInfo(ignore, scriptInfo, functionName, ignore2, ignore3);
			logger::info("Detected 1000+  call on function {} for script {}", functionName, scriptInfo->GetName());
			if (IsCallInStack(a_stack, scriptInfo->GetName(), functionName.c_str()) == true) {
				// Break the recursion
				*a_funcCallQuery = 0;
			} else {
				// might be a regular native call or something not directly causing recursion, don't break it yet
			}

		}
		return func(unk0, a_stack, a_funcCallQuery);
	}

	void StackOverFlowLogHook::thunk(RE::BSScript::Stack* a_stack, const char* a_source, std::uint32_t unk2, char* unk3, std::uint32_t sizeInBytes)
	{
		if (a_stack != nullptr && a_stack->frames > 1000) {
			func(a_stack, "StackFrameOverFlow exception, function call exceeded 1000 call stack limit - returning None", unk2, unk3, sizeInBytes);
		} else {
			func(a_stack, a_source, unk2, unk3, sizeInBytes);
		}
	}
}