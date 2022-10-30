#pragma once
#include "Settings.h"
#include <xbyak/xbyak.h>

namespace ExperimentalHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

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
