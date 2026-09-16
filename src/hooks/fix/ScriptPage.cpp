#include "ScriptPage.h"

namespace hooks::fix::scriptpage
{
	// BSScript::SimpleAllocMemoryPagePolicy::GetLargestAvailablePage
	RE::BSScript::IMemoryPagePolicy::AllocationStatus FixScriptPageAllocation::thunk(RE::BSScript::SimpleAllocMemoryPagePolicy* self, RE::BSTAutoPointer<RE::BSScript::MemoryPage>& a_newPage)
	{
		// TODO: This seems like a weird half-fix. Check if `GetLargestAvailablePage` could be shortcutted to just return kOutOfMemory instead of invoking the original func
		/*if (self->maxAllocatedMemory < self->currentMemorySize) {
			return RE::BSScript::IMemoryPagePolicy::AllocationStatus::kOutOfMemory;
		}*/

		self->dataLock.Lock();
		const int availablePageSize = self->maxAllocatedMemory - self->currentMemorySize;
		const std::uint32_t currentMemorySizeTemp = self->currentMemorySize;
		if (availablePageSize < 0) {
			// set equal so the original function will return kOutOfMemory instead of unintentionally allocating a page
			self->currentMemorySize = self->maxAllocatedMemory;
		}
		const RE::BSScript::IMemoryPagePolicy::AllocationStatus result = func(self, a_newPage);
		if (availablePageSize < 0) {
			// set back to original size
			self->currentMemorySize = currentMemorySizeTemp;
		}
		self->dataLock.Unlock();
		return result;
	}
}
