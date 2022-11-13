#pragma once
#pragma warning(2: 4235)
#include "RE/S/ScriptFunction.h"
#include "Settings.h"
#include <string.h>

#undef GetObject
namespace LoggerHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;

	struct ValidationSignaturesHook
	{
		static std::uint64_t thunk(RE::BSScript::IFunction** a_function, RE::BSScrapArray<RE::BSScript::Variable>* a_varArray, char* a_outString, std::int32_t a_bufferSize)
		{
			std::uint64_t result = func(a_function, a_varArray, a_outString, a_bufferSize);
			auto& function = *a_function;
			if (a_outString[0]) {
				// error occurred, let's improve it
				if (findStringIC(a_outString, "Type mismatch for argument")) {
					logger::info("Type mismatch");
					char mismatchedArgString[1072];
					std::string mismatchedFormat = "Function %s received incompatible arguments! Received types %s instead!";
					std::string types = ConvertArgsToTypesAsString(a_varArray);
					snprintf(mismatchedArgString, 1024, mismatchedFormat.c_str(),
						ConvertFunctionToString(function).c_str(), types.c_str());
					strcpy_s(a_outString, a_bufferSize, mismatchedArgString);
				} else if (findStringIC(a_outString, "Incorrect number of arguments passed")) {
					logger::info("Incorrect number of arguments passed");
					std::string incorrectArgSizeFormat = "Incorrect number of arguments passed to function %s. Expected %u, got %u instead!. ";
					std::string functionName = ConvertFunctionToString(*a_function);
					char incorrectArgString[1072];
					snprintf(incorrectArgString, 1024, incorrectArgSizeFormat.c_str(), functionName.c_str(), function->GetParamCount(), a_varArray->size());
					strcpy_s(a_outString, a_bufferSize, incorrectArgString);
				} else if (findStringIC(a_outString, "Passing NONE to non-object argument")) {
					logger::info("Passing NONE to non-object argument");
					char mismatchedArgString[1072];
					std::string mismatchedFormat = "Function %s received NONE to non-object argument! Received types %s instead!";
					std::string types = ConvertArgsToTypesAsString(a_varArray);
					snprintf(mismatchedArgString, 1024, mismatchedFormat.c_str(),
						ConvertFunctionToString(function).c_str(), types.c_str());
					strcpy_s(a_outString, a_bufferSize, mismatchedArgString);
				} else {
					// Should never happen
				}
			}
			return result;
		}

		// copied from stackoverflow. Just checks if string contained in other string no case sensitivity
		static bool findStringIC(const std::string& strHaystack, const std::string& strNeedle)
		{
			auto it = std::search(
				strHaystack.begin(), strHaystack.end(),
				strNeedle.begin(), strNeedle.end(),
				[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
			return (it != strHaystack.end());
		}

		static std::string ConvertArgsToTypesAsString(RE::BSScrapArray<RE::BSScript::Variable>* a_varArray)
		{
			std::string types = "(";
			for (std::uint32_t i = 0; i < a_varArray->size(); i++) {
				auto& variable = a_varArray->data()[i];
				if (variable.IsObject() && variable.GetObject() && variable.GetObject().get()) {
					types = types + variable.GetObject().get()->type.get()->GetName();
				} else if (variable.IsObjectArray() && variable.GetArray() && variable.GetArray().get()) {
					types = types + variable.GetArray().get()->type_info().TypeAsString();
				} else {
					types = types + a_varArray->data()[i].GetType().TypeAsString();
				}
				if ((i + 1) < a_varArray->size()) {
					types = types + ",";
				}
			}
			types = types + ")";

			return types;
		}
		static std::string ConvertFunctionToString(RE::BSScript::IFunction* function)
		{
			std::string params = "";
			for (std::uint32_t i = 0; i < function->GetParamCount(); i++) {
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
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(98130, 104853), REL::VariantOffset(0x63D, 0x650, 0x63D) };  // TODO: confirm AE and VR
			stl::write_thunk_call<ValidationSignaturesHook>(target.address());

			logger::info("ValidationSignaturesHook hooked at address {:x}", target.address());
			logger::info("ValidationSignaturesHook at offset {:x}", target.offset());
		}
	};

	// "Error: File \" % s \" does not exist or is not currently loaded."
	struct GetFormFromFileHook
	{
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(54832, 55465), REL::VariantOffset(0x7E, 0x7E, 0x81) };
			REL::safe_fill(target.address(), REL::NOP, 0x5);  // Remove the call to setup the log
			REL::Relocation<std::uintptr_t> target2{ RELOCATION_ID(54832, 55465), REL::VariantOffset(0x97, 0x97, 0x9A) };
			REL::safe_fill(target2.address(), REL::NOP, 0x4);  // Remove the call to log the GetFormFromFile error
			logger::info("GetFormFromFileHook hooked at address {:x}", target.address());
			logger::info("GetFormFromFileHook at offset {:x}", target.offset());
		}
	};

	// "Error: Unable to bind script MCMFlaskUtilsScript to FlaskUtilsMCM (7E007E63) because their base types do not match"
	struct BaseTypeMismatch
	{
		// Improve BaseTypeMismatch to distinguish when script not loaded vs script type incorrect
		static bool thunk(const char* a_buffer, const std::size_t bufferCount, const char* a_format, const char* a_scriptName, const char* a_objectName)
		{
			if (a_scriptName == nullptr || a_objectName == nullptr || a_format == nullptr || a_buffer == nullptr) {
				return func(a_buffer, bufferCount, a_format, a_scriptName, a_objectName);
			}
			auto VM = VM::GetSingleton();

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
			std::string result = newScriptName.substr(0, newScriptName.size() - 2) + ")";
			return result;
		}

		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(52730, 53574), REL::VariantOffset(0x3B2, 0x52C, 0x3B2) };
			stl::write_thunk_call<BaseTypeMismatch>(target.address());

			logger::info("BaseTypeMismatch hooked at address {:x}", target.address());
			logger::info("BaseTypeMismatch at offset {:x}", target.offset());
		}
	};

	// "Property %s on script %s attached to %s cannot be initialized because the script no longer contains that property"
	struct NoPropertyOnScriptHook
	{
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(52767, 53611), REL::VariantOffset(0x6FC, 0x6CB, 0x6FC) };
			REL::safe_fill(target.address(), REL::NOP, 0x3);  // erase the call to log the warning

			logger::info("NoPropertyOnScript installed at address {:x}", target.address());
			logger::info("NoPropertyOnScript installed at offset {:x}", target.offset());
		}
	};

	// "Cannot open store for class \"%s\", missing file?"
	struct DisableMissingScriptError
	{
		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(97831, 104575), REL::VariantOffset(0xC4, 0x123, 0xC4) };
			if (REL::Module::IsAE()) {
				std::byte newJump[] = { (std::byte)0xe9, (std::byte)0x8F, (std::byte)0xFF, (std::byte)0xFF, (std::byte)0xFF };
				REL::safe_fill(target.address(), REL::NOP, 0x5);
				REL::safe_write(target.address(), newJump, 0x5);
			} else {
				std::byte newJump[] = { (std::byte)0xe9, (std::byte)0xbc, (std::byte)0x00, (std::byte)0x00, (std::byte)0x00 };
				REL::safe_fill(target.address(), REL::NOP, 0x9);
				REL::safe_write(target.address(), newJump, 0x5);
			}

			logger::info("DisableMissingScriptError installed at address {:x}", target.address());
			logger::info("DisableMissingScriptError installed at offset {:x}", target.offset());
		}
	};

	static inline void InstallHooks()
	{
		auto settings = Settings::GetSingleton();
		if (settings->tweaks.improveValidateArgsErrors) {
			ValidationSignaturesHook::Install();
		}
		if (settings->tweaks.disableGetFormFromFile) {
			GetFormFromFileHook::Install();
		}
		if (settings->tweaks.improveBaseTypeMismatch) {
			BaseTypeMismatch::Install();
		}
		if (settings->tweaks.disableNoPropertyOnScript) {
			NoPropertyOnScriptHook::Install();
		}
		if (settings->tweaks.disableMissingScriptError) {
			DisableMissingScriptError::Install();
		}
	}
}
