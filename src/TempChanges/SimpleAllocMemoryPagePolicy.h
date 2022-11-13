#pragma once

#include "RE/B/BSAtomic.h"
#include "RE/Offsets.h"
#include "RE/Offsets_VTABLE.h"
#include "IMemoryPagePolicy.h"

// TEMPORARY HEADER DEFINITION UNTIL CLIB-NG UPDATES
namespace RETEMP
{
	namespace BSScript
	{
		class SimpleAllocMemoryPagePolicy : public IMemoryPagePolicy
		{
		public:
			inline static constexpr auto RTTI = RE::RTTI_BSScript__SimpleAllocMemoryPagePolicy;
			inline static constexpr auto VTABLE = RE::VTABLE_BSScript__SimpleAllocMemoryPagePolicy;
		};
	}
}
