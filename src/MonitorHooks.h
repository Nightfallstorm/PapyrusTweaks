#pragma once
/* TODO:
* 1. Monitor memory usage
* 2. Monitor stack dumps
* 3. 
*/
namespace MonitorHooks
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

	// TODO change hook from the logging of stack dumps to actual stack dump function
	struct StackDumpHook
	{
		
		
		static std::int32_t thunk(std::uint64_t logger, std::int64_t dumpString, std::uint32_t unk0)
		{
			logger::info("Stack dump detected!");
			RE::DebugMessageBox("Stack dump detected! Check papyrus logs")
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

	static inline void InstallHooks()
	{
		StackDumpHook::Install();
	}
}
