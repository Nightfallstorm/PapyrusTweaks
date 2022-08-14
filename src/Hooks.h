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

	struct StackDumpHook
	{
		static std::int32_t thunk(std::uint64_t logger, std::int64_t dumpString, std::uint32_t unk0)
		{
			logger::info("Stack dump detected!");
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
			REL::Relocation<std::uintptr_t> target{ REL_ID(53195, 00000), OFFSET_3(0xE3, 0x0, 0x0) };  // TODO: AE and VR
			write_thunk_call<StackDumpHook>(target.address());

			logger::info("Stack Dump Detection hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("Stack Dump Detection hooked at offset " + fmt::format("{:x}", target.offset()));


		}
	};

	// TODO: SSE Display Tweaks detects overstress well before this function, find out how?
	// ALSO, marking this hook as "close to stack dumping"
	struct SecondaryVMOverstressHook
	{
		static std::int64_t thunk(VM* vm)
		{
			logger::info("Secondary Overstress detected!");
			return func(vm);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			// int64 __fastcall BSTEventSink_BSScript__StatsEvent_::Handle_14092A9C0(BSTEventSink_BSScript__StatsEvent_ *a1, int64 a2) function call
			// Hook when VM overstresses as SkyrimVM::sub_14092D320(SkyrimVM * this);
			// 1.6.353: TODO
			// 1.5.97: 14092AA01 (Closest ID is 14092A9C0, 53195)
			// 1.4.15: TODO

			REL::Relocation<std::uintptr_t> target{ REL_ID(53195, 00000), OFFSET_3(0x41, 0x0, 0x0) };  // TODO: AE and VR
			write_thunk_call<SecondaryVMOverstressHook>(target.address());

			logger::info("VM Overstressed Detection hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("VM Overstressed Detection hooked at offset " + fmt::format("{:x}", target.offset()));
		}
	};

	// TODO:: AE and VR
	struct VMProcessUpdatesHook
	{
		static std::int64_t thunk()
		{
			float percent = static_cast<float>(RE::SkyrimVM::GetSingleton()->memoryPagePolicy.currentMemorySize) / 
				static_cast<float>(RE::SkyrimVM::GetSingleton()->memoryPagePolicy.maxAllocatedMemory);
			int realPercent = static_cast<int>(percent * 100.0);
			std::string message = std::format("Memory usage: {}%", realPercent);
			//RE::DebugNotification(message.c_str());
			std::string otherMessage = std::format("Extra Memory used: {}", RE::SkyrimVM::GetSingleton()->memoryPagePolicy.maxAdditionalAllocations);
			RE::DebugNotification(otherMessage.c_str());
			return func();
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

			REL::Relocation<std::uintptr_t> target{ REL_ID(53115, 00000), OFFSET_3(0x5A, 0x0, 0x0) };  // TODO: AE and VR
			write_thunk_call<VMProcessUpdatesHook>(target.address());

			logger::info("VM Process Updates hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("VM Process Updates hooked at offset " + fmt::format("{:x}", target.offset()));
		}
	};
}
