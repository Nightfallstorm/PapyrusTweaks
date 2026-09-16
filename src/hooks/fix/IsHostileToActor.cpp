#include "IsHostileToActor.h"

namespace
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using isHostileToActorFunc = bool(VM* a_vm, RE::VMStackID a_stackID, RE::Actor* a_self, RE::Actor* a_other);

	inline REL::Relocation<isHostileToActorFunc> IsHostileToActor;  // Original IsHostileToActor

	// Actor.IsHostileToActor(Actor akActor) crashes when akActor is NONE.
	// We will instead log an error and return false back, fixing the crash
	// Note: a_self should never be null, since this function just cannot be invoked with a null self actor
	bool IsHostileToActorEx(VM* a_vm, RE::VMStackID a_stackID, RE::Actor* a_self, RE::Actor* a_other) {
		if (!a_other) {
			a_vm->TraceStack(
				"Actor argument is NONE for Actor.IsHostileToActor()! Mod authors: This normally crashes the game in vanilla, but is fixed by Papyrus Tweaks NG",
				a_stackID,
				RE::BSScript::ErrorLogger::Severity::kError
			);
			return false;
		}
		return IsHostileToActor(a_vm, a_stackID, a_self, a_other);
	}


}

namespace hooks::fix::ishostiletoactor
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	std::uint64_t FixIsHostileToActorCrash::thunk(std::uint64_t unk, char* functionName, char* className, std::uintptr_t callback, VM** a_vm)
	{
		IsHostileToActor = callback; // Store the original so we can invoke it later
		return func(unk, functionName, className, reinterpret_cast<std::uintptr_t>(IsHostileToActorEx), a_vm);
	}
}
