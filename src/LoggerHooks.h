#pragma once
#pragma warning(2: 4235)
#include "RE/S/ScriptFunction.h"
#include <string.h>
/* TODO:
* 1. "Error: Unable to bind script MCMFlaskUtilsScript to FlaskUtilsMCM (7E007E63) because their base types do not match" - 
* show full inheritance of MCMFlaskUtilsScript and show full types of FlaskUtilMCM
* 2. "Error: File "Telengard.esp" does not exist or is not currently loaded." - Allow optional setting to turn this error off
*/
namespace LoggerHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	// Putting this here avoids a compile error when used in PCH
	template <class T>
	static void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);

		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	// TODO: Test new none check!
	struct ValidationSignaturesHook
	{
		// reimplementation of ValidateSignatureAndFixupNones, but with better logging
		static bool thunk(RE::BSScript::IFunction** a_function, RE::BSScrapArray<RE::BSScript::Variable>* a_varArray, char* a_outString, std::int32_t a_bufferSize)
		{
			if (a_outString != nullptr) {
				*a_outString = 0;
			} else {
				char temp[1024];
				a_outString = temp;
			}
			auto function = *a_function;
			int paramCount = function->GetParamCount();
			if (paramCount == a_varArray->size()) {
				bool mismatchedArgs = false;
				for (int index = 0; index < paramCount; index++) {
					RE::BSFixedString name;
					RE::BSScript::TypeInfo type;
					function->GetParam(index, name, type);
					auto argument = a_varArray->data()[index];
					// check if argument type doesn't fit AND the argument isn't none
					if (!argument.FitsType(type) && !argument.IsNoneObject() && !argument.IsNoneArray()) {
						mismatchedArgs = true;
						break;
					}
				}
				if (mismatchedArgs) {
					char mismatchedArgString[1072];
					std::string mismatchedFormat = "Function %s received incompatible arguments! Received types %s instead!";
					std::string types = "(";
					for (int i = 0; i < a_varArray->size(); i++) {
						types = types + a_varArray->data()[i].GetType().TypeAsString();
						if ((i + 1) < function->GetParamCount()) {
							types = types + ",";
						}
					}
					types = types + ")";
					snprintf(mismatchedArgString, 1024, mismatchedFormat.c_str(),
						ConvertFunctionToString(function).c_str(), types.c_str());
					strcat_s(a_outString, a_bufferSize, mismatchedArgString);
				}
			} else {
				std::string incorrectArgSizeFormat = "Incorrect number of arguments passed to function %s. Expected %u, got %u instead!. ";
				std::string functionName = ConvertFunctionToString(function);
				char incorrectArgString[1072];
				snprintf(incorrectArgString, 1024, incorrectArgSizeFormat.c_str(), functionName.c_str(), paramCount, a_varArray->size());
				strcat_s(a_outString, a_bufferSize, incorrectArgString);
			}
			char discard[1024];
			// Call original validation function to keep behavior the same, but throw away the errors generated from it in favor of ours
			return func(a_function, a_varArray, discard, a_bufferSize);
		}

		static std::string ConvertFunctionToString(RE::BSScript::IFunction* function)
		{
			std::string params = "";
			for (int i = 0; i < function->GetParamCount(); i++) {
				RE::BSFixedString name;
				RE::BSScript::TypeInfo type;
				function->GetParam(i, name, type);
				params = params + type.TypeAsString() + " " + name.c_str();
				if ((i + 1) < function->GetParamCount()) {
					params = params + ",";
				}
			}
			std::string result = std::format(
				"{}.{}({})",
				function->GetObjectTypeName().c_str(),
				function->GetName().c_str(), params);

			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL_ID(98130, 00000), OFFSET_3(0x63D, 0x0, 0x0) };  // TODO: AE and VR
			write_thunk_call<ValidationSignaturesHook>(target.address());

			logger::info("ValidationSignaturesHook hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("ValidationSignaturesHook at offset {}", fmt::format("{:x}", target.offset()));
		}
	};

	// TODO: Make this optional
	struct GetFormFromFileHook
	{
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL_ID(54832, 00000), OFFSET_3(0x7E, 0x0, 0x0) };   // TODO: AE and VR
			REL::safe_fill(target.address(), REL::NOP, 0x5);                                            // Remove the call to setup the log
			REL::Relocation<std::uintptr_t> target2{ REL_ID(54832, 00000), OFFSET_3(0x97, 0x0, 0x0) };  // TODO: AE and VR
			REL::safe_fill(target2.address(), REL::NOP, 0x4);                                           // Remove the call to log the GetFormFromFile error
			logger::info("GetFormFromFileHook hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("GetFormFromFileHook at offset {}", fmt::format("{:x}", target.offset()));
		}
	};

	//"Error: Unable to bind script MCMFlaskUtilsScript to FlaskUtilsMCM (7E007E63) because their base types do not match"
	struct BaseTypeMismatch
	{
		static inline auto newErrorMessage = "Script %s cannot be binded to %s because their base types do not match";
		// Improve BaseTypeMismatch to distinguish when script not loaded vs script type incorrect
		static bool thunk(const char* a_buffer, const std::size_t bufferCount, const char* a_format, const char* a_scriptName, const char* a_objectName)
		{
			if (a_scriptName == nullptr || a_objectName == nullptr || a_format == nullptr || a_buffer == nullptr) {
				return func(a_buffer, bufferCount, a_format, a_scriptName, a_objectName);
			}
			auto VM = RE::BSScript::Internal::VirtualMachine::GetSingleton();

			if (VM->TypeIsValid(a_scriptName)) {
				RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> info;
				VM->GetScriptObjectType1(a_scriptName, info);
				auto newScriptName = printScriptInfoInheritance(info.get());
				return func(a_buffer, bufferCount, a_format, newScriptName.c_str(), a_objectName);
			} else {
				const char* a_newFormat = "Script %s cannot be bound to %s because the script does not exist or is not currently loaded";
				return func(a_buffer, bufferCount, a_newFormat, a_scriptName, a_objectName);
			}
		}

		// expands script name to include inheritance
		static std::string printScriptInfoInheritance(RE::BSScript::ObjectTypeInfo* scriptInfo)
		{
			// example: CustomQuest (CustomQuest->Quest->Form)
			std::string newScriptName = std::string(scriptInfo->name.c_str()) + " (" + std::string(scriptInfo->name.c_str()) + "->";
			while ((scriptInfo = scriptInfo->GetParent()) != nullptr) {
				newScriptName = newScriptName + scriptInfo->name.c_str() + "->";
			};
			logger::info("{}", newScriptName);
			std::string result = newScriptName.substr(0, newScriptName.size() - 2) + ")";
			logger::info("{}", result);
			return result;
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL_ID(52730, 00000), OFFSET_3(0x3B2, 0x0, 0x0) };  // TODO: AE and VR
			write_thunk_call<BaseTypeMismatch>(target.address());

			logger::info("BaseTypeMismatch hooked at address {}", fmt::format("{:x}", target.address()));
			logger::info("BaseTypeMismatch at offset {}", fmt::format("{:x}", target.offset()));
		}
	};

	static inline void InstallHooks()
	{
		ValidationSignaturesHook::Install();
		GetFormFromFileHook::Install();
		BaseTypeMismatch::Install();
	}

}
