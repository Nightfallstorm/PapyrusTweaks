#include "ToggleScript.h"

namespace
{
	// Only unfreeze script processing if script processing is enabled
	void unfreezeScripts(RE::SkyrimVM* a_this, const bool a_frozen)
	{
		if (RE::Script::GetProcessScripts()) {
			a_this->GetVMRuntimeData().frozenLock.Lock();
			a_this->GetVMRuntimeData().isFrozen = a_frozen;
			a_this->GetVMRuntimeData().frozenLock.Unlock();
		}
	}
}

namespace hooks::fix::togglescript
{
	void FixToggleScriptsSaveHook::thunk(RE::SkyrimVM* a_this, const bool a_frozen)
	{
		unfreezeScripts(a_this, a_frozen);
	}

	void FixToggleScriptsDumpHook::thunk(RE::SkyrimVM* a_this, const bool a_frozen)
	{
		unfreezeScripts(a_this, a_frozen);
	}
}