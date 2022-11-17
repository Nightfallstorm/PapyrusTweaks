#pragma once

namespace Papyrus
{
	#define BIND(a_method, ...) a_vm->RegisterFunction(#a_method##sv, script, a_method __VA_OPT__(, ) __VA_ARGS__)
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	inline constexpr auto script = "PapyrusTweaks"sv;

	bool IsTweakInstalled(VM*, StackID, RE::StaticFunctionTag*, RE::BSFixedString a_tweak);

	bool DisableFastMode(VM*, StackID stackID, RE::StaticFunctionTag*);

	bool EnableFastMode(VM*, StackID stackID, RE::StaticFunctionTag*);

	bool Bind(VM* a_vm);
}
