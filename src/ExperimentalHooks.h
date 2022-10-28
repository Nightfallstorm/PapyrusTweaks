#pragma once
#include <xbyak/xbyak.h>
#include "Settings.h"

namespace ExperimentalHooks
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
		auto settings = Settings::GetSingleton();
		if (settings->experimental.speedUpGameGetPlayer) {
			VM::GetSingleton()->SetCallableFromTasklets("Game", "GetPlayer", true);
			logger::info("Applied Game.GetPlayer speed up");
		} else {
			logger::info("Did not apply game.getplayer speed up");
		}
	}
}
