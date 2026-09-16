#include "Hooks.h"

#include <map>

namespace
{
	// Static local function ensures this map will be ready on first insert from static-initialized hooks
	std::map<std::string, void*>& hookMap()
	{
		static std::map<std::string, void*> map;
		return map;
	}

	// Same here, static local function to ensure initialization before use by other static-initialized logic
	std::uint32_t& totalHookTrampolineSize()
	{
		static std::uint32_t size = 0;
		return size;
	}
}

namespace hooks
{
	bool registerHook(const std::string& name, hookInstallFunc* installFunc)
	{
		hookMap().insert(std::make_pair(name, (void*) installFunc));
		return true;
	}

	bool accumulateHookSize(const std::uint32_t& size) {
		totalHookTrampolineSize() += size;

		return true;
	}

	std::uint32_t getHookTrampolineSize()
	{
		return totalHookTrampolineSize();
	}


	void InstallHooks()
	{
		for (const auto& [name, installFunc] : hookMap()) {
			logger::info("Evaluating hook {}", name);
			const auto func = reinterpret_cast<hookInstallFunc*>(installFunc);
			func();
		}
	}
}
