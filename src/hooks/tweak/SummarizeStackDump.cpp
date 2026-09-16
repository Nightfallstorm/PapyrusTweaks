#include "SummarizeStackDump.h"

namespace
{
	std::map<std::string, std::uint32_t> eventMap;
	std::set<RE::BSScript::Stack*> parsedStacks;

	void parseStack(RE::BSTSmartPointer<RE::BSScript::Stack> a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::IFuncCallQuery> a_funcInfo) {
		if (!a_stack || !a_stack.get() || parsedStacks.find(a_stack.get()) != parsedStacks.end()) {
			// prevent parsing a stack multiple times
			return;
		}

		if (a_stack->frames && a_stack->top) {
			auto stackframe = a_stack->top;
			while (stackframe->previousFrame != nullptr) {
				stackframe = stackframe->previousFrame;
			}
			auto function = stackframe->owningFunction.get();
			if (function) {
				auto eventName = std::format("{}.{}", function->GetObjectTypeName().c_str(), function->GetName().c_str());  // ex: QuestScript.NewEvent();
				if (eventMap.contains(eventName)) {
					auto currentFrequency = eventMap.at(eventName);
					eventMap.erase(eventName);
					eventMap.emplace(eventName, ++currentFrequency);
				} else {
					eventMap.emplace(eventName, 1);
				}
			}
			parsedStacks.insert(a_stack.get());
		} else if (a_funcInfo && a_funcInfo.get()) {
			RE::BSScript::Internal::IFuncCallQuery::CallType ignore;
			RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> typeInfo;
			RE::BSFixedString functionName;
			RE::BSScript::Variable ignore1;
			RE::BSScrapArray<RE::BSScript::Variable> ignore2;
			a_funcInfo->GetFunctionCallInfo(ignore, typeInfo, functionName, ignore1, ignore2);
			auto eventName = std::format("{}.{}", typeInfo->GetName(), functionName.c_str());  // ex: QuestScript.NewEvent();
			if (eventMap.contains(eventName)) {
				auto currentFrequency = eventMap.at(eventName);
				eventMap.erase(eventName);
				eventMap.emplace(eventName, ++currentFrequency);
			} else {
				eventMap.emplace(eventName, 1);
			}
			parsedStacks.insert(a_stack.get());
		} else {

		}
	}
}

namespace hooks::tweak::summarizeStackDump
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	// Print out events by order of frequency, to more easily see which events are being spammy/problematic
	void SummarizeStackDumpHook::thunk(RE::BSScript::IVMDebugInterface* a_vm)
		{
			auto VM = VM::GetSingleton();
			VM->errorLogger->PostErrorImpl("###############Stack Dump Summary Start################", RE::BSScript::ErrorLogger::Severity::kInfo);
			for (auto currentFuncMsg = VM->funcMsgQueue.head; currentFuncMsg; currentFuncMsg = currentFuncMsg->next) {
				auto message = reinterpret_cast<RE::BSScript::Internal::FunctionMessage*>(&(currentFuncMsg->elem));
				parseStack(message->stack, message->funcQuery);
			}
			for (const auto& currentFuncMsg : VM->overflowFuncMsgs) {
				parseStack(currentFuncMsg.stack, currentFuncMsg.funcQuery);
			}
			auto currentStackQueue = VM->stacksToResume;
			auto currentEntryIndex = VM->stacksToResume->numEntries;
			for (auto index = currentStackQueue->popIdx; currentEntryIndex; index = (index + 1) & 0x7F) {
				--currentEntryIndex;
				auto suspendedStacks = reinterpret_cast<RE::BSScript::Internal::SuspendedStack*>(&(currentStackQueue->queueBuffer));
				auto stack = suspendedStacks[index].stack;
				auto funcCallQuery = suspendedStacks[index].funcCallQuery;
				parseStack(stack, funcCallQuery);
			}
			currentStackQueue = VM->stacksToSuspend;
			currentEntryIndex = VM->stacksToSuspend->numEntries;
			for (auto index = currentStackQueue->popIdx; currentEntryIndex; index = (index + 1) & 0x7F) {
				--currentEntryIndex;
				auto suspendedStacks = reinterpret_cast<RE::BSScript::Internal::SuspendedStack*>(&(currentStackQueue->queueBuffer));
				auto stack = suspendedStacks[index].stack;
				auto funcCallQuery = suspendedStacks[index].funcCallQuery;
				parseStack(stack, funcCallQuery);
			}

			for (const auto& suspendedStack : *(VM->stacksToResumeOverflow)) {
				parseStack(suspendedStack.stack, suspendedStack.funcCallQuery);
			}

			for (const auto& suspendedStack : *(VM->stacksToSuspendOverflow)) {
				parseStack(suspendedStack.stack, suspendedStack.funcCallQuery);
			}

			for (const auto& [stackID, stack] : VM->allRunningStacks) {
				parseStack(stack, nullptr);
			}
			auto sortedEvents = Util::stl::flip_map(eventMap); // Sort by frequency
			for (auto [frequency, eventName] : sortedEvents) {
				VM->errorLogger->PostErrorImpl(std::format("Event: {}, Frequency: {}", eventName, frequency).c_str(), RE::BSScript::ErrorLogger::Severity::kInfo);
			}
			VM->errorLogger->PostErrorImpl("###############Stack Dump Summary End##################", RE::BSScript::ErrorLogger::Severity::kInfo);
			eventMap.clear();
			parsedStacks.clear();
			return func(a_vm);
		}
}