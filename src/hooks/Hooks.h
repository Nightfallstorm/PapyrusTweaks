#pragma once

/* Static initialized variables
 * This allows hooks to register themselves (and their size) without needing to hardcode reference every single
 * hook/hooksize. As a downside, this requires each hook header to be included from its own `cpp` file to ensure
 * it's not optimized away during compiling, but is worth it for the maintenance upside
 */
#define REGISTER_HOOK_CUSTOM_SIZE(size) static inline auto __hookregister_custom = hooks::accumulateHookSize(size);

#define REGISTER_HOOK_THUNK(hookCount) static inline auto __hookregister_thunks = hooks::accumulateHookSize(hookCount * jumpTrampolineSize);

#define REGISTER_HOOK(HookType) static inline auto __hookregister_type = hooks::registerHook(#HookType, HookType::Install);

static constexpr auto jumpTrampolineSize = 0x14;

namespace hooks {
	using hookInstallFunc = void();
	bool registerHook(const std::string& name, hookInstallFunc* installFunc);

	bool accumulateHookSize(const std::uint32_t& size);

	std::uint32_t getHookTrampolineSize();

	void InstallHooks();
}
