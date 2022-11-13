#pragma once
#include "RE/Offsets.h"
#include "RE/Offsets_VTABLE.h"

// TEMPORARY HEADER DEFINITION UNTIL CLIB-NG UPDATES
namespace RETEMP
{
	namespace BSScript
	{
		struct IMemoryPagePolicy
		{
		public:
			enum class AllocationStatus
			{
				kSuccess,
				kFailed,
				kOutOfMemory
			};
		};
	}
}
