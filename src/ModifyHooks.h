#pragma once
/* TODO:
* 1. Throw stack overflows to prevent a script from causing FPS drops when 1000+ calls deep in a stack
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

	static inline void InstallHooks()
	{
		// TODO
	}
}
