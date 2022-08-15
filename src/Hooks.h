#pragma once

namespace Hooks
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
	
	static std::map<RE::BSScript::IFunction*, int> functionCount;

	// TODO change hook from the logging of stack dumps to actual stack dump functiondddda
	struct StackDumpHook
	{
		
		
		static std::int32_t thunk(std::uint64_t logger, std::int64_t dumpString, std::uint32_t unk0)
		{
			logger::info("Stack dump detected!");
			logger::info("allRunningStacks: size " + std::to_string(VM::GetSingleton()->allRunningStacks.size()));
			VM::GetSingleton()->runningStacksLock.Lock();
			auto stacks = VM::GetSingleton()->allRunningStacks;

			for (auto it = stacks.begin(); it != stacks.end(); ++it) {
				auto stack = it->second;
				if (stack == nullptr || stack.get() == nullptr 
					|| stack.get()->top == nullptr || stack.get()->top->owningFunction == nullptr) {
					logger::info(std::format("stack is unparseable, skipping.. {}", (unsigned long)std::addressof(it)));
					continue;
				}
				auto stackFunction = stack.get()->top->owningFunction;
				auto stackFunc = functionCount.find(stackFunction.get());
				if (stackFunc != functionCount.end()) {
					functionCount[stackFunction.get()]++;
					logger::info("Adding count to stackFunction");
				} else {
					logger::info("Init count to stackFunction");
					functionCount.insert(std::make_pair(stackFunction.get(), 1));
				}
			}
			auto mostCalledFunction = functionCount.begin();
			for (auto function : functionCount) {
				if (function.second > mostCalledFunction->second) {
					mostCalledFunction = functionCount.find(function.first);
				}
			}

			auto message = std::format("a{} has most function calls with {} calls queued",
				mostCalledFunction->first->GetName().c_str(),
				mostCalledFunction->second);
			RE::DebugMessageBox(message.c_str());
			functionCount.clear();
			VM::GetSingleton()->runningStacksLock.Unlock();
			return func(logger, dumpString, unk0);
		}
		

		static inline REL::Relocation<decltype(thunk)> func;

		
		// Install our hook at the specified address
		static inline void Install()
		{
			// int64 __fastcall BSTEventSink_BSScript__StatsEvent_::Handle_14092A9C0(BSTEventSink_BSScript__StatsEvent_ *a1, int64 a2) function call
			// Hook stack dump call as int32 SkyrimScript__Logger::sub_14123FE90(SkyrimScript__Logger *a1, int64 a2, int32 a3)
			// 1.6.353: TODO
			// 1.5.97: 14092AAA3 (Closest ID is 14092A9C0, 53195)
			// 1.4.15: TODO
			REL::Relocation<std::uintptr_t> target{ REL_ID(53195, 00000), REL_OFFSET(0xE3, 0x0, 0x0) };  // TODO: AE and VR
			write_thunk_call<StackDumpHook>(target.address());

			logger::info("Stack Dump Detection hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("Stack Dump Detection hooked at offset " + fmt::format("{:x}", target.offset()));


		}
	};
	static int timeBeforeUpdate;  //temp
	// TODO:: AE and VR
	// Since 
	struct VMProcessUpdatesHook
	{
		
		static const int maxTime = 70;
		static std::int64_t thunk(std::uint64_t shiftedVirtualMachine, RE::BSScript::StatsEvent events)
		{
			timeBeforeUpdate++;
			if (timeBeforeUpdate < maxTime) {
				return func(shiftedVirtualMachine, events);
			}
			if (VM::GetSingleton()->overstressed) {
				//logger::info("VM Overstress detected!");
			}
			auto suspendedStackCount = std::format("runningStacks: {}, suspendedStacks: {}", events.runningStacksCount, events.suspendedStacksCount);  // stacksToResume + stacksToSuspend + their overflows
			RE::DebugNotification(suspendedStackCount.c_str());
			//float percent = static_cast<float>(RE::SkyrimVM::GetSingleton()->memoryPagePolicy.currentMemorySize) / 
			//	static_cast<float>(RE::SkyrimVM::GetSingleton()->memoryPagePolicy.maxAllocatedMemory);
		    //int realPercent = static_cast<int>(percent * 100.0);
			//std::string message = std::format("Memory usage: {}%", realPercent);
			//RE::DebugNotification(message.c_str());
			//std::string otherMessage = std::format("Extra Memory used: {}", RE::SkyrimVM::GetSingleton()->memoryPagePolicy.maxAdditionalAllocations);
			//RE::DebugNotification(otherMessage.c_str());
			timeBeforeUpdate = 0;
			return func(shiftedVirtualMachine, events);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			// int64 __fastcall BSTEventSink_BSScript__StatsEvent_::Handle_14092A9C0(BSTEventSink_BSScript__StatsEvent_ *a1, int64 a2) function call
			// Hook when VM does updates as QueryPerformanceCounterTicks_140C08090();
			// 1.6.353: TODO
			// 1.5.97: 140921F6A (Closest ID is 140921F10, 53115)
			// 1.4.15: TODO

			REL::Relocation<std::uintptr_t> target{ REL_ID(98044, 00000), REL_OFFSET(0x63B, 0x0, 0x0) };  // TODO: AE and VR
			write_thunk_call<VMProcessUpdatesHook>(target.address());

			logger::info("VM Process Updates hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("VM Process Updates hooked at offset " + fmt::format("{:x}", target.offset()));
		}
	};
}
