#include "NativeSpeedUp.h"

#include "Util.h"
#include "configuration/Settings.h"

#include <string>

namespace
{
	std::mutex functionMutex;

	std::vector<std::string> excludedClasses;
	std::vector<std::string> excludedMethodPrefixes;
	std::set<RE::VMStackID> excludedStacks;
	std::mutex excludedStacksLock;

}

namespace hooks::performance::nativespeedup
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	void CallableFromTaskletInterceptHook::ExcludeStackFromSpeedUp(const RE::VMStackID a_id)
	{
		excludedStacksLock.lock();
		excludedStacks.insert(a_id);
		excludedStacksLock.unlock();
	}

	void CallableFromTaskletInterceptHook::UnexcludeStackFromSpeedup(const RE::VMStackID a_id)
	{
		excludedStacksLock.lock();
		if (excludedStacks.contains(a_id)) {
			excludedStacks.erase(a_id);
		}
		excludedStacksLock.unlock();
	}

	bool CallableFromTaskletInterceptHook::callableFromTaskletCheckIntercept(
		RE::BSScript::IFunction* a_function,
		[[maybe_unused]] bool a_callbableFromTasklets,
		[[maybe_unused]] RE::BSScript::Stack* a_stack
	)
	{
		if (a_function->CanBeCalledFromTasklets()) {
			// already fast, no need to check excluded functions
			return true;
		}

		if (a_function->GetIsNative()) {
			auto nativeFunction = reinterpret_cast<RE::BSScript::NF_util::NativeFunctionBase*>(a_function);
			if (nativeFunction->GetIsLatent()) {
				// is latent, return false to keep it delayed since it takes real-world time anyways
				return false;
			}
		}

		excludedStacksLock.lock();
		if (excludedStacks.contains(a_stack->stackID)) {
			// stack called PapyrusTweaks.DisableFastMode(), return false to keep normal behavior
			excludedStacksLock.unlock();
			return false;
		}
		excludedStacksLock.unlock();

		for (const auto& excludedClass : excludedClasses) {
			if (string::iequals(a_function->GetObjectTypeName(), excludedClass)) {
				return false;
			}
		}

		for (const auto& methodPrefix : excludedMethodPrefixes) {
			if (string::istartsWith(a_function->GetName(), methodPrefix)) {
				return false;
			}
		}
#ifdef _DEBUG
		logger::info("Speeding up {}.{}", a_function->GetObjectTypeName(), a_function->GetName());
#endif

		return true;
	}

	void CallableFromTaskletInterceptHook::InitBlacklist()
	{
		if (!Settings::GetSingleton()->experimental.classesToExcludeFromSpeedUp.empty()) {
			auto classes = Settings::GetSingleton()->experimental.classesToExcludeFromSpeedUp;
			classes.erase(std::ranges::remove(classes, ' ').begin(), classes.end());  // Trim whitespace
			classes.erase(std::ranges::remove(classes, '	').begin(), classes.end());  // Trim tabs
			std::stringstream class_stream(classes);                                    // create string stream from the string
			while (class_stream.good()) {
				std::string substr;
				getline(class_stream, substr, ',');  //get first string delimited by comma
				excludedClasses.push_back(substr);
				logger::info("Excluding class: {}", substr);
			}
		}

		if (!Settings::GetSingleton()->experimental.methodPrefixesToExcludeFromSpeedup.empty()) {
			auto methodPrefixes = Settings::GetSingleton()->experimental.methodPrefixesToExcludeFromSpeedup;
			methodPrefixes.erase(std::ranges::remove(methodPrefixes, ' ').begin(), methodPrefixes.end());  // Trim whitespace
			methodPrefixes.erase(std::ranges::remove(methodPrefixes, '	').begin(), methodPrefixes.end());  // Trim tabs
			std::stringstream method_stream(methodPrefixes);                                                        // create string stream from the string
			while (method_stream.good()) {
				std::string substr;
				getline(method_stream, substr, ',');  // get first string delimited by comma
				excludedMethodPrefixes.push_back(substr);
				logger::info("Excluding method prefix: {}", substr);
			}
		}
	}

	std::uint64_t AttemptFunctionCallHook::thunk(VM* a_vm, RE::BSScript::Stack* a_stack, RE::BSTSmartPointer<RE::BSScript::Internal::CodeTasklet>* a_tasklet, bool a_callingFromTasklets)
	{
		std::lock_guard lock_guard(functionMutex);
		return func(a_vm, a_stack, a_tasklet, a_callingFromTasklets);  // VirtualMachine::AttemptFunctionCall
	}
}
